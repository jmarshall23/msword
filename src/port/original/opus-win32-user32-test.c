#include "windows.h"

#include <string.h>

#include "opusinputscript.h"

#define kUserData ((LONG_PTR)0x12345678)
#define kExtraData ((LONG_PTR)0x123456789)
int g_nc_create_count = 0;
int g_create_count = 0;
int g_user_message_count = 0;
int g_paint_count = 0;
WPARAM g_last_char = 0;
WPARAM g_last_keyup = 0;
int g_enum_count = 0;
HWND g_enum_windows[8] = {0};

LRESULT CALLBACK TestWindowProc(HWND window, UINT message, WPARAM wparam,
                                LPARAM parameter) {
    if (message == WM_NCCREATE) {
        ++g_nc_create_count;
        const CREATESTRUCTA *create = (const CREATESTRUCTA *)parameter;
        SetWindowLongPtrA(window, GWLP_USERDATA,
                          (LONG_PTR)(create->lpCreateParams));
        return DefWindowProcA(window, message, 0, parameter);
    }
    if (message == WM_CREATE) {
        ++g_create_count;
        return 0;
    }
    if (message == WM_USER + 1) {
        ++g_user_message_count;
        return (LRESULT)(parameter + 7);
    }
    if (message == WM_PAINT) {
        ++g_paint_count;
        return 0;
    }
    if (message == WM_CHAR) {
        g_last_char = wparam;
        return 0;
    }
    if (message == WM_KEYUP) {
        g_last_keyup = wparam;
        PostQuitMessage(7);
        return 0;
    }
    return DefWindowProcA(window, message, 0, parameter);
}

BOOL CALLBACK CollectWindow(HWND window, LPARAM parameter) {
    (void)parameter;
    if (g_enum_count < 8) g_enum_windows[g_enum_count] = window;
    ++g_enum_count;
    return TRUE;
}

BOOL CALLBACK StopAfterFirstWindow(HWND window, LPARAM parameter) {
    (void)parameter;
    if (g_enum_count < 8) g_enum_windows[g_enum_count] = window;
    ++g_enum_count;
    return FALSE;
}

int main(void) {
    WNDCLASSEXA cls = {0};
    cls.cbSize = sizeof(cls);
    cls.lpfnWndProc = TestWindowProc;
    cls.cbWndExtra = 16;
    cls.lpszClassName = "OpusUser32Test";
    const ATOM atom = RegisterClassExA(&cls);
    if (atom == 0) return 1;
    if (RegisterClassExA(&cls) != 0 ||
        GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
        return 2;
    }

    HWND window = CreateWindowExA(
        WS_EX_DLGMODALFRAME, "OpusUser32Test", "caption",
        WS_POPUP | WS_CAPTION, 10, 20, 200, 100, NULL, NULL, NULL,
        (LPVOID)(kUserData));
    if (window == NULL || !IsWindow(window) || g_nc_create_count != 1 ||
        g_create_count != 1) {
        return 3;
    }
    if (GetWindowLongPtrA(window, GWLP_USERDATA) != kUserData ||
        GetWindowLongA(window, GWL_STYLE) != (WS_POPUP | WS_CAPTION) ||
        GetWindowLongPtrA(window, GWL_EXSTYLE) != WS_EX_DLGMODALFRAME) {
        return 4;
    }
    if (SetWindowLongPtrA(window, 0, kExtraData) != 0 ||
        GetWindowLongPtrA(window, 0) != kExtraData) {
        return 5;
    }
    if (CallNextHookEx(NULL, 0, 1, 2) != 0 ||
        CallNextHookEx((HANDLE)(UINT_PTR)0x1234, HC_GETNEXT, 3, 4) != 0) {
        return 6;
    }
    if (SetWindowLongA(window, 8, 0x11223344) != 0 ||
        GetWindowLongA(window, 8) != 0x11223344 ||
        SetWindowWord(window, 10, 0xabcd) != 0x1122 ||
        GetWindowLongA(window, 8) != (LONG)(0xabcd3344u) ||
        SetWindowWord(window, 12, 0x5566) != 0 ||
        GetWindowWord(window, 12) != 0x5566 ||
        GetWindowWord(window, 14) != 0) {
        return 7;
    }

    char text[16] = {0};
    WCHAR wide_text[16] = {0};
    const WCHAR wide_title[] = {'w', 'i', 'd', 'e', 0};
    const WCHAR lower_word[] = {'w', 'o', 'r', 'd', 0};
    const WCHAR mixed_word[] = {'W', 'o', 'R', 'd', 0};
    const WCHAR later_word[] = {'w', 'o', 'r', 'e', 0};
    const WCHAR high_upper[] = {0x00c4, 0};
    const WCHAR high_lower[] = {0x00e4, 0};
    if (!SetWindowTextA(window, "plain") ||
        GetWindowTextLengthA(window) != 5 ||
        GetWindowTextA(window, text, sizeof(text)) != 5 ||
        text[0] != 'p' || text[4] != 'n' ||
        GetWindowTextW(window, wide_text, 16) != 5 ||
        wide_text[0] != 'p' || wide_text[4] != 'n' ||
        !SetWindowTextW(window, wide_title) ||
        GetWindowTextLengthW(window) != 4 ||
        GetWindowTextA(window, text, sizeof(text)) != 4 ||
        text[0] != 'w' || text[3] != 'e' ||
        CallWindowProcW(TestWindowProc, window, WM_USER + 1, 0, 50) != 57 ||
        CallWindowProcW(NULL, window, WM_USER + 1, 0, 50) != 0 ||
        lstrcmpW(lower_word, lower_word) != 0 ||
        lstrcmpW(lower_word, later_word) >= 0 ||
        lstrcmpW(NULL, lower_word) >= 0 ||
        lstrcmpiW(lower_word, mixed_word) != 0 ||
        lstrcmpiW(high_upper, high_lower) == 0) {
        return 8;
    }

    HDC dc = GetDC(window);
    if (dc == NULL || ReleaseDC(window, dc) != 1) return 9;
    HDC screen = GetDC(NULL);
    if (screen == NULL || ReleaseDC(NULL, screen) != 1) return 10;

    if (GetSystemMetrics(SM_CXSCREEN) != 640 ||
        GetSystemMetrics(SM_CYSCREEN) != 480 ||
        GetSystemMetrics(SM_CXBORDER) != 1 ||
        GetSystemMetrics(SM_CXVSCROLL) != 16 ||
        GetSysColor(COLOR_WINDOW) != RGB(255, 255, 255) ||
        GetSysColor(COLOR_WINDOWTEXT) != RGB(0, 0, 0) ||
        GetSysColor(COLOR_BTNFACE) != RGB(192, 192, 192)) {
        return 11;
    }
    HDC color_dc = CreateCompatibleDC(NULL);
    HBITMAP color_bitmap = CreateCompatibleBitmap(color_dc, 2, 2);
    RECT color_rect = {0, 0, 2, 2};
    if (color_dc == NULL || color_bitmap == NULL ||
        SelectObject(color_dc, color_bitmap) == NULL ||
        GetSysColorBrush(COLOR_MENU) == NULL ||
        GetSysColorBrush(COLOR_MENU) != GetSysColorBrush(COLOR_MENU) ||
        FillRect(color_dc, &color_rect, GetSysColorBrush(COLOR_MENU)) == 0 ||
        GetPixel(color_dc, 1, 1) != GetSysColor(COLOR_MENU) ||
        DeleteObject(GetSysColorBrush(COLOR_MENU)) != FALSE) {
        return 11;
    }
    DeleteDC(color_dc);
    DeleteObject(color_bitmap);

    if ((VkKeyScanA('+') & 0xff) != VK_OEM_PLUS ||
        (VkKeyScanA('+') & 0x0100) == 0 ||
        (VkKeyScanA('-') & 0xff) != VK_OEM_MINUS ||
        (VkKeyScanA('*') & 0xff) != '8' ||
        (VkKeyScanA('=') & 0xff) != VK_OEM_PLUS ||
        (VkKeyScanW('?') & 0xff) != VK_OEM_2 ||
        VkKeyScanA('\x80') != -1) {
        return 16;
    }

    const UINT custom_clipboard = RegisterClipboardFormatA("OpusCustom");
    HANDLE clipboard_text = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, 8);
    char *clipboard_bytes = (char *)GlobalLock(clipboard_text);
    if (custom_clipboard == 0 ||
        custom_clipboard != RegisterClipboardFormatA("OpusCustom") ||
        SetClipboardData(CF_TEXT, clipboard_text) != NULL ||
        GetLastError() != ERROR_ACCESS_DENIED ||
        clipboard_text == NULL || clipboard_bytes == NULL) {
        return 12;
    }
    memcpy(clipboard_bytes, "clip", 5);
    GlobalUnlock(clipboard_text);
    if (!OpenClipboard(window) || GetClipboardOwner() != window ||
        !EmptyClipboard() ||
        IsClipboardFormatAvailable(CF_TEXT) ||
        SetClipboardData(CF_TEXT, clipboard_text) != clipboard_text ||
        !IsClipboardFormatAvailable(CF_TEXT) ||
        GetClipboardData(CF_TEXT) != clipboard_text ||
        SetClipboardData(custom_clipboard,
                         (HANDLE)(UINT_PTR)0x1234) !=
            (HANDLE)(UINT_PTR)0x1234 ||
        GetClipboardData(custom_clipboard) != (HANDLE)(UINT_PTR)0x1234 ||
        OpenClipboard(NULL) ||
        GetLastError() != ERROR_ACCESS_DENIED ||
        !CloseClipboard() ||
        CloseClipboard() ||
        GetLastError() != ERROR_ACCESS_DENIED ||
        !OpenClipboard(NULL) ||
        GetClipboardOwner() != NULL ||
        !EmptyClipboard() ||
        IsClipboardFormatAvailable(CF_TEXT) ||
        GetClipboardData(custom_clipboard) != NULL ||
        !CloseClipboard()) {
        return 13;
    }
    if (GlobalFree(clipboard_text) != NULL) return 14;

    HWND child = CreateWindowExA(0, "STATIC", "child", WS_CHILD, 1, 2, 3, 4,
                                 window, NULL, NULL, NULL);
    HWND popup = CreateWindowExA(0, "STATIC", "popup", WS_POPUP, 1, 2, 3, 4,
                                 window, NULL, NULL, NULL);
    HWND id_child = CreateWindowExA(0, "STATIC", "id-child", WS_CHILD, 7, 8,
                                    9, 10, window,
                                    (HMENU)(UINT_PTR)123, NULL,
                                    NULL);
    HWND grandchild = CreateWindowExA(0, "STATIC", "grandchild", WS_CHILD, 1,
                                      1, 2, 2, id_child, NULL, NULL,
                                      NULL);
    WCHAR class_name[16] = {0};
    const WCHAR static_class[] = {'S', 'T', 'A', 'T', 'I', 'C', 0};
    const WCHAR id_child_text[] = {'i', 'd', '-', 'c', 'h', 'i', 'l', 'd', 0};
    if (child == NULL || popup == NULL || GetParent(child) != window ||
        id_child == NULL || grandchild == NULL ||
        GetWindow(popup, GW_OWNER) != window || GetTopWindow(window) != child ||
        !IsChild(window, child) || GetAncestor(child, GA_PARENT) != window ||
        GetAncestor(grandchild, GA_ROOT) != window ||
        GetAncestor(child, GA_ROOTOWNER) != window ||
        GetDlgCtrlID(id_child) != 123 ||
        GetClassNameW(id_child, class_name, 16) != 6 ||
        class_name[0] != 'S' || class_name[5] != 'C' ||
        FindWindowA("OpusUser32Test", "wide") != window ||
        FindWindowExW(window, child, static_class, id_child_text) != id_child ||
        !MessageBeep(MB_OK)) {
        return 15;
    }
    g_enum_count = 0;
    if (!EnumChildWindows(window, CollectWindow, 0) || g_enum_count != 3 ||
        g_enum_windows[0] != child || g_enum_windows[1] != id_child ||
        g_enum_windows[2] != grandchild) {
        return 16;
    }
    g_enum_count = 0;
    if (EnumWindows(StopAfterFirstWindow, 0) || g_enum_count != 1 ||
        g_enum_windows[0] != window ||
        !EnumThreadWindows(0, CollectWindow, 0) || g_enum_count < 2 ||
        !BringWindowToTop(window) || GetActiveWindow() != window) {
        return 17;
    }
    HWND iconic = CreateWindowExA(0, "STATIC", "iconic",
                                  WS_POPUP | WS_MINIMIZE, 0, 0, 1, 1,
                                  NULL, NULL, NULL, NULL);
    HWND zoomed = CreateWindowExA(0, "STATIC", "zoomed",
                                  WS_POPUP | WS_MAXIMIZE, 0, 0, 1, 1,
                                  NULL, NULL, NULL, NULL);
    if (iconic == NULL || zoomed == NULL || !IsIconic(iconic) ||
        !OpenIcon(iconic) || IsIconic(iconic) || !IsWindowVisible(iconic) ||
        !IsZoomed(zoomed)) {
        return 18;
    }
    PAINTSTRUCT paint = {0};
    HDC paint_dc = BeginPaint(child, &paint);
    if (paint_dc == NULL || paint.hdc != paint_dc || !paint.fErase ||
        paint.rcPaint.left != 0 || paint.rcPaint.top != 0 ||
        paint.rcPaint.right != 3 || paint.rcPaint.bottom != 4) {
        return 19;
    }
    EndPaint(child, &paint);
    if (paint.hdc != NULL) return 20;
    MSG paint_message = {0};
    if (!InvalidateRect(window, NULL, FALSE) ||
        !PeekMessageA(&paint_message, window, WM_PAINT, WM_PAINT, PM_REMOVE) ||
        paint_message.message != WM_PAINT || paint_message.hwnd != window) {
        return 21;
    }
    const int previous_paint_count = g_paint_count;
    if (!RedrawWindow(window, NULL, NULL,
                      RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN) ||
        g_paint_count < previous_paint_count + 1 ||
        PeekMessageA(&paint_message, window, WM_PAINT, WM_PAINT, PM_REMOVE)) {
        return 22;
    }
    if (SetScrollPos(window, SB_VERT, 12, FALSE) != 0 ||
        SetScrollPos(window, SB_VERT, 25, FALSE) != 12) {
        return 23;
    }
    SetScrollRange(window, SB_VERT, 0, 100, FALSE);
    SetScrollRange(window, SB_HORZ, -10, 10, FALSE);
    if (SetScrollPos(window, SB_HORZ, -3, FALSE) != 0 ||
        SetScrollPos(window, SB_VERT, 40, TRUE) != 25 ||
        SetScrollPos(window, SB_HORZ, 7, TRUE) != -3 ||
        SetScrollPos(window, SB_VERT, 150, TRUE) != 40 ||
        SetScrollPos(window, SB_VERT, 60, TRUE) != 100 ||
        SetScrollPos(NULL, SB_VERT, 1, FALSE) != 0 ||
        SetScrollPos(window, 99, 1, FALSE) != 0) {
        return 23;
    }

    RECT client = {0};
    RECT window_rect = {0};
    POINT point = {2, 3};
    RECT rect_a = {0, 0, 10, 10};
    RECT rect_b = {5, 6, 12, 7};
    RECT rect_out = {1, 1, 1, 1};
    RECT rect_empty = {20, 20, 20, 21};
    RECT rect_union = {0, 0, 0, 0};
    if (!GetClientRect(child, &client) || client.left != 0 ||
        client.top != 0 || client.right != 3 || client.bottom != 4 ||
        !ClientToScreen(child, &point) || point.x != 13 || point.y != 25 ||
        !ScreenToClient(child, &point) || point.x != 2 || point.y != 3 ||
        !SetRect(&rect_out, -1, 2, 3, 4) || rect_out.left != -1 ||
        rect_out.top != 2 || rect_out.right != 3 || rect_out.bottom != 4 ||
        !InflateRect(&rect_out, 2, -1) || rect_out.left != -3 ||
        rect_out.top != 3 || rect_out.right != 5 || rect_out.bottom != 3 ||
        !UnionRect(&rect_union, &rect_a, &rect_b) ||
        rect_union.left != 0 || rect_union.top != 0 ||
        rect_union.right != 12 || rect_union.bottom != 10 ||
        !UnionRect(&rect_union, &rect_empty, &rect_b) ||
        rect_union.left != 5 || rect_union.top != 6 ||
        rect_union.right != 12 || rect_union.bottom != 7 ||
        UnionRect(&rect_union, &rect_empty, &rect_empty) ||
        rect_union.left != 0 || rect_union.top != 0 ||
        rect_union.right != 0 || rect_union.bottom != 0 ||
        !PtInRect(&rect_a, (POINT){9, 9}) || PtInRect(&rect_a, (POINT){10, 9}) ||
        !IntersectRect(&rect_out, &rect_a, &rect_b) ||
        rect_out.left != 5 || rect_out.top != 6 || rect_out.right != 10 ||
        rect_out.bottom != 7 || IntersectRect(&rect_out, &rect_a, &rect_empty) ||
        rect_out.left != 0 || rect_out.top != 0 || rect_out.right != 0 ||
        rect_out.bottom != 0 || !OffsetRect(&rect_a, 1, -2) ||
        rect_a.left != 1 || rect_a.top != -2 || rect_a.right != 11 ||
        rect_a.bottom != 8 || IsWindowVisible(child) ||
        ShowWindow(window, SW_SHOW) || ShowWindow(child, SW_SHOW) ||
        !IsWindowVisible(child) || !ShowWindow(window, SW_HIDE) ||
        IsWindowVisible(child) || !MoveWindow(child, 4, 5, 6, 7, TRUE) ||
        !GetWindowRect(child, &window_rect) || window_rect.left != 14 ||
        window_rect.top != 25 || window_rect.right != 20 ||
        window_rect.bottom != 32 || !GetClientRect(child, &client) ||
        client.right != 6 || client.bottom != 7) {
        return 15;
    }

    if (!EnableWindow(window, FALSE) || IsWindowEnabled(window)) return 16;
    if (SetFocus(window) != NULL || GetFocus() != window) return 17;

    HCURSOR cursor_a = (HCURSOR)(UINT_PTR)0x1010;
    HCURSOR cursor_b = (HCURSOR)(UINT_PTR)0x2020;
    BYTE cursor_and_bits[4] = {0};
    BYTE cursor_xor_bits[4] = {0};
    POINT cursor_position = {0};
    HHOOK (*set_windows_hook)(int, FARPROC) = SetWindowsHook;
    BOOL (*unhook_windows_hook)(int, FARPROC) = UnhookWindowsHook;
    LRESULT (*def_hook_proc)(int, WPARAM, LPARAM, HHOOK *) = DefHookProc;
    if (CreateCursor(NULL, 0, 0, 1, 1, cursor_and_bits,
                     cursor_xor_bits) == NULL ||
        CreateIcon(NULL, 1, 1, 1, 1, cursor_and_bits,
                   cursor_xor_bits) == NULL ||
        set_windows_hook(WH_MSGFILTER, NULL) != NULL ||
        !unhook_windows_hook(WH_MSGFILTER, NULL) ||
        def_hook_proc(0, 0, 0, NULL) != 0 ||
        SetCursor(cursor_a) != NULL || SetCursor(cursor_b) != cursor_a ||
        ShowCursor(FALSE) != -1 || GetSystemMetrics(SM_CURSORLEVEL) != -1 ||
        ShowCursor(TRUE) != 0 || GetSystemMetrics(SM_CURSORLEVEL) != 0 ||
        GetMessageTime() == 0 || GetDoubleClickTime() != 500 ||
        SetCapture(window) != NULL || GetCapture() != window ||
        SetCapture(child) != window || GetCapture() != child ||
        !ReleaseCapture() || GetCapture() != NULL ||
        !SetCursorPos(31, 41) || !GetCursorPos(&cursor_position) ||
        cursor_position.x != 31 || cursor_position.y != 41 ||
        GetCursorPos(NULL) || SetCapture(child) != NULL ||
        !DestroyWindow(child) || GetCapture() != NULL) {
        return 18;
    }

    if (EnableWindow(window, TRUE) || !IsWindowEnabled(window) ||
        ShowWindow(window, SW_SHOW)) {
        return 19;
    }
    HWND hit_child = CreateWindowExA(0, "STATIC", "hit-child",
                                     WS_CHILD | WS_VISIBLE, 4, 5, 6, 7,
                                     window, NULL, NULL, NULL);
    if (hit_child == NULL) return 20;
    if (DefWindowProcA(window, WM_NCHITTEST, 0, MAKELPARAM(15, 26)) !=
        HTCLIENT) {
        return 21;
    }
    if (DefWindowProcA(window, WM_NCHITTEST, 0, MAKELPARAM(500, 500)) !=
        HTNOWHERE) {
        return 22;
    }
    if (WindowFromPoint((POINT){15, 26}) != hit_child) return 23;
    if (!EnableWindow(hit_child, FALSE)) return 24;
    if (WindowFromPoint((POINT){15, 26}) != window) return 25;
    if (EnableWindow(hit_child, TRUE) || !IsWindowEnabled(hit_child)) return 26;
    if (WindowFromPoint((POINT){50, 50}) != window) return 27;
    if (WindowFromPoint((POINT){500, 500}) != NULL) return 28;

    RECT adjusted = {0, 0, 100, 50};
    if (!AdjustWindowRectEx(&adjusted, WS_CAPTION | WS_BORDER, FALSE,
                            WS_EX_DLGMODALFRAME) ||
        adjusted.left >= 0 || adjusted.top >= 0 || adjusted.right <= 100 ||
        adjusted.bottom <= 50) {
        return 26;
    }
    adjusted = (RECT){0, 0, 100, 50};
    if (!AdjustWindowRect(&adjusted, WS_CAPTION | WS_BORDER, FALSE) ||
        adjusted.left >= 0 || adjusted.top >= 0 || adjusted.right <= 100 ||
        adjusted.bottom <= 50) {
        return 26;
    }
    RECT work_area = {0};
    if (!SystemParametersInfoA(SPI_GETWORKAREA, 0, &work_area, 0) ||
        work_area.right != 640 || work_area.bottom != 480) {
        return 27;
    }

    if (DefWindowProcA(window, WM_USER + 99, 0, 0) != 0) return 28;
    if (SendMessageA(window, WM_USER + 1, 0, 35) != 42 ||
        g_user_message_count != 2) {
        return 29;
    }

    MSG message = {0};
    if (!PostMessageA(window, WM_USER + 2, 3, 4) ||
        !PeekMessageA(&message, NULL, WM_USER + 2, WM_USER + 2,
                      PM_NOREMOVE) ||
        message.hwnd != window || message.wParam != 3 || message.lParam != 4 ||
        !PeekMessageA(&message, window, WM_USER + 2, WM_USER + 2,
                      PM_REMOVE) ||
        PeekMessageA(&message, window, WM_USER + 2, WM_USER + 2, PM_REMOVE)) {
        return 30;
    }
    HWND message_child = CreateWindowExA(0, "STATIC", "message-child", WS_CHILD,
                                         1, 2, 3, 4, window, NULL, NULL,
                                         NULL);
    if (message_child == NULL ||
        !PostMessageW(message_child, WM_USER + 3, 5, 6) ||
        PeekMessageA(&message, window, WM_USER + 3, WM_USER + 3, PM_REMOVE) ||
        !PeekMessageA(&message, message_child, WM_USER + 3, WM_USER + 3,
                      PM_REMOVE) ||
        message.hwnd != message_child || message.wParam != 5 ||
        message.lParam != 6) {
        return 31;
    }

    BYTE key_state[256] = {0};
    key_state[VK_SHIFT] = 0x80;
    if (!SetKeyboardState(key_state) || GetKeyState(VK_SHIFT) >= 0 ||
        !PostMessageA(window, WM_KEYDOWN, 'A', 1) ||
        GetKeyState('A') >= 0 || GetAsyncKeyState('A') >= 0 ||
        !PeekMessageA(&message, window, WM_KEYDOWN, WM_KEYDOWN, PM_REMOVE) ||
        (OpusUser32ExpectScriptedChar('A', 2), OpusUser32ScriptedCharMatched()) ||
        !TranslateMessage(&message) ||
        OpusUser32ScriptedCharMatched() ||
        !PeekMessageA(&message, window, WM_CHAR, WM_CHAR, PM_REMOVE) ||
        OpusUser32ScriptedCharMatched() ||
        !PostMessageA(window, WM_KEYDOWN, 'A', 1) ||
        !PeekMessageA(&message, window, WM_KEYDOWN, WM_KEYDOWN, PM_REMOVE) ||
        !TranslateMessage(&message) ||
        !PeekMessageA(&message, window, WM_CHAR, WM_CHAR, PM_REMOVE) ||
        !OpusUser32ScriptedCharMatched() ||
        message.wParam != 'A' || DispatchMessageA(&message) != 0 ||
        g_last_char != 'A' || !PostMessageA(window, WM_KEYUP, 'A', 1) ||
        !PeekMessageA(&message, window, WM_KEYUP, WM_KEYUP, PM_REMOVE) ||
        GetKeyState('A') < 0 || GetAsyncKeyState('A') < 0 ||
        GetAsyncKeyState(300) != 0 ||
        !PostMessageA(window, WM_LBUTTONDOWN, 0, 0) ||
        GetAsyncKeyState(VK_LBUTTON) >= 0 ||
        !PeekMessageA(&message, window, WM_LBUTTONDOWN, WM_LBUTTONDOWN,
                      PM_REMOVE) ||
        !PostMessageA(window, WM_LBUTTONUP, 0, 0) ||
        !PeekMessageA(&message, window, WM_LBUTTONUP, WM_LBUTTONUP,
                      PM_REMOVE) ||
        GetAsyncKeyState(VK_LBUTTON) < 0) {
        return 32;
    }

    PostQuitMessage(23);
    if (GetMessageA(&message, NULL, 0, 0) != 0 ||
        message.message != WM_QUIT || message.wParam != 23) {
        return 33;
    }
    if (PeekMessageA(&message, NULL, 0, 0, PM_REMOVE)) return 34;

    g_last_char = 0;
    g_last_keyup = 0;
    BYTE clear_key_state[256] = {0};
    if (!SetKeyboardState(clear_key_state)) return 35;
    OpusUser32PushScriptedInput(window, WM_KEYDOWN, 'B', 1);
    OpusUser32PushScriptedInput(window, WM_KEYUP, 'B', 2);
    while (GetMessageA(&message, NULL, 0, 0)) {
        TranslateMessage(&message);
        DispatchMessageA(&message);
    }
    if (message.message != WM_QUIT || message.wParam != 7 ||
        g_last_char != 'b' || g_last_keyup != 'B') {
        return 36;
    }
    if (PeekMessageA(&message, NULL, 0, 0, PM_REMOVE)) return 37;

    OpusUser32PushScriptedInput(window, WM_USER + 5, 8, 9);
    if (!PeekMessageA(&message, NULL, WM_USER + 5, WM_USER + 5,
                      PM_REMOVE) ||
        message.hwnd != window || message.wParam != 8 || message.lParam != 9) {
        return 38;
    }
    PostQuitMessage(11);
    if (!PeekMessageA(&message, window, WM_USER + 6, WM_USER + 6,
                      PM_NOREMOVE) ||
        message.message != WM_QUIT || message.wParam != 11 ||
        !PeekMessageA(&message, NULL, 0, 0, PM_NOREMOVE) ||
        message.message != WM_QUIT || message.wParam != 11 ||
        !PeekMessageA(&message, NULL, 0, 0, PM_NOREMOVE) ||
        message.message != WM_QUIT || message.wParam != 11 ||
        !PeekMessageA(&message, NULL, 0, 0, PM_REMOVE) ||
        message.message != WM_QUIT || message.wParam != 11 ||
        PeekMessageA(&message, NULL, 0, 0, PM_REMOVE)) {
        return 39;
    }

    if (SetTimer(window, 77, 1, NULL) != 77) return 40;
    if (!WaitMessage() ||
        !PeekMessageA(&message, window, WM_TIMER, WM_TIMER, PM_NOREMOVE) ||
        message.hwnd != window || message.wParam != 77 ||
        !GetMessageA(&message, window, WM_TIMER, WM_TIMER) ||
        message.message != WM_TIMER || message.wParam != 77 ||
        !KillTimer(window, 77)) {
        return 41;
    }
    Sleep(2);
    if (PeekMessageA(&message, window, WM_TIMER, WM_TIMER, PM_REMOVE) ||
        KillTimer(window, 77)) {
        return 42;
    }

    HMENU system_menu = GetSystemMenu(window, FALSE);
    if (system_menu == NULL || !IsMenu(system_menu) ||
        GetSystemMenu(window, FALSE) != system_menu ||
        GetMenuState(system_menu, SC_CLOSE, MF_BYCOMMAND) == (UINT)(-1) ||
        GetMenuItemID(system_menu, 6) != SC_CLOSE ||
        GetSystemMenu(NULL, FALSE) != NULL ||
        GetSystemMenu(window, TRUE) != NULL ||
        IsMenu(system_menu)) {
        return 43;
    }
    HMENU recreated_system_menu = GetSystemMenu(window, FALSE);
    if (recreated_system_menu == NULL || recreated_system_menu == system_menu ||
        !IsMenu(recreated_system_menu)) {
        return 44;
    }

    HMENU menu = CreateMenu();
    HMENU popup_menu = CreatePopupMenu();
    HMENU detached_menu = CreatePopupMenu();
    HMENU holder_menu = CreateMenu();
    const WCHAR item_two[] = {'T', 'w', 'o', 0};
    WCHAR menu_text[16] = {0};
    if (menu == NULL || popup_menu == NULL || detached_menu == NULL ||
        holder_menu == NULL || !IsMenu(menu) || !IsMenu(popup_menu) ||
        GetMenuItemCount(popup_menu) != 0 ||
        !AppendMenuA(popup_menu, MF_STRING, 101, "One") ||
        !AppendMenuW(popup_menu, MF_STRING, 102, item_two) ||
        !InsertMenuA(popup_menu, 0, MF_BYPOSITION | MF_STRING, 100, "Zero") ||
        GetMenuItemCount(popup_menu) != 3 ||
        GetMenuItemID(popup_menu, 0) != 100 ||
        !ModifyMenuA(popup_menu, 101, MF_BYCOMMAND | MF_STRING, 111,
                     "OneOne") ||
        GetMenuStringA(popup_menu, 0, (LPSTR)(text), 4,
                       MF_BYPOSITION) != 3 ||
        text[0] != 'Z' || text[2] != 'r' || text[3] != '\0' ||
        GetMenuStringW(popup_menu, 111, menu_text, 16, MF_BYCOMMAND) != 6 ||
        menu_text[0] != 'O' || menu_text[5] != 'e' ||
        CheckMenuItem(popup_menu, 111, MF_BYCOMMAND | MF_CHECKED) !=
            MF_UNCHECKED ||
        (GetMenuState(popup_menu, 111, MF_BYCOMMAND) & MF_CHECKED) == 0 ||
        !CheckMenuRadioItem(popup_menu, 100, 111, 100, MF_BYCOMMAND) ||
        (GetMenuState(popup_menu, 100, MF_BYCOMMAND) & MF_CHECKED) == 0 ||
        (GetMenuState(popup_menu, 111, MF_BYCOMMAND) & MF_CHECKED) != 0 ||
        !AppendMenuA(popup_menu, MF_SEPARATOR, 0, NULL) ||
        !ModifyMenuA(popup_menu, 0, MF_BYCOMMAND | MF_SEPARATOR,
                     (UINT_PTR)(-1), NULL) ||
        ModifyMenuA(popup_menu, 0, MF_BYCOMMAND | MF_SEPARATOR,
                    (UINT_PTR)(-1), NULL) ||
        GetMenuItemID(popup_menu, 3) != (UINT)(-1) ||
        !DeleteMenu(popup_menu, 3, MF_BYPOSITION) ||
        !RemoveMenu(popup_menu, 100, MF_BYCOMMAND) ||
        GetMenuItemCount(popup_menu) != 2 ||
        !AppendMenuA(menu, MF_POPUP, (UINT_PTR)(popup_menu),
                     "Popup") ||
        GetSubMenu(menu, 0) != popup_menu ||
        GetMenuItemID(menu, 0) != (UINT)(-1) ||
        ((GetMenuState(menu, 0, MF_BYPOSITION) >> 8u) & 0xffu) != 2 ||
        !SetMenuInfo(menu, NULL) ||
        !SetMenuItemBitmaps(popup_menu, 0, MF_BYPOSITION, NULL, NULL) ||
        !SetMenu(window, menu) || GetMenu(window) != menu ||
        !DrawMenuBar(window) ||
        TrackPopupMenu(popup_menu, TPM_RETURNCMD, 0, 0, 0, window, NULL) !=
            0) {
        return 45;
    }
    if (!AppendMenuA(detached_menu, MF_STRING, 201, "Detached") ||
        !AppendMenuA(holder_menu, MF_POPUP,
                     (UINT_PTR)(detached_menu), "Holder") ||
        !RemoveMenu(holder_menu, 0, MF_BYPOSITION) ||
        !DestroyMenu(holder_menu) || !IsMenu(detached_menu) ||
        !DestroyMenu(detached_menu) || !DestroyMenu(menu) || IsMenu(menu) ||
        IsMenu(popup_menu) || GetMenu(window) != NULL) {
        return 46;
    }

    const WCHAR prop_name[] = {'W', 'o', 'r', 'd', 0};
    const WCHAR prop_lookup[] = {'w', 'O', 'R', 'D', 0};
    const HANDLE prop_value = (HANDLE)(UINT_PTR)0x1234;
    const HANDLE prop_replacement = (HANDLE)(UINT_PTR)0x5678;
    const LPCWSTR atom_prop = (LPCWSTR)(UINT_PTR)9;
    const HANDLE atom_value = (HANDLE)(UINT_PTR)0x9abc;
    if (!SetPropW(window, prop_name, prop_value) ||
        GetPropW(window, prop_lookup) != prop_value ||
        GetPropA(window, "word") != prop_value ||
        !SetPropW(window, prop_lookup, prop_replacement) ||
        GetPropW(window, prop_name) != prop_replacement ||
        RemovePropW(window, prop_name) != prop_replacement ||
        GetPropW(window, prop_name) != NULL ||
        !SetPropW(window, atom_prop, atom_value) ||
        GetPropW(window, atom_prop) != atom_value ||
        RemovePropW(window, atom_prop) != atom_value ||
        SetPropW(NULL, prop_name, prop_value) ||
        SetPropW(window, NULL, prop_value) ||
        GetPropW(NULL, prop_name) != NULL ||
        RemovePropW(window, prop_name) != NULL) {
        return 47;
    }

    if (!PostMessageA(message_child, WM_USER + 4, 0, 0) ||
        !DestroyWindow(window) || IsWindow(window) || IsWindow(message_child) ||
        PeekMessageA(&message, NULL, 0, 0, PM_REMOVE)) {
        return 48;
    }
    HWND second = CreateWindowExA(0, "STATIC", "second", WS_POPUP, 0, 0, 1, 1,
                                  NULL, NULL, NULL, NULL);
    if (second == NULL || second == window) return 49;
    return 0;
}
