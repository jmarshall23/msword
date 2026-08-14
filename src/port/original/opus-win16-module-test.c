#include "opus_x64_compat.h"

#include <stdint.h>

typedef long (*GetFreeSpaceProc)(int);
typedef HANDLE (*SelectorProc)(HANDLE);
typedef void (*VoidSelectorProc)(HANDLE);
typedef void (*SetSpeedProc)(int);

int main(void) {
    static const WCHAR msftedit_dll[] = {
        'm', 's', 'f', 't', 'e', 'd', 'i', 't', '.', 'd', 'l', 'l', 0};

    if (GetModuleHandleA(NULL) == NULL || GetModuleHandleW(NULL) == NULL) {
        return 1;
    }
    if (GetModuleHandleA("KERNEL") == NULL ||
        GetModuleHandleA("kernel.dll") == NULL ||
        GetModuleHandleA("USER") == NULL || GetModuleHandleA("GDI") == NULL ||
        GetModuleHandleA("KEYBOARD") == NULL) {
        return 2;
    }
    if (LoadLibraryA("spell.dll") != NULL ||
        LoadLibraryExW(msftedit_dll, NULL, 0) != NULL) {
        return 3;
    }
    char module_path[MAX_PATH] = {0};
    if (GetModuleFileName(NULL, module_path, sizeof(module_path)) == 0 ||
        module_path[0] == '\0') {
        return 8;
    }

    HMODULE kernel = GetModuleHandleA("KERNEL");
    HMODULE user = GetModuleHandleA("USER");
    HMODULE gdi = GetModuleHandleA("GDI");
    HMODULE keyboard = GetModuleHandleA("KEYBOARD");
    GetFreeSpaceProc get_free_space =
        (GetFreeSpaceProc)GetProcAddress(kernel, MAKEINTRESOURCE(169));
    SelectorProc alloc_selector =
        (SelectorProc)GetProcAddress(kernel, MAKEINTRESOURCE(175));
    VoidSelectorProc free_selector =
        (VoidSelectorProc)GetProcAddress(kernel, MAKEINTRESOURCE(176));
    SetSpeedProc set_speed =
        (SetSpeedProc)GetProcAddress(keyboard, MAKEINTRESOURCE(7));
    if (get_free_space == NULL || alloc_selector == NULL ||
        free_selector == NULL || set_speed == NULL ||
        GetProcAddress(user, MAKEINTRESOURCE(7)) == NULL ||
        GetProcAddress(gdi, MAKEINTRESOURCE(441)) == NULL ||
        GetProcAddress(gdi, MAKEINTRESOURCE(442)) == NULL) {
        return 4;
    }
    if (GetProcAddress(kernel, "GetFreeSpace") != NULL ||
        GetProcAddress(kernel, MAKEINTRESOURCE(13)) != NULL ||
        GetProcAddress(gdi, MAKEINTRESOURCE(440)) != NULL ||
        GetProcAddress((HMODULE)(uintptr_t)0x1234, MAKEINTRESOURCE(169)) !=
            NULL) {
        return 5;
    }

    HANDLE handle = (HANDLE)(uintptr_t)0x5678;
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
