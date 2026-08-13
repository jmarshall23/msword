#include "windows.h"

namespace {

constexpr LONG_PTR kUserData = 0x12345678;
constexpr LONG_PTR kExtraData = 0x123456789;
int g_nc_create_count = 0;
int g_create_count = 0;
int g_user_message_count = 0;

LRESULT CALLBACK TestWindowProc(HWND window, UINT message, WPARAM,
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

    HDC dc = GetDC(window);
    if (dc == nullptr || ReleaseDC(window, dc) != 1) return 6;
    HDC screen = GetDC(nullptr);
    if (screen == nullptr || ReleaseDC(nullptr, screen) != 1) return 7;

    if (GetSystemMetrics(SM_CXSCREEN) != 640 ||
        GetSystemMetrics(SM_CYSCREEN) != 480 ||
        GetSystemMetrics(SM_CXBORDER) != 1 ||
        GetSystemMetrics(SM_CXVSCROLL) != 16 ||
        GetSysColor(COLOR_WINDOW) != RGB(255, 255, 255) ||
        GetSysColor(COLOR_WINDOWTEXT) != RGB(0, 0, 0) ||
        GetSysColor(COLOR_BTNFACE) != RGB(192, 192, 192)) {
        return 8;
    }

    HWND child = CreateWindowExA(0, "STATIC", "child", WS_CHILD, 1, 2, 3, 4,
                                 window, nullptr, nullptr, nullptr);
    HWND popup = CreateWindowExA(0, "STATIC", "popup", WS_POPUP, 1, 2, 3, 4,
                                 window, nullptr, nullptr, nullptr);
    if (child == nullptr || popup == nullptr || GetParent(child) != window ||
        GetWindow(popup, GW_OWNER) != window || !IsChild(window, child)) {
        return 9;
    }
    if (!EnableWindow(window, FALSE) || IsWindowEnabled(window)) return 10;
    if (SetFocus(window) != nullptr || GetFocus() != window) return 11;

    RECT adjusted{0, 0, 100, 50};
    if (!AdjustWindowRectEx(&adjusted, WS_CAPTION | WS_BORDER, FALSE,
                            WS_EX_DLGMODALFRAME) ||
        adjusted.left >= 0 || adjusted.top >= 0 || adjusted.right <= 100 ||
        adjusted.bottom <= 50) {
        return 12;
    }
    RECT work_area{};
    if (!SystemParametersInfoA(SPI_GETWORKAREA, 0, &work_area, 0) ||
        work_area.right != 640 || work_area.bottom != 480) {
        return 13;
    }

    if (DefWindowProcA(window, WM_USER + 99, 0, 0) != 0) return 14;
    if (SendMessageA(window, WM_USER + 1, 0, 35) != 42 ||
        g_user_message_count != 1) {
        return 15;
    }

    MSG message{};
    if (!PostMessageA(window, WM_USER + 2, 3, 4) ||
        !PeekMessageA(&message, nullptr, WM_USER + 2, WM_USER + 2,
                      PM_NOREMOVE) ||
        message.hwnd != window || message.wParam != 3 || message.lParam != 4 ||
        !PeekMessageA(&message, window, WM_USER + 2, WM_USER + 2,
                      PM_REMOVE) ||
        PeekMessageA(&message, window, WM_USER + 2, WM_USER + 2, PM_REMOVE)) {
        return 16;
    }
    if (!PostMessageW(child, WM_USER + 3, 5, 6) ||
        PeekMessageA(&message, window, WM_USER + 3, WM_USER + 3, PM_REMOVE) ||
        !PeekMessageA(&message, child, WM_USER + 3, WM_USER + 3, PM_REMOVE) ||
        message.hwnd != child || message.wParam != 5 || message.lParam != 6) {
        return 17;
    }

    PostQuitMessage(23);
    if (GetMessageA(&message, nullptr, 0, 0) != 0 ||
        message.message != WM_QUIT || message.wParam != 23) {
        return 18;
    }

    if (!PostMessageA(child, WM_USER + 4, 0, 0) || !DestroyWindow(window) ||
        IsWindow(window) || IsWindow(child) ||
        PeekMessageA(&message, nullptr, 0, 0, PM_REMOVE)) {
        return 19;
    }
    HWND second = CreateWindowExA(0, "STATIC", "second", WS_POPUP, 0, 0, 1, 1,
                                  nullptr, nullptr, nullptr, nullptr);
    if (second == nullptr || second == window) return 20;
    return 0;
}
