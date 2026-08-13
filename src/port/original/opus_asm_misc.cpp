#include "opus_x64_compat.h"
#include "opus_x64_heap.h"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <type_traits>
#include <utility>

namespace {

template <std::size_t... Index>
long invoke_macro(void* procedure, const int* arguments,
                  std::index_sequence<Index...>) {
    using Procedure =
        long(__cdecl*)(std::conditional_t<true, int,
            std::integral_constant<std::size_t, Index>>...);
    return reinterpret_cast<Procedure>(procedure)(arguments[Index]...);
}

struct NativePlaybackEvent {
    unsigned int message;
    unsigned int virtual_key;
};

struct NativePlaybackQueue {
    int current;
    int count;
    NativePlaybackEvent events[1];
};

}  // namespace

extern "C" {

int N_WCompSzSrt(char* first, char* second, int case_sensitive);
extern HANDLE* lphevtHead;
extern HANDLE* lphrgbKeyState;

char* index(const char* text, const int character) {
    if (text == nullptr) {
        return nullptr;
    }
    const char* cursor = text;
    const unsigned char wanted = static_cast<unsigned char>(character);
    for (;;) {
        if (static_cast<unsigned char>(*cursor) == wanted) {
            return const_cast<char*>(cursor);
        }
        if (*cursor++ == '\0') {
            return nullptr;
        }
    }
}

int LbcCmpLbox(unsigned int, unsigned char** first,
               unsigned char** second) {
    if (first == nullptr || second == nullptr || *first == nullptr ||
        *second == nullptr) {
        return 0;
    }
    const int comparison = N_WCompSzSrt(
        reinterpret_cast<char*>(*first + 1),
        reinterpret_cast<char*>(*second + 1), 0);
    if (comparison == 0) return 0;   // lbcEq
    if (comparison > 0) return 3;    // lbcGt
    if (comparison == -2) return 2;  // lbcLt
    return 1;                        // lbcPrefix
}

long LPushMacroArgs(void* procedure, const int* arguments,
                    const int argument_count) {
    if (procedure == nullptr || argument_count < 0 || argument_count > 16 ||
        (argument_count != 0 && arguments == nullptr)) {
        return 0;
    }
    switch (argument_count) {
    case 0: return invoke_macro(procedure, arguments, std::index_sequence<>{});
    case 1: return invoke_macro(procedure, arguments, std::make_index_sequence<1>{});
    case 2: return invoke_macro(procedure, arguments, std::make_index_sequence<2>{});
    case 3: return invoke_macro(procedure, arguments, std::make_index_sequence<3>{});
    case 4: return invoke_macro(procedure, arguments, std::make_index_sequence<4>{});
    case 5: return invoke_macro(procedure, arguments, std::make_index_sequence<5>{});
    case 6: return invoke_macro(procedure, arguments, std::make_index_sequence<6>{});
    case 7: return invoke_macro(procedure, arguments, std::make_index_sequence<7>{});
    case 8: return invoke_macro(procedure, arguments, std::make_index_sequence<8>{});
    case 9: return invoke_macro(procedure, arguments, std::make_index_sequence<9>{});
    case 10: return invoke_macro(procedure, arguments, std::make_index_sequence<10>{});
    case 11: return invoke_macro(procedure, arguments, std::make_index_sequence<11>{});
    case 12: return invoke_macro(procedure, arguments, std::make_index_sequence<12>{});
    case 13: return invoke_macro(procedure, arguments, std::make_index_sequence<13>{});
    case 14: return invoke_macro(procedure, arguments, std::make_index_sequence<14>{});
    case 15: return invoke_macro(procedure, arguments, std::make_index_sequence<15>{});
    case 16: return invoke_macro(procedure, arguments, std::make_index_sequence<16>{});
    default: return 0;
    }
}

long MemUsed(const int memory_type) {
    if ((memory_type & 1) == 0) {
        return 0;  // no expanded-memory arena exists in the flat port
    }
    const std::size_t used = OpusHeapBytesUsed();
    return static_cast<long>((std::min)(
        used, static_cast<std::size_t>((std::numeric_limits<long>::max)())));
}

LRESULT CALLBACK PlaybackHook(const int code, const WPARAM parameter,
                              const LPARAM message_pointer) {
    if (code != HC_GETNEXT && code != HC_SKIP) {
        return CallNextHookEx(nullptr, code, parameter, message_pointer);
    }
    if (lphevtHead == nullptr || *lphevtHead == nullptr) {
        return 0;
    }

    HANDLE queue_handle = *lphevtHead;
    auto* queue = static_cast<NativePlaybackQueue*>(GlobalLock(queue_handle));
    if (queue == nullptr) {
        return 0;
    }

    bool finished = false;
    if (code == HC_SKIP) {
        ++queue->current;
        finished = queue->current >= queue->count;
    } else if (message_pointer != 0 && queue->current >= 0 &&
               queue->current < queue->count) {
        auto* output = reinterpret_cast<EVENTMSG*>(message_pointer);
        const NativePlaybackEvent& event = queue->events[queue->current];
        output->message = event.message;
        output->paramL = event.virtual_key;
        output->paramH = 1;
        output->time = 0;
        output->hwnd = nullptr;
    }
    GlobalUnlock(queue_handle);

    if (finished) {
        GlobalFree(queue_handle);
        *lphevtHead = nullptr;
        if (lphrgbKeyState != nullptr && *lphrgbKeyState != nullptr) {
            HANDLE state_handle = *lphrgbKeyState;
            auto* state = static_cast<unsigned char*>(GlobalLock(state_handle));
            if (state != nullptr) {
                SetKeyboardState(state);
                GlobalUnlock(state_handle);
            }
            GlobalFree(state_handle);
            *lphrgbKeyState = nullptr;
        }
    }
    return 0;
}

}  // extern "C"
