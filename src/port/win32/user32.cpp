#include "windows.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <string>
#include <vector>

namespace {

constexpr DWORD kWindowMagic = 0x55533232u;
constexpr DWORD kMenuMagic = 0x4d454e55u;

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

struct TimerObject {
    HWND window = nullptr;
    UINT_PTR id = 0;
    UINT elapsed = 0;
    ULONGLONG due = 0;
    LPVOID callback = nullptr;
    bool queued = false;
};

struct MenuItem {
    UINT_PTR id_or_submenu = 0;
    UINT flags = MF_STRING;
    std::string text;
};

struct MenuObject {
    DWORD magic = kMenuMagic;
    bool popup = false;
    std::vector<MenuItem> items;
};

std::vector<RegisteredClass> g_classes;
std::vector<WindowObject*> g_windows;
std::deque<MSG> g_messages;
std::deque<MSG> g_scripted_messages;
std::vector<TimerObject> g_timers;
SHORT g_key_state[256]{};
ATOM g_next_atom = 1;
UINT_PTR g_next_timer_id = 1;
HWND g_active_window = nullptr;
HWND g_focus_window = nullptr;
HWND g_capture_window = nullptr;
HCURSOR g_current_cursor = nullptr;
POINT g_cursor_position{0, 0};
int g_cursor_show_count = 0;
bool g_quit_posted = false;
int g_quit_code = 0;

WindowObject* window_from_handle(HWND handle) {
    auto* window = static_cast<WindowObject*>(handle);
    return window != nullptr && window->magic == kWindowMagic ? window : nullptr;
}

MenuObject* menu_from_handle(HMENU handle) {
    auto* menu = static_cast<MenuObject*>(handle);
    return menu != nullptr && menu->magic == kMenuMagic ? menu : nullptr;
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

std::vector<HWND> window_snapshot() {
    std::vector<HWND> handles;
    handles.reserve(g_windows.size());
    for (auto* window : g_windows) {
        const HWND handle = handle_from_window(window);
        if (window_from_handle(handle) != nullptr) handles.push_back(handle);
    }
    return handles;
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

bool class_matches(const WindowObject& window, LPCSTR class_name) {
    if (class_name == nullptr) return true;
    if (class_name_is_atom(class_name)) {
        return window.klass.atom ==
               static_cast<ATOM>(reinterpret_cast<std::uintptr_t>(class_name));
    }
    return window.klass.name == class_name;
}

bool text_matches(const WindowObject& window, LPCSTR text) {
    return text == nullptr || window.text == text;
}

std::size_t menu_item_index(MenuObject& menu, UINT item, UINT flags) {
    if ((flags & MF_BYPOSITION) != 0) {
        return item < menu.items.size() ? item : menu.items.size();
    }
    for (std::size_t index = 0; index < menu.items.size(); ++index) {
        const auto& candidate = menu.items[index];
        if ((candidate.flags & MF_POPUP) == 0 && candidate.id_or_submenu == item) {
            return index;
        }
    }
    return menu.items.size();
}

UINT menu_item_flags(UINT flags) {
    return flags & ~(MF_APPEND | MF_CHANGE | MF_DELETE | MF_INSERT | MF_REMOVE |
                     MF_BYPOSITION);
}

BOOL insert_menu_item(HMENU handle, UINT position, UINT flags, UINT_PTR new_item,
                      LPCSTR text) {
    auto* menu = menu_from_handle(handle);
    if (menu == nullptr) return FALSE;
    MenuItem item{};
    item.flags = menu_item_flags(flags);
    if ((item.flags & MF_SEPARATOR) != 0) {
        item.id_or_submenu = 0;
    } else {
        item.id_or_submenu = new_item;
        item.text = text != nullptr ? text : "";
    }
    const std::size_t index =
        (flags & MF_APPEND) != 0 ? menu->items.size()
                                 : menu_item_index(*menu, position, flags);
    if (index > menu->items.size()) return FALSE;
    menu->items.insert(menu->items.begin() + static_cast<std::ptrdiff_t>(index),
                       item);
    return TRUE;
}

BOOL change_menu_item(HMENU handle, UINT position, UINT flags, UINT_PTR new_item,
                      LPCSTR text) {
    auto* menu = menu_from_handle(handle);
    if (menu == nullptr) return FALSE;
    const std::size_t index = menu_item_index(*menu, position, flags);
    if (index >= menu->items.size()) return FALSE;
    MenuItem& item = menu->items[index];
    item.flags = menu_item_flags(flags);
    item.id_or_submenu = (item.flags & MF_SEPARATOR) != 0 ? 0 : new_item;
    item.text = (item.flags & MF_SEPARATOR) != 0 || text == nullptr ? "" : text;
    return TRUE;
}

BOOL remove_menu_item(HMENU handle, UINT position, UINT flags) {
    auto* menu = menu_from_handle(handle);
    if (menu == nullptr) return FALSE;
    const std::size_t index = menu_item_index(*menu, position, flags);
    if (index >= menu->items.size()) return FALSE;
    menu->items.erase(menu->items.begin() + static_cast<std::ptrdiff_t>(index));
    return TRUE;
}

void destroy_menu_tree(HMENU handle) {
    auto* menu = menu_from_handle(handle);
    if (menu == nullptr) return;
    std::vector<HMENU> submenus;
    for (const auto& item : menu->items) {
        if ((item.flags & MF_POPUP) != 0) {
            submenus.push_back(reinterpret_cast<HMENU>(item.id_or_submenu));
        }
    }
    menu->magic = 0;
    for (auto* window : g_windows) {
        if (window_from_handle(handle_from_window(window)) != nullptr &&
            window->menu == handle) {
            window->menu = nullptr;
        }
    }
    for (HMENU submenu : submenus) destroy_menu_tree(submenu);
}

bool enum_child_windows(HWND parent, WNDENUMPROC enum_func, LPARAM parameter) {
    for (HWND handle : window_snapshot()) {
        auto* object = window_from_handle(handle);
        if (object == nullptr || object->parent != parent) continue;
        if (!enum_func(handle, parameter)) return false;
        if (!enum_child_windows(handle, enum_func, parameter)) return false;
    }
    return true;
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

POINT point_from_lparam(LPARAM lparam) {
    return {static_cast<SHORT>(LOWORD(lparam)),
            static_cast<SHORT>(HIWORD(lparam))};
}

bool point_in_window(HWND window, POINT point) {
    RECT rectangle{};
    return GetWindowRect(window, &rectangle) && PtInRect(&rectangle, point);
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

void clear_timer_queued(const MSG& message) {
    if (message.message != WM_TIMER) return;
    for (auto& timer : g_timers) {
        if (timer.window == message.hwnd && timer.id == message.wParam) {
            timer.queued = false;
            return;
        }
    }
}

BOOL queue_take(LPMSG message, HWND window, UINT filter_min, UINT filter_max,
                UINT remove_message) {
    if (message == nullptr) return FALSE;
    for (auto it = g_messages.begin(); it != g_messages.end(); ++it) {
        if (!message_matches(*it, window, filter_min, filter_max)) continue;
        *message = *it;
        if ((remove_message & PM_REMOVE) != 0) {
            clear_timer_queued(*it);
            g_messages.erase(it);
        }
        return TRUE;
    }
    if (g_quit_posted) {
        MSG quit{nullptr, WM_QUIT, static_cast<WPARAM>(g_quit_code), 0, 0,
                 {0, 0}};
        *message = quit;
        if ((remove_message & PM_REMOVE) != 0) g_quit_posted = false;
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

void remove_timer_messages(HWND window, UINT_PTR id) {
    for (auto it = g_messages.begin(); it != g_messages.end();) {
        if (it->hwnd == window && it->message == WM_TIMER && it->wParam == id) {
            it = g_messages.erase(it);
        } else {
            ++it;
        }
    }
}

void remove_window_timers(HWND window) {
    g_timers.erase(
        std::remove_if(g_timers.begin(), g_timers.end(),
                       [window](const TimerObject& timer) {
                           return timer.window == window;
                       }),
        g_timers.end());
}

void update_key_state(UINT message, WPARAM wparam) {
    int virtual_key = static_cast<int>(wparam);
    switch (message) {
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
            virtual_key = VK_LBUTTON;
            break;
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
            virtual_key = VK_RBUTTON;
            break;
    }
    if (virtual_key < 0 || virtual_key >= 256) return;
    auto& state = g_key_state[virtual_key];
    switch (message) {
        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
            state = static_cast<SHORT>(state | 0x8000);
            break;
        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
        case WM_KEYUP:
        case WM_SYSKEYUP:
            state = static_cast<SHORT>(state & ~0x8000);
            break;
    }
}

bool pump_timer_once() {
    const ULONGLONG now = GetTickCount64();
    for (auto& timer : g_timers) {
        if (timer.queued || timer.due > now) continue;
        queue_back(timer.window, WM_TIMER, timer.id,
                   reinterpret_cast<LPARAM>(timer.callback));
        timer.queued = true;
        timer.due = now + timer.elapsed;
        return true;
    }
    return false;
}

bool next_timer_due(ULONGLONG* due) {
    bool found = false;
    for (const auto& timer : g_timers) {
        if (timer.queued) continue;
        if (!found || timer.due < *due) *due = timer.due;
        found = true;
    }
    return found;
}

bool pump_once() {
    if (g_scripted_messages.empty()) return pump_timer_once();
    MSG message = g_scripted_messages.front();
    g_scripted_messages.pop_front();
    update_key_state(message.message, message.wParam);
    g_messages.push_back(message);
    return true;
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
    if (pump_once()) return;
    ULONGLONG due = 0;
    if (next_timer_due(&due)) {
        const ULONGLONG now = GetTickCount64();
        if (due > now) Sleep(static_cast<DWORD>(due - now));
        if (pump_once()) return;
    }
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

BOOL EnumWindows(WNDENUMPROC enum_func, LPARAM parameter) {
    if (enum_func == nullptr) return FALSE;
    for (HWND handle : window_snapshot()) {
        auto* object = window_from_handle(handle);
        if (object == nullptr || object->parent != nullptr) continue;
        if (!enum_func(handle, parameter)) return FALSE;
    }
    return TRUE;
}

BOOL EnumChildWindows(HWND parent, WNDENUMPROC enum_func, LPARAM parameter) {
    if (enum_func == nullptr) return FALSE;
    if (parent != nullptr && !IsWindow(parent)) return FALSE;
    return enum_child_windows(parent, enum_func, parameter) ? TRUE : FALSE;
}

BOOL EnumThreadWindows(DWORD, WNDENUMPROC enum_func, LPARAM parameter) {
    return EnumWindows(enum_func, parameter);
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
    remove_window_timers(window);
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

HMENU CreateMenu(void) {
    return static_cast<HMENU>(new MenuObject());
}

HMENU CreatePopupMenu(void) {
    auto* menu = new MenuObject();
    menu->popup = true;
    return static_cast<HMENU>(menu);
}

BOOL IsMenu(HMENU menu) {
    return menu_from_handle(menu) != nullptr ? TRUE : FALSE;
}

BOOL DestroyMenu(HMENU menu) {
    if (!IsMenu(menu)) return FALSE;
    destroy_menu_tree(menu);
    return TRUE;
}

BOOL AppendMenuA(HMENU menu, UINT flags, UINT_PTR new_item,
                 LPCSTR new_item_text) {
    return insert_menu_item(menu, 0, flags | MF_APPEND, new_item, new_item_text);
}

BOOL AppendMenuW(HMENU menu, UINT flags, UINT_PTR new_item,
                 LPCWSTR new_item_text) {
    const std::string text = narrow_string(new_item_text);
    return AppendMenuA(menu, flags, new_item,
                       new_item_text != nullptr ? text.c_str() : nullptr);
}

BOOL InsertMenuA(HMENU menu, UINT position, UINT flags, UINT_PTR new_item,
                 LPCSTR new_item_text) {
    return insert_menu_item(menu, position, flags, new_item, new_item_text);
}

BOOL InsertMenuW(HMENU menu, UINT position, UINT flags, UINT_PTR new_item,
                 LPCWSTR new_item_text) {
    const std::string text = narrow_string(new_item_text);
    return InsertMenuA(menu, position, flags, new_item,
                       new_item_text != nullptr ? text.c_str() : nullptr);
}

BOOL ModifyMenuA(HMENU menu, UINT position, UINT flags, UINT_PTR new_item,
                 LPCSTR new_item_text) {
    return change_menu_item(menu, position, flags, new_item, new_item_text);
}

BOOL ModifyMenuW(HMENU menu, UINT position, UINT flags, UINT_PTR new_item,
                 LPCWSTR new_item_text) {
    const std::string text = narrow_string(new_item_text);
    return ModifyMenuA(menu, position, flags, new_item,
                       new_item_text != nullptr ? text.c_str() : nullptr);
}

BOOL DeleteMenu(HMENU menu, UINT position, UINT flags) {
    return remove_menu_item(menu, position, flags);
}

BOOL RemoveMenu(HMENU menu, UINT position, UINT flags) {
    return remove_menu_item(menu, position, flags);
}

int GetMenuItemCount(HMENU handle) {
    auto* menu = menu_from_handle(handle);
    return menu != nullptr ? static_cast<int>(menu->items.size()) : -1;
}

UINT GetMenuItemID(HMENU handle, int position) {
    auto* menu = menu_from_handle(handle);
    if (menu == nullptr || position < 0 ||
        static_cast<std::size_t>(position) >= menu->items.size()) {
        return static_cast<UINT>(-1);
    }
    const auto& item = menu->items[static_cast<std::size_t>(position)];
    return (item.flags & MF_POPUP) != 0 ? static_cast<UINT>(-1)
                                        : static_cast<UINT>(item.id_or_submenu);
}

HMENU GetSubMenu(HMENU handle, int position) {
    auto* menu = menu_from_handle(handle);
    if (menu == nullptr || position < 0 ||
        static_cast<std::size_t>(position) >= menu->items.size()) {
        return nullptr;
    }
    const auto& item = menu->items[static_cast<std::size_t>(position)];
    return (item.flags & MF_POPUP) != 0
               ? reinterpret_cast<HMENU>(item.id_or_submenu)
               : nullptr;
}

UINT GetMenuState(HMENU handle, UINT id, UINT flags) {
    auto* menu = menu_from_handle(handle);
    if (menu == nullptr) return static_cast<UINT>(-1);
    const std::size_t index = menu_item_index(*menu, id, flags);
    if (index >= menu->items.size()) return static_cast<UINT>(-1);
    const auto& item = menu->items[index];
    if ((item.flags & MF_POPUP) != 0) {
        const auto* submenu =
            menu_from_handle(reinterpret_cast<HMENU>(item.id_or_submenu));
        const UINT count =
            submenu != nullptr ? static_cast<UINT>(submenu->items.size()) : 0;
        return (count << 8u) | (item.flags & 0xffu);
    }
    return item.flags;
}

int GetMenuStringW(HMENU handle, UINT item_id, LPWSTR string, int max_count,
                   UINT flags) {
    auto* menu = menu_from_handle(handle);
    if (menu == nullptr) return 0;
    const std::size_t index = menu_item_index(*menu, item_id, flags);
    if (index >= menu->items.size()) return 0;
    const std::string& text = menu->items[index].text;
    if (string == nullptr || max_count <= 0) return static_cast<int>(text.size());
    const std::size_t limit = static_cast<std::size_t>(max_count);
    const std::size_t count = (std::min)(text.size(), limit - 1);
    for (std::size_t i = 0; i < count; ++i) {
        string[i] = static_cast<WCHAR>(text[i]);
    }
    string[count] = 0;
    return static_cast<int>(count);
}

DWORD CheckMenuItem(HMENU handle, UINT item_id, UINT check) {
    auto* menu = menu_from_handle(handle);
    if (menu == nullptr) return static_cast<DWORD>(-1);
    const std::size_t index = menu_item_index(*menu, item_id, check);
    if (index >= menu->items.size()) return static_cast<DWORD>(-1);
    auto& item = menu->items[index];
    const DWORD previous =
        (item.flags & MF_CHECKED) != 0 ? MF_CHECKED : MF_UNCHECKED;
    if ((check & MF_CHECKED) != 0) {
        item.flags |= MF_CHECKED;
    } else {
        item.flags &= ~static_cast<UINT>(MF_CHECKED);
    }
    return previous;
}

BOOL CheckMenuRadioItem(HMENU handle, UINT first, UINT last, UINT check,
                        UINT flags) {
    auto* menu = menu_from_handle(handle);
    if (menu == nullptr) return FALSE;
    bool checked = false;
    for (std::size_t index = 0; index < menu->items.size(); ++index) {
        auto& item = menu->items[index];
        const UINT key = (flags & MF_BYPOSITION) != 0
                             ? static_cast<UINT>(index)
                             : static_cast<UINT>(item.id_or_submenu);
        if (key < first || key > last) continue;
        if (key == check) {
            item.flags |= MF_CHECKED;
            checked = true;
        } else {
            item.flags &= ~static_cast<UINT>(MF_CHECKED);
        }
    }
    return checked ? TRUE : FALSE;
}

BOOL SetMenuInfo(HMENU menu, const MENUINFO*) {
    return IsMenu(menu);
}

BOOL SetMenuItemBitmaps(HMENU handle, UINT position, UINT flags, HBITMAP,
                        HBITMAP) {
    auto* menu = menu_from_handle(handle);
    return menu != nullptr && menu_item_index(*menu, position, flags) <
                                  menu->items.size()
               ? TRUE
               : FALSE;
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
            return g_cursor_show_count;
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
        case WM_NCHITTEST:
            return object != nullptr &&
                           point_in_window(window, point_from_lparam(lparam))
                       ? HTCLIENT
                       : HTNOWHERE;
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
        case WM_NCHITTEST:
            return object != nullptr &&
                           point_in_window(window, point_from_lparam(lparam))
                       ? HTCLIENT
                       : HTNOWHERE;
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

HMENU GetMenu(HWND window) {
    auto* object = window_from_handle(window);
    return object != nullptr ? object->menu : nullptr;
}

BOOL SetMenu(HWND window, HMENU menu) {
    auto* object = window_from_handle(window);
    if (object == nullptr || (menu != nullptr && !IsMenu(menu))) return FALSE;
    object->menu = menu;
    return TRUE;
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

HWND GetAncestor(HWND window, UINT flags) {
    auto* object = window_from_handle(window);
    if (object == nullptr) return nullptr;
    if (flags == GA_PARENT) return object->parent;
    HWND current = window;
    while (auto* current_object = window_from_handle(current)) {
        if (current_object->parent == nullptr) break;
        current = current_object->parent;
    }
    if (flags == GA_ROOT) return current;
    if (flags == GA_ROOTOWNER) {
        while (auto* current_object = window_from_handle(current)) {
            if (current_object->owner == nullptr) break;
            current = current_object->owner;
        }
        return current;
    }
    return nullptr;
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

BOOL BringWindowToTop(HWND window) {
    auto* object = window_from_handle(window);
    if (object == nullptr) return FALSE;
    auto it = std::find(g_windows.begin(), g_windows.end(), object);
    if (it != g_windows.end()) {
        g_windows.erase(it);
        g_windows.push_back(object);
    }
    g_active_window = window;
    return TRUE;
}

BOOL DrawMenuBar(HWND window) {
    return IsWindow(window);
}

HCURSOR SetCursor(HCURSOR cursor) {
    const HCURSOR previous = g_current_cursor;
    g_current_cursor = cursor;
    return previous;
}

int ShowCursor(BOOL show) {
    return show ? ++g_cursor_show_count : --g_cursor_show_count;
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

HWND WindowFromPoint(POINT point) {
    for (auto it = g_windows.rbegin(); it != g_windows.rend(); ++it) {
        HWND window = handle_from_window(*it);
        if (!IsWindow(window) || !IsWindowVisible(window) ||
            !IsWindowEnabled(window) || !point_in_window(window, point)) {
            continue;
        }
        const LRESULT hit =
            SendMessageA(window, WM_NCHITTEST, 0, MAKELPARAM(point.x, point.y));
        if (hit != HTNOWHERE && hit != HTTRANSPARENT) return window;
    }
    return nullptr;
}

HWND FindWindowA(LPCSTR class_name, LPCSTR window_name) {
    for (HWND handle : window_snapshot()) {
        auto* object = window_from_handle(handle);
        if (object == nullptr || object->parent != nullptr) continue;
        if (class_matches(*object, class_name) &&
            text_matches(*object, window_name)) {
            return handle;
        }
    }
    return nullptr;
}

HWND FindWindowExW(HWND parent, HWND child_after, LPCWSTR class_name,
                   LPCWSTR window_name) {
    const std::string narrow_class = narrow_string(class_name);
    const std::string narrow_text = narrow_string(window_name);
    const LPCSTR class_filter = class_name != nullptr ? narrow_class.c_str() : nullptr;
    const LPCSTR text_filter = window_name != nullptr ? narrow_text.c_str() : nullptr;
    bool after_seen = child_after == nullptr;
    for (HWND handle : window_snapshot()) {
        auto* object = window_from_handle(handle);
        if (object == nullptr || object->parent != parent) continue;
        if (!after_seen) {
            if (handle == child_after) after_seen = true;
            continue;
        }
        if (class_matches(*object, class_filter) &&
            text_matches(*object, text_filter)) {
            return handle;
        }
    }
    return nullptr;
}

int GetClassNameW(HWND window, LPWSTR class_name, int max_count) {
    auto* object = window_from_handle(window);
    if (object == nullptr || class_name == nullptr || max_count <= 0) return 0;
    const auto limit = static_cast<std::size_t>(max_count);
    const std::size_t count = (std::min)(object->klass.name.size(), limit - 1);
    for (std::size_t index = 0; index < count; ++index) {
        class_name[index] = static_cast<WCHAR>(object->klass.name[index]);
    }
    class_name[count] = 0;
    return static_cast<int>(count);
}

int GetDlgCtrlID(HWND control) {
    auto* object = window_from_handle(control);
    if (object == nullptr) return 0;
    return static_cast<int>(reinterpret_cast<INT_PTR>(object->menu));
}

BOOL IsIconic(HWND window) {
    auto* object = window_from_handle(window);
    return object != nullptr && (object->style & WS_MINIMIZE) != 0 ? TRUE : FALSE;
}

BOOL IsZoomed(HWND window) {
    auto* object = window_from_handle(window);
    return object != nullptr && (object->style & WS_MAXIMIZE) != 0 ? TRUE : FALSE;
}

BOOL OpenIcon(HWND window) {
    auto* object = window_from_handle(window);
    if (object == nullptr) return FALSE;
    object->style &= ~static_cast<DWORD>(WS_MINIMIZE);
    return ShowWindow(window, SW_SHOW), TRUE;
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

BOOL MessageBeep(UINT) {
    return TRUE;
}

UINT TrackPopupMenu(HMENU, UINT, int, int, int, HWND, const RECT*) {
    return 0;
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

void OpusUser32PushScriptedInput(HWND window, UINT message, WPARAM wparam,
                                 LPARAM lparam) {
    g_scripted_messages.push_back({window, message, wparam, lparam, 0, {0, 0}});
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

UINT_PTR SetTimer(HWND window, UINT_PTR event, UINT elapsed, LPVOID timer_func) {
    if (window != nullptr && !IsWindow(window)) return 0;
    UINT_PTR id = event;
    if (id == 0) {
        do {
            id = g_next_timer_id++;
            if (g_next_timer_id == 0) g_next_timer_id = 1;
        } while (std::any_of(g_timers.begin(), g_timers.end(),
                             [window, id](const TimerObject& timer) {
                                 return timer.window == window &&
                                        timer.id == id;
                             }));
    }
    remove_timer_messages(window, id);
    const auto interval = (std::max)(elapsed, 1u);
    const ULONGLONG due = GetTickCount64() + interval;
    for (auto& timer : g_timers) {
        if (timer.window == window && timer.id == id) {
            timer.elapsed = interval;
            timer.due = due;
            timer.callback = timer_func;
            timer.queued = false;
            return id;
        }
    }
    g_timers.push_back({window, id, interval, due, timer_func, false});
    return id;
}

BOOL KillTimer(HWND window, UINT_PTR event) {
    const auto previous_size = g_timers.size();
    g_timers.erase(
        std::remove_if(g_timers.begin(), g_timers.end(),
                       [window, event](const TimerObject& timer) {
                           return timer.window == window && timer.id == event;
                       }),
        g_timers.end());
    if (g_timers.size() == previous_size) return FALSE;
    remove_timer_messages(window, event);
    return TRUE;
}

VOID PostQuitMessage(int exit_code) {
    g_quit_posted = true;
    g_quit_code = exit_code;
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
    pump_once();
    return queue_take(message, window, filter_min, filter_max, remove_message);
}

BOOL WaitMessage(void) {
    if (!g_messages.empty() || g_quit_posted || pump_once()) return TRUE;
    pump_block_until_message();
    return TRUE;
}

SHORT GetKeyState(int virtual_key) {
    return virtual_key >= 0 && virtual_key < 256 ? g_key_state[virtual_key] : 0;
}

SHORT GetAsyncKeyState(int virtual_key) {
    return GetKeyState(virtual_key);
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
