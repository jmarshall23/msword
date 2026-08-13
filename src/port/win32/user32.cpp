#include "windows.h"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <string>
#include <vector>

namespace {

constexpr DWORD kWindowMagic = 0x55533232u;

struct RegisteredClass {
    ATOM atom = 0;
    std::string name;
    WNDPROC procedure = nullptr;
    int window_extra = 0;
    HINSTANCE instance = nullptr;
};

struct WindowObject {
    DWORD magic = kWindowMagic;
    RegisteredClass klass;
    std::string text;
    DWORD style = 0;
    DWORD extended_style = 0;
    RECT rectangle{0, 0, 0, 0};
    HWND parent = nullptr;
    HWND owner = nullptr;
    HMENU menu = nullptr;
    HINSTANCE instance = nullptr;
    LONG_PTR user_data = 0;
    bool enabled = true;
    bool visible = false;
    std::vector<unsigned char> extra;
};

std::vector<RegisteredClass> g_classes;
std::vector<WindowObject*> g_windows;
std::deque<MSG> g_messages;
SHORT g_key_state[256]{};
ATOM g_next_atom = 1;
HWND g_active_window = nullptr;
HWND g_focus_window = nullptr;
HWND g_capture_window = nullptr;
HCURSOR g_current_cursor = nullptr;
POINT g_cursor_position{0, 0};

WindowObject* window_from_handle(HWND handle) {
    auto* window = static_cast<WindowObject*>(handle);
    return window != nullptr && window->magic == kWindowMagic ? window : nullptr;
}

std::string narrow_string(LPCWSTR text) {
    std::string result;
    if (text == nullptr) return result;
    while (*text != 0) {
        result.push_back(static_cast<char>(*text & 0xff));
        ++text;
    }
    return result;
}

bool class_name_is_atom(LPCSTR name) {
    return (reinterpret_cast<std::uintptr_t>(name) >> 16u) == 0;
}

RegisteredClass* find_class(LPCSTR name) {
    if (name == nullptr) return nullptr;
    if (class_name_is_atom(name)) {
        const auto atom = static_cast<ATOM>(reinterpret_cast<std::uintptr_t>(name));
        for (auto& klass : g_classes) {
            if (klass.atom == atom) return &klass;
        }
        return nullptr;
    }
    for (auto& klass : g_classes) {
        if (klass.name == name) return &klass;
    }
    return nullptr;
}

RegisteredClass builtin_class(LPCSTR name) {
    RegisteredClass klass{};
    klass.name = name != nullptr && !class_name_is_atom(name) ? name : "";
    return klass;
}

HWND handle_from_window(WindowObject* window) {
    return static_cast<HWND>(window);
}

WindowObject* first_child(HWND parent) {
    for (auto* window : g_windows) {
        if (window_from_handle(handle_from_window(window)) != nullptr &&
            window->parent == parent) {
            return window;
        }
    }
    return nullptr;
}

WindowObject* next_sibling(HWND window) {
    auto* object = window_from_handle(window);
    if (object == nullptr) return nullptr;
    bool found = false;
    for (auto* candidate : g_windows) {
        if (window_from_handle(handle_from_window(candidate)) == nullptr) continue;
        if (found && candidate->parent == object->parent) return candidate;
        if (candidate == object) found = true;
    }
    return nullptr;
}

POINT window_screen_origin(HWND window) {
    POINT point{0, 0};
    for (auto* object = window_from_handle(window); object != nullptr;
         object = window_from_handle(object->parent)) {
        point.x += object->rectangle.left;
        point.y += object->rectangle.top;
    }
    return point;
}

bool rect_empty(const RECT& rectangle) {
    return rectangle.right <= rectangle.left || rectangle.bottom <= rectangle.top;
}

bool message_matches(const MSG& message, HWND window, UINT filter_min,
                     UINT filter_max) {
    if (window != nullptr && message.hwnd != window) return false;
    if (filter_min != 0 || filter_max != 0) {
        if (message.message < filter_min || message.message > filter_max) {
            return false;
        }
    }
    return true;
}

BOOL queue_take(LPMSG message, HWND window, UINT filter_min, UINT filter_max,
                UINT remove_message) {
    if (message == nullptr) return FALSE;
    for (auto it = g_messages.begin(); it != g_messages.end(); ++it) {
        if (!message_matches(*it, window, filter_min, filter_max)) continue;
        *message = *it;
        if ((remove_message & PM_REMOVE) != 0) g_messages.erase(it);
        return TRUE;
    }
    return FALSE;
}

void queue_front(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
    g_messages.push_front({window, message, wparam, lparam, 0, {0, 0}});
}

void queue_back(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
    g_messages.push_back({window, message, wparam, lparam, 0, {0, 0}});
}

void update_key_state(UINT message, WPARAM wparam) {
    if (wparam > 255) return;
    auto& state = g_key_state[wparam];
    switch (message) {
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
            state = static_cast<SHORT>(state | 0x8000);
            break;
        case WM_KEYUP:
        case WM_SYSKEYUP:
            state = static_cast<SHORT>(state & ~0x8000);
            break;
    }
}

UINT translated_char(WPARAM virtual_key) {
    const bool shift_down = (g_key_state[VK_SHIFT] & 0x8000) != 0;
    if (virtual_key >= 'A' && virtual_key <= 'Z') {
        return static_cast<UINT>(shift_down ? virtual_key : virtual_key + 32);
    }
    if (virtual_key >= '0' && virtual_key <= '9') {
        return static_cast<UINT>(virtual_key);
    }
    switch (virtual_key) {
        case VK_SPACE: return ' ';
        case VK_RETURN: return '\r';
        case VK_TAB: return '\t';
        default: return 0;
    }
}

void pump_block_until_message() {
    OutputDebugStringA("user32 GetMessage/WaitMessage needs an event backend\n");
    std::abort();
}

COLORREF system_color(int index) {
    switch (index) {
        case COLOR_WINDOW: return RGB(255, 255, 255);
        case COLOR_WINDOWTEXT: return RGB(0, 0, 0);
        case COLOR_WINDOWFRAME: return RGB(0, 0, 0);
        case COLOR_BTNFACE: return RGB(192, 192, 192);
        case COLOR_BTNTEXT: return RGB(0, 0, 0);
        case COLOR_MENU: return RGB(192, 192, 192);
        case COLOR_APPWORKSPACE: return RGB(128, 128, 128);
        case COLOR_SCROLLBAR: return RGB(192, 192, 192);
        case COLOR_CAPTIONTEXT: return RGB(255, 255, 255);
        case COLOR_BTNSHADOW: return RGB(128, 128, 128);
        case COLOR_GRAYTEXT: return RGB(128, 128, 128);
        case COLOR_HIGHLIGHT: return RGB(0, 0, 128);
        case COLOR_HIGHLIGHTTEXT: return RGB(255, 255, 255);
        default: return RGB(0, 0, 0);
    }
}

LONG_PTR get_window_extra(const WindowObject& window, int index,
                          std::size_t bytes) {
    if (index < 0 ||
        static_cast<std::size_t>(index) + bytes > window.extra.size()) {
        return 0;
    }
    LONG_PTR value = 0;
    std::memcpy(&value, window.extra.data() + index, bytes);
    return value;
}

LONG_PTR set_window_extra(WindowObject& window, int index, LONG_PTR value,
                          std::size_t bytes) {
    if (index < 0 ||
        static_cast<std::size_t>(index) + bytes > window.extra.size()) {
        return 0;
    }
    const LONG_PTR previous = get_window_extra(window, index, bytes);
    std::memcpy(window.extra.data() + index, &value, bytes);
    return previous;
}

}  // namespace

extern "C" {

ATOM RegisterClassExA(const WNDCLASSEXA* window_class) {
    if (window_class == nullptr || window_class->lpszClassName == nullptr) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }
    if (find_class(window_class->lpszClassName) != nullptr) {
        SetLastError(ERROR_CLASS_ALREADY_EXISTS);
        return 0;
    }
    RegisteredClass klass{};
    klass.atom = g_next_atom++;
    klass.name = window_class->lpszClassName;
    klass.procedure = window_class->lpfnWndProc;
    klass.window_extra = (std::max)(0, window_class->cbWndExtra);
    klass.instance = window_class->hInstance;
    g_classes.push_back(klass);
    SetLastError(ERROR_SUCCESS);
    return klass.atom;
}

ATOM RegisterClassA(const WNDCLASSA* window_class) {
    if (window_class == nullptr) return 0;
    WNDCLASSEXA extended{};
    extended.cbSize = sizeof(extended);
    extended.style = window_class->style;
    extended.lpfnWndProc = window_class->lpfnWndProc;
    extended.cbClsExtra = window_class->cbClsExtra;
    extended.cbWndExtra = window_class->cbWndExtra;
    extended.hInstance = window_class->hInstance;
    extended.hIcon = window_class->hIcon;
    extended.hCursor = window_class->hCursor;
    extended.hbrBackground = window_class->hbrBackground;
    extended.lpszMenuName = window_class->lpszMenuName;
    extended.lpszClassName = window_class->lpszClassName;
    return RegisterClassExA(&extended);
}

ATOM RegisterClassExW(const WNDCLASSEXW* window_class) {
    if (window_class == nullptr) return 0;
    WNDCLASSEXA narrow{};
    narrow.cbSize = sizeof(narrow);
    narrow.style = window_class->style;
    narrow.lpfnWndProc = window_class->lpfnWndProc;
    narrow.cbClsExtra = window_class->cbClsExtra;
    narrow.cbWndExtra = window_class->cbWndExtra;
    narrow.hInstance = window_class->hInstance;
    narrow.hIcon = window_class->hIcon;
    narrow.hCursor = window_class->hCursor;
    narrow.hbrBackground = window_class->hbrBackground;
    const std::string class_name = narrow_string(window_class->lpszClassName);
    narrow.lpszClassName = class_name.c_str();
    return RegisterClassExA(&narrow);
}

HWND CreateWindowExA(DWORD extended_style, LPCSTR class_name,
                     LPCSTR window_name, DWORD style, int x, int y, int width,
                     int height, HWND parent, HMENU menu, HINSTANCE instance,
                     LPVOID parameter) {
    RegisteredClass klass{};
    if (auto* registered = find_class(class_name)) {
        klass = *registered;
    } else {
        klass = builtin_class(class_name);
    }
    auto* window = new WindowObject();
    window->klass = klass;
    window->text = window_name != nullptr ? window_name : "";
    window->style = style;
    window->extended_style = extended_style;
    window->rectangle = {x, y, x + width, y + height};
    window->parent = (style & WS_CHILD) != 0 ? parent : nullptr;
    window->owner = (style & WS_CHILD) == 0 ? parent : nullptr;
    window->menu = menu;
    window->instance = instance;
    window->visible = (style & WS_VISIBLE) != 0;
    window->extra.resize(static_cast<std::size_t>(klass.window_extra));
    HWND handle = static_cast<HWND>(window);
    g_windows.push_back(window);
    if (g_active_window == nullptr) g_active_window = handle;
    if (klass.procedure != nullptr) {
        CREATESTRUCTA create{};
        create.lpCreateParams = parameter;
        create.hInstance = instance;
        create.hMenu = menu;
        create.hwndParent = parent;
        create.cy = height;
        create.cx = width;
        create.y = y;
        create.x = x;
        create.style = static_cast<LONG>(style);
        create.lpszName = window_name;
        create.lpszClass = class_name;
        create.dwExStyle = extended_style;
        if (klass.procedure(handle, WM_NCCREATE, 0,
                            reinterpret_cast<LPARAM>(&create)) == 0) {
            DestroyWindow(handle);
            return nullptr;
        }
        klass.procedure(handle, WM_CREATE, 0, reinterpret_cast<LPARAM>(&create));
    }
    return handle;
}

HWND CreateWindowExW(DWORD extended_style, LPCWSTR class_name,
                     LPCWSTR window_name, DWORD style, int x, int y, int width,
                     int height, HWND parent, HMENU menu, HINSTANCE instance,
                     LPVOID parameter) {
    const std::string narrow_class = narrow_string(class_name);
    const std::string narrow_name = narrow_string(window_name);
    return CreateWindowExA(extended_style, narrow_class.c_str(),
                           narrow_name.c_str(), style, x, y, width, height,
                           parent, menu, instance, parameter);
}

BOOL IsWindow(HWND window) {
    return window_from_handle(window) != nullptr;
}

BOOL DestroyWindow(HWND window) {
    auto* object = window_from_handle(window);
    if (object == nullptr) return FALSE;
    while (auto* child = first_child(window)) {
        DestroyWindow(handle_from_window(child));
    }
    if (object->klass.procedure != nullptr) {
        object->klass.procedure(window, WM_DESTROY, 0, 0);
        object->klass.procedure(window, WM_NCDESTROY, 0, 0);
    }
    if (g_active_window == window) g_active_window = nullptr;
    if (g_focus_window == window) g_focus_window = nullptr;
    if (g_capture_window == window) g_capture_window = nullptr;
    for (auto it = g_messages.begin(); it != g_messages.end();) {
        if (it->hwnd == window) {
            it = g_messages.erase(it);
        } else {
            ++it;
        }
    }
    object->magic = 0;

    // ponytail: leak tiny destroyed window records; replace with generation
    // handles when churn matters.
    return TRUE;
}

HDC GetDC(HWND window) {
    if (window != nullptr && !IsWindow(window)) return nullptr;
    return CreateCompatibleDC(nullptr);
}

int ReleaseDC(HWND, HDC device_context) {
    return DeleteDC(device_context) ? 1 : 0;
}

int GetSystemMetrics(int index) {
    switch (index) {
        case SM_CXSCREEN: return 640;
        case SM_CYSCREEN: return 480;
        case SM_CXVSCROLL:
        case SM_CYHSCROLL:
        case SM_CYVSCROLL:
            return 16;
        case SM_CXBORDER:
        case SM_CYBORDER:
            return 1;
        case SM_CYCAPTION: return 19;
        case SM_CYMENU: return 18;
        case SM_CXFULLSCREEN: return 640;
        case SM_CYFULLSCREEN: return 480;
        case SM_MOUSEPRESENT: return 1;
        case SM_CYFRAME:
        case SM_CXFRAME:
        case SM_CXDLGFRAME:
        case SM_CYDLGFRAME:
            return 4;
        case SM_CXSMICON: return 16;
        case SM_CYVTHUMB:
        case SM_CXHTHUMB:
            return 16;
        case SM_CXICON:
        case SM_CYICON:
        case SM_CXCURSOR:
        case SM_CYCURSOR:
            return 32;
        case SM_CURSORLEVEL:
            return 0;
        default:
            return 0;
    }
}

DWORD GetSysColor(int index) {
    return system_color(index);
}

LONG_PTR GetWindowLongPtrA(HWND window, int index) {
    auto* object = window_from_handle(window);
    if (object == nullptr) return 0;
    switch (index) {
        case GWL_STYLE: return object->style;
        case GWL_EXSTYLE: return object->extended_style;
        case GWLP_USERDATA: return object->user_data;
        case GWLP_WNDPROC:
            return reinterpret_cast<LONG_PTR>(object->klass.procedure);
        case GWLP_HINSTANCE:
            return reinterpret_cast<LONG_PTR>(object->instance);
        default:
            return get_window_extra(*object, index, sizeof(LONG_PTR));
    }
}

LONG_PTR GetWindowLongPtrW(HWND window, int index) {
    return GetWindowLongPtrA(window, index);
}

LONG_PTR SetWindowLongPtrA(HWND window, int index, LONG_PTR new_long) {
    auto* object = window_from_handle(window);
    if (object == nullptr) return 0;
    switch (index) {
        case GWL_STYLE: {
            const LONG_PTR previous = object->style;
            object->style = static_cast<DWORD>(new_long);
            return previous;
        }
        case GWL_EXSTYLE: {
            const LONG_PTR previous = object->extended_style;
            object->extended_style = static_cast<DWORD>(new_long);
            return previous;
        }
        case GWLP_USERDATA: {
            const LONG_PTR previous = object->user_data;
            object->user_data = new_long;
            return previous;
        }
        case GWLP_WNDPROC: {
            const LONG_PTR previous =
                reinterpret_cast<LONG_PTR>(object->klass.procedure);
            object->klass.procedure = reinterpret_cast<WNDPROC>(new_long);
            return previous;
        }
        default:
            return set_window_extra(*object, index, new_long, sizeof(LONG_PTR));
    }
}

LONG_PTR SetWindowLongPtrW(HWND window, int index, LONG_PTR new_long) {
    return SetWindowLongPtrA(window, index, new_long);
}

LONG GetWindowLongA(HWND window, int index) {
    auto* object = window_from_handle(window);
    if (object == nullptr) return 0;
    switch (index) {
        case GWL_STYLE:
        case GWL_EXSTYLE:
        case GWLP_USERDATA:
        case GWLP_WNDPROC:
        case GWLP_HINSTANCE:
            return static_cast<LONG>(GetWindowLongPtrA(window, index));
        default:
            return static_cast<LONG>(
                get_window_extra(*object, index, sizeof(LONG)));
    }
}

LONG SetWindowLongA(HWND window, int index, LONG new_long) {
    auto* object = window_from_handle(window);
    if (object == nullptr) return 0;
    switch (index) {
        case GWL_STYLE:
        case GWL_EXSTYLE:
        case GWLP_USERDATA:
        case GWLP_WNDPROC:
        case GWLP_HINSTANCE:
            return static_cast<LONG>(SetWindowLongPtrA(window, index, new_long));
        default:
            return static_cast<LONG>(
                set_window_extra(*object, index, new_long, sizeof(LONG)));
    }
}

WORD GetWindowWord(HWND window, int index) {
    auto* object = window_from_handle(window);
    if (object == nullptr) return 0;
    return static_cast<WORD>(get_window_extra(*object, index, sizeof(WORD)));
}

WORD SetWindowWord(HWND window, int index, WORD new_word) {
    auto* object = window_from_handle(window);
    if (object == nullptr) return 0;
    return static_cast<WORD>(
        set_window_extra(*object, index, new_word, sizeof(WORD)));
}

LRESULT DefWindowProcA(HWND window, UINT message, WPARAM wparam,
                       LPARAM lparam) {
    auto* object = window_from_handle(window);
    switch (message) {
        case WM_NCCREATE:
            return 1;
        case WM_CLOSE:
            return DestroyWindow(window);
        case WM_SETTEXT:
            if (object == nullptr) return FALSE;
            object->text = lparam != 0 ? reinterpret_cast<LPCSTR>(lparam) : "";
            return TRUE;
        case WM_GETTEXT:
            if (object == nullptr || lparam == 0 || wparam == 0) return 0;
            lstrcpynA(reinterpret_cast<LPSTR>(lparam), object->text.c_str(),
                      static_cast<int>(wparam));
            return static_cast<LRESULT>(
                std::strlen(reinterpret_cast<LPCSTR>(lparam)));
        case WM_GETTEXTLENGTH:
            return object != nullptr ? static_cast<LRESULT>(object->text.size())
                                     : 0;
        default:
            return 0;
    }
}

LRESULT DefWindowProcW(HWND window, UINT message, WPARAM wparam,
                       LPARAM lparam) {
    auto* object = window_from_handle(window);
    switch (message) {
        case WM_NCCREATE:
            return 1;
        case WM_CLOSE:
            return DestroyWindow(window);
        case WM_SETTEXT:
            if (object == nullptr) return FALSE;
            object->text =
                lparam != 0 ? narrow_string(reinterpret_cast<LPCWSTR>(lparam))
                             : "";
            return TRUE;
        case WM_GETTEXT:
            if (object == nullptr || lparam == 0 || wparam == 0) return 0;
            {
                auto* output = reinterpret_cast<LPWSTR>(lparam);
                const std::size_t limit = static_cast<std::size_t>(wparam);
                const std::size_t count =
                    (std::min)(object->text.size(), limit - 1);
                for (std::size_t index = 0; index < count; ++index) {
                    output[index] = static_cast<WCHAR>(object->text[index]);
                }
                output[count] = 0;
                return static_cast<LRESULT>(count);
            }
        case WM_GETTEXTLENGTH:
            return object != nullptr ? static_cast<LRESULT>(object->text.size())
                                     : 0;
        default:
            return 0;
    }
}

HCURSOR LoadCursorA(HINSTANCE, LPCSTR cursor_name) {
    return const_cast<LPSTR>(cursor_name);
}

HCURSOR LoadCursorW(HINSTANCE, LPCWSTR cursor_name) {
    return const_cast<LPWSTR>(cursor_name);
}

HWND GetActiveWindow(void) {
    return g_active_window;
}

BOOL IsChild(HWND parent, HWND window) {
    for (auto* object = window_from_handle(window); object != nullptr;
         object = window_from_handle(object->parent)) {
        if (object->parent == parent) return TRUE;
    }
    return FALSE;
}

HWND GetParent(HWND window) {
    auto* object = window_from_handle(window);
    return object != nullptr ? object->parent : nullptr;
}

HWND GetWindow(HWND window, UINT command) {
    auto* object = window_from_handle(window);
    if (object == nullptr) return nullptr;
    switch (command) {
        case GW_OWNER: return object->owner;
        case GW_CHILD: return handle_from_window(first_child(window));
        case GW_HWNDNEXT: return handle_from_window(next_sibling(window));
        default: return nullptr;
    }
}

HWND GetTopWindow(HWND window) {
    return handle_from_window(first_child(window));
}

BOOL GetWindowRect(HWND window, LPRECT rectangle) {
    auto* object = window_from_handle(window);
    if (object == nullptr || rectangle == nullptr) return FALSE;
    const POINT origin = window_screen_origin(object->parent);
    *rectangle = object->rectangle;
    rectangle->left += origin.x;
    rectangle->right += origin.x;
    rectangle->top += origin.y;
    rectangle->bottom += origin.y;
    return TRUE;
}

BOOL GetClientRect(HWND window, LPRECT rectangle) {
    auto* object = window_from_handle(window);
    if (object == nullptr || rectangle == nullptr) return FALSE;
    rectangle->left = 0;
    rectangle->top = 0;
    rectangle->right = object->rectangle.right - object->rectangle.left;
    rectangle->bottom = object->rectangle.bottom - object->rectangle.top;
    return TRUE;
}

BOOL ClientToScreen(HWND window, LPPOINT point) {
    if (!IsWindow(window) || point == nullptr) return FALSE;
    const POINT origin = window_screen_origin(window);
    point->x += origin.x;
    point->y += origin.y;
    return TRUE;
}

BOOL ScreenToClient(HWND window, LPPOINT point) {
    if (!IsWindow(window) || point == nullptr) return FALSE;
    const POINT origin = window_screen_origin(window);
    point->x -= origin.x;
    point->y -= origin.y;
    return TRUE;
}

BOOL SetRect(LPRECT rectangle, int left, int top, int right, int bottom) {
    if (rectangle == nullptr) return FALSE;
    *rectangle = {left, top, right, bottom};
    return TRUE;
}

BOOL InflateRect(LPRECT rectangle, int dx, int dy) {
    if (rectangle == nullptr) return FALSE;
    rectangle->left -= dx;
    rectangle->right += dx;
    rectangle->top -= dy;
    rectangle->bottom += dy;
    return TRUE;
}

BOOL IntersectRect(LPRECT destination, const RECT* source1,
                   const RECT* source2) {
    if (destination == nullptr || source1 == nullptr || source2 == nullptr) {
        return FALSE;
    }
    RECT result{(std::max)(source1->left, source2->left),
                (std::max)(source1->top, source2->top),
                (std::min)(source1->right, source2->right),
                (std::min)(source1->bottom, source2->bottom)};
    if (rect_empty(result)) {
        result = {0, 0, 0, 0};
        *destination = result;
        return FALSE;
    }
    *destination = result;
    return TRUE;
}

BOOL OffsetRect(LPRECT rectangle, int dx, int dy) {
    if (rectangle == nullptr) return FALSE;
    rectangle->left += dx;
    rectangle->right += dx;
    rectangle->top += dy;
    rectangle->bottom += dy;
    return TRUE;
}

BOOL UnionRect(LPRECT destination, const RECT* source1, const RECT* source2) {
    if (destination == nullptr || source1 == nullptr || source2 == nullptr) {
        return FALSE;
    }
    const bool empty1 = rect_empty(*source1);
    const bool empty2 = rect_empty(*source2);
    if (empty1 && empty2) {
        *destination = {0, 0, 0, 0};
        return FALSE;
    }
    if (empty1) {
        *destination = *source2;
        return TRUE;
    }
    if (empty2) {
        *destination = *source1;
        return TRUE;
    }
    *destination = {(std::min)(source1->left, source2->left),
                    (std::min)(source1->top, source2->top),
                    (std::max)(source1->right, source2->right),
                    (std::max)(source1->bottom, source2->bottom)};
    return TRUE;
}

BOOL PtInRect(const RECT* rectangle, POINT point) {
    return rectangle != nullptr && point.x >= rectangle->left &&
           point.x < rectangle->right && point.y >= rectangle->top &&
           point.y < rectangle->bottom
               ? TRUE
               : FALSE;
}

BOOL AdjustWindowRectEx(LPRECT rectangle, DWORD style, BOOL menu,
                        DWORD extended_style) {
    if (rectangle == nullptr) return FALSE;
    int x = 0;
    int y = 0;
    if ((style & (WS_BORDER | WS_DLGFRAME | WS_THICKFRAME)) != 0 ||
        (extended_style & WS_EX_DLGMODALFRAME) != 0) {
        x += GetSystemMetrics(SM_CXFRAME);
        y += GetSystemMetrics(SM_CYFRAME);
    }
    if ((style & WS_CAPTION) != 0) y += GetSystemMetrics(SM_CYCAPTION);
    if (menu) y += GetSystemMetrics(SM_CYMENU);
    rectangle->left -= x;
    rectangle->right += x;
    rectangle->top -= y;
    rectangle->bottom += y;
    return TRUE;
}

BOOL SystemParametersInfoA(UINT action, UINT, LPVOID data, UINT) {
    if (action == SPI_GETWORKAREA && data != nullptr) {
        auto* rectangle = static_cast<RECT*>(data);
        *rectangle = {0, 0, GetSystemMetrics(SM_CXSCREEN),
                      GetSystemMetrics(SM_CYSCREEN)};
        return TRUE;
    }
    return FALSE;
}

BOOL ShowWindow(HWND window, int command_show) {
    auto* object = window_from_handle(window);
    if (object == nullptr) return FALSE;
    const BOOL was_visible = object->visible ? TRUE : FALSE;
    object->visible = command_show != SW_HIDE;
    if (object->visible) {
        object->style |= WS_VISIBLE;
    } else {
        object->style &= ~static_cast<DWORD>(WS_VISIBLE);
    }
    return was_visible;
}

BOOL EnableWindow(HWND window, BOOL enable) {
    auto* object = window_from_handle(window);
    if (object == nullptr) return FALSE;
    const BOOL was_enabled = object->enabled ? TRUE : FALSE;
    object->enabled = enable != FALSE;
    return was_enabled;
}

BOOL IsWindowEnabled(HWND window) {
    auto* object = window_from_handle(window);
    return object != nullptr && object->enabled ? TRUE : FALSE;
}

BOOL IsWindowVisible(HWND window) {
    for (auto* object = window_from_handle(window); object != nullptr;
         object = window_from_handle(object->parent)) {
        if (!object->visible) return FALSE;
    }
    return window_from_handle(window) != nullptr ? TRUE : FALSE;
}

HWND SetActiveWindow(HWND window) {
    if (window != nullptr && !IsWindow(window)) return nullptr;
    const HWND previous = g_active_window;
    g_active_window = window;
    return previous;
}

HWND SetFocus(HWND window) {
    if (window != nullptr && !IsWindow(window)) return nullptr;
    const HWND previous = g_focus_window;
    g_focus_window = window;
    return previous;
}

HWND GetFocus(void) {
    return g_focus_window;
}

HCURSOR SetCursor(HCURSOR cursor) {
    const HCURSOR previous = g_current_cursor;
    g_current_cursor = cursor;
    return previous;
}

HWND SetCapture(HWND window) {
    if (window != nullptr && !IsWindow(window)) return nullptr;
    const HWND previous = g_capture_window;
    g_capture_window = window;
    return previous;
}

HWND GetCapture(void) {
    return g_capture_window;
}

BOOL ReleaseCapture(void) {
    g_capture_window = nullptr;
    return TRUE;
}

BOOL SetCursorPos(int x, int y) {
    g_cursor_position = {x, y};
    return TRUE;
}

BOOL GetCursorPos(LPPOINT point) {
    if (point == nullptr) return FALSE;
    *point = g_cursor_position;
    return TRUE;
}

BOOL SetWindowPos(HWND window, HWND, int x, int y, int cx, int cy, UINT flags) {
    auto* object = window_from_handle(window);
    if (object == nullptr) return FALSE;
    if ((flags & SWP_NOMOVE) == 0) {
        const int width = object->rectangle.right - object->rectangle.left;
        const int height = object->rectangle.bottom - object->rectangle.top;
        object->rectangle.left = x;
        object->rectangle.top = y;
        object->rectangle.right = x + width;
        object->rectangle.bottom = y + height;
    }
    if ((flags & SWP_NOSIZE) == 0) {
        object->rectangle.right = object->rectangle.left + cx;
        object->rectangle.bottom = object->rectangle.top + cy;
    }
    if ((flags & SWP_SHOWWINDOW) != 0) ShowWindow(window, SW_SHOW);
    if ((flags & SWP_HIDEWINDOW) != 0) ShowWindow(window, SW_HIDE);
    return TRUE;
}

BOOL MoveWindow(HWND window, int x, int y, int width, int height, BOOL repaint) {
    return SetWindowPos(window, nullptr, x, y, width, height,
                        repaint ? 0 : SWP_NOREDRAW);
}

BOOL SetWindowTextA(HWND window, LPCSTR text) {
    auto* object = window_from_handle(window);
    if (object == nullptr) return FALSE;
    object->text = text != nullptr ? text : "";
    return TRUE;
}

BOOL SetWindowTextW(HWND window, LPCWSTR text) {
    return SetWindowTextA(window, narrow_string(text).c_str());
}

int GetWindowTextLengthA(HWND window) {
    auto* object = window_from_handle(window);
    return object != nullptr ? static_cast<int>(object->text.size()) : 0;
}

int GetWindowTextLengthW(HWND window) {
    return GetWindowTextLengthA(window);
}

int GetWindowTextA(HWND window, LPSTR text, int max_count) {
    auto* object = window_from_handle(window);
    if (object == nullptr || text == nullptr || max_count <= 0) return 0;
    lstrcpynA(text, object->text.c_str(), max_count);
    return static_cast<int>(std::strlen(text));
}

int GetWindowTextW(HWND window, LPWSTR text, int max_count) {
    auto* object = window_from_handle(window);
    if (object == nullptr || text == nullptr || max_count <= 0) return 0;
    const std::size_t limit = static_cast<std::size_t>(max_count);
    const std::size_t count = (std::min)(object->text.size(), limit - 1);
    for (std::size_t index = 0; index < count; ++index) {
        text[index] = static_cast<WCHAR>(object->text[index]);
    }
    text[count] = 0;
    return static_cast<int>(count);
}

BOOL UpdateWindow(HWND window) {
    return IsWindow(window);
}

BOOL IsDialogMessageA(HWND, LPMSG) {
    return FALSE;
}

BOOL TranslateMessage(const MSG* message) {
    if (message == nullptr) return FALSE;
    if (message->message != WM_KEYDOWN && message->message != WM_SYSKEYDOWN) {
        return FALSE;
    }
    const UINT character = translated_char(message->wParam);
    if (character == 0) return FALSE;
    queue_front(message->hwnd,
                message->message == WM_SYSKEYDOWN ? WM_SYSCHAR : WM_CHAR,
                character, message->lParam);
    return TRUE;
}

LRESULT SendMessageA(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
    auto* object = window_from_handle(window);
    if (object == nullptr) return 0;
    if (object->klass.procedure == nullptr) {
        return DefWindowProcA(window, message, wparam, lparam);
    }
    return object->klass.procedure(window, message, wparam, lparam);
}

LRESULT SendMessageW(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
    auto* object = window_from_handle(window);
    if (object == nullptr) return 0;
    if (object->klass.procedure == nullptr) {
        return DefWindowProcW(window, message, wparam, lparam);
    }
    return object->klass.procedure(window, message, wparam, lparam);
}

LRESULT DispatchMessageA(const MSG* message) {
    if (message == nullptr) return 0;
    return SendMessageA(message->hwnd, message->message, message->wParam,
                        message->lParam);
}

BOOL PostMessageA(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
    if (window != nullptr && !IsWindow(window)) return FALSE;
    update_key_state(message, wparam);
    queue_back(window, message, wparam, lparam);
    return TRUE;
}

BOOL PostMessageW(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
    return PostMessageA(window, message, wparam, lparam);
}

VOID PostQuitMessage(int exit_code) {
    queue_back(nullptr, WM_QUIT, static_cast<WPARAM>(exit_code), 0);
}

BOOL GetMessageA(LPMSG message, HWND window, UINT filter_min, UINT filter_max) {
    if (message == nullptr) return FALSE;
    for (;;) {
        if (queue_take(message, window, filter_min, filter_max, PM_REMOVE)) {
            return message->message == WM_QUIT ? FALSE : TRUE;
        }
        pump_block_until_message();
    }
}

BOOL PeekMessageA(LPMSG message, HWND window, UINT filter_min, UINT filter_max,
                  UINT remove_message) {
    return queue_take(message, window, filter_min, filter_max, remove_message);
}

BOOL WaitMessage(void) {
    if (!g_messages.empty()) return TRUE;
    pump_block_until_message();
    return FALSE;
}

SHORT GetKeyState(int virtual_key) {
    return virtual_key >= 0 && virtual_key < 256 ? g_key_state[virtual_key] : 0;
}

BOOL SetKeyboardState(BYTE* key_state) {
    if (key_state == nullptr) return FALSE;
    for (int index = 0; index < 256; ++index) {
        g_key_state[index] =
            static_cast<SHORT>((key_state[index] & 0x80) != 0 ? 0x8000 : 0) |
            static_cast<SHORT>(key_state[index] & 1);
    }
    return TRUE;
}

int MessageBoxA(HWND, LPCSTR, LPCSTR, UINT) {
    return IDOK;
}

}  // extern "C"
