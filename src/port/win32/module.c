#include "windows.h"

#include <stdint.h>

enum OpusModule {
    kOpusModuleSelf,
    kOpusModuleKernel,
    kOpusModuleUser,
    kOpusModuleGdi,
    kOpusModuleKeyboard,
};

struct OpusModuleSentinel {
    enum OpusModule module;
};

static struct OpusModuleSentinel self_module = {kOpusModuleSelf};
static struct OpusModuleSentinel kernel_module = {kOpusModuleKernel};
static struct OpusModuleSentinel user_module = {kOpusModuleUser};
static struct OpusModuleSentinel gdi_module = {kOpusModuleGdi};
static struct OpusModuleSentinel keyboard_module = {kOpusModuleKeyboard};

static BOOL MatchesModuleNameA(const char* name, const char* module) {
    if (name == NULL) {
        return FALSE;
    }
    while (*module != '\0') {
        char ch = *name++;
        if (ch >= 'a' && ch <= 'z') {
            ch = (char)(ch - ('a' - 'A'));
        }
        if (ch != *module++) {
            return FALSE;
        }
    }
    return *name == '\0' || *name == '.';
}

static BOOL MatchesModuleNameW(const WCHAR* name, const char* module) {
    if (name == NULL) {
        return FALSE;
    }
    while (*module != '\0') {
        WCHAR ch = *name++;
        if (ch >= 'a' && ch <= 'z') {
            ch = (WCHAR)(ch - ('a' - 'A'));
        }
        if (ch != (WCHAR)(*module++)) {
            return FALSE;
        }
    }
    return *name == 0 || *name == '.';
}

static HMODULE ModuleHandleForNameA(const char* name) {
    if (MatchesModuleNameA(name, "KERNEL")) {
        return (HMODULE)&kernel_module;
    }
    if (MatchesModuleNameA(name, "USER")) {
        return (HMODULE)&user_module;
    }
    if (MatchesModuleNameA(name, "GDI")) {
        return (HMODULE)&gdi_module;
    }
    if (MatchesModuleNameA(name, "KEYBOARD")) {
        return (HMODULE)&keyboard_module;
    }
    return NULL;
}

static HMODULE ModuleHandleForNameW(const WCHAR* name) {
    if (MatchesModuleNameW(name, "KERNEL")) {
        return (HMODULE)&kernel_module;
    }
    if (MatchesModuleNameW(name, "USER")) {
        return (HMODULE)&user_module;
    }
    if (MatchesModuleNameW(name, "GDI")) {
        return (HMODULE)&gdi_module;
    }
    if (MatchesModuleNameW(name, "KEYBOARD")) {
        return (HMODULE)&keyboard_module;
    }
    return NULL;
}

static BOOL IsOrdinalName(LPCSTR name) {
    return ((uintptr_t)name >> 16) == 0;
}

static WORD OrdinalFromName(LPCSTR name) {
    return (WORD)(uintptr_t)name;
}

static long OpusWin16GetFreeSpace(int drive) {
    (void)drive;
    return 64L * 1024L * 1024L;
}

static HANDLE OpusWin16AllocSelector(HANDLE handle) {
    return handle;
}

static void OpusWin16PrestoChangoSelector(HANDLE source, HANDLE target) {
    (void)source;
    (void)target;
}

static void OpusWin16FreeSelector(HANDLE handle) {
    (void)handle;
}

static void OpusWin16SetSpeed(int speed) {
    (void)speed;
}

static void OpusWin16ExitWindows(LPSTR args) {
    (void)args;
}

static HBITMAP OpusWin16CreateDIBitmap(HDC device_context,
                                       const BITMAPINFOHEADER* header,
                                       DWORD init, LPCVOID bits,
                                       const BITMAPINFO* info, UINT usage) {
    (void)device_context;
    (void)header;
    (void)init;
    (void)bits;
    (void)info;
    (void)usage;
    return NULL;
}

static int OpusWin16GetDIBits(HDC device_context, HBITMAP bitmap, UINT start,
                              UINT lines, LPVOID bits, BITMAPINFO* info,
                              UINT usage) {
    (void)device_context;
    (void)bitmap;
    (void)start;
    (void)lines;
    (void)bits;
    (void)info;
    (void)usage;
    return 0;
}

static FARPROC ProcAddressForOrdinal(enum OpusModule module, WORD ordinal) {
    switch (module) {
    case kOpusModuleKernel:
        switch (ordinal) {
        case 169:
            return (FARPROC)OpusWin16GetFreeSpace;
        case 175:
            return (FARPROC)OpusWin16AllocSelector;
        case 176:
            return (FARPROC)OpusWin16FreeSelector;
        case 177:
            return (FARPROC)OpusWin16PrestoChangoSelector;
        }
        break;
    case kOpusModuleUser:
        if (ordinal == 7) {
            return (FARPROC)OpusWin16ExitWindows;
        }
        break;
    case kOpusModuleGdi:
        switch (ordinal) {
        case 441:
            return (FARPROC)OpusWin16GetDIBits;
        case 442:
            return (FARPROC)OpusWin16CreateDIBitmap;
        }
        break;
    case kOpusModuleKeyboard:
        if (ordinal == 7) {
            return (FARPROC)OpusWin16SetSpeed;
        }
        break;
    case kOpusModuleSelf:
        break;
    }
    return NULL;
}

static struct OpusModuleSentinel* SentinelFromHandle(HMODULE module) {
    if (module == (HMODULE)&self_module) {
        return &self_module;
    }
    if (module == (HMODULE)&kernel_module) {
        return &kernel_module;
    }
    if (module == (HMODULE)&user_module) {
        return &user_module;
    }
    if (module == (HMODULE)&gdi_module) {
        return &gdi_module;
    }
    if (module == (HMODULE)&keyboard_module) {
        return &keyboard_module;
    }
    return NULL;
}

HMODULE GetModuleHandleA(LPCSTR module_name) {
    return module_name == NULL ? (HMODULE)&self_module
                               : ModuleHandleForNameA(module_name);
}

HMODULE GetModuleHandleW(LPCWSTR module_name) {
    return module_name == NULL ? (HMODULE)&self_module
                               : ModuleHandleForNameW(module_name);
}

HMODULE LoadLibraryA(LPCSTR file_name) {
    (void)file_name;
    return NULL;
}

HMODULE LoadLibraryExW(LPCWSTR file_name, HANDLE file, DWORD flags) {
    (void)file_name;
    (void)file;
    (void)flags;
    return NULL;
}

BOOL FreeLibrary(HMODULE module) {
    (void)module;
    return TRUE;
}

FARPROC GetProcAddress(HMODULE module, LPCSTR name) {
    if (module == NULL || name == NULL || !IsOrdinalName(name)) {
        return NULL;
    }
    const struct OpusModuleSentinel* sentinel = SentinelFromHandle(module);
    if (sentinel == NULL) {
        return NULL;
    }
    return ProcAddressForOrdinal(sentinel->module, OrdinalFromName(name));
}
