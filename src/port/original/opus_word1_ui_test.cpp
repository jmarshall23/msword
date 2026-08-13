#define WIN32_LEAN_AND_MEAN
#include "opus_x64_compat.h"

#include <array>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <utility>
#include <vector>

namespace {

constexpr UINT kWmCommand = 0x0111;
constexpr WPARAM kFileNew = 1813;
constexpr WPARAM kFileOpen = 1843;
constexpr WPARAM kFileSaveAs = 1897;
constexpr WPARAM kFileExit = 2095;
constexpr WPARAM kHelpAbout = 182;
constexpr WPARAM kParaCenter = 1355;
constexpr WPARAM kExportPdf = 0x7103;
constexpr UINT kWmOpusX64QuerySelection = WM_APP + 0x351;
constexpr int kKcControl = 0x100;
constexpr LRESULT kEditUndo = 2229;
constexpr LRESULT kEditCut = 2252;
constexpr LRESULT kEditCopy = 2274;
constexpr LRESULT kEditPaste = 2297;
constexpr LRESULT kEditSelectAll = 5106;

struct WindowSearch {
    DWORD process_id;
    LPCWSTR class_name;
    LPCWSTR caption_fragment;
    HWND result;
};

bool wide_contains(LPCWSTR text, LPCWSTR needle) {
    if (text == nullptr || needle == nullptr || *needle == 0) return false;
    for (; *text != 0; ++text) {
        LPCWSTR cursor = text;
        LPCWSTR expected = needle;
        while (*cursor != 0 && *expected != 0 && *cursor == *expected) {
            ++cursor;
            ++expected;
        }
        if (*expected == 0) return true;
    }
    return false;
}

std::basic_string<WCHAR> argument_to_wide(const wchar_t* text) {
    std::basic_string<WCHAR> wide;
    if (text == nullptr) return wide;
    for (; *text != 0; ++text) wide.push_back(static_cast<WCHAR>(*text));
    return wide;
}

BOOL CALLBACK find_window_callback(const HWND window, const LPARAM value) {
    auto& search = *reinterpret_cast<WindowSearch*>(value);
    DWORD process_id = 0;
    GetWindowThreadProcessId(window, &process_id);
    if (process_id != search.process_id) {
        return TRUE;
    }

    WCHAR class_name[128] = {};
    WCHAR caption[512] = {};
    GetClassNameW(window, class_name, static_cast<int>(std::size(class_name)));
    GetWindowTextW(window, caption, static_cast<int>(std::size(caption)));
    if ((search.class_name == nullptr ||
         lstrcmpW(class_name, search.class_name) == 0) &&
        (search.caption_fragment == nullptr ||
         wide_contains(caption, search.caption_fragment))) {
        search.result = window;
        return FALSE;
    }
    return TRUE;
}

HWND find_process_window(const DWORD process_id, LPCWSTR class_name,
                         LPCWSTR caption_fragment) {
    WindowSearch search{process_id, class_name, caption_fragment, nullptr};
    EnumWindows(find_window_callback, reinterpret_cast<LPARAM>(&search));
    return search.result;
}

BOOL CALLBACK log_window_callback(const HWND window, const LPARAM value) {
    const DWORD expected_process_id = static_cast<DWORD>(value);
    DWORD process_id = 0;
    GetWindowThreadProcessId(window, &process_id);
    if (process_id == expected_process_id) {
        wchar_t class_name[128] = {};
        wchar_t caption[512] = {};
        GetClassNameW(window, class_name,
                      static_cast<int>(std::size(class_name)));
        GetWindowTextW(window, caption, static_cast<int>(std::size(caption)));
        std::wcerr << L"window class='" << class_name << L"' caption='"
                   << caption << L"' visible=" << IsWindowVisible(window)
                   << L" enabled=" << IsWindowEnabled(window) << L'\n';
    }
    return TRUE;
}

void log_process_windows(const DWORD process_id) {
    EnumWindows(log_window_callback, static_cast<LPARAM>(process_id));
}

HWND find_descendant_by_class(const HWND parent, LPCWSTR expected_class) {
    for (HWND child = GetWindow(parent, GW_CHILD); child != nullptr;
         child = GetWindow(child, GW_HWNDNEXT)) {
        WCHAR class_name[128] = {};
        if (GetClassNameW(child, class_name,
                          static_cast<int>(std::size(class_name))) != 0 &&
            lstrcmpiW(class_name, expected_class) == 0) {
            return child;
        }
        if (const HWND descendant =
                find_descendant_by_class(child, expected_class);
            descendant != nullptr) {
            return descendant;
        }
    }
    return nullptr;
}

HWND find_visible_descendant_by_class(const HWND parent, LPCWSTR expected_class) {
    for (HWND child = GetWindow(parent, GW_CHILD); child != nullptr;
         child = GetWindow(child, GW_HWNDNEXT)) {
        WCHAR class_name[128] = {};
        if (GetClassNameW(child, class_name,
                          static_cast<int>(std::size(class_name))) != 0 &&
            lstrcmpiW(class_name, expected_class) == 0 &&
            IsWindowVisible(child)) {
            return child;
        }
        if (const HWND descendant =
                find_visible_descendant_by_class(child, expected_class);
            descendant != nullptr) {
            return descendant;
        }
    }
    return nullptr;
}

void collect_descendants_by_class(const HWND parent,
                                  LPCWSTR expected_class,
                                  std::vector<HWND>& matches) {
    for (HWND child = GetWindow(parent, GW_CHILD); child != nullptr;
         child = GetWindow(child, GW_HWNDNEXT)) {
        WCHAR class_name[128] = {};
        if (GetClassNameW(child, class_name,
                          static_cast<int>(std::size(class_name))) != 0 &&
            lstrcmpiW(class_name, expected_class) == 0) {
            matches.push_back(child);
        }
        collect_descendants_by_class(child, expected_class, matches);
    }
}

HWND wait_for_window(const HANDLE process, const DWORD process_id,
                     LPCWSTR class_name,
                     LPCWSTR caption_fragment,
                     const DWORD timeout_ms) {
    const ULONGLONG deadline = GetTickCount64() + timeout_ms;
    do {
        if (WaitForSingleObject(process, 0) == WAIT_OBJECT_0) {
            return nullptr;
        }
        if (const HWND window = find_process_window(
                process_id, class_name, caption_fragment);
            window != nullptr) {
            return window;
        }
        Sleep(50);
    } while (GetTickCount64() < deadline);
    return nullptr;
}

bool wait_for_window_to_close(const HANDLE process, const DWORD process_id,
                              LPCWSTR class_name,
                              const DWORD timeout_ms) {
    const ULONGLONG deadline = GetTickCount64() + timeout_ms;
    do {
        if (WaitForSingleObject(process, 0) == WAIT_OBJECT_0) {
            return false;
        }
        if (find_process_window(process_id, class_name, nullptr) == nullptr) {
            return true;
        }
        Sleep(50);
    } while (GetTickCount64() < deadline);
    return false;
}

bool control_has_class(const HWND dialog, const int id,
                       LPCWSTR expected) {
    const HWND control = GetDlgItem(dialog, id);
    WCHAR class_name[64] = {};
    return control != nullptr &&
           GetClassNameW(control, class_name,
                         static_cast<int>(std::size(class_name))) != 0 &&
           lstrcmpiW(class_name, expected) == 0;
}

std::size_t count_dark_client_pixels(const HWND window,
                                     const int y_first = 0,
                                     const int y_limit = 120) {
    RECT client{};
    if (window == nullptr || !GetClientRect(window, &client)) {
        return 0;
    }
    const int width = (std::min)(client.right, 800L);
    const int top = (std::max)(0, y_first);
    const int bottom = (std::min)(client.bottom, static_cast<LONG>(y_limit));
    const int height = bottom - top;
    if (width <= 0 || height <= 0) {
        return 0;
    }

    const HDC source = GetDC(window);
    const HDC memory = source != nullptr ? CreateCompatibleDC(source) : nullptr;
    const HBITMAP bitmap = memory != nullptr
                               ? CreateCompatibleBitmap(source, width, height)
                               : nullptr;
    if (bitmap == nullptr) {
        if (memory != nullptr) {
            DeleteDC(memory);
        }
        if (source != nullptr) {
            ReleaseDC(window, source);
        }
        return 0;
    }

    const HGDIOBJ previous = SelectObject(memory, bitmap);
    BitBlt(memory, 0, 0, width, height, source, 0, top, SRCCOPY);
    SelectObject(memory, previous);

    BITMAPINFO info{};
    info.bmiHeader.biSize = sizeof(info.bmiHeader);
    info.bmiHeader.biWidth = width;
    info.bmiHeader.biHeight = -height;
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;
    std::vector<DWORD> pixels(static_cast<std::size_t>(width) * height);
    const int rows = GetDIBits(memory, bitmap, 0, height, pixels.data(), &info,
                              DIB_RGB_COLORS);

    DeleteObject(bitmap);
    DeleteDC(memory);
    ReleaseDC(window, source);
    if (rows != height) {
        return 0;
    }

    std::size_t dark = 0;
    for (const DWORD pixel : pixels) {
        const BYTE blue = static_cast<BYTE>(pixel);
        const BYTE green = static_cast<BYTE>(pixel >> 8);
        const BYTE red = static_cast<BYTE>(pixel >> 16);
        if (red < 96 && green < 96 && blue < 96) {
            ++dark;
        }
    }
    return dark;
}

int longest_light_gap(const HWND window, const int x_first, const int x_limit,
                      const int y) {
    const HDC dc = GetDC(window);
    if (dc == nullptr) {
        return x_limit - x_first;
    }
    int longest = 0;
    int current = 0;
    for (int x = x_first; x < x_limit; ++x) {
        const COLORREF color = GetPixel(dc, x, y);
        const bool light = color == CLR_INVALID ||
            (GetRValue(color) > 160 && GetGValue(color) > 160 &&
             GetBValue(color) > 160);
        if (light) {
            longest = (std::max)(longest, ++current);
        } else {
            current = 0;
        }
    }
    ReleaseDC(window, dc);
    return longest;
}

bool post_keyboard_character(const HWND window, const WCHAR character) {
    UINT virtual_key = 0;
    if (character >= OPUSW("a")[0] && character <= OPUSW("z")[0]) {
        virtual_key = static_cast<UINT>(
            character - OPUSW("a")[0] + OPUSW("A")[0]);
    } else if (character >= OPUSW("0")[0] &&
               character <= OPUSW("9")[0]) {
        virtual_key = static_cast<UINT>(character);
    } else if (character == OPUSW(" ")[0]) {
        virtual_key = VK_SPACE;
    } else if (character == OPUSW("\r")[0]) {
        virtual_key = VK_RETURN;
    } else {
        return false;
    }

    const UINT scan_code = MapVirtualKeyW(virtual_key, MAPVK_VK_TO_VSC);
    const LPARAM key_down =
        static_cast<LPARAM>(1u | (static_cast<DWORD>(scan_code) << 16));
    const LPARAM key_up = key_down | 0xC0000000;
    return PostMessageW(window, WM_KEYDOWN, virtual_key, key_down) &&
           PostMessageW(window, WM_KEYUP, virtual_key, key_up);
}

bool wait_for_zoom_state(const HANDLE process, const HWND window,
                         const bool zoomed, const DWORD timeout_ms) {
    const ULONGLONG deadline = GetTickCount64() + timeout_ms;
    do {
        if (WaitForSingleObject(process, 0) == WAIT_OBJECT_0) {
            return false;
        }
        if ((IsZoomed(window) != FALSE) == zoomed) {
            return true;
        }
        Sleep(25);
    } while (GetTickCount64() < deadline);
    return false;
}

bool wait_for_gui_flag(const HANDLE process, const DWORD thread_id,
                       const DWORD flag, const bool set,
                       const DWORD timeout_ms) {
    const ULONGLONG deadline = GetTickCount64() + timeout_ms;
    do {
        if (WaitForSingleObject(process, 0) == WAIT_OBJECT_0) {
            return false;
        }
        GUITHREADINFO gui{};
        gui.cbSize = sizeof(gui);
        if (GetGUIThreadInfo(thread_id, &gui) &&
            (((gui.flags & flag) != 0) == set)) {
            return true;
        }
        Sleep(25);
    } while (GetTickCount64() < deadline);
    return false;
}

bool wait_for_capture(const HANDLE process, const DWORD thread_id,
                      const HWND expected, const DWORD timeout_ms) {
    const ULONGLONG deadline = GetTickCount64() + timeout_ms;
    do {
        if (WaitForSingleObject(process, 0) == WAIT_OBJECT_0) {
            return false;
        }
        GUITHREADINFO gui{};
        gui.cbSize = sizeof(gui);
        if (GetGUIThreadInfo(thread_id, &gui) &&
            gui.hwndCapture == expected) {
            return true;
        }
        Sleep(25);
    } while (GetTickCount64() < deadline);
    return false;
}

bool wait_for_focus(const HANDLE process, const DWORD thread_id,
                    const HWND expected, const DWORD timeout_ms) {
    const ULONGLONG deadline = GetTickCount64() + timeout_ms;
    do {
        if (WaitForSingleObject(process, 0) == WAIT_OBJECT_0) {
            return false;
        }
        GUITHREADINFO gui{};
        gui.cbSize = sizeof(gui);
        if (GetGUIThreadInfo(thread_id, &gui) && gui.hwndFocus == expected) {
            return true;
        }
        Sleep(25);
    } while (GetTickCount64() < deadline);
    return false;
}

bool make_foreground_and_focus(const HWND main_window, const HWND focus,
                               const DWORD target_thread_id) {
    const DWORD current_thread_id = GetCurrentThreadId();
    const HWND old_foreground = GetForegroundWindow();
    const DWORD foreground_thread_id =
        old_foreground != nullptr
            ? GetWindowThreadProcessId(old_foreground, nullptr)
            : 0;
    const BOOL attached_foreground =
        foreground_thread_id != 0 && foreground_thread_id != current_thread_id &&
                foreground_thread_id != target_thread_id
            ? AttachThreadInput(current_thread_id, foreground_thread_id, TRUE)
            : FALSE;
    const BOOL attached_target =
        current_thread_id != target_thread_id
            ? AttachThreadInput(current_thread_id, target_thread_id, TRUE)
            : FALSE;
    ShowWindow(main_window, SW_RESTORE);
    BringWindowToTop(main_window);
    SetForegroundWindow(main_window);
    SetActiveWindow(main_window);
    const bool focused = SetFocus(focus) != nullptr || GetFocus() == focus;
    if (attached_target) {
        AttachThreadInput(current_thread_id, target_thread_id, FALSE);
    }
    if (attached_foreground) {
        AttachThreadInput(current_thread_id, foreground_thread_id, FALSE);
    }
    Sleep(100);
    if (focused && GetForegroundWindow() == main_window) {
        return true;
    }

    RECT client{};
    POINT activation_point{};
    if (!GetClientRect(focus, &client)) {
        return false;
    }
    activation_point.x = (std::min)(client.right - 1, 20L);
    activation_point.y = (std::min)(client.bottom - 1, 10L);
    if (activation_point.x < 0 || activation_point.y < 0 ||
        !ClientToScreen(focus, &activation_point) ||
        !SetCursorPos(activation_point.x, activation_point.y)) {
        return false;
    }
    std::array<INPUT, 2> click{};
    click[0].type = INPUT_MOUSE;
    click[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
    click[1].type = INPUT_MOUSE;
    click[1].mi.dwFlags = MOUSEEVENTF_LEFTUP;
    if (SendInput(static_cast<UINT>(click.size()), click.data(),
                  sizeof(INPUT)) != click.size()) {
        return false;
    }
    Sleep(200);
    GUITHREADINFO gui{};
    gui.cbSize = sizeof(gui);
    return GetForegroundWindow() == main_window &&
           GetGUIThreadInfo(target_thread_id, &gui) && gui.hwndFocus == focus;
}

bool send_virtual_key(const WORD virtual_key) {
    std::array<INPUT, 2> input{};
    input[0].type = INPUT_KEYBOARD;
    input[0].ki.wVk = virtual_key;
    input[1] = input[0];
    input[1].ki.dwFlags = KEYEVENTF_KEYUP;
    return SendInput(static_cast<UINT>(input.size()), input.data(),
                     sizeof(INPUT)) == input.size();
}

bool send_control_key(const WORD virtual_key) {
    std::array<INPUT, 4> input{};
    input[0].type = INPUT_KEYBOARD;
    input[0].ki.wVk = VK_CONTROL;
    input[1].type = INPUT_KEYBOARD;
    input[1].ki.wVk = virtual_key;
    input[2] = input[1];
    input[2].ki.dwFlags = KEYEVENTF_KEYUP;
    input[3] = input[0];
    input[3].ki.dwFlags = KEYEVENTF_KEYUP;

    /* Word's Win16 key loop samples modifier state as each dequeued message is
     * handled. Deliver the transitions separately to model a real held Ctrl key
     * instead of allowing the whole synthetic chord to be released before the
     * target thread samples it.
     */
    for (INPUT& event : input) {
        if (SendInput(1, &event, sizeof(INPUT)) != 1) {
            return false;
        }
        Sleep(35);
    }
    return true;
}

bool execute_control_shortcut(const HWND pane, const WORD virtual_key) {
    return SendMessageW(pane, kWmOpusX64QuerySelection, 80,
                        kKcControl | virtual_key) != 0;
}

bool send_control_shift_key(const WORD virtual_key) {
    std::array<INPUT, 6> input{};
    input[0].type = INPUT_KEYBOARD;
    input[0].ki.wVk = VK_CONTROL;
    input[1].type = INPUT_KEYBOARD;
    input[1].ki.wVk = VK_SHIFT;
    input[2].type = INPUT_KEYBOARD;
    input[2].ki.wVk = virtual_key;
    input[3] = input[2];
    input[3].ki.dwFlags = KEYEVENTF_KEYUP;
    input[4] = input[1];
    input[4].ki.dwFlags = KEYEVENTF_KEYUP;
    input[5] = input[0];
    input[5].ki.dwFlags = KEYEVENTF_KEYUP;
    return SendInput(static_cast<UINT>(input.size()), input.data(),
                     sizeof(INPUT)) == input.size();
}

bool send_physical_text(const std::basic_string<WCHAR>& text) {
    for (const WCHAR character : text) {
        WORD virtual_key = 0;
        if (character >= OPUSW("a")[0] && character <= OPUSW("z")[0]) {
            virtual_key = static_cast<WORD>(character - OPUSW("a")[0] +
                                            OPUSW("A")[0]);
        } else if (character >= OPUSW("0")[0] &&
                   character <= OPUSW("9")[0]) {
            virtual_key = static_cast<WORD>(character);
        } else if (character == OPUSW(" ")[0]) {
            virtual_key = VK_SPACE;
        } else if (character == OPUSW("\r")[0]) {
            virtual_key = VK_RETURN;
        } else {
            return false;
        }
        if (!send_virtual_key(virtual_key)) {
            return false;
        }
        Sleep(character == OPUSW("\r")[0] ? 300 : 50);
    }
    return true;
}

bool send_mouse_button(const DWORD flags) {
    INPUT input{};
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = flags;
    return SendInput(1, &input, sizeof(input)) == 1;
}

bool choose_combo_item_with_mouse(const HWND combo, const LRESULT index) {
    RECT combo_rectangle{};
    if (combo == nullptr || index < 0 ||
        !GetWindowRect(combo, &combo_rectangle)) {
        return false;
    }

    const POINT arrow{
        combo_rectangle.right - 8,
        combo_rectangle.top +
            (combo_rectangle.bottom - combo_rectangle.top) / 2};
    if (!SetCursorPos(arrow.x, arrow.y) ||
        !send_mouse_button(MOUSEEVENTF_LEFTDOWN) ||
        !send_mouse_button(MOUSEEVENTF_LEFTUP)) {
        return false;
    }
    Sleep(250);

    COMBOBOXINFO info{};
    info.cbSize = sizeof(info);
    RECT list_rectangle{};
    const LRESULT item_height = SendMessageW(combo, CB_GETITEMHEIGHT, 0, 0);
    if (!GetComboBoxInfo(combo, &info) || info.hwndList == nullptr ||
        item_height <= 0 || !IsWindowVisible(info.hwndList) ||
        !GetWindowRect(info.hwndList, &list_rectangle)) {
        return false;
    }

    const LRESULT requested_top =
        (std::max)(0LL, static_cast<long long>(index) - 2);
    SendMessageW(info.hwndList, LB_SETTOPINDEX, requested_top, 0);
    UpdateWindow(info.hwndList);
    RECT item_rectangle{};
    if (SendMessageW(info.hwndList, LB_GETITEMRECT, index,
                     reinterpret_cast<LPARAM>(&item_rectangle)) == LB_ERR) {
        return false;
    }
    POINT choice{
        item_rectangle.left +
            (item_rectangle.right - item_rectangle.left) / 2,
        item_rectangle.top +
            (item_rectangle.bottom - item_rectangle.top) / 2};
    if (!ClientToScreen(info.hwndList, &choice) ||
        choice.x < list_rectangle.left || choice.x >= list_rectangle.right ||
        choice.y < list_rectangle.top || choice.y >= list_rectangle.bottom ||
        !SetCursorPos(choice.x, choice.y) ||
        !send_mouse_button(MOUSEEVENTF_LEFTDOWN) ||
        !send_mouse_button(MOUSEEVENTF_LEFTUP)) {
        return false;
    }
    Sleep(350);
    return SendMessageW(combo, CB_GETCURSEL, 0, 0) == index;
}

bool window_is_responsive(const HANDLE process, const HWND window) {
    DWORD_PTR message_result = 0;
    return WaitForSingleObject(process, 0) != WAIT_OBJECT_0 &&
           !IsHungAppWindow(window) &&
           SendMessageTimeoutW(window, WM_NULL, 0, 0, SMTO_ABORTIFHUNG,
                               2000, &message_result) != 0;
}

bool capture_window_bmp(const HWND window, LPCWSTR path) {
    RECT bounds{};
    if (window == nullptr || path == nullptr || !GetWindowRect(window, &bounds))
        return false;
    const int width = bounds.right - bounds.left;
    const int height = bounds.bottom - bounds.top;
    if (width <= 0 || height <= 0) return false;
    HDC screen = GetDC(nullptr);
    HDC memory = screen != nullptr ? CreateCompatibleDC(screen) : nullptr;
    HBITMAP bitmap = memory != nullptr ?
        CreateCompatibleBitmap(screen, width, height) : nullptr;
    HGDIOBJ previous = bitmap != nullptr ? SelectObject(memory, bitmap) : nullptr;
    const bool copied = bitmap != nullptr &&
        BitBlt(memory, 0, 0, width, height, screen, bounds.left, bounds.top,
               SRCCOPY | CAPTUREBLT) != FALSE;
    BITMAPINFO info{};
    info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth = width;
    info.bmiHeader.biHeight = -height;
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;
    std::vector<unsigned char> pixels(
        static_cast<std::size_t>(width) * height * 4);
    const bool read = copied && GetDIBits(
        memory, bitmap, 0, height, pixels.data(), &info, DIB_RGB_COLORS) != 0;
    bool written = false;
    if (read) {
        BITMAPFILEHEADER file_header{};
        file_header.bfType = 0x4d42;
        file_header.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
        file_header.bfSize = file_header.bfOffBits +
            static_cast<DWORD>(pixels.size());
        HANDLE file = CreateFileW(path, GENERIC_WRITE, 0, nullptr,
                                  CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file != INVALID_HANDLE_VALUE) {
            DWORD count = 0;
            written = WriteFile(file, &file_header, sizeof(file_header),
                                &count, nullptr) &&
                      count == sizeof(file_header) &&
                      WriteFile(file, &info.bmiHeader,
                                sizeof(info.bmiHeader), &count, nullptr) &&
                      count == sizeof(info.bmiHeader) &&
                      WriteFile(file, pixels.data(),
                                static_cast<DWORD>(pixels.size()), &count,
                                nullptr) && count == pixels.size();
            CloseHandle(file);
        }
    }
    if (previous != nullptr) SelectObject(memory, previous);
    if (bitmap != nullptr) DeleteObject(bitmap);
    if (memory != nullptr) DeleteDC(memory);
    if (screen != nullptr) ReleaseDC(nullptr, screen);
    return written;
}

int fail(PROCESS_INFORMATION& process, const int code,
         const char* message) {
    std::cerr << message << '\n';
    if (process.hProcess != nullptr &&
        WaitForSingleObject(process.hProcess, 0) != WAIT_OBJECT_0) {
        TerminateProcess(process.hProcess, static_cast<UINT>(code));
        WaitForSingleObject(process.hProcess, 2000);
    }
    if (process.hThread != nullptr) {
        CloseHandle(process.hThread);
    }
    if (process.hProcess != nullptr) {
        CloseHandle(process.hProcess);
    }
    return code;
}

}  // namespace

int run_ui_test(const int argument_count, WCHAR** arguments) {
    SetProcessDpiAwarenessContext(
        DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    if (argument_count < 2 || argument_count > 4) {
        std::cerr <<
            "usage: opus_word1_ui_test WORD1.exe "
            "[--typing|--interaction|--selection|--caret|--formatting|--color|"
            "--font-typing|--unicode|--clipboard|--about|--save-as|--pdf-export|"
            "--docx-open FILE|--docx-format-open FILE|"
            "--docx-pdf-export FILE]\n";
        return 1;
    }
    const std::basic_string<WCHAR> executable_argument =
        arguments[1] != nullptr ? arguments[1] : OPUSW("");
    const std::basic_string<WCHAR> mode_argument =
        argument_count >= 3 && arguments[2] != nullptr ?
            arguments[2] : std::basic_string<WCHAR>{};
    const std::basic_string<WCHAR> docx_argument =
        argument_count >= 4 && arguments[3] != nullptr ?
            arguments[3] : std::basic_string<WCHAR>{};
    const bool typing_mode =
        argument_count == 3 &&
        lstrcmpW(mode_argument.c_str(), OPUSW("--typing")) == 0;
    const bool interaction_mode =
        argument_count == 3 &&
        lstrcmpW(mode_argument.c_str(), OPUSW("--interaction")) == 0;
    const bool selection_mode =
        argument_count == 3 &&
        lstrcmpW(mode_argument.c_str(), OPUSW("--selection")) == 0;
    const bool caret_mode =
        argument_count == 3 &&
        lstrcmpW(mode_argument.c_str(), OPUSW("--caret")) == 0;
    const bool formatting_mode =
        argument_count == 3 &&
        lstrcmpW(mode_argument.c_str(), OPUSW("--formatting")) == 0;
    const bool color_mode =
        argument_count == 3 &&
        lstrcmpW(mode_argument.c_str(), OPUSW("--color")) == 0;
    const bool font_typing_mode =
        argument_count == 3 &&
        lstrcmpW(mode_argument.c_str(), OPUSW("--font-typing")) == 0;
    const bool unicode_mode =
        argument_count == 3 &&
        lstrcmpW(mode_argument.c_str(), OPUSW("--unicode")) == 0;
    const bool about_mode =
        argument_count == 3 &&
        lstrcmpW(mode_argument.c_str(), OPUSW("--about")) == 0;
    const bool clipboard_mode =
        argument_count == 3 &&
        lstrcmpW(mode_argument.c_str(), OPUSW("--clipboard")) == 0;
    const bool save_as_mode =
        argument_count == 3 &&
        lstrcmpW(mode_argument.c_str(), OPUSW("--save-as")) == 0;
    const bool pdf_export_mode =
        argument_count == 3 &&
        lstrcmpW(mode_argument.c_str(), OPUSW("--pdf-export")) == 0;
    const bool docx_pdf_export_mode =
        argument_count == 4 &&
        lstrcmpW(mode_argument.c_str(), OPUSW("--docx-pdf-export")) == 0;
    const bool formatted_docx_open_mode =
        argument_count == 4 &&
        (lstrcmpW(mode_argument.c_str(), OPUSW("--docx-format-open")) == 0 ||
         docx_pdf_export_mode);
    const bool docx_open_mode =
        argument_count == 4 &&
        (lstrcmpW(mode_argument.c_str(), OPUSW("--docx-open")) == 0 ||
         formatted_docx_open_mode);
    if (argument_count == 3 && !typing_mode && !interaction_mode &&
        !selection_mode && !caret_mode && !formatting_mode && !color_mode &&
        !font_typing_mode && !unicode_mode && !clipboard_mode && !about_mode &&
        !save_as_mode && !pdf_export_mode) {
        std::cerr << "unknown test mode\n";
        return 1;
    }
    if (argument_count == 4 && !docx_open_mode) {
        std::cerr << "unknown test mode\n";
        return 1;
    }

    std::string docx_path;
    std::string pdf_path;
    bool keep_pdf = false;
    if (docx_open_mode) {
        const int path_size = WideCharToMultiByte(
            CP_ACP, 0, docx_argument.c_str(), -1, nullptr, 0, nullptr,
            nullptr);
        if (path_size <= 1) {
            std::cerr << "DOCX open test received an invalid path\n";
            return 83;
        }
        docx_path.resize(static_cast<std::size_t>(path_size));
        WideCharToMultiByte(CP_ACP, 0, docx_argument.c_str(), -1,
                            docx_path.data(), path_size, nullptr, nullptr);
        docx_path.pop_back();
        if (GetFileAttributesA(docx_path.c_str()) == INVALID_FILE_ATTRIBUTES) {
            std::cerr << "DOCX open test file does not exist\n";
            return 83;
        }
    }
    if (docx_open_mode) {
        SetEnvironmentVariableA("WORD1_TEST_FILE_DIALOG_PATH",
                                docx_path.c_str());
    }
    if (pdf_export_mode || docx_pdf_export_mode) {
        char requested_pdf[32768]{};
        const DWORD requested_length = GetEnvironmentVariableA(
            "WORD1_TEST_KEEP_PDF", requested_pdf,
            static_cast<DWORD>(std::size(requested_pdf)));
        keep_pdf = requested_length > 0 &&
            requested_length < std::size(requested_pdf);
        if (keep_pdf) {
            pdf_path = requested_pdf;
            DeleteFileA(pdf_path.c_str());
            SetEnvironmentVariableA("WORD1_TEST_PDF_PATH", pdf_path.c_str());
        } else {
        char temporary_directory[MAX_PATH]{};
        char temporary_seed[MAX_PATH]{};
        if (GetTempPathA(static_cast<DWORD>(std::size(temporary_directory)),
                         temporary_directory) == 0 ||
            GetTempFileNameA(temporary_directory, "OWP", 0,
                             temporary_seed) == 0) {
            std::cerr << "PDF export test could not reserve a path\n";
            return 85;
        }
        DeleteFileA(temporary_seed);
        pdf_path = std::string(temporary_seed) + ".pdf";
        SetEnvironmentVariableA("WORD1_TEST_PDF_PATH", pdf_path.c_str());
        }
    }

    std::basic_string<WCHAR> command_line;
    command_line.push_back(OPUSW("\"")[0]);
    command_line += executable_argument;
    command_line.push_back(OPUSW("\"")[0]);
    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION process{};
    LPWCH environment = GetEnvironmentStringsW();
    if (!CreateProcessW(executable_argument.c_str(), command_line.data(),
                        nullptr, nullptr, FALSE, CREATE_UNICODE_ENVIRONMENT,
                        environment, nullptr, &startup, &process)) {
        if (environment != nullptr) FreeEnvironmentStringsW(environment);
        SetEnvironmentVariableA("WORD1_TEST_FILE_DIALOG_PATH", nullptr);
        SetEnvironmentVariableA("WORD1_TEST_PDF_PATH", nullptr);
        std::cerr << "CreateProcessW failed: " << GetLastError() << '\n';
        return 2;
    }
    if (environment != nullptr) FreeEnvironmentStringsW(environment);
    SetEnvironmentVariableA("WORD1_TEST_FILE_DIALOG_PATH", nullptr);
    SetEnvironmentVariableA("WORD1_TEST_PDF_PATH", nullptr);

    const HWND main_window = wait_for_window(
        process.hProcess, process.dwProcessId, nullptr,
        OPUSW("Microsoft Word - Document1"), 8000);
    if (main_window == nullptr) {
        return fail(process, 3, "WORD1 main window did not appear");
    }
    if (pdf_export_mode) {
        const HWND pane = find_descendant_by_class(main_window,
                                                   OPUSW("OpusWwd"));
        DWORD ignored_process_id = 0;
        const DWORD thread_id =
            GetWindowThreadProcessId(main_window, &ignored_process_id);
        const LRESULT initial_length = pane != nullptr ?
            SendMessageW(pane, kWmOpusX64QuerySelection, 41, 0) : 0;
        const bool focused = pane != nullptr &&
            (make_foreground_and_focus(main_window, pane, thread_id) ||
             wait_for_focus(process.hProcess, thread_id, pane, 500));
        bool typed = focused;
        for (const wchar_t character : std::wstring(L"pdf export text")) {
            typed = typed &&
                post_keyboard_character(pane, static_cast<WCHAR>(character));
        }
        Sleep(750);
        const bool committed = typed &&
            SendMessageW(pane, kWmOpusX64QuerySelection, 41, 0) >
                initial_length;
        if (!committed ||
            !PostMessageW(main_window, kWmCommand, kExportPdf, 0)) {
            std::cerr << "PDF preparation: focused=" << focused
                      << " typed=" << typed << " committed=" << committed
                      << " length=" << initial_length << "->"
                      << (pane != nullptr ? SendMessageW(
                              pane, kWmOpusX64QuerySelection, 41, 0) : 0)
                      << '\n';
            if (!keep_pdf) DeleteFileA(pdf_path.c_str());
            return fail(process, 85,
                        "PDF export test could not prepare its document");
        }
        const ULONGLONG deadline = GetTickCount64() + 15000;
        bool pdf_written = false;
        while (!pdf_written && GetTickCount64() < deadline &&
               WaitForSingleObject(process.hProcess, 0) != WAIT_OBJECT_0) {
            HANDLE pdf = CreateFileA(pdf_path.c_str(), GENERIC_READ,
                                     FILE_SHARE_READ | FILE_SHARE_WRITE,
                                     nullptr, OPEN_EXISTING,
                                     FILE_ATTRIBUTE_NORMAL, nullptr);
            if (pdf != INVALID_HANDLE_VALUE) {
                char header[5]{};
                DWORD read = 0;
                pdf_written = ReadFile(pdf, header, sizeof(header), &read,
                                       nullptr) && read == sizeof(header) &&
                              std::memcmp(header, "%PDF-", sizeof(header)) == 0;
                CloseHandle(pdf);
            }
            Sleep(100);
        }
        DWORD exit_code = STILL_ACTIVE;
        GetExitCodeProcess(process.hProcess, &exit_code);
        TerminateProcess(process.hProcess, 0);
        WaitForSingleObject(process.hProcess, 2000);
        CloseHandle(process.hThread);
        CloseHandle(process.hProcess);
        if (!keep_pdf) DeleteFileA(pdf_path.c_str());
        if (!pdf_written) {
            std::cerr << "PDF export failed or crashed: exit=0x" << std::hex
                      << exit_code << std::dec << '\n';
            return 86;
        }
        return 0;
    }
    if (save_as_mode) {
        Sleep(1000);
        if (!PostMessageW(main_window, kWmCommand, kFileSaveAs, 0)) {
            return fail(process, 69, "could not send File Save As");
        }
        const HWND save_as_dialog = wait_for_window(
            process.hProcess, process.dwProcessId, OPUSW("#32770"),
            OPUSW("Save As"), 5000);
        if (save_as_dialog == nullptr) {
            std::cerr << "Save As stage="
                      << reinterpret_cast<INT_PTR>(GetPropA(
                             main_window, "OpusX64SaveAsStage"))
                      << '\n';
            log_process_windows(process.dwProcessId);
            return fail(process, 70,
                        "Windows 95 File Save As dialog did not appear");
        }
        if (!control_has_class(save_as_dialog, 2, OPUSW("Button")) ||
            !window_is_responsive(process.hProcess, save_as_dialog)) {
            return fail(process, 71,
                        "File Save As dialog did not finish initializing");
        }
        const bool cancel_posted =
            PostMessageW(save_as_dialog, kWmCommand, 2, 0) != FALSE;
        const bool dialog_closed = cancel_posted &&
            wait_for_window_to_close(process.hProcess, process.dwProcessId,
                                     OPUSW("#32770"), 5000) &&
            wait_for_window_to_close(process.hProcess, process.dwProcessId,
                                     OPUSW("OpusSdmDialog"), 5000);
        const bool app_responsive = dialog_closed &&
            window_is_responsive(process.hProcess, main_window);
        if (!cancel_posted || !dialog_closed || !app_responsive) {
            DWORD exit_code = STILL_ACTIVE;
            GetExitCodeProcess(process.hProcess, &exit_code);
            std::cerr << "Save As cancel state: posted=" << cancel_posted
                      << " closed=" << dialog_closed
                      << " responsive=" << app_responsive
                      << " exit=0x" << std::hex << exit_code << std::dec
                      << '\n';
            log_process_windows(process.dwProcessId);
            return fail(process, 72,
                        "File Save As dialog did not cancel cleanly");
        }
        TerminateProcess(process.hProcess, 0);
        WaitForSingleObject(process.hProcess, 2000);
        CloseHandle(process.hThread);
        CloseHandle(process.hProcess);
        return 0;
    }
    if (docx_open_mode) {
        HWND pane = find_descendant_by_class(main_window, OPUSW("OpusWwd"));
        if (pane == nullptr ||
            !PostMessageW(main_window, kWmCommand, kFileOpen, 0)) {
            return fail(process, 78, "DOCX open test could not open its document");
        }
        const ULONGLONG open_deadline = GetTickCount64() + 10000;
        WCHAR document_caption[512]{};
        do {
            GetWindowTextW(main_window, document_caption,
                           static_cast<int>(std::size(document_caption)));
            if (!wide_contains(document_caption, OPUSW("Document1"))) {
                break;
            }
            Sleep(100);
        } while (GetTickCount64() < open_deadline &&
                 WaitForSingleObject(process.hProcess, 0) != WAIT_OBJECT_0);

        // The legacy open command updates the caption before it has finished
        // activating the replacement MDI child and document pane.
        Sleep(750);
        pane = find_visible_descendant_by_class(main_window,
                                                OPUSW("OpusWwd"));
        const bool reopened =
            !wide_contains(document_caption, OPUSW("Document1"));
        const LRESULT imported_length = pane != nullptr ?
            SendMessageW(pane, kWmOpusX64QuerySelection, 41, 0) : 0;
        const LRESULT displayed_lines = pane != nullptr ?
            SendMessageW(pane, kWmOpusX64QuerySelection, 30, 0) : 0;
        const LRESULT title_hps = pane != nullptr ?
            SendMessageW(pane, kWmOpusX64QuerySelection, 52, 0) : -1;
        const LRESULT title_bold = pane != nullptr ?
            SendMessageW(pane, kWmOpusX64QuerySelection, 87, 0) : -1;
        const LRESULT title_color = pane != nullptr ?
            SendMessageW(pane, kWmOpusX64QuerySelection, 90, 0) : -1;
        const LRESULT title_alignment = pane != nullptr ?
            SendMessageW(pane, kWmOpusX64QuerySelection, 91, 0) : -1;
        const LRESULT page_width = pane != nullptr ?
            SendMessageW(pane, kWmOpusX64QuerySelection, 97, 0) : -1;
        const LRESULT page_height = pane != nullptr ?
            SendMessageW(pane, kWmOpusX64QuerySelection, 98, 0) : -1;
        const LRESULT left_margin = pane != nullptr ?
            SendMessageW(pane, kWmOpusX64QuerySelection, 99, 0) : -1;
        const LRESULT top_margin = pane != nullptr ?
            SendMessageW(pane, kWmOpusX64QuerySelection, 101, 0) : -1;
        const LRESULT body_font_index = pane != nullptr ?
            SendMessageW(pane, kWmOpusX64QuerySelection, 103, 20) : -1;
        const LRESULT body_font_charset = pane != nullptr ?
            SendMessageW(pane, kWmOpusX64QuerySelection, 104, 20) : -1;
        WCHAR capture_path[32768]{};
        const DWORD capture_length = GetEnvironmentVariableW(
            OPUSW("WORD1_TEST_CAPTURE_PATH"), capture_path,
            static_cast<DWORD>(std::size(capture_path)));
        if (capture_length > 0 && capture_length < std::size(capture_path)) {
            ShowWindow(main_window, SW_RESTORE);
            SetForegroundWindow(main_window);
            UpdateWindow(main_window);
            Sleep(2000);
            if (!capture_window_bmp(main_window, capture_path)) {
                return fail(process, 84, "DOCX capture failed");
            }
        }
        bool rich_pdf_valid = !docx_pdf_export_mode;
        DWORD_PTR export_result = 0;
        if (docx_pdf_export_mode && SendMessageTimeoutW(
                main_window, kWmCommand, kExportPdf, 0,
                SMTO_ABORTIFHUNG | SMTO_BLOCK, 30000, &export_result)) {
            const ULONGLONG pdf_deadline = GetTickCount64() + 15000;
            do {
                std::ifstream input(pdf_path, std::ios::binary);
                if (input) {
                    const std::string pdf_data(
                        std::istreambuf_iterator<char>(input), {});
                    rich_pdf_valid = pdf_data.starts_with("%PDF-") &&
                        pdf_data.find("JUSTIN MARSHALL") != std::string::npos &&
                        pdf_data.find("/Helvetica-Bold") != std::string::npos &&
                        pdf_data.find("0 0 1 rg") != std::string::npos &&
                        pdf_data.find("/MediaBox [0 0 612 792]") !=
                            std::string::npos;
                }
                if (!rich_pdf_valid) Sleep(100);
            } while (!rich_pdf_valid && GetTickCount64() < pdf_deadline &&
                     WaitForSingleObject(process.hProcess, 0) != WAIT_OBJECT_0);
        }
        TerminateProcess(process.hProcess, 0);
        WaitForSingleObject(process.hProcess, 2000);
        CloseHandle(process.hThread);
        CloseHandle(process.hProcess);
        if (!reopened || pane == nullptr || imported_length < 100 ||
            displayed_lines < 1 ||
            (formatted_docx_open_mode &&
             (title_hps < 40 || title_bold != 1 || title_color != 2 ||
              title_alignment != 1 || page_width != 12240 ||
              page_height != 15840 || left_margin != 835 ||
              top_margin != 691 || body_font_index != 2 ||
              body_font_charset != ANSI_CHARSET)) || !rich_pdf_valid) {
            std::cerr << "DOCX open state: length=" << imported_length
                      << " displayedLines=" << displayed_lines
                      << " title={hps:" << title_hps
                      << ",bold:" << title_bold
                      << ",color:" << title_color
                      << ",alignment:" << title_alignment << "}"
                      << " page={" << page_width << "x" << page_height
                      << ",left:" << left_margin << ",top:" << top_margin
                      << "} bodyFont={index:" << body_font_index
                      << ",charset:" << body_font_charset << "}\n";
            if (docx_pdf_export_mode)
                std::cerr << "richPdf=" << rich_pdf_valid
                          << " exportStage="
                          << (pane != nullptr ? SendMessageW(
                                  pane, kWmOpusX64QuerySelection, 105, 0) : -1)
                          << '\n';
            if (!keep_pdf && !pdf_path.empty()) DeleteFileA(pdf_path.c_str());
            return 84;
        }
        if (!keep_pdf && !pdf_path.empty()) DeleteFileA(pdf_path.c_str());
        return 0;
    }
    if (about_mode) {
        Sleep(1000);
        if (!PostMessageW(main_window, kWmCommand, kHelpAbout, 0)) {
            return fail(process, 65, "could not send Help About");
        }
        const HWND about_dialog = wait_for_window(
            process.hProcess, process.dwProcessId, OPUSW("OpusSdmDialog"),
            nullptr, 5000);
        if (about_dialog == nullptr) {
            DWORD exit_code = 0;
            GetExitCodeProcess(process.hProcess, &exit_code);
            std::cerr << "Help About process exit=0x" << std::hex << exit_code
                      << std::dec << " mainWindow=" << IsWindow(main_window)
                      << " responsive="
                      << window_is_responsive(process.hProcess, main_window)
                      << " stage="
                      << reinterpret_cast<INT_PTR>(GetPropA(
                             main_window, "OpusX64AboutStage"))
                      << '\n';
            log_process_windows(process.dwProcessId);
            return fail(process, 66, "Help About dialog did not appear");
        }
        if (!control_has_class(about_dialog, 1, OPUSW("Button")) ||
            !window_is_responsive(process.hProcess, about_dialog)) {
            return fail(process, 67,
                        "Help About dialog did not finish initializing");
        }
        if (!PostMessageW(about_dialog, kWmCommand, 1, 0) ||
            !wait_for_window_to_close(process.hProcess, process.dwProcessId,
                                      OPUSW("OpusSdmDialog"), 5000) ||
            !window_is_responsive(process.hProcess, main_window)) {
            return fail(process, 68, "Help About dialog did not close cleanly");
        }
        TerminateProcess(process.hProcess, 0);
        WaitForSingleObject(process.hProcess, 2000);
        CloseHandle(process.hThread);
        CloseHandle(process.hProcess);
        return 0;
    }
    if (clipboard_mode) {
        const HWND pane = find_descendant_by_class(main_window, OPUSW("OpusWwd"));
        if (pane == nullptr) {
            return fail(process, 73,
                        "shortcut test could not find the document pane");
        }
        const std::array<std::pair<WORD, LRESULT>, 5> bindings{{
            {'A', kEditSelectAll},
            {'C', kEditCopy},
            {'V', kEditPaste},
            {'X', kEditCut},
            {'Z', kEditUndo},
        }};
        for (const auto& [key, expected_command] : bindings) {
            const LRESULT actual_command = SendMessageW(
                pane, kWmOpusX64QuerySelection, 81, kKcControl | key);
            if (actual_command != expected_command) {
                std::cerr << "Ctrl+" << static_cast<char>(key)
                          << " command=" << actual_command
                          << " expected=" << expected_command << '\n';
                return fail(process, 74,
                            "a Word 95 editing shortcut is not bound");
            }
        }
        if (!execute_control_shortcut(pane, 'A')) {
            return fail(process, 75, "Ctrl+A did not execute Select All");
        }
        Sleep(300);
        const LRESULT selected_first = SendMessageW(
            pane, kWmOpusX64QuerySelection, 0, 0);
        const LRESULT selected_lim = SendMessageW(
            pane, kWmOpusX64QuerySelection, 1, 0);
        if (selected_lim <= selected_first) {
            std::cerr << "Select All range=" << selected_first << ','
                      << selected_lim << '\n';
            return fail(process, 75,
                        "Ctrl+A did not change the document selection");
        }
        TerminateProcess(process.hProcess, 0);
        WaitForSingleObject(process.hProcess, 2000);
        CloseHandle(process.hThread);
        CloseHandle(process.hProcess);
        return 0;
    }
    if (unicode_mode) {
        const HWND pane = find_descendant_by_class(main_window, OPUSW("OpusWwd"));
        DWORD ignored_process_id = 0;
        const DWORD thread_id =
            GetWindowThreadProcessId(main_window, &ignored_process_id);
        if (pane == nullptr ||
            !make_foreground_and_focus(main_window, pane, thread_id)) {
            return fail(process, 87,
                        "Unicode test could not focus the document pane");
        }
        const LRESULT cp_first = SendMessageW(
            pane, kWmOpusX64QuerySelection, 0, 0);
        const std::array<std::uint32_t, 5> scalars{
            0x041f, 0x03a9, 0x0645, 0x3053, 0x1f642};
        for (std::size_t index = 0; index < scalars.size(); ++index) {
            const std::uint32_t scalar = scalars[index];
            bool accepted = false;
            if (index == 0) {
                DWORD_PTR result = 0;
                accepted = SendMessageTimeoutW(
                    pane, WM_UNICHAR, static_cast<WPARAM>(scalar), 1,
                    SMTO_ABORTIFHUNG | SMTO_BLOCK, 3000, &result) != FALSE;
            } else {
                accepted = SendMessageW(
                    pane, kWmOpusX64QuerySelection, 107,
                    static_cast<LPARAM>(scalar)) != FALSE;
            }
            if (!accepted) {
                return fail(process, 88,
                            "Unicode input message was not accepted");
            }
        }
        Sleep(500);
        const LRESULT cp_after = SendMessageW(
            pane, kWmOpusX64QuerySelection, 0, 0);
        bool scalars_preserved = cp_after ==
            cp_first + static_cast<LRESULT>(scalars.size());
        std::array<LRESULT, 5> actual_scalars{};
        for (std::size_t index = 0;
             index < scalars.size(); ++index) {
            actual_scalars[index] = SendMessageW(
                pane, kWmOpusX64QuerySelection, 106,
                cp_first + static_cast<LRESULT>(index));
            scalars_preserved = scalars_preserved &&
                actual_scalars[index] == scalars[index];
        }
        if (!scalars_preserved) {
            std::cerr << "Unicode input range=" << cp_first << "->"
                      << cp_after << " scalars=";
            for (const LRESULT actual : actual_scalars)
                std::cerr << actual << ',';
            std::cerr << '\n';
            return fail(process, 89,
                        "Unicode code points were not preserved in the document");
        }
        TerminateProcess(process.hProcess, 0);
        WaitForSingleObject(process.hProcess, 2000);
        CloseHandle(process.hThread);
        CloseHandle(process.hProcess);
        return 0;
    }
    if (font_typing_mode) {
        const HWND pane = find_descendant_by_class(main_window, OPUSW("OpusWwd"));
        DWORD ignored_process_id = 0;
        const DWORD thread_id =
            GetWindowThreadProcessId(main_window, &ignored_process_id);
        std::vector<HWND> combos;
        collect_descendants_by_class(main_window, OPUSW("ComboBox"), combos);
        HWND font_combo = nullptr;
        HWND size_combo = nullptr;
        LRESULT font_index = CB_ERR;
        LRESULT size_index = CB_ERR;
        for (const HWND combo : combos) {
            if (font_combo == nullptr) {
                font_index = SendMessageW(
                    combo, CB_FINDSTRINGEXACT, static_cast<WPARAM>(-1),
                    reinterpret_cast<LPARAM>(OPUSW("Courier New")));
                if (font_index != CB_ERR) {
                    font_combo = combo;
                }
            }
            if (size_combo == nullptr) {
                size_index = SendMessageW(
                    combo, CB_FINDSTRINGEXACT, static_cast<WPARAM>(-1),
                    reinterpret_cast<LPARAM>(OPUSW("24")));
                if (size_index != CB_ERR) {
                    size_combo = combo;
                }
            }
        }
        if (pane == nullptr || font_combo == nullptr || size_combo == nullptr) {
            return fail(process, 47,
                        "font typing test could not find the ribbon controls");
        }
        const LRESULT second_font_index = SendMessageW(
            font_combo, CB_FINDSTRINGEXACT, static_cast<WPARAM>(-1),
            reinterpret_cast<LPARAM>(OPUSW("Arial")));
        const LRESULT second_size_index = SendMessageW(
            size_combo, CB_FINDSTRINGEXACT, static_cast<WPARAM>(-1),
            reinterpret_cast<LPARAM>(OPUSW("36")));
        const LRESULT large_size_index = SendMessageW(
            size_combo, CB_FINDSTRINGEXACT, static_cast<WPARAM>(-1),
            reinterpret_cast<LPARAM>(OPUSW("72")));
        const LRESULT listed_size_index = SendMessageW(
            size_combo, CB_FINDSTRINGEXACT, static_cast<WPARAM>(-1),
            reinterpret_cast<LPARAM>(OPUSW("72")));
        if (second_font_index == CB_ERR || second_size_index == CB_ERR ||
            large_size_index == CB_ERR || listed_size_index == CB_ERR) {
            return fail(process, 47,
                        "font typing test could not find its second font");
        }

        const LRESULT initial_ftc =
            SendMessageW(pane, kWmOpusX64QuerySelection, 49, 0);
        const LRESULT initial_hps =
            SendMessageW(pane, kWmOpusX64QuerySelection, 50, 0);
        if (SendMessageW(pane, kWmOpusX64QuerySelection, 54, 195) != 195) {
            return fail(process, 47,
                        "the 8-bit master-font index was sign-extended");
        }
        if (SendMessageW(pane, kWmOpusX64QuerySelection, 58, 0) != 4) {
            return fail(process, 47,
                        "the packed font identifier is not 32 bits");
        }
        if (!make_foreground_and_focus(main_window, pane, thread_id) ||
            !choose_combo_item_with_mouse(font_combo, font_index) ||
            !wait_for_focus(process.hProcess, thread_id, pane, 1500)) {
            return fail(process, 49,
                        "font typing test could not mouse-select the font");
        }
        Sleep(300);
        const LRESULT applied_ftc =
            SendMessageW(pane, kWmOpusX64QuerySelection, 49, 0);

        if (!choose_combo_item_with_mouse(size_combo, size_index) ||
            !wait_for_focus(process.hProcess, thread_id, pane, 1500)) {
            return fail(process, 51,
                        "font typing test could not mouse-select point size");
        }
        Sleep(300);
        const LRESULT applied_hps =
            SendMessageW(pane, kWmOpusX64QuerySelection, 50, 0);
        const LRESULT cp_before =
            SendMessageW(pane, kWmOpusX64QuerySelection, 0, 0);
        if (!send_physical_text(OPUSW("fonttest")) ||
            !send_virtual_key(VK_RIGHT)) {
            return fail(process, 52,
                        "font typing test could not type and commit text");
        }
        Sleep(1000);
        const LRESULT inserted_ftc = SendMessageW(
            pane, kWmOpusX64QuerySelection, 51, cp_before);
        const LRESULT inserted_hps = SendMessageW(
            pane, kWmOpusX64QuerySelection, 52, cp_before);
        const LRESULT first_line_height = SendMessageW(
            pane, kWmOpusX64QuerySelection, 33, 0);
        const LRESULT first_formatted_height = SendMessageW(
            pane, kWmOpusX64QuerySelection, 55, cp_before);
        const LRESULT first_formatter_hps = SendMessageW(
            pane, kWmOpusX64QuerySelection, 56, cp_before);
        const LRESULT first_font_height = SendMessageW(
            pane, kWmOpusX64QuerySelection, 57, cp_before);
        std::cerr << "font properties=" << initial_ftc << ',' << initial_hps
                  << " applied=" << applied_ftc << ',' << applied_hps
                  << " inserted=" << inserted_ftc << ',' << inserted_hps
                  << " lineHeight=" << first_line_height << " formatted="
                  << first_formatted_height << " formatter="
                  << first_formatter_hps << ',' << first_font_height << '\n';
        if (!window_is_responsive(process.hProcess, main_window) ||
            applied_ftc < 0 || applied_ftc == initial_ftc ||
            applied_hps != 48 || applied_hps == initial_hps ||
            inserted_ftc != applied_ftc || inserted_hps != applied_hps) {
            return fail(process, 53,
                        "newly typed text did not retain the ribbon font");
        }

        if (!choose_combo_item_with_mouse(font_combo, second_font_index) ||
            !wait_for_focus(process.hProcess, thread_id, pane, 1500)) {
            return fail(process, 55,
                        "font typing test could not mouse-select its second font");
        }
        if (!choose_combo_item_with_mouse(size_combo, second_size_index) ||
            !wait_for_focus(process.hProcess, thread_id, pane, 1500)) {
            return fail(process, 56,
                        "font typing test could not mouse-select its second size");
        }
        Sleep(300);
        const LRESULT second_ftc =
            SendMessageW(pane, kWmOpusX64QuerySelection, 49, 0);
        const LRESULT second_hps =
            SendMessageW(pane, kWmOpusX64QuerySelection, 50, 0);
        const LRESULT second_ibst =
            SendMessageW(pane, kWmOpusX64QuerySelection, 53, 0);
        const LRESULT second_cp =
            SendMessageW(pane, kWmOpusX64QuerySelection, 0, 0);
        if (!send_physical_text(OPUSW(" secondfont")) ||
            !send_virtual_key(VK_RIGHT)) {
            return fail(process, 57,
                        "font typing test could not type its second run");
        }
        Sleep(1000);
        const LRESULT second_inserted_ftc = SendMessageW(
            pane, kWmOpusX64QuerySelection, 51, second_cp);
        const LRESULT second_inserted_hps = SendMessageW(
            pane, kWmOpusX64QuerySelection, 52, second_cp);
        const LRESULT first_inserted_hps_after_second = SendMessageW(
            pane, kWmOpusX64QuerySelection, 52, cp_before);
        const LRESULT second_line_height = SendMessageW(
            pane, kWmOpusX64QuerySelection, 33, 0);
        const LRESULT second_formatted_height = SendMessageW(
            pane, kWmOpusX64QuerySelection, 55, second_cp);
        const LRESULT second_formatter_hps = SendMessageW(
            pane, kWmOpusX64QuerySelection, 56, second_cp);
        const LRESULT second_font_height = SendMessageW(
            pane, kWmOpusX64QuerySelection, 57, second_cp);
        const LRESULT formatted_chp_hps = SendMessageW(
            pane, kWmOpusX64QuerySelection, 59, second_cp);
        const LRESULT formatted_fcid_hps = SendMessageW(
            pane, kWmOpusX64QuerySelection, 60, second_cp);
        const LRESULT formatted_chp_count = SendMessageW(
            pane, kWmOpusX64QuerySelection, 61, second_cp);
        const LRESULT formatted_ich_max = SendMessageW(
            pane, kWmOpusX64QuerySelection, 62, second_cp);
        const LRESULT formatted_cp_first = SendMessageW(
            pane, kWmOpusX64QuerySelection, 13, 0);
        const LRESULT formatted_cp_lim = SendMessageW(
            pane, kWmOpusX64QuerySelection, 14, 0);
        const LRESULT formatted_ich_count = SendMessageW(
            pane, kWmOpusX64QuerySelection, 17, 0);
        std::cerr << " second=" << second_ftc << ',' << second_hps
                  << " masterFont=" << second_ibst
                  << " secondInserted=" << second_inserted_ftc << ','
                  << second_inserted_hps << " firstStill="
                  << first_inserted_hps_after_second << " lineHeight="
                  << second_line_height << " formatted="
                  << second_formatted_height << " formatter="
                  << second_formatter_hps << ',' << second_font_height
                  << " formattedRuns=" << formatted_chp_count << ':'
                  << formatted_chp_hps << ',' << formatted_fcid_hps << '@'
                  << formatted_ich_max << " lineRange="
                  << formatted_cp_first << ',' << formatted_cp_lim << '/'
                  << formatted_ich_count
                  << '\n';
        if (!window_is_responsive(process.hProcess, main_window) ||
            second_ftc < 0 || second_ftc == applied_ftc ||
            second_hps != 72 || second_inserted_ftc != second_ftc ||
            second_inserted_hps != second_hps || first_line_height <= 0 ||
            second_formatted_height <= first_formatted_height ||
            formatted_chp_hps != 72 || formatted_fcid_hps != 72) {
            return fail(process, 58,
                        "the second typed run did not retain font and size");
        }

        const std::size_t mixed_line_pixels =
            count_dark_client_pixels(pane, 0, 300);
        if (!send_physical_text(OPUSW("\r"))) {
            return fail(process, 59,
                        "font typing test could not start its next line");
        }
        Sleep(400);
        const std::size_t after_enter_pixels =
            count_dark_client_pixels(pane, 0, 300);
        const std::size_t after_enter_first_band =
            count_dark_client_pixels(pane, 0, 50);
        const std::size_t after_enter_second_band =
            count_dark_client_pixels(pane, 50, 131);
        InvalidateRect(pane, nullptr, TRUE);
        UpdateWindow(pane);
        Sleep(400);
        const std::size_t after_forced_repaint_pixels =
            count_dark_client_pixels(pane, 0, 300);
        const LRESULT editable_cp_mac = SendMessageW(
            pane, kWmOpusX64QuerySelection, 41, 0);
        bool fetch_bytes_match = true;
        LRESULT fetch_mismatch_cp = -1;
        LRESULT fetch_raw = -1;
        LRESULT fetch_formatted = -1;
        for (LRESULT cp = 0; cp < editable_cp_mac; ++cp) {
            fetch_raw = SendMessageW(
                pane, kWmOpusX64QuerySelection, 69, cp);
            fetch_formatted = SendMessageW(
                pane, kWmOpusX64QuerySelection, 70, cp);
            if (fetch_raw != fetch_formatted) {
                fetch_bytes_match = false;
                fetch_mismatch_cp = cp;
                break;
            }
        }
        const LRESULT cache_pages = SendMessageW(
            pane, kWmOpusX64QuerySelection, 71, 0);
        const bool cache_pages_separate =
            LOWORD(cache_pages) != HIWORD(cache_pages);
        if (!choose_combo_item_with_mouse(size_combo, large_size_index) ||
            !wait_for_focus(process.hProcess, thread_id, pane, 1500)) {
            return fail(process, 59,
                        "font typing test could not start its larger line");
        }
        const std::size_t after_size_pixels =
            count_dark_client_pixels(pane, 0, 300);
        const LRESULT large_cp =
            SendMessageW(pane, kWmOpusX64QuerySelection, 0, 0);
        if (!send_physical_text(OPUSW("largeline")) ||
            !send_virtual_key(VK_RIGHT)) {
            return fail(process, 59,
                        "font typing test could not type its larger line");
        }
        Sleep(1200);
        const std::size_t large_line_pixels =
            count_dark_client_pixels(pane, 0, 450);
        const std::size_t large_line_band_pixels =
            count_dark_client_pixels(pane, 131, 292);
        const LRESULT large_inserted_ftc = SendMessageW(
            pane, kWmOpusX64QuerySelection, 51, large_cp);
        const LRESULT large_inserted_hps = SendMessageW(
            pane, kWmOpusX64QuerySelection, 52, large_cp);
        const LRESULT display_line_count = SendMessageW(
            pane, kWmOpusX64QuerySelection, 30, 0);
        std::cerr << " visualPixels=" << mixed_line_pixels << "->"
                  << after_enter_pixels << "->" << after_size_pixels << "->"
                  << large_line_pixels << " larger=" << large_inserted_ftc
                  << ',' << large_inserted_hps << " displayLines="
                  << display_line_count << " bands="
                  << after_enter_first_band << ','
                  << after_enter_second_band << " repaint="
                  << after_forced_repaint_pixels << " largeBand="
                  << large_line_band_pixels << " fetch="
                  << fetch_bytes_match << '/' << cache_pages_separate << '@'
                  << fetch_mismatch_cp << ':' << fetch_raw << '/'
                  << fetch_formatted
                  << " dr="
                  << SendMessageW(pane, kWmOpusX64QuerySelection, 36, 0)
                  << ','
                  << SendMessageW(pane, kWmOpusX64QuerySelection, 37, 0)
                  << " dirty="
                  << SendMessageW(pane, kWmOpusX64QuerySelection, 39, 0)
                  << ','
                  << SendMessageW(pane, kWmOpusX64QuerySelection, 40, 0);
        for (LRESULT line = 0; line < display_line_count; ++line) {
            std::cerr << " [" << line << " cp="
                      << SendMessageW(pane, kWmOpusX64QuerySelection, 31,
                                      line)
                      << " y="
                      << SendMessageW(pane, kWmOpusX64QuerySelection, 32,
                                      line)
                      << " h="
                      << SendMessageW(pane, kWmOpusX64QuerySelection, 33,
                                      line)
                      << " n="
                      << SendMessageW(pane, kWmOpusX64QuerySelection, 34,
                                      line)
                      << " dirty="
                      << SendMessageW(pane, kWmOpusX64QuerySelection, 35,
                                      line)
                      << ']';
        }
        std::cerr << '\n';
        if (large_inserted_ftc != second_ftc || large_inserted_hps != 144 ||
            display_line_count < 3 || mixed_line_pixels == 0 ||
            after_enter_first_band == 0 || after_enter_second_band == 0 ||
            after_forced_repaint_pixels * 4 < mixed_line_pixels * 3 ||
            large_line_band_pixels == 0 ||
            large_line_pixels <= after_forced_repaint_pixels ||
            !fetch_bytes_match || !cache_pages_separate) {
            return fail(process, 60,
                        "mixed-font lines disappeared after resizing");
        }

        POINT resize_drag_start{
            static_cast<LONG>(SendMessageW(
                pane, kWmOpusX64QuerySelection, 4, 0)),
            static_cast<LONG>(SendMessageW(
                pane, kWmOpusX64QuerySelection, 7, 0)) - 2};
        POINT resize_drag_end{5, 5};
        if (!make_foreground_and_focus(main_window, pane, thread_id) ||
            !ClientToScreen(pane, &resize_drag_start) ||
            !ClientToScreen(pane, &resize_drag_end) ||
            !SetCursorPos(resize_drag_start.x, resize_drag_start.y) ||
            !send_mouse_button(MOUSEEVENTF_LEFTDOWN) ||
            !wait_for_capture(process.hProcess, thread_id, pane, 1500) ||
            !SetCursorPos(resize_drag_end.x, resize_drag_end.y)) {
            send_mouse_button(MOUSEEVENTF_LEFTUP);
            return fail(process, 61,
                        "font typing test could not select the document");
        }
        Sleep(200);
        send_mouse_button(MOUSEEVENTF_LEFTUP);
        Sleep(300);
        const LRESULT selected_first = SendMessageW(
            pane, kWmOpusX64QuerySelection, 0, 0);
        const LRESULT selected_lim = SendMessageW(
            pane, kWmOpusX64QuerySelection, 1, 0);
        std::cerr << " selectionBeforeResize=" << selected_first << ','
                  << selected_lim << '\n';
        if (selected_lim <= selected_first ||
            !make_foreground_and_focus(main_window, size_combo, thread_id) ||
            !send_control_key('A') || !send_physical_text(OPUSW("48")) ||
            !make_foreground_and_focus(main_window, pane, thread_id)) {
            return fail(process, 61,
                        "font typing test could not resize selected text");
        }
        Sleep(1200);
        const LRESULT selected_first_hps = SendMessageW(
            pane, kWmOpusX64QuerySelection, 52, cp_before);
        const LRESULT selected_second_hps = SendMessageW(
            pane, kWmOpusX64QuerySelection, 52, second_cp);
        const LRESULT selected_formatted_height = SendMessageW(
            pane, kWmOpusX64QuerySelection, 55, cp_before);
        std::cerr << " selected=" << selected_first << ',' << selected_lim
                  << " resized=" << selected_first_hps << ','
                  << selected_second_hps << " formatted="
                  << selected_formatted_height << '\n';
        if (selected_first_hps != 96 || selected_second_hps != 96 ||
            selected_formatted_height <= second_formatted_height) {
            return fail(process, 62,
                        "selected text did not retain its point size");
        }

        if (!make_foreground_and_focus(main_window, size_combo, thread_id) ||
            SendMessageW(size_combo, CB_SHOWDROPDOWN, TRUE, 0) == 0) {
            return fail(process, 63,
                        "font typing test could not open the Points list");
        }
        Sleep(200);
        COMBOBOXINFO size_info{};
        size_info.cbSize = sizeof(size_info);
        RECT size_list_rectangle{};
        const LRESULT size_item_height = SendMessageW(
            size_combo, CB_GETITEMHEIGHT, 0, 0);
        if (!GetComboBoxInfo(size_combo, &size_info) ||
            size_info.hwndList == nullptr || size_item_height <= 0 ||
            !GetWindowRect(size_info.hwndList, &size_list_rectangle)) {
            return fail(process, 63,
                        "font typing test could not locate the Points list");
        }
        const LRESULT requested_top =
            (std::max)(0LL, static_cast<long long>(listed_size_index) - 2);
        SendMessageW(size_info.hwndList, LB_SETTOPINDEX, requested_top, 0);
        const LRESULT actual_top = SendMessageW(
            size_info.hwndList, LB_GETTOPINDEX, 0, 0);
        POINT size_list_choice{
            size_list_rectangle.left + 12,
            size_list_rectangle.top + 2 +
                static_cast<LONG>((listed_size_index - actual_top) *
                                  size_item_height + size_item_height / 2)};
        std::cerr << " pointsList=" << size_list_rectangle.left << ','
                  << size_list_rectangle.top << ','
                  << size_list_rectangle.right << ','
                  << size_list_rectangle.bottom << " item="
                  << size_item_height << " index=" << listed_size_index
                  << " top=" << actual_top << " choice="
                  << size_list_choice.x << ',' << size_list_choice.y << '\n';
        if (size_list_choice.y >= size_list_rectangle.bottom ||
            !SetCursorPos(size_list_choice.x, size_list_choice.y) ||
            !send_mouse_button(MOUSEEVENTF_LEFTDOWN) ||
            !send_mouse_button(MOUSEEVENTF_LEFTUP)) {
            return fail(process, 63,
                        "font typing test could not commit Points list");
        }
        Sleep(600);
        DWORD list_exit_code = STILL_ACTIVE;
        GetExitCodeProcess(process.hProcess, &list_exit_code);
        const bool process_after_list =
            WaitForSingleObject(process.hProcess, 0) != WAIT_OBJECT_0;
        const bool returned_to_document = process_after_list &&
            (make_foreground_and_focus(main_window, pane, thread_id) ||
             make_foreground_and_focus(main_window, pane, thread_id));
        Sleep(1200);
        const LRESULT listed_first_hps = SendMessageW(
            pane, kWmOpusX64QuerySelection, 52, cp_before);
        const LRESULT listed_second_hps = SendMessageW(
            pane, kWmOpusX64QuerySelection, 52, second_cp);
        const LRESULT listed_formatted_height = SendMessageW(
            pane, kWmOpusX64QuerySelection, 55, cp_before);
        std::cerr << " listedResize=" << listed_first_hps << ','
                  << listed_second_hps << " formatted="
                  << listed_formatted_height << " documentFocus="
                  << returned_to_document << " process="
                  << process_after_list << "/0x" << std::hex
                  << list_exit_code << std::dec << " windows="
                  << IsWindow(main_window) << ',' << IsWindow(pane) << '\n';
        if (listed_first_hps != 144 || listed_second_hps != 144 ||
            listed_formatted_height <= selected_formatted_height) {
            return fail(process, 64,
                        "Points list did not resize selected text");
        }
        TerminateProcess(process.hProcess, 0);
        WaitForSingleObject(process.hProcess, 2000);
        CloseHandle(process.hThread);
        CloseHandle(process.hProcess);
        return 0;
    }
    if (formatting_mode) {
        const HWND pane = find_descendant_by_class(main_window, OPUSW("OpusWwd"));
        DWORD ignored_process_id = 0;
        const DWORD thread_id =
            GetWindowThreadProcessId(main_window, &ignored_process_id);
        if (pane == nullptr ||
            !make_foreground_and_focus(main_window, pane, thread_id) ||
            !send_physical_text(OPUSW("formatting selection"))) {
            return fail(process, 43,
                        "formatting test could not type its selection");
        }
        Sleep(500);
        POINT drag_start{155, 10};
        POINT drag_end{20, 10};
        if (!ClientToScreen(pane, &drag_start) ||
            !ClientToScreen(pane, &drag_end) ||
            !SetCursorPos(drag_start.x, drag_start.y) ||
            !send_mouse_button(MOUSEEVENTF_LEFTDOWN) ||
            !wait_for_capture(process.hProcess, thread_id, pane, 1500) ||
            !SetCursorPos(drag_end.x, drag_end.y)) {
            send_mouse_button(MOUSEEVENTF_LEFTUP);
            return fail(process, 44,
                        "formatting test could not drag a selection");
        }
        Sleep(150);
        send_mouse_button(MOUSEEVENTF_LEFTUP);
        Sleep(500);
        const LRESULT selected_first =
            SendMessageW(pane, kWmOpusX64QuerySelection, 0, 0);
        const LRESULT selected_lim =
            SendMessageW(pane, kWmOpusX64QuerySelection, 1, 0);
        std::cerr << "format selection=" << selected_first << ','
                  << selected_lim << '\n';
        if (selected_lim <= selected_first ||
            !PostMessageW(main_window, kWmCommand, kParaCenter, 0)) {
            return fail(process, 45,
                        "formatting test could not apply centered alignment");
        }
        Sleep(1500);
        if (!window_is_responsive(process.hProcess, main_window)) {
            return fail(process, 46,
                        "centered alignment crashed or hung WORD1");
        }
        TerminateProcess(process.hProcess, 0);
        WaitForSingleObject(process.hProcess, 2000);
        CloseHandle(process.hThread);
        CloseHandle(process.hProcess);
        return 0;
    }
    if (color_mode) {
        const HWND pane = find_descendant_by_class(main_window, OPUSW("OpusWwd"));
        DWORD ignored_process_id = 0;
        const DWORD thread_id =
            GetWindowThreadProcessId(main_window, &ignored_process_id);
        if (pane == nullptr ||
            !make_foreground_and_focus(main_window, pane, thread_id) ||
            !send_physical_text(OPUSW("testtest")) ||
            !execute_control_shortcut(pane, 'A')) {
            return fail(process, 76,
                        "color test could not create its selection");
        }
        Sleep(500);
        const LRESULT selected_first = SendMessageW(
            pane, kWmOpusX64QuerySelection, 0, 0);
        const LRESULT selected_lim = SendMessageW(
            pane, kWmOpusX64QuerySelection, 1, 0);
        const LRESULT before = SendMessageW(
            pane, kWmOpusX64QuerySelection, 83, 0);
        const LRESULT prm_before = SendMessageW(
            pane, kWmOpusX64QuerySelection, 85, 0);
        const LRESULT applied = SendMessageW(
            pane, kWmOpusX64QuerySelection, 82, 6);
        Sleep(500);
        const LRESULT after = SendMessageW(
            pane, kWmOpusX64QuerySelection, 83, 0);
        const LRESULT selection_color = SendMessageW(
            pane, kWmOpusX64QuerySelection, 79, 0);
        const LRESULT prm_after = SendMessageW(
            pane, kWmOpusX64QuerySelection, 85, 0);
        const LRESULT selection_style = SendMessageW(
            pane, kWmOpusX64QuerySelection, 86, 0);
        std::cerr << "color selection=" << selected_first << ','
                  << selected_lim << " property=" << before << "->" << after
                  << " selectionColor=" << selection_color
                  << " applied=" << applied << " prm=0x" << std::hex
                  << prm_before << "->0x" << prm_after << std::dec
                  << " style=" << selection_style << '\n';
        if (selected_lim <= selected_first || applied == 0 || after != 6) {
            return fail(process, 77,
                        "text color was not stored in the selected characters");
        }
        TerminateProcess(process.hProcess, 0);
        WaitForSingleObject(process.hProcess, 2000);
        CloseHandle(process.hThread);
        CloseHandle(process.hProcess);
        return 0;
    }
    if (caret_mode) {
        const HWND pane = find_descendant_by_class(main_window, OPUSW("OpusWwd"));
        if (pane == nullptr) {
            return fail(process, 40, "caret test could not find OpusWwd");
        }
        DWORD ignored_process_id = 0;
        const DWORD thread_id =
            GetWindowThreadProcessId(main_window, &ignored_process_id);
        if (!make_foreground_and_focus(main_window, pane, thread_id)) {
            return fail(process, 41,
                        "caret test could not focus the document pane");
        }
        const auto report = [pane](const char* stage) {
            std::cerr << stage
                      << " cp=" << SendMessageW(
                             pane, kWmOpusX64QuerySelection, 0, 0)
                      << " xw=" << SendMessageW(
                             pane, kWmOpusX64QuerySelection, 4, 0)
                      << " yw=" << SendMessageW(
                             pane, kWmOpusX64QuerySelection, 7, 0)
                      << " dyp=" << SendMessageW(
                             pane, kWmOpusX64QuerySelection, 8, 0)
                      << " on=" << SendMessageW(
                             pane, kWmOpusX64QuerySelection, 9, 0)
                      << " hidden=" << SendMessageW(
                             pane, kWmOpusX64QuerySelection, 10, 0)
                      << " xpFirst=" << SendMessageW(
                             pane, kWmOpusX64QuerySelection, 11, 0)
                      << " xpLim=" << SendMessageW(
                             pane, kWmOpusX64QuerySelection, 12, 0)
                      << " fliCp=" << SendMessageW(
                             pane, kWmOpusX64QuerySelection, 13, 0)
                      << ',' << SendMessageW(
                             pane, kWmOpusX64QuerySelection, 14, 0)
                      << " fliXp=" << SendMessageW(
                             pane, kWmOpusX64QuerySelection, 15, 0)
                      << ',' << SendMessageW(
                             pane, kWmOpusX64QuerySelection, 16, 0)
                      << " ich=" << SendMessageW(
                             pane, kWmOpusX64QuerySelection, 17, 0)
                      << " dxp=" << SendMessageW(
                             pane, kWmOpusX64QuerySelection, 18, 0)
                      << ',' << SendMessageW(
                             pane, kWmOpusX64QuerySelection, 19, 0)
                      << " chrSizes=" << SendMessageW(
                             pane, kWmOpusX64QuerySelection, 24, 0)
                      << ',' << SendMessageW(
                             pane, kWmOpusX64QuerySelection, 25, 0)
                      << ',' << SendMessageW(
                             pane, kWmOpusX64QuerySelection, 26, 0)
                      << ',' << SendMessageW(
                             pane, kWmOpusX64QuerySelection, 27, 0)
                      << ',' << SendMessageW(
                             pane, kWmOpusX64QuerySelection, 28, 0)
                      << ',' << SendMessageW(
                             pane, kWmOpusX64QuerySelection, 29, 0)
                      << '\n';
        };
        const LRESULT initial_x =
            SendMessageW(pane, kWmOpusX64QuerySelection, 4, 0);
        const LRESULT initial_y =
            SendMessageW(pane, kWmOpusX64QuerySelection, 7, 0);
        report("initial");
        if (!send_physical_text(OPUSW("ab\rc"))) {
            return fail(process, 41, "caret test could not send its keys");
        }

        // A cursor key leaves the original quick-insert loop and commits its
        // reserved 32-byte piece before the diagnostic query reads selCur. At
        // the document end VK_RIGHT does not change the insertion CP.
        if (!send_virtual_key(VK_RIGHT)) {
            return fail(process, 41,
                        "caret test could not commit the insert loop");
        }
        Sleep(1000);
        const LRESULT final_cp =
            SendMessageW(pane, kWmOpusX64QuerySelection, 0, 0);
        const LRESULT final_x =
            SendMessageW(pane, kWmOpusX64QuerySelection, 4, 0);
        const LRESULT final_y =
            SendMessageW(pane, kWmOpusX64QuerySelection, 7, 0);
        const LRESULT final_ins =
            SendMessageW(pane, kWmOpusX64QuerySelection, 2, 0);
        report("final");
        if (final_cp != 5 || final_ins != 1 || final_x <= initial_x ||
            final_y <= initial_y) {
            return fail(process, 42,
                        "typing did not advance the caret across a paragraph");
        }
        TerminateProcess(process.hProcess, 0);
        WaitForSingleObject(process.hProcess, 2000);
        CloseHandle(process.hThread);
        CloseHandle(process.hProcess);
        return 0;
    }
    if (selection_mode) {
        const HWND pane = find_descendant_by_class(main_window, OPUSW("OpusWwd"));
        if (pane == nullptr) {
            return fail(process, 36, "selection test could not find OpusWwd");
        }
        const std::wstring sentence = L"physical keyboard input line one";
        for (const wchar_t character : sentence) {
            if (!post_keyboard_character(
                    pane, static_cast<WCHAR>(character))) {
                return fail(process, 37,
                            "selection test could not post its sentence");
            }
            Sleep(12);
        }
        Sleep(750);
        const LRESULT typed_first =
            SendMessageW(pane, kWmOpusX64QuerySelection, 0, 0);
        const LRESULT typed_lim =
            SendMessageW(pane, kWmOpusX64QuerySelection, 1, 0);
        const LRESULT typed_ins =
            SendMessageW(pane, kWmOpusX64QuerySelection, 2, 0);
        SendMessageW(pane, kWmOpusX64QuerySelection, 55, 0);
        std::cerr << "widths:";
        for (LPARAM ich = 0; ich != 16; ++ich) {
            std::cerr << ' ' << SendMessageW(
                pane, kWmOpusX64QuerySelection, 63, ich);
        }
        std::cerr << " drWidths:";
        for (LPARAM ich = 0; ich != 16; ++ich) {
            std::cerr << ' ' << SendMessageW(
                pane, kWmOpusX64QuerySelection, 64, ich);
        }
        std::cerr << " dpi="
                  << SendMessageW(pane, kWmOpusX64QuerySelection, 65, 0)
                  << ','
                  << SendMessageW(pane, kWmOpusX64QuerySelection, 66, 0)
                  << ','
                  << SendMessageW(pane, kWmOpusX64QuerySelection, 67, 0)
                  << " mappings:";
        for (int x = 10; x <= 250; x += 10) {
            SendMessageW(pane, WM_LBUTTONDOWN, MK_LBUTTON,
                         MAKELPARAM(x, 10));
            SendMessageW(pane, WM_LBUTTONUP, 0, MAKELPARAM(x, 10));
            std::cerr << ' ' << x << '=' << SendMessageW(
                pane, kWmOpusX64QuerySelection, 0, 0);
        }
        std::cerr << '\n';
        PostMessageW(pane, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(250, 10));
        PostMessageW(pane, WM_LBUTTONUP, 0, MAKELPARAM(250, 10));
        Sleep(500);
        const LRESULT clicked_first =
            SendMessageW(pane, kWmOpusX64QuerySelection, 0, 0);
        const LRESULT clicked_lim =
            SendMessageW(pane, kWmOpusX64QuerySelection, 1, 0);
        const LRESULT clicked_ins =
            SendMessageW(pane, kWmOpusX64QuerySelection, 2, 0);
        const LRESULT clicked_style =
            SendMessageW(pane, kWmOpusX64QuerySelection, 3, 0);
        const LRESULT clicked_xw =
            SendMessageW(pane, kWmOpusX64QuerySelection, 4, 0);
        const LRESULT clicked_double =
            SendMessageW(pane, kWmOpusX64QuerySelection, 5, 0);
        const LRESULT clicked_sk =
            SendMessageW(pane, kWmOpusX64QuerySelection, 6, 0);
        std::cerr << "typed=" << typed_first << ',' << typed_lim << ','
                  << typed_ins << " clicked=" << clicked_first << ','
                  << clicked_lim << ',' << clicked_ins
                  << " style=" << clicked_style << " xw=" << clicked_xw
                  << " double=" << clicked_double << " sk=" << clicked_sk
                  << '\n';
        if (typed_first != static_cast<LRESULT>(sentence.size()) ||
            typed_lim != typed_first || typed_ins != 1) {
            return fail(process, 38,
                        "typing did not leave a canonical insertion selection");
        }
        if (clicked_first < 15 || clicked_first > typed_first ||
            clicked_lim != clicked_first || clicked_ins != 1 ||
            clicked_double != 0 || clicked_sk != 32) {
            return fail(process, 39,
                        "sentence-end click produced an invalid selection");
        }
        TerminateProcess(process.hProcess, 0);
        WaitForSingleObject(process.hProcess, 2000);
        CloseHandle(process.hThread);
        CloseHandle(process.hProcess);
        return 0;
    }
    if (interaction_mode) {
        DWORD ignored_process_id = 0;
        const DWORD thread_id =
            GetWindowThreadProcessId(main_window, &ignored_process_id);

        if (IsZoomed(main_window)) {
            PostMessageW(main_window, WM_SYSCOMMAND, SC_RESTORE, 0);
            if (!wait_for_zoom_state(process.hProcess, main_window, false,
                                     3000)) {
                return fail(process, 17,
                            "SC_RESTORE was swallowed by the Opus handler");
            }
        }
        if (!PostMessageW(main_window, WM_SYSCOMMAND, SC_MAXIMIZE, 0) ||
            !wait_for_zoom_state(process.hProcess, main_window, true, 3000)) {
            return fail(process, 18,
                        "SC_MAXIMIZE was swallowed by the Opus handler");
        }
        PostMessageW(main_window, WM_SYSCOMMAND, SC_RESTORE, 0);
        if (!wait_for_zoom_state(process.hProcess, main_window, false, 3000)) {
            return fail(process, 19, "WORD1 did not restore from maximize");
        }

        const HWND pane = find_descendant_by_class(main_window, OPUSW("OpusWwd"));
        RECT window_before_move{};
        if (pane == nullptr ||
            !make_foreground_and_focus(main_window, pane, thread_id) ||
            !GetWindowRect(main_window, &window_before_move)) {
            return fail(process, 30,
                        "could not prepare the native window move test");
        }
        POINT client_origin{};
        if (!ClientToScreen(main_window, &client_origin)) {
            return fail(process, 30,
                        "could not locate the WORD1 nonclient area");
        }
        POINT caption_point{(window_before_move.left + window_before_move.right) /
                                2,
                            -1};
        for (int y = window_before_move.top; y < client_origin.y; ++y) {
            if (SendMessageW(main_window, WM_NCHITTEST, 0,
                             MAKELPARAM(caption_point.x, y)) == HTCAPTION) {
                caption_point.y = y;
                break;
            }
        }
        if (caption_point.y < 0 ||
            !SetCursorPos(caption_point.x, caption_point.y) ||
            !send_mouse_button(MOUSEEVENTF_LEFTDOWN)) {
            return fail(process, 30,
                        "WORD1 did not expose a draggable caption");
        }
        Sleep(100);
        SetCursorPos(caption_point.x + 40, caption_point.y + 24);
        Sleep(100);
        send_mouse_button(MOUSEEVENTF_LEFTUP);
        Sleep(250);
        RECT window_after_move{};
        if (!GetWindowRect(main_window, &window_after_move) ||
            (window_before_move.left == window_after_move.left &&
             window_before_move.top == window_after_move.top)) {
            return fail(process, 32,
                        "dragging the caption did not move the WORD1 window");
        }

        if (!PostMessageW(main_window, WM_SYSCOMMAND, SC_KEYMENU,
                          OPUSW("f")[0]) ||
            !wait_for_gui_flag(process.hProcess, thread_id, GUI_INMENUMODE,
                               true, 3000)) {
            return fail(process, 20,
                        "the File menu did not enter native menu mode");
        }
        if (!post_keyboard_character(main_window, OPUSW("n")[0])) {
            return fail(process, 21, "could not choose File New by mnemonic");
        }
        const HWND new_dialog = wait_for_window(
            process.hProcess, process.dwProcessId, OPUSW("OpusSdmDialog"),
            OPUSW("New"), 5000);
        if (new_dialog == nullptr) {
            return fail(process, 22,
                        "File New did not execute through the real menu loop");
        }
        if (!PostMessageW(new_dialog, kWmCommand, 2, 0) ||
            !wait_for_window_to_close(process.hProcess, process.dwProcessId,
                                      OPUSW("OpusSdmDialog"), 5000)) {
            return fail(process, 23, "File New dialog did not cancel");
        }

        if (!make_foreground_and_focus(main_window, pane, thread_id)) {
            return fail(process, 24,
                        "could not give real keyboard focus to the document");
        }
        const std::basic_string<WCHAR> physical_text =
            OPUSW("physical keyboard input line one\rphysical keyboard input ")
            OPUSW("line two\rphysical keyboard input line three");
        const LRESULT cp_before_typing =
            SendMessageW(pane, kWmOpusX64QuerySelection, 0, 0);
        const LRESULT caret_x_before_typing =
            SendMessageW(pane, kWmOpusX64QuerySelection, 4, 0);
        const LRESULT caret_y_before_typing =
            SendMessageW(pane, kWmOpusX64QuerySelection, 7, 0);
        const std::size_t dark_pixels_before = count_dark_client_pixels(pane);
        if (!send_physical_text(physical_text)) {
            return fail(process, 25, "SendInput could not type into WORD1");
        }

        // SendInput returns after placing events in the GUI input queue. Give
        // the original insert loop time to drain the complete long sequence
        // before a synchronous diagnostic message can overtake its tail.
        Sleep(2500);
        const LRESULT typed_cp_first =
            SendMessageW(pane, kWmOpusX64QuerySelection, 0, 0);

        // The first non-key message commits Word's quick-insert block. Let its
        // normal idle pass finish rebuilding all display lines before issuing
        // more cross-thread diagnostic messages.
        Sleep(750);
        const LRESULT typed_cp_lim =
            SendMessageW(pane, kWmOpusX64QuerySelection, 1, 0);
        const LRESULT typed_is_insertion =
            SendMessageW(pane, kWmOpusX64QuerySelection, 2, 0);
        const LRESULT caret_x_after_typing =
            SendMessageW(pane, kWmOpusX64QuerySelection, 4, 0);
        const LRESULT caret_y_after_typing =
            SendMessageW(pane, kWmOpusX64QuerySelection, 7, 0);
        const LRESULT displayed_line_count =
            SendMessageW(pane, kWmOpusX64QuerySelection, 30, 0);
        std::cerr << "display lines=" << displayed_line_count
				  << " terminalCp="
				  << SendMessageW(pane, kWmOpusX64QuerySelection, 31,
				                  displayed_line_count)
                  << " dr="
                  << SendMessageW(pane, kWmOpusX64QuerySelection, 36, 0)
                  << ','
                  << SendMessageW(pane, kWmOpusX64QuerySelection, 37, 0);
        std::cerr << " dirty="
                  << SendMessageW(pane, kWmOpusX64QuerySelection, 39, 0)
                  << ','
                  << SendMessageW(pane, kWmOpusX64QuerySelection, 40, 0)
                  << " cpMac="
                  << SendMessageW(pane, kWmOpusX64QuerySelection, 41, 0)
				  << '/'
				  << SendMessageW(pane, kWmOpusX64QuerySelection, 46, 0)
                  << " drSize="
                  << SendMessageW(pane, kWmOpusX64QuerySelection, 44, 0)
                  << 'x'
                  << SendMessageW(pane, kWmOpusX64QuerySelection, 42, 0)
                  << " wwSize="
                  << SendMessageW(pane, kWmOpusX64QuerySelection, 45, 0)
                  << 'x'
                  << SendMessageW(pane, kWmOpusX64QuerySelection, 43, 0);
        for (LPARAM line = 0; line < displayed_line_count && line < 8;
             ++line) {
            std::cerr << " [" << line << " cp="
                      << SendMessageW(pane, kWmOpusX64QuerySelection, 31,
                                      line)
                      << " yp="
                      << SendMessageW(pane, kWmOpusX64QuerySelection, 32,
                                      line)
                      << " dyp="
                      << SendMessageW(pane, kWmOpusX64QuerySelection, 33,
                                      line)
                      << " dcp="
                      << SendMessageW(pane, kWmOpusX64QuerySelection, 34,
                                      line)
					  << " dlk="
					  << SendMessageW(pane, kWmOpusX64QuerySelection, 47,
					                  line)
					  << " end="
					  << SendMessageW(pane, kWmOpusX64QuerySelection, 48,
					                  line)
                      << "]";
        }
        std::cerr << '\n';
        const std::size_t dark_pixels_after = count_dark_client_pixels(pane);
        LRESULT expected_cp_after_typing =
            cp_before_typing + static_cast<LRESULT>(physical_text.size());
        for (const WCHAR character : physical_text) {
            if (character == OPUSW("\r")[0]) {
                ++expected_cp_after_typing;
            }
        }
        if (!window_is_responsive(process.hProcess, main_window) ||
            dark_pixels_after < dark_pixels_before + 8) {
            std::cerr << "typing paint state: before=" << dark_pixels_before
                      << " after=" << dark_pixels_after
                      << " cpBefore=" << cp_before_typing
                      << " cpFirst=" << typed_cp_first
                      << " cpLim=" << typed_cp_lim
                      << " fIns=" << typed_is_insertion << '\n';
            return fail(process, 26,
                        "real keyboard input was not painted in the document");
        }

        if (typed_cp_first != expected_cp_after_typing ||
            typed_cp_first != typed_cp_lim || typed_is_insertion != 1 ||
            caret_x_after_typing <= caret_x_before_typing ||
            caret_y_after_typing <= caret_y_before_typing) {
            std::cerr << "selection after typing: cpBefore=" << cp_before_typing
                      << " cpFirst=" << typed_cp_first
                      << " cpLim=" << typed_cp_lim
                      << " fIns=" << typed_is_insertion
                      << " caretBefore=" << caret_x_before_typing << ','
                      << caret_y_before_typing
                      << " caretAfter=" << caret_x_after_typing << ','
                      << caret_y_after_typing << '\n';
            return fail(process, 33,
                        "typing did not leave a valid insertion selection");
        }

        RECT window_rectangle{};
        const LRESULT lines_before_move = SendMessageW(
            pane, kWmOpusX64QuerySelection, 30, 0);
        POINT populated_client_origin{};
        if (!GetWindowRect(main_window, &window_rectangle) ||
            !ClientToScreen(main_window, &populated_client_origin)) {
            return fail(process, 59, "could not move the populated window");
        }
        POINT populated_caption{
            (window_rectangle.left + window_rectangle.right) / 2, -1};
        for (int y = window_rectangle.top; y < populated_client_origin.y;
             ++y) {
            if (SendMessageW(main_window, WM_NCHITTEST, 0,
                             MAKELPARAM(populated_caption.x, y)) ==
                HTCAPTION) {
                populated_caption.y = y;
                break;
            }
        }
        if (populated_caption.y < 0 ||
            !SetCursorPos(populated_caption.x, populated_caption.y) ||
            !send_mouse_button(MOUSEEVENTF_LEFTDOWN)) {
            return fail(process, 59, "could not drag the populated window");
        }
        Sleep(150);
        SetCursorPos(populated_caption.x + 32, populated_caption.y + 24);
        Sleep(150);
        send_mouse_button(MOUSEEVENTF_LEFTUP);
        Sleep(1000);
        const std::size_t dark_pixels_after_move =
            count_dark_client_pixels(pane);
        const LRESULT lines_after_move = SendMessageW(
            pane, kWmOpusX64QuerySelection, 30, 0);
        std::cerr << "move paint: pixels=" << dark_pixels_after << "->"
                  << dark_pixels_after_move << " lines=" << lines_before_move
                  << "->" << lines_after_move << '\n';
        if (!window_is_responsive(process.hProcess, main_window) ||
            lines_after_move < lines_before_move ||
            dark_pixels_after_move * 4 < dark_pixels_after * 3) {
            return fail(process, 60,
                        "moving the window lost populated display lines");
        }

        POINT sentence_end{250, 10};
        if (!ClientToScreen(pane, &sentence_end) ||
            !SetCursorPos(sentence_end.x, sentence_end.y) ||
            !send_mouse_button(MOUSEEVENTF_LEFTDOWN)) {
            return fail(process, 34,
                        "could not click near the first sentence end");
        }
        Sleep(75);
        send_mouse_button(MOUSEEVENTF_LEFTUP);
        Sleep(250);
        const LRESULT clicked_cp_first =
            SendMessageW(pane, kWmOpusX64QuerySelection, 0, 0);
        const LRESULT clicked_cp_lim =
            SendMessageW(pane, kWmOpusX64QuerySelection, 1, 0);
        const LRESULT clicked_is_insertion =
            SendMessageW(pane, kWmOpusX64QuerySelection, 2, 0);
        const LRESULT clicked_style =
            SendMessageW(pane, kWmOpusX64QuerySelection, 3, 0);
        const LRESULT clicked_double =
            SendMessageW(pane, kWmOpusX64QuerySelection, 5, 0);
        if (clicked_cp_first < 0 || clicked_cp_first > typed_cp_first ||
            clicked_cp_first != clicked_cp_lim ||
            clicked_is_insertion != 1 || clicked_double != 0) {
            std::cerr << "selection after sentence-end click: cpFirst="
                      << clicked_cp_first << " cpLim=" << clicked_cp_lim
                      << " fIns=" << clicked_is_insertion
                      << " sty=" << clicked_style
                      << " double=" << clicked_double << '\n';
            return fail(process, 35,
                        "sentence-end click mapped to the wrong selection");
        }

        RECT pane_client{};
        POINT drag_start{20, 10};
        POINT drag_end{180, 10};
        if (!GetClientRect(pane, &pane_client) || pane_client.right < 200 ||
            pane_client.bottom < 30 || !ClientToScreen(pane, &drag_start) ||
            !ClientToScreen(pane, &drag_end) ||
            !SetCursorPos(drag_start.x, drag_start.y) ||
            !send_mouse_button(MOUSEEVENTF_LEFTDOWN)) {
            return fail(process, 27, "could not begin a real mouse selection");
        }
        if (!wait_for_capture(process.hProcess, thread_id, pane, 1500)) {
            send_mouse_button(MOUSEEVENTF_LEFTUP);
            return fail(process, 28,
                        "the original selection engine did not capture mouse");
        }
        SetCursorPos(drag_end.x, drag_end.y);
        Sleep(100);
        if (!send_mouse_button(MOUSEEVENTF_LEFTUP) ||
            !wait_for_capture(process.hProcess, thread_id, nullptr, 1500) ||
            !window_is_responsive(process.hProcess, main_window)) {
            return fail(process, 29,
                        "mouse selection did not release capture cleanly");
        }
        Sleep(250);
        const LRESULT dragged_first = SendMessageW(
            pane, kWmOpusX64QuerySelection, 0, 0);
        const LRESULT dragged_lim = SendMessageW(
            pane, kWmOpusX64QuerySelection, 1, 0);
        const int selection_light_gap = longest_light_gap(pane, 20, 180, 10);
        std::cerr << "selection paint=" << dragged_first << ','
                  << dragged_lim << " longestLightGap="
                  << selection_light_gap << '\n';
        if (dragged_lim <= dragged_first || selection_light_gap > 16) {
            return fail(process, 65,
                        "mouse selection paint contains character gaps");
        }

        TerminateProcess(process.hProcess, 0);
        WaitForSingleObject(process.hProcess, 2000);
        CloseHandle(process.hThread);
        CloseHandle(process.hProcess);
        return 0;
    }
    if (typing_mode) {
        DWORD ignored_process_id = 0;
        const DWORD thread_id =
            GetWindowThreadProcessId(main_window, &ignored_process_id);
        GUITHREADINFO gui{};
        gui.cbSize = sizeof(gui);
        if (!GetGUIThreadInfo(thread_id, &gui) || gui.hwndFocus == nullptr) {
            return fail(process, 13, "active document pane has no focus");
        }
        const std::size_t dark_pixels_before =
            count_dark_client_pixels(gui.hwndFocus);

        // Cross the original 32-byte quick-insert boundary and fill enough
        // display lines to exercise idle normalization and the SCC-above PLC.
        std::wstring text;
        for (int line = 0; line != 40; ++line) {
            text += L"original line ";
            if (line < 10) {
                text += L'0';
            }
            text += std::to_wstring(line);
            text += L'\r';
        }
        for (const wchar_t character : text) {
            if (character != L'\0') {
                bool posted = false;
                for (int attempt = 0; attempt != 20 && !posted; ++attempt) {
                    GUITHREADINFO current_gui{};
                    current_gui.cbSize = sizeof(current_gui);
                    posted = GetGUIThreadInfo(thread_id, &current_gui) &&
                             current_gui.hwndFocus != nullptr &&
                             post_keyboard_character(current_gui.hwndFocus,
                                                     static_cast<WCHAR>(
                                                         character));
                    if (!posted) {
                        Sleep(50);
                    }
                }
                if (!posted) {
                    return fail(process, 15,
                                "could not post a character to the document");
                }
                Sleep(10);
            }
        }
        Sleep(4000);
        if (!window_is_responsive(process.hProcess, main_window)) {
            return fail(process, 14,
                        "original insert loop crashed or stopped responding");
        }
        GUITHREADINFO final_gui{};
        final_gui.cbSize = sizeof(final_gui);
        const std::size_t dark_pixels_after =
            GetGUIThreadInfo(thread_id, &final_gui)
                ? count_dark_client_pixels(final_gui.hwndFocus)
                : 0;
        if (dark_pixels_after < dark_pixels_before + 100) {
            return fail(process, 16,
                        "typed text was not painted in the document pane");
        }
        TerminateProcess(process.hProcess, 0);
        WaitForSingleObject(process.hProcess, 2000);
        CloseHandle(process.hThread);
        CloseHandle(process.hProcess);
        return 0;
    }
    if (!PostMessageW(main_window, kWmCommand, kFileNew, 0)) {
        return fail(process, 4, "could not send File New");
    }

    const HWND new_dialog = wait_for_window(
        process.hProcess, process.dwProcessId, OPUSW("OpusSdmDialog"),
        OPUSW("New"), 5000);
    if (new_dialog == nullptr) {
        return fail(process, 5, "File New dialog did not appear");
    }
    const bool controls_present =
        control_has_class(new_dialog, 1, OPUSW("Button")) &&
        control_has_class(new_dialog, 2, OPUSW("Button")) &&
        control_has_class(new_dialog, 0x0400, OPUSW("Button")) &&
        control_has_class(new_dialog, 0x0402, OPUSW("Button")) &&
        control_has_class(new_dialog, 0x0403, OPUSW("Button")) &&
        control_has_class(new_dialog, 0x0404, OPUSW("Edit")) &&
        control_has_class(new_dialog, 0x0405, OPUSW("ListBox"));
    if (!controls_present) {
        return fail(process, 6, "File New controls do not match the SDM contract");
    }
    if (!PostMessageW(new_dialog, kWmCommand, 1, 0)) {
        return fail(process, 7, "could not accept File New");
    }
    if (!wait_for_window_to_close(process.hProcess, process.dwProcessId,
                                  OPUSW("OpusSdmDialog"), 5000)) {
        return fail(process, 8, "File New dialog did not finish");
    }
    if (wait_for_window(process.hProcess, process.dwProcessId, nullptr,
                        OPUSW("Microsoft Word - Document2"), 5000) == nullptr) {
        return fail(process, 9,
                    "original File New action did not create Document2");
    }

    if (!PostMessageW(main_window, kWmCommand, kFileExit, 0)) {
        return fail(process, 10, "could not send File Exit");
    }
    if (WaitForSingleObject(process.hProcess, 5000) != WAIT_OBJECT_0) {
        return fail(process, 11, "two-document File Exit timed out");
    }
    DWORD exit_code = 0;
    if (!GetExitCodeProcess(process.hProcess, &exit_code) || exit_code != 0) {
        return fail(process, 12, "two-document File Exit was not clean");
    }

    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    return 0;
}

int wmain(const int argument_count, wchar_t** arguments) {
    std::vector<std::basic_string<WCHAR>> storage;
    storage.reserve(static_cast<std::size_t>(argument_count));
    for (int index = 0; index < argument_count; ++index) {
        storage.push_back(argument_to_wide(arguments[index]));
    }

    std::vector<WCHAR*> wide_arguments;
    wide_arguments.reserve(storage.size());
    for (auto& argument : storage) {
        wide_arguments.push_back(argument.data());
    }
    return run_ui_test(argument_count, wide_arguments.data());
}

#ifndef _WIN32
int main(const int argument_count, char** arguments) {
    std::vector<std::basic_string<WCHAR>> storage;
    storage.reserve(static_cast<std::size_t>(argument_count));
    for (int index = 0; index < argument_count; ++index) {
        std::basic_string<WCHAR> argument;
        if (arguments[index] != nullptr) {
            for (const unsigned char* text =
                     reinterpret_cast<const unsigned char*>(arguments[index]);
                 *text != 0; ++text) {
                argument.push_back(static_cast<WCHAR>(*text));
            }
        }
        storage.push_back(std::move(argument));
    }

    std::vector<WCHAR*> wide_arguments;
    wide_arguments.reserve(storage.size());
    for (auto& argument : storage) {
        wide_arguments.push_back(argument.data());
    }
    return run_ui_test(argument_count, wide_arguments.data());
}
#endif
