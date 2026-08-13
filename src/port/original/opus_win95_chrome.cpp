#define WIN32_LEAN_AND_MEAN
#include "opus_x64_compat.h"
#include <windowsx.h>
#include <imm.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <deque>
#include <string>
#include <vector>

#define USEBCM
#include "opuscmd.h"

extern "C" int OpusGetWin95VerticalRulerMetrics(
    HWND pane, int* page_top, int* page_bottom, int* top_margin,
    int* bottom_margin, int* pixels_per_inch);
extern "C" int OpusGetWin95ContinuousPageMetrics(
    HWND pane, int* page_left, int* page_top, int* page_right,
    int* page_bottom, int* page_gap, int* has_previous, int* has_next);
extern "C" int OpusScrollWin95ContinuousPages(HWND pane, int pixels);
extern "C" int OpusGetWin95ZoomPercent(HWND pane);
extern "C" int OpusGetWin95CurrentPageIndex(HWND pane);
extern "C" int OpusAdjustWin95Zoom(HWND pane, int percent_delta);
extern "C" int OpusRecenterWin95PageView(HWND pane);
extern "C" int OpusSetWin95VerticalMargin(
    HWND pane, int top_margin, int margin_pixels);
extern "C" int OpusApplyWin95TextColor(HWND pane, int color_index);
extern "C" int OpusGetWin95HorizontalRulerMetrics(
    HWND ruler, int* zero, int* active_left, int* active_right,
    int* pixels_per_inch, int* left_indent, int* first_indent,
    int* right_indent, int* default_tab, int* tab_count,
    int* tab_positions, unsigned char* tab_types, int tab_capacity);
extern "C" int OpusGetWin95HorizontalPageMargins(
    HWND ruler, int* left_margin, int* right_margin);
extern "C" int OpusAdjustWin95HorizontalMargin(
    HWND ruler, int left_margin, int delta_pixels);
extern "C" void OpusDrawWin95HorizontalRuler(HWND ruler);
extern "C" int OpusExportCurrentDocumentPdf(void);
extern "C" int OpusGetUnicodeSelection(
    int* doc, long* cp_first, long* cp_lim);
extern "C" int OpusUnicodeLegacyByteForScalar(unsigned int scalar);
extern "C" int OpusUnicodeSetScalar(
    int doc, long cp, unsigned int scalar);
extern "C" int OpusUnicodeSetInputLanguage(const char* language);
extern "C" int OpusUnicodeGetInputLanguage(char* language, int capacity);
extern "C" int OpusQueueUnicodeWmChar(HWND pane, unsigned int code_unit);

namespace {

constexpr WCHAR kToolbarClass[] = OPUSW("OpusWin95Toolbar");
constexpr WCHAR kRulerOverlayClass[] = OPUSW("OpusWin95RulerOverlay");
constexpr WCHAR kComboBoxClass[] = OPUSW("ComboBox");
constexpr WCHAR kComboBoxWindowClass[] = OPUSW("COMBOBOX");
constexpr WCHAR kDocumentPaneClass[] = OPUSW("OpusWwd");
constexpr WCHAR kRulerClass[] = OPUSW("OpusRul");
constexpr WCHAR kMenuPopupClass[] = OPUSW("#32768");
constexpr WCHAR kArialFace[] = OPUSW("Arial");
constexpr WCHAR kToolbarFace[] = OPUSW("MS Sans Serif");
constexpr WCHAR kNormalStyle[] = OPUSW("Normal");
constexpr WCHAR kDefaultSize[] = OPUSW("10");
constexpr WCHAR kZoom100[] = OPUSW("100%");
constexpr WCHAR kZoomPageWidth[] = OPUSW("Page Width");
constexpr WCHAR kZoomWholePage[] = OPUSW("Whole Page");
constexpr WCHAR kZoomPrintPreview[] = OPUSW("Print Preview");
constexpr WCHAR kDwmApiLibrary[] = OPUSW("dwmapi.dll");
constexpr WCHAR kFormatGlyphColor[] = OPUSW("A");
constexpr WCHAR kFormatGlyphBold[] = OPUSW("B");
constexpr WCHAR kFormatGlyphItalic[] = OPUSW("I");
constexpr WCHAR kFormatGlyphUnderline[] = OPUSW("U");
constexpr int kToolbarBitmap = 201;
constexpr int kSpriteCell = 20;
constexpr COLORREF kButtonFace = RGB(192, 192, 192);
constexpr UINT_PTR kMenuRepaintTimer = 0x7f51;
constexpr COLORREF kButtonShadow = RGB(128, 128, 128);
constexpr COLORREF kButtonDarkShadow = RGB(0, 0, 0);
constexpr COLORREF kButtonHighlight = RGB(255, 255, 255);
constexpr COLORREF kCaptionBlue = RGB(0, 0, 128);
constexpr UINT kComboStyle = 0x7501;
constexpr UINT kComboFont = 0x7502;
constexpr UINT kComboSize = 0x7503;
constexpr UINT kComboZoom = 0x7504;
constexpr UINT kCmdToggleStandardToolbar = 0x7101;
constexpr UINT kCmdToggleFormattingToolbar = 0x7102;
constexpr UINT kCmdExportPdf = 0x7103;
constexpr UINT kCmdTextColorBase = 0x7200;
constexpr UINT kCmdLanguageBase = 0x7300;
constexpr UINT_PTR kSyncTimer = 0x951;
constexpr UINT kWmCommitUnicodeScalar = WM_APP + 0x452;
constexpr std::size_t kMaxPendingUnicodeInput = 0xffff;
constexpr WCHAR kOriginalPaneProcProperty[] = OPUSW("OpusWord95OriginalPaneProc");

enum class FormatGlyph {
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
};

struct SpriteButton {
    int sprite;
    UINT command;
    int group;
    bool latch;
};

struct FormatButton {
    FormatGlyph glyph;
    UINT command;
    int group;
    bool latch;
};

constexpr std::array<SpriteButton, 18> kStandardButtons{{
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
}};

constexpr std::array<FormatButton, 14> kFormatButtons{{
    {FormatGlyph::bold, bcmBold, 0, true},
    {FormatGlyph::italic, bcmItalic, 0, true},
    {FormatGlyph::underline, bcmULine, 0, true},
    {FormatGlyph::color, bcmColor, 0, false},
    {FormatGlyph::align_left, bcmParaLeft, 1, true},
    {FormatGlyph::align_center, bcmParaCenter, 1, true},
    {FormatGlyph::align_right, bcmParaRight, 1, true},
    {FormatGlyph::align_justify, bcmParaBoth, 1, true},
    {FormatGlyph::numbered, imiRenumParas, 2, false},
    {FormatGlyph::bullets, imiRenumParas, 2, false},
    {FormatGlyph::indent_left, bcmUnIndent, 2, false},
    {FormatGlyph::indent_right, bcmIndent, 2, false},
    {FormatGlyph::table, bcmInsTable, 3, false},
    {FormatGlyph::borders, bcmParagraph, 3, false},
}};

struct HitResult {
    bool hit = false;
    bool format = false;
    int index = -1;
    UINT command = 0;
};

struct ToolbarState {
    HBITMAP sprite = nullptr;
    HFONT font = nullptr;
    HWND style_combo = nullptr;
    HWND font_combo = nullptr;
    HWND size_combo = nullptr;
    HWND zoom_combo = nullptr;
    HWND source_style = nullptr;
    HWND source_font = nullptr;
    HWND source_size = nullptr;
    int copied_style_count = -1;
    int copied_font_count = -1;
    int copied_size_count = -1;
    ULONGLONG suppress_sync_until = 0;
    ULONGLONG page_view_start_after = 0;
    bool style_edit_dirty = false;
    bool font_edit_dirty = false;
    bool size_edit_dirty = false;
    bool standard_visible = true;
    bool formatting_visible = true;
    bool startup_page_view_requested = false;
    int ruler_refreshes_remaining = 0;
    int text_color_index = 0;
    COLORREF text_color = RGB(0, 0, 0);
    HitResult pressed{};
    std::array<bool, kStandardButtons.size()> standard_latched{};
    std::array<bool, kFormatButtons.size()> format_latched{};
};

HBRUSH g_menu_brush = nullptr;
HMENU g_table_menu = nullptr;
HMENU g_toolbars_menu = nullptr;
HMENU g_language_menu = nullptr;
WNDPROC g_original_app_proc = nullptr;
bool g_word95_page_view_active = false;
wchar_t g_pending_high_surrogate = 0;

struct LanguageChoice {
    UINT command;
    const WCHAR* label;
    const char* tag;
};

struct PendingUnicodeInput {
    int doc = -1;
    long cp = 0;
    std::uint32_t scalar = 0;
    int retries = 0;
};

std::deque<PendingUnicodeInput> g_pending_unicode_inputs;
PendingUnicodeInput g_active_unicode_input;
bool g_unicode_input_active = false;

constexpr std::array<LanguageChoice, 17> kLanguageChoices{{
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
}};

struct VerticalRulerDragState {
    HWND pane = nullptr;
    bool active = false;
    bool top = false;
    int preview_y = 0;
    int page_offset = 0;
};

VerticalRulerDragState g_vertical_ruler_drag{};

struct HorizontalRulerDragState {
    HWND overlay = nullptr;
    HWND ruler = nullptr;
    bool active = false;
    bool left = false;
    int origin_x = 0;
    int preview_x = 0;
};

HorizontalRulerDragState g_horizontal_ruler_drag{};

struct DocumentWheelState {
    HWND pane = nullptr;
    int scroll_remainder = 0;
    int zoom_remainder = 0;
};

DocumentWheelState g_document_wheel{};

struct PageSnapshot {
    HWND pane = nullptr;
    int page_index = -1;
    int zoom_percent = 100;
    int width = 0;
    int height = 0;
    HBITMAP bitmap = nullptr;
    ULONGLONG last_used = 0;
};

std::vector<PageSnapshot> g_page_snapshots;

int dpi_for_window(HWND window) {
    using GetDpiForWindowProc = UINT(WINAPI*)(HWND);
    static const auto get_dpi = reinterpret_cast<GetDpiForWindowProc>(
        GetProcAddress(GetModuleHandleW(OPUSW("user32.dll")),
                       "GetDpiForWindow"));
    return get_dpi != nullptr && window != nullptr ?
               static_cast<int>(get_dpi(window)) :
               96;
}

int scale(HWND window, int value) {
    return MulDiv(value, dpi_for_window(window), 96);
}

void set_window_classic(HWND window) {
    using SetWindowThemeProc = HRESULT(WINAPI*)(HWND, LPCWSTR, LPCWSTR);
    static HMODULE theme_module = LoadLibraryExW(
        OPUSW("uxtheme.dll"), nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
    static const auto set_theme = theme_module != nullptr ?
        reinterpret_cast<SetWindowThemeProc>(
            GetProcAddress(theme_module, "SetWindowTheme")) : nullptr;
    if (set_theme != nullptr && window != nullptr) {
        set_theme(window, OPUSW(""), OPUSW(""));
    }
}

void style_menu_tree(HMENU menu) {
    if (menu == nullptr) {
        return;
    }
    MENUINFO info{};
    info.cbSize = sizeof(info);
    info.fMask = MIM_BACKGROUND | MIM_APPLYTOSUBMENUS;
    info.hbrBack = g_menu_brush;
    SetMenuInfo(menu, &info);
    const int count = GetMenuItemCount(menu);
    for (int index = 0; index < count; ++index) {
        style_menu_tree(GetSubMenu(menu, index));
    }
}

BOOL CALLBACK repaint_menu_popup(HWND window, LPARAM) {
    WCHAR class_name[32]{};
    GetClassNameW(window, class_name,
                  static_cast<int>(std::size(class_name)));
    if (lstrcmpW(class_name, kMenuPopupClass) == 0 && IsWindowVisible(window)) {
        RedrawWindow(window, nullptr, nullptr,
                     RDW_ERASE | RDW_INVALIDATE | RDW_UPDATENOW |
                         RDW_ALLCHILDREN | RDW_FRAME);
    }
    return TRUE;
}

void configure_word95_menus(HWND window) {
    HMENU root = GetMenu(window);
    if (root == nullptr || GetMenuItemCount(root) < 7) {
        return;
    }

    auto menu_name = [root](int index) {
        wchar_t label[80]{};
        GetMenuStringW(root, index, label,
                       static_cast<int>(sizeof(label) / sizeof(label[0])),
                       MF_BYPOSITION);
        std::wstring name;
        for (const wchar_t character : std::wstring(label)) {
            if (character != L'&') {
                name.push_back(character);
            }
        }
        return name;
    };
    auto find_named = [&menu_name, root](const wchar_t* expected) {
        const int count = GetMenuItemCount(root);
        for (int index = 0; index < count; ++index) {
            if (lstrcmpiW(menu_name(index).c_str(), expected) == 0) {
                return index;
            }
        }
        return -1;
    };
    auto find_named_in = [](HMENU menu, const wchar_t* expected) {
        const int count = menu != nullptr ? GetMenuItemCount(menu) : 0;
        for (int index = 0; index < count; ++index) {
            wchar_t label[80]{};
            GetMenuStringW(menu, index, label,
                           static_cast<int>(sizeof(label) / sizeof(label[0])),
                           MF_BYPOSITION);
            std::wstring name;
            for (const wchar_t character : std::wstring(label)) {
                if (character != L'&') {
                    name.push_back(character);
                }
            }
            if (lstrcmpiW(name.c_str(), expected) == 0) {
                return index;
            }
        }
        return -1;
    };

    // Word's menu loader can finish replacing the startup menu after this layer
    // is created, so normalize by label every time we resync.
    const int file_index = find_named(L"File");
    if (file_index >= 0 && GetMenuItemCount(root) > file_index + 4) {
        HMENU insert = GetSubMenu(root, file_index + 3);
        HMENU format = GetSubMenu(root, file_index + 4);
        if (insert != nullptr) {
            ModifyMenuW(root, file_index + 3,
                        MF_BYPOSITION | MF_POPUP | MF_STRING,
                        reinterpret_cast<UINT_PTR>(insert), L"&Insert");
        }
        if (format != nullptr) {
            ModifyMenuW(root, file_index + 4,
                        MF_BYPOSITION | MF_POPUP | MF_STRING,
                        reinterpret_cast<UINT_PTR>(format), L"F&ormat");
        }
    }
    HMENU file_menu = file_index >= 0 ? GetSubMenu(root, file_index) : nullptr;
    if (file_menu != nullptr &&
        GetMenuState(file_menu, kCmdExportPdf, MF_BYCOMMAND) ==
            static_cast<UINT>(-1)) {
        int exit_position = find_named_in(file_menu, L"Exit");
        if (exit_position < 0) exit_position = GetMenuItemCount(file_menu);
        InsertMenuW(file_menu, exit_position, MF_BYPOSITION | MF_SEPARATOR,
                    0, nullptr);
        InsertMenuW(file_menu, exit_position, MF_BYPOSITION | MF_STRING,
                    kCmdExportPdf, L"E&xport as PDF...");
    }
    const int utilities_index = find_named(L"Utilities");
    if (utilities_index >= 0) {
        HMENU tools = GetSubMenu(root, utilities_index);
        ModifyMenuW(root, utilities_index,
                    MF_BYPOSITION | MF_POPUP | MF_STRING,
                    reinterpret_cast<UINT_PTR>(tools), L"&Tools");
    }
    const int tools_index = find_named(L"Tools");
    HMENU tools_menu = tools_index >= 0 ? GetSubMenu(root, tools_index) :
        (utilities_index >= 0 ? GetSubMenu(root, utilities_index) : nullptr);
    if (g_language_menu == nullptr || !IsMenu(g_language_menu)) {
        g_language_menu = CreatePopupMenu();
        if (g_language_menu != nullptr) {
            for (const LanguageChoice& choice : kLanguageChoices) {
                AppendMenuW(g_language_menu, MF_STRING, choice.command,
                            choice.label);
            }
        }
    }
    if (tools_menu != nullptr && g_language_menu != nullptr) {
        bool present = false;
        for (int index = 0; index < GetMenuItemCount(tools_menu); ++index) {
            if (GetSubMenu(tools_menu, index) == g_language_menu) {
                present = true;
                break;
            }
        }
        if (!present) {
            AppendMenuW(tools_menu, MF_SEPARATOR, 0, nullptr);
            AppendMenuW(tools_menu, MF_POPUP | MF_STRING,
                        reinterpret_cast<UINT_PTR>(g_language_menu),
                        L"&Language");
        }
    }

    if (g_table_menu == nullptr || !IsMenu(g_table_menu)) {
        g_table_menu = CreatePopupMenu();
        if (g_table_menu != nullptr) {
            AppendMenuW(g_table_menu, MF_STRING, bcmInsTable,
                        OPUSW("&Insert Table..."));
            AppendMenuW(g_table_menu, MF_STRING, imiEditTable,
                        OPUSW("&Rows and Columns..."));
            AppendMenuW(g_table_menu, MF_STRING, bcmFormatTable,
                        OPUSW("Table &Properties..."));
            AppendMenuW(g_table_menu, MF_SEPARATOR, 0, nullptr);
            AppendMenuW(g_table_menu, MF_STRING, bcmSelectTable,
                        OPUSW("&Select Table"));
            AppendMenuW(g_table_menu, MF_STRING, bcmNextCell,
                        OPUSW("Move to &Next Cell"));
            AppendMenuW(g_table_menu, MF_STRING, bcmPrevCell,
                        OPUSW("Move to Pre&vious Cell"));
            AppendMenuW(g_table_menu, MF_SEPARATOR, 0, nullptr);
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
    const int window_index = find_named(L"Window");
    if (g_table_menu != nullptr && window_index >= 0) {
        InsertMenuW(root, window_index,
                    MF_BYPOSITION | MF_POPUP | MF_STRING,
                    reinterpret_cast<UINT_PTR>(g_table_menu), L"Ta&ble");
    }

    if (g_toolbars_menu == nullptr || !IsMenu(g_toolbars_menu)) {
        g_toolbars_menu = CreatePopupMenu();
        if (g_toolbars_menu != nullptr) {
            AppendMenuW(g_toolbars_menu, MF_STRING,
                        kCmdToggleStandardToolbar, OPUSW("&Standard"));
            AppendMenuW(g_toolbars_menu, MF_STRING,
                        kCmdToggleFormattingToolbar, OPUSW("&Formatting"));
            AppendMenuW(g_toolbars_menu, MF_SEPARATOR, 0, nullptr);
            AppendMenuW(g_toolbars_menu, MF_STRING, bcmRuler,
                        OPUSW("&Ruler"));
            AppendMenuW(g_toolbars_menu, MF_STRING, bcmStatusArea,
                        OPUSW("&Status Bar"));
            AppendMenuW(g_toolbars_menu, MF_SEPARATOR, 0, nullptr);
            AppendMenuW(g_toolbars_menu, MF_STRING, bcmCustomize,
                        OPUSW("&Customize..."));
        }
    }

    const int view_index = find_named(L"View");
    HMENU view_menu = view_index >= 0 ? GetSubMenu(root, view_index) : nullptr;
    if (view_menu != nullptr && g_toolbars_menu != nullptr) {
        for (int index = GetMenuItemCount(view_menu) - 1; index >= 0;
             --index) {
            if (GetSubMenu(view_menu, index) == g_toolbars_menu ||
                find_named_in(view_menu, L"Toolbars") == index) {
                RemoveMenu(view_menu, index, MF_BYPOSITION);
            }
        }
        AppendMenuW(view_menu, MF_POPUP | MF_STRING,
                    reinterpret_cast<UINT_PTR>(g_toolbars_menu),
                    L"&Toolbars");

        if (GetMenuState(view_menu, bcmRibbon, MF_BYCOMMAND) !=
            static_cast<UINT>(-1)) {
            ModifyMenuW(view_menu, bcmRibbon,
                        MF_BYCOMMAND | MF_STRING,
                        kCmdToggleFormattingToolbar,
                        L"&Formatting Toolbar");
        }
    }

    const int normalized_window_index = find_named(L"Window");
    HMENU window_menu = normalized_window_index >= 0 ?
        GetSubMenu(root, normalized_window_index) : nullptr;
    if (window_menu != nullptr) {
        if (GetMenuState(window_menu, imiNewWnd, MF_BYCOMMAND) ==
            static_cast<UINT>(-1)) {
            AppendMenuW(window_menu, MF_STRING, imiNewWnd,
                        L"&New Window");
        }
        if (GetMenuState(window_menu, bcmArrangeWnd, MF_BYCOMMAND) ==
            static_cast<UINT>(-1)) {
            AppendMenuW(window_menu, MF_STRING, bcmArrangeWnd,
                        L"&Arrange All");
        }
        if (GetMenuState(window_menu, bcmZoomWnd, MF_BYCOMMAND) ==
            static_cast<UINT>(-1)) {
            AppendMenuW(window_menu, MF_SEPARATOR, 0, nullptr);
            AppendMenuW(window_menu, MF_STRING, bcmZoomWnd,
                        L"Ma&ximize Document");
            AppendMenuW(window_menu, MF_STRING, bcmRestoreWnd,
                        L"&Restore Document");
            AppendMenuW(window_menu, MF_STRING, bcmCloseWnd,
                        L"&Close Document");
        }
    }
}

void apply_caption_colors(HWND window) {
    using DwmSetWindowAttributeProc = HRESULT(WINAPI*)(HWND, DWORD,
                                                       LPCVOID, DWORD);
    HMODULE module = LoadLibraryExW(
        kDwmApiLibrary, nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (module == nullptr) {
        return;
    }
    const auto set_attribute = reinterpret_cast<DwmSetWindowAttributeProc>(
        GetProcAddress(module, "DwmSetWindowAttribute"));
    if (set_attribute != nullptr) {
        constexpr DWORD kDwmBorderColor = 34;
        constexpr DWORD kDwmCaptionColor = 35;
        constexpr DWORD kDwmTextColor = 36;
        constexpr DWORD kDwmCornerPreference = 33;
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
    return toolbar != nullptr ? reinterpret_cast<ToolbarState*>(
        GetWindowLongPtrW(toolbar, GWLP_USERDATA)) : nullptr;
}

int toolbar_height(HWND toolbar) {
    const ToolbarState* state = toolbar_state(toolbar);
    if (state == nullptr ||
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

int formatting_row_top(HWND toolbar, const ToolbarState& state) {
    return scale(toolbar, state.standard_visible ? 34 : 1);
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
            return RECT{x, scale(toolbar, 2), x + button,
                        scale(toolbar, 2) + button};
        }
        x += button + gap;
        previous_group = kStandardButtons[index].group;
    }
    return RECT{};
}

int format_buttons_start(HWND toolbar) {
    return scale(toolbar, 4 + 118 + 4 + 146 + 4 + 52 + 8);
}

RECT format_button_rect(HWND toolbar, const ToolbarState& state, int target) {
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
            return RECT{x, top, x + button, top + button};
        }
        x += button + gap;
        previous_group = kFormatButtons[index].group;
    }
    return RECT{};
}

int standard_buttons_end(HWND toolbar) {
    const RECT last = standard_button_rect(
        toolbar, static_cast<int>(kStandardButtons.size()) - 1);
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
    HPEN old = static_cast<HPEN>(SelectObject(dc, top_outer));
    MoveToEx(dc, rect.left, rect.bottom - 1, nullptr);
    LineTo(dc, rect.left, rect.top);
    LineTo(dc, rect.right - 1, rect.top);
    SelectObject(dc, top_inner);
    MoveToEx(dc, rect.left + 1, rect.bottom - 2, nullptr);
    LineTo(dc, rect.left + 1, rect.top + 1);
    LineTo(dc, rect.right - 2, rect.top + 1);
    SelectObject(dc, bottom_outer);
    MoveToEx(dc, rect.left, rect.bottom - 1, nullptr);
    LineTo(dc, rect.right - 1, rect.bottom - 1);
    LineTo(dc, rect.right - 1, rect.top - 1);
    SelectObject(dc, bottom_inner);
    MoveToEx(dc, rect.left + 1, rect.bottom - 2, nullptr);
    LineTo(dc, rect.right - 2, rect.bottom - 2);
    LineTo(dc, rect.right - 2, rect.top);
    SelectObject(dc, old);
    DeleteObject(top_outer);
    DeleteObject(top_inner);
    DeleteObject(bottom_outer);
    DeleteObject(bottom_inner);
}

void draw_standard_glyph(HWND toolbar, HDC dc, HDC sprite_dc,
                         const SpriteButton& spec, RECT rect, bool sunken) {
    const int glyph = scale(toolbar, 20);
    const int offset = sunken ? scale(toolbar, 1) : 0;
    const int x = rect.left + (rect.right - rect.left - glyph) / 2 + offset;
    const int y = rect.top + (rect.bottom - rect.top - glyph) / 2 + offset;
    SetStretchBltMode(dc, COLORONCOLOR);
    StretchBlt(dc, x, y, glyph, glyph, sprite_dc,
               spec.sprite * kSpriteCell, 0, kSpriteCell, kSpriteCell,
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
        if (glyph == FormatGlyph::align_left) {
            x2 -= short_by;
        } else if (glyph == FormatGlyph::align_center) {
            x1 += short_by / 2;
            x2 -= short_by / 2;
        } else if (glyph == FormatGlyph::align_right) {
            x1 += short_by;
        } else if (glyph == FormatGlyph::align_justify && row == 3) {
            x2 -= width / 5;
        }
        MoveToEx(dc, x1, y, nullptr);
        LineTo(dc, x2, y);
    }
}

void draw_format_glyph(HWND toolbar, HDC dc, const FormatButton& spec,
                       RECT rect, bool sunken, HDC sprite_dc,
                       COLORREF text_color) {
    const int offset = sunken ? scale(toolbar, 1) : 0;
    RECT area{rect.left + scale(toolbar, 4) + offset,
              rect.top + scale(toolbar, 4) + offset,
              rect.right - scale(toolbar, 4) + offset,
              rect.bottom - scale(toolbar, 4) + offset};
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, RGB(0, 0, 0));
    HFONT old_font = nullptr;
    LOGFONTW logical{};
    logical.lfHeight = -(area.bottom - area.top);
    lstrcpyW(logical.lfFaceName, kArialFace);
    if (spec.glyph == FormatGlyph::bold) {
        logical.lfWeight = FW_BOLD;
    } else if (spec.glyph == FormatGlyph::italic) {
        logical.lfItalic = TRUE;
    } else if (spec.glyph == FormatGlyph::underline) {
        logical.lfUnderline = TRUE;
    }
    if (spec.glyph == FormatGlyph::bold ||
        spec.glyph == FormatGlyph::italic ||
        spec.glyph == FormatGlyph::underline ||
        spec.glyph == FormatGlyph::color) {
        HFONT font = CreateFontIndirectW(&logical);
        old_font = static_cast<HFONT>(SelectObject(dc, font));
        DrawTextW(dc, spec.glyph == FormatGlyph::color ? kFormatGlyphColor :
                  spec.glyph == FormatGlyph::bold ? kFormatGlyphBold :
                  spec.glyph == FormatGlyph::italic ? kFormatGlyphItalic :
                  kFormatGlyphUnderline,
                  1, &area, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        SelectObject(dc, old_font);
        DeleteObject(font);
        if (spec.glyph == FormatGlyph::color) {
            HBRUSH swatch = CreateSolidBrush(text_color);
            RECT swatch_rect{area.left + 1, area.bottom - 3,
                             area.right - 1, area.bottom};
            FillRect(dc, &swatch_rect, swatch);
            DeleteObject(swatch);
        }
        return;
    }
    if (spec.glyph == FormatGlyph::align_left ||
        spec.glyph == FormatGlyph::align_center ||
        spec.glyph == FormatGlyph::align_right ||
        spec.glyph == FormatGlyph::align_justify) {
        draw_alignment(dc, area, spec.glyph);
        return;
    }
    if (spec.glyph == FormatGlyph::numbered ||
        spec.glyph == FormatGlyph::bullets) {
        for (int row = 0; row < 3; ++row) {
            if (spec.glyph == FormatGlyph::numbered) {
                wchar_t number[2]{static_cast<wchar_t>(L'1' + row), L'\0'};
                TextOutW(dc, area.left, area.top + row * 5 - 1, number, 1);
            } else {
                HBRUSH old_brush = static_cast<HBRUSH>(SelectObject(
                    dc, GetStockObject(BLACK_BRUSH)));
                Ellipse(dc, area.left + 1, area.top + row * 5 + 1,
                        area.left + 4, area.top + row * 5 + 4);
                SelectObject(dc, old_brush);
            }
            MoveToEx(dc, area.left + 6, area.top + row * 5 + 2, nullptr);
            LineTo(dc, area.right, area.top + row * 5 + 2);
        }
        return;
    }
    if (spec.glyph == FormatGlyph::indent_left ||
        spec.glyph == FormatGlyph::indent_right) {
        for (int row = 0; row < 3; ++row) {
            MoveToEx(dc, area.left + 6, area.top + 2 + row * 5, nullptr);
            LineTo(dc, area.right, area.top + 2 + row * 5);
        }
        const bool right = spec.glyph == FormatGlyph::indent_right;
        POINT triangle[3]{{right ? area.left + 1 : area.left + 6, area.top + 6},
                          {right ? area.left + 6 : area.left + 1, area.top + 3},
                          {right ? area.left + 6 : area.left + 1, area.top + 9}};
        HBRUSH brush = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
        HBRUSH old_brush = static_cast<HBRUSH>(SelectObject(dc, brush));
        Polygon(dc, triangle, 3);
        SelectObject(dc, old_brush);
        return;
    }
    if (spec.glyph == FormatGlyph::table && sprite_dc != nullptr) {
        const int glyph = area.bottom - area.top;
        StretchBlt(dc, area.left, area.top, glyph, glyph, sprite_dc,
                   12 * kSpriteCell, 0, kSpriteCell, kSpriteCell, SRCCOPY);
        return;
    }
    if (spec.glyph == FormatGlyph::borders) {
        Rectangle(dc, area.left + 1, area.top + 1,
                  area.right - 1, area.bottom - 1);
        const int middle_x = (area.left + area.right) / 2;
        const int middle_y = (area.top + area.bottom) / 2;
        MoveToEx(dc, middle_x, area.top + 1, nullptr);
        LineTo(dc, middle_x, area.bottom - 1);
        MoveToEx(dc, area.left + 1, middle_y, nullptr);
        LineTo(dc, area.right - 1, middle_y);
    }
}

void paint_toolbar(HWND toolbar, ToolbarState& state) {
    PAINTSTRUCT paint{};
    HDC dc = BeginPaint(toolbar, &paint);
    RECT client{};
    GetClientRect(toolbar, &client);
    HBRUSH face = CreateSolidBrush(kButtonFace);
    FillRect(dc, &client, face);
    DeleteObject(face);

    if (state.standard_visible && state.formatting_visible) {
        HPEN separator = CreatePen(PS_SOLID, 1, kButtonShadow);
        HPEN highlight = CreatePen(PS_SOLID, 1, kButtonHighlight);
        HPEN old_pen = static_cast<HPEN>(SelectObject(dc, separator));
        const int row_line = scale(toolbar, 32);
        MoveToEx(dc, 0, row_line, nullptr);
        LineTo(dc, client.right, row_line);
        SelectObject(dc, highlight);
        MoveToEx(dc, 0, row_line + 1, nullptr);
        LineTo(dc, client.right, row_line + 1);
        SelectObject(dc, old_pen);
        DeleteObject(separator);
        DeleteObject(highlight);
    }

    HDC sprite_dc = CreateCompatibleDC(dc);
    HBITMAP old_bitmap = state.sprite != nullptr ?
        static_cast<HBITMAP>(SelectObject(sprite_dc, state.sprite)) : nullptr;
    if (state.standard_visible) {
        for (int index = 0;
             index < static_cast<int>(kStandardButtons.size()); ++index) {
            RECT rect = standard_button_rect(toolbar, index);
            const bool down =
                (state.pressed.hit && !state.pressed.format &&
                 state.pressed.index == index) || state.standard_latched[index];
            draw_classic_edge(dc, rect, down);
            if (state.sprite != nullptr) {
                draw_standard_glyph(toolbar, dc, sprite_dc,
                                    kStandardButtons[index], rect, down);
            }
        }
    }
    if (state.formatting_visible) {
        for (int index = 0;
             index < static_cast<int>(kFormatButtons.size()); ++index) {
            RECT rect = format_button_rect(toolbar, state, index);
            const bool down =
                (state.pressed.hit && state.pressed.format &&
                 state.pressed.index == index) || state.format_latched[index];
            draw_classic_edge(dc, rect, down);
            draw_format_glyph(toolbar, dc, kFormatButtons[index], rect, down,
                               state.sprite != nullptr ? sprite_dc : nullptr,
                               state.text_color);
        }
    }
    if (old_bitmap != nullptr) {
        SelectObject(sprite_dc, old_bitmap);
    }
    DeleteDC(sprite_dc);
    EndPaint(toolbar, &paint);
}

HitResult hit_test(HWND toolbar, const ToolbarState& state, POINT point) {
    if (state.standard_visible) {
        for (int index = 0;
             index < static_cast<int>(kStandardButtons.size()); ++index) {
            RECT rect = standard_button_rect(toolbar, index);
            if (PtInRect(&rect, point)) {
                return {true, false, index, kStandardButtons[index].command};
            }
        }
    }
    if (state.formatting_visible) {
        for (int index = 0;
             index < static_cast<int>(kFormatButtons.size()); ++index) {
            RECT rect = format_button_rect(toolbar, state, index);
            if (PtInRect(&rect, point)) {
                return {true, true, index, kFormatButtons[index].command};
            }
        }
    }
    return {};
}

bool is_toolbar_descendant(HWND toolbar, HWND candidate) {
    return candidate == toolbar || IsChild(toolbar, candidate) != FALSE;
}

std::string combo_item(HWND combo, int index) {
    const LRESULT length = SendMessageA(combo, CB_GETLBTEXTLEN, index, 0);
    if (length == CB_ERR || length < 0) {
        return {};
    }
    std::vector<char> text(static_cast<std::size_t>(length) + 1);
    SendMessageA(combo, CB_GETLBTEXT, index,
                 reinterpret_cast<LPARAM>(text.data()));
    return text.data();
}

bool combo_contains(HWND combo, const char* value) {
    return SendMessageA(combo, CB_FINDSTRINGEXACT, static_cast<WPARAM>(-1),
                        reinterpret_cast<LPARAM>(value)) != CB_ERR;
}

struct ComboEnumeration {
    HWND toolbar;
    std::vector<HWND> combos;
};

BOOL CALLBACK collect_original_combos(HWND candidate, LPARAM parameter) {
    auto& enumeration = *reinterpret_cast<ComboEnumeration*>(parameter);
    if (is_toolbar_descendant(enumeration.toolbar, candidate)) {
        return TRUE;
    }
    WCHAR class_name[64]{};
    GetClassNameW(candidate, class_name,
                  static_cast<int>(sizeof(class_name) / sizeof(class_name[0])));
    if (lstrcmpiW(class_name, kComboBoxClass) == 0) {
        enumeration.combos.push_back(candidate);
    }
    return TRUE;
}

void locate_source_combos(HWND toolbar, ToolbarState& state) {
    ComboEnumeration enumeration{toolbar, {}};
    EnumChildWindows(GetParent(toolbar), collect_original_combos,
                     reinterpret_cast<LPARAM>(&enumeration));
    for (HWND combo : enumeration.combos) {
        if (combo_contains(combo, "Courier New") &&
            combo_contains(combo, "Arial")) {
            state.source_font = combo;
        } else if (combo_contains(combo, "24") &&
                   combo_contains(combo, "72")) {
            state.source_size = combo;
        } else if (combo_contains(combo, "Normal") ||
                   SendMessageA(combo, CB_GETCOUNT, 0, 0) > 0) {
            state.source_style = combo;
        }
    }
}

std::wstring wide_from_ansi(const std::string& text) {
    if (text.empty()) {
        return {};
    }
    const int count = MultiByteToWideChar(CP_ACP, 0, text.c_str(), -1,
                                          nullptr, 0);
    std::vector<wchar_t> wide(static_cast<std::size_t>(count));
    MultiByteToWideChar(CP_ACP, 0, text.c_str(), -1, wide.data(), count);
    return wide.data();
}

std::string ansi_from_wide(const std::wstring& text) {
    if (text.empty()) {
        return {};
    }
    const int count = WideCharToMultiByte(CP_ACP, 0, text.c_str(), -1,
                                          nullptr, 0, nullptr, nullptr);
    std::vector<char> ansi(static_cast<std::size_t>(count));
    WideCharToMultiByte(CP_ACP, 0, text.c_str(), -1, ansi.data(), count,
                        nullptr, nullptr);
    return ansi.data();
}

bool combo_or_child_has_focus(HWND combo) {
    const HWND focus = GetFocus();
    return focus == combo || (focus != nullptr && IsChild(combo, focus));
}

void sync_combo(HWND mirror, HWND source, int& copied_count) {
    if (mirror == nullptr || source == nullptr || !IsWindow(source)) {
        return;
    }
    const int count = static_cast<int>(SendMessageA(source, CB_GETCOUNT, 0, 0));
    if (count >= 0 && count != copied_count) {
        std::wstring mirror_text;
        const int mirror_length = GetWindowTextLengthW(mirror);
        mirror_text.resize(static_cast<std::size_t>(mirror_length) + 1);
        GetWindowTextW(mirror, &mirror_text[0], mirror_length + 1);
        SendMessageW(mirror, CB_RESETCONTENT, 0, 0);
        for (int index = 0; index < count; ++index) {
            const std::wstring item = wide_from_ansi(combo_item(source, index));
            SendMessageW(mirror, CB_ADDSTRING, 0,
                         reinterpret_cast<LPARAM>(item.c_str()));
        }
        SetWindowTextW(mirror, mirror_text.c_str());
        copied_count = count;
    }
    if (!combo_or_child_has_focus(mirror)) {
        const LRESULT selection = SendMessageA(source, CB_GETCURSEL, 0, 0);
        if (selection != CB_ERR && selection < count) {
            SendMessageW(mirror, CB_SETCURSEL, selection, 0);
        } else {
            const int length = GetWindowTextLengthA(source);
            std::vector<char> text(static_cast<std::size_t>(length) + 1);
            GetWindowTextA(source, text.data(), static_cast<int>(text.size()));
            const std::wstring source_text = wide_from_ansi(text.data());
            if (!source_text.empty()) {
                SetWindowTextW(mirror, source_text.c_str());
            }
        }
        SendMessageW(mirror, CB_SETEDITSEL, 0, MAKELPARAM(-1, 0));
    }
}

void sync_mirrors(HWND toolbar, ToolbarState& state) {
    if (state.source_font == nullptr || !IsWindow(state.source_font) ||
        state.source_size == nullptr || !IsWindow(state.source_size) ||
        state.source_style == nullptr || !IsWindow(state.source_style)) {
        locate_source_combos(toolbar, state);
    }
    sync_combo(state.style_combo, state.source_style,
               state.copied_style_count);
    sync_combo(state.font_combo, state.source_font, state.copied_font_count);
    sync_combo(state.size_combo, state.source_size, state.copied_size_count);
}

BOOL CALLBACK find_document_pane(HWND candidate, LPARAM parameter) {
    WCHAR class_name[64]{};
    GetClassNameW(candidate, class_name,
                  static_cast<int>(sizeof(class_name) / sizeof(class_name[0])));
    if (lstrcmpiW(class_name, kDocumentPaneClass) == 0) {
        *reinterpret_cast<HWND*>(parameter) = candidate;
        return FALSE;
    }
    return TRUE;
}

void restore_document_focus(HWND source) {
    HWND pane = nullptr;
    EnumChildWindows(GetAncestor(source, GA_ROOT), find_document_pane,
                     reinterpret_cast<LPARAM>(&pane));
    if (pane != nullptr) {
        SetFocus(pane);
    }
}

void restore_document_focus_from_root(HWND root) {
    HWND pane = nullptr;
    EnumChildWindows(root, find_document_pane,
                     reinterpret_cast<LPARAM>(&pane));
    if (pane != nullptr) {
        SetFocus(pane);
    }
}

HBITMAP create_color_menu_swatch(HWND owner, COLORREF color,
                                 bool automatic) {
    HDC screen = GetDC(owner);
    if (screen == nullptr) {
        return nullptr;
    }
    const int size = scale(owner, 14);
    HBITMAP bitmap = CreateCompatibleBitmap(screen, size, size);
    HDC memory = CreateCompatibleDC(screen);
    HBITMAP previous = bitmap != nullptr ? static_cast<HBITMAP>(
        SelectObject(memory, bitmap)) : nullptr;
    if (bitmap != nullptr) {
        RECT bounds{0, 0, size, size};
        FillRect(memory, &bounds, GetSysColorBrush(COLOR_MENU));
        RECT swatch{scale(owner, 2), scale(owner, 2),
                    size - scale(owner, 1), size - scale(owner, 1)};
        HBRUSH fill = CreateSolidBrush(color);
        FillRect(memory, &swatch, fill);
        DeleteObject(fill);
        FrameRect(memory, &swatch,
                  static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));
        if (automatic) {
            HPEN pen = CreatePen(PS_SOLID, 1, RGB(255, 255, 255));
            HPEN old_pen = static_cast<HPEN>(SelectObject(memory, pen));
            MoveToEx(memory, swatch.left + 1, swatch.bottom - 2, nullptr);
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
    if (state == nullptr) {
        return;
    }
    struct Choice {
        int index;
        COLORREF color;
        const WCHAR* name;
    };
    const std::array<Choice, 9> choices{{
        {0, RGB(0, 0, 0), OPUSW("Automatic")},
        {1, RGB(0, 0, 0), OPUSW("Black")},
        {2, RGB(0, 0, 255), OPUSW("Blue")},
        {3, RGB(0, 255, 255), OPUSW("Cyan")},
        {4, RGB(0, 128, 0), OPUSW("Green")},
        {5, RGB(255, 0, 255), OPUSW("Magenta")},
        {6, RGB(255, 0, 0), OPUSW("Red")},
        {7, RGB(255, 255, 0), OPUSW("Yellow")},
        {8, RGB(255, 255, 255), OPUSW("White")},
    }};

    HMENU popup = CreatePopupMenu();
    if (popup == nullptr) {
        return;
    }
    std::vector<HBITMAP> swatches;
    swatches.reserve(choices.size());
    for (const auto& choice : choices) {
        const UINT command = kCmdTextColorBase + choice.index;
        AppendMenuW(popup, MF_STRING, command, choice.name);
        HBITMAP swatch = create_color_menu_swatch(
            toolbar, choice.color, choice.index == 0);
        swatches.push_back(swatch);
        if (swatch != nullptr) {
            SetMenuItemBitmaps(popup, command, MF_BYCOMMAND, swatch, swatch);
        }
        if (choice.index == state->text_color_index) {
            CheckMenuItem(popup, command, MF_BYCOMMAND | MF_CHECKED);
        }
    }
    style_menu_tree(popup);

    RECT anchor = format_button_rect(toolbar, *state, 3);
    POINT point{anchor.left, anchor.bottom};
    ClientToScreen(toolbar, &point);
    const UINT selected = TrackPopupMenu(
        popup, TPM_RETURNCMD | TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RIGHTBUTTON,
        point.x, point.y, 0, app, nullptr);
    if (selected >= kCmdTextColorBase &&
        selected < kCmdTextColorBase + choices.size()) {
        const int index = static_cast<int>(selected - kCmdTextColorBase);
        HWND pane = nullptr;
        EnumChildWindows(app, find_document_pane,
                         reinterpret_cast<LPARAM>(&pane));
        if (pane != nullptr &&
            OpusApplyWin95TextColor(pane, choices[index].index)) {
            state->text_color_index = choices[index].index;
            state->text_color = choices[index].color;
            InvalidateRect(toolbar, nullptr, FALSE);
            UpdateWindow(toolbar);
        } else {
            MessageBeep(MB_ICONEXCLAMATION);
        }
    }
    DestroyMenu(popup);
    for (HBITMAP swatch : swatches) {
        if (swatch != nullptr) {
            DeleteObject(swatch);
        }
    }
    restore_document_focus_from_root(app);
}

void forward_combo(HWND mirror, HWND source, int notification,
                   bool& edit_dirty) {
    if (mirror == nullptr || source == nullptr || !IsWindow(source)) {
        return;
    }
    if (notification == CBN_SELCHANGE || notification == CBN_SELENDOK) {
        edit_dirty = false;
    }
    std::wstring wide;
    const LRESULT mirror_selection = SendMessageW(mirror, CB_GETCURSEL, 0, 0);
    if (mirror_selection != CB_ERR) {
        const LRESULT length = SendMessageW(
            mirror, CB_GETLBTEXTLEN, mirror_selection, 0);
        wide.resize(static_cast<std::size_t>(length) + 1, L'\0');
        SendMessageW(mirror, CB_GETLBTEXT, mirror_selection,
                     reinterpret_cast<LPARAM>(&wide[0]));
    } else {
        const int length = GetWindowTextLengthW(mirror);
        wide.resize(static_cast<std::size_t>(length) + 1, L'\0');
        GetWindowTextW(mirror, &wide[0], length + 1);
    }
    const std::string text = ansi_from_wide(wide.c_str());
    const LRESULT selection = SendMessageA(
        source, CB_FINDSTRINGEXACT, static_cast<WPARAM>(-1),
        reinterpret_cast<LPARAM>(text.c_str()));
    if (selection != CB_ERR) {
        SendMessageA(source, CB_SETCURSEL, selection, 0);
    }
    SetWindowTextA(source, text.c_str());
    const HWND parent = GetParent(source);
    const int control_id = GetDlgCtrlID(source);
    if (notification == CBN_SELCHANGE) {
        SendMessageW(parent, WM_COMMAND,
                     MAKEWPARAM(control_id, CBN_SELCHANGE),
                     reinterpret_cast<LPARAM>(source));
    } else if (notification == CBN_SELENDOK) {
        SendMessageW(parent, WM_COMMAND,
                     MAKEWPARAM(control_id, CBN_SELENDOK),
                     reinterpret_cast<LPARAM>(source));
        restore_document_focus(source);
    } else if (notification == CBN_EDITCHANGE) {
        // A dropdown selection can emit EDITCHANGE too; only free-form text (no
        // selected list item) needs the legacy control's focus-loss commit
        // path.
        edit_dirty = SendMessageW(mirror, CB_GETCURSEL, 0, 0) == CB_ERR;
        SendMessageW(parent, WM_COMMAND,
                     MAKEWPARAM(control_id, CBN_EDITCHANGE),
                     reinterpret_cast<LPARAM>(source));
    } else if (notification == CBN_DROPDOWN) {
        SendMessageW(parent, WM_COMMAND,
                     MAKEWPARAM(control_id, CBN_DROPDOWN),
                     reinterpret_cast<LPARAM>(source));
    } else if (notification == CBN_KILLFOCUS && edit_dirty) {
        SendMessageW(parent, WM_COMMAND,
                     MAKEWPARAM(control_id, CBN_KILLFOCUS),
                     reinterpret_cast<LPARAM>(source));
        edit_dirty = false;
    }
}

void position_combos(HWND toolbar, ToolbarState& state) {
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
    ShowWindow(state.style_combo,
               state.formatting_visible ? SW_SHOWNOACTIVATE : SW_HIDE);
    ShowWindow(state.font_combo,
               state.formatting_visible ? SW_SHOWNOACTIVATE : SW_HIDE);
    ShowWindow(state.size_combo,
               state.formatting_visible ? SW_SHOWNOACTIVATE : SW_HIDE);
    MoveWindow(state.style_combo, x_style, y, width_style, height, TRUE);
    MoveWindow(state.font_combo, x_font, y, width_font, height, TRUE);
    MoveWindow(state.size_combo, x_size, y, width_size, height, TRUE);

    if (state.zoom_combo != nullptr) {
        ShowWindow(state.zoom_combo,
                   state.standard_visible ? SW_SHOWNOACTIVATE : SW_HIDE);
        MoveWindow(state.zoom_combo, standard_buttons_end(toolbar),
                   scale(toolbar, 3), scale(toolbar, 82), height, TRUE);
    }
}

HWND create_combo(HWND toolbar, UINT id) {
    HWND combo = CreateWindowExW(
        0, kComboBoxWindowClass, nullptr,
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL |
            CBS_DROPDOWN | CBS_AUTOHSCROLL,
        0, 0, 10, 200, toolbar,
        reinterpret_cast<HMENU>(static_cast<UINT_PTR>(id)),
        GetModuleHandleW(nullptr), nullptr);
    set_window_classic(combo);
    return combo;
}

HWND create_zoom_combo(HWND toolbar) {
    HWND combo = CreateWindowExW(
        0, kComboBoxWindowClass, nullptr,
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL |
            CBS_DROPDOWN | CBS_AUTOHSCROLL,
        0, 0, 10, 200, toolbar,
        reinterpret_cast<HMENU>(static_cast<UINT_PTR>(kComboZoom)),
        GetModuleHandleW(nullptr), nullptr);
    set_window_classic(combo);
    SendMessageW(combo, CB_ADDSTRING, 0,
                 reinterpret_cast<LPARAM>(kZoom100));
    SendMessageW(combo, CB_ADDSTRING, 0,
                 reinterpret_cast<LPARAM>(kZoomPageWidth));
    SendMessageW(combo, CB_ADDSTRING, 0,
                 reinterpret_cast<LPARAM>(kZoomWholePage));
    SendMessageW(combo, CB_ADDSTRING, 0,
                 reinterpret_cast<LPARAM>(kZoomPrintPreview));
    SendMessageW(combo, CB_SETCURSEL, 0, 0);
    return combo;
}

void update_zoom_combo(HWND pane, int percent) {
    const HWND app = GetAncestor(pane, GA_ROOT);
    const HWND toolbar = FindWindowExW(app, nullptr, kToolbarClass, nullptr);
    ToolbarState* state = toolbar_state(toolbar);
    if (state == nullptr || state->zoom_combo == nullptr) {
        return;
    }
    const std::wstring text = std::to_wstring(percent) + L"%";
    SendMessageW(state->zoom_combo, CB_SETCURSEL,
                 static_cast<WPARAM>(-1), 0);
    SetWindowTextW(state->zoom_combo, text.c_str());
}

void update_toolbar_menu_checks(HWND app, const ToolbarState& state) {
    const UINT standard = MF_BYCOMMAND |
        (state.standard_visible ? MF_CHECKED : MF_UNCHECKED);
    const UINT formatting = MF_BYCOMMAND |
        (state.formatting_visible ? MF_CHECKED : MF_UNCHECKED);
    if (g_toolbars_menu != nullptr && IsMenu(g_toolbars_menu)) {
        CheckMenuItem(g_toolbars_menu, kCmdToggleStandardToolbar, standard);
        CheckMenuItem(g_toolbars_menu, kCmdToggleFormattingToolbar,
                      formatting);
    }
    HMENU root = GetMenu(app);
    if (root != nullptr) {
        CheckMenuItem(root, kCmdToggleStandardToolbar, standard);
        CheckMenuItem(root, kCmdToggleFormattingToolbar, formatting);
    }
}

void request_toolbar_layout(HWND app, HWND toolbar) {
    if (app == nullptr || toolbar == nullptr) {
        return;
    }
    RECT client{};
    GetClientRect(app, &client);
    const int height = toolbar_height(toolbar);
    ShowWindow(toolbar, height > 0 ? SW_SHOWNOACTIVATE : SW_HIDE);
    MoveWindow(toolbar, 0, 0, client.right - client.left, height, TRUE);
    const UINT size_type = IsZoomed(app) ? SIZE_MAXIMIZED : SIZE_RESTORED;
    SendMessageW(app, WM_SIZE, size_type,
                 MAKELPARAM(client.right - client.left,
                            client.bottom - client.top));
    InvalidateRect(app, nullptr, TRUE);
    DrawMenuBar(app);
}

void toggle_toolbar_row(HWND app, HWND toolbar, bool standard) {
    ToolbarState* state = toolbar_state(toolbar);
    if (state == nullptr) {
        return;
    }
    if (standard) {
        state->standard_visible = !state->standard_visible;
    } else {
        state->formatting_visible = !state->formatting_visible;
    }
    state->pressed = {};
    position_combos(toolbar, *state);
    update_toolbar_menu_checks(app, *state);
    request_toolbar_layout(app, toolbar);
}

void show_toolbar_context_menu(HWND toolbar, POINT screen_point) {
    ToolbarState* state = toolbar_state(toolbar);
    if (state == nullptr) {
        return;
    }
    HMENU popup = CreatePopupMenu();
    if (popup == nullptr) {
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
    AppendMenuW(popup, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(popup, MF_STRING, bcmCustomize, OPUSW("&Customize..."));
    style_menu_tree(popup);
    const UINT command = TrackPopupMenu(
        popup, TPM_RETURNCMD | TPM_RIGHTBUTTON,
        screen_point.x, screen_point.y, 0, GetParent(toolbar), nullptr);
    DestroyMenu(popup);
    if (command != 0) {
        PostMessageW(GetParent(toolbar), WM_COMMAND,
                     MAKEWPARAM(command, 0), 0);
    }
}

WNDPROC original_pane_proc(HWND pane) {
    return reinterpret_cast<WNDPROC>(
        GetPropW(pane, kOriginalPaneProcProperty));
}

void redraw_vertical_ruler(HWND pane) {
    RECT ruler{};
    GetClientRect(pane, &ruler);
    ruler.right = std::min(ruler.right, ruler.left + scale(pane, 22));
    InvalidateRect(pane, &ruler, FALSE);
    UpdateWindow(pane);
}

bool vertical_margin_marker_at(HWND pane, POINT point, bool* top_marker,
                               int* page_offset) {
    RECT client{};
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
    const std::array<int, 3> offsets{
        has_previous ? -page_step : 0,
        0,
        has_next ? page_step : 0};
    int closest_distance = hit_radius + 1;
    bool closest_top = false;
    int closest_offset = 0;
    for (std::size_t index = 0; index < offsets.size(); ++index) {
        const int offset = offsets[index];
        if ((index == 0 && !has_previous) ||
            (index == 2 && !has_next)) {
            continue;
        }
        const int top_distance = std::abs(
            point.y - (page_top + offset + top_margin));
        const int bottom_distance = std::abs(
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
    if (page_offset != nullptr) {
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
    const int minimum_text_height = std::max(
        scale(pane, 12), std::max(1, pixels_per_inch / 4));
    if (g_vertical_ruler_drag.top) {
        const int bottom_y = page_bottom - bottom_margin;
        y = std::max(page_top,
                     std::min(y, bottom_y - minimum_text_height));
    } else {
        const int top_y = page_top + top_margin;
        y = std::max(top_y + minimum_text_height,
                     std::min(y, page_bottom));
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
    g_vertical_ruler_drag = {};
    redraw_vertical_ruler(pane);
}

PageSnapshot* find_page_snapshot(HWND pane, int page_index, int width,
                                 int height, int zoom_percent) {
    for (auto& snapshot : g_page_snapshots) {
        if (snapshot.pane == pane && snapshot.page_index == page_index &&
            snapshot.width == width && snapshot.height == height &&
            snapshot.zoom_percent == zoom_percent) {
            snapshot.last_used = GetTickCount64();
            return &snapshot;
        }
    }
    return nullptr;
}

void prune_page_snapshots(HWND pane) {
    std::size_t pane_count = 0;
    for (const auto& snapshot : g_page_snapshots) {
        if (snapshot.pane == pane) {
            ++pane_count;
        }
    }
    while (pane_count > 4) {
        auto oldest = g_page_snapshots.end();
        for (auto iterator = g_page_snapshots.begin();
             iterator != g_page_snapshots.end(); ++iterator) {
            if (iterator->pane == pane &&
                (oldest == g_page_snapshots.end() ||
                 iterator->last_used < oldest->last_used)) {
                oldest = iterator;
            }
        }
        if (oldest == g_page_snapshots.end()) {
            break;
        }
        if (oldest->bitmap != nullptr) {
            DeleteObject(oldest->bitmap);
        }
        g_page_snapshots.erase(oldest);
        --pane_count;
    }
}

void clear_page_snapshots(HWND pane) {
    for (auto iterator = g_page_snapshots.begin();
         iterator != g_page_snapshots.end();) {
        if (iterator->pane == pane) {
            if (iterator->bitmap != nullptr) {
                DeleteObject(iterator->bitmap);
            }
            iterator = g_page_snapshots.erase(iterator);
        } else {
            ++iterator;
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
    if (snapshot == nullptr) {
        PageSnapshot created{};
        created.pane = pane;
        created.page_index = page_index;
        created.zoom_percent = zoom_percent;
        created.width = width;
        created.height = height;
        created.bitmap = CreateCompatibleBitmap(dc, width, height);
        created.last_used = GetTickCount64();
        if (created.bitmap == nullptr) {
            return;
        }
        HDC memory = CreateCompatibleDC(dc);
        HBITMAP previous = static_cast<HBITMAP>(
            SelectObject(memory, created.bitmap));
        RECT background{0, 0, width, height};
        FillRect(memory, &background,
                 static_cast<HBRUSH>(GetStockObject(WHITE_BRUSH)));
        SelectObject(memory, previous);
        DeleteDC(memory);
        g_page_snapshots.push_back(created);
        snapshot = &g_page_snapshots.back();
        prune_page_snapshots(pane);
        snapshot = find_page_snapshot(
            pane, page_index, width, height, zoom_percent);
    }
    if (snapshot == nullptr || snapshot->bitmap == nullptr) {
        return;
    }

    RECT client{};
    RECT page{page_left, page_top, page_right, page_bottom};
    RECT visible{};
    GetClientRect(pane, &client);
    if (!IntersectRect(&visible, &page, &client)) {
        return;
    }
    HDC memory = CreateCompatibleDC(dc);
    HBITMAP previous = static_cast<HBITMAP>(
        SelectObject(memory, snapshot->bitmap));
    BitBlt(memory, visible.left - page_left, visible.top - page_top,
           visible.right - visible.left, visible.bottom - visible.top,
           dc, visible.left, visible.top, SRCCOPY);
    SelectObject(memory, previous);
    DeleteDC(memory);
}

void draw_neighbor_sheet(HDC dc, const RECT& sheet, const RECT& client,
                         const PageSnapshot* snapshot) {
    RECT visible{};
    if (!IntersectRect(&visible, &sheet, &client)) {
        return;
    }

    RECT shadow = sheet;
    OffsetRect(&shadow, 3, 3);
    RECT visible_shadow{};
    if (IntersectRect(&visible_shadow, &shadow, &client)) {
        HBRUSH shadow_brush = CreateSolidBrush(RGB(96, 96, 96));
        FillRect(dc, &visible_shadow, shadow_brush);
        DeleteObject(shadow_brush);
    }

    if (snapshot != nullptr && snapshot->bitmap != nullptr) {
        HDC memory = CreateCompatibleDC(dc);
        HBITMAP previous = static_cast<HBITMAP>(
            SelectObject(memory, snapshot->bitmap));
        BitBlt(dc, visible.left, visible.top,
               visible.right - visible.left, visible.bottom - visible.top,
               memory, visible.left - sheet.left, visible.top - sheet.top,
               SRCCOPY);
        SelectObject(memory, previous);
        DeleteDC(memory);
    } else {
        HBRUSH page_brush = CreateSolidBrush(RGB(255, 255, 255));
        FillRect(dc, &visible, page_brush);
        DeleteObject(page_brush);
    }
    HBRUSH border_brush = CreateSolidBrush(RGB(96, 96, 96));
    FrameRect(dc, &sheet, border_brush);
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

    RECT client{};
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
        const RECT sheet{
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
    const RECT current{page_left, page_top, page_right, page_bottom};
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

    RECT client{};
    GetClientRect(pane, &client);
    const int page_height = page_bottom - page_top;
    if (page_right <= page_left || page_height <= 0) {
        return;
    }
    const int page_step = page_height + page_gap;
    const int page_index = OpusGetWin95CurrentPageIndex(pane);
    const int zoom_percent = OpusGetWin95ZoomPercent(pane);
    if (has_previous) {
        RECT previous{page_left, page_top - page_step,
                      page_right, page_bottom - page_step};
        draw_neighbor_sheet(
            dc, previous, client,
            find_page_snapshot(pane, page_index - 1,
                               page_right - page_left, page_height,
                               zoom_percent));
    }
    if (has_next) {
        RECT next{page_left, page_top + page_step,
                  page_right, page_bottom + page_step};
        draw_neighbor_sheet(
            dc, next, client,
            find_page_snapshot(pane, page_index + 1,
                               page_right - page_left, page_height,
                               zoom_percent));
    }
}

void draw_vertical_ruler(HWND pane, HDC dc) {
    RECT client{};
    GetClientRect(pane, &client);
    const int width = scale(pane, 22);
    RECT ruler{client.left, client.top,
               std::min(client.right, client.left + width), client.bottom};
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
    const std::array<int, 3> offsets{
        has_previous ? -page_step : 0,
        0,
        has_next ? page_step : 0};
    HBRUSH active_brush = CreateSolidBrush(RGB(255, 255, 255));
    for (std::size_t index = 0; index < offsets.size(); ++index) {
        if ((index == 0 && !has_previous) ||
            (index == 2 && !has_next)) {
            continue;
        }
        const int instance_top = page_top + offsets[index];
        const int instance_bottom = page_bottom + offsets[index];
        RECT writable{ruler.left,
                      instance_top + displayed_top_margin,
                      ruler.right,
                      instance_bottom - displayed_bottom_margin};
        writable.top = std::max(ruler.top, writable.top);
        writable.bottom = std::min(ruler.bottom, writable.bottom);
        if (writable.bottom > writable.top) {
            FillRect(dc, &writable, active_brush);
            DrawEdge(dc, &writable, EDGE_SUNKEN, BF_TOP | BF_BOTTOM);
        }
    }
    DeleteObject(active_brush);
    DrawEdge(dc, &ruler, EDGE_RAISED, BF_LEFT | BF_RIGHT);

    HPEN active_pen = CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
    HPEN subdued_pen = CreatePen(PS_SOLID, 1, RGB(96, 96, 96));
    HPEN previous_pen = static_cast<HPEN>(SelectObject(dc, active_pen));
    HFONT previous_font = static_cast<HFONT>(
        SelectObject(dc, GetStockObject(DEFAULT_GUI_FONT)));
    const int previous_bk = SetBkMode(dc, TRANSPARENT);
    const COLORREF previous_text = SetTextColor(dc, RGB(0, 0, 0));
    const int eighth = std::max(1, pixels_per_inch / 8);
    for (std::size_t index = 0; index < offsets.size(); ++index) {
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
            MoveToEx(dc, ruler.right - scale(pane, 2) - length, y, nullptr);
            LineTo(dc, ruler.right - scale(pane, 2), y);
            if (tick_index > 0 && tick_index % 8 == 0) {
                const std::wstring label = std::to_wstring(tick_index / 8);
                SetTextColor(dc, writable_tick ? RGB(0, 0, 0) :
                                                 RGB(80, 80, 80));
                TextOutW(dc, ruler.left + scale(pane, 3),
                         y - scale(pane, 6), label.c_str(),
                         static_cast<int>(label.size()));
            }
        }
    }

    SelectObject(dc, active_pen);
    SetTextColor(dc, RGB(0, 0, 0));
    const int marker_x = ruler.right - scale(pane, 2);
    const int marker_half = scale(pane, 4);
    const auto draw_margin_marker = [&](int y) {
        if (y < ruler.top || y >= ruler.bottom) {
            return;
        }
        POINT points[3]{{marker_x, y},
                        {marker_x - marker_half, y - marker_half},
                        {marker_x - marker_half, y + marker_half}};
        HBRUSH marker = CreateSolidBrush(RGB(255, 255, 255));
        HBRUSH old = static_cast<HBRUSH>(SelectObject(dc, marker));
        Polygon(dc, points, 3);
        SelectObject(dc, old);
        DeleteObject(marker);
    };
    for (std::size_t index = 0; index < offsets.size(); ++index) {
        if ((index == 0 && !has_previous) ||
            (index == 2 && !has_next)) {
            continue;
        }
        draw_margin_marker(page_top + offsets[index] +
                           displayed_top_margin);
        draw_margin_marker(page_bottom + offsets[index] -
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
    RECT client{};
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
    std::array<int, 1> tab_positions{};
    std::array<unsigned char, 1> tab_types{};
    if (!OpusGetWin95HorizontalRulerMetrics(
            ruler, &zero, &active_left, &active_right, &pixels_per_inch,
            &left_indent, &first_indent, &right_indent, &default_tab,
            &tab_count, tab_positions.data(), tab_types.data(), 0)) {
        return false;
    }
    const int hit_radius = scale(ruler, 5);
    const int left_distance = std::abs(point.x - active_left);
    const int right_distance = std::abs(point.x - active_right);
    if (left_distance > hit_radius && right_distance > hit_radius) {
        return false;
    }
    *left_boundary = left_distance <= right_distance;
    return true;
}

void redraw_horizontal_ruler_overlay(HWND overlay) {
    InvalidateRect(overlay, nullptr, FALSE);
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
    std::array<int, 1> tab_positions{};
    std::array<unsigned char, 1> tab_types{};
    if (!OpusGetWin95HorizontalRulerMetrics(
            ruler, &zero, &active_left, &active_right, &pixels_per_inch,
            &left_indent, &first_indent, &right_indent, &default_tab,
            &tab_count, tab_positions.data(), tab_types.data(), 0) ||
        !OpusGetWin95HorizontalPageMargins(
            ruler, &left_margin, &right_margin)) {
        return;
    }
    const int minimum_text_width = std::max(
        scale(ruler, 12), std::max(1, pixels_per_inch / 4));
    if (g_horizontal_ruler_drag.left) {
        x = std::max(active_left - left_margin,
                     std::min(x, active_right - minimum_text_width));
    } else {
        x = std::max(active_left + minimum_text_width,
                     std::min(x, active_right + right_margin));
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
    g_horizontal_ruler_drag = {};
    redraw_horizontal_ruler_overlay(overlay);
}

void draw_horizontal_ruler(HWND ruler, HDC dc) {
    RECT client{};
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
    std::array<int, 32> tab_positions{};
    std::array<unsigned char, 32> tab_types{};
    if (!OpusGetWin95HorizontalRulerMetrics(
            ruler, &zero, &active_left, &active_right, &pixels_per_inch,
            &left_indent, &first_indent, &right_indent, &default_tab,
            &tab_count, tab_positions.data(), tab_types.data(),
            static_cast<int>(tab_positions.size())) || pixels_per_inch <= 0) {
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

    active_left = std::max<int>(static_cast<int>(client.left),
                                std::min<int>(active_left,
                                              static_cast<int>(client.right)));
    active_right = std::max<int>(static_cast<int>(client.left),
                                 std::min<int>(active_right,
                                               static_cast<int>(client.right)));
    HBRUSH outside_brush = CreateSolidBrush(kButtonFace);
    FillRect(dc, &client, outside_brush);
    DeleteObject(outside_brush);

    RECT active{active_left, client.top, active_right, client.bottom};
    if (active.right > active.left) {
        HBRUSH active_brush = CreateSolidBrush(RGB(255, 255, 255));
        FillRect(dc, &active, active_brush);
        DeleteObject(active_brush);
        DrawEdge(dc, &active, EDGE_SUNKEN, BF_LEFT | BF_RIGHT);
    }
    DrawEdge(dc, &client, EDGE_RAISED, BF_TOP | BF_BOTTOM);

    HPEN active_pen = CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
    HPEN subdued_pen = CreatePen(PS_SOLID, 1, RGB(96, 96, 96));
    HPEN previous_pen = static_cast<HPEN>(SelectObject(dc, active_pen));
    HFONT previous_font = static_cast<HFONT>(
        SelectObject(dc, GetStockObject(DEFAULT_GUI_FONT)));
    const int previous_bk = SetBkMode(dc, TRANSPARENT);
    const COLORREF previous_text = SetTextColor(dc, RGB(0, 0, 0));

    const int scale_line = std::max(client.top + 8, client.bottom - 8);
    MoveToEx(dc, client.left, scale_line, nullptr);
    LineTo(dc, client.right, scale_line);
    const int eighth = std::max(1, pixels_per_inch / 8);
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
        MoveToEx(dc, x, scale_line, nullptr);
        LineTo(dc, x, scale_line - length);
        if (tick % 8 == 0) {
            const std::wstring label = std::to_wstring(tick / 8);
            SetTextColor(dc, writable ? RGB(0, 0, 0) : RGB(80, 80, 80));
            TextOutW(dc, x + 3, client.top - 1, label.c_str(),
                     static_cast<int>(label.size()));
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
        MoveToEx(dc, x, top, nullptr);
        LineTo(dc, x, bottom);
        switch (tab_types[index] & 7) {
        case 0:
            LineTo(dc, x + 5, bottom);
            break;
        case 1:
            MoveToEx(dc, x - 4, bottom, nullptr);
            LineTo(dc, x + 5, bottom);
            break;
        case 2:
            LineTo(dc, x - 5, bottom);
            break;
        default:
            MoveToEx(dc, x - 3, bottom, nullptr);
            LineTo(dc, x + 2, bottom);
            SetPixel(dc, x + 4, bottom - 1, RGB(0, 0, 0));
            break;
        }
    }

    HBRUSH marker_brush = CreateSolidBrush(RGB(255, 255, 255));
    HBRUSH previous_brush = static_cast<HBRUSH>(
        SelectObject(dc, marker_brush));
    const int marker_top = scale_line + 1;
    const int marker_bottom = client.bottom - 1;
    auto down_triangle = [&](int x) {
        POINT points[3]{{x - 5, marker_top}, {x + 5, marker_top},
                        {x, std::min(marker_bottom, marker_top + 6)}};
        Polygon(dc, points, 3);
    };
    auto up_triangle = [&](int x, bool with_base) {
        POINT points[3]{{x - 5, marker_bottom - 2},
                        {x + 5, marker_bottom - 2},
                        {x, std::max(marker_top, marker_bottom - 8)}};
        Polygon(dc, points, 3);
        if (with_base) {
            Rectangle(dc, x - 3, marker_bottom - 2,
                      x + 4, marker_bottom + 1);
        }
    };
    down_triangle(first_indent);
    up_triangle(left_indent, true);
    up_triangle(right_indent, false);
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
        POINT point{GET_X_LPARAM(l_param), GET_Y_LPARAM(l_param)};
        ScreenToClient(overlay, &point);
        bool left_boundary = false;
        return horizontal_margin_boundary_at(
                   overlay, point, &left_boundary) ?
                   HTCLIENT : HTTRANSPARENT;
    }
    case WM_MOUSEACTIVATE:
        return MA_NOACTIVATE;
    case WM_SETCURSOR:
        SetCursor(LoadCursorW(nullptr, MAKEINTRESOURCEW(32644)));
        return TRUE;
    case WM_LBUTTONDOWN: {
        POINT point{GET_X_LPARAM(l_param), GET_Y_LPARAM(l_param)};
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
        std::array<int, 1> tab_positions{};
        std::array<unsigned char, 1> tab_types{};
        if (OpusGetWin95HorizontalRulerMetrics(
                ruler, &zero, &active_left, &active_right,
                &pixels_per_inch, &left_indent, &first_indent,
                &right_indent, &default_tab, &tab_count,
                tab_positions.data(), tab_types.data(), 0)) {
            g_horizontal_ruler_drag.overlay = overlay;
            g_horizontal_ruler_drag.ruler = ruler;
            g_horizontal_ruler_drag.active = true;
            g_horizontal_ruler_drag.left = left_boundary;
            g_horizontal_ruler_drag.origin_x = left_boundary ?
                active_left : active_right;
            g_horizontal_ruler_drag.preview_x =
                g_horizontal_ruler_drag.origin_x;
            SetCapture(overlay);
            SetCursor(LoadCursorW(nullptr, MAKEINTRESOURCEW(32644)));
        }
        return 0;
    }
    case WM_MOUSEMOVE:
        if (g_horizontal_ruler_drag.active &&
            g_horizontal_ruler_drag.overlay == overlay) {
            update_horizontal_margin_drag(overlay, GET_X_LPARAM(l_param));
            SetCursor(LoadCursorW(nullptr, MAKEINTRESOURCEW(32644)));
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
            g_horizontal_ruler_drag = {};
            if (GetCapture() == overlay) {
                ReleaseCapture();
            }
            const bool changed = OpusAdjustWin95HorizontalMargin(
                ruler, left_boundary ? 1 : 0, delta) != 0;
            if (!changed) {
                MessageBeep(MB_ICONEXCLAMATION);
            } else {
                HWND pane = nullptr;
                EnumChildWindows(GetAncestor(ruler, GA_ROOT),
                                 find_document_pane,
                                 reinterpret_cast<LPARAM>(&pane));
                if (pane != nullptr) {
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
            reinterpret_cast<HWND>(l_param) != overlay) {
            cancel_horizontal_margin_drag(overlay);
        }
        break;
    case WM_CANCELMODE:
        if (g_horizontal_ruler_drag.active &&
            g_horizontal_ruler_drag.overlay == overlay) {
            g_horizontal_ruler_drag = {};
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
        PAINTSTRUCT paint{};
        HDC dc = BeginPaint(overlay, &paint);
        if (dc != nullptr) {
            draw_horizontal_ruler(GetParent(overlay), dc);
        }
        EndPaint(overlay, &paint);
        return 0;
    }
    }
    return DefWindowProcW(overlay, message, w_param, l_param);
}

void ensure_horizontal_ruler_overlay(HWND ruler) {
    RECT client{};
    GetClientRect(ruler, &client);
    HWND overlay = FindWindowExW(ruler, nullptr, kRulerOverlayClass, nullptr);
    if (overlay == nullptr) {
        const LONG_PTR style = GetWindowLongPtrW(ruler, GWL_STYLE);
        if ((style & WS_CLIPCHILDREN) == 0) {
            SetWindowLongPtrW(ruler, GWL_STYLE, style | WS_CLIPCHILDREN);
            SetWindowPos(ruler, nullptr, 0, 0, 0, 0,
                         SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
                         SWP_NOACTIVATE | SWP_FRAMECHANGED);
        }
        overlay = CreateWindowExW(
            WS_EX_NOACTIVATE,
            kRulerOverlayClass, nullptr, WS_CHILD | WS_VISIBLE,
            0, 0, client.right - client.left, client.bottom - client.top,
            ruler, nullptr, GetModuleHandleW(nullptr), nullptr);
    }
    if (overlay != nullptr) {
        SetWindowPos(overlay, HWND_TOP, 0, 0,
                     client.right - client.left, client.bottom - client.top,
                     SWP_NOACTIVATE | SWP_SHOWWINDOW);
        InvalidateRect(overlay, nullptr, FALSE);
        UpdateWindow(overlay);
    }
}

void show_document_context_menu(HWND pane, POINT screen_point) {
    HMENU popup = CreatePopupMenu();
    if (popup == nullptr) {
        return;
    }
    AppendMenuW(popup, MF_STRING, bcmUndo, OPUSW("&Undo"));
    AppendMenuW(popup, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(popup, MF_STRING, bcmCut, OPUSW("Cu&t"));
    AppendMenuW(popup, MF_STRING, bcmCopy, OPUSW("&Copy"));
    AppendMenuW(popup, MF_STRING, bcmPaste, OPUSW("&Paste"));
    AppendMenuW(popup, MF_STRING, bcmSelectAll, OPUSW("Select &All"));
    AppendMenuW(popup, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(popup, MF_STRING, bcmCharacter, OPUSW("&Font..."));
    AppendMenuW(popup, MF_STRING, bcmParagraph, OPUSW("&Paragraph..."));
    AppendMenuW(popup, MF_STRING, bcmTabs, OPUSW("&Tabs..."));
    AppendMenuW(popup, MF_STRING, imiRenumParas,
                OPUSW("&Bullets and Numbering..."));
    AppendMenuW(popup, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(popup, MF_STRING, bcmInsTable, OPUSW("Insert &Table..."));
    AppendMenuW(popup, MF_STRING, bcmFormatTable,
                OPUSW("Table &Properties..."));
    AppendMenuW(popup, MF_STRING, bcmInsPic, OPUSW("Insert &Picture..."));
    AppendMenuW(popup, MF_STRING, bcmInsField, OPUSW("Insert F&ield..."));
    style_menu_tree(popup);
    const HWND app = GetAncestor(pane, GA_ROOT);
    const UINT command = TrackPopupMenu(
        popup, TPM_RETURNCMD | TPM_RIGHTBUTTON,
        screen_point.x, screen_point.y, 0, app, nullptr);
    DestroyMenu(popup);
    if (command != 0) {
        PostMessageW(app, WM_COMMAND, MAKEWPARAM(command, 0), 0);
    }
}

void update_language_menu_check() {
    if (g_language_menu == nullptr) return;
    char selected[32]{};
    OpusUnicodeGetInputLanguage(selected, static_cast<int>(std::size(selected)));
    UINT command = kCmdLanguageBase;
    for (const LanguageChoice& choice : kLanguageChoices) {
        if (_stricmp(selected, choice.tag) == 0) {
            command = choice.command;
            break;
        }
    }
    CheckMenuRadioItem(g_language_menu, kCmdLanguageBase,
                       kCmdLanguageBase +
                           static_cast<UINT>(kLanguageChoices.size() - 1),
                       command, MF_BYCOMMAND);
}

bool start_next_unicode_input(HWND pane) {
    if (g_unicode_input_active || g_pending_unicode_inputs.empty())
        return true;
    int doc_before = -1;
    long cp_before = 0;
    long cp_lim_before = 0;
    if (!OpusGetUnicodeSelection(&doc_before, &cp_before, &cp_lim_before)) {
        g_pending_unicode_inputs.pop_front();
        return false;
    }
    g_active_unicode_input = g_pending_unicode_inputs.front();
    g_pending_unicode_inputs.pop_front();
    g_active_unicode_input.doc = doc_before;
    g_active_unicode_input.cp = cp_before;
    g_active_unicode_input.retries = 0;
    g_unicode_input_active = true;
    const WPARAM legacy = g_active_unicode_input.scalar < 0x20 ?
        static_cast<WPARAM>(g_active_unicode_input.scalar) :
        static_cast<WPARAM>(OpusUnicodeLegacyByteForScalar(
            g_active_unicode_input.scalar) & 0xff);
    if (!PostMessageW(pane, WM_CHAR, legacy, 1) ||
        !PostMessageW(pane, WM_LBUTTONUP, 0, 0) ||
        !PostMessageW(pane, kWmCommitUnicodeScalar, 0, 0)) {
        g_unicode_input_active = false;
        return false;
    }
    return true;
}

bool queue_unicode_input(HWND pane, const std::uint32_t scalar) {
    if (pane == nullptr || scalar == 0 || scalar > 0x10ffff ||
        (scalar >= 0xd800 && scalar <= 0xdfff)) return false;
    if (g_pending_unicode_inputs.size() >= kMaxPendingUnicodeInput)
        return false;
    try {
        g_pending_unicode_inputs.push_back({-1, 0, scalar, 0});
    } catch (...) {
        return false;
    }
    return start_next_unicode_input(pane);
}

bool insert_unicode_scalar(HWND pane, const std::uint32_t scalar) {
    return scalar >= 0x20 && queue_unicode_input(pane, scalar);
}

void insert_unicode_text(HWND pane, const std::wstring& text) {
    for (std::size_t index = 0; index < text.size();) {
        std::uint32_t scalar = static_cast<std::uint16_t>(text[index++]);
        if (scalar >= 0xd800 && scalar <= 0xdbff && index < text.size()) {
            const std::uint32_t low = static_cast<std::uint16_t>(text[index]);
            if (low >= 0xdc00 && low <= 0xdfff) {
                ++index;
                scalar = 0x10000 + ((scalar - 0xd800) << 10) +
                         (low - 0xdc00);
            }
        }
        if (scalar == L'\r' || scalar == L'\n' || scalar == L'\t')
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
                InvalidateRect(pane, nullptr, FALSE);
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
        insert_unicode_scalar(pane, static_cast<std::uint32_t>(w_param));
        return 0;
    }

    /* The original message loop marks already-translated byte input by setting
     * bit 15 (0x80xx). That is an internal Word flag, not a Unicode code point,
     * and must reach the legacy pane procedure unchanged.
     */
    if (message == WM_CHAR && (w_param & 0xff00) != 0x8000 &&
        w_param >= 0x80) {
        OpusQueueUnicodeWmChar(
            pane, static_cast<unsigned int>(w_param));
        return 0;
    }
    if (message == WM_IME_COMPOSITION &&
        (l_param & GCS_RESULTSTR) != 0) {
        HIMC context = ImmGetContext(pane);
        if (context != nullptr) {
            const LONG byte_count = ImmGetCompositionStringW(
                context, GCS_RESULTSTR, nullptr, 0);
            if (byte_count > 0 && byte_count <= 1024 * 1024 &&
                (byte_count % sizeof(wchar_t)) == 0) {
                std::wstring result(
                    static_cast<std::size_t>(byte_count) / sizeof(wchar_t),
                    L'\0');
                if (ImmGetCompositionStringW(context, GCS_RESULTSTR,
                        &result[0], static_cast<DWORD>(byte_count)) ==
                    byte_count) {
                    insert_unicode_text(pane, result);
                }
            }
            ImmReleaseContext(pane, context);
            return 0;
        }
    }
    if (message == WM_MOUSEWHEEL) {
        if (g_document_wheel.pane != pane) {
            g_document_wheel = {};
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
        POINT point{GET_X_LPARAM(l_param), GET_Y_LPARAM(l_param)};
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
                SetCursor(LoadCursorW(nullptr, MAKEINTRESOURCEW(32645)));
                return 0;
            }
        }
    }
    if (message == WM_MOUSEMOVE) {
        if (g_vertical_ruler_drag.active &&
            g_vertical_ruler_drag.pane == pane) {
            update_vertical_margin_drag(pane, GET_Y_LPARAM(l_param));
            SetCursor(LoadCursorW(nullptr, MAKEINTRESOURCEW(32645)));
            return 0;
        }
        POINT point{GET_X_LPARAM(l_param), GET_Y_LPARAM(l_param)};
        bool top_marker = false;
        if (vertical_margin_marker_at(
                pane, point, &top_marker, nullptr)) {
            SetCursor(LoadCursorW(nullptr, MAKEINTRESOURCEW(32645)));
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
        g_vertical_ruler_drag = {};
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
        g_vertical_ruler_drag = {};
        if (GetCapture() == pane) {
            ReleaseCapture();
        }
        redraw_vertical_ruler(pane);
        return 0;
    }
    if (message == WM_CAPTURECHANGED &&
        g_vertical_ruler_drag.active &&
        g_vertical_ruler_drag.pane == pane &&
        reinterpret_cast<HWND>(l_param) != pane) {
        cancel_vertical_margin_drag(pane);
    }
    if (message == WM_CANCELMODE && g_vertical_ruler_drag.active &&
        g_vertical_ruler_drag.pane == pane) {
        g_vertical_ruler_drag = {};
        if (GetCapture() == pane) {
            ReleaseCapture();
        }
        redraw_vertical_ruler(pane);
        return 0;
    }
    if (message == WM_SETCURSOR && LOWORD(l_param) == HTCLIENT) {
        POINT point{};
        GetCursorPos(&point);
        ScreenToClient(pane, &point);
        bool top_marker = false;
        if ((g_vertical_ruler_drag.active &&
             g_vertical_ruler_drag.pane == pane) ||
            vertical_margin_marker_at(
                pane, point, &top_marker, nullptr)) {
            SetCursor(LoadCursorW(nullptr, MAKEINTRESOURCEW(32645)));
            return TRUE;
        }
    }
    if (message == WM_PAINT) {
        const LRESULT result = original != nullptr ?
            CallWindowProcW(original, pane, message, w_param, l_param) :
            DefWindowProcW(pane, message, w_param, l_param);
        HDC dc = GetDC(pane);
        if (dc != nullptr) {
            update_current_page_snapshot(pane, dc);
            draw_continuous_page_workspace(pane, dc);
            draw_continuous_page_neighbors(pane, dc);
            draw_vertical_ruler(pane, dc);
            ReleaseDC(pane, dc);
        }
        return result;
    }
    if (message == WM_SIZE) {
        const LRESULT result = original != nullptr ?
            CallWindowProcW(original, pane, message, w_param, l_param) :
            DefWindowProcW(pane, message, w_param, l_param);
        OpusRecenterWin95PageView(pane);
        InvalidateRect(pane, nullptr, TRUE);
        return result;
    }
    if (message == WM_RBUTTONDOWN) {
        SetFocus(pane);
        return 0;
    }
    if (message == WM_RBUTTONUP) {
        POINT point{GET_X_LPARAM(l_param), GET_Y_LPARAM(l_param)};
        ClientToScreen(pane, &point);
        show_document_context_menu(pane, point);
        return 0;
    }
    if (message == WM_CONTEXTMENU) {
        POINT point{GET_X_LPARAM(l_param), GET_Y_LPARAM(l_param)};
        if (point.x == -1 && point.y == -1) {
            GetCursorPos(&point);
        }
        show_document_context_menu(pane, point);
        return 0;
    }
    if (message == WM_NCDESTROY) {
        if (g_document_wheel.pane == pane) {
            g_document_wheel = {};
        }
        g_pending_unicode_inputs.clear();
        g_unicode_input_active = false;
        g_pending_high_surrogate = 0;
        clear_page_snapshots(pane);
        RemovePropW(pane, kOriginalPaneProcProperty);
        if (original != nullptr) {
            SetWindowLongPtrW(pane, GWLP_WNDPROC,
                              reinterpret_cast<LONG_PTR>(original));
        }
    }
    return original != nullptr ?
        CallWindowProcW(original, pane, message, w_param, l_param) :
        DefWindowProcW(pane, message, w_param, l_param);
}

BOOL CALLBACK subclass_document_pane(HWND candidate, LPARAM) {
    WCHAR class_name[64]{};
    GetClassNameW(candidate, class_name,
                  static_cast<int>(sizeof(class_name) / sizeof(class_name[0])));
    if (lstrcmpiW(class_name, kDocumentPaneClass) == 0 &&
        original_pane_proc(candidate) == nullptr) {
        SetLastError(0);
        WNDPROC original = reinterpret_cast<WNDPROC>(SetWindowLongPtrW(
            candidate, GWLP_WNDPROC,
            reinterpret_cast<LONG_PTR>(document_pane_proc)));
        if (original != nullptr || GetLastError() == 0) {
            SetPropW(candidate, kOriginalPaneProcProperty,
                     reinterpret_cast<HANDLE>(original));
            RedrawWindow(candidate, nullptr, nullptr,
                         RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
        }
    }
    return TRUE;
}

BOOL CALLBACK redraw_word95_ruler_window(HWND candidate, LPARAM) {
    WCHAR class_name[64]{};
    GetClassNameW(candidate, class_name,
                  static_cast<int>(sizeof(class_name) / sizeof(class_name[0])));
    if (lstrcmpiW(class_name, kRulerClass) == 0 ||
        lstrcmpiW(class_name, kDocumentPaneClass) == 0) {
        RedrawWindow(candidate, nullptr, nullptr,
                     RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
    }
    return TRUE;
}

BOOL CALLBACK paint_word95_horizontal_ruler(HWND candidate, LPARAM) {
    WCHAR class_name[64]{};
    GetClassNameW(candidate, class_name,
                  static_cast<int>(sizeof(class_name) / sizeof(class_name[0])));
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
        HWND toolbar = FindWindowExW(app, nullptr, kToolbarClass, nullptr);
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
            command < kCmdLanguageBase + kLanguageChoices.size()) {
            const LanguageChoice& choice =
                kLanguageChoices[command - kCmdLanguageBase];
            OpusUnicodeSetInputLanguage(choice.tag);
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
        HWND toolbar = FindWindowExW(app, nullptr, kToolbarClass, nullptr);
        ToolbarState* state = toolbar_state(toolbar);
        if (state != nullptr) {
            update_toolbar_menu_checks(app, *state);
        }

        /* On current Windows versions a popup using a classic background brush
         * can receive its initial erase before the menu items have been
         * invalidated. Repaint once the #32768 popup exists so the complete
         * File menu is visible without requiring mouse hover.
         */
        SetTimer(app, kMenuRepaintTimer, 15, nullptr);
    } else if (message == WM_TIMER && w_param == kMenuRepaintTimer) {
        KillTimer(app, kMenuRepaintTimer);
        EnumThreadWindows(GetCurrentThreadId(), repaint_menu_popup, 0);
        return 0;
    } else if (message == WM_NCDESTROY) {
        g_original_app_proc = nullptr;
        if (original != nullptr) {
            SetWindowLongPtrW(app, GWLP_WNDPROC,
                              reinterpret_cast<LONG_PTR>(original));
        }
    }
    return original != nullptr ?
        CallWindowProcW(original, app, message, w_param, l_param) :
        DefWindowProcW(app, message, w_param, l_param);
}

bool subclass_app_window(HWND app) {
    if (g_original_app_proc != nullptr) {
        return true;
    }
    SetLastError(0);
    WNDPROC original = reinterpret_cast<WNDPROC>(SetWindowLongPtrW(
        app, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(app_window_proc)));
    if (original == nullptr && GetLastError() != 0) {
        return false;
    }
    g_original_app_proc = original;
    return true;
}

LRESULT CALLBACK toolbar_window_proc(HWND window, UINT message,
                                     WPARAM w_param, LPARAM l_param) {
    auto* state = reinterpret_cast<ToolbarState*>(
        GetWindowLongPtrW(window, GWLP_USERDATA));
    switch (message) {
    case WM_CREATE: {
        auto* created = new ToolbarState{};
        SetWindowLongPtrW(window, GWLP_USERDATA,
                          reinterpret_cast<LONG_PTR>(created));
        created->sprite = static_cast<HBITMAP>(LoadImageW(
            GetModuleHandleW(nullptr), MAKEINTRESOURCEW(kToolbarBitmap),
            IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION));
        LOGFONTW logical{};
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
                     reinterpret_cast<WPARAM>(created->font), FALSE);
        SendMessageW(created->font_combo, WM_SETFONT,
                     reinterpret_cast<WPARAM>(created->font), FALSE);
        SendMessageW(created->size_combo, WM_SETFONT,
                     reinterpret_cast<WPARAM>(created->font), FALSE);
        SendMessageW(created->zoom_combo, WM_SETFONT,
                     reinterpret_cast<WPARAM>(created->font), FALSE);
        position_combos(window, *created);
        sync_mirrors(window, *created);
        SetTimer(window, kSyncTimer, 350, nullptr);
        return 0;
    }
    case WM_SIZE:
        if (state != nullptr) {
            position_combos(window, *state);
        }
        return 0;
    case WM_TIMER:
        if (state != nullptr && w_param == kSyncTimer) {
            if (GetTickCount64() >= state->suppress_sync_until) {
                sync_mirrors(window, *state);
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
                HWND pane = nullptr;
                EnumChildWindows(app, find_document_pane,
                                 reinterpret_cast<LPARAM>(&pane));
                if (pane != nullptr) {
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
        if (state != nullptr) {
            const UINT id = LOWORD(w_param);
            const int notification = HIWORD(w_param);
            if (notification == CBN_SELCHANGE ||
                notification == CBN_SELENDOK ||
                notification == CBN_EDITCHANGE) {
                state->suppress_sync_until = GetTickCount64() + 1000;
            }
            if (id == kComboStyle) {
                forward_combo(state->style_combo, state->source_style,
                              notification, state->style_edit_dirty);
                return 0;
            }
            if (id == kComboFont) {
                forward_combo(state->font_combo, state->source_font,
                              notification, state->font_edit_dirty);
                return 0;
            }
            if (id == kComboSize) {
                forward_combo(state->size_combo, state->source_size,
                              notification, state->size_edit_dirty);
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
        if (state != nullptr) {
            POINT point{GET_X_LPARAM(l_param), GET_Y_LPARAM(l_param)};
            state->pressed = hit_test(window, *state, point);
            if (state->pressed.hit) {
                SetCapture(window);
                InvalidateRect(window, nullptr, FALSE);
            }
        }
        return 0;
    case WM_CONTEXTMENU:
        if (state != nullptr) {
            POINT point{GET_X_LPARAM(l_param), GET_Y_LPARAM(l_param)};
            if (point.x == -1 && point.y == -1) {
                GetCursorPos(&point);
            }
            show_toolbar_context_menu(window, point);
        }
        return 0;
    case WM_RBUTTONDOWN:
        return 0;
    case WM_RBUTTONUP:
        if (state != nullptr) {
            POINT point{GET_X_LPARAM(l_param), GET_Y_LPARAM(l_param)};
            ClientToScreen(window, &point);
            show_toolbar_context_menu(window, point);
        }
        return 0;
    case WM_LBUTTONUP:
        if (state != nullptr && state->pressed.hit) {
            POINT point{GET_X_LPARAM(l_param), GET_Y_LPARAM(l_param)};
            const HitResult released = hit_test(window, *state, point);
            const HitResult pressed = state->pressed;
            state->pressed = {};
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
            InvalidateRect(window, nullptr, FALSE);
        }
        return 0;
    case WM_CAPTURECHANGED:
        if (state != nullptr && state->pressed.hit) {
            state->pressed = {};
            InvalidateRect(window, nullptr, FALSE);
        }
        return 0;
    case WM_ERASEBKGND:
        return 1;
    case WM_PAINT:
        if (state != nullptr) {
            paint_toolbar(window, *state);
            return 0;
        }
        break;
    case WM_DESTROY:
        if (state != nullptr) {
            KillTimer(window, kSyncTimer);
            if (state->sprite != nullptr) {
                DeleteObject(state->sprite);
            }
            if (state->font != nullptr) {
                DeleteObject(state->font);
            }
            delete state;
            SetWindowLongPtrW(window, GWLP_USERDATA, 0);
        }
        break;
    }
    return DefWindowProcW(window, message, w_param, l_param);
}

bool register_toolbar_class() {
    WNDCLASSEXW window_class{};
    window_class.cbSize = sizeof(window_class);
    window_class.style = CS_DBLCLKS;
    window_class.lpfnWndProc = toolbar_window_proc;
    window_class.hInstance = GetModuleHandleW(nullptr);
    window_class.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(32512));
    window_class.lpszClassName = kToolbarClass;
    return RegisterClassExW(&window_class) != 0 ||
           GetLastError() == ERROR_CLASS_ALREADY_EXISTS;
}

bool register_ruler_overlay_class() {
    WNDCLASSEXW window_class{};
    window_class.cbSize = sizeof(window_class);
    window_class.lpfnWndProc = ruler_overlay_proc;
    window_class.hInstance = GetModuleHandleW(nullptr);
    window_class.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(32512));
    window_class.lpszClassName = kRulerOverlayClass;
    return RegisterClassExW(&window_class) != 0 ||
           GetLastError() == ERROR_CLASS_ALREADY_EXISTS;
}

}  // namespace

extern "C" int OpusQueueUnicodeWmChar(
    HWND pane, const unsigned int code_unit) {
    if (code_unit >= 0xd800 && code_unit <= 0xdbff) {
        g_pending_high_surrogate = static_cast<wchar_t>(code_unit);
        return TRUE;
    }
    std::uint32_t scalar = code_unit;
    if (code_unit >= 0xdc00 && code_unit <= 0xdfff) {
        if (g_pending_high_surrogate < 0xd800 ||
            g_pending_high_surrogate > 0xdbff) {
            g_pending_high_surrogate = 0;
            return FALSE;
        }
        scalar = 0x10000 +
            ((static_cast<std::uint32_t>(g_pending_high_surrogate) -
              0xd800) << 10) + (code_unit - 0xdc00);
    }
    g_pending_high_surrogate = 0;
    return insert_unicode_scalar(pane, scalar);
}

extern "C" void OpusDrawWin95HorizontalRuler(HWND ruler) {
    HDC dc = GetDC(ruler);
    if (dc != nullptr) {
        draw_horizontal_ruler(ruler, dc);
        ReleaseDC(ruler, dc);
    }
}

extern "C" HWND vhwndWin95Toolbar = nullptr;

extern "C" int OpusWin95ToolbarHeight(void) {
    return toolbar_height(vhwndWin95Toolbar);
}

extern "C" int OpusWin95ChromeActive(void) {
    return vhwndWin95Toolbar != nullptr && IsWindow(vhwndWin95Toolbar);
}

extern "C" void OpusSyncWin95Toolbar(void) {
    if (OpusWin95ChromeActive()) {
        SendMessageW(vhwndWin95Toolbar, WM_TIMER, kSyncTimer, 0);
        configure_word95_menus(GetParent(vhwndWin95Toolbar));
        style_menu_tree(GetMenu(GetParent(vhwndWin95Toolbar)));
        ToolbarState* state = toolbar_state(vhwndWin95Toolbar);
        if (state != nullptr) {
            update_toolbar_menu_checks(GetParent(vhwndWin95Toolbar), *state);
        }
        subclass_all_document_panes(GetParent(vhwndWin95Toolbar));
        DrawMenuBar(GetParent(vhwndWin95Toolbar));
    }
}

extern "C" void OpusSizeWin95Toolbar(HWND parent) {
    if (!OpusWin95ChromeActive()) {
        return;
    }
    RECT client{};
    GetClientRect(parent, &client);
    const int height = OpusWin95ToolbarHeight();
    ShowWindow(vhwndWin95Toolbar,
               height > 0 ? SW_SHOWNOACTIVATE : SW_HIDE);
    MoveWindow(vhwndWin95Toolbar, 0, 0, client.right - client.left,
               height, TRUE);
}

extern "C" int OpusCreateWin95Chrome(HWND parent) {
    if (OpusWin95ChromeActive()) {
        return TRUE;
    }
    if (!register_toolbar_class() || !register_ruler_overlay_class()) {
        return FALSE;
    }
    if (g_menu_brush == nullptr) {
        g_menu_brush = CreateSolidBrush(kButtonFace);
    }
    configure_word95_menus(parent);
    style_menu_tree(GetMenu(parent));
    DrawMenuBar(parent);
    apply_caption_colors(parent);
    set_window_classic(parent);

    vhwndWin95Toolbar = CreateWindowExW(
        0, kToolbarClass, nullptr,
        WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
        0, 0, 1, scale(parent, 64), parent, nullptr,
        GetModuleHandleW(nullptr), nullptr);
    if (vhwndWin95Toolbar == nullptr) {
        return FALSE;
    }
    if (!subclass_app_window(parent)) {
        DestroyWindow(vhwndWin95Toolbar);
        vhwndWin95Toolbar = nullptr;
        return FALSE;
    }
    subclass_all_document_panes(parent);
    OpusSizeWin95Toolbar(parent);
    ToolbarState* state = toolbar_state(vhwndWin95Toolbar);
    if (state != nullptr) {
        update_toolbar_menu_checks(parent, *state);
    }
    SetWindowPos(vhwndWin95Toolbar, HWND_TOP, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    return TRUE;
}

extern "C" void OpusRequestWin95PageView(HWND parent) {
    HWND pane = nullptr;
    EnumChildWindows(parent, find_document_pane,
                     reinterpret_cast<LPARAM>(&pane));
    if (pane == nullptr) {
        return;
    }

    HWND toolbar = FindWindowExW(parent, nullptr, kToolbarClass, nullptr);
    ToolbarState* state = toolbar_state(toolbar);
    if (state != nullptr) {
        state->page_view_start_after = GetTickCount64() + 750;
        state->startup_page_view_requested = false;
    }
}
