#include "windows.h"
#include "DbgHelp.h"
#include "imm.h"
#include "shellapi.h"

#include <string.h>

DWORD SymSetOptions(DWORD options) { return options; }

BOOL SymInitialize(HANDLE process, LPCSTR user_search_path,
                   BOOL invade_process) {
    (void)process;
    (void)user_search_path;
    (void)invade_process;
    return FALSE;
}

BOOL SymFromAddr(HANDLE process, DWORD64 address, DWORD64* displacement,
                 PSYMBOL_INFO symbol) {
    (void)process;
    (void)address;
    if (displacement != NULL) *displacement = 0;
    if (symbol != NULL) symbol->Name[0] = 0;
    return FALSE;
}

BOOL SymGetLineFromAddr64(HANDLE process, DWORD64 address,
                          DWORD* displacement, PIMAGEHLP_LINE64 line) {
    (void)process;
    (void)address;
    if (displacement != NULL) *displacement = 0;
    if (line != NULL) line->FileName = NULL;
    return FALSE;
}

BOOL SymCleanup(HANDLE process) {
    (void)process;
    return TRUE;
}

PVOID SymFunctionTableAccess64(HANDLE process, DWORD64 address_base) {
    (void)process;
    (void)address_base;
    return NULL;
}

DWORD64 SymGetModuleBase64(HANDLE process, DWORD64 address) {
    (void)process;
    (void)address;
    return 0;
}

BOOL StackWalk64(DWORD machine_type, HANDLE process, HANDLE thread,
                 LPSTACKFRAME64 stack_frame, PVOID context_record,
                 PVOID read_memory_routine,
                 PVOID (*function_table_access_routine)(HANDLE, DWORD64),
                 DWORD64 (*get_module_base_routine)(HANDLE, DWORD64),
                 PVOID translate_address) {
    (void)machine_type;
    (void)process;
    (void)thread;
    (void)stack_frame;
    (void)context_record;
    (void)read_memory_routine;
    (void)function_table_access_routine;
    (void)get_module_base_routine;
    (void)translate_address;
    return FALSE;
}

PRUNTIME_FUNCTION RtlLookupFunctionEntry(DWORD64 control_pc, DWORD64* image_base,
                                         PVOID history_table) {
    (void)control_pc;
    if (image_base != NULL) *image_base = 0;
    (void)history_table;
    return NULL;
}

PVOID RtlVirtualUnwind(DWORD handler_type, DWORD64 image_base,
                       DWORD64 control_pc, PRUNTIME_FUNCTION function_entry,
                       PCONTEXT context_record, PVOID* handler_data,
                       DWORD64* establisher_frame, PVOID context_pointers) {
    (void)handler_type;
    (void)image_base;
    (void)control_pc;
    (void)function_entry;
    (void)context_record;
    if (handler_data != NULL) *handler_data = NULL;
    if (establisher_frame != NULL) *establisher_frame = 0;
    (void)context_pointers;
    return NULL;
}

UINT RegisterWindowMessage(LPCSTR string) {
    return RegisterClipboardFormatA(string);
}

BOOL EndMenu(void) { return TRUE; }

BOOL ValidateRect(HWND window, const RECT* rectangle) {
    (void)window;
    (void)rectangle;
    return TRUE;
}

BOOL GetInputState(void) { return FALSE; }

BOOL FlashWindow(HWND window, BOOL invert) {
    (void)window;
    (void)invert;
    return FALSE;
}

BOOL InSendMessage(void) { return FALSE; }

int AddFontResource(LPCSTR file_name) {
    (void)file_name;
    return 0;
}

BOOL AnyPopup(void) { return FALSE; }
UINT GetCaretBlinkTime(void) { return 500; }

BOOL CreateCaret(HWND window, HBITMAP bitmap, int width, int height) {
    (void)window;
    (void)bitmap;
    (void)width;
    (void)height;
    return TRUE;
}

BOOL DestroyCaret(void) { return TRUE; }

BOOL ShowCaret(HWND window) {
    (void)window;
    return TRUE;
}

BOOL HideCaret(HWND window) {
    (void)window;
    return TRUE;
}

BOOL SetCaretPos(int x, int y) {
    (void)x;
    (void)y;
    return TRUE;
}

BOOL GetUpdateRect(HWND window, LPRECT rectangle, BOOL erase) {
    (void)window;
    (void)rectangle;
    (void)erase;
    return FALSE;
}

BOOL ScrollWindow(HWND window, int x_amount, int y_amount,
                  const RECT* rect, const RECT* clip_rect) {
    (void)window;
    (void)x_amount;
    (void)y_amount;
    (void)rect;
    (void)clip_rect;
    return TRUE;
}

BOOL EnableMenuItem(HMENU menu, UINT item, UINT enable) {
    (void)menu;
    (void)item;
    (void)enable;
    return TRUE;
}

LRESULT CallWindowProc(WNDPROC previous, HWND window, UINT message,
                       WPARAM wparam, LPARAM lparam) {
    return CallWindowProcW(previous, window, message, wparam, lparam);
}

BOOL GrayString(HDC dc, HBRUSH brush, FARPROC output, LPARAM data, int count,
                int x, int y, int width, int height) {
    (void)dc;
    (void)brush;
    (void)output;
    (void)data;
    (void)count;
    (void)x;
    (void)y;
    (void)width;
    (void)height;
    return TRUE;
}

BOOL InvertRect(HDC dc, const RECT* rect) {
    (void)dc;
    (void)rect;
    return TRUE;
}

int MessageBox(HWND window, LPCSTR text, LPCSTR caption, UINT type) {
    return MessageBoxA(window, text, caption, type);
}

BOOL SetWindowText(HWND window, LPCSTR text) {
    return SetWindowTextA(window, text);
}

int GetClassName(HWND window, LPSTR class_name, int max_count) {
    (void)window;
    if (class_name != NULL && max_count > 0) class_name[0] = 0;
    return 0;
}

int GetKeyboardState(BYTE* key_state) {
    if (key_state != NULL) memset(key_state, 0, 256);
    return TRUE;
}

UINT EnumClipboardFormats(UINT format) {
    (void)format;
    return 0;
}

int GetClipboardFormatName(UINT format, LPSTR name, int max_count) {
    (void)format;
    if (name != NULL && max_count > 0) name[0] = 0;
    return 0;
}

ATOM GlobalAddAtom(LPCSTR string) {
    return (ATOM)RegisterClipboardFormatA(string);
}

ATOM GlobalFindAtom(LPCSTR string) {
    return (ATOM)RegisterClipboardFormatA(string);
}

ATOM GlobalDeleteAtom(ATOM atom) { return atom; }

UINT GlobalGetAtomName(ATOM atom, LPSTR buffer, int size) {
    (void)atom;
    if (buffer != NULL && size > 0) buffer[0] = 0;
    return 0;
}

int GetClipBox(HDC dc, LPRECT rectangle) {
    (void)dc;
    if (rectangle != NULL) SetRect(rectangle, 0, 0, 640, 480);
    return SIMPLEREGION;
}

LONG SetBitmapBits(HBITMAP bitmap, DWORD count, const void* bits) {
    (void)bitmap;
    (void)bits;
    return (LONG)count;
}

BOOL EnumMetaFile(HDC dc, HMETAFILE metafile, FARPROC callback, LPARAM data) {
    (void)dc;
    (void)metafile;
    (void)callback;
    (void)data;
    return TRUE;
}

BOOL PlayMetaFileRecord(HDC dc, void* handle_table, void* meta_record,
                        UINT handles) {
    (void)dc;
    (void)handle_table;
    (void)meta_record;
    (void)handles;
    return TRUE;
}

BOOL DeleteMetaFile(HMETAFILE metafile) {
    (void)metafile;
    return TRUE;
}

int SetRectRgn(HRGN region, int left, int top, int right, int bottom) {
    (void)region;
    (void)left;
    (void)top;
    (void)right;
    (void)bottom;
    return TRUE;
}

int CombineRgn(HRGN destination, HRGN source1, HRGN source2, int mode) {
    (void)destination;
    (void)source1;
    (void)source2;
    (void)mode;
    return SIMPLEREGION;
}

BOOL FillRgn(HDC dc, HRGN region, HBRUSH brush) {
    (void)dc;
    (void)region;
    (void)brush;
    return TRUE;
}

BOOL InvertRgn(HDC dc, HRGN region) {
    (void)dc;
    (void)region;
    return TRUE;
}

int GetTextFace(HDC dc, int count, LPSTR face_name) {
    (void)dc;
    if (face_name != NULL && count > 0) face_name[0] = 0;
    return 0;
}

BOOL ScrollDC(HDC dc, int dx, int dy, const RECT* scroll, const RECT* clip,
              HRGN update_region, LPRECT update_rect) {
    (void)dc;
    (void)dx;
    (void)dy;
    (void)scroll;
    (void)clip;
    (void)update_region;
    if (update_rect != NULL) SetRect(update_rect, 0, 0, 0, 0);
    return TRUE;
}

DWORD SetMapperFlags(HDC dc, DWORD flags) {
    (void)dc;
    return flags;
}

int lstrlenA(LPCSTR string) {
    return string != NULL ? (int)strlen(string) : 0;
}

LPSTR lstrcatA(LPSTR destination, LPCSTR source) {
    return strcat(destination, source);
}

void Yield(void) {}
int SetSwapAreaSize(int size) { return size; }

HIMC ImmGetContext(HWND window) {
    (void)window;
    return NULL;
}

LONG ImmGetCompositionStringW(HIMC context, DWORD index, LPVOID buffer,
                              DWORD buffer_length) {
    (void)context;
    (void)index;
    (void)buffer;
    (void)buffer_length;
    return 0;
}

BOOL ImmReleaseContext(HWND window, HIMC context) {
    (void)window;
    (void)context;
    return TRUE;
}

HINSTANCE ShellExecuteA(HWND hwnd, LPCSTR operation, LPCSTR file,
                        LPCSTR parameters, LPCSTR directory,
                        int show_command) {
    (void)hwnd;
    (void)operation;
    (void)file;
    (void)parameters;
    (void)directory;
    (void)show_command;
    return (HINSTANCE)(uintptr_t)32;
}
