#include "opus-native-compat.h"

#include <stdint.h>
#include <string.h>
#include "shellapi.h"

/* Win16 exported several helpers that disappeared when code segments and GDI
 * packed-coordinate APIs went away. These adapters preserve the contracts used
 * by the original Word C sources on the flat Win64 process model.
 */

#ifdef RegisterClipboardFormat
#undef RegisterClipboardFormat
#endif

static DWORD pack_signed_pair(LONG first, LONG second) {
    return MAKELONG((SHORT)first, (SHORT)second);
}

BOOL OpusChangeMenu(HMENU menu, UINT item, const void* new_text,
                    UINT_PTR new_item, UINT flags) {
    UINT operation_flags =
        flags & ~(MF_APPEND | MF_CHANGE | MF_DELETE | MF_REMOVE);
    LPCSTR text = (operation_flags & MF_BITMAP) != 0
                      ? NULL
                      : (LPCSTR)new_text;

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

void InitApploader(void) {
    /* P-code segment loading is unnecessary in a flat native image. */
}

HANDLE GetCodeHandle(FARPROC procedure) {
    (void)procedure;
    return GetModuleHandleW(NULL);
}

void CacheCodeSegment(HANDLE handle, WORD segment) {
    (void)handle;
    (void)segment;
}

void GlobalLruNewest(HANDLE handle) {
    (void)handle;
}

void GlobalLruOldest(HANDLE handle) {
    (void)handle;
}

HANDLE GetPhysicalFontHandle(HANDLE device_context) {
    return GetCurrentObject((HDC)device_context, OBJ_FONT);
}

UINT RegisterClipboardFormat(const char* name) {
    return RegisterClipboardFormatA(name);
}

DWORD GetBitmapDimension(HBITMAP bitmap) {
    SIZE size = {0};

    return GetBitmapDimensionEx(bitmap, &size)
               ? pack_signed_pair(size.cx, size.cy)
               : 0;
}

DWORD SetBitmapDimension(HBITMAP bitmap, int x, int y) {
    SIZE previous = {0};

    return SetBitmapDimensionEx(bitmap, x, y, &previous)
               ? pack_signed_pair(previous.cx, previous.cy)
               : 0;
}

DWORD GetViewportExt(HDC device_context) {
    SIZE size = {0};

    return GetViewportExtEx(device_context, &size)
               ? pack_signed_pair(size.cx, size.cy)
               : 0;
}

DWORD GetViewportOrg(HDC device_context) {
    POINT point = {0};

    return GetViewportOrgEx(device_context, &point)
               ? pack_signed_pair(point.x, point.y)
               : 0;
}

DWORD SetViewportExt(HDC device_context, int x, int y) {
    SIZE previous = {0};

    return SetViewportExtEx(device_context, x, y, &previous)
               ? pack_signed_pair(previous.cx, previous.cy)
               : 0;
}

DWORD SetViewportOrg(HDC device_context, int x, int y) {
    POINT previous = {0};

    return SetViewportOrgEx(device_context, x, y, &previous)
               ? pack_signed_pair(previous.x, previous.y)
               : 0;
}

DWORD SetWindowExt(HDC device_context, int x, int y) {
    SIZE previous = {0};

    return SetWindowExtEx(device_context, x, y, &previous)
               ? pack_signed_pair(previous.cx, previous.cy)
               : 0;
}

DWORD SetWindowOrg(HDC device_context, int x, int y) {
    POINT previous = {0};

    return SetWindowOrgEx(device_context, x, y, &previous)
               ? pack_signed_pair(previous.x, previous.y)
               : 0;
}

DWORD GetTextExtent(HDC device_context, const char* text, int count) {
    SIZE size = {0};

    return device_context != NULL && text != NULL && count >= 0 &&
                   GetTextExtentPoint32A(device_context, text, count, &size)
               ? pack_signed_pair(size.cx, size.cy)
               : 0;
}

UINT GetMenuItemId(HMENU menu, int position) {
    return GetMenuItemID(menu, position);
}

BYTE GetTempDrive(BYTE requested_drive) {
    char path[MAX_PATH] = {0};
    DWORD length;
    char drive;

    (void)requested_drive;
    length = GetTempPathA(MAX_PATH, path);
    if (length >= 2 && path[1] == ':') {
        drive = path[0];
        CharUpperBuffA(&drive, 1);
        return (BYTE)drive;
    }
    return 0;
}

void beep(void) {
    MessageBeep(MB_OK);
}

BOOL MoveTo(HDC device_context, int x, int y) {
    return MoveToEx(device_context, x, y, NULL);
}

short GetEnvironment(char* name, char* environment, int byte_capacity) {
    (void)name;
    if (environment != NULL && byte_capacity > 0) environment[0] = '\0';
    return 0;
}

int ShellExec(char* file, char* arguments) {
    intptr_t result;

    if (file == NULL || *file == '\0') return 0;
    result = (intptr_t)ShellExecuteA(
        NULL, "open", file,
        arguments != NULL && *arguments != '\0' ? arguments : NULL, NULL,
        SW_SHOWNORMAL);
    return result > 32;
}

int FSetFirstLbx(void** dialog, WORD tmc) {
    (void)dialog;
    (void)tmc;
    return TRUE;
}

/* Word's Win16 thunk used this only after the legacy EXITWINDOWS export could
 * not be dynamically loaded. Ending the Word process must not implicitly log
 * off the modern desktop session.
 */
void EXITWINDOWS(char* arguments) {
    (void)arguments;
}

void CopyRgbLcb(const void* source, void* destination, long byte_count) {
    if (source != NULL && destination != NULL && byte_count > 0) {
        memmove(destination, source, (size_t)byte_count);
    }
}

char* PchString(char* text, ...) {
    return text;
}

char* LpchString(char* text, ...) {
    return text;
}

WORD pstackTop = 0;

WORD HstackMyData(void) {
    return 0;
}

DWORD LCallOtherStack(void* procedure, WORD stack, const WORD* words,
                      WORD word_count) {
    int arguments[16] = {0};
    int count = word_count > 16 ? 16 : word_count;
    int index;

    (void)stack;
    for (index = 0; index < count; ++index) {
        arguments[index] = words == NULL ? 0 : words[index];
    }
    return (DWORD)LPushMacroArgs(procedure, arguments, count);
}
