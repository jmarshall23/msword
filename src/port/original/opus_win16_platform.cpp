#include "opus_x64_compat.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include "shellapi.h"

/* Win16 exported several helpers that disappeared when code segments and GDI
 * packed-coordinate APIs went away. These adapters preserve the contracts used
 * by the original Word C sources on the flat Win64 process model.
 */

#ifdef RegisterClipboardFormat
#undef RegisterClipboardFormat
#endif

namespace {

DWORD PackSignedPair(const LONG first, const LONG second) noexcept {
    return MAKELONG(static_cast<SHORT>(first), static_cast<SHORT>(second));
}

}  // namespace

extern "C" {

BOOL OpusChangeMenu(HMENU menu, UINT item, const void* new_text,
                    UINT_PTR new_item, UINT flags) {
    const UINT operation_flags =
        flags & ~(MF_APPEND | MF_CHANGE | MF_DELETE | MF_REMOVE);
    const auto text = (operation_flags & MF_BITMAP) != 0
                          ? nullptr
                          : static_cast<LPCSTR>(new_text);
    if ((flags & MF_APPEND) != 0) {
        return AppendMenuA(menu, operation_flags, new_item, text);
    }
    if ((flags & MF_DELETE) != 0) {
        return DeleteMenu(menu, item, operation_flags);
    }
    if ((flags & MF_REMOVE) != 0) {
        return RemoveMenu(menu, item, operation_flags);
    }
    if ((flags & MF_CHANGE) != 0) {
        return ModifyMenuA(menu, item, operation_flags, new_item, text);
    }
    return InsertMenuA(menu, item, operation_flags, new_item, text);
}

long LPushMacroArgs(void* procedure, const int* arguments,
                    int argument_count);

BYTE fProtectModeWindows = TRUE;

void InitApploader() {
    /* P-code segment loading is unnecessary in a flat native image. */
}

HANDLE GetCodeHandle(FARPROC) {
    return GetModuleHandleW(nullptr);
}

void CacheCodeSegment(HANDLE, WORD) {
}

void GlobalLruNewest(HANDLE) {
}

void GlobalLruOldest(HANDLE) {
}

HANDLE GetPhysicalFontHandle(HANDLE device_context) {
    return GetCurrentObject(static_cast<HDC>(device_context), OBJ_FONT);
}

UINT RegisterClipboardFormat(const char* name) {
    return RegisterClipboardFormatA(name);
}

DWORD GetBitmapDimension(HBITMAP bitmap) {
    SIZE size = {};
    return GetBitmapDimensionEx(bitmap, &size)
        ? PackSignedPair(size.cx, size.cy) : 0;
}

DWORD SetBitmapDimension(HBITMAP bitmap, int x, int y) {
    SIZE previous = {};
    return SetBitmapDimensionEx(bitmap, x, y, &previous)
        ? PackSignedPair(previous.cx, previous.cy) : 0;
}

DWORD GetViewportExt(HDC device_context) {
    SIZE size = {};
    return GetViewportExtEx(device_context, &size)
        ? PackSignedPair(size.cx, size.cy) : 0;
}

DWORD GetViewportOrg(HDC device_context) {
    POINT point = {};
    return GetViewportOrgEx(device_context, &point)
        ? PackSignedPair(point.x, point.y) : 0;
}

DWORD SetViewportExt(HDC device_context, int x, int y) {
    SIZE previous = {};
    return SetViewportExtEx(device_context, x, y, &previous)
        ? PackSignedPair(previous.cx, previous.cy) : 0;
}

DWORD SetViewportOrg(HDC device_context, int x, int y) {
    POINT previous = {};
    return SetViewportOrgEx(device_context, x, y, &previous)
        ? PackSignedPair(previous.x, previous.y) : 0;
}

DWORD SetWindowExt(HDC device_context, int x, int y) {
    SIZE previous = {};
    return SetWindowExtEx(device_context, x, y, &previous)
        ? PackSignedPair(previous.cx, previous.cy) : 0;
}

DWORD SetWindowOrg(HDC device_context, int x, int y) {
    POINT previous = {};
    return SetWindowOrgEx(device_context, x, y, &previous)
        ? PackSignedPair(previous.x, previous.y) : 0;
}

DWORD GetTextExtent(HDC device_context, const char* text, int count) {
    SIZE size = {};
    return device_context != nullptr && text != nullptr && count >= 0 &&
                   GetTextExtentPoint32A(device_context, text, count, &size)
               ? PackSignedPair(size.cx, size.cy)
               : 0;
}

UINT GetMenuItemId(HMENU menu, int position) {
    return GetMenuItemID(menu, position);
}

BYTE GetTempDrive(BYTE) {
    char path[MAX_PATH] = {};
    const DWORD length = GetTempPathA(MAX_PATH, path);
    if (length >= 2 && path[1] == ':') {
        char drive = path[0];
        CharUpperBuffA(&drive, 1);
        return static_cast<BYTE>(drive);
    }
    return 0;
}

void beep() {
    MessageBeep(MB_OK);
}

BOOL MoveTo(HDC device_context, int x, int y) {
    return MoveToEx(device_context, x, y, nullptr);
}

short GetEnvironment(char*, char* environment, int byte_capacity) {
    if (environment != nullptr && byte_capacity > 0) {
        environment[0] = '\0';
    }
    return 0;
}

int ShellExec(char* file, char* arguments) {
    if (file == nullptr || *file == '\0') {
        return 0;
    }
    const auto result = reinterpret_cast<std::intptr_t>(ShellExecuteA(
        nullptr, "open", file,
        arguments != nullptr && *arguments != '\0' ? arguments : nullptr,
        nullptr, SW_SHOWNORMAL));
    return result > 32;
}

int FSetFirstLbx(void**, WORD) { return true; }

/* Word's Win16 thunk used this only after the legacy EXITWINDOWS export could
 * not be dynamically loaded. Ending the Word process must not implicitly log
 * off the modern desktop session.
 */
void EXITWINDOWS(char*) {
}

void CopyRgbLcb(const void* source, void* destination, long byte_count) {
    if (source != nullptr && destination != nullptr && byte_count > 0) {
        std::memmove(destination, source, static_cast<std::size_t>(byte_count));
    }
}

char* PchString(char* text, ...) {
    return text;
}

char* LpchString(char* text, ...) {
    return text;
}

WORD pstackTop = 0;

WORD HstackMyData() {
    return 0;
}

DWORD LCallOtherStack(void* procedure, WORD, const WORD* words,
                      WORD word_count) {
    int arguments[16] = {};
    const int count = (word_count > 16) ? 16 : word_count;
    for (int index = 0; index < count; ++index) {
        arguments[index] = words == nullptr ? 0 : words[index];
    }
    return static_cast<DWORD>(LPushMacroArgs(procedure, arguments, count));
}

}  // extern "C"
