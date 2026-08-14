#include "windows.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define OPUS_WINDOW_MAGIC 0x55533232u
#define OPUS_MENU_MAGIC 0x4d454e55u

struct RegisteredClass {
    ATOM atom;
    char* name;
    WNDPROC procedure;
    int window_extra;
    HINSTANCE instance;
};

struct WindowProperty {
    BOOL atom;
    WORD atom_value;
    char* name;
    HANDLE data;
};

struct WindowObject {
    DWORD magic;
    struct RegisteredClass klass;
    char* text;
    DWORD style;
    DWORD extended_style;
    RECT rectangle;
    HWND parent;
    HWND owner;
    HMENU menu;
    HMENU system_menu;
    HINSTANCE instance;
    LONG_PTR user_data;
    BOOL enabled;
    BOOL visible;
    int scroll_min[2];
    int scroll_max[2];
    int scroll_pos[2];
    unsigned char* extra;
    size_t extra_size;
    struct WindowProperty* properties;
    size_t property_count;
    size_t property_capacity;
};

struct TimerObject {
    HWND window;
    UINT_PTR id;
    UINT elapsed;
    ULONGLONG due;
    LPVOID callback;
    BOOL queued;
};

struct MenuItem {
    UINT_PTR id_or_submenu;
    UINT flags;
    char* text;
};

struct MenuObject {
    DWORD magic;
    BOOL popup;
    struct MenuItem* items;
    size_t item_count;
    size_t item_capacity;
};

struct ClipboardFormat {
    UINT id;
    char* name;
};

struct ClipboardItem {
    UINT format;
    HANDLE data;
};

static struct RegisteredClass* g_classes;
static size_t g_class_count;
static size_t g_class_capacity;
static struct WindowObject** g_windows;
static size_t g_window_count;
static size_t g_window_capacity;
static MSG* g_messages;
static size_t g_message_count;
static size_t g_message_capacity;
static MSG* g_scripted_messages;
static size_t g_scripted_count;
static size_t g_scripted_capacity;
static struct TimerObject* g_timers;
static size_t g_timer_count;
static size_t g_timer_capacity;
static SHORT g_key_state[256];
static ATOM g_next_atom = 1;
static UINT_PTR g_next_timer_id = 1;
static HWND g_active_window;
static HWND g_focus_window;
static HWND g_capture_window;
static HCURSOR g_current_cursor;
static POINT g_cursor_position;
static int g_cursor_show_count;
static BOOL g_quit_posted;
static int g_quit_code;
static UINT g_expected_scripted_char;
static BOOL g_matched_scripted_char;
static HBRUSH g_system_color_brushes[32];
static struct ClipboardFormat* g_clipboard_formats;
static size_t g_clipboard_format_count;
static size_t g_clipboard_format_capacity;
static UINT g_next_clipboard_format = 0xc000;
static struct ClipboardItem* g_clipboard_items;
static size_t g_clipboard_item_count;
static size_t g_clipboard_item_capacity;
static BOOL g_clipboard_open;
static HWND g_clipboard_owner;

static int max_int(int left, int right) {
    return left > right ? left : right;
}

static int min_int(int left, int right) {
    return left < right ? left : right;
}

static char* dup_string(const char* text) {
    if (text == NULL) text = "";
    const size_t length = strlen(text) + 1;
    char* copy = (char*)malloc(length);
    if (copy == NULL) return NULL;
    memcpy(copy, text, length);
    return copy;
}

static char* narrow_string_dup(LPCWSTR text) {
    if (text == NULL) return dup_string("");
    size_t length = 0;
    while (text[length] != 0) ++length;
    char* result = (char*)malloc(length + 1);
    if (result == NULL) return NULL;
    size_t index;
    for (index = 0; index < length; ++index) {
        result[index] = (char)(text[index] & 0xff);
    }
    result[length] = '\0';
    return result;
}

static BOOL reserve_bytes(void** data, size_t* capacity, size_t count,
                          size_t element_size) {
    if (count <= *capacity) return TRUE;
    size_t next = *capacity == 0 ? 4 : *capacity * 2;
    while (next < count) next *= 2;
    void* resized = realloc(*data, next * element_size);
    if (resized == NULL) return FALSE;
    *data = resized;
    *capacity = next;
    return TRUE;
}

static struct WindowObject* window_from_handle(HWND handle) {
    struct WindowObject* window = (struct WindowObject*)handle;
    return window != NULL && window->magic == OPUS_WINDOW_MAGIC ? window
                                                                : NULL;
}

static struct MenuObject* menu_from_handle(HMENU handle) {
    struct MenuObject* menu = (struct MenuObject*)handle;
    return menu != NULL && menu->magic == OPUS_MENU_MAGIC ? menu : NULL;
}

static BOOL class_name_is_atom(LPCSTR name) {
    return ((uintptr_t)name >> 16u) == 0;
}

static BOOL property_name_is_atom(const void* name) {
    return ((uintptr_t)name >> 16u) == 0;
}

static char fold_property_char(char value) {
    return value >= 'A' && value <= 'Z' ? (char)(value + ('a' - 'A')) : value;
}

static WCHAR fold_wide_ascii(WCHAR value) {
    return value >= 'A' && value <= 'Z' ? (WCHAR)(value + ('a' - 'A')) : value;
}

static int compare_wide_strings(LPCWSTR first, LPCWSTR second,
                                BOOL ignore_case) {
    static const WCHAR empty[] = {0};
    if (first == second) return 0;
    if (first == NULL) first = empty;
    if (second == NULL) second = empty;
    for (;;) {
        const WCHAR left = ignore_case ? fold_wide_ascii(*first) : *first;
        const WCHAR right = ignore_case ? fold_wide_ascii(*second) : *second;
        if (left != right || left == 0 || right == 0) return left - right;
        ++first;
        ++second;
    }
}

static char* property_name_key_w(LPCWSTR name) {
    if (name == NULL || property_name_is_atom(name)) return dup_string("");
    size_t length = 0;
    while (name[length] != 0) ++length;
    char* result = (char*)malloc(length + 1);
    if (result == NULL) return NULL;
    size_t index;
    for (index = 0; index < length; ++index) {
        result[index] = fold_property_char((char)(name[index] & 0xff));
    }
    result[length] = '\0';
    return result;
}

static char* property_name_key_a(LPCSTR name) {
    if (name == NULL || property_name_is_atom(name)) return dup_string("");
    const size_t length = strlen(name);
    char* result = (char*)malloc(length + 1);
    if (result == NULL) return NULL;
    size_t index;
    for (index = 0; index < length; ++index) {
        result[index] = fold_property_char(name[index]);
    }
    result[length] = '\0';
    return result;
}

static size_t property_index_w(struct WindowObject* window, LPCWSTR name) {
    if (name == NULL) return window->property_count;
    if (property_name_is_atom(name)) {
        const WORD atom = (WORD)(uintptr_t)name;
        if (atom == 0) return window->property_count;
        size_t index;
        for (index = 0; index < window->property_count; ++index) {
            struct WindowProperty* property = &window->properties[index];
            if (property->atom && property->atom_value == atom) return index;
        }
        return window->property_count;
    }
    char* key = property_name_key_w(name);
    if (key == NULL) return window->property_count;
    size_t result = window->property_count;
    size_t index;
    for (index = 0; index < window->property_count; ++index) {
        struct WindowProperty* property = &window->properties[index];
        if (!property->atom && strcmp(property->name, key) == 0) {
            result = index;
            break;
        }
    }
    free(key);
    return result;
}

static size_t property_index_a(struct WindowObject* window, LPCSTR name) {
    if (name == NULL) return window->property_count;
    if (property_name_is_atom(name)) {
        const WORD atom = (WORD)(uintptr_t)name;
        if (atom == 0) return window->property_count;
        size_t index;
        for (index = 0; index < window->property_count; ++index) {
            struct WindowProperty* property = &window->properties[index];
            if (property->atom && property->atom_value == atom) return index;
        }
        return window->property_count;
    }
    char* key = property_name_key_a(name);
    if (key == NULL) return window->property_count;
    size_t result = window->property_count;
    size_t index;
    for (index = 0; index < window->property_count; ++index) {
        struct WindowProperty* property = &window->properties[index];
        if (!property->atom && strcmp(property->name, key) == 0) {
            result = index;
            break;
        }
    }
    free(key);
    return result;
}

static struct RegisteredClass* find_class(LPCSTR name) {
    if (name == NULL) return NULL;
    if (class_name_is_atom(name)) {
        const ATOM atom = (ATOM)(uintptr_t)name;
        size_t index;
        for (index = 0; index < g_class_count; ++index) {
            if (g_classes[index].atom == atom) return &g_classes[index];
        }
        return NULL;
    }
    size_t index;
    for (index = 0; index < g_class_count; ++index) {
        if (strcmp(g_classes[index].name, name) == 0) return &g_classes[index];
    }
    return NULL;
}

static struct RegisteredClass builtin_class(LPCSTR name) {
    struct RegisteredClass klass;
    memset(&klass, 0, sizeof(klass));
    klass.name = dup_string(name != NULL && !class_name_is_atom(name) ? name : "");
    return klass;
}

static HWND handle_from_window(struct WindowObject* window) {
    return (HWND)window;
}

static BOOL append_window(struct WindowObject* window) {
    if (!reserve_bytes((void**)&g_windows, &g_window_capacity, g_window_count + 1,
                       sizeof(g_windows[0]))) {
        return FALSE;
    }
    g_windows[g_window_count++] = window;
    return TRUE;
}

static HWND* window_snapshot(size_t* count) {
    *count = 0;
    if (g_window_count == 0) return NULL;
    HWND* handles = (HWND*)calloc(g_window_count, sizeof(handles[0]));
    if (handles == NULL) return NULL;
    size_t index;
    for (index = 0; index < g_window_count; ++index) {
        HWND handle = handle_from_window(g_windows[index]);
        if (window_from_handle(handle) != NULL) handles[(*count)++] = handle;
    }
    return handles;
}

static struct WindowObject* first_child(HWND parent) {
    size_t index;
    for (index = 0; index < g_window_count; ++index) {
        struct WindowObject* window = g_windows[index];
        if (window_from_handle(handle_from_window(window)) != NULL &&
            window->parent == parent) {
            return window;
        }
    }
    return NULL;
}

static struct WindowObject* next_sibling(HWND window) {
    struct WindowObject* object = window_from_handle(window);
    if (object == NULL) return NULL;
    BOOL found = FALSE;
    size_t index;
    for (index = 0; index < g_window_count; ++index) {
        struct WindowObject* candidate = g_windows[index];
        if (window_from_handle(handle_from_window(candidate)) == NULL) continue;
        if (found && candidate->parent == object->parent) return candidate;
        if (candidate == object) found = TRUE;
    }
    return NULL;
}

static BOOL class_matches(const struct WindowObject* window, LPCSTR class_name) {
    if (class_name == NULL) return TRUE;
    if (class_name_is_atom(class_name)) {
        return window->klass.atom == (ATOM)(uintptr_t)class_name;
    }
    return strcmp(window->klass.name, class_name) == 0;
}

static BOOL text_matches(const struct WindowObject* window, LPCSTR text) {
    return text == NULL || strcmp(window->text, text) == 0;
}

static size_t menu_item_index(struct MenuObject* menu, UINT item, UINT flags) {
    if ((flags & MF_BYPOSITION) != 0) {
        return item < menu->item_count ? item : menu->item_count;
    }
    size_t index;
    for (index = 0; index < menu->item_count; ++index) {
        struct MenuItem* candidate = &menu->items[index];
        if ((candidate->flags & MF_POPUP) == 0 &&
            candidate->id_or_submenu == item) {
            return index;
        }
    }
    return menu->item_count;
}

static UINT menu_item_flags(UINT flags) {
    return flags & ~(MF_APPEND | MF_CHANGE | MF_DELETE | MF_INSERT | MF_REMOVE |
                     MF_BYPOSITION);
}

static BOOL insert_menu_item(HMENU handle, UINT position, UINT flags,
                             UINT_PTR new_item, LPCSTR text) {
    struct MenuObject* menu = menu_from_handle(handle);
    if (menu == NULL) return FALSE;
    const size_t index = (flags & MF_APPEND) != 0
                             ? menu->item_count
                             : menu_item_index(menu, position, flags);
    if (index > menu->item_count) return FALSE;
    if (!reserve_bytes((void**)&menu->items, &menu->item_capacity,
                       menu->item_count + 1, sizeof(menu->items[0]))) {
        return FALSE;
    }
    memmove(&menu->items[index + 1], &menu->items[index],
            (menu->item_count - index) * sizeof(menu->items[0]));
    struct MenuItem* item = &menu->items[index];
    memset(item, 0, sizeof(*item));
    item->flags = menu_item_flags(flags);
    item->id_or_submenu = new_item;
    if ((item->flags & MF_SEPARATOR) == 0) {
        item->text = dup_string(text != NULL ? text : "");
        if (item->text == NULL) return FALSE;
    }
    ++menu->item_count;
    return TRUE;
}

static BOOL change_menu_item(HMENU handle, UINT position, UINT flags,
                             UINT_PTR new_item, LPCSTR text) {
    struct MenuObject* menu = menu_from_handle(handle);
    if (menu == NULL) return FALSE;
    const size_t index = menu_item_index(menu, position, flags);
    if (index >= menu->item_count) return FALSE;
    struct MenuItem* item = &menu->items[index];
    item->flags = menu_item_flags(flags);
    item->id_or_submenu = new_item;
    item->text = dup_string((item->flags & MF_SEPARATOR) != 0 || text == NULL
                                ? ""
                                : text);
    return item->text != NULL;
}

static BOOL remove_menu_item(HMENU handle, UINT position, UINT flags) {
    struct MenuObject* menu = menu_from_handle(handle);
    if (menu == NULL) return FALSE;
    const size_t index = menu_item_index(menu, position, flags);
    if (index >= menu->item_count) return FALSE;
    memmove(&menu->items[index], &menu->items[index + 1],
            (menu->item_count - index - 1) * sizeof(menu->items[0]));
    --menu->item_count;
    return TRUE;
}

static void destroy_menu_tree(HMENU handle) {
    struct MenuObject* menu = menu_from_handle(handle);
    if (menu == NULL) return;
    size_t index;
    HMENU* submenus = NULL;
    size_t submenu_count = 0;
    if (menu->item_count != 0) {
        submenus = (HMENU*)calloc(menu->item_count, sizeof(submenus[0]));
    }
    for (index = 0; index < menu->item_count; ++index) {
        if ((menu->items[index].flags & MF_POPUP) != 0 && submenus != NULL) {
            submenus[submenu_count++] = (HMENU)menu->items[index].id_or_submenu;
        }
    }
    menu->magic = 0;
    for (index = 0; index < g_window_count; ++index) {
        struct WindowObject* window = g_windows[index];
        if (window_from_handle(handle_from_window(window)) != NULL &&
            window->menu == handle) {
            window->menu = NULL;
        }
        if (window_from_handle(handle_from_window(window)) != NULL &&
            window->system_menu == handle) {
            window->system_menu = NULL;
        }
    }
    for (index = 0; index < submenu_count; ++index) destroy_menu_tree(submenus[index]);
    free(submenus);
}

static BOOL enum_child_windows(HWND parent, WNDENUMPROC enum_func,
                               LPARAM parameter) {
    size_t count = 0;
    HWND* handles = window_snapshot(&count);
    size_t index;
    for (index = 0; index < count; ++index) {
        struct WindowObject* object = window_from_handle(handles[index]);
        if (object == NULL || object->parent != parent) continue;
        if (!enum_func(handles[index], parameter)) {
            free(handles);
            return FALSE;
        }
        if (!enum_child_windows(handles[index], enum_func, parameter)) {
            free(handles);
            return FALSE;
        }
    }
    free(handles);
    return TRUE;
}

static POINT window_screen_origin(HWND window) {
    POINT point = {0, 0};
    struct WindowObject* object = window_from_handle(window);
    while (object != NULL) {
        point.x += object->rectangle.left;
        point.y += object->rectangle.top;
        object = window_from_handle(object->parent);
    }
    return point;
}

static BOOL rect_empty(const RECT* rectangle) {
    return rectangle->right <= rectangle->left ||
           rectangle->bottom <= rectangle->top;
}

static POINT point_from_lparam(LPARAM lparam) {
    POINT point = {(SHORT)LOWORD(lparam), (SHORT)HIWORD(lparam)};
    return point;
}

static BOOL point_in_window(HWND window, POINT point) {
    RECT rectangle;
    return GetWindowRect(window, &rectangle) && PtInRect(&rectangle, point);
}

static BOOL message_matches(const MSG* message, HWND window, UINT filter_min,
                            UINT filter_max) {
    if (window != NULL && message->hwnd != window) return FALSE;
    if (filter_min != 0 || filter_max != 0) {
        if (message->message < filter_min || message->message > filter_max) {
            return FALSE;
        }
    }
    return TRUE;
}

static void clear_timer_queued(const MSG* message) {
    if (message->message != WM_TIMER) return;
    size_t index;
    for (index = 0; index < g_timer_count; ++index) {
        struct TimerObject* timer = &g_timers[index];
        if (timer->window == message->hwnd && timer->id == message->wParam) {
            timer->queued = FALSE;
            return;
        }
    }
}

static BOOL queue_take(LPMSG message, HWND window, UINT filter_min,
                       UINT filter_max, UINT remove_message) {
    if (message == NULL) return FALSE;
    size_t index;
    for (index = 0; index < g_message_count; ++index) {
        if (!message_matches(&g_messages[index], window, filter_min, filter_max)) {
            continue;
        }
        *message = g_messages[index];
        if ((remove_message & PM_REMOVE) != 0) {
            if (g_expected_scripted_char != 0 && message->message == WM_CHAR &&
                message->wParam == g_expected_scripted_char) {
                g_matched_scripted_char = TRUE;
                g_expected_scripted_char = 0;
            }
            clear_timer_queued(&g_messages[index]);
            memmove(&g_messages[index], &g_messages[index + 1],
                    (g_message_count - index - 1) * sizeof(g_messages[0]));
            --g_message_count;
        }
        return TRUE;
    }
    if (g_quit_posted) {
        MSG quit = {NULL, WM_QUIT, (WPARAM)g_quit_code, 0, 0, {0, 0}};
        *message = quit;
        if ((remove_message & PM_REMOVE) != 0) g_quit_posted = FALSE;
        return TRUE;
    }
    return FALSE;
}

static BOOL queue_push(MSG** queue, size_t* count, size_t* capacity,
                       const MSG* message, BOOL front) {
    if (!reserve_bytes((void**)queue, capacity, *count + 1, sizeof((*queue)[0]))) {
        return FALSE;
    }
    if (front) {
        memmove(&(*queue)[1], &(*queue)[0], *count * sizeof((*queue)[0]));
        (*queue)[0] = *message;
    } else {
        (*queue)[*count] = *message;
    }
    ++*count;
    return TRUE;
}

static void queue_front(HWND window, UINT message, WPARAM wparam,
                        LPARAM lparam) {
    MSG item = {window, message, wparam, lparam, 0, {0, 0}};
    queue_push(&g_messages, &g_message_count, &g_message_capacity, &item, TRUE);
}

static void queue_back(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
    MSG item = {window, message, wparam, lparam, 0, {0, 0}};
    queue_push(&g_messages, &g_message_count, &g_message_capacity, &item, FALSE);
}

static void remove_timer_messages(HWND window, UINT_PTR id) {
    size_t index = 0;
    while (index < g_message_count) {
        if (g_messages[index].hwnd == window &&
            g_messages[index].message == WM_TIMER &&
            g_messages[index].wParam == id) {
            memmove(&g_messages[index], &g_messages[index + 1],
                    (g_message_count - index - 1) * sizeof(g_messages[0]));
            --g_message_count;
        } else {
            ++index;
        }
    }
}

static void remove_window_timers(HWND window) {
    size_t index = 0;
    while (index < g_timer_count) {
        if (g_timers[index].window == window) {
            memmove(&g_timers[index], &g_timers[index + 1],
                    (g_timer_count - index - 1) * sizeof(g_timers[0]));
            --g_timer_count;
        } else {
            ++index;
        }
    }
}

static void update_key_state(UINT message, WPARAM wparam) {
    int virtual_key = (int)wparam;
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
    switch (message) {
        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
            g_key_state[virtual_key] = (SHORT)(g_key_state[virtual_key] | 0x8000);
            break;
        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
        case WM_KEYUP:
        case WM_SYSKEYUP:
            g_key_state[virtual_key] = (SHORT)(g_key_state[virtual_key] & ~0x8000);
            break;
    }
}

static BOOL pump_timer_once(void) {
    const ULONGLONG now = GetTickCount64();
    size_t index;
    for (index = 0; index < g_timer_count; ++index) {
        struct TimerObject* timer = &g_timers[index];
        if (timer->queued || timer->due > now) continue;
        queue_back(timer->window, WM_TIMER, timer->id, (LPARAM)timer->callback);
        timer->queued = TRUE;
        timer->due = now + timer->elapsed;
        return TRUE;
    }
    return FALSE;
}

static BOOL next_timer_due(ULONGLONG* due) {
    BOOL found = FALSE;
    size_t index;
    for (index = 0; index < g_timer_count; ++index) {
        struct TimerObject* timer = &g_timers[index];
        if (timer->queued) continue;
        if (!found || timer->due < *due) *due = timer->due;
        found = TRUE;
    }
    return found;
}

static BOOL pump_once(void) {
    if (g_scripted_count == 0) return pump_timer_once();
    MSG message = g_scripted_messages[0];
    memmove(&g_scripted_messages[0], &g_scripted_messages[1],
            (g_scripted_count - 1) * sizeof(g_scripted_messages[0]));
    --g_scripted_count;
    if (message.hwnd == NULL && message.message != WM_QUIT) {
        message.hwnd = g_focus_window != NULL ? g_focus_window : g_active_window;
    }
    update_key_state(message.message, message.wParam);
    queue_push(&g_messages, &g_message_count, &g_message_capacity, &message, FALSE);
    return TRUE;
}

static UINT translated_char(WPARAM virtual_key) {
    const BOOL shift_down = (g_key_state[VK_SHIFT] & 0x8000) != 0;
    if (virtual_key >= 'A' && virtual_key <= 'Z') {
        return (UINT)(shift_down ? virtual_key : virtual_key + 32);
    }
    if (virtual_key >= '0' && virtual_key <= '9') return (UINT)virtual_key;
    switch (virtual_key) {
        case VK_SPACE: return ' ';
        case VK_RETURN: return '\r';
        case VK_TAB: return '\t';
        default: return 0;
    }
}

static void pump_block_until_message(void) {
    if (pump_once()) return;
    ULONGLONG due = 0;
    if (next_timer_due(&due)) {
        const ULONGLONG now = GetTickCount64();
        if (due > now) Sleep((DWORD)(due - now));
        if (pump_once()) return;
    }
    OutputDebugStringA("user32 GetMessage/WaitMessage needs an event backend\n");
    abort();
}

static COLORREF system_color(int index) {
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

static int system_color_brush_slot(int index) {
    return index >= 0 && index < 31 ? index : 31;
}

static HBRUSH stock_brush_for_color(COLORREF color) {
    switch (color & 0x00ffffffu) {
        case RGB(0, 0, 0): return (HBRUSH)GetStockObject(BLACK_BRUSH);
        case RGB(255, 255, 255): return (HBRUSH)GetStockObject(WHITE_BRUSH);
        case RGB(192, 192, 192): return (HBRUSH)GetStockObject(LTGRAY_BRUSH);
        case RGB(128, 128, 128): return (HBRUSH)GetStockObject(GRAY_BRUSH);
        default: return NULL;
    }
}

static struct ClipboardFormat* find_clipboard_format(const char* name) {
    size_t index;
    for (index = 0; index < g_clipboard_format_count; ++index) {
        if (strcmp(g_clipboard_formats[index].name, name) == 0) {
            return &g_clipboard_formats[index];
        }
    }
    return NULL;
}

static struct ClipboardItem* find_clipboard_item(UINT format) {
    size_t index;
    for (index = 0; index < g_clipboard_item_count; ++index) {
        if (g_clipboard_items[index].format == format) {
            return &g_clipboard_items[index];
        }
    }
    return NULL;
}

static LONG_PTR get_window_extra(const struct WindowObject* window, int index,
                                 size_t bytes) {
    if (index < 0 || (size_t)index + bytes > window->extra_size) return 0;
    LONG_PTR value = 0;
    memcpy(&value, window->extra + index, bytes);
    return value;
}

static LONG_PTR set_window_extra(struct WindowObject* window, int index,
                                 LONG_PTR value, size_t bytes) {
    if (index < 0 || (size_t)index + bytes > window->extra_size) return 0;
    const LONG_PTR previous = get_window_extra(window, index, bytes);
    memcpy(window->extra + index, &value, bytes);
    return previous;
}

ATOM RegisterClassExA(const WNDCLASSEXA* window_class) {
    if (window_class == NULL || window_class->lpszClassName == NULL) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }
    if (find_class(window_class->lpszClassName) != NULL) {
        SetLastError(ERROR_CLASS_ALREADY_EXISTS);
        return 0;
    }
    if (!reserve_bytes((void**)&g_classes, &g_class_capacity, g_class_count + 1,
                       sizeof(g_classes[0]))) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return 0;
    }
    struct RegisteredClass* klass = &g_classes[g_class_count++];
    memset(klass, 0, sizeof(*klass));
    klass->atom = g_next_atom++;
    klass->name = dup_string(window_class->lpszClassName);
    klass->procedure = window_class->lpfnWndProc;
    klass->window_extra = max_int(0, window_class->cbWndExtra);
    klass->instance = window_class->hInstance;
    SetLastError(ERROR_SUCCESS);
    return klass->atom;
}

ATOM RegisterClassA(const WNDCLASSA* window_class) {
    if (window_class == NULL) return 0;
    WNDCLASSEXA extended;
    memset(&extended, 0, sizeof(extended));
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
    if (window_class == NULL) return 0;
    char* class_name = narrow_string_dup(window_class->lpszClassName);
    char* menu_name = narrow_string_dup(window_class->lpszMenuName);
    WNDCLASSEXA narrow;
    memset(&narrow, 0, sizeof(narrow));
    narrow.cbSize = sizeof(narrow);
    narrow.style = window_class->style;
    narrow.lpfnWndProc = window_class->lpfnWndProc;
    narrow.cbClsExtra = window_class->cbClsExtra;
    narrow.cbWndExtra = window_class->cbWndExtra;
    narrow.hInstance = window_class->hInstance;
    narrow.hIcon = window_class->hIcon;
    narrow.hCursor = window_class->hCursor;
    narrow.hbrBackground = window_class->hbrBackground;
    narrow.lpszMenuName = menu_name;
    narrow.lpszClassName = class_name;
    ATOM atom = RegisterClassExA(&narrow);
    free(class_name);
    free(menu_name);
    return atom;
}

UINT RegisterClipboardFormatA(LPCSTR name) {
    if (name == NULL || name[0] == '\0') {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }
    struct ClipboardFormat* existing = find_clipboard_format(name);
    if (existing != NULL) return existing->id;
    if (!reserve_bytes((void**)&g_clipboard_formats,
                       &g_clipboard_format_capacity,
                       g_clipboard_format_count + 1,
                       sizeof(g_clipboard_formats[0]))) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return 0;
    }
    char* copy = dup_string(name);
    if (copy == NULL) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return 0;
    }
    struct ClipboardFormat* format =
        &g_clipboard_formats[g_clipboard_format_count++];
    format->id = g_next_clipboard_format++;
    format->name = copy;
    SetLastError(ERROR_SUCCESS);
    return format->id;
}

HWND CreateWindowExA(DWORD extended_style, LPCSTR class_name,
                     LPCSTR window_name, DWORD style, int x, int y, int width,
                     int height, HWND parent, HMENU menu, HINSTANCE instance,
                     LPVOID parameter) {
    struct RegisteredClass klass;
    struct RegisteredClass* registered = find_class(class_name);
    if (registered != NULL) {
        klass = *registered;
    } else {
        klass = builtin_class(class_name);
    }
    struct WindowObject* window = (struct WindowObject*)calloc(1, sizeof(*window));
    if (window == NULL) return NULL;
    window->magic = OPUS_WINDOW_MAGIC;
    window->klass = klass;
    window->text = dup_string(window_name != NULL ? window_name : "");
    window->style = style;
    window->extended_style = extended_style;
    window->rectangle.left = x;
    window->rectangle.top = y;
    window->rectangle.right = x + width;
    window->rectangle.bottom = y + height;
    window->parent = (style & WS_CHILD) != 0 ? parent : NULL;
    window->owner = (style & WS_CHILD) == 0 ? parent : NULL;
    window->menu = menu;
    window->instance = instance;
    window->enabled = TRUE;
    window->visible = (style & WS_VISIBLE) != 0;
    window->scroll_max[SB_HORZ] = 100;
    window->scroll_max[SB_VERT] = 100;
    window->extra_size = (size_t)max_int(0, klass.window_extra);
    if (window->extra_size != 0) {
        window->extra = (unsigned char*)calloc(window->extra_size, 1);
    }
    HWND handle = (HWND)window;
    if (!append_window(window)) return NULL;
    if (g_active_window == NULL) g_active_window = handle;
    if (klass.procedure != NULL) {
        CREATESTRUCTA create;
        memset(&create, 0, sizeof(create));
        create.lpCreateParams = parameter;
        create.hInstance = instance;
        create.hMenu = menu;
        create.hwndParent = parent;
        create.cy = height;
        create.cx = width;
        create.y = y;
        create.x = x;
        create.style = (LONG)style;
        create.lpszName = window_name;
        create.lpszClass = class_name;
        create.dwExStyle = extended_style;
        if (klass.procedure(handle, WM_NCCREATE, 0, (LPARAM)&create) == 0) {
            DestroyWindow(handle);
            return NULL;
        }
        klass.procedure(handle, WM_CREATE, 0, (LPARAM)&create);
    }
    return handle;
}

HWND CreateWindowExW(DWORD extended_style, LPCWSTR class_name,
                     LPCWSTR window_name, DWORD style, int x, int y, int width,
                     int height, HWND parent, HMENU menu, HINSTANCE instance,
                     LPVOID parameter) {
    char* narrow_class = narrow_string_dup(class_name);
    char* narrow_name = narrow_string_dup(window_name);
    HWND result = CreateWindowExA(extended_style, narrow_class, narrow_name, style,
                                  x, y, width, height, parent, menu, instance,
                                  parameter);
    free(narrow_class);
    free(narrow_name);
    return result;
}

BOOL IsWindow(HWND window) {
    return window_from_handle(window) != NULL;
}

BOOL EnumWindows(WNDENUMPROC enum_func, LPARAM parameter) {
    if (enum_func == NULL) return FALSE;
    size_t count = 0;
    HWND* handles = window_snapshot(&count);
    size_t index;
    for (index = 0; index < count; ++index) {
        struct WindowObject* object = window_from_handle(handles[index]);
        if (object == NULL || object->parent != NULL) continue;
        if (!enum_func(handles[index], parameter)) {
            free(handles);
            return FALSE;
        }
    }
    free(handles);
    return TRUE;
}

BOOL EnumChildWindows(HWND parent, WNDENUMPROC enum_func, LPARAM parameter) {
    if (enum_func == NULL) return FALSE;
    if (parent != NULL && !IsWindow(parent)) return FALSE;
    return enum_child_windows(parent, enum_func, parameter);
}

BOOL EnumThreadWindows(DWORD thread_id, WNDENUMPROC enum_func, LPARAM parameter) {
    (void)thread_id;
    return EnumWindows(enum_func, parameter);
}

BOOL DestroyWindow(HWND window) {
    struct WindowObject* object = window_from_handle(window);
    if (object == NULL) return FALSE;
    struct WindowObject* child;
    while ((child = first_child(window)) != NULL) {
        DestroyWindow(handle_from_window(child));
    }
    if (object->klass.procedure != NULL) {
        object->klass.procedure(window, WM_DESTROY, 0, 0);
        object->klass.procedure(window, WM_NCDESTROY, 0, 0);
    }
    if (g_active_window == window) g_active_window = NULL;
    if (g_focus_window == window) g_focus_window = NULL;
    if (g_capture_window == window) g_capture_window = NULL;
    if (object->system_menu != NULL) destroy_menu_tree(object->system_menu);
    remove_window_timers(window);
    size_t index = 0;
    while (index < g_message_count) {
        if (g_messages[index].hwnd == window) {
            memmove(&g_messages[index], &g_messages[index + 1],
                    (g_message_count - index - 1) * sizeof(g_messages[0]));
            --g_message_count;
        } else {
            ++index;
        }
    }
    object->magic = 0;
    return TRUE;
}

static HMENU create_menu_object(BOOL popup) {
    struct MenuObject* menu = (struct MenuObject*)calloc(1, sizeof(*menu));
    if (menu == NULL) return NULL;
    menu->magic = OPUS_MENU_MAGIC;
    menu->popup = popup;
    return (HMENU)menu;
}

HMENU CreateMenu(void) {
    return create_menu_object(FALSE);
}

HMENU CreatePopupMenu(void) {
    return create_menu_object(TRUE);
}

BOOL IsMenu(HMENU menu) {
    return menu_from_handle(menu) != NULL;
}

BOOL DestroyMenu(HMENU menu) {
    if (!IsMenu(menu)) return FALSE;
    destroy_menu_tree(menu);
    return TRUE;
}

static HMENU create_system_menu(void) {
    HMENU menu = CreatePopupMenu();
    if (menu == NULL) return NULL;
    if (!AppendMenuA(menu, MF_STRING, SC_RESTORE, "Restore") ||
        !AppendMenuA(menu, MF_STRING, SC_MOVE, "Move") ||
        !AppendMenuA(menu, MF_STRING, SC_SIZE, "Size") ||
        !AppendMenuA(menu, MF_STRING, SC_MINIMIZE, "Minimize") ||
        !AppendMenuA(menu, MF_STRING, SC_MAXIMIZE, "Maximize") ||
        !AppendMenuA(menu, MF_SEPARATOR, 0, NULL) ||
        !AppendMenuA(menu, MF_STRING, SC_CLOSE, "Close")) {
        DestroyMenu(menu);
        return NULL;
    }
    return menu;
}

BOOL AppendMenuA(HMENU menu, UINT flags, UINT_PTR new_item, LPCSTR new_item_text) {
    return insert_menu_item(menu, 0, flags | MF_APPEND, new_item, new_item_text);
}

BOOL AppendMenuW(HMENU menu, UINT flags, UINT_PTR new_item, LPCWSTR new_item_text) {
    char* text = narrow_string_dup(new_item_text);
    BOOL result = insert_menu_item(menu, 0, flags | MF_APPEND, new_item, text);
    free(text);
    return result;
}

BOOL InsertMenuA(HMENU menu, UINT position, UINT flags, UINT_PTR new_item,
                 LPCSTR new_item_text) {
    return insert_menu_item(menu, position, flags, new_item, new_item_text);
}

BOOL InsertMenuW(HMENU menu, UINT position, UINT flags, UINT_PTR new_item,
                 LPCWSTR new_item_text) {
    char* text = narrow_string_dup(new_item_text);
    BOOL result = insert_menu_item(menu, position, flags, new_item, text);
    free(text);
    return result;
}

BOOL ModifyMenuA(HMENU menu, UINT position, UINT flags, UINT_PTR new_item,
                 LPCSTR new_item_text) {
    return change_menu_item(menu, position, flags, new_item, new_item_text);
}

BOOL ModifyMenuW(HMENU menu, UINT position, UINT flags, UINT_PTR new_item,
                 LPCWSTR new_item_text) {
    char* text = narrow_string_dup(new_item_text);
    BOOL result = change_menu_item(menu, position, flags, new_item, text);
    free(text);
    return result;
}

BOOL DeleteMenu(HMENU menu, UINT position, UINT flags) {
    return remove_menu_item(menu, position, flags);
}

BOOL RemoveMenu(HMENU menu, UINT position, UINT flags) {
    return remove_menu_item(menu, position, flags);
}

int GetMenuItemCount(HMENU handle) {
    struct MenuObject* menu = menu_from_handle(handle);
    return menu != NULL ? (int)menu->item_count : -1;
}

UINT GetMenuItemID(HMENU handle, int position) {
    struct MenuObject* menu = menu_from_handle(handle);
    if (menu == NULL || position < 0 || (size_t)position >= menu->item_count) {
        return (UINT)-1;
    }
    struct MenuItem* item = &menu->items[position];
    return (item->flags & MF_POPUP) != 0 ? (UINT)-1 : (UINT)item->id_or_submenu;
}

HMENU GetSubMenu(HMENU handle, int position) {
    struct MenuObject* menu = menu_from_handle(handle);
    if (menu == NULL || position < 0 || (size_t)position >= menu->item_count) {
        return NULL;
    }
    struct MenuItem* item = &menu->items[position];
    return (item->flags & MF_POPUP) != 0 ? (HMENU)item->id_or_submenu : NULL;
}

UINT GetMenuState(HMENU handle, UINT id, UINT flags) {
    struct MenuObject* menu = menu_from_handle(handle);
    if (menu == NULL) return (UINT)-1;
    const size_t index = menu_item_index(menu, id, flags);
    if (index >= menu->item_count) return (UINT)-1;
    UINT state = menu->items[index].flags;
    if ((state & MF_POPUP) != 0) {
        struct MenuObject* submenu = menu_from_handle((HMENU)menu->items[index].id_or_submenu);
        if (submenu != NULL) state |= (UINT)(submenu->item_count << 8u);
    }
    return state;
}

int GetMenuStringW(HMENU handle, UINT item_id, LPWSTR string, int max_count,
                   UINT flags) {
    struct MenuObject* menu = menu_from_handle(handle);
    if (menu == NULL || string == NULL || max_count <= 0) return 0;
    const size_t index = menu_item_index(menu, item_id, flags);
    if (index >= menu->item_count) {
        string[0] = 0;
        return 0;
    }
    const char* text =
        menu->items[index].text != NULL ? menu->items[index].text : "";
    const size_t limit = (size_t)max_count;
    const size_t count = min_int((int)strlen(text), (int)limit - 1);
    size_t i;
    for (i = 0; i < count; ++i) string[i] = (WCHAR)(unsigned char)text[i];
    string[count] = 0;
    return (int)count;
}

int GetMenuStringA(HMENU handle, UINT item_id, LPSTR string, int max_count,
                   UINT flags) {
    struct MenuObject* menu = menu_from_handle(handle);
    if (menu == NULL || string == NULL || max_count <= 0) return 0;
    const size_t index = menu_item_index(menu, item_id, flags);
    if (index >= menu->item_count) {
        string[0] = '\0';
        return 0;
    }
    const char* text =
        menu->items[index].text != NULL ? menu->items[index].text : "";
    const size_t count = (size_t)min_int((int)strlen(text), max_count - 1);
    memcpy(string, text, count);
    string[count] = '\0';
    return (int)count;
}

DWORD CheckMenuItem(HMENU handle, UINT item_id, UINT check) {
    struct MenuObject* menu = menu_from_handle(handle);
    if (menu == NULL) return (DWORD)-1;
    const size_t index = menu_item_index(menu, item_id, check);
    if (index >= menu->item_count) return (DWORD)-1;
    const DWORD previous = (menu->items[index].flags & MF_CHECKED) != 0
                               ? MF_CHECKED
                               : MF_UNCHECKED;
    menu->items[index].flags &= ~MF_CHECKED;
    menu->items[index].flags |= check & MF_CHECKED;
    return previous;
}

BOOL CheckMenuRadioItem(HMENU handle, UINT first, UINT last, UINT check,
                        UINT flags) {
    struct MenuObject* menu = menu_from_handle(handle);
    if (menu == NULL) return FALSE;
    BOOL found = FALSE;
    size_t index;
    for (index = 0; index < menu->item_count; ++index) {
        UINT id = (flags & MF_BYPOSITION) != 0 ? (UINT)index
                                               : (UINT)menu->items[index].id_or_submenu;
        if (id < first || id > last) continue;
        menu->items[index].flags &= ~MF_CHECKED;
        if (id == check) {
            menu->items[index].flags |= MF_CHECKED;
            found = TRUE;
        }
    }
    return found;
}

BOOL SetMenuInfo(HMENU menu, const MENUINFO* menu_info) {
    (void)menu_info;
    return IsMenu(menu);
}

BOOL SetMenuItemBitmaps(HMENU handle, UINT position, UINT flags,
                        HBITMAP unchecked_bitmap, HBITMAP checked_bitmap) {
    (void)unchecked_bitmap;
    (void)checked_bitmap;
    struct MenuObject* menu = menu_from_handle(handle);
    return menu != NULL && menu_item_index(menu, position, flags) < menu->item_count;
}

HDC GetDC(HWND window) {
    if (window != NULL && !IsWindow(window)) return NULL;
    return CreateCompatibleDC(NULL);
}

int ReleaseDC(HWND window, HDC device_context) {
    (void)window;
    return DeleteDC(device_context) ? 1 : 0;
}

HDC BeginPaint(HWND window, LPPAINTSTRUCT paint) {
    if (!IsWindow(window) || paint == NULL) return NULL;
    memset(paint, 0, sizeof(*paint));
    paint->hdc = GetDC(window);
    paint->fErase = TRUE;
    GetClientRect(window, &paint->rcPaint);
    return paint->hdc;
}

VOID EndPaint(HWND window, LPPAINTSTRUCT paint) {
    if (paint == NULL || paint->hdc == NULL) return;
    ReleaseDC(window, paint->hdc);
    paint->hdc = NULL;
}

BOOL InvalidateRect(HWND window, const RECT* rectangle, BOOL erase) {
    (void)rectangle;
    (void)erase;
    if (!IsWindow(window)) return FALSE;
    return PostMessageA(window, WM_PAINT, 0, 0);
}

BOOL RedrawWindow(HWND window, const RECT* update_rect, HRGN update_region,
                  UINT flags) {
    (void)update_region;
    if (!IsWindow(window)) return FALSE;
    const BOOL update_now = (flags & RDW_UPDATENOW) != 0;
    if ((flags & RDW_INVALIDATE) != 0 && !update_now &&
        !InvalidateRect(window, update_rect, (flags & RDW_ERASE) != 0)) {
        return FALSE;
    }
    if (update_now) {
        SendMessageA(window, WM_PAINT, 0, 0);
    }
    if ((flags & RDW_ALLCHILDREN) != 0) {
        size_t index;
        for (index = 0; index < g_window_count; ++index) {
            if (g_windows[index] != NULL && g_windows[index]->parent == window) {
                HWND child = (HWND)g_windows[index];
                if ((flags & RDW_INVALIDATE) != 0 && !update_now) {
                    InvalidateRect(child, update_rect, (flags & RDW_ERASE) != 0);
                }
                if (update_now) {
                    SendMessageA(child, WM_PAINT, 0, 0);
                }
            }
        }
    }
    return TRUE;
}

int GetSystemMetrics(int index) {
    switch (index) {
        case SM_CXSCREEN: return 640;
        case SM_CYSCREEN: return 480;
        case SM_CXBORDER:
        case SM_CYBORDER: return 1;
        case SM_CXVSCROLL:
        case SM_CYHSCROLL: return 16;
        case SM_CXFRAME:
        case SM_CYFRAME: return 4;
        case SM_CYCAPTION: return 18;
        case SM_CYMENU: return 20;
        case SM_CURSORLEVEL: return g_cursor_show_count;
        default: return 0;
    }
}

DWORD GetSysColor(int index) {
    return system_color(index);
}

HBRUSH GetSysColorBrush(int index) {
    const COLORREF color = system_color(index);
    HBRUSH stock = stock_brush_for_color(color);
    if (stock != NULL) return stock;
    const int slot = system_color_brush_slot(index);
    if (g_system_color_brushes[slot] == NULL) {
        g_system_color_brushes[slot] = CreateSolidBrush(color);
    }
    return g_system_color_brushes[slot];
}

HWND GetClipboardOwner(void) {
    return g_clipboard_owner;
}

BOOL OpenClipboard(HWND window) {
    if (g_clipboard_open) {
        SetLastError(ERROR_ACCESS_DENIED);
        return FALSE;
    }
    g_clipboard_open = TRUE;
    g_clipboard_owner = window;
    SetLastError(ERROR_SUCCESS);
    return TRUE;
}

BOOL EmptyClipboard(void) {
    if (!g_clipboard_open) {
        SetLastError(ERROR_ACCESS_DENIED);
        return FALSE;
    }
    g_clipboard_item_count = 0;
    SetLastError(ERROR_SUCCESS);
    return TRUE;
}

HANDLE GetClipboardData(UINT format) {
    struct ClipboardItem* item = find_clipboard_item(format);
    return item != NULL ? item->data : NULL;
}

HANDLE SetClipboardData(UINT format, HANDLE memory) {
    if (!g_clipboard_open) {
        SetLastError(ERROR_ACCESS_DENIED);
        return NULL;
    }
    struct ClipboardItem* item = find_clipboard_item(format);
    if (item == NULL) {
        if (!reserve_bytes((void**)&g_clipboard_items,
                           &g_clipboard_item_capacity,
                           g_clipboard_item_count + 1,
                           sizeof(g_clipboard_items[0]))) {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return NULL;
        }
        item = &g_clipboard_items[g_clipboard_item_count++];
        item->format = format;
    }
    item->data = memory;
    SetLastError(ERROR_SUCCESS);
    return memory;
}

BOOL IsClipboardFormatAvailable(UINT format) {
    return find_clipboard_item(format) != NULL;
}

BOOL CloseClipboard(void) {
    if (!g_clipboard_open) {
        SetLastError(ERROR_ACCESS_DENIED);
        return FALSE;
    }
    g_clipboard_open = FALSE;
    SetLastError(ERROR_SUCCESS);
    return TRUE;
}

LONG_PTR GetWindowLongPtrA(HWND window, int index) {
    struct WindowObject* object = window_from_handle(window);
    if (object == NULL) return 0;
    switch (index) {
        case GWL_STYLE: return object->style;
        case GWL_EXSTYLE: return object->extended_style;
        case GWLP_USERDATA: return object->user_data;
        default: return get_window_extra(object, index, sizeof(LONG_PTR));
    }
}

LONG_PTR GetWindowLongPtrW(HWND window, int index) {
    return GetWindowLongPtrA(window, index);
}

LONG_PTR SetWindowLongPtrA(HWND window, int index, LONG_PTR new_long) {
    struct WindowObject* object = window_from_handle(window);
    if (object == NULL) return 0;
    LONG_PTR previous = 0;
    switch (index) {
        case GWL_STYLE:
            previous = object->style;
            object->style = (DWORD)new_long;
            return previous;
        case GWL_EXSTYLE:
            previous = object->extended_style;
            object->extended_style = (DWORD)new_long;
            return previous;
        case GWLP_USERDATA:
            previous = object->user_data;
            object->user_data = new_long;
            return previous;
        default:
            return set_window_extra(object, index, new_long, sizeof(LONG_PTR));
    }
}

LONG_PTR SetWindowLongPtrW(HWND window, int index, LONG_PTR new_long) {
    return SetWindowLongPtrA(window, index, new_long);
}

LONG GetWindowLongA(HWND window, int index) {
    struct WindowObject* object = window_from_handle(window);
    if (object == NULL) return 0;
    switch (index) {
        case GWL_STYLE: return (LONG)object->style;
        case GWL_EXSTYLE: return (LONG)object->extended_style;
        default: return (LONG)get_window_extra(object, index, sizeof(LONG));
    }
}

LONG SetWindowLongA(HWND window, int index, LONG new_long) {
    struct WindowObject* object = window_from_handle(window);
    if (object == NULL) return 0;
    switch (index) {
        case GWL_STYLE: {
            const LONG previous = (LONG)object->style;
            object->style = (DWORD)new_long;
            return previous;
        }
        case GWL_EXSTYLE: {
            const LONG previous = (LONG)object->extended_style;
            object->extended_style = (DWORD)new_long;
            return previous;
        }
        default:
            return (LONG)set_window_extra(object, index, new_long, sizeof(LONG));
    }
}

WORD GetWindowWord(HWND window, int index) {
    struct WindowObject* object = window_from_handle(window);
    return object != NULL ? (WORD)get_window_extra(object, index, sizeof(WORD)) : 0;
}

WORD SetWindowWord(HWND window, int index, WORD new_word) {
    struct WindowObject* object = window_from_handle(window);
    return object != NULL ? (WORD)set_window_extra(object, index, new_word, sizeof(WORD)) : 0;
}

HANDLE GetPropA(HWND window, LPCSTR string) {
    struct WindowObject* object = window_from_handle(window);
    if (object == NULL || string == NULL) return NULL;
    const size_t index = property_index_a(object, string);
    return index < object->property_count ? object->properties[index].data : NULL;
}

HANDLE GetPropW(HWND window, LPCWSTR string) {
    struct WindowObject* object = window_from_handle(window);
    if (object == NULL || string == NULL) return NULL;
    const size_t index = property_index_w(object, string);
    return index < object->property_count ? object->properties[index].data : NULL;
}

BOOL SetPropW(HWND window, LPCWSTR string, HANDLE data) {
    struct WindowObject* object = window_from_handle(window);
    if (object == NULL || string == NULL) return FALSE;
    size_t index = property_index_w(object, string);
    if (index < object->property_count) {
        object->properties[index].data = data;
        return TRUE;
    }
    if (!reserve_bytes((void**)&object->properties, &object->property_capacity,
                       object->property_count + 1, sizeof(object->properties[0]))) {
        return FALSE;
    }
    struct WindowProperty* property = &object->properties[object->property_count++];
    memset(property, 0, sizeof(*property));
    property->data = data;
    if (property_name_is_atom(string)) {
        property->atom = TRUE;
        property->atom_value = (WORD)(uintptr_t)string;
    } else {
        property->name = property_name_key_w(string);
    }
    return TRUE;
}

HANDLE RemovePropW(HWND window, LPCWSTR string) {
    struct WindowObject* object = window_from_handle(window);
    if (object == NULL || string == NULL) return NULL;
    const size_t index = property_index_w(object, string);
    if (index >= object->property_count) return NULL;
    const HANDLE data = object->properties[index].data;
    memmove(&object->properties[index], &object->properties[index + 1],
            (object->property_count - index - 1) * sizeof(object->properties[0]));
    --object->property_count;
    return data;
}

int lstrcmpW(LPCWSTR first, LPCWSTR second) {
    return compare_wide_strings(first, second, FALSE);
}

int lstrcmpiW(LPCWSTR first, LPCWSTR second) {
    return compare_wide_strings(first, second, TRUE);
}

LRESULT DefWindowProcA(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
    struct WindowObject* object = window_from_handle(window);
    switch (message) {
        case WM_NCCREATE: return 1;
        case WM_CLOSE: return DestroyWindow(window);
        case WM_NCHITTEST:
            return object != NULL && point_in_window(window, point_from_lparam(lparam))
                       ? HTCLIENT
                       : HTNOWHERE;
        case WM_SETTEXT:
            if (object == NULL) return FALSE;
            object->text = dup_string(lparam != 0 ? (LPCSTR)lparam : "");
            return TRUE;
        case WM_GETTEXT:
            if (object == NULL || lparam == 0 || wparam == 0) return 0;
            lstrcpynA((LPSTR)lparam, object->text, (int)wparam);
            return (LRESULT)strlen((LPCSTR)lparam);
        case WM_GETTEXTLENGTH:
            return object != NULL ? (LRESULT)strlen(object->text) : 0;
        default:
            return 0;
    }
}

LRESULT DefWindowProcW(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
    struct WindowObject* object = window_from_handle(window);
    switch (message) {
        case WM_NCCREATE: return 1;
        case WM_CLOSE: return DestroyWindow(window);
        case WM_NCHITTEST:
            return object != NULL && point_in_window(window, point_from_lparam(lparam))
                       ? HTCLIENT
                       : HTNOWHERE;
        case WM_SETTEXT:
            if (object == NULL) return FALSE;
            object->text = narrow_string_dup(lparam != 0 ? (LPCWSTR)lparam : NULL);
            return TRUE;
        case WM_GETTEXT:
            if (object == NULL || lparam == 0 || wparam == 0) return 0;
            return GetWindowTextW(window, (LPWSTR)lparam, (int)wparam);
        case WM_GETTEXTLENGTH:
            return object != NULL ? (LRESULT)strlen(object->text) : 0;
        default:
            return 0;
    }
}

LRESULT CallWindowProcW(WNDPROC previous, HWND window, UINT message,
                        WPARAM wparam, LPARAM lparam) {
    return previous != NULL ? previous(window, message, wparam, lparam) : 0;
}

HCURSOR LoadCursorA(HINSTANCE instance, LPCSTR cursor_name) {
    (void)instance;
    return (HCURSOR)cursor_name;
}

HCURSOR LoadCursorW(HINSTANCE instance, LPCWSTR cursor_name) {
    (void)instance;
    return (HCURSOR)cursor_name;
}

HCURSOR CreateCursor(HINSTANCE instance, int hot_spot_x, int hot_spot_y,
                     int width, int height, LPCVOID and_plane,
                     LPCVOID xor_plane) {
    (void)instance;
    (void)hot_spot_x;
    (void)hot_spot_y;
    (void)width;
    (void)height;
    (void)xor_plane;
    return and_plane != NULL ? (HCURSOR)and_plane : (HCURSOR)(uintptr_t)1;
}

HICON CreateIcon(HINSTANCE instance, int width, int height, BYTE planes,
                 BYTE bits_pixel, LPCVOID and_bits, LPCVOID xor_bits) {
    (void)instance;
    (void)width;
    (void)height;
    (void)planes;
    (void)bits_pixel;
    (void)xor_bits;
    return and_bits != NULL ? (HICON)and_bits : (HICON)(uintptr_t)1;
}

HHOOK SetWindowsHook(int hook, FARPROC procedure) {
    (void)hook;
    (void)procedure;
    return NULL;
}

BOOL UnhookWindowsHook(int hook, FARPROC procedure) {
    (void)hook;
    (void)procedure;
    return TRUE;
}

LRESULT DefHookProc(int code, WPARAM wparam, LPARAM lparam, HHOOK* hook) {
    (void)code;
    (void)wparam;
    (void)lparam;
    (void)hook;
    return 0;
}

HWND GetActiveWindow(void) {
    return g_active_window;
}

BOOL IsChild(HWND parent, HWND window) {
    struct WindowObject* object = window_from_handle(window);
    while (object != NULL) {
        if (object->parent == parent) return TRUE;
        object = window_from_handle(object->parent);
    }
    return FALSE;
}

HWND GetParent(HWND window) {
    struct WindowObject* object = window_from_handle(window);
    return object != NULL ? object->parent : NULL;
}

HWND GetWindow(HWND window, UINT command) {
    struct WindowObject* object = window_from_handle(window);
    if (object == NULL) return NULL;
    switch (command) {
        case GW_OWNER: return object->owner;
        case GW_CHILD: return handle_from_window(first_child(window));
        case GW_HWNDNEXT: return handle_from_window(next_sibling(window));
        default: return NULL;
    }
}

HMENU GetMenu(HWND window) {
    struct WindowObject* object = window_from_handle(window);
    return object != NULL ? object->menu : NULL;
}

HMENU GetSystemMenu(HWND window, BOOL revert) {
    struct WindowObject* object = window_from_handle(window);
    if (object == NULL) return NULL;
    if (revert) {
        if (object->system_menu != NULL) {
            destroy_menu_tree(object->system_menu);
            object->system_menu = NULL;
        }
        return NULL;
    }
    if (object->system_menu == NULL) {
        object->system_menu = create_system_menu();
    }
    return object->system_menu;
}

BOOL SetMenu(HWND window, HMENU menu) {
    struct WindowObject* object = window_from_handle(window);
    if (object == NULL || (menu != NULL && !IsMenu(menu))) return FALSE;
    object->menu = menu;
    return TRUE;
}

HWND GetTopWindow(HWND window) {
    return handle_from_window(first_child(window));
}

BOOL GetWindowRect(HWND window, LPRECT rectangle) {
    struct WindowObject* object = window_from_handle(window);
    if (object == NULL || rectangle == NULL) return FALSE;
    const POINT origin = window_screen_origin(object->parent);
    *rectangle = object->rectangle;
    rectangle->left += origin.x;
    rectangle->right += origin.x;
    rectangle->top += origin.y;
    rectangle->bottom += origin.y;
    return TRUE;
}

BOOL GetClientRect(HWND window, LPRECT rectangle) {
    struct WindowObject* object = window_from_handle(window);
    if (object == NULL || rectangle == NULL) return FALSE;
    rectangle->left = 0;
    rectangle->top = 0;
    rectangle->right = object->rectangle.right - object->rectangle.left;
    rectangle->bottom = object->rectangle.bottom - object->rectangle.top;
    return TRUE;
}

BOOL ClientToScreen(HWND window, LPPOINT point) {
    if (!IsWindow(window) || point == NULL) return FALSE;
    const POINT origin = window_screen_origin(window);
    point->x += origin.x;
    point->y += origin.y;
    return TRUE;
}

BOOL ScreenToClient(HWND window, LPPOINT point) {
    if (!IsWindow(window) || point == NULL) return FALSE;
    const POINT origin = window_screen_origin(window);
    point->x -= origin.x;
    point->y -= origin.y;
    return TRUE;
}

BOOL SetRect(LPRECT rectangle, int left, int top, int right, int bottom) {
    if (rectangle == NULL) return FALSE;
    rectangle->left = left;
    rectangle->top = top;
    rectangle->right = right;
    rectangle->bottom = bottom;
    return TRUE;
}

BOOL InflateRect(LPRECT rectangle, int dx, int dy) {
    if (rectangle == NULL) return FALSE;
    rectangle->left -= dx;
    rectangle->right += dx;
    rectangle->top -= dy;
    rectangle->bottom += dy;
    return TRUE;
}

BOOL IntersectRect(LPRECT destination, const RECT* source1, const RECT* source2) {
    if (destination == NULL || source1 == NULL || source2 == NULL) return FALSE;
    RECT result = {max_int(source1->left, source2->left),
                   max_int(source1->top, source2->top),
                   min_int(source1->right, source2->right),
                   min_int(source1->bottom, source2->bottom)};
    if (rect_empty(&result)) {
        SetRect(destination, 0, 0, 0, 0);
        return FALSE;
    }
    *destination = result;
    return TRUE;
}

BOOL OffsetRect(LPRECT rectangle, int dx, int dy) {
    if (rectangle == NULL) return FALSE;
    rectangle->left += dx;
    rectangle->right += dx;
    rectangle->top += dy;
    rectangle->bottom += dy;
    return TRUE;
}

BOOL UnionRect(LPRECT destination, const RECT* source1, const RECT* source2) {
    if (destination == NULL || source1 == NULL || source2 == NULL) return FALSE;
    const BOOL empty1 = rect_empty(source1);
    const BOOL empty2 = rect_empty(source2);
    if (empty1 && empty2) {
        SetRect(destination, 0, 0, 0, 0);
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
    SetRect(destination, min_int(source1->left, source2->left),
            min_int(source1->top, source2->top),
            max_int(source1->right, source2->right),
            max_int(source1->bottom, source2->bottom));
    return TRUE;
}

BOOL PtInRect(const RECT* rectangle, POINT point) {
    return rectangle != NULL && point.x >= rectangle->left &&
           point.x < rectangle->right && point.y >= rectangle->top &&
           point.y < rectangle->bottom;
}

BOOL AdjustWindowRectEx(LPRECT rectangle, DWORD style, BOOL menu,
                        DWORD extended_style) {
    if (rectangle == NULL) return FALSE;
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

BOOL AdjustWindowRect(LPRECT rectangle, DWORD style, BOOL menu) {
    return AdjustWindowRectEx(rectangle, style, menu, 0);
}

BOOL SystemParametersInfoA(UINT action, UINT parameter, LPVOID data, UINT flags) {
    (void)parameter;
    (void)flags;
    if (action == SPI_GETWORKAREA && data != NULL) {
        RECT* rectangle = (RECT*)data;
        SetRect(rectangle, 0, 0, GetSystemMetrics(SM_CXSCREEN),
                GetSystemMetrics(SM_CYSCREEN));
        return TRUE;
    }
    return FALSE;
}

BOOL ShowWindow(HWND window, int command_show) {
    struct WindowObject* object = window_from_handle(window);
    if (object == NULL) return FALSE;
    const BOOL was_visible = object->visible ? TRUE : FALSE;
    object->visible = command_show != SW_HIDE;
    if (object->visible) {
        object->style |= WS_VISIBLE;
    } else {
        object->style &= ~(DWORD)WS_VISIBLE;
    }
    return was_visible;
}

BOOL EnableWindow(HWND window, BOOL enable) {
    struct WindowObject* object = window_from_handle(window);
    if (object == NULL) return FALSE;
    const BOOL was_enabled = object->enabled ? TRUE : FALSE;
    object->enabled = enable != FALSE;
    return was_enabled;
}

BOOL IsWindowEnabled(HWND window) {
    struct WindowObject* object = window_from_handle(window);
    return object != NULL && object->enabled;
}

BOOL IsWindowVisible(HWND window) {
    struct WindowObject* object = window_from_handle(window);
    while (object != NULL) {
        if (!object->visible) return FALSE;
        object = window_from_handle(object->parent);
    }
    return window_from_handle(window) != NULL;
}

HWND GetAncestor(HWND window, UINT flags) {
    struct WindowObject* object = window_from_handle(window);
    if (object == NULL) return NULL;
    if (flags == GA_PARENT) return object->parent;
    HWND current = window;
    struct WindowObject* current_object;
    while ((current_object = window_from_handle(current)) != NULL) {
        if (current_object->parent == NULL) break;
        current = current_object->parent;
    }
    if (flags == GA_ROOT) return current;
    if (flags == GA_ROOTOWNER) {
        while ((current_object = window_from_handle(current)) != NULL) {
            if (current_object->owner == NULL) break;
            current = current_object->owner;
        }
        return current;
    }
    return NULL;
}

HWND SetActiveWindow(HWND window) {
    if (window != NULL && !IsWindow(window)) return NULL;
    const HWND previous = g_active_window;
    g_active_window = window;
    return previous;
}

HWND SetFocus(HWND window) {
    if (window != NULL && !IsWindow(window)) return NULL;
    const HWND previous = g_focus_window;
    g_focus_window = window;
    return previous;
}

HWND GetFocus(void) {
    return g_focus_window;
}

BOOL BringWindowToTop(HWND window) {
    struct WindowObject* object = window_from_handle(window);
    if (object == NULL) return FALSE;
    size_t index;
    for (index = 0; index < g_window_count; ++index) {
        if (g_windows[index] == object) {
            memmove(&g_windows[index], &g_windows[index + 1],
                    (g_window_count - index - 1) * sizeof(g_windows[0]));
            g_windows[g_window_count - 1] = object;
            break;
        }
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
    if (window != NULL && !IsWindow(window)) return NULL;
    const HWND previous = g_capture_window;
    g_capture_window = window;
    return previous;
}

HWND GetCapture(void) {
    return g_capture_window;
}

BOOL ReleaseCapture(void) {
    g_capture_window = NULL;
    return TRUE;
}

BOOL SetCursorPos(int x, int y) {
    g_cursor_position.x = x;
    g_cursor_position.y = y;
    return TRUE;
}

BOOL GetCursorPos(LPPOINT point) {
    if (point == NULL) return FALSE;
    *point = g_cursor_position;
    return TRUE;
}

LONG GetMessageTime(void) { return (LONG)GetTickCount(); }

UINT GetDoubleClickTime(void) { return 500; }

HWND WindowFromPoint(POINT point) {
    size_t index = g_window_count;
    while (index > 0) {
        --index;
        HWND window = handle_from_window(g_windows[index]);
        if (!IsWindow(window) || !IsWindowVisible(window) ||
            !IsWindowEnabled(window) || !point_in_window(window, point)) {
            continue;
        }
        const LRESULT hit =
            SendMessageA(window, WM_NCHITTEST, 0, MAKELPARAM(point.x, point.y));
        if (hit != HTNOWHERE && hit != HTTRANSPARENT) return window;
    }
    return NULL;
}

HWND FindWindowA(LPCSTR class_name, LPCSTR window_name) {
    size_t count = 0;
    HWND* handles = window_snapshot(&count);
    HWND result = NULL;
    size_t index;
    for (index = 0; index < count; ++index) {
        struct WindowObject* object = window_from_handle(handles[index]);
        if (object == NULL || object->parent != NULL) continue;
        if (class_matches(object, class_name) && text_matches(object, window_name)) {
            result = handles[index];
            break;
        }
    }
    free(handles);
    return result;
}

HWND FindWindowExW(HWND parent, HWND child_after, LPCWSTR class_name,
                   LPCWSTR window_name) {
    char* narrow_class = narrow_string_dup(class_name);
    char* narrow_text = narrow_string_dup(window_name);
    LPCSTR class_filter = class_name != NULL ? narrow_class : NULL;
    LPCSTR text_filter = window_name != NULL ? narrow_text : NULL;
    BOOL after_seen = child_after == NULL;
    size_t count = 0;
    HWND* handles = window_snapshot(&count);
    HWND result = NULL;
    size_t index;
    for (index = 0; index < count; ++index) {
        struct WindowObject* object = window_from_handle(handles[index]);
        if (object == NULL || object->parent != parent) continue;
        if (!after_seen) {
            if (handles[index] == child_after) after_seen = TRUE;
            continue;
        }
        if (class_matches(object, class_filter) && text_matches(object, text_filter)) {
            result = handles[index];
            break;
        }
    }
    free(handles);
    free(narrow_class);
    free(narrow_text);
    return result;
}

int GetClassNameW(HWND window, LPWSTR class_name, int max_count) {
    struct WindowObject* object = window_from_handle(window);
    if (object == NULL || class_name == NULL || max_count <= 0) return 0;
    const size_t limit = (size_t)max_count;
    const size_t count = min_int((int)strlen(object->klass.name), (int)limit - 1);
    size_t index;
    for (index = 0; index < count; ++index) {
        class_name[index] = (WCHAR)(unsigned char)object->klass.name[index];
    }
    class_name[count] = 0;
    return (int)count;
}

int GetDlgCtrlID(HWND control) {
    struct WindowObject* object = window_from_handle(control);
    return object != NULL ? (int)(INT_PTR)object->menu : 0;
}

BOOL IsIconic(HWND window) {
    struct WindowObject* object = window_from_handle(window);
    return object != NULL && (object->style & WS_MINIMIZE) != 0;
}

BOOL IsZoomed(HWND window) {
    struct WindowObject* object = window_from_handle(window);
    return object != NULL && (object->style & WS_MAXIMIZE) != 0;
}

BOOL OpenIcon(HWND window) {
    struct WindowObject* object = window_from_handle(window);
    if (object == NULL) return FALSE;
    object->style &= ~(DWORD)WS_MINIMIZE;
    ShowWindow(window, SW_SHOW);
    return TRUE;
}

BOOL SetWindowPos(HWND window, HWND insert_after, int x, int y, int cx, int cy,
                  UINT flags) {
    (void)insert_after;
    struct WindowObject* object = window_from_handle(window);
    if (object == NULL) return FALSE;
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
    return SetWindowPos(window, NULL, x, y, width, height,
                        repaint ? 0 : SWP_NOREDRAW);
}

static int scroll_bar_index(int bar) {
    return bar == SB_HORZ || bar == SB_VERT ? bar : -1;
}

VOID SetScrollRange(HWND window, int bar, int minimum, int maximum,
                    BOOL redraw) {
    (void)redraw;
    struct WindowObject* object = window_from_handle(window);
    const int index = scroll_bar_index(bar);
    if (object == NULL || index < 0) return;
    object->scroll_min[index] = minimum;
    object->scroll_max[index] = maximum;
}

int SetScrollPos(HWND window, int bar, int position, BOOL redraw) {
    (void)redraw;
    struct WindowObject* object = window_from_handle(window);
    const int index = scroll_bar_index(bar);
    if (object == NULL || index < 0) return 0;
    const int previous = object->scroll_pos[index];
    object->scroll_pos[index] =
        min_int(max_int(position, object->scroll_min[index]),
                object->scroll_max[index]);
    return previous;
}

BOOL SetWindowTextA(HWND window, LPCSTR text) {
    struct WindowObject* object = window_from_handle(window);
    if (object == NULL) return FALSE;
    object->text = dup_string(text != NULL ? text : "");
    return object->text != NULL;
}

BOOL SetWindowTextW(HWND window, LPCWSTR text) {
    char* narrow = narrow_string_dup(text);
    BOOL result = SetWindowTextA(window, narrow);
    free(narrow);
    return result;
}

int GetWindowTextLengthA(HWND window) {
    struct WindowObject* object = window_from_handle(window);
    return object != NULL ? (int)strlen(object->text) : 0;
}

int GetWindowTextLengthW(HWND window) {
    return GetWindowTextLengthA(window);
}

int GetWindowTextA(HWND window, LPSTR text, int max_count) {
    struct WindowObject* object = window_from_handle(window);
    if (object == NULL || text == NULL || max_count <= 0) return 0;
    lstrcpynA(text, object->text, max_count);
    return (int)strlen(text);
}

int GetWindowTextW(HWND window, LPWSTR text, int max_count) {
    struct WindowObject* object = window_from_handle(window);
    if (object == NULL || text == NULL || max_count <= 0) return 0;
    const size_t limit = (size_t)max_count;
    const size_t count = min_int((int)strlen(object->text), (int)limit - 1);
    size_t index;
    for (index = 0; index < count; ++index) {
        text[index] = (WCHAR)(unsigned char)object->text[index];
    }
    text[count] = 0;
    return (int)count;
}

BOOL UpdateWindow(HWND window) {
    return IsWindow(window);
}

BOOL MessageBeep(UINT type) {
    (void)type;
    return TRUE;
}

UINT TrackPopupMenu(HMENU menu, UINT flags, int x, int y, int reserved,
                    HWND window, const RECT* rect) {
    (void)menu;
    (void)flags;
    (void)x;
    (void)y;
    (void)reserved;
    (void)window;
    (void)rect;
    return 0;
}

BOOL IsDialogMessageA(HWND window, LPMSG message) {
    (void)window;
    (void)message;
    return FALSE;
}

BOOL TranslateMessage(const MSG* message) {
    if (message == NULL) return FALSE;
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
    struct WindowObject* object = window_from_handle(window);
    if (object == NULL) return 0;
    if (object->klass.procedure == NULL) {
        return DefWindowProcA(window, message, wparam, lparam);
    }
    return object->klass.procedure(window, message, wparam, lparam);
}

LRESULT SendMessageW(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
    struct WindowObject* object = window_from_handle(window);
    if (object == NULL) return 0;
    if (object->klass.procedure == NULL) {
        return DefWindowProcW(window, message, wparam, lparam);
    }
    return object->klass.procedure(window, message, wparam, lparam);
}

LRESULT DispatchMessageA(const MSG* message) {
    if (message == NULL) return 0;
    return SendMessageA(message->hwnd, message->message, message->wParam,
                        message->lParam);
}

void OpusUser32PushScriptedInput(HWND window, UINT message, WPARAM wparam,
                                 LPARAM lparam) {
    MSG item = {window, message, wparam, lparam, 0, {0, 0}};
    queue_push(&g_scripted_messages, &g_scripted_count, &g_scripted_capacity,
               &item, FALSE);
}

void OpusUser32ExpectScriptedChar(UINT character) {
    g_expected_scripted_char = character;
    g_matched_scripted_char = FALSE;
}

BOOL OpusUser32ScriptedCharMatched(void) { return g_matched_scripted_char; }

BOOL PostMessageA(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
    if (window != NULL && !IsWindow(window)) return FALSE;
    update_key_state(message, wparam);
    queue_back(window, message, wparam, lparam);
    return TRUE;
}

BOOL PostMessageW(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
    return PostMessageA(window, message, wparam, lparam);
}

LRESULT CallNextHookEx(HANDLE hook, int code, WPARAM wparam, LPARAM lparam) {
    (void)hook;
    (void)code;
    (void)wparam;
    (void)lparam;
    return 0;
}

SHORT VkKeyScanA(CHAR ch) {
    const unsigned char c = (unsigned char)ch;
    if (c >= 'a' && c <= 'z') return (SHORT)(c - 'a' + 'A');
    if (c >= 'A' && c <= 'Z') return (SHORT)(0x0100 | c);
    if (c >= '0' && c <= '9') return (SHORT)c;
    switch (c) {
        case ' ': return VK_SPACE;
        case '\t': return VK_TAB;
        case '\r': return VK_RETURN;
        case '+': return (SHORT)(0x0100 | VK_OEM_PLUS);
        case '=': return VK_OEM_PLUS;
        case '-': return VK_OEM_MINUS;
        case '_': return (SHORT)(0x0100 | VK_OEM_MINUS);
        case '*': return (SHORT)(0x0100 | '8');
        case '?': return (SHORT)(0x0100 | VK_OEM_2);
        case '/': return VK_OEM_2;
        case ',': return VK_OEM_COMMA;
        case '.': return VK_OEM_PERIOD;
        default: return -1;
    }
}

SHORT VkKeyScanW(WCHAR ch) {
    return ch <= 0x7f ? VkKeyScanA((CHAR)ch) : (SHORT)-1;
}

UINT_PTR SetTimer(HWND window, UINT_PTR event, UINT elapsed, LPVOID timer_func) {
    if (window != NULL && !IsWindow(window)) return 0;
    UINT_PTR id = event;
    if (id == 0) {
        BOOL exists;
        do {
            id = g_next_timer_id++;
            if (g_next_timer_id == 0) g_next_timer_id = 1;
            exists = FALSE;
            size_t index;
            for (index = 0; index < g_timer_count; ++index) {
                if (g_timers[index].window == window && g_timers[index].id == id) {
                    exists = TRUE;
                    break;
                }
            }
        } while (exists);
    }
    remove_timer_messages(window, id);
    const UINT interval = max_int(elapsed, 1u);
    const ULONGLONG due = GetTickCount64() + interval;
    size_t index;
    for (index = 0; index < g_timer_count; ++index) {
        struct TimerObject* timer = &g_timers[index];
        if (timer->window == window && timer->id == id) {
            timer->elapsed = interval;
            timer->due = due;
            timer->callback = timer_func;
            timer->queued = FALSE;
            return id;
        }
    }
    if (!reserve_bytes((void**)&g_timers, &g_timer_capacity, g_timer_count + 1,
                       sizeof(g_timers[0]))) {
        return 0;
    }
    g_timers[g_timer_count++] =
        (struct TimerObject){window, id, interval, due, timer_func, FALSE};
    return id;
}

BOOL KillTimer(HWND window, UINT_PTR event) {
    const size_t previous_size = g_timer_count;
    size_t index = 0;
    while (index < g_timer_count) {
        if (g_timers[index].window == window && g_timers[index].id == event) {
            memmove(&g_timers[index], &g_timers[index + 1],
                    (g_timer_count - index - 1) * sizeof(g_timers[0]));
            --g_timer_count;
        } else {
            ++index;
        }
    }
    if (g_timer_count == previous_size) return FALSE;
    remove_timer_messages(window, event);
    return TRUE;
}

VOID PostQuitMessage(int exit_code) {
    g_quit_posted = TRUE;
    g_quit_code = exit_code;
}

BOOL GetMessageA(LPMSG message, HWND window, UINT filter_min, UINT filter_max) {
    if (message == NULL) return FALSE;
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
    if (g_message_count != 0 || g_quit_posted || pump_once()) return TRUE;
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
    if (key_state == NULL) return FALSE;
    int index;
    for (index = 0; index < 256; ++index) {
        g_key_state[index] =
            (SHORT)(((key_state[index] & 0x80) != 0 ? 0x8000 : 0) |
                    (key_state[index] & 1));
    }
    return TRUE;
}

int MessageBoxA(HWND window, LPCSTR text, LPCSTR caption, UINT type) {
    (void)window;
    (void)text;
    (void)caption;
    (void)type;
    return IDOK;
}
