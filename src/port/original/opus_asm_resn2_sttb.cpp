#include "opus_x64_layout.h"
#include "opus_x64_compat.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>

extern "C" __declspec(dllimport) unsigned short __stdcall
RtlCaptureStackBackTrace(unsigned long frames_to_skip,
                         unsigned long frames_to_capture,
                         void** back_trace,
                         unsigned long* back_trace_hash);
extern "C" __declspec(dllimport) HMODULE __stdcall
GetModuleHandleW(LPCWSTR module_name);

/* AMD64 translations of the STTB access block in Opus/asm/resn2.asm. STTB
 * storage and offset-table layout continue to come from original word.h; only
 * segmented-pointer arithmetic is replaced here.
 */

namespace {

extern "C" void OpusX64Trace(const char* message);

void TraceInvalidSttb(const char* const reason, const int index,
                      const int count, const std::size_t data_size,
                      const std::size_t table_size,
                      const int offset) {
    char message[256] = {};
    std::snprintf(message, sizeof(message),
                  "PstFromSttb invalid (%s): index=%d count=%d data=%zu "
                  "table=%zu offset=%d",
                  reason, index, count, data_size, table_size, offset);
    OpusX64Trace(message);
    void* frames[12] = {};
    const unsigned short frame_count =
        RtlCaptureStackBackTrace(1, 12, frames, nullptr);
    const auto module_base = reinterpret_cast<std::uintptr_t>(
        GetModuleHandleW(nullptr));
    for (unsigned short frame = 0; frame < frame_count; ++frame) {
        const auto address = reinterpret_cast<std::uintptr_t>(frames[frame]);
        if (address >= module_base) {
            std::snprintf(message, sizeof(message),
                          "PstFromSttb caller #%u WORD1+0x%llx", frame,
                          static_cast<unsigned long long>(address - module_base));
            OpusX64Trace(message);
        }
    }
}

unsigned char* StringAt(void** hsttb, const int index) {
    if (hsttb == nullptr || *hsttb == nullptr) {
        TraceInvalidSttb("null handle", index, 0, 0, 0, 0);
        return nullptr;
    }
    const int count = OpusSttbCount(*hsttb);
    if (index < 0 || index >= count) {
        TraceInvalidSttb("index", index, count, 0, 0, 0);
        return nullptr;
    }
    auto* const base = static_cast<unsigned char*>(OpusSttbData(*hsttb));
    const std::size_t data_size = OpusSttbDataSize(hsttb);
    const std::size_t table_size =
        static_cast<std::size_t>(count) * sizeof(int);
    if (base == nullptr || data_size < table_size) {
        TraceInvalidSttb("storage", index, count, data_size, table_size, 0);
        return nullptr;
    }
    const int offset = reinterpret_cast<const int*>(base)[index];
    if (offset < 0 || static_cast<std::size_t>(offset) < table_size ||
        static_cast<std::size_t>(offset) >= data_size) {
        TraceInvalidSttb("offset", index, count, data_size, table_size,
                         offset);
        return nullptr;
    }
    unsigned char* const text = base + offset;
    const std::size_t bytes_left = data_size - static_cast<std::size_t>(offset);
    if (text[0] != 0xffu &&
        static_cast<std::size_t>(text[0]) + 1u > bytes_left) {
        TraceInvalidSttb("length", index, count, data_size, table_size,
                         offset);
        return nullptr;
    }
    return text;
}

}  // namespace

extern "C" unsigned char* HpstFromSttb(void** hsttb, const int index) {
    return StringAt(hsttb, index);
}

extern "C" unsigned char* PstFromSttb(void** hsttb, const int index) {
    return StringAt(hsttb, index);
}

extern "C" int FStcpEntryIsNull(void** hsttb, const int index) {
    const unsigned char* const text = StringAt(hsttb, index);
    // The native routine returned all bits set for true.
    return text != nullptr && text[0] == 0xffu ? -1 : 0;
}

extern "C" int GetStFromSttb(void** hsttb, const int index,
                              unsigned char* destination) {
    const unsigned char* const source = StringAt(hsttb, index);
    if (source == nullptr || destination == nullptr) {
        if (destination != nullptr) {
            destination[0] = 0;
        }
        return 0;
    }
    if (OpusSttbUsesStyleRules(*hsttb) && source[0] == 0xffu) {
        destination[0] = 0xffu;
        return 0;
    }
    std::memmove(destination, source,
                 static_cast<std::size_t>(source[0]) + 1u);
    return 0;
}

extern "C" int IbstFindSt(void** hsttb, const unsigned char* countedText) {
    if (hsttb == nullptr || *hsttb == nullptr || countedText == nullptr) {
        return -1;
    }
    const std::size_t byteCount = static_cast<std::size_t>(countedText[0]);
    const int count = OpusSttbCount(*hsttb);
    for (int index = 0; index < count; ++index) {
        const unsigned char* const candidate = StringAt(hsttb, index);
        if (candidate != nullptr && candidate[0] == countedText[0] &&
            std::memcmp(candidate + 1, countedText + 1, byteCount) == 0) {
            return index;
        }
    }
    return -1;
}

extern "C" int OpusValidateSttb(void** hsttb) {
    if (hsttb == nullptr || *hsttb == nullptr) {
        return 0;
    }
    const int count = OpusSttbCount(*hsttb);
    if (count < 0) {
        return 0;
    }
    for (int index = 0; index < count; ++index) {
        if (StringAt(hsttb, index) == nullptr) {
            return 0;
        }
    }
    return 1;
}
