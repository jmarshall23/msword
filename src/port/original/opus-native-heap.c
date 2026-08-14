#include "opus-native-compat.h"
#include "opus-native-heap.h"

#include <windows.h>

#include <limits.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef struct NativeHandle {
    void *data;
    size_t size;
} NativeHandle;

typedef struct NativeSegment {
    void *data;
    size_t size;
} NativeSegment;

typedef char native_handle_data_offset[(offsetof(NativeHandle, data) == 0) ? 1 : -1];

enum {
    kPrcTokenNil = 0x3fff,
    kPrcTokenMac = kPrcTokenNil - 1,
    kCompactHandleMac = 0x3fff,
    kNativeHandleSlots = 131071,
    kSegmentSlots = 65536,
    kGuardBytes = 64,
    kMaxAllocationBytes = 256u * 1024u * 1024u,
    kGuardValue = 0xa5
};

static SRWLOCK prc_registry_lock = SRWLOCK_INIT;
static SRWLOCK compact_handle_lock = SRWLOCK_INIT;
static SRWLOCK native_handle_lock = SRWLOCK_INIT;
static SRWLOCK segment_registry_lock = SRWLOCK_INIT;
static void **prc_handles[kPrcTokenNil];
static void **compact_handles[kCompactHandleMac + 1];
static void **native_handles[kNativeHandleSlots];
static NativeSegment native_segments[kSegmentSlots];
static _Atomic size_t heap_bytes_used;

static HANDLE process_heap(void) { return GetProcessHeap(); }

static size_t min_size(size_t first, size_t second) {
    return first < second ? first : second;
}

static size_t payload_size(const size_t logical_size) {
    return logical_size == 0 ? 1 : logical_size;
}

static bool guarded_size(const size_t logical_size, size_t *result) {
    if (logical_size > kMaxAllocationBytes) return false;
    const size_t payload = payload_size(logical_size);
    if (payload > (size_t)-1 - kGuardBytes) {
        return false;
    }
    *result = payload + kGuardBytes;
    return true;
}

static bool valid_native_handle(void **opaque_handle) {
    if (opaque_handle == NULL) return false;
    void **const tombstone = (void **)(uintptr_t)1;
    const size_t start = ((uintptr_t)opaque_handle >> 4) % kNativeHandleSlots;
    AcquireSRWLockShared(&native_handle_lock);
    bool valid = false;
    for (size_t probe = 0; probe < kNativeHandleSlots; ++probe) {
        void **const candidate =
            native_handles[(start + probe) % kNativeHandleSlots];
        if (candidate == opaque_handle) {
            valid = true;
            break;
        }
        if (candidate == NULL) break;
        if (candidate == tombstone) continue;
    }
    ReleaseSRWLockShared(&native_handle_lock);
    return valid;
}

static bool register_native_handle(void **opaque_handle) {
    void **const tombstone = (void **)(uintptr_t)1;
    const size_t start = ((uintptr_t)opaque_handle >> 4) % kNativeHandleSlots;
    AcquireSRWLockExclusive(&native_handle_lock);
    size_t first_tombstone = kNativeHandleSlots;
    for (size_t probe = 0; probe < kNativeHandleSlots; ++probe) {
        const size_t index = (start + probe) % kNativeHandleSlots;
        if (native_handles[index] == opaque_handle) {
            ReleaseSRWLockExclusive(&native_handle_lock);
            return true;
        }
        if (native_handles[index] == tombstone &&
            first_tombstone == kNativeHandleSlots) first_tombstone = index;
        if (native_handles[index] == NULL) {
            native_handles[first_tombstone == kNativeHandleSlots ?
                               index : first_tombstone] = opaque_handle;
            ReleaseSRWLockExclusive(&native_handle_lock);
            return true;
        }
    }
    if (first_tombstone != kNativeHandleSlots) {
        native_handles[first_tombstone] = opaque_handle;
        ReleaseSRWLockExclusive(&native_handle_lock);
        return true;
    }
    ReleaseSRWLockExclusive(&native_handle_lock);
    return false;
}

static bool unregister_native_handle(void **opaque_handle) {
    void **const tombstone = (void **)(uintptr_t)1;
    const size_t start = ((uintptr_t)opaque_handle >> 4) % kNativeHandleSlots;
    AcquireSRWLockExclusive(&native_handle_lock);
    for (size_t probe = 0; probe < kNativeHandleSlots; ++probe) {
        const size_t index = (start + probe) % kNativeHandleSlots;
        if (native_handles[index] == opaque_handle) {
            native_handles[index] = tombstone;
            ReleaseSRWLockExclusive(&native_handle_lock);
            return true;
        }
        if (native_handles[index] == NULL) break;
    }
    ReleaseSRWLockExclusive(&native_handle_lock);
    return false;
}

static void set_guard(void *const allocation, const size_t logical_size) {
    memset((unsigned char *)allocation + payload_size(logical_size),
           kGuardValue, kGuardBytes);
}

static bool guard_is_valid(const void *const allocation,
                           const size_t logical_size) {
    const unsigned char *guard =
        (const unsigned char *)allocation + payload_size(logical_size);
    for (size_t index = 0; index < kGuardBytes; ++index) {
        if (guard[index] != kGuardValue) {
            return false;
        }
    }
    return true;
}

static void report_guard_failure(void) {
    const DWORD kOpusHeapGuardFailure = 0xe0421001u;
    RaiseException(kOpusHeapGuardFailure, EXCEPTION_NONCONTINUABLE, 0,
                   NULL);
    TerminateProcess(GetCurrentProcess(), kOpusHeapGuardFailure);
    __assume(false);
}

static void unregister_prc_handle(void **handle) {
    AcquireSRWLockExclusive(&prc_registry_lock);
    for (int token = 1; token <= kPrcTokenMac; ++token) {
        if (prc_handles[token] == handle) {
            prc_handles[token] = NULL;
            break;
        }
    }
    ReleaseSRWLockExclusive(&prc_registry_lock);
}

static void unregister_compact_handle(void **handle) {
    AcquireSRWLockExclusive(&compact_handle_lock);
    for (int token = 1; token <= kCompactHandleMac; ++token) {
        if (compact_handles[token] == handle) {
            compact_handles[token] = NULL;
            break;
        }
    }
    ReleaseSRWLockExclusive(&compact_handle_lock);
}

SB sbMac = 0;
SB _sbCur = sbDds;
SB sbCur = sbDds;
WORD mpsbps[kSegmentSlots] = {0};

int FInitSegTable(const SB requested_max) {
    sbMac = requested_max < (SB)kSegmentSlots ? requested_max : (SB)kSegmentSlots;
    return 1;
}

HP OpusHpOfSbIbImpl(const SB segment, const uintptr_t offset) {
    if (segment == sbDds) {
        return (HP)offset;
    }
    if (segment >= kSegmentSlots) {
        return (HP)((uintptr_t)segment + offset);
    }
    AcquireSRWLockShared(&segment_registry_lock);
    void *base = native_segments[(size_t)segment].data;
    const size_t size = native_segments[(size_t)segment].size;
    ReleaseSRWLockShared(&segment_registry_lock);
    return base == NULL || offset > size ? NULL : (HP)((uintptr_t)base + offset);
}

SB OpusSbOfHp(const void *pointer) {
    if (pointer == NULL) {
        return 0;
    }
    const uintptr_t address = (uintptr_t)pointer;
    AcquireSRWLockShared(&segment_registry_lock);
    for (size_t index = 2; index < kSegmentSlots; ++index) {
        const NativeSegment *segment = &native_segments[index];
        const uintptr_t base = (uintptr_t)segment->data;
        if (base != 0 && address >= base && address < base + segment->size) {
            ReleaseSRWLockShared(&segment_registry_lock);
            return (SB)index;
        }
    }
    ReleaseSRWLockShared(&segment_registry_lock);
    return sbDds;
}

/* Some original modules declared the native helper as a routine instead of
 * including sbmgr.h's macro spelling. */
SB SbOfHp(const void *pointer) { return OpusSbOfHp(pointer); }

IB OpusIbOfHp(const void *pointer) {
    if (pointer == NULL) {
        return 0;
    }
    const uintptr_t address = (uintptr_t)pointer;
    AcquireSRWLockShared(&segment_registry_lock);
    for (size_t index = 2; index < kSegmentSlots; ++index) {
        const NativeSegment *segment = &native_segments[index];
        const uintptr_t base = (uintptr_t)segment->data;
        if (base != 0 && address >= base && address < base + segment->size) {
            ReleaseSRWLockShared(&segment_registry_lock);
            return (IB)(address - base);
        }
    }
    ReleaseSRWLockShared(&segment_registry_lock);
    return (IB)address;
}

unsigned int CbAllocSb(const SB segment, const unsigned int byte_count,
                       int flags) {
    (void)flags;
    if (segment == sbDds) {
        return byte_count;
    }
    if (segment < 2 || segment >= kSegmentSlots) {
        return 0;
    }
    size_t allocation_size = 0;
    if (!guarded_size(byte_count, &allocation_size)) {
        return 0;
    }
    void *allocation =
        HeapAlloc(process_heap(), HEAP_ZERO_MEMORY, allocation_size);
    if (allocation == NULL) {
        return 0;
    }
    set_guard(allocation, byte_count);
    AcquireSRWLockExclusive(&segment_registry_lock);
    NativeSegment *slot = &native_segments[(size_t)segment];
    if (slot->data != NULL) {
        ReleaseSRWLockExclusive(&segment_registry_lock);
        HeapFree(process_heap(), 0, allocation);
        return 0;
    }
    slot->data = allocation;
    slot->size = byte_count;
    mpsbps[(size_t)segment] = 1;
    ReleaseSRWLockExclusive(&segment_registry_lock);
    atomic_fetch_add_explicit(&heap_bytes_used, byte_count, memory_order_relaxed);
    return byte_count;
}

unsigned int CbReallocSb(const SB segment, const unsigned int byte_count,
                         int flags) {
    (void)flags;
    if (segment == sbDds) {
        return byte_count;
    }
    if (segment < 2 || segment >= kSegmentSlots) {
        return 0;
    }
    AcquireSRWLockExclusive(&segment_registry_lock);
    NativeSegment *slot = &native_segments[(size_t)segment];
    if (slot->data == NULL) {
        ReleaseSRWLockExclusive(&segment_registry_lock);
        return CbAllocSb(segment, byte_count, 0);
    }
    const size_t old_size = slot->size;
    if (!guard_is_valid(slot->data, old_size)) {
        ReleaseSRWLockExclusive(&segment_registry_lock);
        report_guard_failure();
    }
    size_t allocation_size = 0;
    if (!guarded_size(byte_count, &allocation_size)) {
        ReleaseSRWLockExclusive(&segment_registry_lock);
        return 0;
    }
    void *resized =
        HeapReAlloc(process_heap(), HEAP_ZERO_MEMORY, slot->data,
                    allocation_size);
    if (resized == NULL) {
        ReleaseSRWLockExclusive(&segment_registry_lock);
        return 0;
    }
    set_guard(resized, byte_count);
    slot->data = resized;
    slot->size = byte_count;
    ReleaseSRWLockExclusive(&segment_registry_lock);
    if (byte_count >= old_size) {
        atomic_fetch_add_explicit(&heap_bytes_used, byte_count - old_size,
                                  memory_order_relaxed);
    } else {
        atomic_fetch_sub_explicit(&heap_bytes_used, old_size - byte_count,
                                  memory_order_relaxed);
    }
    return byte_count;
}

void FreeSb(const SB segment) {
    if (segment < 2 || segment >= kSegmentSlots) {
        return;
    }
    AcquireSRWLockExclusive(&segment_registry_lock);
    NativeSegment *slot = &native_segments[(size_t)segment];
    void *allocation = slot->data;
    const size_t byte_count = slot->size;
    slot->data = NULL;
    slot->size = 0;
    mpsbps[(size_t)segment] = 0;
    ReleaseSRWLockExclusive(&segment_registry_lock);
    if (allocation != NULL) {
        if (!guard_is_valid(allocation, byte_count)) {
            report_guard_failure();
        }
        atomic_fetch_sub_explicit(&heap_bytes_used, byte_count,
                                  memory_order_relaxed);
        HeapFree(process_heap(), 0, allocation);
    }
}

unsigned int CbSizeSb(const SB segment) {
    if (segment == sbDds || segment >= kSegmentSlots) {
        return 0;
    }
    AcquireSRWLockShared(&segment_registry_lock);
    const size_t size = native_segments[(size_t)segment].size;
    ReleaseSRWLockShared(&segment_registry_lock);
    return (unsigned int)min_size(size, UINT_MAX);
}

SB SbAllocEmmCb(const unsigned int byte_count) {
    const size_t limit = min_size((size_t)sbMac, kSegmentSlots);
    for (size_t index = 2; index < limit; ++index) {
        AcquireSRWLockShared(&segment_registry_lock);
        const bool available = native_segments[index].data == NULL;
        ReleaseSRWLockShared(&segment_registry_lock);
        if (available && CbAllocSb((SB)index, byte_count, 0) != 0) {
            return (SB)index;
        }
    }
    return 0;
}

void FreeEmmSb(const SB segment) { FreeSb(segment); }
long CbFreeEmm(void) { return 0; }
void EndEmm(void) {}
int ReloadSb(const SB segment) {
    _sbCur = sbCur = segment;
    return segment != 0;
}

SB SbScanNext(int scan_state) {
    (void)scan_state;
    const size_t limit = min_size((size_t)sbMac, kSegmentSlots);
    for (size_t index = 2; index < limit; ++index) {
        if (mpsbps[index] == 0) {
            return (SB)index;
        }
    }
    return 0;
}

void ResetSbCur(void) { _sbCur = sbCur = sbDds; }

void *LpConvHp(void *pointer) { return pointer; }

int FAssureHcb(void ***handle_address, const int byte_count_needed,
               int *byte_count, int *byte_capacity) {
    if (handle_address == NULL || byte_count_needed < 0 ||
        byte_capacity == NULL || *byte_capacity < 0) {
        return 0;
    }
    if (byte_count != NULL) {
        *byte_count = byte_count_needed;
    }
    if (*handle_address != NULL && byte_count_needed <= *byte_capacity) {
        return 1;
    }
    const int doubled = *byte_capacity > INT_MAX / 2 ? INT_MAX :
        (*byte_capacity > 0 ? *byte_capacity * 2 : 16);
    const int new_capacity =
        byte_count_needed > doubled ? byte_count_needed : doubled;
    if (*handle_address == NULL) {
        *handle_address = OpusHAllocateCb((size_t)new_capacity);
        if (*handle_address == NULL) {
            return 0;
        }
    } else if (!OpusFChngSizeHCb(*handle_address,
                                 (size_t)new_capacity, 0)) {
        return 0;
    }
    *byte_capacity = new_capacity;
    return 1;
}

unsigned HcCompactH(void **handle) {
    if (handle == NULL) {
        return 0;
    }
    AcquireSRWLockExclusive(&compact_handle_lock);
    for (int token = 1; token <= kCompactHandleMac; ++token) {
        if (compact_handles[token] == handle) {
            ReleaseSRWLockExclusive(&compact_handle_lock);
            return (unsigned)token;
        }
    }
    for (int free_token = 1; free_token <= kCompactHandleMac; ++free_token) {
        if (compact_handles[free_token] == NULL) {
            compact_handles[free_token] = handle;
            ReleaseSRWLockExclusive(&compact_handle_lock);
            return (unsigned)free_token;
        }
    }
    ReleaseSRWLockExclusive(&compact_handle_lock);
    return 0;
}

void **HExpandHc(const unsigned compact_handle) {
    if (compact_handle == 0 || compact_handle > kCompactHandleMac) {
        return NULL;
    }
    AcquireSRWLockShared(&compact_handle_lock);
    void **const handle = compact_handles[compact_handle];
    ReleaseSRWLockShared(&compact_handle_lock);
    return handle;
}

void **OpusHAllocateCb(const size_t byte_count) {
    NativeHandle *handle = (NativeHandle *)
        HeapAlloc(process_heap(), HEAP_ZERO_MEMORY, sizeof(NativeHandle));
    if (handle == NULL) {
        return NULL;
    }
    size_t allocation_size = 0;
    if (!guarded_size(byte_count, &allocation_size)) {
        HeapFree(process_heap(), 0, handle);
        return NULL;
    }
    handle->data = HeapAlloc(process_heap(), 0, allocation_size);
    if (handle->data == NULL) {
        HeapFree(process_heap(), 0, handle);
        return NULL;
    }
    set_guard(handle->data, byte_count);
    handle->size = byte_count;
    if (!register_native_handle((void **)handle)) {
        HeapFree(process_heap(), 0, handle->data);
        HeapFree(process_heap(), 0, handle);
        return NULL;
    }
    atomic_fetch_add_explicit(&heap_bytes_used, byte_count, memory_order_relaxed);
    return (void **)handle;
}

void OpusFreeH(void **opaque_handle) {
    if (opaque_handle == NULL) {
        return;
    }
    if (!unregister_native_handle(opaque_handle)) {
        report_guard_failure();
    }
    unregister_prc_handle(opaque_handle);
    unregister_compact_handle(opaque_handle);
    NativeHandle *handle = (NativeHandle *)opaque_handle;
    if (handle->data != NULL &&
        !guard_is_valid(handle->data, handle->size)) {
        report_guard_failure();
    }
    atomic_fetch_sub_explicit(&heap_bytes_used, handle->size,
                              memory_order_relaxed);
    if (handle->data != NULL) {
        HeapFree(process_heap(), 0, handle->data);
    }
    HeapFree(process_heap(), 0, handle);
}

void OpusFreePh(void ***handle_address) {
    if (handle_address == NULL || *handle_address == NULL) {
        return;
    }
    OpusFreeH(*handle_address);
    *handle_address = NULL;
}

int OpusFChngSizeHCb(void **opaque_handle, const size_t byte_count,
                     const int allow_shrink) {
    if (opaque_handle == NULL) {
        return 0;
    }
    if (!valid_native_handle(opaque_handle)) {
        report_guard_failure();
    }
    NativeHandle *handle = (NativeHandle *)opaque_handle;
    if (!allow_shrink && byte_count <= handle->size) {
        return 1;
    }
    if (handle->data != NULL &&
        !guard_is_valid(handle->data, handle->size)) {
        report_guard_failure();
    }
    size_t allocation_size = 0;
    if (!guarded_size(byte_count, &allocation_size)) {
        return 0;
    }
    void *resized = HeapReAlloc(process_heap(), 0, handle->data, allocation_size);
    if (resized == NULL) {
        return 0;
    }
    const size_t old_size = handle->size;
    set_guard(resized, byte_count);
    handle->data = resized;
    handle->size = byte_count;
    if (byte_count >= old_size) {
        atomic_fetch_add_explicit(&heap_bytes_used, byte_count - old_size,
                                  memory_order_relaxed);
    } else {
        atomic_fetch_sub_explicit(&heap_bytes_used, old_size - byte_count,
                                  memory_order_relaxed);
    }
    return 1;
}

int OpusFChngSizePhqLcb(void ***handle_address, const size_t byte_count) {
    return handle_address != NULL &&
           OpusFChngSizeHCb(*handle_address, byte_count, 1);
}

size_t OpusCbOfH(void **opaque_handle) {
    return !valid_native_handle(opaque_handle)
               ? 0
               : ((NativeHandle *)opaque_handle)->size;
}

size_t OpusHeapBytesUsed(void) {
    return atomic_load_explicit(&heap_bytes_used, memory_order_relaxed);
}

void *OpusDerefH(void **opaque_handle) {
    return !valid_native_handle(opaque_handle)
               ? NULL
               : ((NativeHandle *)opaque_handle)->data;
}

void *OpusHpAlloc(const size_t byte_count) {
    if (byte_count > kMaxAllocationBytes) return NULL;
    void *allocation =
        HeapAlloc(process_heap(), 0, byte_count == 0 ? 1 : byte_count);
    if (allocation != NULL) {
        const SIZE_T size = HeapSize(process_heap(), 0, allocation);
        if (size != (SIZE_T)-1) {
            atomic_fetch_add_explicit(&heap_bytes_used, size,
                                      memory_order_relaxed);
        }
    }
    return allocation;
}

void OpusFreeHp(void *pointer) {
    if (pointer != NULL) {
        const SIZE_T size = HeapSize(process_heap(), 0, pointer);
        if (size == (SIZE_T)-1) report_guard_failure();
        atomic_fetch_sub_explicit(&heap_bytes_used, size, memory_order_relaxed);
        HeapFree(process_heap(), 0, pointer);
    }
}

int OpusPrcTokenFromHandle(void **handle) {
    if (handle == NULL) {
        return kPrcTokenNil;
    }

    int free_token = 0;
    AcquireSRWLockExclusive(&prc_registry_lock);
    for (int token = 1; token <= kPrcTokenMac; ++token) {
        if (prc_handles[token] == handle) {
            ReleaseSRWLockExclusive(&prc_registry_lock);
            return token;
        }
        if (free_token == 0 && prc_handles[token] == NULL) {
            free_token = token;
        }
    }
    if (free_token != 0) {
        prc_handles[free_token] = handle;
    }
    ReleaseSRWLockExclusive(&prc_registry_lock);
    return free_token == 0 ? kPrcTokenNil : free_token;
}

void **OpusPrcHandleFromToken(const int token) {
    if (token <= 0 || token > kPrcTokenMac) {
        return NULL;
    }
    AcquireSRWLockShared(&prc_registry_lock);
    void **const handle = prc_handles[token];
    ReleaseSRWLockShared(&prc_registry_lock);
    return handle;
}

/* Flat-memory implementations of the LMEM public API used by the original
 * interpreter and SDM-facing code.  SB is intentionally ignored: every
 * native handle is addressable in the process' single 64-bit address space. */
void **PpvAllocCb(SB segment, WORD byte_count) {
    (void)segment;
    return OpusHAllocateCb(byte_count);
}

void **PpvAllocCbWW(SB segment, int byte_count) {
    (void)segment;
    return byte_count < 0 ? NULL : OpusHAllocateCb((size_t)byte_count);
}

int FReallocPpv(SB segment, void **handle, WORD byte_count) {
    (void)segment;
    return OpusFChngSizeHCb(handle, byte_count, 1);
}

void FreePpv(SB segment, void **handle) {
    (void)segment;
    OpusFreeH(handle);
}

WORD CbSizePpv(SB segment, void **handle) {
    (void)segment;
    const size_t size = OpusCbOfH(handle);
    return (WORD)min_size(size, USHRT_MAX);
}

void *PvAllocFixedCb(SB segment, WORD byte_count) {
    (void)segment;
    return OpusHpAlloc(byte_count);
}

WORD CbSizeFixedPv(SB segment, void *pointer) {
    (void)segment;
    if (pointer == NULL) {
        return 0;
    }
    const SIZE_T size = HeapSize(process_heap(), 0, pointer);
    return size == (SIZE_T)-1 ? 0 : (WORD)min_size(size, USHRT_MAX);
}

void CreateHeap(SB segment) { (void)segment; }

WORD CbCompactHeap(SB segment, WORD requested) {
    (void)segment;
    HeapCompact(process_heap(), 0);
    return requested;
}

WORD CbAvailHeap(SB segment) {
    (void)segment;
    /* LMEM callers use this as a 16-bit capacity hint, not as an allocation
     * guarantee. Native allocation failure remains authoritative. */
    return USHRT_MAX;
}
