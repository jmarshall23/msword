#define WIN32_LEAN_AND_MEAN
#include "opus_x64_compat.h"
#include "imm.h"
#include "windowsx.h"

#ifndef OPUSW
#define OPUSW(text) u##text
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define USEBCM
#include "opuscmd.h"

int OpusGetWin95VerticalRulerMetrics(
    HWND pane, int* page_top, int* page_bottom, int* top_margin,
    int* bottom_margin, int* pixels_per_inch);
int OpusGetWin95ContinuousPageMetrics(
    HWND pane, int* page_left, int* page_top, int* page_right,
    int* page_bottom, int* page_gap, int* has_previous, int* has_next);
int OpusScrollWin95ContinuousPages(HWND pane, int pixels);
int OpusGetWin95ZoomPercent(HWND pane);
int OpusGetWin95CurrentPageIndex(HWND pane);
int OpusAdjustWin95Zoom(HWND pane, int percent_delta);
int OpusRecenterWin95PageView(HWND pane);
int OpusSetWin95VerticalMargin(
    HWND pane, int top_margin, int margin_pixels);
int OpusApplyWin95TextColor(HWND pane, int color_index);
int OpusGetWin95HorizontalRulerMetrics(
    HWND ruler, int* zero, int* active_left, int* active_right,
    int* pixels_per_inch, int* left_indent, int* first_indent,
    int* right_indent, int* default_tab, int* tab_count,
    int* tab_positions, unsigned char* tab_types, int tab_capacity);
int OpusGetWin95HorizontalPageMargins(
    HWND ruler, int* left_margin, int* right_margin);
int OpusAdjustWin95HorizontalMargin(
    HWND ruler, int left_margin, int delta_pixels);
void OpusDrawWin95HorizontalRuler(HWND ruler);
int OpusExportCurrentDocumentPdf(void);
int OpusGetUnicodeSelection(
    int* doc, long* cp_first, long* cp_lim);
int OpusUnicodeLegacyByteForScalar(unsigned int scalar);
int OpusUnicodeSetScalar(
    int doc, long cp, unsigned int scalar);
int OpusUnicodeSetInputLanguage(const char* language);
int OpusUnicodeGetInputLanguage(char* language, int capacity);
int OpusQueueUnicodeWmChar(HWND pane, unsigned int code_unit);

static const WCHAR kToolbarClass[] = OPUSW("OpusWin95Toolbar");
static const WCHAR kRulerOverlayClass[] = OPUSW("OpusWin95RulerOverlay");
static const WCHAR kComboBoxClass[] = OPUSW("ComboBox");
static const WCHAR kComboBoxWindowClass[] = OPUSW("COMBOBOX");
static const WCHAR kDocumentPaneClass[] = OPUSW("OpusWwd");
static const WCHAR kRulerClass[] = OPUSW("OpusRul");
static const WCHAR kMenuPopupClass[] = OPUSW("#32768");
static const WCHAR kArialFace[] = OPUSW("Arial");
static const WCHAR kToolbarFace[] = OPUSW("MS Sans Serif");
static const WCHAR kNormalStyle[] = OPUSW("Normal");
static const WCHAR kDefaultSize[] = OPUSW("10");
static const WCHAR kZoom100[] = OPUSW("100%");
static const WCHAR kZoomPageWidth[] = OPUSW("Page Width");
static const WCHAR kZoomWholePage[] = OPUSW("Whole Page");
static const WCHAR kZoomPrintPreview[] = OPUSW("Print Preview");
static const WCHAR kDwmApiLibrary[] = OPUSW("dwmapi.dll");
static const WCHAR kFormatGlyphColor[] = OPUSW("A");
static const WCHAR kFormatGlyphBold[] = OPUSW("B");
static const WCHAR kFormatGlyphItalic[] = OPUSW("I");
static const WCHAR kFormatGlyphUnderline[] = OPUSW("U");
static const WCHAR kOriginalPaneProcProperty[] = OPUSW("OpusWord95OriginalPaneProc");

enum {
    kToolbarBitmap = 201,
    kSpriteCell = 20,
    kComboStyle = 0x7501,
    kComboFont = 0x7502,
    kComboSize = 0x7503,
    kComboZoom = 0x7504,
    kCmdToggleStandardToolbar = 0x7101,
    kCmdToggleFormattingToolbar = 0x7102,
    kCmdExportPdf = 0x7103,
    kCmdTextColorBase = 0x7200,
    kCmdLanguageBase = 0x7300,
    kStandardButtonCount = 18,
    kFormatButtonCount = 14,
    kLanguageChoiceCount = 17,
    kMaxPendingUnicodeInput = 0xffff,
    kWmCommitUnicodeScalar = WM_APP + 0x452
};

static const COLORREF kButtonFace = RGB(192, 192, 192);
static const UINT_PTR kMenuRepaintTimer = 0x7f51;
static const COLORREF kButtonShadow = RGB(128, 128, 128);
static const COLORREF kButtonDarkShadow = RGB(0, 0, 0);
static const COLORREF kButtonHighlight = RGB(255, 255, 255);
static const COLORREF kCaptionBlue = RGB(0, 0, 128);
static const UINT_PTR kSyncTimer = 0x951;

typedef enum FormatGlyph {
    bold,
    italic,
    underline,
    color,
    align_left,
    align_center,
    align_right,
    align_justify,
    numbered,
    bullets,
    indent_left,
    indent_right,
    table,
    borders
} FormatGlyph;

typedef struct SpriteButton {
    int sprite;
    UINT command;
    int group;
    bool latch;
} SpriteButton;

typedef struct FormatButton {
    FormatGlyph glyph;
    UINT command;
    int group;
    bool latch;
} FormatButton;

static const SpriteButton kStandardButtons[kStandardButtonCount] = {
    {0, bcmFileNew, 0, false},
    {1, imiOpen, 0, false},
    {2, bcmSave, 0, false},
    {3, bcmPrint, 1, false},
    {4, bcmPrintPreview, 1, false},
    {5, imiSpelling, 1, false},
    {6, bcmCut, 2, false},
    {7, bcmCopy, 2, false},
    {8, bcmPaste, 2, false},
    {9, bcmCopyLooks, 2, false},
    {10, bcmUndo, 3, false},
    {11, bcmRepeat, 3, false},
    {12, bcmInsTable, 4, false},
    {13, bcmSection, 4, false},
    {15, bcmInsPic, 4, false},
    {14, bcmShowAll, 4, true},
    {15, bcmHelp, 5, false},
    {16, bcmHelp, 5, false},
};

static const FormatButton kFormatButtons[kFormatButtonCount] = {
    {bold, bcmBold, 0, true},
    {italic, bcmItalic, 0, true},
    {underline, bcmULine, 0, true},
    {color, bcmColor, 0, false},
    {align_left, bcmParaLeft, 1, true},
    {align_center, bcmParaCenter, 1, true},
    {align_right, bcmParaRight, 1, true},
    {align_justify, bcmParaBoth, 1, true},
    {numbered, imiRenumParas, 2, false},
    {bullets, imiRenumParas, 2, false},
    {indent_left, bcmUnIndent, 2, false},
    {indent_right, bcmIndent, 2, false},
    {table, bcmInsTable, 3, false},
    {borders, bcmParagraph, 3, false},
};

typedef struct HitResult {
    bool hit;
    bool format;
    int index;
    UINT command;
} HitResult;

typedef struct ToolbarState {
    HBITMAP sprite;
    HFONT font;
    HWND style_combo;
    HWND font_combo;
    HWND size_combo;
    HWND zoom_combo;
    HWND source_style;
    HWND source_font;
    HWND source_size;
    int copied_style_count;
    int copied_font_count;
    int copied_size_count;
    ULONGLONG suppress_sync_until;
    ULONGLONG page_view_start_after;
    bool style_edit_dirty;
    bool font_edit_dirty;
    bool size_edit_dirty;
    bool standard_visible;
    bool formatting_visible;
    bool startup_page_view_requested;
    int ruler_refreshes_remaining;
    int text_color_index;
    COLORREF text_color;
    HitResult pressed;
    bool standard_latched[kStandardButtonCount];
    bool format_latched[kFormatButtonCount];
} ToolbarState;

static HBRUSH g_menu_brush = NULL;
static HMENU g_table_menu = NULL;
static HMENU g_toolbars_menu = NULL;
static HMENU g_language_menu = NULL;
static WNDPROC g_original_app_proc = NULL;
static bool g_word95_page_view_active;
static WCHAR g_pending_high_surrogate;

typedef struct LanguageChoice {
    UINT command;
    const WCHAR* label;
    const char* tag;
} LanguageChoice;

typedef struct PendingUnicodeInput {
    int doc;
    long cp;
    uint32_t scalar;
    int retries;
} PendingUnicodeInput;

typedef struct PendingUnicodeQueue {
    PendingUnicodeInput* data;
    size_t size;
    size_t capacity;
} PendingUnicodeQueue;

static PendingUnicodeQueue g_pending_unicode_inputs;
static PendingUnicodeInput g_active_unicode_input;
static bool g_unicode_input_active;

static const LanguageChoice kLanguageChoices[kLanguageChoiceCount] = {
    {kCmdLanguageBase, OPUSW("&Automatic (Keyboard)"), "auto"},
    {kCmdLanguageBase + 1, OPUSW("&English (United States)"), "en-US"},
    {kCmdLanguageBase + 2, OPUSW("&Spanish"), "es-ES"},
    {kCmdLanguageBase + 3, OPUSW("&French"), "fr-FR"},
    {kCmdLanguageBase + 4, OPUSW("&German"), "de-DE"},
    {kCmdLanguageBase + 5, OPUSW("&Polish / Central European"), "pl-PL"},
    {kCmdLanguageBase + 6, OPUSW("&Greek"), "el-GR"},
    {kCmdLanguageBase + 7, OPUSW("&Russian / Cyrillic"), "ru-RU"},
    {kCmdLanguageBase + 8, OPUSW("&Turkish"), "tr-TR"},
    {kCmdLanguageBase + 9, OPUSW("&Hebrew"), "he-IL"},
    {kCmdLanguageBase + 10, OPUSW("&Arabic"), "ar-SA"},
    {kCmdLanguageBase + 11, OPUSW("&Thai"), "th-TH"},
    {kCmdLanguageBase + 12, OPUSW("&Vietnamese"), "vi-VN"},
    {kCmdLanguageBase + 13, OPUSW("&Japanese"), "ja-JP"},
    {kCmdLanguageBase + 14, OPUSW("Chinese (&Simplified)"), "zh-CN"},
    {kCmdLanguageBase + 15, OPUSW("Chinese (&Traditional)"), "zh-TW"},
    {kCmdLanguageBase + 16, OPUSW("&Korean"), "ko-KR"},
};

typedef struct VerticalRulerDragState {
    HWND pane;
    bool active;
    bool top;
    int preview_y;
    int page_offset;
} VerticalRulerDragState;

static VerticalRulerDragState g_vertical_ruler_drag;

typedef struct HorizontalRulerDragState {
    HWND overlay;
    HWND ruler;
    bool active;
    bool left;
    int origin_x;
    int preview_x;
} HorizontalRulerDragState;

static HorizontalRulerDragState g_horizontal_ruler_drag;

typedef struct DocumentWheelState {
    HWND pane;
    int scroll_remainder;
    int zoom_remainder;
} DocumentWheelState;

static DocumentWheelState g_document_wheel;

typedef struct PageSnapshot {
    HWND pane;
    int page_index;
    int zoom_percent;
    int width;
    int height;
    HBITMAP bitmap;
    ULONGLONG last_used;
} PageSnapshot;

typedef struct PageSnapshotList {
    PageSnapshot* data;
    size_t size;
    size_t capacity;
} PageSnapshotList;

static PageSnapshotList g_page_snapshots;

static int min_int(int left, int right) {
    return left < right ? left : right;
}

static int max_int(int left, int right) {
    return left > right ? left : right;
}

static void toolbar_state_init(ToolbarState* state) {
    memset(state, 0, sizeof(*state));
    state->copied_style_count = -1;
    state->copied_font_count = -1;
    state->copied_size_count = -1;
    state->standard_visible = true;
    state->formatting_visible = true;
    state->text_color = RGB(0, 0, 0);
    state->pressed.index = -1;
}

static bool hwnd_list_push(HWND** data, size_t* size, size_t* capacity,
                           HWND value) {
    if (*size == *capacity) {
        const size_t next = *capacity == 0 ? 8 : *capacity * 2;
        HWND* grown = (HWND*) realloc(*data, next * sizeof((*data)[0]));
        if (grown == NULL) return false;
        *data = grown;
        *capacity = next;
    }
    (*data)[(*size)++] = value;
    return true;
}

static bool pending_unicode_push(PendingUnicodeInput input) {
    if (g_pending_unicode_inputs.size >= kMaxPendingUnicodeInput) return false;
    if (g_pending_unicode_inputs.size == g_pending_unicode_inputs.capacity) {
        const size_t next = g_pending_unicode_inputs.capacity == 0 ? 16 :
            g_pending_unicode_inputs.capacity * 2;
        PendingUnicodeInput* data = (PendingUnicodeInput*) realloc(
            g_pending_unicode_inputs.data, next * sizeof(data[0]));
        if (data == NULL) return false;
        g_pending_unicode_inputs.data = data;
        g_pending_unicode_inputs.capacity = next;
    }
    g_pending_unicode_inputs.data[g_pending_unicode_inputs.size++] = input;
    return true;
}

static bool pending_unicode_empty(void) {
    return g_pending_unicode_inputs.size == 0;
}

static PendingUnicodeInput pending_unicode_pop_front(void) {
    PendingUnicodeInput value = g_pending_unicode_inputs.data[0];
    memmove(g_pending_unicode_inputs.data, g_pending_unicode_inputs.data + 1,
            (g_pending_unicode_inputs.size - 1) *
                sizeof(g_pending_unicode_inputs.data[0]));
    --g_pending_unicode_inputs.size;
    return value;
}

static void pending_unicode_clear(void) {
    g_pending_unicode_inputs.size = 0;
}

static bool page_snapshot_push(PageSnapshot snapshot) {
    if (g_page_snapshots.size == g_page_snapshots.capacity) {
        const size_t next = g_page_snapshots.capacity == 0 ? 8 :
            g_page_snapshots.capacity * 2;
        PageSnapshot* data = (PageSnapshot*) realloc(
            g_page_snapshots.data, next * sizeof(data[0]));
        if (data == NULL) return false;
        g_page_snapshots.data = data;
        g_page_snapshots.capacity = next;
    }
    g_page_snapshots.data[g_page_snapshots.size++] = snapshot;
    return true;
}

static void page_snapshot_delete(size_t index) {
    if (index >= g_page_snapshots.size) return;
    if (g_page_snapshots.data[index].bitmap != NULL) {
        DeleteObject(g_page_snapshots.data[index].bitmap);
    }
    memmove(g_page_snapshots.data + index, g_page_snapshots.data + index + 1,
            (g_page_snapshots.size - index - 1) *
                sizeof(g_page_snapshots.data[0]));
    --g_page_snapshots.size;
}

static int format_wide_decimal(WCHAR* output, size_t capacity, int value,
                               const WCHAR* suffix) {
    char ascii[32];
    int length;
    if (capacity == 0) return 0;
    memset(output, 0, capacity * sizeof(output[0]));
    snprintf(ascii, sizeof(ascii), "%d", value);
    length = 0;
    while (ascii[length] != '\0' && (size_t) length + 1 < capacity) {
        output[length] = (WCHAR) (unsigned char) ascii[length];
        ++length;
    }
    if (suffix != NULL) {
        for (int index = 0; suffix[index] != 0 &&
             (size_t) length + 1 < capacity; ++index) {
            output[length++] = suffix[index];
        }
    }
    output[length] = 0;
    return length;
}

static void draw_vertical_margin_marker(HDC dc, const RECT* ruler,
                                        int marker_x, int marker_half,
                                        int y) {
    if (y < ruler->top || y >= ruler->bottom) {
        return;
    }
    POINT points[3] = {{marker_x, y},
                       {marker_x - marker_half, y - marker_half},
                       {marker_x - marker_half, y + marker_half}};
    HBRUSH marker = CreateSolidBrush(RGB(255, 255, 255));
    HBRUSH old = (HBRUSH) SelectObject(dc, marker);
    Polygon(dc, points, 3);
    SelectObject(dc, old);
    DeleteObject(marker);
}

static void draw_down_triangle(HDC dc, int x, int marker_top,
                               int marker_bottom) {
    POINT points[3] = {{x - 5, marker_top}, {x + 5, marker_top},
                       {x, min_int(marker_bottom, marker_top + 6)}};
    Polygon(dc, points, 3);
}

static void draw_up_triangle(HDC dc, int x, int marker_top,
                             int marker_bottom, bool with_base) {
    POINT points[3] = {{x - 5, marker_bottom - 2},
                       {x + 5, marker_bottom - 2},
                       {x, max_int(marker_top, marker_bottom - 8)}};
    Polygon(dc, points, 3);
    if (with_base) {
        Rectangle(dc, x - 3, marker_bottom - 2, x + 4,
                  marker_bottom + 1);
    }
}

int dpi_for_window(HWND window) {
    typedef UINT(WINAPI* GetDpiForWindowProc)(HWND);
    static GetDpiForWindowProc get_dpi;
    static bool looked_up;
    if (!looked_up) {
        get_dpi = (GetDpiForWindowProc) GetProcAddress(
            GetModuleHandleW(OPUSW("user32.dll")), "GetDpiForWindow");
        looked_up = true;
    }
    return get_dpi != NULL && window != NULL ?
               (int) get_dpi(window) :
               96;
}

int scale(HWND window, int value) {
    return MulDiv(value, dpi_for_window(window), 96);
}

void set_window_classic(HWND window) {
    typedef HRESULT(WINAPI* SetWindowThemeProc)(HWND, LPCWSTR, LPCWSTR);
    static SetWindowThemeProc set_theme;
    static bool looked_up;
    if (!looked_up) {
        HMODULE theme_module = LoadLibraryExW(
            OPUSW("uxtheme.dll"), NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);
        set_theme = theme_module != NULL ?
            (SetWindowThemeProc) GetProcAddress(theme_module,
                                                "SetWindowTheme") :
            NULL;
        looked_up = true;
    }
    if (set_theme != NULL && window != NULL) {
        set_theme(window, OPUSW(""), OPUSW(""));
    }
}

void style_menu_tree(HMENU menu) {
    if (menu == NULL) {
        return;
    }
    MENUINFO info = {0};
    info.cbSize = sizeof(info);
    info.fMask = MIM_BACKGROUND | MIM_APPLYTOSUBMENUS;
    info.hbrBack = g_menu_brush;
    SetMenuInfo(menu, &info);
    const int count = GetMenuItemCount(menu);
    for (int index = 0; index < count; ++index) {
        style_menu_tree(GetSubMenu(menu, index));
    }
}

BOOL CALLBACK repaint_menu_popup(HWND window, LPARAM parameter) {
    (void) parameter;
    WCHAR class_name[32] = {0};
    GetClassNameW(window, class_name,
                  (int) (sizeof(class_name) / sizeof(class_name[0])));
    if (lstrcmpW(class_name, kMenuPopupClass) == 0 && IsWindowVisible(window)) {
        RedrawWindow(window, NULL, NULL,
                     RDW_ERASE | RDW_INVALIDATE | RDW_UPDATENOW |
                         RDW_ALLCHILDREN | RDW_FRAME);
    }
    return TRUE;
}

static int find_named_menu(HMENU menu, LPCWSTR expected) {
    const int count = menu != NULL ? GetMenuItemCount(menu) : 0;
    for (int index = 0; index < count; ++index) {
        WCHAR label[80] = {0};
        WCHAR name[80] = {0};
        int output = 0;
        GetMenuStringW(menu, index, label,
                       (int) (sizeof(label) / sizeof(label[0])),
                       MF_BYPOSITION);
        for (int input = 0; label[input] != 0 && output < 79; ++input) {
            if (label[input] != OPUSW("&")[0]) {
                name[output++] = label[input];
            }
        }
        if (lstrcmpiW(name, expected) == 0) {
            return index;
        }
    }
    return -1;
}

void configure_word95_menus(HWND window) {
    HMENU root = GetMenu(window);
    if (root == NULL || GetMenuItemCount(root) < 7) {
        return;
    }

    // Word's menu loader can finish replacing the startup menu after this layer
    // is created, so normalize by label every time we resync.
    const int file_index = find_named_menu(root, OPUSW("File"));
    if (file_index >= 0 && GetMenuItemCount(root) > file_index + 4) {
        HMENU insert = GetSubMenu(root, file_index + 3);
        HMENU format = GetSubMenu(root, file_index + 4);
        if (insert != NULL) {
            ModifyMenuW(root, file_index + 3,
                        MF_BYPOSITION | MF_POPUP | MF_STRING,
                        ((UINT_PTR) insert), OPUSW("&Insert"));
        }
        if (format != NULL) {
            ModifyMenuW(root, file_index + 4,
                        MF_BYPOSITION | MF_POPUP | MF_STRING,
                        ((UINT_PTR) format),
                        OPUSW("F&ormat"));
        }
    }
    HMENU file_menu = file_index >= 0 ? GetSubMenu(root, file_index) : NULL;
    if (file_menu != NULL &&
        GetMenuState(file_menu, kCmdExportPdf, MF_BYCOMMAND) ==
            ((UINT) -1)) {
        int exit_position = find_named_menu(file_menu, OPUSW("Exit"));
        if (exit_position < 0) exit_position = GetMenuItemCount(file_menu);
        InsertMenuW(file_menu, exit_position, MF_BYPOSITION | MF_SEPARATOR,
                    0, NULL);
        InsertMenuW(file_menu, exit_position, MF_BYPOSITION | MF_STRING,
                    kCmdExportPdf, OPUSW("E&xport as PDF..."));
    }
    const int utilities_index = find_named_menu(root, OPUSW("Utilities"));
    if (utilities_index >= 0) {
        HMENU tools = GetSubMenu(root, utilities_index);
        ModifyMenuW(root, utilities_index,
                    MF_BYPOSITION | MF_POPUP | MF_STRING,
                    ((UINT_PTR) tools), OPUSW("&Tools"));
    }
    const int tools_index = find_named_menu(root, OPUSW("Tools"));
    HMENU tools_menu = tools_index >= 0 ? GetSubMenu(root, tools_index) :
        (utilities_index >= 0 ? GetSubMenu(root, utilities_index) : NULL);
    if (g_language_menu == NULL || !IsMenu(g_language_menu)) {
        g_language_menu = CreatePopupMenu();
        if (g_language_menu != NULL) {
            for (int index = 0; index < kLanguageChoiceCount; ++index) {
                AppendMenuW(g_language_menu, MF_STRING,
                            kLanguageChoices[index].command,
                            kLanguageChoices[index].label);
            }
        }
    }
    if (tools_menu != NULL && g_language_menu != NULL) {
        bool present = false;
        for (int index = 0; index < GetMenuItemCount(tools_menu); ++index) {
            if (GetSubMenu(tools_menu, index) == g_language_menu) {
                present = true;
                break;
            }
        }
        if (!present) {
            AppendMenuW(tools_menu, MF_SEPARATOR, 0, NULL);
            AppendMenuW(tools_menu, MF_POPUP | MF_STRING,
                        ((UINT_PTR) g_language_menu),
                        OPUSW("&Language"));
        }
    }

    if (g_table_menu == NULL || !IsMenu(g_table_menu)) {
        g_table_menu = CreatePopupMenu();
        if (g_table_menu != NULL) {
            AppendMenuW(g_table_menu, MF_STRING, bcmInsTable,
                        OPUSW("&Insert Table..."));
            AppendMenuW(g_table_menu, MF_STRING, imiEditTable,
                        OPUSW("&Rows and Columns..."));
            AppendMenuW(g_table_menu, MF_STRING, bcmFormatTable,
                        OPUSW("Table &Properties..."));
            AppendMenuW(g_table_menu, MF_SEPARATOR, 0, NULL);
            AppendMenuW(g_table_menu, MF_STRING, bcmSelectTable,
                        OPUSW("&Select Table"));
            AppendMenuW(g_table_menu, MF_STRING, bcmNextCell,
                        OPUSW("Move to &Next Cell"));
            AppendMenuW(g_table_menu, MF_STRING, bcmPrevCell,
                        OPUSW("Move to Pre&vious Cell"));
            AppendMenuW(g_table_menu, MF_SEPARATOR, 0, NULL);
            AppendMenuW(g_table_menu, MF_STRING, bcmTableToText,
                        OPUSW("Convert Table to Te&xt..."));
            AppendMenuW(g_table_menu, MF_STRING, imiSort,
                        OPUSW("&Sort..."));
            AppendMenuW(g_table_menu, MF_STRING, imiCalculate,
                        OPUSW("&Formula/Calculate"));
        }
    }
    for (int index = GetMenuItemCount(root) - 1; index >= 0; --index) {
        if (GetSubMenu(root, index) == g_table_menu) {
            RemoveMenu(root, index, MF_BYPOSITION);
        }
    }
    const int window_index = find_named_menu(root, OPUSW("Window"));
    if (g_table_menu != NULL && window_index >= 0) {
        InsertMenuW(root, window_index,
                    MF_BYPOSITION | MF_POPUP | MF_STRING,
                    ((UINT_PTR) g_table_menu),
                    OPUSW("Ta&ble"));
    }

    if (g_toolbars_menu == NULL || !IsMenu(g_toolbars_menu)) {
        g_toolbars_menu = CreatePopupMenu();
        if (g_toolbars_menu != NULL) {
            AppendMenuW(g_toolbars_menu, MF_STRING,
                        kCmdToggleStandardToolbar, OPUSW("&Standard"));
            AppendMenuW(g_toolbars_menu, MF_STRING,
                        kCmdToggleFormattingToolbar, OPUSW("&Formatting"));
            AppendMenuW(g_toolbars_menu, MF_SEPARATOR, 0, NULL);
            AppendMenuW(g_toolbars_menu, MF_STRING, bcmRuler,
                        OPUSW("&Ruler"));
            AppendMenuW(g_toolbars_menu, MF_STRING, bcmStatusArea,
                        OPUSW("&Status Bar"));
            AppendMenuW(g_toolbars_menu, MF_SEPARATOR, 0, NULL);
            AppendMenuW(g_toolbars_menu, MF_STRING, bcmCustomize,
                        OPUSW("&Customize..."));
        }
    }

    const int view_index = find_named_menu(root, OPUSW("View"));
    HMENU view_menu = view_index >= 0 ? GetSubMenu(root, view_index) : NULL;
    if (view_menu != NULL && g_toolbars_menu != NULL) {
        for (int index = GetMenuItemCount(view_menu) - 1; index >= 0;
             --index) {
            if (GetSubMenu(view_menu, index) == g_toolbars_menu ||
                find_named_menu(view_menu, OPUSW("Toolbars")) == index) {
                RemoveMenu(view_menu, index, MF_BYPOSITION);
            }
        }
        AppendMenuW(view_menu, MF_POPUP | MF_STRING,
                    ((UINT_PTR) g_toolbars_menu),
                    OPUSW("&Toolbars"));

        if (GetMenuState(view_menu, bcmRibbon, MF_BYCOMMAND) !=
            ((UINT) -1)) {
            ModifyMenuW(view_menu, bcmRibbon,
                        MF_BYCOMMAND | MF_STRING,
                        kCmdToggleFormattingToolbar,
                        OPUSW("&Formatting Toolbar"));
        }
    }

    const int normalized_window_index = find_named_menu(root, OPUSW("Window"));
    HMENU window_menu = normalized_window_index >= 0 ?
        GetSubMenu(root, normalized_window_index) : NULL;
    if (window_menu != NULL) {
        if (GetMenuState(window_menu, imiNewWnd, MF_BYCOMMAND) ==
            ((UINT) -1)) {
            AppendMenuW(window_menu, MF_STRING, imiNewWnd,
                        OPUSW("&New Window"));
        }
        if (GetMenuState(window_menu, bcmArrangeWnd, MF_BYCOMMAND) ==
            ((UINT) -1)) {
            AppendMenuW(window_menu, MF_STRING, bcmArrangeWnd,
                        OPUSW("&Arrange All"));
        }
        if (GetMenuState(window_menu, bcmZoomWnd, MF_BYCOMMAND) ==
            ((UINT) -1)) {
            AppendMenuW(window_menu, MF_SEPARATOR, 0, NULL);
            AppendMenuW(window_menu, MF_STRING, bcmZoomWnd,
                        OPUSW("Ma&ximize Document"));
            AppendMenuW(window_menu, MF_STRING, bcmRestoreWnd,
                        OPUSW("&Restore Document"));
            AppendMenuW(window_menu, MF_STRING, bcmCloseWnd,
                        OPUSW("&Close Document"));
        }
    }
}

void apply_caption_colors(HWND window) {
    typedef HRESULT(WINAPI* DwmSetWindowAttributeProc)(HWND, DWORD,
                                                       LPCVOID, DWORD);
    HMODULE module = LoadLibraryExW(
        kDwmApiLibrary, NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (module == NULL) {
        return;
    }
    DwmSetWindowAttributeProc set_attribute =
        (DwmSetWindowAttributeProc) GetProcAddress(
            module, "DwmSetWindowAttribute");
    if (set_attribute != NULL) {
        const DWORD kDwmBorderColor = 34;
        const DWORD kDwmCaptionColor = 35;
        const DWORD kDwmTextColor = 36;
        const DWORD kDwmCornerPreference = 33;
        const COLORREF border = kButtonShadow;
        const COLORREF caption = kCaptionBlue;
        const COLORREF text = RGB(255, 255, 255);
        const DWORD do_not_round = 1;
        set_attribute(window, kDwmBorderColor, &border, sizeof(border));
        set_attribute(window, kDwmCaptionColor, &caption, sizeof(caption));
        set_attribute(window, kDwmTextColor, &text, sizeof(text));
        set_attribute(window, kDwmCornerPreference, &do_not_round,
                      sizeof(do_not_round));
    }
    FreeLibrary(module);
}

ToolbarState* toolbar_state(HWND toolbar) {
    return toolbar != NULL ?
        (ToolbarState*) GetWindowLongPtrW(toolbar, GWLP_USERDATA) : NULL;
}

int toolbar_height(HWND toolbar) {
    const ToolbarState* state = toolbar_state(toolbar);
    if (state == NULL ||
        (state->standard_visible && state->formatting_visible)) {
        return scale(toolbar, 64);
    }
    if (state->standard_visible) {
        return scale(toolbar, 33);
    }
    if (state->formatting_visible) {
        return scale(toolbar, 31);
    }
    return 0;
}

int formatting_row_top(HWND toolbar, const ToolbarState* state) {
    return scale(toolbar, state->standard_visible ? 34 : 1);
}

RECT standard_button_rect(HWND toolbar, int target) {
    const int button = scale(toolbar, 27);
    const int gap = scale(toolbar, 2);
    const int group_gap = scale(toolbar, 7);
    int x = scale(toolbar, 4);
    int previous_group = kStandardButtons[0].group;
    for (int index = 0; index <= target; ++index) {
        if (index > 0 && kStandardButtons[index].group != previous_group) {
            x += group_gap;
        }
        if (index == target) {
            return (RECT){x, scale(toolbar, 2), x + button,
                        scale(toolbar, 2) + button};
        }
        x += button + gap;
        previous_group = kStandardButtons[index].group;
    }
    return (RECT){};
}

int format_buttons_start(HWND toolbar) {
    return scale(toolbar, 4 + 118 + 4 + 146 + 4 + 52 + 8);
}

RECT format_button_rect(HWND toolbar, const ToolbarState* state, int target) {
    const int button = scale(toolbar, 27);
    const int gap = scale(toolbar, 2);
    const int group_gap = scale(toolbar, 7);
    int x = format_buttons_start(toolbar);
    int previous_group = kFormatButtons[0].group;
    for (int index = 0; index <= target; ++index) {
        if (index > 0 && kFormatButtons[index].group != previous_group) {
            x += group_gap;
        }
        if (index == target) {
            const int top = formatting_row_top(toolbar, state);
            return (RECT){x, top, x + button, top + button};
        }
        x += button + gap;
        previous_group = kFormatButtons[index].group;
    }
    return (RECT){};
}

int standard_buttons_end(HWND toolbar) {
    const RECT last = standard_button_rect(
        toolbar, kStandardButtonCount - 1);
    return last.right + scale(toolbar, 8);
}

void draw_classic_edge(HDC dc, RECT rect, bool sunken) {
    const HPEN top_outer = CreatePen(PS_SOLID, 1,
        sunken ? kButtonDarkShadow : kButtonHighlight);
    const HPEN top_inner = CreatePen(PS_SOLID, 1,
        sunken ? kButtonShadow : kButtonFace);
    const HPEN bottom_outer = CreatePen(PS_SOLID, 1,
        sunken ? kButtonHighlight : kButtonDarkShadow);
    const HPEN bottom_inner = CreatePen(PS_SOLID, 1,
        sunken ? kButtonFace : kButtonShadow);
    HPEN old = ((HPEN) SelectObject(dc, top_outer));
    MoveToEx(dc, rect.left, rect.bottom - 1, NULL);
    LineTo(dc, rect.left, rect.top);
    LineTo(dc, rect.right - 1, rect.top);
    SelectObject(dc, top_inner);
    MoveToEx(dc, rect.left + 1, rect.bottom - 2, NULL);
    LineTo(dc, rect.left + 1, rect.top + 1);
    LineTo(dc, rect.right - 2, rect.top + 1);
    SelectObject(dc, bottom_outer);
    MoveToEx(dc, rect.left, rect.bottom - 1, NULL);
    LineTo(dc, rect.right - 1, rect.bottom - 1);
    LineTo(dc, rect.right - 1, rect.top - 1);
    SelectObject(dc, bottom_inner);
    MoveToEx(dc, rect.left + 1, rect.bottom - 2, NULL);
    LineTo(dc, rect.right - 2, rect.bottom - 2);
    LineTo(dc, rect.right - 2, rect.top);
    SelectObject(dc, old);
    DeleteObject(top_outer);
    DeleteObject(top_inner);
    DeleteObject(bottom_outer);
    DeleteObject(bottom_inner);
}

void draw_standard_glyph(HWND toolbar, HDC dc, HDC sprite_dc,
                         const SpriteButton* spec, RECT rect, bool sunken) {
    const int glyph = scale(toolbar, 20);
    const int offset = sunken ? scale(toolbar, 1) : 0;
    const int x = rect.left + (rect.right - rect.left - glyph) / 2 + offset;
    const int y = rect.top + (rect.bottom - rect.top - glyph) / 2 + offset;
    SetStretchBltMode(dc, COLORONCOLOR);
    StretchBlt(dc, x, y, glyph, glyph, sprite_dc,
               spec->sprite * kSpriteCell, 0, kSpriteCell, kSpriteCell,
               SRCCOPY);
}

void draw_alignment(HDC dc, RECT area, FormatGlyph glyph) {
    const int width = area.right - area.left;
    const int left = area.left + 2;
    const int right = area.right - 2;
    for (int row = 0; row < 4; ++row) {
        const int y = area.top + 2 + row * 4;
        int x1 = left;
        int x2 = right;
        const int short_by = (row % 2 == 0) ? width / 4 : width / 6;
        if (glyph == align_left) {
            x2 -= short_by;
        } else if (glyph == align_center) {
            x1 += short_by / 2;
            x2 -= short_by / 2;
        } else if (glyph == align_right) {
            x1 += short_by;
        } else if (glyph == align_justify && row == 3) {
            x2 -= width / 5;
        }
        MoveToEx(dc, x1, y, NULL);
        LineTo(dc, x2, y);
    }
}

void draw_format_glyph(HWND toolbar, HDC dc, const FormatButton* spec,
                       RECT rect, bool sunken, HDC sprite_dc,
                       COLORREF text_color) {
    const int offset = sunken ? scale(toolbar, 1) : 0;
    RECT area = {rect.left + scale(toolbar, 4) + offset,
              rect.top + scale(toolbar, 4) + offset,
              rect.right - scale(toolbar, 4) + offset,
              rect.bottom - scale(toolbar, 4) + offset};
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, RGB(0, 0, 0));
    HFONT old_font = NULL;
    LOGFONTW logical = {0};
    logical.lfHeight = -(area.bottom - area.top);
    lstrcpyW(logical.lfFaceName, kArialFace);
    if (spec->glyph == bold) {
        logical.lfWeight = FW_BOLD;
    } else if (spec->glyph == italic) {
        logical.lfItalic = TRUE;
    } else if (spec->glyph == underline) {
        logical.lfUnderline = TRUE;
    }
    if (spec->glyph == bold ||
        spec->glyph == italic ||
        spec->glyph == underline ||
        spec->glyph == color) {
        HFONT font = CreateFontIndirectW(&logical);
        old_font = ((HFONT) SelectObject(dc, font));
        DrawTextW(dc, spec->glyph == color ? kFormatGlyphColor :
                  spec->glyph == bold ? kFormatGlyphBold :
                  spec->glyph == italic ? kFormatGlyphItalic :
                  kFormatGlyphUnderline,
                  1, &area, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        SelectObject(dc, old_font);
        DeleteObject(font);
        if (spec->glyph == color) {
            HBRUSH swatch = CreateSolidBrush(text_color);
            RECT swatch_rect = {area.left + 1, area.bottom - 3,
                             area.right - 1, area.bottom};
            FillRect(dc, &swatch_rect, swatch);
            DeleteObject(swatch);
        }
        return;
    }
    if (spec->glyph == align_left ||
        spec->glyph == align_center ||
        spec->glyph == align_right ||
        spec->glyph == align_justify) {
        draw_alignment(dc, area, spec->glyph);
        return;
    }
    if (spec->glyph == numbered ||
        spec->glyph == bullets) {
        for (int row = 0; row < 3; ++row) {
            if (spec->glyph == numbered) {
                WCHAR number[2] = {
                    (WCHAR) (OPUSW("1")[0] + row), 0};
                TextOutW(dc, area.left, area.top + row * 5 - 1, number, 1);
            } else {
                HBRUSH old_brush = ((HBRUSH) SelectObject(
                    dc, GetStockObject(BLACK_BRUSH)));
                Ellipse(dc, area.left + 1, area.top + row * 5 + 1,
                        area.left + 4, area.top + row * 5 + 4);
                SelectObject(dc, old_brush);
            }
            MoveToEx(dc, area.left + 6, area.top + row * 5 + 2, NULL);
            LineTo(dc, area.right, area.top + row * 5 + 2);
        }
        return;
    }
    if (spec->glyph == indent_left ||
        spec->glyph == indent_right) {
        for (int row = 0; row < 3; ++row) {
            MoveToEx(dc, area.left + 6, area.top + 2 + row * 5, NULL);
            LineTo(dc, area.right, area.top + 2 + row * 5);
        }
        const bool right = spec->glyph == indent_right;
        POINT triangle[3] = {{right ? area.left + 1 : area.left + 6, area.top + 6},
                          {right ? area.left + 6 : area.left + 1, area.top + 3},
                          {right ? area.left + 6 : area.left + 1, area.top + 9}};
        HBRUSH brush = ((HBRUSH) GetStockObject(BLACK_BRUSH));
        HBRUSH old_brush = ((HBRUSH) SelectObject(dc, brush));
        Polygon(dc, triangle, 3);
        SelectObject(dc, old_brush);
        return;
    }
    if (spec->glyph == table && sprite_dc != NULL) {
        const int glyph = area.bottom - area.top;
        StretchBlt(dc, area.left, area.top, glyph, glyph, sprite_dc,
                   12 * kSpriteCell, 0, kSpriteCell, kSpriteCell, SRCCOPY);
        return;
    }
    if (spec->glyph == borders) {
        Rectangle(dc, area.left + 1, area.top + 1,
                  area.right - 1, area.bottom - 1);
        const int middle_x = (area.left + area.right) / 2;
        const int middle_y = (area.top + area.bottom) / 2;
        MoveToEx(dc, middle_x, area.top + 1, NULL);
        LineTo(dc, middle_x, area.bottom - 1);
        MoveToEx(dc, area.left + 1, middle_y, NULL);
        LineTo(dc, area.right - 1, middle_y);
    }
}

void paint_toolbar(HWND toolbar, ToolbarState* state) {
    PAINTSTRUCT paint = {0};
    HDC dc = BeginPaint(toolbar, &paint);
    RECT client = {0};
    GetClientRect(toolbar, &client);
    HBRUSH face = CreateSolidBrush(kButtonFace);
    FillRect(dc, &client, face);
    DeleteObject(face);

    if (state->standard_visible && state->formatting_visible) {
        HPEN separator = CreatePen(PS_SOLID, 1, kButtonShadow);
        HPEN highlight = CreatePen(PS_SOLID, 1, kButtonHighlight);
        HPEN old_pen = ((HPEN) SelectObject(dc, separator));
        const int row_line = scale(toolbar, 32);
        MoveToEx(dc, 0, row_line, NULL);
        LineTo(dc, client.right, row_line);
        SelectObject(dc, highlight);
        MoveToEx(dc, 0, row_line + 1, NULL);
        LineTo(dc, client.right, row_line + 1);
        SelectObject(dc, old_pen);
        DeleteObject(separator);
        DeleteObject(highlight);
    }

    HDC sprite_dc = CreateCompatibleDC(dc);
    HBITMAP old_bitmap = state->sprite != NULL ?
        (HBITMAP) SelectObject(sprite_dc, state->sprite) : NULL;
    if (state->standard_visible) {
        for (int index = 0;
             index < ((int) kStandardButtonCount); ++index) {
            RECT rect = standard_button_rect(toolbar, index);
            const bool down =
                (state->pressed.hit && !state->pressed.format &&
                 state->pressed.index == index) || state->standard_latched[index];
            draw_classic_edge(dc, rect, down);
            if (state->sprite != NULL) {
                draw_standard_glyph(toolbar, dc, sprite_dc, &kStandardButtons[index], rect, down);
            }
        }
    }
    if (state->formatting_visible) {
        for (int index = 0;
             index < ((int) kFormatButtonCount); ++index) {
            RECT rect = format_button_rect(toolbar, state, index);
            const bool down =
                (state->pressed.hit && state->pressed.format &&
                 state->pressed.index == index) || state->format_latched[index];
            draw_classic_edge(dc, rect, down);
            draw_format_glyph(toolbar, dc, &kFormatButtons[index], rect, down,
                               state->sprite != NULL ? sprite_dc : NULL,
                               state->text_color);
        }
    }
    if (old_bitmap != NULL) {
        SelectObject(sprite_dc, old_bitmap);
    }
    DeleteDC(sprite_dc);
    EndPaint(toolbar, &paint);
}

HitResult hit_test(HWND toolbar, const ToolbarState* state, POINT point) {
    if (state->standard_visible) {
        for (int index = 0;
             index < ((int) kStandardButtonCount); ++index) {
            RECT rect = standard_button_rect(toolbar, index);
            if (PtInRect(&rect, point)) {
                return (HitResult){true, false, index, kStandardButtons[index].command};
            }
        }
    }
    if (state->formatting_visible) {
        for (int index = 0;
             index < ((int) kFormatButtonCount); ++index) {
            RECT rect = format_button_rect(toolbar, state, index);
            if (PtInRect(&rect, point)) {
                return (HitResult){true, true, index, kFormatButtons[index].command};
            }
        }
    }
    return (HitResult){false, false, -1, 0};
}

bool is_toolbar_descendant(HWND toolbar, HWND candidate) {
    return candidate == toolbar || IsChild(toolbar, candidate) != FALSE;
}

static char* combo_item(HWND combo, int index) {
    const LRESULT length = SendMessageA(combo, CB_GETLBTEXTLEN, index, 0);
    if (length == CB_ERR || length < 0) {
        return NULL;
    }
    char* text = (char*) calloc((size_t) length + 1, 1);
    if (text == NULL) return NULL;
    SendMessageA(combo, CB_GETLBTEXT, index, (LPARAM) text);
    return text;
}

bool combo_contains(HWND combo, const char* value) {
    return SendMessageA(combo, CB_FINDSTRINGEXACT, ((WPARAM) (-1)),
                        ((LPARAM) value)) != CB_ERR;
}

typedef struct ComboEnumeration {
    HWND toolbar;
    HWND* combos;
    size_t combo_count;
    size_t combo_capacity;
} ComboEnumeration;

BOOL CALLBACK collect_original_combos(HWND candidate, LPARAM parameter) {
    ComboEnumeration* enumeration = (ComboEnumeration*) parameter;
    if (is_toolbar_descendant(enumeration->toolbar, candidate)) {
        return TRUE;
    }
    WCHAR class_name[64] = {0};
    GetClassNameW(candidate, class_name,
                  (int) (sizeof(class_name) / sizeof(class_name[0])));
    if (lstrcmpiW(class_name, kComboBoxClass) == 0) {
        hwnd_list_push(&enumeration->combos, &enumeration->combo_count,
                       &enumeration->combo_capacity, candidate);
    }
    return TRUE;
}

void locate_source_combos(HWND toolbar, ToolbarState* state) {
    ComboEnumeration enumeration = {0};
    enumeration.toolbar = toolbar;
    EnumChildWindows(GetParent(toolbar), collect_original_combos,
                     ((LPARAM) &enumeration));
    for (size_t index = 0; index < enumeration.combo_count; ++index) {
        HWND combo = enumeration.combos[index];
        if (combo_contains(combo, "Courier New") &&
            combo_contains(combo, "Arial")) {
            state->source_font = combo;
        } else if (combo_contains(combo, "24") &&
                   combo_contains(combo, "72")) {
            state->source_size = combo;
        } else if (combo_contains(combo, "Normal") ||
                   SendMessageA(combo, CB_GETCOUNT, 0, 0) > 0) {
            state->source_style = combo;
        }
    }
    free(enumeration.combos);
}

static WCHAR* wide_from_ansi(const char* text) {
    if (text == NULL || *text == '\0') {
        return NULL;
    }
    const int count = MultiByteToWideChar(CP_ACP, 0, text, -1, NULL, 0);
    WCHAR* wide = (WCHAR*) calloc((size_t) count, sizeof(WCHAR));
    if (wide == NULL) return NULL;
    MultiByteToWideChar(CP_ACP, 0, text, -1, wide, count);
    return wide;
}

static char* ansi_from_wide(LPCWSTR text) {
    if (text == NULL || *text == 0) {
        return NULL;
    }
    const int count = WideCharToMultiByte(CP_ACP, 0, text, -1,
                                          NULL, 0, NULL, NULL);
    char* ansi = (char*) calloc((size_t) count, 1);
    if (ansi == NULL) return NULL;
    WideCharToMultiByte(CP_ACP, 0, text, -1, ansi, count, NULL, NULL);
    return ansi;
}

bool combo_or_child_has_focus(HWND combo) {
    const HWND focus = GetFocus();
    return focus == combo || (focus != NULL && IsChild(combo, focus));
}

void sync_combo(HWND mirror, HWND source, int* copied_count) {
    if (mirror == NULL || source == NULL || !IsWindow(source)) {
        return;
    }
    const int count = (int) SendMessageA(source, CB_GETCOUNT, 0, 0);
    if (count >= 0 && count != *copied_count) {
        const int mirror_length = GetWindowTextLengthW(mirror);
        WCHAR* mirror_text = (WCHAR*) calloc((size_t) mirror_length + 1,
                                             sizeof(WCHAR));
        if (mirror_text == NULL) return;
        GetWindowTextW(mirror, mirror_text, mirror_length + 1);
        SendMessageW(mirror, CB_RESETCONTENT, 0, 0);
        for (int index = 0; index < count; ++index) {
            char* item_ansi = combo_item(source, index);
            WCHAR* item = wide_from_ansi(item_ansi);
            if (item != NULL) {
                SendMessageW(mirror, CB_ADDSTRING, 0, (LPARAM) item);
            }
            free(item);
            free(item_ansi);
        }
        SetWindowTextW(mirror, mirror_text);
        free(mirror_text);
        *copied_count = count;
    }
    if (!combo_or_child_has_focus(mirror)) {
        const LRESULT selection = SendMessageA(source, CB_GETCURSEL, 0, 0);
        if (selection != CB_ERR && selection < count) {
            SendMessageW(mirror, CB_SETCURSEL, selection, 0);
        } else {
            const int length = GetWindowTextLengthA(source);
            char* text = (char*) calloc((size_t) length + 1, 1);
            if (text == NULL) return;
            GetWindowTextA(source, text, length + 1);
            WCHAR* source_text = wide_from_ansi(text);
            if (source_text != NULL && source_text[0] != 0) {
                SetWindowTextW(mirror, source_text);
            }
            free(source_text);
            free(text);
        }
        SendMessageW(mirror, CB_SETEDITSEL, 0, MAKELPARAM(-1, 0));
    }
}

void sync_mirrors(HWND toolbar, ToolbarState* state) {
    if (state->source_font == NULL || !IsWindow(state->source_font) ||
        state->source_size == NULL || !IsWindow(state->source_size) ||
        state->source_style == NULL || !IsWindow(state->source_style)) {
        locate_source_combos(toolbar, state);
    }
    sync_combo(state->style_combo, state->source_style,
               &state->copied_style_count);
    sync_combo(state->font_combo, state->source_font,
               &state->copied_font_count);
    sync_combo(state->size_combo, state->source_size,
               &state->copied_size_count);
}

BOOL CALLBACK find_document_pane(HWND candidate, LPARAM parameter) {
    WCHAR class_name[64] = {0};
    GetClassNameW(candidate, class_name,
                  (int) (sizeof(class_name) / sizeof(class_name[0])));
    if (lstrcmpiW(class_name, kDocumentPaneClass) == 0) {
        *((HWND*) (parameter)) = candidate;
        return FALSE;
    }
    return TRUE;
}

void restore_document_focus(HWND source) {
    HWND pane = NULL;
    EnumChildWindows(GetAncestor(source, GA_ROOT), find_document_pane,
                     ((LPARAM) &pane));
    if (pane != NULL) {
        SetFocus(pane);
    }
}

void restore_document_focus_from_root(HWND root) {
    HWND pane = NULL;
    EnumChildWindows(root, find_document_pane,
                     ((LPARAM) &pane));
    if (pane != NULL) {
        SetFocus(pane);
    }
}

HBITMAP create_color_menu_swatch(HWND owner, COLORREF color,
                                 bool automatic) {
    HDC screen = GetDC(owner);
    if (screen == NULL) {
        return NULL;
    }
    const int size = scale(owner, 14);
    HBITMAP bitmap = CreateCompatibleBitmap(screen, size, size);
    HDC memory = CreateCompatibleDC(screen);
    HBITMAP previous = bitmap != NULL ?
        (HBITMAP) SelectObject(memory, bitmap) : NULL;
    if (bitmap != NULL) {
        RECT bounds = {0, 0, size, size};
        FillRect(memory, &bounds, GetSysColorBrush(COLOR_MENU));
        RECT swatch = {scale(owner, 2), scale(owner, 2),
                    size - scale(owner, 1), size - scale(owner, 1)};
        HBRUSH fill = CreateSolidBrush(color);
        FillRect(memory, &swatch, fill);
        DeleteObject(fill);
        FrameRect(memory, &swatch,
                  ((HBRUSH) GetStockObject(BLACK_BRUSH)));
        if (automatic) {
            HPEN pen = CreatePen(PS_SOLID, 1, RGB(255, 255, 255));
            HPEN old_pen = ((HPEN) SelectObject(memory, pen));
            MoveToEx(memory, swatch.left + 1, swatch.bottom - 2, NULL);
            LineTo(memory, swatch.right - 1, swatch.top + 1);
            SelectObject(memory, old_pen);
            DeleteObject(pen);
        }
        SelectObject(memory, previous);
    }
    DeleteDC(memory);
    ReleaseDC(owner, screen);
    return bitmap;
}

void show_text_color_palette(HWND app, HWND toolbar) {
    ToolbarState* state = toolbar_state(toolbar);
    if (state == NULL) {
        return;
    }
    typedef struct Choice {
        int index;
        COLORREF color;
        const WCHAR* name;
    } Choice;
    static const Choice choices[] = {
        {0, RGB(0, 0, 0), OPUSW("Automatic")},
        {1, RGB(0, 0, 0), OPUSW("Black")},
        {2, RGB(0, 0, 255), OPUSW("Blue")},
        {3, RGB(0, 255, 255), OPUSW("Cyan")},
        {4, RGB(0, 128, 0), OPUSW("Green")},
        {5, RGB(255, 0, 255), OPUSW("Magenta")},
        {6, RGB(255, 0, 0), OPUSW("Red")},
        {7, RGB(255, 255, 0), OPUSW("Yellow")},
        {8, RGB(255, 255, 255), OPUSW("White")},
    };
    enum { kTextColorChoiceCount = sizeof(choices) / sizeof(choices[0]) };

    HMENU popup = CreatePopupMenu();
    if (popup == NULL) {
        return;
    }
    HBITMAP swatches[kTextColorChoiceCount] = {0};
    for (int index = 0; index < kTextColorChoiceCount; ++index) {
        const Choice* choice = &choices[index];
        const UINT command = kCmdTextColorBase + choice->index;
        AppendMenuW(popup, MF_STRING, command, choice->name);
        HBITMAP swatch = create_color_menu_swatch(
            toolbar, choice->color, choice->index == 0);
        swatches[index] = swatch;
        if (swatch != NULL) {
            SetMenuItemBitmaps(popup, command, MF_BYCOMMAND, swatch, swatch);
        }
        if (choice->index == state->text_color_index) {
            CheckMenuItem(popup, command, MF_BYCOMMAND | MF_CHECKED);
        }
    }
    style_menu_tree(popup);

    RECT anchor = format_button_rect(toolbar, state, 3);
    POINT point = {anchor.left, anchor.bottom};
    ClientToScreen(toolbar, &point);
    const UINT selected = TrackPopupMenu(
        popup, TPM_RETURNCMD | TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RIGHTBUTTON,
        point.x, point.y, 0, app, NULL);
    if (selected >= kCmdTextColorBase &&
        selected < kCmdTextColorBase + kTextColorChoiceCount) {
        const int index = (int) (selected - kCmdTextColorBase);
        HWND pane = NULL;
        EnumChildWindows(app, find_document_pane,
                         ((LPARAM) &pane));
        if (pane != NULL &&
            OpusApplyWin95TextColor(pane, choices[index].index)) {
            state->text_color_index = choices[index].index;
            state->text_color = choices[index].color;
            InvalidateRect(toolbar, NULL, FALSE);
            UpdateWindow(toolbar);
        } else {
            MessageBeep(MB_ICONEXCLAMATION);
        }
    }
    DestroyMenu(popup);
    for (int index = 0; index < kTextColorChoiceCount; ++index) {
        if (swatches[index] != NULL) {
            DeleteObject(swatches[index]);
        }
    }
    restore_document_focus_from_root(app);
}

void forward_combo(HWND mirror, HWND source, int notification,
                   bool* edit_dirty) {
    if (mirror == NULL || source == NULL || !IsWindow(source)) {
        return;
    }
    if (notification == CBN_SELCHANGE || notification == CBN_SELENDOK) {
        *edit_dirty = false;
    }
    WCHAR* wide = NULL;
    const LRESULT mirror_selection = SendMessageW(mirror, CB_GETCURSEL, 0, 0);
    if (mirror_selection != CB_ERR) {
        const LRESULT length = SendMessageW(
            mirror, CB_GETLBTEXTLEN, mirror_selection, 0);
        wide = (WCHAR*) calloc((size_t) length + 1, sizeof(WCHAR));
        if (wide == NULL) return;
        SendMessageW(mirror, CB_GETLBTEXT, mirror_selection,
                     (LPARAM) wide);
    } else {
        const int length = GetWindowTextLengthW(mirror);
        wide = (WCHAR*) calloc((size_t) length + 1, sizeof(WCHAR));
        if (wide == NULL) return;
        GetWindowTextW(mirror, wide, length + 1);
    }
    char* text = ansi_from_wide(wide);
    free(wide);
    if (text == NULL) return;
    const LRESULT selection = SendMessageA(
        source, CB_FINDSTRINGEXACT, (WPARAM) -1, (LPARAM) text);
    if (selection != CB_ERR) {
        SendMessageA(source, CB_SETCURSEL, selection, 0);
    }
    SetWindowTextA(source, text);
    free(text);
    const HWND parent = GetParent(source);
    const int control_id = GetDlgCtrlID(source);
    if (notification == CBN_SELCHANGE) {
        SendMessageW(parent, WM_COMMAND,
                     MAKEWPARAM(control_id, CBN_SELCHANGE),
                     ((LPARAM) source));
    } else if (notification == CBN_SELENDOK) {
        SendMessageW(parent, WM_COMMAND,
                     MAKEWPARAM(control_id, CBN_SELENDOK),
                     ((LPARAM) source));
        restore_document_focus(source);
    } else if (notification == CBN_EDITCHANGE) {
        // A dropdown selection can emit EDITCHANGE too; only free-form text (no
        // selected list item) needs the legacy control's focus-loss commit
        // path.
        *edit_dirty = SendMessageW(mirror, CB_GETCURSEL, 0, 0) == CB_ERR;
        SendMessageW(parent, WM_COMMAND,
                     MAKEWPARAM(control_id, CBN_EDITCHANGE),
                     ((LPARAM) source));
    } else if (notification == CBN_DROPDOWN) {
        SendMessageW(parent, WM_COMMAND,
                     MAKEWPARAM(control_id, CBN_DROPDOWN),
                     ((LPARAM) source));
    } else if (notification == CBN_KILLFOCUS && *edit_dirty) {
        SendMessageW(parent, WM_COMMAND,
                     MAKEWPARAM(control_id, CBN_KILLFOCUS),
                     ((LPARAM) source));
        *edit_dirty = false;
    }
}

void position_combos(HWND toolbar, ToolbarState* state) {
    const int y = formatting_row_top(toolbar, state) + scale(toolbar, 1);

    // For CBS_DROPDOWN the window height also reserves the popup list. The
    // visible, closed control still uses its system edit-field height.
    const int height = scale(toolbar, 220);
    const int x_style = scale(toolbar, 4);
    const int width_style = scale(toolbar, 118);
    const int x_font = scale(toolbar, 126);
    const int width_font = scale(toolbar, 146);
    const int x_size = scale(toolbar, 276);
    const int width_size = scale(toolbar, 52);
    ShowWindow(state->style_combo,
               state->formatting_visible ? SW_SHOWNOACTIVATE : SW_HIDE);
    ShowWindow(state->font_combo,
               state->formatting_visible ? SW_SHOWNOACTIVATE : SW_HIDE);
    ShowWindow(state->size_combo,
               state->formatting_visible ? SW_SHOWNOACTIVATE : SW_HIDE);
    MoveWindow(state->style_combo, x_style, y, width_style, height, TRUE);
    MoveWindow(state->font_combo, x_font, y, width_font, height, TRUE);
    MoveWindow(state->size_combo, x_size, y, width_size, height, TRUE);

    if (state->zoom_combo != NULL) {
        ShowWindow(state->zoom_combo,
                   state->standard_visible ? SW_SHOWNOACTIVATE : SW_HIDE);
        MoveWindow(state->zoom_combo, standard_buttons_end(toolbar),
                   scale(toolbar, 3), scale(toolbar, 82), height, TRUE);
    }
}

HWND create_combo(HWND toolbar, UINT id) {
    HWND combo = CreateWindowExW(
        0, kComboBoxWindowClass, NULL,
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL |
            CBS_DROPDOWN | CBS_AUTOHSCROLL,
        0, 0, 10, 200, toolbar,
        ((HMENU) ((UINT_PTR) (id))),
        GetModuleHandleW(NULL), NULL);
    set_window_classic(combo);
    return combo;
}

HWND create_zoom_combo(HWND toolbar) {
    HWND combo = CreateWindowExW(
        0, kComboBoxWindowClass, NULL,
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL |
            CBS_DROPDOWN | CBS_AUTOHSCROLL,
        0, 0, 10, 200, toolbar,
        ((HMENU) ((UINT_PTR) (kComboZoom))),
        GetModuleHandleW(NULL), NULL);
    set_window_classic(combo);
    SendMessageW(combo, CB_ADDSTRING, 0,
                 ((LPARAM) kZoom100));
    SendMessageW(combo, CB_ADDSTRING, 0,
                 ((LPARAM) kZoomPageWidth));
    SendMessageW(combo, CB_ADDSTRING, 0,
                 ((LPARAM) kZoomWholePage));
    SendMessageW(combo, CB_ADDSTRING, 0,
                 ((LPARAM) kZoomPrintPreview));
    SendMessageW(combo, CB_SETCURSEL, 0, 0);
    return combo;
}

void update_zoom_combo(HWND pane, int percent) {
    const HWND app = GetAncestor(pane, GA_ROOT);
    const HWND toolbar = FindWindowExW(app, NULL, kToolbarClass, NULL);
    ToolbarState* state = toolbar_state(toolbar);
    if (state == NULL || state->zoom_combo == NULL) {
        return;
    }
    WCHAR text[16] = {0};
    format_wide_decimal(text, sizeof(text) / sizeof(text[0]), percent,
                        OPUSW("%"));
    SendMessageW(state->zoom_combo, CB_SETCURSEL, (WPARAM) -1, 0);
    SetWindowTextW(state->zoom_combo, text);
}

void update_toolbar_menu_checks(HWND app, const ToolbarState* state) {
    const UINT standard = MF_BYCOMMAND |
        (state->standard_visible ? MF_CHECKED : MF_UNCHECKED);
    const UINT formatting = MF_BYCOMMAND |
        (state->formatting_visible ? MF_CHECKED : MF_UNCHECKED);
    if (g_toolbars_menu != NULL && IsMenu(g_toolbars_menu)) {
        CheckMenuItem(g_toolbars_menu, kCmdToggleStandardToolbar, standard);
        CheckMenuItem(g_toolbars_menu, kCmdToggleFormattingToolbar,
                      formatting);
    }
    HMENU root = GetMenu(app);
    if (root != NULL) {
        CheckMenuItem(root, kCmdToggleStandardToolbar, standard);
        CheckMenuItem(root, kCmdToggleFormattingToolbar, formatting);
    }
}

void request_toolbar_layout(HWND app, HWND toolbar) {
    if (app == NULL || toolbar == NULL) {
        return;
    }
    RECT client = {0};
    GetClientRect(app, &client);
    const int height = toolbar_height(toolbar);
    ShowWindow(toolbar, height > 0 ? SW_SHOWNOACTIVATE : SW_HIDE);
    MoveWindow(toolbar, 0, 0, client.right - client.left, height, TRUE);
    const UINT size_type = IsZoomed(app) ? SIZE_MAXIMIZED : SIZE_RESTORED;
    SendMessageW(app, WM_SIZE, size_type,
                 MAKELPARAM(client.right - client.left,
                            client.bottom - client.top));
    InvalidateRect(app, NULL, TRUE);
    DrawMenuBar(app);
}

void toggle_toolbar_row(HWND app, HWND toolbar, bool standard) {
    ToolbarState* state = toolbar_state(toolbar);
    if (state == NULL) {
        return;
    }
    if (standard) {
        state->standard_visible = !state->standard_visible;
    } else {
        state->formatting_visible = !state->formatting_visible;
    }
    state->pressed = (HitResult){false, false, -1, 0};
    position_combos(toolbar, state);
    update_toolbar_menu_checks(app, state);
    request_toolbar_layout(app, toolbar);
}

void show_toolbar_context_menu(HWND toolbar, POINT screen_point) {
    ToolbarState* state = toolbar_state(toolbar);
    if (state == NULL) {
        return;
    }
    HMENU popup = CreatePopupMenu();
    if (popup == NULL) {
        return;
    }
    AppendMenuW(popup,
                MF_STRING |
                    (state->standard_visible ? MF_CHECKED : MF_UNCHECKED),
                kCmdToggleStandardToolbar, OPUSW("&Standard"));
    AppendMenuW(popup,
                MF_STRING |
                    (state->formatting_visible ? MF_CHECKED : MF_UNCHECKED),
                kCmdToggleFormattingToolbar, OPUSW("&Formatting"));
    AppendMenuW(popup, MF_STRING, bcmRuler, OPUSW("&Ruler"));
    AppendMenuW(popup, MF_STRING, bcmStatusArea, OPUSW("&Status Bar"));
    AppendMenuW(popup, MF_SEPARATOR, 0, NULL);
    AppendMenuW(popup, MF_STRING, bcmCustomize, OPUSW("&Customize..."));
    style_menu_tree(popup);
    const UINT command = TrackPopupMenu(
        popup, TPM_RETURNCMD | TPM_RIGHTBUTTON,
        screen_point.x, screen_point.y, 0, GetParent(toolbar), NULL);
    DestroyMenu(popup);
    if (command != 0) {
        PostMessageW(GetParent(toolbar), WM_COMMAND,
                     MAKEWPARAM(command, 0), 0);
    }
}

WNDPROC original_pane_proc(HWND pane) {
    return ((WNDPROC)
        GetPropW(pane, kOriginalPaneProcProperty));
}

void redraw_vertical_ruler(HWND pane) {
    RECT ruler = {0};
    GetClientRect(pane, &ruler);
    ruler.right = min_int(ruler.right, ruler.left + scale(pane, 22));
    InvalidateRect(pane, &ruler, FALSE);
    UpdateWindow(pane);
}

bool vertical_margin_marker_at(HWND pane, POINT point, bool* top_marker,
                               int* page_offset) {
    RECT client = {0};
    GetClientRect(pane, &client);
    if (point.x < client.left ||
        point.x >= client.left + scale(pane, 22)) {
        return false;
    }

    int page_top = 0;
    int page_bottom = 0;
    int top_margin = 0;
    int bottom_margin = 0;
    int pixels_per_inch = 0;
    if (!OpusGetWin95VerticalRulerMetrics(
            pane, &page_top, &page_bottom, &top_margin, &bottom_margin,
            &pixels_per_inch)) {
        return false;
    }

    const int hit_radius = scale(pane, 7);
    int page_left = 0;
    int continuous_top = 0;
    int page_right = 0;
    int continuous_bottom = 0;
    int page_gap = 0;
    int has_previous = 0;
    int has_next = 0;
    OpusGetWin95ContinuousPageMetrics(
        pane, &page_left, &continuous_top, &page_right,
        &continuous_bottom, &page_gap, &has_previous, &has_next);
    const int page_step = page_bottom - page_top + page_gap;
    const int offsets[3] = {
        has_previous ? -page_step : 0,
        0,
        has_next ? page_step : 0};
    int closest_distance = hit_radius + 1;
    bool closest_top = false;
    int closest_offset = 0;
    for (size_t index = 0; index < 3; ++index) {
        const int offset = offsets[index];
        if ((index == 0 && !has_previous) ||
            (index == 2 && !has_next)) {
            continue;
        }
        const int top_distance = abs(
            point.y - (page_top + offset + top_margin));
        const int bottom_distance = abs(
            point.y - (page_bottom + offset - bottom_margin));
        if (top_distance < closest_distance) {
            closest_distance = top_distance;
            closest_top = true;
            closest_offset = offset;
        }
        if (bottom_distance < closest_distance) {
            closest_distance = bottom_distance;
            closest_top = false;
            closest_offset = offset;
        }
    }
    if (closest_distance > hit_radius) {
        return false;
    }
    *top_marker = closest_top;
    if (page_offset != NULL) {
        *page_offset = closest_offset;
    }
    return true;
}

void update_vertical_margin_drag(HWND pane, int y) {
    if (!g_vertical_ruler_drag.active ||
        g_vertical_ruler_drag.pane != pane) {
        return;
    }

    int page_top = 0;
    int page_bottom = 0;
    int top_margin = 0;
    int bottom_margin = 0;
    int pixels_per_inch = 0;
    if (!OpusGetWin95VerticalRulerMetrics(
            pane, &page_top, &page_bottom, &top_margin, &bottom_margin,
            &pixels_per_inch)) {
        return;
    }

    page_top += g_vertical_ruler_drag.page_offset;
    page_bottom += g_vertical_ruler_drag.page_offset;
    const int minimum_text_height = max_int(
        scale(pane, 12), max_int(1, pixels_per_inch / 4));
    if (g_vertical_ruler_drag.top) {
        const int bottom_y = page_bottom - bottom_margin;
        y = max_int(page_top,
                     min_int(y, bottom_y - minimum_text_height));
    } else {
        const int top_y = page_top + top_margin;
        y = max_int(top_y + minimum_text_height,
                     min_int(y, page_bottom));
    }
    if (y != g_vertical_ruler_drag.preview_y) {
        g_vertical_ruler_drag.preview_y = y;
        redraw_vertical_ruler(pane);
    }
}

void cancel_vertical_margin_drag(HWND pane) {
    if (!g_vertical_ruler_drag.active ||
        g_vertical_ruler_drag.pane != pane) {
        return;
    }
    g_vertical_ruler_drag = (VerticalRulerDragState){0};
    redraw_vertical_ruler(pane);
}

PageSnapshot* find_page_snapshot(HWND pane, int page_index, int width,
                                 int height, int zoom_percent) {
    for (size_t index = 0; index < g_page_snapshots.size; ++index) {
        PageSnapshot* snapshot = &g_page_snapshots.data[index];
        if (snapshot->pane == pane && snapshot->page_index == page_index &&
            snapshot->width == width && snapshot->height == height &&
            snapshot->zoom_percent == zoom_percent) {
            snapshot->last_used = GetTickCount64();
            return snapshot;
        }
    }
    return NULL;
}

void prune_page_snapshots(HWND pane) {
    size_t pane_count = 0;
    for (size_t index = 0; index < g_page_snapshots.size; ++index) {
        if (g_page_snapshots.data[index].pane == pane) {
            ++pane_count;
        }
    }
    while (pane_count > 4) {
        size_t oldest = g_page_snapshots.size;
        for (size_t index = 0; index < g_page_snapshots.size; ++index) {
            PageSnapshot* snapshot = &g_page_snapshots.data[index];
            if (snapshot->pane == pane &&
                (oldest == g_page_snapshots.size ||
                 snapshot->last_used < g_page_snapshots.data[oldest].last_used)) {
                oldest = index;
            }
        }
        if (oldest == g_page_snapshots.size) {
            break;
        }
        page_snapshot_delete(oldest);
        --pane_count;
    }
}

void clear_page_snapshots(HWND pane) {
    for (size_t index = 0; index < g_page_snapshots.size;) {
        if (g_page_snapshots.data[index].pane == pane) {
            page_snapshot_delete(index);
        } else {
            ++index;
        }
    }
}

void update_current_page_snapshot(HWND pane, HDC dc) {
    int page_left = 0;
    int page_top = 0;
    int page_right = 0;
    int page_bottom = 0;
    int page_gap = 0;
    int has_previous = 0;
    int has_next = 0;
    if (!OpusGetWin95ContinuousPageMetrics(
            pane, &page_left, &page_top, &page_right, &page_bottom,
            &page_gap, &has_previous, &has_next)) {
        return;
    }
    const int page_index = OpusGetWin95CurrentPageIndex(pane);
    const int zoom_percent = OpusGetWin95ZoomPercent(pane);
    const int width = page_right - page_left;
    const int height = page_bottom - page_top;
    if (page_index < 0 || width <= 0 || height <= 0) {
        return;
    }

    PageSnapshot* snapshot = find_page_snapshot(
        pane, page_index, width, height, zoom_percent);
    if (snapshot == NULL) {
        PageSnapshot created = {0};
        created.pane = pane;
        created.page_index = page_index;
        created.zoom_percent = zoom_percent;
        created.width = width;
        created.height = height;
        created.bitmap = CreateCompatibleBitmap(dc, width, height);
        created.last_used = GetTickCount64();
        if (created.bitmap == NULL) {
            return;
        }
        HDC memory = CreateCompatibleDC(dc);
        HBITMAP previous = ((HBITMAP)
            SelectObject(memory, created.bitmap));
        RECT background = {0, 0, width, height};
        FillRect(memory, &background,
                 ((HBRUSH) GetStockObject(WHITE_BRUSH)));
        SelectObject(memory, previous);
        DeleteDC(memory);
        if (!page_snapshot_push(created)) {
            DeleteObject(created.bitmap);
            return;
        }
        snapshot = &g_page_snapshots.data[g_page_snapshots.size - 1];
        prune_page_snapshots(pane);
        snapshot = find_page_snapshot(
            pane, page_index, width, height, zoom_percent);
    }
    if (snapshot == NULL || snapshot->bitmap == NULL) {
        return;
    }

    RECT client = {0};
    RECT page = {page_left, page_top, page_right, page_bottom};
    RECT visible = {0};
    GetClientRect(pane, &client);
    if (!IntersectRect(&visible, &page, &client)) {
        return;
    }
    HDC memory = CreateCompatibleDC(dc);
    HBITMAP previous = ((HBITMAP)
        SelectObject(memory, snapshot->bitmap));
    BitBlt(memory, visible.left - page_left, visible.top - page_top,
           visible.right - visible.left, visible.bottom - visible.top,
           dc, visible.left, visible.top, SRCCOPY);
    SelectObject(memory, previous);
    DeleteDC(memory);
}

void draw_neighbor_sheet(HDC dc, const RECT* sheet, const RECT* client,
                         const PageSnapshot* snapshot) {
    RECT visible = {0};
    if (!IntersectRect(&visible, sheet, client)) {
        return;
    }

    RECT shadow = *sheet;
    OffsetRect(&shadow, 3, 3);
    RECT visible_shadow = {0};
    if (IntersectRect(&visible_shadow, &shadow, client)) {
        HBRUSH shadow_brush = CreateSolidBrush(RGB(96, 96, 96));
        FillRect(dc, &visible_shadow, shadow_brush);
        DeleteObject(shadow_brush);
    }

    if (snapshot != NULL && snapshot->bitmap != NULL) {
        HDC memory = CreateCompatibleDC(dc);
        HBITMAP previous = ((HBITMAP)
            SelectObject(memory, snapshot->bitmap));
        BitBlt(dc, visible.left, visible.top,
               visible.right - visible.left, visible.bottom - visible.top,
               memory, visible.left - sheet->left, visible.top - sheet->top,
               SRCCOPY);
        SelectObject(memory, previous);
        DeleteDC(memory);
    } else {
        HBRUSH page_brush = CreateSolidBrush(RGB(255, 255, 255));
        FillRect(dc, &visible, page_brush);
        DeleteObject(page_brush);
    }
    HBRUSH border_brush = CreateSolidBrush(RGB(96, 96, 96));
    FrameRect(dc, sheet, border_brush);
    DeleteObject(border_brush);
}

void draw_continuous_page_workspace(HWND pane, HDC dc) {
    int page_left = 0;
    int page_top = 0;
    int page_right = 0;
    int page_bottom = 0;
    int page_gap = 0;
    int has_previous = 0;
    int has_next = 0;
    if (!OpusGetWin95ContinuousPageMetrics(
            pane, &page_left, &page_top, &page_right, &page_bottom,
            &page_gap, &has_previous, &has_next)) {
        return;
    }

    RECT client = {0};
    GetClientRect(pane, &client);
    const int page_height = page_bottom - page_top;
    if (page_right <= page_left || page_height <= 0) {
        return;
    }

    /* The original Page View eraser assumes that the page consumes the window
     * width. In the continuous-page shell that can leave white invalidation
     * bands across the gray inter-page gap. Restore the workspace after the
     * legacy paint, excluding only real sheets, before drawing cached neighbor
     * pages.
     */
    const int saved = SaveDC(dc);
    IntersectClipRect(dc, client.left, client.top,
                     client.right, client.bottom);
    const int page_step = page_height + page_gap;
    const int offsets[] = {
        0,
        has_previous ? -page_step : 0,
        has_next ? page_step : 0};
    const int first = has_previous ? 0 : 1;
    const int last = has_next ? 3 : 2;
    for (int index = first; index < last; ++index) {
        const RECT sheet = {
            page_left, page_top + offsets[index],
            page_right, page_bottom + offsets[index]};
        ExcludeClipRect(dc, sheet.left, sheet.top,
                       sheet.right, sheet.bottom);
    }
    FillRect(dc, &client, GetSysColorBrush(COLOR_APPWORKSPACE));
    RestoreDC(dc, saved);

    /* Rebuild the current sheet's shadow after cleaning the surrounding
     * workspace. Clip the shadow away from the page so it cannot cover document
     * pixels that the legacy formatter just drew.
     */
    const RECT current = {page_left, page_top, page_right, page_bottom};
    RECT shadow = current;
    OffsetRect(&shadow, 3, 3);
    const int shadow_saved = SaveDC(dc);
    IntersectClipRect(dc, shadow.left, shadow.top,
                     shadow.right, shadow.bottom);
    ExcludeClipRect(dc, current.left, current.top,
                    current.right, current.bottom);
    HBRUSH shadow_brush = CreateSolidBrush(RGB(96, 96, 96));
    FillRect(dc, &shadow, shadow_brush);
    DeleteObject(shadow_brush);
    RestoreDC(dc, shadow_saved);
    HBRUSH border_brush = CreateSolidBrush(RGB(96, 96, 96));
    FrameRect(dc, &current, border_brush);
    DeleteObject(border_brush);
}

void draw_continuous_page_neighbors(HWND pane, HDC dc) {
    int page_left = 0;
    int page_top = 0;
    int page_right = 0;
    int page_bottom = 0;
    int page_gap = 0;
    int has_previous = 0;
    int has_next = 0;
    if (!OpusGetWin95ContinuousPageMetrics(
            pane, &page_left, &page_top, &page_right, &page_bottom,
            &page_gap, &has_previous, &has_next)) {
        return;
    }

    RECT client = {0};
    GetClientRect(pane, &client);
    const int page_height = page_bottom - page_top;
    if (page_right <= page_left || page_height <= 0) {
        return;
    }
    const int page_step = page_height + page_gap;
    const int page_index = OpusGetWin95CurrentPageIndex(pane);
    const int zoom_percent = OpusGetWin95ZoomPercent(pane);
    if (has_previous) {
        RECT previous = {page_left, page_top - page_step,
                      page_right, page_bottom - page_step};
        draw_neighbor_sheet(
            dc, &previous, &client,
            find_page_snapshot(pane, page_index - 1,
                               page_right - page_left, page_height,
                               zoom_percent));
    }
    if (has_next) {
        RECT next = {page_left, page_top + page_step,
                  page_right, page_bottom + page_step};
        draw_neighbor_sheet(
            dc, &next, &client,
            find_page_snapshot(pane, page_index + 1,
                               page_right - page_left, page_height,
                               zoom_percent));
    }
}

void draw_vertical_ruler(HWND pane, HDC dc) {
    RECT client = {0};
    GetClientRect(pane, &client);
    const int width = scale(pane, 22);
    RECT ruler = {client.left, client.top,
               min_int(client.right, client.left + width), client.bottom};
    HBRUSH outside_brush = CreateSolidBrush(kButtonFace);
    FillRect(dc, &ruler, outside_brush);
    DeleteObject(outside_brush);

    int page_top = 0;
    int page_bottom = 0;
    int top_margin = 0;
    int bottom_margin = 0;
    int pixels_per_inch = 0;
    if (!OpusGetWin95VerticalRulerMetrics(
            pane, &page_top, &page_bottom, &top_margin, &bottom_margin,
            &pixels_per_inch) || pixels_per_inch <= 0) {
        return;
    }

    int page_left = 0;
    int continuous_top = 0;
    int page_right = 0;
    int continuous_bottom = 0;
    int page_gap = 0;
    int has_previous = 0;
    int has_next = 0;
    if (!OpusGetWin95ContinuousPageMetrics(
            pane, &page_left, &continuous_top, &page_right,
            &continuous_bottom, &page_gap, &has_previous, &has_next)) {
        page_gap = 0;
        has_previous = 0;
        has_next = 0;
    }

    int displayed_top_margin = top_margin;
    int displayed_bottom_margin = bottom_margin;
    if (g_vertical_ruler_drag.active &&
        g_vertical_ruler_drag.pane == pane) {
        if (g_vertical_ruler_drag.top) {
            displayed_top_margin = g_vertical_ruler_drag.preview_y -
                (page_top + g_vertical_ruler_drag.page_offset);
        } else {
            displayed_bottom_margin =
                page_bottom + g_vertical_ruler_drag.page_offset -
                g_vertical_ruler_drag.preview_y;
        }
    }

    const int page_step = page_bottom - page_top + page_gap;
    const int offsets[3] = {
        has_previous ? -page_step : 0,
        0,
        has_next ? page_step : 0};
    HBRUSH active_brush = CreateSolidBrush(RGB(255, 255, 255));
    for (size_t index = 0; index < 3; ++index) {
        if ((index == 0 && !has_previous) ||
            (index == 2 && !has_next)) {
            continue;
        }
        const int instance_top = page_top + offsets[index];
        const int instance_bottom = page_bottom + offsets[index];
        RECT writable = {ruler.left,
                      instance_top + displayed_top_margin,
                      ruler.right,
                      instance_bottom - displayed_bottom_margin};
        writable.top = max_int(ruler.top, writable.top);
        writable.bottom = min_int(ruler.bottom, writable.bottom);
        if (writable.bottom > writable.top) {
            FillRect(dc, &writable, active_brush);
            DrawEdge(dc, &writable, EDGE_SUNKEN, BF_TOP | BF_BOTTOM);
        }
    }
    DeleteObject(active_brush);
    DrawEdge(dc, &ruler, EDGE_RAISED, BF_LEFT | BF_RIGHT);

    HPEN active_pen = CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
    HPEN subdued_pen = CreatePen(PS_SOLID, 1, RGB(96, 96, 96));
    HPEN previous_pen = ((HPEN) SelectObject(dc, active_pen));
    HFONT previous_font = ((HFONT)
        SelectObject(dc, GetStockObject(DEFAULT_GUI_FONT)));
    const int previous_bk = SetBkMode(dc, TRANSPARENT);
    const COLORREF previous_text = SetTextColor(dc, RGB(0, 0, 0));
    const int eighth = max_int(1, pixels_per_inch / 8);
    for (size_t index = 0; index < 3; ++index) {
        if ((index == 0 && !has_previous) ||
            (index == 2 && !has_next)) {
            continue;
        }
        const int instance_top = page_top + offsets[index];
        const int instance_bottom = page_bottom + offsets[index];
        const int active_top = instance_top + displayed_top_margin;
        const int active_bottom = instance_bottom - displayed_bottom_margin;
        int tick_index = 0;
        for (int y = instance_top; y < instance_bottom && y < ruler.bottom;
             y += eighth, ++tick_index) {
            if (y < ruler.top) {
                continue;
            }
            const bool writable_tick =
                y >= active_top && y <= active_bottom;
            SelectObject(dc, writable_tick ? active_pen : subdued_pen);
            int length = scale(pane, 3);
            if (tick_index % 8 == 0) {
                length = scale(pane, 9);
            } else if (tick_index % 4 == 0) {
                length = scale(pane, 7);
            } else if (tick_index % 2 == 0) {
                length = scale(pane, 5);
            }
            MoveToEx(dc, ruler.right - scale(pane, 2) - length, y, NULL);
            LineTo(dc, ruler.right - scale(pane, 2), y);
            if (tick_index > 0 && tick_index % 8 == 0) {
                WCHAR label[16] = {0};
                const int label_length = format_wide_decimal(
                    label, sizeof(label) / sizeof(label[0]), tick_index / 8,
                    NULL);
                SetTextColor(dc, writable_tick ? RGB(0, 0, 0) :
                                                 RGB(80, 80, 80));
                TextOutW(dc, ruler.left + scale(pane, 3),
                         y - scale(pane, 6), label, label_length);
            }
        }
    }

    SelectObject(dc, active_pen);
    SetTextColor(dc, RGB(0, 0, 0));
    const int marker_x = ruler.right - scale(pane, 2);
    const int marker_half = scale(pane, 4);
    for (size_t index = 0; index < 3; ++index) {
        if ((index == 0 && !has_previous) ||
            (index == 2 && !has_next)) {
            continue;
        }
        draw_vertical_margin_marker(dc, &ruler, marker_x, marker_half,
                                    page_top + offsets[index] +
                                        displayed_top_margin);
        draw_vertical_margin_marker(dc, &ruler, marker_x, marker_half,
                                    page_bottom + offsets[index] -
                                        displayed_bottom_margin);
    }

    SetTextColor(dc, previous_text);
    SetBkMode(dc, previous_bk);
    SelectObject(dc, previous_font);
    SelectObject(dc, previous_pen);
    DeleteObject(active_pen);
    DeleteObject(subdued_pen);
}

bool horizontal_margin_boundary_at(HWND overlay, POINT point,
                                   bool* left_boundary) {
    HWND ruler = GetParent(overlay);
    RECT client = {0};
    GetClientRect(overlay, &client);

    /* Keep the lower marker lane transparent so native indent/tab dragging
     * continues to use the original ruler implementation.
     */
    if (point.y < client.top ||
        point.y >= client.bottom - scale(ruler, 7)) {
        return false;
    }

    int zero = 0;
    int active_left = 0;
    int active_right = 0;
    int pixels_per_inch = 0;
    int left_indent = 0;
    int first_indent = 0;
    int right_indent = 0;
    int default_tab = 0;
    int tab_count = 0;
    int tab_positions[1] = {0};
    unsigned char tab_types[1] = {0};
    if (!OpusGetWin95HorizontalRulerMetrics(
            ruler, &zero, &active_left, &active_right, &pixels_per_inch,
            &left_indent, &first_indent, &right_indent, &default_tab,
            &tab_count, tab_positions, tab_types, 0)) {
        return false;
    }
    const int hit_radius = scale(ruler, 5);
    const int left_distance = abs(point.x - active_left);
    const int right_distance = abs(point.x - active_right);
    if (left_distance > hit_radius && right_distance > hit_radius) {
        return false;
    }
    *left_boundary = left_distance <= right_distance;
    return true;
}

void redraw_horizontal_ruler_overlay(HWND overlay) {
    InvalidateRect(overlay, NULL, FALSE);
    UpdateWindow(overlay);
}

void update_horizontal_margin_drag(HWND overlay, int x) {
    if (!g_horizontal_ruler_drag.active ||
        g_horizontal_ruler_drag.overlay != overlay) {
        return;
    }
    HWND ruler = g_horizontal_ruler_drag.ruler;
    int zero = 0;
    int active_left = 0;
    int active_right = 0;
    int pixels_per_inch = 0;
    int left_indent = 0;
    int first_indent = 0;
    int right_indent = 0;
    int default_tab = 0;
    int tab_count = 0;
    int left_margin = 0;
    int right_margin = 0;
    int tab_positions[1] = {0};
    unsigned char tab_types[1] = {0};
    if (!OpusGetWin95HorizontalRulerMetrics(
            ruler, &zero, &active_left, &active_right, &pixels_per_inch,
            &left_indent, &first_indent, &right_indent, &default_tab,
            &tab_count, tab_positions, tab_types, 0) ||
        !OpusGetWin95HorizontalPageMargins(
            ruler, &left_margin, &right_margin)) {
        return;
    }
    const int minimum_text_width = max_int(
        scale(ruler, 12), max_int(1, pixels_per_inch / 4));
    if (g_horizontal_ruler_drag.left) {
        x = max_int(active_left - left_margin,
                     min_int(x, active_right - minimum_text_width));
    } else {
        x = max_int(active_left + minimum_text_width,
                     min_int(x, active_right + right_margin));
    }
    if (x != g_horizontal_ruler_drag.preview_x) {
        g_horizontal_ruler_drag.preview_x = x;
        redraw_horizontal_ruler_overlay(overlay);
    }
}

void cancel_horizontal_margin_drag(HWND overlay) {
    if (!g_horizontal_ruler_drag.active ||
        g_horizontal_ruler_drag.overlay != overlay) {
        return;
    }
    g_horizontal_ruler_drag = (HorizontalRulerDragState){0};
    redraw_horizontal_ruler_overlay(overlay);
}

void draw_horizontal_ruler(HWND ruler, HDC dc) {
    RECT client = {0};
    GetClientRect(ruler, &client);
    if (client.right <= client.left || client.bottom <= client.top) {
        return;
    }

    int zero = 0;
    int active_left = 0;
    int active_right = 0;
    int pixels_per_inch = 0;
    int left_indent = 0;
    int first_indent = 0;
    int right_indent = 0;
    int default_tab = 0;
    int tab_count = 0;
    int tab_positions[32] = {0};
    unsigned char tab_types[32] = {0};
    if (!OpusGetWin95HorizontalRulerMetrics(
            ruler, &zero, &active_left, &active_right, &pixels_per_inch,
            &left_indent, &first_indent, &right_indent, &default_tab,
            &tab_count, tab_positions, tab_types,
            (int) (sizeof(tab_positions) / sizeof(tab_positions[0]))) ||
        pixels_per_inch <= 0) {
        return;
    }

    if (g_horizontal_ruler_drag.active &&
        g_horizontal_ruler_drag.ruler == ruler) {
        const int delta = g_horizontal_ruler_drag.preview_x -
                          g_horizontal_ruler_drag.origin_x;
        if (g_horizontal_ruler_drag.left) {
            active_left = g_horizontal_ruler_drag.preview_x;
            zero += delta;
            left_indent += delta;
            first_indent += delta;
            for (int index = 0; index < tab_count; ++index) {
                tab_positions[index] += delta;
            }
        } else {
            active_right = g_horizontal_ruler_drag.preview_x;
            right_indent += delta;
        }
    }

    active_left = max_int((int) client.left,
                          min_int(active_left, (int) client.right));
    active_right = max_int((int) client.left,
                           min_int(active_right, (int) client.right));
    HBRUSH outside_brush = CreateSolidBrush(kButtonFace);
    FillRect(dc, &client, outside_brush);
    DeleteObject(outside_brush);

    RECT active = {active_left, client.top, active_right, client.bottom};
    if (active.right > active.left) {
        HBRUSH active_brush = CreateSolidBrush(RGB(255, 255, 255));
        FillRect(dc, &active, active_brush);
        DeleteObject(active_brush);
        DrawEdge(dc, &active, EDGE_SUNKEN, BF_LEFT | BF_RIGHT);
    }
    DrawEdge(dc, &client, EDGE_RAISED, BF_TOP | BF_BOTTOM);

    HPEN active_pen = CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
    HPEN subdued_pen = CreatePen(PS_SOLID, 1, RGB(96, 96, 96));
    HPEN previous_pen = ((HPEN) SelectObject(dc, active_pen));
    HFONT previous_font = ((HFONT)
        SelectObject(dc, GetStockObject(DEFAULT_GUI_FONT)));
    const int previous_bk = SetBkMode(dc, TRANSPARENT);
    const COLORREF previous_text = SetTextColor(dc, RGB(0, 0, 0));

    const int scale_line = max_int(client.top + 8, client.bottom - 8);
    MoveToEx(dc, client.left, scale_line, NULL);
    LineTo(dc, client.right, scale_line);
    const int eighth = max_int(1, pixels_per_inch / 8);
    int first_tick = (client.left - zero) / eighth;
    if (zero + first_tick * eighth > client.left) {
        --first_tick;
    }
    for (int tick = first_tick;; ++tick) {
        const int x = zero + tick * eighth;
        if (x > client.right) {
            break;
        }
        if (x < client.left) {
            continue;
        }
        const bool writable = x >= active_left && x <= active_right;
        SelectObject(dc, writable ? active_pen : subdued_pen);
        int length = 2;
        if (tick % 8 == 0) {
            length = 7;
        } else if (tick % 4 == 0) {
            length = 5;
        } else if (tick % 2 == 0) {
            length = 3;
        }
        MoveToEx(dc, x, scale_line, NULL);
        LineTo(dc, x, scale_line - length);
        if (tick % 8 == 0) {
            WCHAR label[16] = {0};
            const int label_length = format_wide_decimal(
                label, sizeof(label) / sizeof(label[0]), tick / 8, NULL);
            SetTextColor(dc, writable ? RGB(0, 0, 0) : RGB(80, 80, 80));
            TextOutW(dc, x + 3, client.top - 1, label, label_length);
        }
    }

    SelectObject(dc, active_pen);
    SetTextColor(dc, RGB(0, 0, 0));
    if (default_tab > 0) {
        for (int x = active_left + default_tab;
             x < active_right; x += default_tab) {
            Rectangle(dc, x - 1, client.bottom - 4,
                      x + 2, client.bottom - 2);
        }
    }

    for (int index = 0; index < tab_count; ++index) {
        const int x = tab_positions[index];
        const int top = scale_line + 1;
        const int bottom = client.bottom - 2;
        MoveToEx(dc, x, top, NULL);
        LineTo(dc, x, bottom);
        switch (tab_types[index] & 7) {
        case 0:
            LineTo(dc, x + 5, bottom);
            break;
        case 1:
            MoveToEx(dc, x - 4, bottom, NULL);
            LineTo(dc, x + 5, bottom);
            break;
        case 2:
            LineTo(dc, x - 5, bottom);
            break;
        default:
            MoveToEx(dc, x - 3, bottom, NULL);
            LineTo(dc, x + 2, bottom);
            SetPixel(dc, x + 4, bottom - 1, RGB(0, 0, 0));
            break;
        }
    }

    HBRUSH marker_brush = CreateSolidBrush(RGB(255, 255, 255));
    HBRUSH previous_brush = ((HBRUSH)
        SelectObject(dc, marker_brush));
    const int marker_top = scale_line + 1;
    const int marker_bottom = client.bottom - 1;
    draw_down_triangle(dc, first_indent, marker_top, marker_bottom);
    draw_up_triangle(dc, left_indent, marker_top, marker_bottom, true);
    draw_up_triangle(dc, right_indent, marker_top, marker_bottom, false);
    SelectObject(dc, previous_brush);
    DeleteObject(marker_brush);

    SetTextColor(dc, previous_text);
    SetBkMode(dc, previous_bk);
    SelectObject(dc, previous_font);
    SelectObject(dc, previous_pen);
    DeleteObject(active_pen);
    DeleteObject(subdued_pen);
}

LRESULT CALLBACK ruler_overlay_proc(HWND overlay, UINT message,
                                    WPARAM w_param, LPARAM l_param) {
    switch (message) {
    case WM_NCHITTEST: {
        if (g_horizontal_ruler_drag.active &&
            g_horizontal_ruler_drag.overlay == overlay) {
            return HTCLIENT;
        }
        POINT point = {GET_X_LPARAM(l_param), GET_Y_LPARAM(l_param)};
        ScreenToClient(overlay, &point);
        bool left_boundary = false;
        return horizontal_margin_boundary_at(
                   overlay, point, &left_boundary) ?
                   HTCLIENT : HTTRANSPARENT;
    }
    case WM_MOUSEACTIVATE:
        return MA_NOACTIVATE;
    case WM_SETCURSOR:
        SetCursor(LoadCursorW(NULL, MAKEINTRESOURCEW(32644)));
        return TRUE;
    case WM_LBUTTONDOWN: {
        POINT point = {GET_X_LPARAM(l_param), GET_Y_LPARAM(l_param)};
        bool left_boundary = false;
        if (!horizontal_margin_boundary_at(
                overlay, point, &left_boundary)) {
            return 0;
        }
        HWND ruler = GetParent(overlay);
        int zero = 0;
        int active_left = 0;
        int active_right = 0;
        int pixels_per_inch = 0;
        int left_indent = 0;
        int first_indent = 0;
        int right_indent = 0;
        int default_tab = 0;
        int tab_count = 0;
        int tab_positions[1] = {0};
        unsigned char tab_types[1] = {0};
        if (OpusGetWin95HorizontalRulerMetrics(
                ruler, &zero, &active_left, &active_right,
                &pixels_per_inch, &left_indent, &first_indent,
                &right_indent, &default_tab, &tab_count,
                tab_positions, tab_types, 0)) {
            g_horizontal_ruler_drag.overlay = overlay;
            g_horizontal_ruler_drag.ruler = ruler;
            g_horizontal_ruler_drag.active = true;
            g_horizontal_ruler_drag.left = left_boundary;
            g_horizontal_ruler_drag.origin_x = left_boundary ?
                active_left : active_right;
            g_horizontal_ruler_drag.preview_x =
                g_horizontal_ruler_drag.origin_x;
            SetCapture(overlay);
            SetCursor(LoadCursorW(NULL, MAKEINTRESOURCEW(32644)));
        }
        return 0;
    }
    case WM_MOUSEMOVE:
        if (g_horizontal_ruler_drag.active &&
            g_horizontal_ruler_drag.overlay == overlay) {
            update_horizontal_margin_drag(overlay, GET_X_LPARAM(l_param));
            SetCursor(LoadCursorW(NULL, MAKEINTRESOURCEW(32644)));
            return 0;
        }
        break;
    case WM_LBUTTONUP:
        if (g_horizontal_ruler_drag.active &&
            g_horizontal_ruler_drag.overlay == overlay) {
            update_horizontal_margin_drag(overlay, GET_X_LPARAM(l_param));
            const HWND ruler = g_horizontal_ruler_drag.ruler;
            const bool left_boundary = g_horizontal_ruler_drag.left;
            const int delta = g_horizontal_ruler_drag.preview_x -
                              g_horizontal_ruler_drag.origin_x;
            g_horizontal_ruler_drag = (HorizontalRulerDragState){0};
            if (GetCapture() == overlay) {
                ReleaseCapture();
            }
            const bool changed = OpusAdjustWin95HorizontalMargin(
                ruler, left_boundary ? 1 : 0, delta) != 0;
            if (!changed) {
                MessageBeep(MB_ICONEXCLAMATION);
            } else {
                HWND pane = NULL;
                EnumChildWindows(GetAncestor(ruler, GA_ROOT),
                                 find_document_pane,
                                 ((LPARAM) &pane));
                if (pane != NULL) {
                    clear_page_snapshots(pane);
                }
            }
            if (IsWindow(overlay)) {
                redraw_horizontal_ruler_overlay(overlay);
            }
            return 0;
        }
        break;
    case WM_CAPTURECHANGED:
        if (g_horizontal_ruler_drag.active &&
            g_horizontal_ruler_drag.overlay == overlay &&
            ((HWND) l_param) != overlay) {
            cancel_horizontal_margin_drag(overlay);
        }
        break;
    case WM_CANCELMODE:
        if (g_horizontal_ruler_drag.active &&
            g_horizontal_ruler_drag.overlay == overlay) {
            g_horizontal_ruler_drag = (HorizontalRulerDragState){0};
            if (GetCapture() == overlay) {
                ReleaseCapture();
            }
            redraw_horizontal_ruler_overlay(overlay);
            return 0;
        }
        break;
    case WM_ERASEBKGND:
        return 1;
    case WM_PAINT: {
        PAINTSTRUCT paint = {0};
        HDC dc = BeginPaint(overlay, &paint);
        if (dc != NULL) {
            draw_horizontal_ruler(GetParent(overlay), dc);
        }
        EndPaint(overlay, &paint);
        return 0;
    }
    }
    return DefWindowProcW(overlay, message, w_param, l_param);
}

void ensure_horizontal_ruler_overlay(HWND ruler) {
    RECT client = {0};
    GetClientRect(ruler, &client);
    HWND overlay = FindWindowExW(ruler, NULL, kRulerOverlayClass, NULL);
    if (overlay == NULL) {
        const LONG_PTR style = GetWindowLongPtrW(ruler, GWL_STYLE);
        if ((style & WS_CLIPCHILDREN) == 0) {
            SetWindowLongPtrW(ruler, GWL_STYLE, style | WS_CLIPCHILDREN);
            SetWindowPos(ruler, NULL, 0, 0, 0, 0,
                         SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
                         SWP_NOACTIVATE | SWP_FRAMECHANGED);
        }
        overlay = CreateWindowExW(
            WS_EX_NOACTIVATE,
            kRulerOverlayClass, NULL, WS_CHILD | WS_VISIBLE,
            0, 0, client.right - client.left, client.bottom - client.top,
            ruler, NULL, GetModuleHandleW(NULL), NULL);
    }
    if (overlay != NULL) {
        SetWindowPos(overlay, HWND_TOP, 0, 0,
                     client.right - client.left, client.bottom - client.top,
                     SWP_NOACTIVATE | SWP_SHOWWINDOW);
        InvalidateRect(overlay, NULL, FALSE);
        UpdateWindow(overlay);
    }
}

void show_document_context_menu(HWND pane, POINT screen_point) {
    HMENU popup = CreatePopupMenu();
    if (popup == NULL) {
        return;
    }
    AppendMenuW(popup, MF_STRING, bcmUndo, OPUSW("&Undo"));
    AppendMenuW(popup, MF_SEPARATOR, 0, NULL);
    AppendMenuW(popup, MF_STRING, bcmCut, OPUSW("Cu&t"));
    AppendMenuW(popup, MF_STRING, bcmCopy, OPUSW("&Copy"));
    AppendMenuW(popup, MF_STRING, bcmPaste, OPUSW("&Paste"));
    AppendMenuW(popup, MF_STRING, bcmSelectAll, OPUSW("Select &All"));
    AppendMenuW(popup, MF_SEPARATOR, 0, NULL);
    AppendMenuW(popup, MF_STRING, bcmCharacter, OPUSW("&Font..."));
    AppendMenuW(popup, MF_STRING, bcmParagraph, OPUSW("&Paragraph..."));
    AppendMenuW(popup, MF_STRING, bcmTabs, OPUSW("&Tabs..."));
    AppendMenuW(popup, MF_STRING, imiRenumParas,
                OPUSW("&Bullets and Numbering..."));
    AppendMenuW(popup, MF_SEPARATOR, 0, NULL);
    AppendMenuW(popup, MF_STRING, bcmInsTable, OPUSW("Insert &Table..."));
    AppendMenuW(popup, MF_STRING, bcmFormatTable,
                OPUSW("Table &Properties..."));
    AppendMenuW(popup, MF_STRING, bcmInsPic, OPUSW("Insert &Picture..."));
    AppendMenuW(popup, MF_STRING, bcmInsField, OPUSW("Insert F&ield..."));
    style_menu_tree(popup);
    const HWND app = GetAncestor(pane, GA_ROOT);
    const UINT command = TrackPopupMenu(
        popup, TPM_RETURNCMD | TPM_RIGHTBUTTON,
        screen_point.x, screen_point.y, 0, app, NULL);
    DestroyMenu(popup);
    if (command != 0) {
        PostMessageW(app, WM_COMMAND, MAKEWPARAM(command, 0), 0);
    }
}

void update_language_menu_check() {
    if (g_language_menu == NULL) return;
    char selected[32] = {0};
    OpusUnicodeGetInputLanguage(selected, (int) sizeof(selected));
    UINT command = kCmdLanguageBase;
    for (int index = 0; index < kLanguageChoiceCount; ++index) {
        if (_stricmp(selected, kLanguageChoices[index].tag) == 0) {
            command = kLanguageChoices[index].command;
            break;
        }
    }
    CheckMenuRadioItem(g_language_menu, kCmdLanguageBase,
                       kCmdLanguageBase +
                           ((UINT) kLanguageChoiceCount - 1),
                       command, MF_BYCOMMAND);
}

bool start_next_unicode_input(HWND pane) {
    if (g_unicode_input_active || pending_unicode_empty())
        return true;
    int doc_before = -1;
    long cp_before = 0;
    long cp_lim_before = 0;
    if (!OpusGetUnicodeSelection(&doc_before, &cp_before, &cp_lim_before)) {
        pending_unicode_pop_front();
        return false;
    }
    g_active_unicode_input = pending_unicode_pop_front();
    g_active_unicode_input.doc = doc_before;
    g_active_unicode_input.cp = cp_before;
    g_active_unicode_input.retries = 0;
    g_unicode_input_active = true;
    const WPARAM legacy = g_active_unicode_input.scalar < 0x20 ?
        (WPARAM) g_active_unicode_input.scalar :
        ((WPARAM) (OpusUnicodeLegacyByteForScalar(
            g_active_unicode_input.scalar) & 0xff));
    if (!PostMessageW(pane, WM_CHAR, legacy, 1) ||
        !PostMessageW(pane, WM_LBUTTONUP, 0, 0) ||
        !PostMessageW(pane, kWmCommitUnicodeScalar, 0, 0)) {
        g_unicode_input_active = false;
        return false;
    }
    return true;
}

bool queue_unicode_input(HWND pane, const uint32_t scalar) {
    if (pane == NULL || scalar == 0 || scalar > 0x10ffff ||
        (scalar >= 0xd800 && scalar <= 0xdfff)) return false;
    PendingUnicodeInput input = {-1, 0, scalar, 0};
    if (!pending_unicode_push(input)) {
        return false;
    }
    return start_next_unicode_input(pane);
}

bool insert_unicode_scalar(HWND pane, const uint32_t scalar) {
    return scalar >= 0x20 && queue_unicode_input(pane, scalar);
}

void insert_unicode_text(HWND pane, const WCHAR* text, size_t length) {
    for (size_t index = 0; index < length;) {
        uint32_t scalar = (uint16_t) text[index++];
        if (scalar >= 0xd800 && scalar <= 0xdbff && index < length) {
            const uint32_t low = (uint16_t) text[index];
            if (low >= 0xdc00 && low <= 0xdfff) {
                ++index;
                scalar = 0x10000 + ((scalar - 0xd800) << 10) +
                         (low - 0xdc00);
            }
        }
        if (scalar == OPUSW("\r")[0] || scalar == OPUSW("\n")[0] ||
            scalar == OPUSW("\t")[0])
            queue_unicode_input(pane, scalar);
        else
            insert_unicode_scalar(pane, scalar);
    }
}

LRESULT CALLBACK document_pane_proc(HWND pane, UINT message,
                                     WPARAM w_param, LPARAM l_param) {
    WNDPROC original = original_pane_proc(pane);
    if (message == kWmCommitUnicodeScalar) {
        if (g_unicode_input_active) {
            int doc_after = -1;
            long cp_after = 0;
            long cp_lim_after = 0;
            if (OpusGetUnicodeSelection(
                    &doc_after, &cp_after, &cp_lim_after) &&
                doc_after == g_active_unicode_input.doc &&
                cp_after > g_active_unicode_input.cp) {
                if (g_active_unicode_input.scalar >= 0x20)
                    OpusUnicodeSetScalar(
                        g_active_unicode_input.doc,
                        g_active_unicode_input.cp,
                        g_active_unicode_input.scalar);
                g_unicode_input_active = false;
                InvalidateRect(pane, NULL, FALSE);
                start_next_unicode_input(pane);
            } else if (doc_after == g_active_unicode_input.doc &&
                       g_active_unicode_input.retries++ < 8) {
                PostMessageW(pane, WM_LBUTTONUP, 0, 0);
                PostMessageW(pane, kWmCommitUnicodeScalar, 0, 0);
            } else {
                g_unicode_input_active = false;
                start_next_unicode_input(pane);
            }
        }
        return 0;
    }
    if (message == WM_UNICHAR) {
        if (w_param == UNICODE_NOCHAR) return TRUE;
        insert_unicode_scalar(pane, (uint32_t) w_param);
        return 0;
    }

    /* The original message loop marks already-translated byte input by setting
     * bit 15 (0x80xx). That is an internal Word flag, not a Unicode code point,
     * and must reach the legacy pane procedure unchanged.
     */
    if (message == WM_CHAR && (w_param & 0xff00) != 0x8000 &&
        w_param >= 0x80) {
        OpusQueueUnicodeWmChar(pane, (unsigned int) w_param);
        return 0;
    }
    if (message == WM_IME_COMPOSITION &&
        (l_param & GCS_RESULTSTR) != 0) {
        HIMC context = ImmGetContext(pane);
        if (context != NULL) {
            const LONG byte_count = ImmGetCompositionStringW(
                context, GCS_RESULTSTR, NULL, 0);
            if (byte_count > 0 && byte_count <= 1024 * 1024 &&
                (byte_count % sizeof(WCHAR)) == 0) {
                const size_t length = (size_t) byte_count / sizeof(WCHAR);
                WCHAR* result = (WCHAR*) calloc(length, sizeof(WCHAR));
                if (ImmGetCompositionStringW(context, GCS_RESULTSTR,
                        result, (DWORD) byte_count) ==
                    byte_count) {
                    insert_unicode_text(pane, result, length);
                }
                free(result);
            }
            ImmReleaseContext(pane, context);
            return 0;
        }
    }
    if (message == WM_MOUSEWHEEL) {
        if (g_document_wheel.pane != pane) {
            g_document_wheel = (DocumentWheelState){0};
            g_document_wheel.pane = pane;
        }
        const int wheel_delta = GET_WHEEL_DELTA_WPARAM(w_param);
        if ((GET_KEYSTATE_WPARAM(w_param) & MK_CONTROL) != 0) {
            g_document_wheel.zoom_remainder += wheel_delta;
            int percent_delta = 0;
            while (g_document_wheel.zoom_remainder >= WHEEL_DELTA) {
                percent_delta += 10;
                g_document_wheel.zoom_remainder -= WHEEL_DELTA;
            }
            while (g_document_wheel.zoom_remainder <= -WHEEL_DELTA) {
                percent_delta -= 10;
                g_document_wheel.zoom_remainder += WHEEL_DELTA;
            }
            if (percent_delta != 0) {
                const int percent = OpusAdjustWin95Zoom(
                    pane, percent_delta);
                update_zoom_combo(pane, percent);
            }
            return 0;
        }

        const int scaled_delta = g_document_wheel.scroll_remainder -
            wheel_delta * scale(pane, 48);
        const int pixels = scaled_delta / WHEEL_DELTA;
        g_document_wheel.scroll_remainder =
            scaled_delta % WHEEL_DELTA;
        if (pixels != 0 &&
            OpusScrollWin95ContinuousPages(pane, pixels)) {
            return 0;
        }
    }
    if (message == WM_LBUTTONDOWN) {
        POINT point = {GET_X_LPARAM(l_param), GET_Y_LPARAM(l_param)};
        bool top_marker = false;
        int page_offset = 0;
        if (vertical_margin_marker_at(
                pane, point, &top_marker, &page_offset)) {
            int page_top = 0;
            int page_bottom = 0;
            int top_margin = 0;
            int bottom_margin = 0;
            int pixels_per_inch = 0;
            if (OpusGetWin95VerticalRulerMetrics(
                    pane, &page_top, &page_bottom, &top_margin,
                    &bottom_margin, &pixels_per_inch)) {
                g_vertical_ruler_drag.pane = pane;
                g_vertical_ruler_drag.active = true;
                g_vertical_ruler_drag.top = top_marker;
                g_vertical_ruler_drag.page_offset = page_offset;
                g_vertical_ruler_drag.preview_y = top_marker ?
                    page_top + page_offset + top_margin :
                    page_bottom + page_offset - bottom_margin;
                SetFocus(pane);
                SetCapture(pane);
                SetCursor(LoadCursorW(NULL, MAKEINTRESOURCEW(32645)));
                return 0;
            }
        }
    }
    if (message == WM_MOUSEMOVE) {
        if (g_vertical_ruler_drag.active &&
            g_vertical_ruler_drag.pane == pane) {
            update_vertical_margin_drag(pane, GET_Y_LPARAM(l_param));
            SetCursor(LoadCursorW(NULL, MAKEINTRESOURCEW(32645)));
            return 0;
        }
        POINT point = {GET_X_LPARAM(l_param), GET_Y_LPARAM(l_param)};
        bool top_marker = false;
        if (vertical_margin_marker_at(
                pane, point, &top_marker, NULL)) {
            SetCursor(LoadCursorW(NULL, MAKEINTRESOURCEW(32645)));
            return 0;
        }
    }
    if (message == WM_LBUTTONUP && g_vertical_ruler_drag.active &&
        g_vertical_ruler_drag.pane == pane) {
        update_vertical_margin_drag(pane, GET_Y_LPARAM(l_param));
        int page_top = 0;
        int page_bottom = 0;
        int top_margin = 0;
        int bottom_margin = 0;
        int pixels_per_inch = 0;
        const bool have_metrics = OpusGetWin95VerticalRulerMetrics(
            pane, &page_top, &page_bottom, &top_margin, &bottom_margin,
            &pixels_per_inch) != 0;
        const bool top_marker = g_vertical_ruler_drag.top;
        const int preview_y = g_vertical_ruler_drag.preview_y;
        const int page_offset = g_vertical_ruler_drag.page_offset;
        g_vertical_ruler_drag = (VerticalRulerDragState){0};
        if (GetCapture() == pane) {
            ReleaseCapture();
        }
        if (have_metrics) {
            const int margin_pixels = top_marker ?
                preview_y - (page_top + page_offset) :
                page_bottom + page_offset - preview_y;
            const bool changed = OpusSetWin95VerticalMargin(
                pane, top_marker ? 1 : 0, margin_pixels) != 0;
            if (!changed) {
                MessageBeep(MB_ICONEXCLAMATION);
            } else {
                clear_page_snapshots(pane);
            }
        }
        redraw_vertical_ruler(pane);
        return 0;
    }
    if (message == WM_KEYDOWN && w_param == VK_ESCAPE &&
        g_vertical_ruler_drag.active &&
        g_vertical_ruler_drag.pane == pane) {
        g_vertical_ruler_drag = (VerticalRulerDragState){0};
        if (GetCapture() == pane) {
            ReleaseCapture();
        }
        redraw_vertical_ruler(pane);
        return 0;
    }
    if (message == WM_CAPTURECHANGED &&
        g_vertical_ruler_drag.active &&
        g_vertical_ruler_drag.pane == pane &&
        ((HWND) l_param) != pane) {
        cancel_vertical_margin_drag(pane);
    }
    if (message == WM_CANCELMODE && g_vertical_ruler_drag.active &&
        g_vertical_ruler_drag.pane == pane) {
        g_vertical_ruler_drag = (VerticalRulerDragState){0};
        if (GetCapture() == pane) {
            ReleaseCapture();
        }
        redraw_vertical_ruler(pane);
        return 0;
    }
    if (message == WM_SETCURSOR && LOWORD(l_param) == HTCLIENT) {
        POINT point = {0};
        GetCursorPos(&point);
        ScreenToClient(pane, &point);
        bool top_marker = false;
        if ((g_vertical_ruler_drag.active &&
             g_vertical_ruler_drag.pane == pane) ||
            vertical_margin_marker_at(
                pane, point, &top_marker, NULL)) {
            SetCursor(LoadCursorW(NULL, MAKEINTRESOURCEW(32645)));
            return TRUE;
        }
    }
    if (message == WM_PAINT) {
        const LRESULT result = original != NULL ?
            CallWindowProcW(original, pane, message, w_param, l_param) :
            DefWindowProcW(pane, message, w_param, l_param);
        HDC dc = GetDC(pane);
        if (dc != NULL) {
            update_current_page_snapshot(pane, dc);
            draw_continuous_page_workspace(pane, dc);
            draw_continuous_page_neighbors(pane, dc);
            draw_vertical_ruler(pane, dc);
            ReleaseDC(pane, dc);
        }
        return result;
    }
    if (message == WM_SIZE) {
        const LRESULT result = original != NULL ?
            CallWindowProcW(original, pane, message, w_param, l_param) :
            DefWindowProcW(pane, message, w_param, l_param);
        OpusRecenterWin95PageView(pane);
        InvalidateRect(pane, NULL, TRUE);
        return result;
    }
    if (message == WM_RBUTTONDOWN) {
        SetFocus(pane);
        return 0;
    }
    if (message == WM_RBUTTONUP) {
        POINT point = {GET_X_LPARAM(l_param), GET_Y_LPARAM(l_param)};
        ClientToScreen(pane, &point);
        show_document_context_menu(pane, point);
        return 0;
    }
    if (message == WM_CONTEXTMENU) {
        POINT point = {GET_X_LPARAM(l_param), GET_Y_LPARAM(l_param)};
        if (point.x == -1 && point.y == -1) {
            GetCursorPos(&point);
        }
        show_document_context_menu(pane, point);
        return 0;
    }
    if (message == WM_NCDESTROY) {
        if (g_document_wheel.pane == pane) {
            g_document_wheel = (DocumentWheelState){0};
        }
        pending_unicode_clear();
        g_unicode_input_active = false;
        g_pending_high_surrogate = 0;
        clear_page_snapshots(pane);
        RemovePropW(pane, kOriginalPaneProcProperty);
        if (original != NULL) {
            SetWindowLongPtrW(pane, GWLP_WNDPROC,
                              (LONG_PTR) original);
        }
    }
    return original != NULL ?
        CallWindowProcW(original, pane, message, w_param, l_param) :
        DefWindowProcW(pane, message, w_param, l_param);
}

BOOL CALLBACK subclass_document_pane(HWND candidate, LPARAM parameter) {
    (void) parameter;
    WCHAR class_name[64] = {0};
    GetClassNameW(candidate, class_name,
                  (int) (sizeof(class_name) / sizeof(class_name[0])));
    if (lstrcmpiW(class_name, kDocumentPaneClass) == 0 &&
        original_pane_proc(candidate) == NULL) {
        SetLastError(0);
        WNDPROC original = ((WNDPROC) SetWindowLongPtrW(
            candidate, GWLP_WNDPROC,
            (LONG_PTR) document_pane_proc));
        if (original != NULL || GetLastError() == 0) {
            SetPropW(candidate, kOriginalPaneProcProperty,
                     (HANDLE) original);
            RedrawWindow(candidate, NULL, NULL,
                         RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
        }
    }
    return TRUE;
}

BOOL CALLBACK redraw_word95_ruler_window(HWND candidate, LPARAM parameter) {
    (void) parameter;
    WCHAR class_name[64] = {0};
    GetClassNameW(candidate, class_name,
                  (int) (sizeof(class_name) / sizeof(class_name[0])));
    if (lstrcmpiW(class_name, kRulerClass) == 0 ||
        lstrcmpiW(class_name, kDocumentPaneClass) == 0) {
        RedrawWindow(candidate, NULL, NULL,
                     RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
    }
    return TRUE;
}

BOOL CALLBACK paint_word95_horizontal_ruler(HWND candidate, LPARAM parameter) {
    (void) parameter;
    WCHAR class_name[64] = {0};
    GetClassNameW(candidate, class_name,
                  (int) (sizeof(class_name) / sizeof(class_name[0])));
    if (lstrcmpiW(class_name, kRulerClass) == 0) {
        ensure_horizontal_ruler_overlay(candidate);
    }
    return TRUE;
}

void subclass_all_document_panes(HWND app) {
    EnumChildWindows(app, subclass_document_pane, 0);
}

LRESULT CALLBACK app_window_proc(HWND app, UINT message,
                                 WPARAM w_param, LPARAM l_param) {
    WNDPROC original = g_original_app_proc;
    if (message == WM_COMMAND) {
        const UINT command = LOWORD(w_param);
        HWND toolbar = FindWindowExW(app, NULL, kToolbarClass, NULL);
        if (command == bcmPageView) {
            /* CmdPageView is a legacy toggle. The restored shell treats the
             * startup Page View choice as a stable document mode, so repeated
             * selections do not accidentally drop back to flush galley view.
             */
            if (g_word95_page_view_active) {
                return 0;
            }
            g_word95_page_view_active = true;
        }
        if (command == bcmColor) {
            show_text_color_palette(app, toolbar);
            return 0;
        }
        if (command == kCmdExportPdf) {
            OpusExportCurrentDocumentPdf();
            return 0;
        }
        if (command >= kCmdLanguageBase &&
            command < kCmdLanguageBase + kLanguageChoiceCount) {
            const LanguageChoice* choice =
                &kLanguageChoices[command - kCmdLanguageBase];
            OpusUnicodeSetInputLanguage(choice->tag);
            update_language_menu_check();
            DrawMenuBar(app);
            return 0;
        }
        if (command == kCmdToggleStandardToolbar) {
            toggle_toolbar_row(app, toolbar, true);
            return 0;
        }
        if (command == kCmdToggleFormattingToolbar) {
            toggle_toolbar_row(app, toolbar, false);
            return 0;
        }
    } else if (message == WM_INITMENUPOPUP) {
        update_language_menu_check();
        HWND toolbar = FindWindowExW(app, NULL, kToolbarClass, NULL);
        ToolbarState* state = toolbar_state(toolbar);
        if (state != NULL) {
            update_toolbar_menu_checks(app, state);
        }

        /* On current Windows versions a popup using a classic background brush
         * can receive its initial erase before the menu items have been
         * invalidated. Repaint once the #32768 popup exists so the complete
         * File menu is visible without requiring mouse hover.
         */
        SetTimer(app, kMenuRepaintTimer, 15, NULL);
    } else if (message == WM_TIMER && w_param == kMenuRepaintTimer) {
        KillTimer(app, kMenuRepaintTimer);
        EnumThreadWindows(GetCurrentThreadId(), repaint_menu_popup, 0);
        return 0;
    } else if (message == WM_NCDESTROY) {
        g_original_app_proc = NULL;
        if (original != NULL) {
            SetWindowLongPtrW(app, GWLP_WNDPROC,
                              (LONG_PTR) original);
        }
    }
    return original != NULL ?
        CallWindowProcW(original, app, message, w_param, l_param) :
        DefWindowProcW(app, message, w_param, l_param);
}

bool subclass_app_window(HWND app) {
    if (g_original_app_proc != NULL) {
        return true;
    }
    SetLastError(0);
    WNDPROC original = ((WNDPROC) SetWindowLongPtrW(
        app, GWLP_WNDPROC, (LONG_PTR) app_window_proc));
    if (original == NULL && GetLastError() != 0) {
        return false;
    }
    g_original_app_proc = original;
    return true;
}

LRESULT CALLBACK toolbar_window_proc(HWND window, UINT message,
                                     WPARAM w_param, LPARAM l_param) {
    ToolbarState* state = (ToolbarState*) GetWindowLongPtrW(
        window, GWLP_USERDATA);
    switch (message) {
    case WM_CREATE: {
        ToolbarState* created = (ToolbarState*) calloc(1, sizeof(*created));
        if (created == NULL) return -1;
        toolbar_state_init(created);
        SetWindowLongPtrW(window, GWLP_USERDATA, (LONG_PTR) created);
        created->sprite = ((HBITMAP) LoadImageW(
            GetModuleHandleW(NULL), MAKEINTRESOURCEW(kToolbarBitmap),
            IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION));
        LOGFONTW logical = {0};
        logical.lfHeight = -MulDiv(8, dpi_for_window(window), 72);
        logical.lfWeight = FW_NORMAL;
        logical.lfCharSet = DEFAULT_CHARSET;
        lstrcpyW(logical.lfFaceName, kToolbarFace);
        created->font = CreateFontIndirectW(&logical);
        created->style_combo = create_combo(window, kComboStyle);
        created->font_combo = create_combo(window, kComboFont);
        created->size_combo = create_combo(window, kComboSize);
        created->zoom_combo = create_zoom_combo(window);
        SetWindowTextW(created->style_combo, kNormalStyle);
        SetWindowTextW(created->font_combo, kArialFace);
        SetWindowTextW(created->size_combo, kDefaultSize);
        SendMessageW(created->style_combo, WM_SETFONT,
                     (WPARAM) created->font, FALSE);
        SendMessageW(created->font_combo, WM_SETFONT,
                     (WPARAM) created->font, FALSE);
        SendMessageW(created->size_combo, WM_SETFONT,
                     (WPARAM) created->font, FALSE);
        SendMessageW(created->zoom_combo, WM_SETFONT,
                     (WPARAM) created->font, FALSE);
        position_combos(window, created);
        sync_mirrors(window, created);
        SetTimer(window, kSyncTimer, 350, NULL);
        return 0;
    }
    case WM_SIZE:
        if (state != NULL) {
            position_combos(window, state);
        }
        return 0;
    case WM_TIMER:
        if (state != NULL && w_param == kSyncTimer) {
            if (GetTickCount64() >= state->suppress_sync_until) {
                sync_mirrors(window, state);
            }
            HWND app = GetParent(window);
            subclass_all_document_panes(app);
            if (state->ruler_refreshes_remaining > 0) {
                EnumChildWindows(app, redraw_word95_ruler_window, 0);
                --state->ruler_refreshes_remaining;
            }
            if (!state->startup_page_view_requested &&
                state->page_view_start_after != 0 &&
                GetTickCount64() >= state->page_view_start_after) {
                HWND pane = NULL;
                EnumChildWindows(app, find_document_pane,
                                 ((LPARAM) &pane));
                if (pane != NULL) {
                    state->startup_page_view_requested = true;
                    state->ruler_refreshes_remaining = 6;
                    SendMessageW(state->zoom_combo, CB_SETCURSEL, 1, 0);
                    PostMessageW(app, WM_COMMAND,
                                 MAKEWPARAM(bcmPageView, 0), 0);
                }
            }
            if (g_word95_page_view_active) {
                EnumChildWindows(app, paint_word95_horizontal_ruler, 0);
            }
        }
        return 0;
    case WM_COMMAND:
        if (state != NULL) {
            const UINT id = LOWORD(w_param);
            const int notification = HIWORD(w_param);
            if (notification == CBN_SELCHANGE ||
                notification == CBN_SELENDOK ||
                notification == CBN_EDITCHANGE) {
                state->suppress_sync_until = GetTickCount64() + 1000;
            }
            if (id == kComboStyle) {
                forward_combo(state->style_combo, state->source_style,
                              notification, &state->style_edit_dirty);
                return 0;
            }
            if (id == kComboFont) {
                forward_combo(state->font_combo, state->source_font,
                              notification, &state->font_edit_dirty);
                return 0;
            }
            if (id == kComboSize) {
                forward_combo(state->size_combo, state->source_size,
                              notification, &state->size_edit_dirty);
                return 0;
            }
            if (id == kComboZoom && notification == CBN_SELENDOK) {
                const LRESULT selection = SendMessageW(
                    state->zoom_combo, CB_GETCURSEL, 0, 0);
                if (selection == 1) {
                    PostMessageW(GetParent(window), WM_COMMAND,
                                 MAKEWPARAM(bcmPageView, 0), 0);
                } else if (selection == 2 || selection == 3) {
                    PostMessageW(GetParent(window), WM_COMMAND,
                                 MAKEWPARAM(bcmPrintPreview, 0), 0);
                }
                restore_document_focus_from_root(GetParent(window));
                return 0;
            }
        }
        break;
    case WM_LBUTTONDOWN:
        if (state != NULL) {
            POINT point = {GET_X_LPARAM(l_param), GET_Y_LPARAM(l_param)};
            state->pressed = hit_test(window, state, point);
            if (state->pressed.hit) {
                SetCapture(window);
                InvalidateRect(window, NULL, FALSE);
            }
        }
        return 0;
    case WM_CONTEXTMENU:
        if (state != NULL) {
            POINT point = {GET_X_LPARAM(l_param), GET_Y_LPARAM(l_param)};
            if (point.x == -1 && point.y == -1) {
                GetCursorPos(&point);
            }
            show_toolbar_context_menu(window, point);
        }
        return 0;
    case WM_RBUTTONDOWN:
        return 0;
    case WM_RBUTTONUP:
        if (state != NULL) {
            POINT point = {GET_X_LPARAM(l_param), GET_Y_LPARAM(l_param)};
            ClientToScreen(window, &point);
            show_toolbar_context_menu(window, point);
        }
        return 0;
    case WM_LBUTTONUP:
        if (state != NULL && state->pressed.hit) {
            POINT point = {GET_X_LPARAM(l_param), GET_Y_LPARAM(l_param)};
            const HitResult released = hit_test(window, state, point);
            const HitResult pressed = state->pressed;
            state->pressed = (HitResult){false, false, -1, 0};
            if (GetCapture() == window) {
                ReleaseCapture();
            }
            if (released.hit && released.format == pressed.format &&
                released.index == pressed.index) {
                if (pressed.format &&
                    kFormatButtons[pressed.index].latch) {
                    if (pressed.index >= 4 && pressed.index <= 7) {
                        for (int index = 4; index <= 7; ++index) {
                            state->format_latched[index] = false;
                        }
                    }
                    state->format_latched[pressed.index] =
                        !state->format_latched[pressed.index];
                } else if (!pressed.format &&
                           kStandardButtons[pressed.index].latch) {
                    state->standard_latched[pressed.index] =
                        !state->standard_latched[pressed.index];
                }
                PostMessageW(GetParent(window), WM_COMMAND,
                             MAKEWPARAM(pressed.command, 0), 0);
            }
            InvalidateRect(window, NULL, FALSE);
        }
        return 0;
    case WM_CAPTURECHANGED:
        if (state != NULL && state->pressed.hit) {
            state->pressed = (HitResult){false, false, -1, 0};
            InvalidateRect(window, NULL, FALSE);
        }
        return 0;
    case WM_ERASEBKGND:
        return 1;
    case WM_PAINT:
        if (state != NULL) {
            paint_toolbar(window, state);
            return 0;
        }
        break;
    case WM_DESTROY:
        if (state != NULL) {
            KillTimer(window, kSyncTimer);
            if (state->sprite != NULL) {
                DeleteObject(state->sprite);
            }
            if (state->font != NULL) {
                DeleteObject(state->font);
            }
            free(state);
            SetWindowLongPtrW(window, GWLP_USERDATA, 0);
        }
        break;
    }
    return DefWindowProcW(window, message, w_param, l_param);
}

bool register_toolbar_class() {
    WNDCLASSEXW window_class = {0};
    window_class.cbSize = sizeof(window_class);
    window_class.style = CS_DBLCLKS;
    window_class.lpfnWndProc = toolbar_window_proc;
    window_class.hInstance = GetModuleHandleW(NULL);
    window_class.hCursor = LoadCursorW(NULL, MAKEINTRESOURCEW(32512));
    window_class.lpszClassName = kToolbarClass;
    return RegisterClassExW(&window_class) != 0 ||
           GetLastError() == ERROR_CLASS_ALREADY_EXISTS;
}

bool register_ruler_overlay_class() {
    WNDCLASSEXW window_class = {0};
    window_class.cbSize = sizeof(window_class);
    window_class.lpfnWndProc = ruler_overlay_proc;
    window_class.hInstance = GetModuleHandleW(NULL);
    window_class.hCursor = LoadCursorW(NULL, MAKEINTRESOURCEW(32512));
    window_class.lpszClassName = kRulerOverlayClass;
    return RegisterClassExW(&window_class) != 0 ||
           GetLastError() == ERROR_CLASS_ALREADY_EXISTS;
}

int OpusQueueUnicodeWmChar(
    HWND pane, const unsigned int code_unit) {
    if (code_unit >= 0xd800 && code_unit <= 0xdbff) {
        g_pending_high_surrogate = (WCHAR) code_unit;
        return TRUE;
    }
    uint32_t scalar = code_unit;
    if (code_unit >= 0xdc00 && code_unit <= 0xdfff) {
        if (g_pending_high_surrogate < 0xd800 ||
            g_pending_high_surrogate > 0xdbff) {
            g_pending_high_surrogate = 0;
            return FALSE;
        }
        scalar = 0x10000 +
            (((uint32_t) g_pending_high_surrogate - 0xd800) << 10) +
            (code_unit - 0xdc00);
    }
    g_pending_high_surrogate = 0;
    return insert_unicode_scalar(pane, scalar);
}

void OpusDrawWin95HorizontalRuler(HWND ruler) {
    HDC dc = GetDC(ruler);
    if (dc != NULL) {
        draw_horizontal_ruler(ruler, dc);
        ReleaseDC(ruler, dc);
    }
}

HWND vhwndWin95Toolbar = NULL;

int OpusWin95ToolbarHeight(void) {
    return toolbar_height(vhwndWin95Toolbar);
}

int OpusWin95ChromeActive(void) {
    return vhwndWin95Toolbar != NULL && IsWindow(vhwndWin95Toolbar);
}

void OpusSyncWin95Toolbar(void) {
    if (OpusWin95ChromeActive()) {
        SendMessageW(vhwndWin95Toolbar, WM_TIMER, kSyncTimer, 0);
        configure_word95_menus(GetParent(vhwndWin95Toolbar));
        style_menu_tree(GetMenu(GetParent(vhwndWin95Toolbar)));
        ToolbarState* state = toolbar_state(vhwndWin95Toolbar);
        if (state != NULL) {
            update_toolbar_menu_checks(GetParent(vhwndWin95Toolbar), state);
        }
        subclass_all_document_panes(GetParent(vhwndWin95Toolbar));
        DrawMenuBar(GetParent(vhwndWin95Toolbar));
    }
}

void OpusSizeWin95Toolbar(HWND parent) {
    if (!OpusWin95ChromeActive()) {
        return;
    }
    RECT client = {0};
    GetClientRect(parent, &client);
    const int height = OpusWin95ToolbarHeight();
    ShowWindow(vhwndWin95Toolbar,
               height > 0 ? SW_SHOWNOACTIVATE : SW_HIDE);
    MoveWindow(vhwndWin95Toolbar, 0, 0, client.right - client.left,
               height, TRUE);
}

int OpusCreateWin95Chrome(HWND parent) {
    if (OpusWin95ChromeActive()) {
        return TRUE;
    }
    if (!register_toolbar_class() || !register_ruler_overlay_class()) {
        return FALSE;
    }
    if (g_menu_brush == NULL) {
        g_menu_brush = CreateSolidBrush(kButtonFace);
    }
    configure_word95_menus(parent);
    style_menu_tree(GetMenu(parent));
    DrawMenuBar(parent);
    apply_caption_colors(parent);
    set_window_classic(parent);

    vhwndWin95Toolbar = CreateWindowExW(
        0, kToolbarClass, NULL,
        WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
        0, 0, 1, scale(parent, 64), parent, NULL,
        GetModuleHandleW(NULL), NULL);
    if (vhwndWin95Toolbar == NULL) {
        return FALSE;
    }
    if (!subclass_app_window(parent)) {
        DestroyWindow(vhwndWin95Toolbar);
        vhwndWin95Toolbar = NULL;
        return FALSE;
    }
    subclass_all_document_panes(parent);
    OpusSizeWin95Toolbar(parent);
    ToolbarState* state = toolbar_state(vhwndWin95Toolbar);
    if (state != NULL) {
        update_toolbar_menu_checks(parent, state);
    }
    SetWindowPos(vhwndWin95Toolbar, HWND_TOP, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    return TRUE;
}

void OpusRequestWin95PageView(HWND parent) {
    HWND pane = NULL;
    EnumChildWindows(parent, find_document_pane,
                     ((LPARAM) &pane));
    if (pane == NULL) {
        return;
    }

    HWND toolbar = FindWindowExW(parent, NULL, kToolbarClass, NULL);
    ToolbarState* state = toolbar_state(toolbar);
    if (state != NULL) {
        state->page_view_start_after = GetTickCount64() + 750;
        state->startup_page_view_requested = false;
    }
}
