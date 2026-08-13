#include "windows.h"

namespace {

constexpr LONG_PTR kUserData = 0x12345678;
constexpr LONG_PTR kExtraData = 0x123456789;
int g_nc_create_count = 0;
int g_create_count = 0;
int g_user_message_count = 0;
WPARAM g_last_char = 0;

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
    if (!EnableWindow(window, FALSE) || IsWindowEnabled(window)) return 12;
    if (SetFocus(window) != nullptr || GetFocus() != window) return 13;

    RECT adjusted{0, 0, 100, 50};
    if (!AdjustWindowRectEx(&adjusted, WS_CAPTION | WS_BORDER, FALSE,
                            WS_EX_DLGMODALFRAME) ||
        adjusted.left >= 0 || adjusted.top >= 0 || adjusted.right <= 100 ||
        adjusted.bottom <= 50) {
        return 14;
    }
    RECT work_area{};
    if (!SystemParametersInfoA(SPI_GETWORKAREA, 0, &work_area, 0) ||
        work_area.right != 640 || work_area.bottom != 480) {
        return 15;
    }

    if (DefWindowProcA(window, WM_USER + 99, 0, 0) != 0) return 16;
    if (SendMessageA(window, WM_USER + 1, 0, 35) != 42 ||
        g_user_message_count != 1) {
        return 17;
    }

    MSG message{};
    if (!PostMessageA(window, WM_USER + 2, 3, 4) ||
        !PeekMessageA(&message, nullptr, WM_USER + 2, WM_USER + 2,
                      PM_NOREMOVE) ||
        message.hwnd != window || message.wParam != 3 || message.lParam != 4 ||
        !PeekMessageA(&message, window, WM_USER + 2, WM_USER + 2,
                      PM_REMOVE) ||
        PeekMessageA(&message, window, WM_USER + 2, WM_USER + 2, PM_REMOVE)) {
        return 18;
    }
    if (!PostMessageW(child, WM_USER + 3, 5, 6) ||
        PeekMessageA(&message, window, WM_USER + 3, WM_USER + 3, PM_REMOVE) ||
        !PeekMessageA(&message, child, WM_USER + 3, WM_USER + 3, PM_REMOVE) ||
        message.hwnd != child || message.wParam != 5 || message.lParam != 6) {
        return 19;
    }

    BYTE key_state[256]{};
    key_state[VK_SHIFT] = 0x80;
    if (!SetKeyboardState(key_state) || GetKeyState(VK_SHIFT) >= 0 ||
        !PostMessageA(window, WM_KEYDOWN, 'A', 1) ||
        GetKeyState('A') >= 0 ||
        !PeekMessageA(&message, window, WM_KEYDOWN, WM_KEYDOWN, PM_REMOVE) ||
        !TranslateMessage(&message) ||
        !PeekMessageA(&message, window, WM_CHAR, WM_CHAR, PM_REMOVE) ||
        message.wParam != 'A' || DispatchMessageA(&message) != 0 ||
        g_last_char != 'A' || !PostMessageA(window, WM_KEYUP, 'A', 1) ||
        !PeekMessageA(&message, window, WM_KEYUP, WM_KEYUP, PM_REMOVE) ||
        GetKeyState('A') < 0) {
        return 20;
    }

    PostQuitMessage(23);
    if (GetMessageA(&message, nullptr, 0, 0) != 0 ||
        message.message != WM_QUIT || message.wParam != 23) {
        return 21;
    }

    if (!PostMessageA(child, WM_USER + 4, 0, 0) || !DestroyWindow(window) ||
        IsWindow(window) || IsWindow(child) ||
        PeekMessageA(&message, nullptr, 0, 0, PM_REMOVE)) {
        return 22;
    }
    HWND second = CreateWindowExA(0, "STATIC", "second", WS_POPUP, 0, 0, 1, 1,
                                  nullptr, nullptr, nullptr, nullptr);
    if (second == nullptr || second == window) return 23;
    return 0;
}
