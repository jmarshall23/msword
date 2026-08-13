#include "opus_x64_compat.h"

#include <cstdint>

using GetFreeSpaceProc = long (*)(int);
using SelectorProc = HANDLE (*)(HANDLE);
using VoidSelectorProc = void (*)(HANDLE);
using SetSpeedProc = void (*)(int);

int main() {
    if (GetModuleHandleA(nullptr) == nullptr ||
        GetModuleHandleW(nullptr) == nullptr) {
        return 1;
    }
    if (GetModuleHandleA("KERNEL") == nullptr ||
        GetModuleHandleA("kernel.dll") == nullptr ||
        GetModuleHandleA("USER") == nullptr ||
        GetModuleHandleA("GDI") == nullptr ||
        GetModuleHandleA("KEYBOARD") == nullptr) {
        return 2;
    }
    if (LoadLibraryA("spell.dll") != nullptr ||
        LoadLibraryExW(OPUSW("msftedit.dll"), nullptr, 0) != nullptr) {
        return 3;
    }

    HMODULE kernel = GetModuleHandleA("KERNEL");
    HMODULE user = GetModuleHandleA("USER");
    HMODULE gdi = GetModuleHandleA("GDI");
    HMODULE keyboard = GetModuleHandleA("KEYBOARD");
    auto get_free_space = reinterpret_cast<GetFreeSpaceProc>(
        GetProcAddress(kernel, MAKEINTRESOURCE(169)));
    auto alloc_selector = reinterpret_cast<SelectorProc>(
        GetProcAddress(kernel, MAKEINTRESOURCE(175)));
    auto free_selector = reinterpret_cast<VoidSelectorProc>(
        GetProcAddress(kernel, MAKEINTRESOURCE(176)));
    auto set_speed = reinterpret_cast<SetSpeedProc>(
        GetProcAddress(keyboard, MAKEINTRESOURCE(7)));
    if (get_free_space == nullptr || alloc_selector == nullptr ||
        free_selector == nullptr || set_speed == nullptr ||
        GetProcAddress(user, MAKEINTRESOURCE(7)) == nullptr ||
        GetProcAddress(gdi, MAKEINTRESOURCE(441)) == nullptr ||
        GetProcAddress(gdi, MAKEINTRESOURCE(442)) == nullptr) {
        return 4;
    }
    if (GetProcAddress(kernel, "GetFreeSpace") != nullptr ||
        GetProcAddress(kernel, MAKEINTRESOURCE(13)) != nullptr ||
        GetProcAddress(gdi, MAKEINTRESOURCE(440)) != nullptr ||
        GetProcAddress(reinterpret_cast<HMODULE>(static_cast<uintptr_t>(0x1234)),
                       MAKEINTRESOURCE(169)) != nullptr) {
        return 5;
    }

    HANDLE handle = reinterpret_cast<HANDLE>(static_cast<uintptr_t>(0x5678));
    if (alloc_selector(handle) != handle) {
        return 6;
    }
    free_selector(handle);
    set_speed(31);
    if (get_free_space(0) <= 0) {
        return 7;
    }
    return 0;
}
