#include "windows.h"

#include <cstdint>

namespace {

enum class OpusModule {
    Self,
    Kernel,
    User,
    Gdi,
    Keyboard,
};

struct OpusModuleSentinel {
    OpusModule module;
};

OpusModuleSentinel self_module{OpusModule::Self};
OpusModuleSentinel kernel_module{OpusModule::Kernel};
OpusModuleSentinel user_module{OpusModule::User};
OpusModuleSentinel gdi_module{OpusModule::Gdi};
OpusModuleSentinel keyboard_module{OpusModule::Keyboard};

bool MatchesModuleName(const char* name, const char* module) {
    if (name == nullptr) {
        return false;
    }
    while (*module != '\0') {
        char ch = *name++;
        if (ch >= 'a' && ch <= 'z') {
            ch = static_cast<char>(ch - ('a' - 'A'));
        }
        if (ch != *module++) {
            return false;
        }
    }
    return *name == '\0' || *name == '.';
}

bool MatchesModuleName(const WCHAR* name, const char* module) {
    if (name == nullptr) {
        return false;
    }
    while (*module != '\0') {
        WCHAR ch = *name++;
        if (ch >= 'a' && ch <= 'z') {
            ch = static_cast<WCHAR>(ch - ('a' - 'A'));
        }
        if (ch != static_cast<WCHAR>(*module++)) {
            return false;
        }
    }
    return *name == 0 || *name == '.';
}

HMODULE ModuleHandleForName(const char* name) {
    if (MatchesModuleName(name, "KERNEL")) {
        return reinterpret_cast<HMODULE>(&kernel_module);
    }
    if (MatchesModuleName(name, "USER")) {
        return reinterpret_cast<HMODULE>(&user_module);
    }
    if (MatchesModuleName(name, "GDI")) {
        return reinterpret_cast<HMODULE>(&gdi_module);
    }
    if (MatchesModuleName(name, "KEYBOARD")) {
        return reinterpret_cast<HMODULE>(&keyboard_module);
    }
    return nullptr;
}

HMODULE ModuleHandleForName(const WCHAR* name) {
    if (MatchesModuleName(name, "KERNEL")) {
        return reinterpret_cast<HMODULE>(&kernel_module);
    }
    if (MatchesModuleName(name, "USER")) {
        return reinterpret_cast<HMODULE>(&user_module);
    }
    if (MatchesModuleName(name, "GDI")) {
        return reinterpret_cast<HMODULE>(&gdi_module);
    }
    if (MatchesModuleName(name, "KEYBOARD")) {
        return reinterpret_cast<HMODULE>(&keyboard_module);
    }
    return nullptr;
}

bool IsOrdinalName(LPCSTR name) {
    return (reinterpret_cast<uintptr_t>(name) >> 16) == 0;
}

WORD OrdinalFromName(LPCSTR name) {
    return static_cast<WORD>(reinterpret_cast<uintptr_t>(name));
}

long OpusWin16GetFreeSpace(int) {
    return 64L * 1024L * 1024L;
}

HANDLE OpusWin16AllocSelector(HANDLE handle) {
    return handle;
}

void OpusWin16PrestoChangoSelector(HANDLE, HANDLE) {
}

void OpusWin16FreeSelector(HANDLE) {
}

void OpusWin16SetSpeed(int) {
}

void OpusWin16ExitWindows(LPSTR) {
}

HBITMAP OpusWin16CreateDIBitmap(HDC, const BITMAPINFOHEADER*, DWORD, LPCVOID,
                                const BITMAPINFO*, UINT) {
    return nullptr;
}

int OpusWin16GetDIBits(HDC, HBITMAP, UINT, UINT, LPVOID, BITMAPINFO*, UINT) {
    return 0;
}

FARPROC ProcAddressForOrdinal(OpusModule module, WORD ordinal) {
    switch (module) {
    case OpusModule::Kernel:
        switch (ordinal) {
        case 169:
            return reinterpret_cast<FARPROC>(OpusWin16GetFreeSpace);
        case 175:
            return reinterpret_cast<FARPROC>(OpusWin16AllocSelector);
        case 176:
            return reinterpret_cast<FARPROC>(OpusWin16FreeSelector);
        case 177:
            return reinterpret_cast<FARPROC>(OpusWin16PrestoChangoSelector);
        }
        break;
    case OpusModule::User:
        if (ordinal == 7) {
            return reinterpret_cast<FARPROC>(OpusWin16ExitWindows);
        }
        break;
    case OpusModule::Gdi:
        switch (ordinal) {
        case 441:
            return reinterpret_cast<FARPROC>(OpusWin16GetDIBits);
        case 442:
            return reinterpret_cast<FARPROC>(OpusWin16CreateDIBitmap);
        }
        break;
    case OpusModule::Keyboard:
        if (ordinal == 7) {
            return reinterpret_cast<FARPROC>(OpusWin16SetSpeed);
        }
        break;
    case OpusModule::Self:
        break;
    }
    return nullptr;
}

OpusModuleSentinel* SentinelFromHandle(HMODULE module) {
    if (module == reinterpret_cast<HMODULE>(&self_module)) {
        return &self_module;
    }
    if (module == reinterpret_cast<HMODULE>(&kernel_module)) {
        return &kernel_module;
    }
    if (module == reinterpret_cast<HMODULE>(&user_module)) {
        return &user_module;
    }
    if (module == reinterpret_cast<HMODULE>(&gdi_module)) {
        return &gdi_module;
    }
    if (module == reinterpret_cast<HMODULE>(&keyboard_module)) {
        return &keyboard_module;
    }
    return nullptr;
}

}  // namespace

extern "C" {

HMODULE GetModuleHandleA(LPCSTR module_name) {
    return module_name == nullptr ? reinterpret_cast<HMODULE>(&self_module)
                                  : ModuleHandleForName(module_name);
}

HMODULE GetModuleHandleW(LPCWSTR module_name) {
    return module_name == nullptr ? reinterpret_cast<HMODULE>(&self_module)
                                  : ModuleHandleForName(module_name);
}

HMODULE LoadLibraryA(LPCSTR) {
    return nullptr;
}

HMODULE LoadLibraryExW(LPCWSTR, HANDLE, DWORD) {
    return nullptr;
}

BOOL FreeLibrary(HMODULE) {
    return TRUE;
}

FARPROC GetProcAddress(HMODULE module, LPCSTR name) {
    if (module == nullptr || name == nullptr || !IsOrdinalName(name)) {
        return nullptr;
    }
    const auto* sentinel = SentinelFromHandle(module);
    if (sentinel == nullptr) {
        return nullptr;
    }
    return ProcAddressForOrdinal(sentinel->module, OrdinalFromName(name));
}

}
