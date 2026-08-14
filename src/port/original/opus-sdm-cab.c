#include "opus_x64_compat.h"
#include "opus_x64_heap.h"

#include <limits.h>
#include <stdint.h>
#include <string.h>

/* Native CAB storage for Microsoft's SDM 2.22 interface.
 *
 * The SDM implementation checked into this archive is 16-bit object code,
 * but its CAB layout and complete public contract are present in sdm.h and
 * sdmproc.h.  This file preserves that contract with flat movable handles.
 */

typedef uint16_t Word;
typedef void **CabHandle;

typedef struct CabHeader {
    Word simple_words;
    Word handle_words;
} CabHeader;

enum {
    kWordBytes = sizeof(Word),
    kCabMinWords = (sizeof(CabHeader) + sizeof(Word)) / sizeof(Word),
    kPointerWords = sizeof(void *) / sizeof(Word),
    kHandleBaseWords =
        (kCabMinWords + kPointerWords - 1) / kPointerWords * kPointerWords,
    kHandleBaseIag = kHandleBaseWords - kCabMinWords,
    kNinch = 0xffff
};

static unsigned char *cab_bytes(CabHandle cab) {
    return cab == NULL ? NULL : (unsigned char *)OpusDerefH(cab);
}

static CabHeader *cab_header(CabHandle cab) {
    return (CabHeader *)cab_bytes(cab);
}

static size_t cab_word_count(CabHandle cab) {
    return OpusCbOfH(cab) / kWordBytes;
}

static void ***handle_slot(CabHandle cab, Word argument_index) {
    CabHeader *header = cab_header(cab);
    if (header == NULL || argument_index < kHandleBaseIag ||
        argument_index >= kHandleBaseIag + header->handle_words ||
        (argument_index - kHandleBaseIag) % kPointerWords != 0) {
        return NULL;
    }
    return (void ***)(cab_bytes(cab) +
                      (kCabMinWords + argument_index) * kWordBytes);
}

static size_t bounded_strlen(const char *text, size_t byte_count) {
    size_t length = 0;
    while (length < byte_count && text[length] != '\0') {
        ++length;
    }
    return length;
}

static void clear_cab_data(CabHandle cab) {
    CabHeader *header = cab_header(cab);
    Word index;

    if (header == NULL) {
        return;
    }
    for (index = (Word)kHandleBaseIag;
         index < kHandleBaseIag + header->handle_words;
         index = (Word)(index + kPointerWords)) {
        void ***slot = handle_slot(cab, index);
        if (slot != NULL && *slot != NULL) {
            OpusFreeH(*slot);
            *slot = NULL;
        }
    }
}

static int initialize_cab(CabHandle cab, Word initializer) {
    unsigned char *bytes = cab_bytes(cab);
    size_t total_words;
    size_t handle_count;
    CabHeader *header;

    if (bytes == NULL) {
        return 0;
    }

    total_words = initializer & 0x00ffu;
    handle_count = initializer >> 8u;
    if (total_words < kCabMinWords || total_words > cab_word_count(cab) ||
        kHandleBaseIag + handle_count * kPointerWords >
            total_words - kCabMinWords) {
        return 0;
    }

    memset(bytes, 0, total_words * kWordBytes);
    header = (CabHeader *)bytes;
    header->simple_words = (Word)(total_words - kCabMinWords);
    header->handle_words = (Word)(handle_count * kPointerWords);
    return 1;
}

static int set_raw(CabHandle cab, const void *source, size_t byte_count,
                   Word argument_index) {
    void ***slot = handle_slot(cab, argument_index);
    void **replacement;

    if (slot == NULL || (source == NULL && byte_count != 0)) {
        return 0;
    }

    replacement = OpusHAllocateCb(byte_count);
    if (replacement == NULL) {
        return 0;
    }
    if (byte_count != 0) {
        memcpy(OpusDerefH(replacement), source, byte_count);
    }

    if (*slot != NULL) {
        OpusFreeH(*slot);
    }
    *slot = replacement;
    return 1;
}

static const unsigned char *raw_data(CabHandle cab, Word argument_index) {
    void ***slot = handle_slot(cab, argument_index);
    return slot == NULL || *slot == NULL
               ? NULL
               : (const unsigned char *)OpusDerefH(*slot);
}

static size_t raw_size(CabHandle cab, Word argument_index) {
    void ***slot = handle_slot(cab, argument_index);
    return slot == NULL || *slot == NULL ? 0 : OpusCbOfH(*slot);
}

static void copy_sz(CabHandle cab, char *destination, Word destination_bytes,
                    Word argument_index) {
    const char *source;
    size_t source_size;
    size_t source_length;
    size_t count;

    if (destination == NULL || destination_bytes == 0) {
        return;
    }
    source = (const char *)raw_data(cab, argument_index);
    if (source == NULL) {
        destination[0] = '\0';
        return;
    }
    source_size = raw_size(cab, argument_index);
    source_length = source_size == 0 ? 0 : bounded_strlen(source, source_size);
    count = source_length < (size_t)(destination_bytes - 1)
                ? source_length
                : (size_t)(destination_bytes - 1);
    memcpy(destination, source, count);
    destination[count] = '\0';
}

CabHandle hcabDlgCur = NULL;
uintptr_t wRefDlgCur = 0;

CabHandle HcabAlloc_sdm21(Word initializer) {
    size_t total_words = initializer & 0x00ffu;
    CabHandle cab;

    if (total_words < kCabMinWords) {
        return NULL;
    }
    cab = OpusHAllocateCb(total_words * kWordBytes);
    if (cab == NULL || !initialize_cab(cab, initializer)) {
        OpusFreeH(cab);
        return NULL;
    }
    return cab;
}

void InitCab_sdm21(CabHandle cab, Word initializer) {
    if (cab == NULL) {
        return;
    }
    clear_cab_data(cab);
    initialize_cab(cab, initializer);
}

void ReinitCab(CabHandle cab, Word initializer) {
    InitCab_sdm21(cab, initializer);
}

void FreeCabData(CabHandle cab) { clear_cab_data(cab); }

void FreeCab(CabHandle cab) {
    clear_cab_data(cab);
    OpusFreeH(cab);
}

void *PcabLockCab(CabHandle cab) { return cab_bytes(cab); }
void UnlockCab(CabHandle cab) { (void)cab; }

void NinchCab(CabHandle cab) {
    CabHeader *header = cab_header(cab);
    Word *words;
    Word simple_first;
    Word index;

    if (header == NULL) {
        return;
    }
    clear_cab_data(cab);
    words = (Word *)cab_bytes(cab);
    simple_first = header->handle_words == 0
                       ? 0
                       : (Word)(kHandleBaseIag + header->handle_words);
    for (index = simple_first; index < header->simple_words; ++index) {
        words[kCabMinWords + index] = kNinch;
    }
}

int FSetCabRgb(CabHandle cab, const char *source, Word byte_count,
               Word argument_index) {
    return set_raw(cab, source, byte_count, argument_index);
}

void GetCabRgb(CabHandle cab, char *destination, Word byte_count,
               Word argument_index) {
    size_t count;
    const unsigned char *source;

    if (destination == NULL || byte_count == 0) {
        return;
    }
    count = raw_size(cab, argument_index);
    if (count > byte_count) {
        count = byte_count;
    }
    source = raw_data(cab, argument_index);
    if (source != NULL && count != 0) {
        memcpy(destination, source, count);
    }
    if (count < byte_count) {
        memset(destination + count, 0, byte_count - count);
    }
}

int FSetCabSz(CabHandle cab, const char *source, Word argument_index) {
    if (source == NULL) {
        return 0;
    }
    return set_raw(cab, source, strlen(source) + 1, argument_index);
}

void GetCabSz(CabHandle cab, char *destination, Word destination_bytes,
              Word argument_index) {
    copy_sz(cab, destination, destination_bytes, argument_index);
}

int FSetCabSt(CabHandle cab, const unsigned char *source,
              Word argument_index) {
    size_t length;
    void ***slot;
    void **replacement;
    unsigned char *destination;

    if (source == NULL) {
        return 0;
    }
    length = source[0];
    slot = handle_slot(cab, argument_index);
    if (slot == NULL) {
        return 0;
    }
    replacement = OpusHAllocateCb(length + 1);
    if (replacement == NULL) {
        return 0;
    }
    destination = (unsigned char *)OpusDerefH(replacement);
    memcpy(destination, source + 1, length);
    destination[length] = 0;
    if (*slot != NULL) {
        OpusFreeH(*slot);
    }
    *slot = replacement;
    return 1;
}

void GetCabSt(CabHandle cab, unsigned char *destination,
              Word destination_bytes, Word argument_index) {
    const unsigned char *source;
    size_t size;
    size_t source_length;
    size_t count;

    if (destination == NULL || destination_bytes == 0) {
        return;
    }
    source = raw_data(cab, argument_index);
    size = raw_size(cab, argument_index);
    source_length = source == NULL || size == 0
                        ? 0
                        : bounded_strlen((const char *)source, size);
    count = source_length < (size_t)(destination_bytes - 1)
                ? source_length
                : (size_t)(destination_bytes - 1);
    destination[0] = (unsigned char)(count < UCHAR_MAX ? count : UCHAR_MAX);
    if (count != 0) {
        memcpy(destination + 1, source, count);
    }
}

void GetCabStz(CabHandle cab, unsigned char *destination,
               Word destination_bytes, Word argument_index) {
    size_t terminator;

    GetCabSt(cab, destination, destination_bytes, argument_index);
    if (destination != NULL && destination_bytes > 1) {
        terminator = (size_t)destination[0] + 1;
        if (terminator > (size_t)(destination_bytes - 1)) {
            terminator = (size_t)(destination_bytes - 1);
        }
        destination[terminator] = 0;
    }
}

CabHandle HcabDupeCab(CabHandle source) {
    const CabHeader *source_header = cab_header(source);
    size_t total_words;
    Word initializer;
    CabHandle duplicate;
    CabHeader *destination_header;
    const Word *source_words;
    Word *destination_words;
    Word simple_first;
    Word index;

    if (source_header == NULL) {
        return NULL;
    }
    total_words = cab_word_count(source);
    if (total_words > 0xffu ||
        source_header->handle_words % kPointerWords != 0) {
        return NULL;
    }
    initializer = (Word)(
        total_words |
        ((source_header->handle_words / kPointerWords) << 8u));
    duplicate = HcabAlloc_sdm21(initializer);
    if (duplicate == NULL) {
        return NULL;
    }

    destination_header = cab_header(duplicate);
    source_words = (const Word *)cab_bytes(source);
    destination_words = (Word *)cab_bytes(duplicate);
    simple_first = source_header->handle_words == 0
                       ? 0
                       : (Word)(kHandleBaseIag +
                                source_header->handle_words);
    for (index = simple_first; index < source_header->simple_words; ++index) {
        destination_words[kCabMinWords + index] =
            source_words[kCabMinWords + index];
    }

    for (index = (Word)kHandleBaseIag;
         index < kHandleBaseIag + source_header->handle_words;
         index = (Word)(index + kPointerWords)) {
        void ***source_slot = handle_slot(source, index);
        if (source_slot != NULL && *source_slot != NULL &&
            !set_raw(duplicate, OpusDerefH(*source_slot),
                     OpusCbOfH(*source_slot), index)) {
            FreeCab(duplicate);
            return NULL;
        }
    }
    destination_header->simple_words = source_header->simple_words;
    destination_header->handle_words = source_header->handle_words;
    return duplicate;
}
