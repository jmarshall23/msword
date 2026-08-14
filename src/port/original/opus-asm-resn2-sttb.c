#include "opus_x64_layout.h"
#include "opus_x64_compat.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

__declspec(dllimport) unsigned short __stdcall RtlCaptureStackBackTrace(
    unsigned long frames_to_skip, unsigned long frames_to_capture,
    void** back_trace, unsigned long* back_trace_hash);
__declspec(dllimport) HMODULE __stdcall GetModuleHandleW(LPCWSTR module_name);

/* AMD64 translations of the STTB access block in Opus/asm/resn2.asm. STTB
 * storage and offset-table layout continue to come from original word.h; only
 * segmented-pointer arithmetic is replaced here.
 */

void OpusX64Trace(const char* message);

static void trace_invalid_sttb(const char* reason, int index, int count,
                               size_t data_size, size_t table_size,
                               int offset) {
    char message[256] = {0};
    void* frames[12] = {0};
    unsigned short frame_count;
    uintptr_t module_base;
    unsigned short frame;

    snprintf(message, sizeof(message),
             "PstFromSttb invalid (%s): index=%d count=%d data=%zu "
             "table=%zu offset=%d",
             reason, index, count, data_size, table_size, offset);
    OpusX64Trace(message);
    frame_count = RtlCaptureStackBackTrace(1, 12, frames, NULL);
    module_base = (uintptr_t)GetModuleHandleW(NULL);
    for (frame = 0; frame < frame_count; ++frame) {
        uintptr_t address = (uintptr_t)frames[frame];
        if (address >= module_base) {
            snprintf(message, sizeof(message),
                     "PstFromSttb caller #%u WORD1+0x%llx", frame,
                     (unsigned long long)(address - module_base));
            OpusX64Trace(message);
        }
    }
}

static unsigned char* string_at(void** hsttb, int index) {
    int count;
    unsigned char* base;
    size_t data_size;
    size_t table_size;
    int offset;
    unsigned char* text;
    size_t bytes_left;

    if (hsttb == NULL || *hsttb == NULL) {
        trace_invalid_sttb("null handle", index, 0, 0, 0, 0);
        return NULL;
    }
    count = OpusSttbCount(*hsttb);
    if (index < 0 || index >= count) {
        trace_invalid_sttb("index", index, count, 0, 0, 0);
        return NULL;
    }
    base = (unsigned char*)OpusSttbData(*hsttb);
    data_size = OpusSttbDataSize(hsttb);
    table_size = (size_t)count * sizeof(int);
    if (base == NULL || data_size < table_size) {
        trace_invalid_sttb("storage", index, count, data_size, table_size, 0);
        return NULL;
    }
    offset = ((const int*)base)[index];
    if (offset < 0 || (size_t)offset < table_size ||
        (size_t)offset >= data_size) {
        trace_invalid_sttb("offset", index, count, data_size, table_size,
                           offset);
        return NULL;
    }
    text = base + offset;
    bytes_left = data_size - (size_t)offset;
    if (text[0] != 0xffu && (size_t)text[0] + 1u > bytes_left) {
        trace_invalid_sttb("length", index, count, data_size, table_size,
                           offset);
        return NULL;
    }
    return text;
}

unsigned char* HpstFromSttb(void** hsttb, int index) {
    return string_at(hsttb, index);
}

unsigned char* PstFromSttb(void** hsttb, int index) {
    return string_at(hsttb, index);
}

int FStcpEntryIsNull(void** hsttb, int index) {
    const unsigned char* text = string_at(hsttb, index);

    /* The native routine returned all bits set for true. */
    return text != NULL && text[0] == 0xffu ? -1 : 0;
}

int GetStFromSttb(void** hsttb, int index, unsigned char* destination) {
    const unsigned char* source = string_at(hsttb, index);

    if (source == NULL || destination == NULL) {
        if (destination != NULL) destination[0] = 0;
        return 0;
    }
    if (OpusSttbUsesStyleRules(*hsttb) && source[0] == 0xffu) {
        destination[0] = 0xffu;
        return 0;
    }
    memmove(destination, source, (size_t)source[0] + 1u);
    return 0;
}

int IbstFindSt(void** hsttb, const unsigned char* counted_text) {
    size_t byte_count;
    int count;
    int index;

    if (hsttb == NULL || *hsttb == NULL || counted_text == NULL) return -1;
    byte_count = (size_t)counted_text[0];
    count = OpusSttbCount(*hsttb);
    for (index = 0; index < count; ++index) {
        const unsigned char* candidate = string_at(hsttb, index);
        if (candidate != NULL && candidate[0] == counted_text[0] &&
            memcmp(candidate + 1, counted_text + 1, byte_count) == 0) {
            return index;
        }
    }
    return -1;
}

int OpusValidateSttb(void** hsttb) {
    int count;
    int index;

    if (hsttb == NULL || *hsttb == NULL) return 0;
    count = OpusSttbCount(*hsttb);
    if (count < 0) return 0;
    for (index = 0; index < count; ++index) {
        if (string_at(hsttb, index) == NULL) return 0;
    }
    return 1;
}
