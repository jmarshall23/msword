#include "windows.h"

#include "opusinputscript.h"

namespace {

constexpr LONG_PTR kUserData = 0x12345678;
constexpr LONG_PTR kExtraData = 0x123456789;
int g_nc_create_count = 0;
int g_create_count = 0;
int g_user_message_count = 0;
WPARAM g_last_char = 0;
WPARAM g_last_keyup = 0;

LRESULT CALLBACK TestWindowProc(HWND window, UINT message, WPARAM wparam,
                                LPARAM parameter) {
    if (message == WM_NCCREATE) {
        ++g_nc_create_count;
        const auto* create = reinterpret_cast<const CREATESTRUCTA*>(parameter);
        SetWindowLongPtrA(window, GWLP_USERDATA,
                          reinterpret_cast<LONG_PTR>(create->lpCreateParams));
        return DefWindowProcA(window, message, 0, parameter);
    }
    if (message == WM_CREATE) {
        ++g_create_count;
        return 0;
    }
    if (message == WM_USER + 1) {
        ++g_user_message_count;
        return static_cast<LRESULT>(parameter + 7);
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

}  // namespace

int main() {
    WNDCLASSEXA cls{};
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
        WS_POPUP | WS_CAPTION, 10, 20, 200, 100, nullptr, nullptr, nullptr,
        reinterpret_cast<LPVOID>(kUserData));
    if (window == nullptr || !IsWindow(window) || g_nc_create_count != 1 ||
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
    if (SetWindowLongA(window, 8, 0x11223344) != 0 ||
        GetWindowLongA(window, 8) != 0x11223344 ||
        SetWindowWord(window, 10, 0xabcd) != 0x1122 ||
        GetWindowLongA(window, 8) != static_cast<LONG>(0xabcd3344u) ||
        SetWindowWord(window, 12, 0x5566) != 0 ||
        GetWindowWord(window, 12) != 0x5566 ||
        GetWindowWord(window, 14) != 0) {
        return 6;
    }

    char text[16]{};
    WCHAR wide_text[16]{};
    const WCHAR wide_title[] = {'w', 'i', 'd', 'e', 0};
    if (!SetWindowTextA(window, "plain") ||
        GetWindowTextLengthA(window) != 5 ||
        GetWindowTextA(window, text, sizeof(text)) != 5 ||
        text[0] != 'p' || text[4] != 'n' ||
        GetWindowTextW(window, wide_text, 16) != 5 ||
        wide_text[0] != 'p' || wide_text[4] != 'n' ||
        !SetWindowTextW(window, wide_title) ||
        GetWindowTextLengthW(window) != 4 ||
        GetWindowTextA(window, text, sizeof(text)) != 4 ||
        text[0] != 'w' || text[3] != 'e') {
        return 7;
    }

    HDC dc = GetDC(window);
    if (dc == nullptr || ReleaseDC(window, dc) != 1) return 8;
    HDC screen = GetDC(nullptr);
    if (screen == nullptr || ReleaseDC(nullptr, screen) != 1) return 9;

    if (GetSystemMetrics(SM_CXSCREEN) != 640 ||
        GetSystemMetrics(SM_CYSCREEN) != 480 ||
        GetSystemMetrics(SM_CXBORDER) != 1 ||
        GetSystemMetrics(SM_CXVSCROLL) != 16 ||
        GetSysColor(COLOR_WINDOW) != RGB(255, 255, 255) ||
        GetSysColor(COLOR_WINDOWTEXT) != RGB(0, 0, 0) ||
        GetSysColor(COLOR_BTNFACE) != RGB(192, 192, 192)) {
        return 10;
    }

    HWND child = CreateWindowExA(0, "STATIC", "child", WS_CHILD, 1, 2, 3, 4,
                                 window, nullptr, nullptr, nullptr);
    HWND popup = CreateWindowExA(0, "STATIC", "popup", WS_POPUP, 1, 2, 3, 4,
                                 window, nullptr, nullptr, nullptr);
    if (child == nullptr || popup == nullptr || GetParent(child) != window ||
        GetWindow(popup, GW_OWNER) != window || GetTopWindow(window) != child ||
        !IsChild(window, child)) {
        return 11;
    }

    RECT client{};
    RECT window_rect{};
    POINT point{2, 3};
    RECT rect_a{0, 0, 10, 10};
    RECT rect_b{5, 6, 12, 7};
    RECT rect_out{1, 1, 1, 1};
    RECT rect_empty{20, 20, 20, 21};
    RECT rect_union{0, 0, 0, 0};
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
        !PtInRect(&rect_a, {9, 9}) || PtInRect(&rect_a, {10, 9}) ||
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
        return 12;
    }

    if (!EnableWindow(window, FALSE) || IsWindowEnabled(window)) return 13;
    if (SetFocus(window) != nullptr || GetFocus() != window) return 14;

    HCURSOR cursor_a = reinterpret_cast<HCURSOR>(0x1010);
    HCURSOR cursor_b = reinterpret_cast<HCURSOR>(0x2020);
    POINT cursor_position{};
    if (SetCursor(cursor_a) != nullptr || SetCursor(cursor_b) != cursor_a ||
        ShowCursor(FALSE) != -1 || GetSystemMetrics(SM_CURSORLEVEL) != -1 ||
        ShowCursor(TRUE) != 0 || GetSystemMetrics(SM_CURSORLEVEL) != 0 ||
        SetCapture(window) != nullptr || GetCapture() != window ||
        SetCapture(child) != window || GetCapture() != child ||
        !ReleaseCapture() || GetCapture() != nullptr ||
        !SetCursorPos(31, 41) || !GetCursorPos(&cursor_position) ||
        cursor_position.x != 31 || cursor_position.y != 41 ||
        GetCursorPos(nullptr) || SetCapture(child) != nullptr ||
        !DestroyWindow(child) || GetCapture() != nullptr) {
        return 15;
    }

    if (EnableWindow(window, TRUE) || !IsWindowEnabled(window) ||
        ShowWindow(window, SW_SHOW)) {
        return 16;
    }
    HWND hit_child = CreateWindowExA(0, "STATIC", "hit-child",
                                     WS_CHILD | WS_VISIBLE, 4, 5, 6, 7,
                                     window, nullptr, nullptr, nullptr);
    if (hit_child == nullptr) return 17;
    if (DefWindowProcA(window, WM_NCHITTEST, 0, MAKELPARAM(15, 26)) !=
        HTCLIENT) {
        return 18;
    }
    if (DefWindowProcA(window, WM_NCHITTEST, 0, MAKELPARAM(500, 500)) !=
        HTNOWHERE) {
        return 19;
    }
    if (WindowFromPoint({15, 26}) != hit_child) return 20;
    if (!EnableWindow(hit_child, FALSE)) return 21;
    if (WindowFromPoint({15, 26}) != window) return 22;
    if (EnableWindow(hit_child, TRUE) || !IsWindowEnabled(hit_child)) return 23;
    if (WindowFromPoint({50, 50}) != window) return 24;
    if (WindowFromPoint({500, 500}) != nullptr) return 25;

    RECT adjusted{0, 0, 100, 50};
    if (!AdjustWindowRectEx(&adjusted, WS_CAPTION | WS_BORDER, FALSE,
                            WS_EX_DLGMODALFRAME) ||
        adjusted.left >= 0 || adjusted.top >= 0 || adjusted.right <= 100 ||
        adjusted.bottom <= 50) {
        return 26;
    }
    RECT work_area{};
    if (!SystemParametersInfoA(SPI_GETWORKAREA, 0, &work_area, 0) ||
        work_area.right != 640 || work_area.bottom != 480) {
        return 27;
    }

    if (DefWindowProcA(window, WM_USER + 99, 0, 0) != 0) return 28;
    if (SendMessageA(window, WM_USER + 1, 0, 35) != 42 ||
        g_user_message_count != 1) {
        return 29;
    }

    MSG message{};
    if (!PostMessageA(window, WM_USER + 2, 3, 4) ||
        !PeekMessageA(&message, nullptr, WM_USER + 2, WM_USER + 2,
                      PM_NOREMOVE) ||
        message.hwnd != window || message.wParam != 3 || message.lParam != 4 ||
        !PeekMessageA(&message, window, WM_USER + 2, WM_USER + 2,
                      PM_REMOVE) ||
        PeekMessageA(&message, window, WM_USER + 2, WM_USER + 2, PM_REMOVE)) {
        return 30;
    }
    HWND message_child = CreateWindowExA(0, "STATIC", "message-child", WS_CHILD,
                                         1, 2, 3, 4, window, nullptr, nullptr,
                                         nullptr);
    if (message_child == nullptr ||
        !PostMessageW(message_child, WM_USER + 3, 5, 6) ||
        PeekMessageA(&message, window, WM_USER + 3, WM_USER + 3, PM_REMOVE) ||
        !PeekMessageA(&message, message_child, WM_USER + 3, WM_USER + 3,
                      PM_REMOVE) ||
        message.hwnd != message_child || message.wParam != 5 ||
        message.lParam != 6) {
        return 31;
    }

    BYTE key_state[256]{};
    key_state[VK_SHIFT] = 0x80;
    if (!SetKeyboardState(key_state) || GetKeyState(VK_SHIFT) >= 0 ||
        !PostMessageA(window, WM_KEYDOWN, 'A', 1) ||
        GetKeyState('A') >= 0 || GetAsyncKeyState('A') >= 0 ||
        !PeekMessageA(&message, window, WM_KEYDOWN, WM_KEYDOWN, PM_REMOVE) ||
        !TranslateMessage(&message) ||
        !PeekMessageA(&message, window, WM_CHAR, WM_CHAR, PM_REMOVE) ||
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
    if (GetMessageA(&message, nullptr, 0, 0) != 0 ||
        message.message != WM_QUIT || message.wParam != 23) {
        return 33;
    }
    if (PeekMessageA(&message, nullptr, 0, 0, PM_REMOVE)) return 34;

    g_last_char = 0;
    g_last_keyup = 0;
    BYTE clear_key_state[256]{};
    if (!SetKeyboardState(clear_key_state)) return 35;
    OpusUser32PushScriptedInput(window, WM_KEYDOWN, 'B', 1);
    OpusUser32PushScriptedInput(window, WM_KEYUP, 'B', 2);
    while (GetMessageA(&message, nullptr, 0, 0)) {
        TranslateMessage(&message);
        DispatchMessageA(&message);
    }
    if (message.message != WM_QUIT || message.wParam != 7 ||
        g_last_char != 'b' || g_last_keyup != 'B') {
        return 36;
    }
    if (PeekMessageA(&message, nullptr, 0, 0, PM_REMOVE)) return 37;

    OpusUser32PushScriptedInput(window, WM_USER + 5, 8, 9);
    if (!PeekMessageA(&message, nullptr, WM_USER + 5, WM_USER + 5,
                      PM_REMOVE) ||
        message.hwnd != window || message.wParam != 8 || message.lParam != 9) {
        return 38;
    }
    PostQuitMessage(11);
    if (!PeekMessageA(&message, window, WM_USER + 6, WM_USER + 6,
                      PM_NOREMOVE) ||
        message.message != WM_QUIT || message.wParam != 11 ||
        !PeekMessageA(&message, nullptr, 0, 0, PM_NOREMOVE) ||
        message.message != WM_QUIT || message.wParam != 11 ||
        !PeekMessageA(&message, nullptr, 0, 0, PM_NOREMOVE) ||
        message.message != WM_QUIT || message.wParam != 11 ||
        !PeekMessageA(&message, nullptr, 0, 0, PM_REMOVE) ||
        message.message != WM_QUIT || message.wParam != 11 ||
        PeekMessageA(&message, nullptr, 0, 0, PM_REMOVE)) {
        return 39;
    }

    if (SetTimer(window, 77, 1, nullptr) != 77) return 40;
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

    if (!PostMessageA(message_child, WM_USER + 4, 0, 0) ||
        !DestroyWindow(window) || IsWindow(window) || IsWindow(message_child) ||
        PeekMessageA(&message, nullptr, 0, 0, PM_REMOVE)) {
        return 43;
    }
    HWND second = CreateWindowExA(0, "STATIC", "second", WS_POPUP, 0, 0, 1, 1,
                                  nullptr, nullptr, nullptr, nullptr);
    if (second == nullptr || second == window) return 44;
    return 0;
}
