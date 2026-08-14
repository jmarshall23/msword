#include "opus_x64_compat.h"
#include "opus_x64_heap.h"
#include "objbase.h"
extern "C" {
#include "dac.h"
}

#include <algorithm>
#include <atomic>
#include <cctype>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

extern "C" {
extern void** hcabDlgCur;
extern std::uintptr_t wRefDlgCur;
extern HWND vhWndMsgBoxParent;
void GetCabSz(void**, char*, std::uint16_t, std::uint16_t);
int FSetCabSz(void**, const char*, std::uint16_t);
int OpusModernPathIsDocx(const char*);
int OpusModernPathIsOdt(const char*);
int OpusModernDocxToTextFile(const char*, const char*);
int OpusModernOdtToTextFile(const char*, const char*);
int OpusModernRtfFileToDocx(const char*, const char*);
int OpusModernRtfFileToOdt(const char*, const char*);
int OpusSaveDocumentAsDocx(int, const char*);
}

extern "C" void OpusX64TraceRibbon(const char*, int, int, int, int,
                                     long, long, int);

/* Flat, stateful implementation of the public SDM 2.21 dialog API used by Opus.
 * The archive contains the SDM headers and 16-bit .obj files, but not its C
 * sources. This layer retains the original CAB/TMC/HDLG contracts and supplies
 * the dialog state that the original Word modules expect. Native window
 * creation is deliberately separate from this state layer.
 */

namespace {

using Word = std::uint16_t;
using Dword = std::uint32_t;
using Hdlg = Word;
using Tmc = Word;
using Hcab = void**;
using OriginalListProc = Word (*)(Word, char*, int, Word, Word, Word);
using FontValueProc = int (*)(const char*);
using FontNameFromValueProc = void (*)(int, char*, int);

enum class ControlKind {
    unknown,
    button,
    combo,
    edit,
    list,
    statik,
};

struct Rec {
    int x;
    int y;
    int dx;
    int dy;
};

struct DltHeader {
    Rec rec;
    Word hid;
    Tmc tmc_sel_init;
    void* dialog_proc;
    Word base_item_count;
    Word border;
};

struct Dli {
    HWND hwnd;
    int dx;
    int dy;
    Dword flags;
    std::uintptr_t reference;
    unsigned char* runtime_items;
};

struct ControlState {
    ControlKind kind = ControlKind::unknown;
    Word value = 0;
    Dword selection = 0;
    bool enabled = true;
    bool visible = true;
    Word text_limit = 0xffff;
    Rec rectangle{};
    HWND window = nullptr;
    std::string text;
    std::vector<unsigned char> large_value;
    std::vector<std::string> entries;
};

struct DialogState {
    Hdlg handle = 0;
    DltHeader** template_handle = nullptr;
    Hcab cab = nullptr;
    HWND window = nullptr;
    std::uintptr_t reference = 0;
    Word hid = 0;
    Word sab = 0;
    Tmc focus = 0;
    Tmc default_tmc = 1;
    Tmc result_tmc = 2;
    bool visible = false;
    bool dying = false;
    bool modal = false;
    bool native_modal = false;
    bool commands_active = false;
    std::string caption;
    std::string current_directory;
    std::string file_pattern;
    std::unordered_map<Tmc, ControlState> controls;
    std::vector<ControlState> untracked_controls;
};

std::unordered_map<Hdlg, DialogState> g_dialogs;
DialogState g_no_dialog;
Hdlg g_next_dialog = 1;
Hdlg g_current_dialog = 0;
Hdlg g_focus_dialog = 0;
bool g_initialized = false;
bool g_noninteractive = false;
OriginalListProc g_list_font_name = nullptr;
OriginalListProc g_list_font_size = nullptr;
OriginalListProc g_list_styles = nullptr;
OriginalListProc g_list_character_color = nullptr;
FontValueProc g_font_name_to_value = nullptr;
FontValueProc g_font_size_to_value = nullptr;
FontNameFromValueProc g_font_name_from_value = nullptr;

struct Win95SaveAlias {
    bool active = false;
    bool created = false;
    std::string selected_path;
    std::string legacy_path;
};

Win95SaveAlias g_win95_save_alias;
std::unordered_map<std::string, std::string> g_win95_saved_aliases;
std::string g_win95_staging_directory;

struct Win95AliasCleanup {
    ~Win95AliasCleanup() {
        if (g_win95_save_alias.created &&
            !g_win95_save_alias.legacy_path.empty()) {
            DeleteFileA(g_win95_save_alias.legacy_path.c_str());
        }
        for (const auto& saved : g_win95_saved_aliases) {
            DeleteFileA(saved.first.c_str());
        }
        if (!g_win95_staging_directory.empty()) {
            RemoveDirectoryA(g_win95_staging_directory.c_str());
        }
    }
};

Win95AliasCleanup g_win95_alias_cleanup;

std::string win95_alias_key(std::string path) {
    std::transform(path.begin(), path.end(), path.begin(),
                   [](const unsigned char character) {
                       return static_cast<char>(std::tolower(character));
                   });
    return path;
}

bool safe_dialog_file_path(const std::string& path, const bool must_exist) {
    if (path.empty() || path.size() >= 32760 ||
        path.starts_with(R"(\\.\)") ||
        path.starts_with(R"(\\?\GLOBALROOT\)")) return false;
    for (std::size_t index = 0; index < path.size(); ++index) {
        if (path[index] == ':' && index != 1) return false;
    }
    const DWORD attributes = GetFileAttributesA(path.c_str());
    if (attributes != INVALID_FILE_ATTRIBUTES)
        return (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
    return !must_exist && (GetLastError() == ERROR_FILE_NOT_FOUND ||
                           GetLastError() == ERROR_PATH_NOT_FOUND);
}

bool import_file_within_limit(const std::string& path) {
    if (!safe_dialog_file_path(path, true)) return false;
    WIN32_FILE_ATTRIBUTE_DATA attributes{};
    if (!GetFileAttributesExA(path.c_str(), GetFileExInfoStandard,
                              &attributes)) return false;
    constexpr ULONGLONG kMaximumImportBytes = 256ull * 1024ull * 1024ull;
    const ULONGLONG size =
        (static_cast<ULONGLONG>(attributes.nFileSizeHigh) << 32) |
        attributes.nFileSizeLow;
    return size <= kMaximumImportBytes;
}

bool make_win95_staging_path(std::string& path,
                             const char* desired_extension = ".DOC") {
    if (_stricmp(desired_extension, ".DOC") != 0 &&
        _stricmp(desired_extension, ".TXT") != 0) return false;
    if (g_win95_staging_directory.empty()) {
        char temporary_root[32768]{};
        const DWORD root_length = GetTempPathA(
            static_cast<DWORD>(std::size(temporary_root)), temporary_root);
        if (root_length == 0 || root_length >= std::size(temporary_root))
            return false;
        for (int attempt = 0; attempt < 32; ++attempt) {
            GUID identifier{};
            if (FAILED(CoCreateGuid(&identifier))) return false;
            char leaf[9]{};
            _snprintf_s(leaf, std::size(leaf), _TRUNCATE, "W%07lX",
                        identifier.Data1 & 0x0ffffffful);
            const std::string candidate = std::string(temporary_root) + leaf;

            /* The original normalizer requires DOS 8.3-safe path components
             * even though the native file APIs support long names.
             */
            if (candidate.size() >= 52) return false;
            if (CreateDirectoryA(candidate.c_str(), nullptr)) {
                const DWORD attributes = GetFileAttributesA(candidate.c_str());
                if (attributes == INVALID_FILE_ATTRIBUTES ||
                    (attributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0) {
                    RemoveDirectoryA(candidate.c_str());
                    return false;
                }
                g_win95_staging_directory = candidate;
                break;
            }
            if (GetLastError() != ERROR_ALREADY_EXISTS) return false;
        }
        if (g_win95_staging_directory.empty()) return false;
    }
    static std::atomic<unsigned long> sequence{0};
    for (int attempt = 0; attempt < 32; ++attempt) {
        char leaf[18]{};
        _snprintf_s(leaf, std::size(leaf), _TRUNCATE, "\\D%07lX%s",
                    ++sequence & 0x0ffffffful, desired_extension);
        const std::string candidate = g_win95_staging_directory + leaf;
        if (candidate.size() >= 120) return false;
        const DWORD attributes = GetFileAttributesA(candidate.c_str());
        const DWORD attribute_error = GetLastError();
        if (attributes == INVALID_FILE_ATTRIBUTES &&
            (attribute_error == ERROR_FILE_NOT_FOUND ||
             attribute_error == ERROR_PATH_NOT_FOUND)) {
            path = candidate;
            return true;
        }
    }
    return false;
}

bool atomic_copy_file(const std::string& source, const std::string& target) {
    if (source.empty() || target.empty()) return false;
    static std::atomic<unsigned long> sequence{0};
    std::string temporary;
    bool copied = false;
    for (int attempt = 0; attempt < 64; ++attempt) {
        temporary = target + ".word1tmp-" +
            std::to_string(GetCurrentProcessId()) + "-" +
            std::to_string(++sequence);
        if (CopyFileA(source.c_str(), temporary.c_str(), TRUE)) {
            copied = true;
            break;
        }
        if (GetLastError() != ERROR_FILE_EXISTS &&
            GetLastError() != ERROR_ALREADY_EXISTS) return false;
    }
    if (!copied) return false;
    HANDLE file = CreateFileA(temporary.c_str(), GENERIC_WRITE,
                              FILE_SHARE_READ, nullptr, OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL, nullptr);
    const bool flushed = file != INVALID_HANDLE_VALUE &&
                         FlushFileBuffers(file) != FALSE;
    if (file != INVALID_HANDLE_VALUE) CloseHandle(file);
    if (!flushed) {
        DeleteFileA(temporary.c_str());
        return false;
    }
    const DWORD attributes = GetFileAttributesA(target.c_str());
    bool committed = false;
    if (attributes != INVALID_FILE_ATTRIBUTES &&
        (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
        committed = ReplaceFileA(target.c_str(), temporary.c_str(), nullptr,
            REPLACEFILE_WRITE_THROUGH, nullptr, nullptr) != FALSE;
        if (!committed) {
            committed = MoveFileExA(temporary.c_str(), target.c_str(),
                MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != FALSE;
        }
    } else if (attributes == INVALID_FILE_ATTRIBUTES &&
               GetLastError() == ERROR_FILE_NOT_FOUND) {
        committed = MoveFileExA(temporary.c_str(), target.c_str(),
                                MOVEFILE_WRITE_THROUGH) != FALSE;
    }
    if (!committed) DeleteFileA(temporary.c_str());
    return committed;
}

std::string counted_path(const unsigned char* st_file) {
    if (st_file == nullptr || st_file[0] == 0 || st_file[0] >= 120) {
        return {};
    }
    return std::string(reinterpret_cast<const char*>(st_file + 1),
                       st_file[0]);
}

constexpr Word kDlmInit = 0x0001;
constexpr Word kDlmTerm = 0x0003;
constexpr Word kDlmExit = 0x0004;
constexpr Word kDlmChange = 0x0005;
constexpr Word kDlmClick = 0x0006;
constexpr Word kDlmDblClk = 0x0007;
constexpr Word kDlmSetItemFocus = 0x000b;
constexpr Word kDlmKillItemFocus = 0x000c;
constexpr Word kDlmSetDialogFocus = 0x000d;
constexpr Word kDlmKillDialogFocus = 0x000e;
constexpr Word kDlmDialogClick = 0x0012;
constexpr UINT kWmCommitRibbonSelection = WM_APP + 0x352;
constexpr Word kIddNewDoc = 2;
constexpr Word kIddOpen = 3;
constexpr Word kIddSaveAs = 4;
constexpr Word kIddCharacter = 16;
constexpr Word kIddApplyStyle = 23;
constexpr Word kIddDefineStyle = 24;
constexpr Word kIddAbout = 44;
constexpr Word kCxtRibbonIconBar = 0x8005;
constexpr Word kCxtRulerIconBar = 0x8006;
constexpr Tmc kTmcOk = 1;
constexpr Tmc kTmcCancel = 2;
constexpr Tmc kTmcUserMin = 0x0400;
constexpr Tmc kTmcCharacterName = kTmcUserMin;
constexpr Tmc kTmcCharacterSize = kTmcUserMin + 2;
constexpr Tmc kTmcCharacterColor = kTmcUserMin + 4;
constexpr Tmc kTmcApplyStyle = kTmcUserMin;
constexpr Tmc kTmcApplyDefine = kTmcUserMin + 2;
constexpr Tmc kTmcApplyBanter = kTmcUserMin + 3;
constexpr Tmc kTmcDefineStyle = kTmcUserMin;
constexpr Tmc kTmcDefineChars = kTmcUserMin + 2;
constexpr Tmc kTmcDefineParas = kTmcUserMin + 3;
constexpr Tmc kTmcDefineTabs = kTmcUserMin + 4;
constexpr Tmc kTmcDefinePosition = kTmcUserMin + 5;
constexpr Tmc kTmcDefineOptions = kTmcUserMin + 6;
constexpr Tmc kTmcDefineBanter = kTmcUserMin + 7;
constexpr Tmc kTmcDefineBasedOn = kTmcUserMin + 8;
constexpr Tmc kTmcDefineNext = kTmcUserMin + 10;
constexpr Tmc kTmcDefineTemplate = kTmcUserMin + 12;
constexpr Tmc kTmcDefineCommit = kTmcUserMin + 13;
constexpr Tmc kTmcDefineDelete = kTmcUserMin + 14;
constexpr Tmc kTmcDefineRename = kTmcUserMin + 15;
constexpr Tmc kTmcDefineMerge = kTmcUserMin + 16;
constexpr Word kTmmCount = 2;
constexpr Word kTmmText = 3;
constexpr Word kUnknownListCount = 0xffff;
constexpr Tmc kTmcSummary = 0x0400;
constexpr Tmc kTmcNewDot = 0x0401;
constexpr Tmc kTmcRNewDoc = 0x0402;
constexpr Tmc kTmcRNewDot = 0x0403;
constexpr Tmc kTmcNewType = 0x0404;
constexpr Tmc kTmcNewTypeList = 0x0405;
constexpr Tmc kTmcOpenFileName = 0x0400;
constexpr Tmc kTmcOpenFileList = 0x0401;
constexpr Tmc kTmcOpenFileDir = 0x0402;
constexpr Tmc kTmcOpenCatalog = 0x0403;
constexpr Tmc kTmcOpenReadOnly = 0x0404;
constexpr Tmc kTmcSaveFile = kTmcUserMin;
constexpr Tmc kTmcSaveOptions = kTmcUserMin + 1;
constexpr Tmc kTmcSaveDirectoryText = kTmcUserMin + 2;
constexpr Tmc kTmcSaveDirectoryList = kTmcUserMin + 3;
constexpr Tmc kTmcSaveDirectory = kTmcUserMin + 4;
constexpr Tmc kTmcSaveFormatPrompt = kTmcUserMin + 5;
constexpr Tmc kTmcSaveFormat = kTmcUserMin + 6;
constexpr int kSaveFormatRtf = 6;
constexpr Tmc kTmcSaveQuick = kTmcUserMin + 7;
constexpr Tmc kTmcSaveBackup = kTmcUserMin + 8;
constexpr Tmc kTmcSaveLockAnnotations = kTmcUserMin + 9;
constexpr Word kOpenFileNameIag = 1;
constexpr std::size_t kOpenCabBytes = 24;
constexpr std::size_t kOpenReadOnlyOffset = 20;
constexpr Word kNewTypeIag = 1;
constexpr std::size_t kNewCabBytes = 24;
constexpr std::size_t kNewDotOffset = 16;

using NativeDialogProc = int (*)(Word, Tmc, Word, Word, Word);

void set_sds_handle(Hdlg current, Hdlg focus);
LRESULT CALLBACK native_dialog_window_proc(HWND, UINT, WPARAM, LPARAM);
void handle_dialog_command(Hdlg, WPARAM, LPARAM);

DialogState* find_dialog(const Hdlg dialog) {
    const auto found = g_dialogs.find(dialog);
    return found == g_dialogs.end() ? nullptr : &found->second;
}

DialogState& active_dialog() {
    auto* dialog = find_dialog(g_current_dialog);
    return dialog == nullptr ? g_no_dialog : *dialog;
}

#define g_dialog active_dialog()

ControlState& control(const Tmc tmc) {
    return g_dialog.controls[static_cast<Tmc>(tmc & ~0x8000u)];
}

const ControlState* find_control(const Tmc tmc) {
    const auto found =
        g_dialog.controls.find(static_cast<Tmc>(tmc & ~0x8000u));
    return found == g_dialog.controls.end() ? nullptr : &found->second;
}

void sync_current_dialog_globals() {
    const auto* dialog = find_dialog(g_current_dialog);
    hcabDlgCur = dialog == nullptr ? nullptr : dialog->cab;
    wRefDlgCur = dialog == nullptr ? 0 : dialog->reference;
    set_sds_handle(g_current_dialog, g_focus_dialog);
}

int scaled_x(const int value) {
    return MulDiv(value, dac.dxSysFontChar == 0 ? 8 : dac.dxSysFontChar, 4);
}

int scaled_y(const int value) {
    return MulDiv(value, dac.dySysFontChar == 0 ? 16 : dac.dySysFontChar, 8);
}

ATOM ensure_native_dialog_class() {
    static ATOM atom = 0;
    if (atom != 0) {
        return atom;
    }
    WNDCLASSEXA cls{};
    cls.cbSize = sizeof(cls);
    cls.style = CS_DBLCLKS;
    cls.lpfnWndProc = native_dialog_window_proc;
    cls.hInstance = GetModuleHandleW(nullptr);
    cls.hCursor = LoadCursorA(nullptr, IDC_ARROW);
    cls.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
    cls.lpszClassName = "OpusSdmDialog";
    atom = RegisterClassExA(&cls);
    if (atom == 0 && GetLastError() == ERROR_CLASS_ALREADY_EXISTS) {
        atom = 1;
    }
    return atom;
}

HWND create_dialog_host(const DialogState& dialog, const Dli* initializer) {
    if (dialog.modal &&
        (dialog.hid == kIddOpen || dialog.hid == kIddNewDoc ||
         dialog.hid == kIddSaveAs || dialog.hid == kIddAbout ||
         dialog.hid == kIddCharacter || dialog.hid == kIddApplyStyle ||
         dialog.hid == kIddDefineStyle)) {
        if (ensure_native_dialog_class() == 0) {
            return nullptr;
        }

        HWND owner = initializer != nullptr ? initializer->hwnd : nullptr;
        if (owner == nullptr || !IsWindow(owner)) {
            owner = vhWndMsgBoxParent;
        }
        if (owner == nullptr || !IsWindow(owner)) {
            owner = GetActiveWindow();
        }

        int client_width = scaled_x(dialog.hid == kIddNewDoc ? 127 : 206);
        int client_height = scaled_y(dialog.hid == kIddNewDoc ? 114 : 104);
        if (dialog.template_handle != nullptr &&
            *dialog.template_handle != nullptr) {
            client_width = (std::max)(1, scaled_x(
                (*dialog.template_handle)->rec.dx));
            client_height = (std::max)(1, scaled_y(
                (*dialog.template_handle)->rec.dy));
        }
        constexpr DWORD style =
            WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_CLIPCHILDREN;
        constexpr DWORD extended_style =
            WS_EX_DLGMODALFRAME | WS_EX_CONTROLPARENT;
        RECT window_rect{0, 0, client_width, client_height};
        AdjustWindowRectEx(&window_rect, style, false, extended_style);
        const int width = window_rect.right - window_rect.left;
        const int height = window_rect.bottom - window_rect.top;

        RECT anchor{};
        if (owner == nullptr || !GetWindowRect(owner, &anchor)) {
            SystemParametersInfoA(SPI_GETWORKAREA, 0, &anchor, 0);
        }
        const int x = anchor.left +
                      (anchor.right - anchor.left - width) / 2;
        const int y = anchor.top +
                      (anchor.bottom - anchor.top - height) / 2;
        const char* caption = "Open";
        switch (dialog.hid) {
            case kIddNewDoc:
                caption = "New";
                break;
            case kIddSaveAs:
                caption = "Save As";
                break;
            case kIddCharacter:
                caption = "Character";
                break;
            case kIddApplyStyle:
                caption = "Apply Style";
                break;
            case kIddDefineStyle:
                caption = "Define Style";
                break;
            case kIddAbout:
                caption = "About Microsoft Word";
                break;
        }
        return CreateWindowExA(
            extended_style, "OpusSdmDialog", caption, style, x, y, width,
            height, owner, nullptr, GetModuleHandleW(nullptr),
            reinterpret_cast<void*>(static_cast<std::uintptr_t>(
                dialog.handle)));
    }

    if (initializer == nullptr || initializer->hwnd == nullptr ||
        !IsWindow(initializer->hwnd)) {
        return nullptr;
    }
    if (ensure_native_dialog_class() == 0) {
        return nullptr;
    }

    int x = initializer->dx;
    int y = initializer->dy;
    int width = 1;
    int height = 1;
    if (dialog.template_handle != nullptr &&
        *dialog.template_handle != nullptr) {
        const auto& rec = (*dialog.template_handle)->rec;
        x += scaled_x(rec.x);
        y += scaled_y(rec.y);
        width = (std::max)(1, scaled_x(rec.dx));
        height = (std::max)(1, scaled_y(rec.dy));
    }

    /* HdlgStartDlg is used by Opus only for ribbon/ruler child dialogs. Win16
     * propagated WM_SETVISIBLE when their icon-bar parent was shown; Win64 does
     * not. Keep the child style visible so parent visibility is inherited
     * without depending on that retired message.
     */
    const DWORD style =
        WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
    return CreateWindowExA(
        0, "OpusSdmDialog", "", style, x, y, width, height,
        initializer->hwnd, nullptr, GetModuleHandleW(nullptr),
        reinterpret_cast<void*>(static_cast<std::uintptr_t>(dialog.handle)));
}

HWND create_native_control(DialogState& dialog, const Tmc tmc,
                           const char* window_class, const char* caption,
                           const Rec& rectangle, const DWORD control_style) {
    auto& state = dialog.controls[tmc];
    if (_stricmp(window_class, "BUTTON") == 0) {
        state.kind = ControlKind::button;
    } else if (_stricmp(window_class, "COMBOBOX") == 0) {
        state.kind = ControlKind::combo;
    } else if (_stricmp(window_class, "EDIT") == 0) {
        state.kind = ControlKind::edit;
    } else if (_stricmp(window_class, "LISTBOX") == 0) {
        state.kind = ControlKind::list;
    } else if (_stricmp(window_class, "STATIC") == 0) {
        state.kind = ControlKind::statik;
    } else {
        state.kind = ControlKind::unknown;
    }
    state.rectangle = rectangle;
    state.text = caption == nullptr ? "" : caption;
    state.visible = (control_style & WS_VISIBLE) != 0 ||
                    (control_style & WS_CHILD) == 0;
    return state.window;
}

void create_static_text(DialogState& dialog, const char* caption,
                        const Rec& rectangle) {
    ControlState state{};
    state.kind = ControlKind::statik;
    state.rectangle = rectangle;
    state.text = caption == nullptr ? "" : caption;
    dialog.untracked_controls.push_back(std::move(state));
}

void create_untracked_control(DialogState& dialog, const char* window_class,
                              const char* caption, const Rec& rectangle,
                              const DWORD control_style) {
    ControlState state{};
    if (_stricmp(window_class, "BUTTON") == 0) {
        state.kind = ControlKind::button;
    } else if (_stricmp(window_class, "STATIC") == 0) {
        state.kind = ControlKind::statik;
    }
    state.rectangle = rectangle;
    state.text = caption == nullptr ? "" : caption;
    state.visible = (control_style & WS_VISIBLE) != 0 ||
                    (control_style & WS_CHILD) == 0;
    dialog.untracked_controls.push_back(std::move(state));
}

std::string join_path(const std::string& directory,
                      const std::string& leaf) {
    if (directory.empty()) {
        return leaf;
    }
    const char last = directory.back();
    return directory + (last == '\\' || last == '/' ? "" : "\\") + leaf;
}

void add_native_list_entry(DialogState& dialog, const Tmc tmc,
                           const std::string& entry) {
    auto& state = dialog.controls[tmc];
    state.entries.push_back(entry);
}

void set_open_directory_label(DialogState& dialog) {
    dialog.controls[kTmcOpenFileDir].text = dialog.current_directory;
}

void establish_open_directory(DialogState& dialog,
                              const std::string& specification) {
    char full_path[32768] = {};
    char* file_part = nullptr;
    const std::string source = specification.empty() ? "*.*" : specification;
    const DWORD length = GetFullPathNameA(
        source.c_str(), static_cast<DWORD>(sizeof(full_path)), full_path,
        &file_part);
    if (length == 0 || length >= sizeof(full_path)) {
        GetCurrentDirectoryA(static_cast<DWORD>(sizeof(full_path)), full_path);
        dialog.current_directory = full_path;
        dialog.file_pattern = "*.*";
        return;
    }

    const DWORD attributes = GetFileAttributesA(full_path);
    if (attributes != INVALID_FILE_ATTRIBUTES &&
        (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
        dialog.current_directory = full_path;
        dialog.file_pattern = "*.*";
        return;
    }

    dialog.file_pattern =
        file_part == nullptr || *file_part == '\0' ? "*.*" : file_part;
    if (file_part == nullptr || file_part == full_path) {
        GetCurrentDirectoryA(static_cast<DWORD>(sizeof(full_path)), full_path);
        dialog.current_directory = full_path;
    } else {
        std::size_t directory_length =
            static_cast<std::size_t>(file_part - full_path);
        while (directory_length > 3 &&
               (full_path[directory_length - 1] == '\\' ||
                full_path[directory_length - 1] == '/')) {
            --directory_length;
        }
        dialog.current_directory.assign(full_path, directory_length);
    }
}

void populate_open_lists(DialogState& dialog) {
    auto& files = dialog.controls[kTmcOpenFileList];
    auto& directories = dialog.controls[kTmcOpenFileDir];
    files.entries.clear();
    directories.entries.clear();

    WIN32_FIND_DATAA find_data{};
    HANDLE find = FindFirstFileA(
        join_path(dialog.current_directory, dialog.file_pattern).c_str(),
        &find_data);
    if (find != INVALID_HANDLE_VALUE) {
        do {
            if ((find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
                add_native_list_entry(dialog, kTmcOpenFileList,
                                      find_data.cFileName);
            }
        } while (FindNextFileA(find, &find_data));
        FindClose(find);
    }

    char parent_path[32768] = {};
    const std::string parent_spec = join_path(dialog.current_directory, "..");
    if (GetFullPathNameA(parent_spec.c_str(),
                         static_cast<DWORD>(sizeof(parent_path)), parent_path,
                         nullptr) != 0 &&
        _stricmp(parent_path, dialog.current_directory.c_str()) != 0) {
        add_native_list_entry(dialog, kTmcOpenFileDir, "[..]");
    }
    find = FindFirstFileA(join_path(dialog.current_directory, "*.*").c_str(),
                          &find_data);
    if (find != INVALID_HANDLE_VALUE) {
        do {
            if ((find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0 &&
                std::strcmp(find_data.cFileName, ".") != 0 &&
                std::strcmp(find_data.cFileName, "..") != 0) {
                add_native_list_entry(
                    dialog, kTmcOpenFileDir,
                    "[" + std::string(find_data.cFileName) + "]");
            }
        } while (FindNextFileA(find, &find_data));
        FindClose(find);
    }
    set_open_directory_label(dialog);
}

void read_open_cab(DialogState& dialog) {
    char file_name[32768] = {};
    if (dialog.cab != nullptr) {
        GetCabSz(dialog.cab, file_name,
                 static_cast<Word>((std::min)(sizeof(file_name),
                                              std::size_t{0xffffu})),
                 kOpenFileNameIag);
    }
    auto& edit = dialog.controls[kTmcOpenFileName];
    edit.text = file_name;

    if (dialog.cab != nullptr && *dialog.cab != nullptr &&
        OpusCbOfH(dialog.cab) >= kOpenCabBytes) {
        int read_only = 0;
        std::memcpy(&read_only,
                    static_cast<const unsigned char*>(*dialog.cab) +
                        kOpenReadOnlyOffset,
                    sizeof(read_only));
        auto& checkbox = dialog.controls[kTmcOpenReadOnly];
        checkbox.value = read_only != 0;
    }
    establish_open_directory(dialog, edit.text);
    populate_open_lists(dialog);
}

void sync_open_cab(DialogState& dialog) {
    auto edit = dialog.controls.find(kTmcOpenFileName);
    if (dialog.cab != nullptr && edit != dialog.controls.end()) {
        FSetCabSz(dialog.cab, edit->second.text.c_str(), kOpenFileNameIag);
    }

    auto checkbox = dialog.controls.find(kTmcOpenReadOnly);
    if (dialog.cab != nullptr && *dialog.cab != nullptr &&
        OpusCbOfH(dialog.cab) >= kOpenCabBytes &&
        checkbox != dialog.controls.end()) {
        const int read_only = checkbox->second.value != 0;
        std::memcpy(static_cast<unsigned char*>(*dialog.cab) +
                        kOpenReadOnlyOffset,
                    &read_only, sizeof(read_only));
    }
}

void populate_new_type_list(DialogState& dialog) {
    auto& list = dialog.controls[kTmcNewTypeList];
    list.entries.clear();

    const std::string initial = dialog.controls[kTmcNewType].text;
    add_native_list_entry(dialog, kTmcNewTypeList,
                          initial.empty() ? "NORMAL" : initial);

    WIN32_FIND_DATAA find_data{};
    HANDLE find = FindFirstFileA("*.DOT", &find_data);
    if (find == INVALID_HANDLE_VALUE) {
        return;
    }
    do {
        if ((find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
            continue;
        }
        const auto duplicate = std::find_if(
            list.entries.begin(), list.entries.end(),
            [&find_data](const std::string& entry) {
                return _stricmp(entry.c_str(), find_data.cFileName) == 0;
            });
        if (duplicate == list.entries.end()) {
            add_native_list_entry(dialog, kTmcNewTypeList,
                                  find_data.cFileName);
        }
    } while (FindNextFileA(find, &find_data));
    FindClose(find);
}

void read_new_cab(DialogState& dialog) {
    char type_name[32768] = {};
    if (dialog.cab != nullptr) {
        GetCabSz(dialog.cab, type_name,
                 static_cast<Word>((std::min)(sizeof(type_name),
                                              std::size_t{0xffffu})),
                 kNewTypeIag);
    }
    auto& edit = dialog.controls[kTmcNewType];
    edit.text = type_name;

    bool new_template = false;
    if (dialog.cab != nullptr && *dialog.cab != nullptr &&
        OpusCbOfH(dialog.cab) >= kNewCabBytes) {
        int value = 0;
        std::memcpy(&value,
                    static_cast<const unsigned char*>(*dialog.cab) +
                        kNewDotOffset,
                    sizeof(value));
        new_template = value != 0;
    }
    dialog.controls[kTmcNewDot].value = new_template;
    dialog.controls[kTmcRNewDoc].value = !new_template;
    dialog.controls[kTmcRNewDot].value = new_template;
    populate_new_type_list(dialog);
}

void sync_new_cab(DialogState& dialog) {
    auto edit = dialog.controls.find(kTmcNewType);
    if (dialog.cab != nullptr && edit != dialog.controls.end()) {
        FSetCabSz(dialog.cab, edit->second.text.c_str(), kNewTypeIag);
    }

    bool new_template = false;
    const auto radio = dialog.controls.find(kTmcRNewDot);
    if (radio != dialog.controls.end()) {
        new_template = radio->second.value != 0;
    }
    dialog.controls[kTmcNewDot].value = new_template;
    if (dialog.cab != nullptr && *dialog.cab != nullptr &&
        OpusCbOfH(dialog.cab) >= kNewCabBytes) {
        const int value = new_template;
        std::memcpy(static_cast<unsigned char*>(*dialog.cab) + kNewDotOffset,
                    &value, sizeof(value));
    }
}

void materialize_new_template(DialogState& dialog) {
    if (dialog.hid != kIddNewDoc || dialog.window == nullptr) {
        return;
    }
    create_native_control(dialog, kTmcOk, "BUTTON", "OK", {74, 6, 46, 14},
                          WS_TABSTOP | BS_DEFPUSHBUTTON);
    create_native_control(dialog, kTmcCancel, "BUTTON", "Cancel",
                          {74, 23, 46, 14}, WS_TABSTOP | BS_PUSHBUTTON);
    create_native_control(dialog, kTmcSummary, "BUTTON", "&Summary...",
                          {74, 39, 46, 14}, WS_TABSTOP | BS_PUSHBUTTON);
    create_untracked_control(dialog, "BUTTON", "New", {5, 2, 61, 36},
                             BS_GROUPBOX);
    create_native_control(dialog, kTmcRNewDoc, "BUTTON", "&Document",
                          {9, 12, 42, 10},
                          WS_TABSTOP | WS_GROUP | BS_AUTORADIOBUTTON);
    create_native_control(dialog, kTmcRNewDot, "BUTTON", "&Template",
                          {9, 24, 42, 10},
                          WS_TABSTOP | BS_AUTORADIOBUTTON);
    create_static_text(dialog, "&Use Template:", {5, 42, 53, 10});
    create_native_control(dialog, kTmcNewType, "EDIT", "",
                          {5, 53, 61, 12},
                          WS_TABSTOP | WS_BORDER | ES_AUTOHSCROLL);
    create_native_control(dialog, kTmcNewTypeList, "LISTBOX", "",
                          {8, 65, 68, 40},
                          WS_TABSTOP | WS_BORDER | WS_VSCROLL | LBS_NOTIFY |
                              LBS_SORT | LBS_NOINTEGRALHEIGHT);
    dialog.caption = "New";
    dialog.native_modal = true;
    read_new_cab(dialog);
}

void materialize_open_template(DialogState& dialog) {
    if (dialog.hid != kIddOpen || dialog.window == nullptr) {
        return;
    }
    create_native_control(dialog, kTmcOpenFileName, "EDIT", "",
                          {67, 6, 83, 12},
                          WS_TABSTOP | WS_BORDER | ES_AUTOHSCROLL);
    create_static_text(dialog, "Open File &Name:", {5, 7, 61, 9});
    create_native_control(dialog, kTmcOpenFileList, "LISTBOX", "",
                          {5, 32, 60, 64},
                          WS_TABSTOP | WS_BORDER | WS_VSCROLL | LBS_NOTIFY |
                              LBS_SORT | LBS_NOINTEGRALHEIGHT);
    create_static_text(dialog, "&Files:", {5, 21, 25, 9});
    create_native_control(dialog, kTmcOpenFileDir, "LISTBOX", "",
                          {71, 48, 65, 48},
                          WS_TABSTOP | WS_BORDER | WS_VSCROLL | LBS_NOTIFY |
                              LBS_SORT | LBS_NOINTEGRALHEIGHT);
    create_static_text(dialog, "&Directories:", {71, 38, 50, 9});
    create_untracked_control(dialog, "STATIC", "", {70, 24, 78, 8}, SS_LEFT);
    create_native_control(dialog, kTmcOk, "BUTTON", "OK",
                          {154, 5, 47, 14},
                          WS_TABSTOP | BS_DEFPUSHBUTTON);
    create_native_control(dialog, kTmcCancel, "BUTTON", "Cancel",
                          {154, 22, 47, 14}, WS_TABSTOP | BS_PUSHBUTTON);
    create_native_control(dialog, kTmcOpenCatalog, "BUTTON", "F&ind...",
                          {154, 39, 47, 14}, WS_TABSTOP | BS_PUSHBUTTON);
    create_native_control(dialog, kTmcOpenReadOnly, "BUTTON", "&Read Only",
                          {155, 56, 47, 12},
                          WS_TABSTOP | BS_AUTOCHECKBOX);
    dialog.caption = "Open";
    dialog.native_modal = true;
    read_open_cab(dialog);
}

std::string read_cab_string(const DialogState& dialog, const Word iag) {
    char buffer[1024] = {};
    if (dialog.cab != nullptr) {
        GetCabSz(dialog.cab, buffer, static_cast<Word>(sizeof(buffer)), iag);
    }
    return buffer;
}

void materialize_about_template(DialogState& dialog) {
    if (dialog.hid != kIddAbout || dialog.window == nullptr) {
        return;
    }

    const auto static_from_cab = [&dialog](const Word iag, const Rec& rec,
                                           const DWORD alignment) {
        const std::string text = read_cab_string(dialog, iag);
        create_untracked_control(dialog, "STATIC", text.c_str(), rec,
                                 alignment);
    };
    static_from_cab(1, {4, 5, 192, 9}, SS_CENTER);
    static_from_cab(2, {4, 22, 192, 9}, SS_CENTER);
    static_from_cab(3, {4, 34, 192, 9}, SS_CENTER);
    create_native_control(dialog, kTmcOk, "BUTTON", "OK",
                          {84, 48, 34, 14},
                          WS_TABSTOP | BS_DEFPUSHBUTTON);
    create_untracked_control(dialog, "STATIC", "", {0, 64, 205, 4},
                             SS_ETCHEDHORZ);
    create_untracked_control(dialog, "STATIC", "Conventional Memory:",
                             {5, 74, 110, 9}, SS_RIGHT);
    create_untracked_control(dialog, "STATIC", "Expanded Memory:",
                             {5, 83, 110, 9}, SS_RIGHT);
    create_untracked_control(dialog, "STATIC", "Math Co-processor:",
                             {5, 92, 110, 9}, SS_RIGHT);
    create_untracked_control(dialog, "STATIC", "Disk Space:",
                             {5, 101, 110, 9}, SS_RIGHT);
    static_from_cab(4, {120, 74, 76, 9}, SS_LEFT);
    static_from_cab(5, {120, 83, 76, 9}, SS_LEFT);
    static_from_cab(6, {120, 92, 76, 9}, SS_LEFT);
    static_from_cab(7, {120, 101, 76, 9}, SS_LEFT);
    dialog.caption = "About Microsoft Word";
    dialog.native_modal = true;
}

struct CabSaveNative {
    Word simple_words;
    Word handle_words;
    Word sab;
    Word alignment;
    char** file_name;
    int directory_list;
    int format;
    int quick_save;
    int backup;
    int lock_annotations;
    int options;
};

CabSaveNative* save_cab(DialogState& dialog) {
    if (dialog.cab == nullptr || *dialog.cab == nullptr ||
        OpusCbOfH(dialog.cab) < sizeof(CabSaveNative)) {
        return nullptr;
    }
    return static_cast<CabSaveNative*>(*dialog.cab);
}

void set_save_check(DialogState& dialog, const Tmc tmc, const bool checked) {
    auto& state = dialog.controls[tmc];
    state.value = checked;
}

void populate_save_directories(DialogState& dialog) {
    char directory[32768] = {};
    if (dialog.current_directory.empty()) {
        GetCurrentDirectoryA(static_cast<DWORD>(sizeof(directory)), directory);
        dialog.current_directory = directory;
    }

    auto& list = dialog.controls[kTmcSaveDirectoryList];
    list.entries.clear();

    char parent[32768] = {};
    const std::string parent_spec = join_path(dialog.current_directory, "..");
    if (GetFullPathNameA(parent_spec.c_str(),
                         static_cast<DWORD>(sizeof(parent)), parent,
                         nullptr) != 0 &&
        _stricmp(parent, dialog.current_directory.c_str()) != 0) {
        add_native_list_entry(dialog, kTmcSaveDirectoryList, "[..]");
    }

    WIN32_FIND_DATAA find_data{};
    HANDLE find = FindFirstFileA(
        join_path(dialog.current_directory, "*.*").c_str(), &find_data);
    if (find != INVALID_HANDLE_VALUE) {
        do {
            if ((find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0 &&
                std::strcmp(find_data.cFileName, ".") != 0 &&
                std::strcmp(find_data.cFileName, "..") != 0) {
                add_native_list_entry(
                    dialog, kTmcSaveDirectoryList,
                    "[" + std::string(find_data.cFileName) + "]");
            }
        } while (FindNextFileA(find, &find_data));
        FindClose(find);
    }

    auto& label = dialog.controls[kTmcSaveDirectory];
    label.text = dialog.current_directory;
}

void set_save_options_visible(DialogState& dialog, const bool visible) {
    const Tmc option_controls[] = {
        kTmcSaveFormatPrompt, kTmcSaveFormat, kTmcSaveQuick,
        kTmcSaveBackup, kTmcSaveLockAnnotations};
    for (const Tmc tmc : option_controls) {
        auto found = dialog.controls.find(tmc);
        if (found == dialog.controls.end()) {
            continue;
        }
        found->second.visible = visible;
    }
    if (dialog.window == nullptr || !IsWindow(dialog.window)) {
        return;
    }
    RECT client{0, 0, scaled_x(150), scaled_y(visible ? 157 : 102)};
    const DWORD style = static_cast<DWORD>(GetWindowLongPtrA(
        dialog.window, GWL_STYLE));
    const DWORD extended_style = static_cast<DWORD>(GetWindowLongPtrA(
        dialog.window, GWL_EXSTYLE));
    AdjustWindowRectEx(&client, style, false, extended_style);
    SetWindowPos(dialog.window, nullptr, 0, 0,
                 client.right - client.left, client.bottom - client.top,
                 SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
}

void read_save_cab(DialogState& dialog) {
    auto& edit = dialog.controls[kTmcSaveFile];
    edit.text = read_cab_string(dialog, 1);

    if (auto* cab = save_cab(dialog); cab != nullptr) {
        set_save_check(dialog, kTmcSaveQuick, cab->quick_save != 0);
        set_save_check(dialog, kTmcSaveBackup, cab->backup != 0);
        set_save_check(dialog, kTmcSaveLockAnnotations,
                       cab->lock_annotations != 0);
        auto& format = dialog.controls[kTmcSaveFormat];
        format.value = static_cast<Word>(cab->format);
        const std::string entry = "Current document format";
        format.entries = {entry};
    }
    populate_save_directories(dialog);
}

void sync_save_cab(DialogState& dialog) {
    auto& edit = dialog.controls[kTmcSaveFile];
    if (dialog.cab != nullptr) {
        FSetCabSz(dialog.cab, edit.text.c_str(), 1);
    }
    if (auto* cab = save_cab(dialog); cab != nullptr) {
        const auto checked = [&dialog](const Tmc tmc) {
            const auto found = dialog.controls.find(tmc);
            return found != dialog.controls.end() && found->second.value != 0;
        };
        cab->quick_save = checked(kTmcSaveQuick);
        cab->backup = checked(kTmcSaveBackup);
        cab->lock_annotations = checked(kTmcSaveLockAnnotations);
        cab->options = dialog.sab == 0 &&
                       dialog.controls[kTmcSaveFormat].visible;
    }
}

void materialize_save_as_template(DialogState& dialog) {
    if (dialog.hid != kIddSaveAs || dialog.window == nullptr) {
        return;
    }
    create_static_text(dialog, "Save File &Name:", {4, 2, 60, 9});
    create_native_control(dialog, kTmcSaveFile, "EDIT", "",
                          {4, 13, 81, 12},
                          WS_TABSTOP | WS_BORDER | ES_AUTOHSCROLL);
    create_native_control(dialog, kTmcOk, "BUTTON", "OK", {97, 6, 47, 14},
                          WS_TABSTOP | BS_DEFPUSHBUTTON);
    create_native_control(dialog, kTmcCancel, "BUTTON", "Cancel",
                          {97, 25, 47, 14}, WS_TABSTOP | BS_PUSHBUTTON);
    create_native_control(dialog, kTmcSaveOptions, "BUTTON", "&Options >>",
                          {97, 42, 47, 14}, WS_TABSTOP | BS_PUSHBUTTON);
    create_native_control(dialog, kTmcSaveDirectoryText, "STATIC",
                          "&Directories:", {4, 38, 73, 9}, SS_LEFT);
    create_native_control(dialog, kTmcSaveDirectoryList, "LISTBOX", "",
                          {4, 50, 89, 48},
                          WS_TABSTOP | WS_BORDER | WS_VSCROLL | LBS_NOTIFY |
                              LBS_SORT | LBS_NOINTEGRALHEIGHT);
    create_native_control(dialog, kTmcSaveDirectory, "STATIC", "",
                          {4, 27, 90, 9}, SS_LEFT);
    create_native_control(dialog, kTmcSaveFormatPrompt, "STATIC",
                          "&File Format:", {4, 105, 50, 9}, SS_LEFT);
    create_native_control(dialog, kTmcSaveFormat, "COMBOBOX", "",
                          {55, 103, 91, 72},
                          WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWNLIST);
    create_native_control(dialog, kTmcSaveQuick, "BUTTON", "Fast &Save",
                          {4, 116, 46, 12},
                          WS_TABSTOP | BS_AUTOCHECKBOX);
    create_native_control(dialog, kTmcSaveBackup, "BUTTON", "Create &Backup",
                          {4, 128, 62, 12},
                          WS_TABSTOP | BS_AUTOCHECKBOX);
    create_native_control(dialog, kTmcSaveLockAnnotations, "BUTTON",
                          "&Lock for Annotations", {4, 140, 92, 12},
                          WS_TABSTOP | BS_AUTOCHECKBOX);
    dialog.caption = "Save As";
    dialog.native_modal = true;
    read_save_cab(dialog);
    set_save_options_visible(dialog, false);
}

unsigned int preview_color(const ControlKind kind) {
    switch (kind) {
        case ControlKind::button:
            return 0xffc0c0c0u;
        case ControlKind::edit:
            return 0xffffffffu;
        case ControlKind::list:
            return 0xfff8f8f8u;
        case ControlKind::combo:
            return 0xffe8f0f8u;
        case ControlKind::statik:
            return 0xfff0f0f0u;
        case ControlKind::unknown:
            break;
    }
    return 0xffd0d0d0u;
}

void preview_set_pixel(unsigned int* pixels, const int width, const int height,
                       const int x, const int y, const unsigned int color) {
    if (x < 0 || y < 0 || x >= width || y >= height) {
        return;
    }
    pixels[y * width + x] = color;
}

void preview_fill_rect(unsigned int* pixels, const int width, const int height,
                       const Rec& rec, const unsigned int color) {
    const int left = (std::max)(0, rec.x);
    const int top = (std::max)(0, rec.y);
    const int right = (std::min)(width, rec.x + (std::max)(1, rec.dx));
    const int bottom = (std::min)(height, rec.y + (std::max)(1, rec.dy));
    for (int y = top; y < bottom; ++y) {
        for (int x = left; x < right; ++x) {
            pixels[y * width + x] = color;
        }
    }
}

void preview_frame_rect(unsigned int* pixels, const int width, const int height,
                        const Rec& rec, const unsigned int color) {
    const int right = rec.x + (std::max)(1, rec.dx) - 1;
    const int bottom = rec.y + (std::max)(1, rec.dy) - 1;
    for (int x = rec.x; x <= right; ++x) {
        preview_set_pixel(pixels, width, height, x, rec.y, color);
        preview_set_pixel(pixels, width, height, x, bottom, color);
    }
    for (int y = rec.y; y <= bottom; ++y) {
        preview_set_pixel(pixels, width, height, rec.x, y, color);
        preview_set_pixel(pixels, width, height, right, y, color);
    }
}

void preview_draw_text_mark(unsigned int* pixels, const int width,
                            const int height, const Rec& rec,
                            const std::string& text) {
    if (text.empty()) {
        return;
    }
    if (text.find('/') != std::string::npos ||
        text.find('\\') != std::string::npos) {
        return;
    }
    if (text.size() >= 2 &&
        std::isalpha(static_cast<unsigned char>(text[0])) && text[1] == ':') {
        return;
    }
    unsigned int hash = 2166136261u;
    for (const unsigned char ch : text) {
        if (ch == '&') {
            continue;
        }
        hash ^= ch;
        hash *= 16777619u;
    }
    const int y = rec.y + rec.dy / 2;
    const int count = (std::min)(rec.dx - 2, 3 + static_cast<int>(hash % 9u));
    for (int index = 0; index < count; ++index) {
        const unsigned int bit = (hash >> (index % 16)) & 1u;
        preview_set_pixel(pixels, width, height, rec.x + 1 + index, y,
                          bit ? 0xff000000u : 0xff606060u);
    }
}

void preview_render_control(unsigned int* pixels, const int width,
                            const int height, const ControlState& control) {
    if (!control.visible) {
        return;
    }
    preview_fill_rect(pixels, width, height, control.rectangle,
                      preview_color(control.kind));
    preview_frame_rect(pixels, width, height, control.rectangle, 0xff000000u);
    preview_draw_text_mark(pixels, width, height, control.rectangle,
                           control.text);
}

int render_dialog_preview(const Word hid, unsigned int* pixels,
                          const int width, const int height) {
    if (pixels == nullptr || width <= 0 || height <= 0) {
        return 0;
    }
    DialogState dialog{};
    dialog.hid = hid;
    dialog.modal = true;
    dialog.window = create_dialog_host(dialog, nullptr);
    if (dialog.window == nullptr) {
        return 0;
    }
    dialog.visible = true;

    materialize_save_as_template(dialog);
    materialize_about_template(dialog);
    if (dialog.controls.empty() && dialog.untracked_controls.empty()) {
        DestroyWindow(dialog.window);
        return 0;
    }

    std::fill(pixels, pixels + static_cast<std::size_t>(width) * height,
              0xffffffffu);
    preview_frame_rect(pixels, width, height, {0, 0, width, height},
                       0xff000000u);

    for (const auto& control : dialog.untracked_controls) {
        preview_render_control(pixels, width, height, control);
    }

    std::vector<std::pair<Tmc, const ControlState*>> controls;
    controls.reserve(dialog.controls.size());
    for (const auto& entry : dialog.controls) {
        controls.push_back({entry.first, &entry.second});
    }
    std::sort(controls.begin(), controls.end(),
              [](const auto& left, const auto& right) {
                  return left.first < right.first;
              });
    for (const auto& entry : controls) {
        preview_render_control(pixels, width, height, *entry.second);
    }
    DestroyWindow(dialog.window);
    return 1;
}

bool is_font_name_control(const DialogState& dialog, const Tmc tmc) {
    return (dialog.hid == kCxtRibbonIconBar && tmc == kTmcUserMin) ||
           (dialog.hid == kIddCharacter && tmc == kTmcCharacterName);
}

bool is_font_size_control(const DialogState& dialog, const Tmc tmc) {
    return (dialog.hid == kCxtRibbonIconBar && tmc == kTmcUserMin + 1) ||
           (dialog.hid == kIddCharacter && tmc == kTmcCharacterSize);
}

void refresh_font_control_value(DialogState& dialog, const Tmc raw_tmc,
                                ControlState& state,
                                const bool) {
    const Tmc tmc = static_cast<Tmc>(raw_tmc & ~0x8000u);

    if (is_font_name_control(dialog, tmc) &&
        g_font_name_to_value != nullptr && !state.text.empty()) {
        state.value =
            static_cast<Word>(g_font_name_to_value(state.text.c_str()));
    } else if (is_font_size_control(dialog, tmc) &&
               g_font_size_to_value != nullptr && !state.text.empty()) {
        state.value =
            static_cast<Word>(g_font_size_to_value(state.text.c_str()));
    }
}

int CALLBACK collect_system_font(const LOGFONTA* logical_font,
                                 const TEXTMETRICA*, DWORD, LPARAM parameter) {
    if (logical_font == nullptr || logical_font->lfFaceName[0] == '\0' ||
        logical_font->lfFaceName[0] == '@') {
        return 1;
    }
    auto* fonts = reinterpret_cast<std::vector<std::string>*>(parameter);
    fonts->emplace_back(logical_font->lfFaceName);
    return 1;
}

std::vector<std::string> installed_windows_fonts() {
    std::vector<std::string> fonts;
    const HDC dc = GetDC(nullptr);
    if (dc != nullptr) {
        LOGFONTA logical_font{};
        logical_font.lfCharSet = DEFAULT_CHARSET;
        EnumFontFamiliesExA(dc, &logical_font,
                            reinterpret_cast<FONTENUMPROCA>(collect_system_font),
                            reinterpret_cast<LPARAM>(&fonts), 0);
        ReleaseDC(nullptr, dc);
    }
    std::sort(fonts.begin(), fonts.end(),
              [](const std::string& left, const std::string& right) {
                  return _stricmp(left.c_str(), right.c_str()) < 0;
              });
    fonts.erase(std::unique(fonts.begin(), fonts.end(),
                            [](const std::string& left,
                               const std::string& right) {
                                return _stricmp(left.c_str(), right.c_str()) == 0;
                            }),
                fonts.end());
    return fonts;
}

void replace_list_entries(DialogState& dialog, const Tmc tmc,
                          const std::vector<std::string>& entries) {
    auto& state = dialog.controls[tmc];
    const std::string edit_text = state.text;
    state.entries = entries;
    state.text = edit_text;
}

bool populate_windows_font_control(DialogState& dialog, const Tmc tmc) {
    if (is_font_name_control(dialog, tmc)) {
        replace_list_entries(dialog, tmc, installed_windows_fonts());
        return !dialog.controls[tmc].entries.empty();
    }
    if (is_font_size_control(dialog, tmc)) {
        static const std::vector<std::string> sizes = {
            "8",  "9",  "10", "11", "12", "14", "16", "18",
            "20", "22", "24", "26", "28", "36", "48", "72"};
        replace_list_entries(dialog, tmc, sizes);
        return true;
    }
    return false;
}

struct OriginalListBinding {
    OriginalListProc proc = nullptr;
    Word parameter = 0;
};

OriginalListBinding original_list_binding(const DialogState& dialog,
                                          const Tmc raw_tmc) {
    const Tmc tmc = static_cast<Tmc>(raw_tmc & ~0x8000u);
    if (dialog.hid == kCxtRibbonIconBar) {
        if (tmc == kTmcUserMin) {
            return {g_list_font_name, 0};
        }
        if (tmc == kTmcUserMin + 1) {
            return {g_list_font_size, kTmcUserMin};
        }
    } else if (dialog.hid == kCxtRulerIconBar && tmc == kTmcUserMin) {
        return {g_list_styles, 0};
    } else if (dialog.hid == kIddCharacter) {
        if (tmc == kTmcCharacterName) {
            return {g_list_font_name, 0};
        }
        if (tmc == kTmcCharacterSize) {
            return {g_list_font_size, kTmcCharacterName};
        }
        if (tmc == kTmcCharacterColor) {
            return {g_list_character_color, 0};
        }
    } else if (dialog.hid == kIddApplyStyle && tmc == kTmcApplyStyle) {
        return {g_list_styles, 0};
    } else if (dialog.hid == kIddDefineStyle &&
               (tmc == kTmcDefineStyle || tmc == kTmcDefineBasedOn ||
                tmc == kTmcDefineNext)) {
        return {g_list_styles, 0};
    }
    return {};
}

bool populate_original_list(DialogState& dialog, const Tmc raw_tmc) {
    const Tmc tmc = static_cast<Tmc>(raw_tmc & ~0x8000u);
    if (populate_windows_font_control(dialog, tmc)) {
        return true;
    }
    const auto binding = original_list_binding(dialog, tmc);
    if (binding.proc == nullptr) {
        return false;
    }

    auto& state = dialog.controls[tmc];
    const std::string edit_text = state.text;
    state.entries.clear();

    const Hdlg previous_current = g_current_dialog;
    g_current_dialog = dialog.handle;
    sync_current_dialog_globals();

    char buffer[512] = {};
    const Word reported_count =
        binding.proc(kTmmCount, buffer, 0, 0, tmc, binding.parameter);
    const unsigned limit = reported_count == kUnknownListCount
                               ? 4096u
                               : static_cast<unsigned>(reported_count);
    for (unsigned index = 0; index < limit; ++index) {
        buffer[0] = '\0';
        if (binding.proc(kTmmText, buffer, static_cast<int>(index), 0, tmc,
                         binding.parameter) == 0) {
            break;
        }
        state.entries.emplace_back(buffer);
    }

    g_current_dialog = previous_current;
    sync_current_dialog_globals();
    state.text = edit_text;
    return !state.entries.empty();
}

void read_style_cab(DialogState& dialog, const Tmc tmc) {
    char style_name[256] = {};
    if (dialog.cab != nullptr) {
        /* Pointer arguments begin at iag 1 in the native, aligned CAB layout
         * used by CABAPPLYSTYLE and CABDEFINESTYLE.
         */
        GetCabSz(dialog.cab, style_name,
                 static_cast<Word>(sizeof(style_name)), 1);
    }
    auto& state = dialog.controls[tmc];
    state.text = style_name;
}

void sync_style_cab(DialogState& dialog, const Tmc tmc) {
    auto found = dialog.controls.find(tmc);
    if (found == dialog.controls.end()) {
        return;
    }
    auto& state = found->second;
    if (dialog.cab != nullptr) {
        FSetCabSz(dialog.cab, state.text.c_str(), 1);
    }
}

struct CabCharacterNative {
    Word simple_words;
    Word handle_words;
    Word sab;
    Word alignment;
    int ftc;
    int hps;
    int color;
    int bold;
    int italic;
    int small_caps;
    int hidden;
    int underline;
    int word_underline;
    int double_underline;
    int position;
    int position_amount;
    int spacing;
    int spacing_amount;
};

CabCharacterNative* character_cab(DialogState& dialog) {
    if (dialog.cab == nullptr || *dialog.cab == nullptr ||
        OpusCbOfH(dialog.cab) < sizeof(CabCharacterNative)) {
        return nullptr;
    }
    return static_cast<CabCharacterNative*>(*dialog.cab);
}

void set_native_check(DialogState& dialog, const Tmc tmc, const int value) {
    auto& state = dialog.controls[tmc];
    state.value = static_cast<Word>(value);
}

void read_character_cab(DialogState& dialog) {
    auto* cab = character_cab(dialog);
    if (cab == nullptr) {
        return;
    }
    char font_name[LF_FACESIZE] = {};
    if (g_font_name_from_value != nullptr) {
        g_font_name_from_value(cab->ftc, font_name,
                               static_cast<int>(sizeof(font_name)));
    }
    auto& font = dialog.controls[kTmcCharacterName];
    font.text = font_name;
    font.value = static_cast<Word>(cab->ftc);

    char size_text[32] = {};
    if (cab->hps >= 0 && cab->hps != 0x8001) {
        if ((cab->hps & 1) == 0) {
            std::snprintf(size_text, sizeof(size_text), "%d", cab->hps / 2);
        } else {
            std::snprintf(size_text, sizeof(size_text), "%d.5", cab->hps / 2);
        }
    }
    auto& size = dialog.controls[kTmcCharacterSize];
    size.text = size_text;
    size.value = static_cast<Word>(cab->hps);

    if (cab->color >= 0 &&
        static_cast<std::size_t>(cab->color) <
            dialog.controls[kTmcCharacterColor].entries.size()) {
        dialog.controls[kTmcCharacterColor].value =
            static_cast<Word>(cab->color);
    }
    set_native_check(dialog, kTmcUserMin + 5, cab->bold);
    set_native_check(dialog, kTmcUserMin + 6, cab->italic);
    set_native_check(dialog, kTmcUserMin + 7, cab->small_caps);
    set_native_check(dialog, kTmcUserMin + 8, cab->hidden);
    set_native_check(dialog, kTmcUserMin + 9, cab->underline);
    set_native_check(dialog, kTmcUserMin + 10, cab->word_underline);
    set_native_check(dialog, kTmcUserMin + 11, cab->double_underline);
    const int position = cab->position >= 0 && cab->position <= 2
                             ? cab->position
                             : 0;
    set_native_check(dialog, static_cast<Tmc>(kTmcUserMin + 13 + position), 1);
    const int spacing = cab->spacing >= 0 && cab->spacing <= 2
                            ? cab->spacing
                            : 0;
    set_native_check(dialog, static_cast<Tmc>(kTmcUserMin + 18 + spacing), 1);
}

void sync_character_cab(DialogState& dialog) {
    auto* cab = character_cab(dialog);
    if (cab == nullptr) {
        return;
    }
    const auto read_text = [&dialog](const Tmc tmc) {
        auto& state = dialog.controls[tmc];
        return state.text;
    };
    if (g_font_name_to_value != nullptr) {
        const int ftc =
            g_font_name_to_value(read_text(kTmcCharacterName).c_str());
        if (ftc >= 0) {
            cab->ftc = ftc;
        }
    }
    if (g_font_size_to_value != nullptr) {
        const int hps =
            g_font_size_to_value(read_text(kTmcCharacterSize).c_str());
        if (hps >= 0) {
            cab->hps = hps;
        }
    }
    cab->color = dialog.controls[kTmcCharacterColor].value;
    const auto checked = [&dialog](const Tmc tmc) {
        return dialog.controls[tmc].value != 0;
    };
    cab->bold = checked(kTmcUserMin + 5);
    cab->italic = checked(kTmcUserMin + 6);
    cab->small_caps = checked(kTmcUserMin + 7);
    cab->hidden = checked(kTmcUserMin + 8);
    cab->underline = checked(kTmcUserMin + 9);
    cab->word_underline = checked(kTmcUserMin + 10);
    cab->double_underline = checked(kTmcUserMin + 11);
    for (int index = 0; index < 3; ++index) {
        if (checked(static_cast<Tmc>(kTmcUserMin + 13 + index))) {
            cab->position = index;
        }
        if (checked(static_cast<Tmc>(kTmcUserMin + 18 + index))) {
            cab->spacing = index;
        }
    }
}

void materialize_character_template(DialogState& dialog) {
    if (dialog.hid != kIddCharacter || dialog.window == nullptr) {
        return;
    }
    constexpr DWORD combo_style =
        WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWN | CBS_AUTOHSCROLL;
    create_static_text(dialog, "Character", {4, 4, 44, 9});
    create_static_text(dialog, "&Font:", {4, 14, 35, 9});
    create_native_control(dialog, kTmcCharacterName, "COMBOBOX", "",
                          {4, 24, 80, 68}, combo_style);
    create_static_text(dialog, "&Points:", {88, 14, 35, 9});
    create_native_control(dialog, kTmcCharacterSize, "COMBOBOX", "",
                          {88, 24, 40, 68}, combo_style);
    create_static_text(dialog, "Co&lor:", {4, 38, 35, 9});
    create_native_control(dialog, kTmcCharacterColor, "COMBOBOX", "",
                          {4, 48, 44, 72},
                          WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWNLIST);

    const struct {
        Tmc tmc;
        const char* caption;
        Rec rec;
    } checks[] = {
        {kTmcUserMin + 5, "&Bold", {5, 63, 26, 12}},
        {kTmcUserMin + 6, "&Italic", {5, 75, 34, 12}},
        {kTmcUserMin + 7, "Small &Caps", {5, 87, 50, 12}},
        {kTmcUserMin + 8, "&Hidden", {5, 99, 34, 12}},
        {kTmcUserMin + 9, "&Underline", {5, 111, 46, 12}},
        {kTmcUserMin + 10, "&Word underline", {5, 123, 66, 12}},
        {kTmcUserMin + 11, "&Double underline", {5, 135, 74, 12}},
    };
    for (const auto& check : checks) {
        create_native_control(dialog, check.tmc, "BUTTON", check.caption,
                              check.rec, WS_TABSTOP | BS_AUTOCHECKBOX);
    }

    create_untracked_control(dialog, "BUTTON", "Position", {84, 44, 89, 48},
                             BS_GROUPBOX);
    create_native_control(dialog, kTmcUserMin + 13, "BUTTON", "&Normal",
                          {87, 54, 45, 12},
                          WS_TABSTOP | WS_GROUP | BS_AUTORADIOBUTTON);
    create_native_control(dialog, kTmcUserMin + 14, "BUTTON", "&Superscript",
                          {87, 65, 54, 12},
                          WS_TABSTOP | BS_AUTORADIOBUTTON);
    create_native_control(dialog, kTmcUserMin + 15, "BUTTON", "Subsc&ript",
                          {87, 76, 50, 12},
                          WS_TABSTOP | BS_AUTORADIOBUTTON);
    create_static_text(dialog, "B&y:", {141, 67, 13, 9});
    create_native_control(dialog, kTmcUserMin + 16, "EDIT", "",
                          {141, 77, 30, 12},
                          WS_TABSTOP | WS_BORDER | ES_AUTOHSCROLL);

    create_untracked_control(dialog, "BUTTON", "Character Spacing",
                             {84, 96, 89, 49}, BS_GROUPBOX);
    create_native_control(dialog, kTmcUserMin + 18, "BUTTON", "N&ormal",
                          {87, 107, 45, 12},
                          WS_TABSTOP | WS_GROUP | BS_AUTORADIOBUTTON);
    create_native_control(dialog, kTmcUserMin + 19, "BUTTON", "&Expanded",
                          {87, 119, 46, 12},
                          WS_TABSTOP | BS_AUTORADIOBUTTON);
    create_native_control(dialog, kTmcUserMin + 20, "BUTTON", "&Condensed",
                          {87, 130, 50, 12},
                          WS_TABSTOP | BS_AUTORADIOBUTTON);
    create_static_text(dialog, "By&:", {141, 120, 12, 9});
    create_native_control(dialog, kTmcUserMin + 21, "EDIT", "",
                          {141, 130, 30, 12},
                          WS_TABSTOP | WS_BORDER | ES_AUTOHSCROLL);
    create_native_control(dialog, kTmcOk, "BUTTON", "OK", {140, 6, 34, 14},
                          WS_TABSTOP | BS_DEFPUSHBUTTON);
    create_native_control(dialog, kTmcCancel, "BUTTON", "Cancel",
                          {140, 23, 34, 14}, WS_TABSTOP | BS_PUSHBUTTON);
    dialog.caption = "Character";
    dialog.native_modal = true;
    populate_original_list(dialog, kTmcCharacterName);
    populate_original_list(dialog, kTmcCharacterSize);
    populate_original_list(dialog, kTmcCharacterColor);
    read_character_cab(dialog);
}

void materialize_apply_style_template(DialogState& dialog) {
    if (dialog.hid != kIddApplyStyle || dialog.window == nullptr) {
        return;
    }
    create_static_text(dialog, "&Style Name:", {4, 3, 55, 9});
    create_native_control(dialog, kTmcApplyStyle, "COMBOBOX", "",
                          {4, 13, 96, 60},
                          WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWN |
                              CBS_AUTOHSCROLL);
    create_native_control(dialog, kTmcOk, "BUTTON", "OK", {104, 6, 40, 14},
                          WS_TABSTOP | BS_DEFPUSHBUTTON);
    create_native_control(dialog, kTmcCancel, "BUTTON", "Cancel",
                          {104, 23, 40, 14}, WS_TABSTOP | BS_PUSHBUTTON);
    create_native_control(dialog, kTmcApplyDefine, "BUTTON", "&Define...",
                          {103, 39, 42, 14}, WS_TABSTOP | BS_PUSHBUTTON);
    create_native_control(dialog, kTmcApplyBanter, "STATIC", "",
                          {4, 75, 141, 24}, SS_LEFT);
    dialog.caption = "Apply Style";
    dialog.native_modal = true;
    read_style_cab(dialog, kTmcApplyStyle);
    populate_original_list(dialog, kTmcApplyStyle);
}

void materialize_define_style_template(DialogState& dialog) {
    if (dialog.hid != kIddDefineStyle || dialog.window == nullptr) {
        return;
    }
    create_static_text(dialog, "Define &Style Name:", {4, 3, 78, 10});
    create_native_control(dialog, kTmcDefineStyle, "COMBOBOX", "",
                          {4, 14, 76, 60},
                          WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWN |
                              CBS_AUTOHSCROLL);
    create_native_control(dialog, kTmcDefineChars, "BUTTON", "&Character...",
                          {85, 14, 52, 14}, WS_TABSTOP | BS_PUSHBUTTON);
    create_native_control(dialog, kTmcDefineParas, "BUTTON", "&Paragraph...",
                          {85, 30, 52, 14}, WS_TABSTOP | BS_PUSHBUTTON);
    create_native_control(dialog, kTmcDefineTabs, "BUTTON", "&Tabs...",
                          {85, 46, 52, 14}, WS_TABSTOP | BS_PUSHBUTTON);
    create_native_control(dialog, kTmcDefinePosition, "BUTTON", "Pos&ition...",
                          {85, 62, 52, 14}, WS_TABSTOP | BS_PUSHBUTTON);
    create_native_control(dialog, kTmcOk, "BUTTON", "OK", {140, 14, 45, 14},
                          WS_TABSTOP | BS_DEFPUSHBUTTON);
    create_native_control(dialog, kTmcCancel, "BUTTON", "Cancel",
                          {140, 30, 45, 14}, WS_TABSTOP | BS_PUSHBUTTON);
    create_native_control(dialog, kTmcDefineOptions, "BUTTON", "&Options >>",
                          {140, 62, 45, 14}, WS_TABSTOP | BS_PUSHBUTTON);
    create_native_control(dialog, kTmcDefineBanter, "STATIC", "",
                          {4, 76, 180, 24}, SS_LEFT);
    create_native_control(dialog, kTmcDefineCommit, "BUTTON", "&Define",
                          {4, 129, 43, 14}, WS_TABSTOP | BS_PUSHBUTTON);
    create_native_control(dialog, kTmcDefineDelete, "BUTTON", "De&lete",
                          {49, 129, 43, 14}, WS_TABSTOP | BS_PUSHBUTTON);
    create_native_control(dialog, kTmcDefineRename, "BUTTON", "&Rename...",
                          {94, 129, 43, 14}, WS_TABSTOP | BS_PUSHBUTTON);
    create_native_control(dialog, kTmcDefineMerge, "BUTTON", "&Merge...",
                          {139, 129, 43, 14}, WS_TABSTOP | BS_PUSHBUTTON);
    dialog.caption = "Define Style";
    dialog.native_modal = true;
    read_style_cab(dialog, kTmcDefineStyle);
    populate_original_list(dialog, kTmcDefineStyle);
}

void materialize_icon_bar_template(DialogState& dialog) {
    constexpr DWORD combo_style =
        WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWN | CBS_AUTOHSCROLL;

    if (dialog.hid == kCxtRibbonIconBar) {
        const bool win3 = (*dialog.template_handle)->rec.dx == 160;
        if (win3) {
            create_static_text(dialog, "Font:", {4, 3, 20, 8});
            create_native_control(dialog, kTmcUserMin, "COMBOBOX", "",
                                  {26, 1, 76, 67}, combo_style);
            create_static_text(dialog, "Pts:", {108, 3, 14, 8});
            create_native_control(dialog, kTmcUserMin + 1, "COMBOBOX", "",
                                  {127, 1, 28, 67}, combo_style);
        } else {
            create_static_text(dialog, "Font:", {4, 3, 20, 8});
            create_native_control(dialog, kTmcUserMin, "COMBOBOX", "",
                                  {29, 1, 80, 68}, combo_style);
            create_static_text(dialog, "Pts:", {115, 3, 16, 8});
            create_native_control(dialog, kTmcUserMin + 1, "COMBOBOX", "",
                                  {134, 1, 32, 68}, combo_style);
        }
    } else if (dialog.hid == kCxtRulerIconBar) {
        const bool win3 = (*dialog.template_handle)->rec.dx == 102;
        if (win3) {
            create_static_text(dialog, "Style:", {4, 3, 20, 8});
            create_native_control(dialog, kTmcUserMin, "COMBOBOX", "",
                                  {26, 1, 76, 68}, combo_style);
        } else {
            create_static_text(dialog, "Style:", {4, 3, 24, 8});
            create_native_control(dialog, kTmcUserMin, "COMBOBOX", "",
                                  {29, 1, 80, 68}, combo_style);
        }
    }
    if (dialog.hid == kCxtRibbonIconBar) {
        populate_original_list(dialog, kTmcUserMin);
        populate_original_list(dialog, kTmcUserMin + 1);
    }
}

HWND ensure_control_window(const Tmc raw_tmc) {
    auto& state = control(raw_tmc);
    if (state.window != nullptr && IsWindow(state.window)) {
        return state.window;
    }
    return nullptr;
}

void copy_text(const std::string& source, char* destination,
               const Word capacity) {
    if (destination == nullptr || capacity == 0) {
        return;
    }
    const std::size_t count =
        (std::min)(source.size(), static_cast<std::size_t>(capacity - 1));
    std::memcpy(destination, source.data(), count);
    destination[count] = '\0';
}

std::string counted_or_zero_terminated(const char* text) {
    if (text == nullptr) {
        return {};
    }
    return std::string(text);
}

bool invoke_dialog_proc(DialogState& dialog, const Word message,
                        const Tmc tmc, const Word new_value = 0,
                        const Word old_value = 0,
                        const Word parameter = 0) {
    if (dialog.template_handle == nullptr ||
        *dialog.template_handle == nullptr ||
        (*dialog.template_handle)->dialog_proc == nullptr) {
        return true;
    }
    const auto proc = reinterpret_cast<NativeDialogProc>(
        (*dialog.template_handle)->dialog_proc);
    return proc(message, tmc, new_value, old_value, parameter) != 0;
}

void ensure_icon_bar_commands_active(DialogState& dialog, const Tmc tmc) {
    if (dialog.hid != kCxtRibbonIconBar || dialog.commands_active) {
        return;
    }
    dialog.commands_active = true;
    invoke_dialog_proc(dialog, kDlmSetDialogFocus, tmc);
    invoke_dialog_proc(dialog, kDlmSetItemFocus, tmc);
}

void commit_ribbon_list_selection(DialogState& dialog, const Tmc tmc) {
    if (dialog.hid != kCxtRibbonIconBar || !dialog.commands_active) {
        return;
    }

    /* The Win16 SDM/icon-bar loop applied a combo selection when its dialog
     * focus was terminated. A native Win32 drop-down keeps focus on the combo
     * after the list closes, so that termination never reliably occurs.
     * CBN_SELENDOK identifies the completed choice, after all intermediate
     * CBN_SELCHANGE notifications, and is the safe point to send the same
     * original callbacks. Typed edit text still commits on CBN_KILLFOCUS.
     */
    OpusX64TraceRibbon("commit-begin", kDlmKillDialogFocus, tmc,
                       dialog.controls[tmc].value, dialog.commands_active,
                       0, 0, 0);
    const bool item_result =
        invoke_dialog_proc(dialog, kDlmKillItemFocus, tmc);
    const bool dialog_result =
        invoke_dialog_proc(dialog, kDlmKillDialogFocus, tmc);
    dialog.commands_active = false;

    /* Win16 SDM returned focus to the document when an icon-bar session ended.
     * A native combo otherwise retains focus, forcing the user to click the
     * document; that mouse click reloads the properties at the caret and
     * discards the just-applied insertion font and size. Route the focus return
     * through Microsoft's original FDlgIb handler.
     */
    const bool focus_result =
        invoke_dialog_proc(dialog, kDlmDialogClick, tmc);
    OpusX64TraceRibbon("commit-end", kDlmKillDialogFocus, tmc,
                       item_result, dialog_result, focus_result, 0, 0);
}

std::string selected_list_text(const ControlState& control_state) {
    if (control_state.value >= control_state.entries.size()) {
        return {};
    }
    return control_state.entries[control_state.value];
}

void select_new_type(DialogState& dialog) {
    const auto found = dialog.controls.find(kTmcNewTypeList);
    if (found == dialog.controls.end()) {
        return;
    }
    const std::string selected = selected_list_text(found->second);
    if (selected.empty()) {
        return;
    }
    auto& edit = dialog.controls[kTmcNewType];
    edit.text = selected;
}

void select_open_file(DialogState& dialog) {
    const auto found = dialog.controls.find(kTmcOpenFileList);
    if (found == dialog.controls.end()) {
        return;
    }
    const std::string selected = selected_list_text(found->second);
    if (selected.empty()) {
        return;
    }
    auto& edit = dialog.controls[kTmcOpenFileName];
    edit.text = join_path(dialog.current_directory, selected);
}

void enter_open_directory(DialogState& dialog) {
    const auto found = dialog.controls.find(kTmcOpenFileDir);
    if (found == dialog.controls.end()) {
        return;
    }
    std::string selected = selected_list_text(found->second);
    if (selected.size() < 2 || selected.front() != '[' ||
        selected.back() != ']') {
        return;
    }
    selected = selected.substr(1, selected.size() - 2);
    char full_path[32768] = {};
    const std::string destination =
        join_path(dialog.current_directory, selected);
    if (GetFullPathNameA(destination.c_str(),
                         static_cast<DWORD>(sizeof(full_path)), full_path,
                         nullptr) == 0) {
        return;
    }
    dialog.current_directory = full_path;
    populate_open_lists(dialog);
    auto& edit = dialog.controls[kTmcOpenFileName];
    edit.text = join_path(dialog.current_directory, dialog.file_pattern);
}

void enter_save_directory(DialogState& dialog) {
    const auto found = dialog.controls.find(kTmcSaveDirectoryList);
    if (found == dialog.controls.end()) {
        return;
    }
    std::string selected = selected_list_text(found->second);
    if (selected.size() < 2 || selected.front() != '[' ||
        selected.back() != ']') {
        return;
    }
    selected = selected.substr(1, selected.size() - 2);
    char full_path[32768] = {};
    const std::string destination =
        join_path(dialog.current_directory, selected);
    if (GetFullPathNameA(destination.c_str(),
                         static_cast<DWORD>(sizeof(full_path)), full_path,
                         nullptr) == 0) {
        return;
    }
    dialog.current_directory = full_path;
    populate_save_directories(dialog);

    auto& edit = dialog.controls[kTmcSaveFile];
    const std::size_t leaf = edit.text.find_last_of("\\/");
    const std::string file_name =
        leaf == std::string::npos ? edit.text : edit.text.substr(leaf + 1);
    edit.text = file_name.empty() ? file_name
                                  : join_path(dialog.current_directory,
                                              file_name);
}

void finish_native_dialog(DialogState& dialog, const Tmc result) {
    if (dialog.hid == kIddOpen) {
        sync_open_cab(dialog);
    } else if (dialog.hid == kIddSaveAs) {
        sync_save_cab(dialog);
    } else if (dialog.hid == kIddNewDoc) {
        sync_new_cab(dialog);
    } else if (dialog.hid == kIddApplyStyle) {
        sync_style_cab(dialog, kTmcApplyStyle);
    } else if (dialog.hid == kIddDefineStyle) {
        sync_style_cab(dialog, kTmcDefineStyle);
    } else if (dialog.hid == kIddCharacter) {
        sync_character_cab(dialog);
    }
    if (!invoke_dialog_proc(dialog, kDlmTerm, result)) {
        return;
    }
    dialog.result_tmc = result;
    dialog.dying = true;
    dialog.visible = false;
    if (dialog.window != nullptr && IsWindow(dialog.window)) {
        ShowWindow(dialog.window, SW_HIDE);
    }
}

void handle_dialog_command(const Hdlg handle, const WPARAM w_param,
                           const LPARAM l_param) {
    auto* dialog = find_dialog(handle);
    if (dialog == nullptr || dialog->dying) {
        return;
    }
    const Tmc tmc = static_cast<Tmc>(LOWORD(w_param) & ~0x8000u);
    const Word notification = HIWORD(w_param);
    auto found = dialog->controls.find(tmc);
    (void)l_param;
    g_current_dialog = handle;
    g_focus_dialog = handle;
    sync_current_dialog_globals();

    if (dialog->hid == kIddSaveAs && tmc == kTmcSaveDirectoryList &&
        (notification == LBN_SELCHANGE || notification == LBN_DBLCLK)) {
        if (found == dialog->controls.end()) {
            return;
        }
        if (notification == LBN_DBLCLK) {
            enter_save_directory(*dialog);
        }
        invoke_dialog_proc(*dialog,
                           notification == LBN_DBLCLK ? kDlmDblClk
                                                     : kDlmClick,
                           tmc, found->second.value);
        return;
    }
    if (dialog->hid == kIddOpen && tmc == kTmcOpenFileList &&
        (notification == LBN_SELCHANGE || notification == LBN_DBLCLK)) {
        select_open_file(*dialog);
        invoke_dialog_proc(*dialog,
                           notification == LBN_DBLCLK ? kDlmDblClk
                                                     : kDlmClick,
                           tmc);
        if (notification == LBN_DBLCLK && !dialog->dying) {
            finish_native_dialog(*dialog, kTmcOk);
        }
        return;
    }
    if (dialog->hid == kIddOpen && tmc == kTmcOpenFileDir &&
        (notification == LBN_SELCHANGE || notification == LBN_DBLCLK)) {
        invoke_dialog_proc(*dialog,
                           notification == LBN_DBLCLK ? kDlmDblClk
                                                     : kDlmClick,
                           tmc);
        if (notification == LBN_DBLCLK && !dialog->dying) {
            enter_open_directory(*dialog);
        }
        return;
    }
    if (notification == BN_CLICKED &&
        (tmc == kTmcOk || tmc == kTmcCancel ||
         (dialog->hid == kIddOpen && tmc == kTmcOpenCatalog))) {
        finish_native_dialog(*dialog, tmc);
        return;
    }

    if (found != dialog->controls.end() &&
        found->second.kind == ControlKind::combo) {
        OpusX64TraceRibbon("combo", notification, tmc,
                           found->second.value, dialog->commands_active,
                           0, 0, 0);
        if (notification == CBN_SETFOCUS) {
            if (!dialog->commands_active) {
                dialog->commands_active = true;
                invoke_dialog_proc(*dialog, kDlmSetDialogFocus, tmc);
            }
            invoke_dialog_proc(*dialog, kDlmSetItemFocus, tmc);
            return;
        }
        if (notification == CBN_KILLFOCUS) {
            if (dialog->commands_active) {
                invoke_dialog_proc(*dialog, kDlmKillItemFocus, tmc);
                invoke_dialog_proc(*dialog, kDlmKillDialogFocus, tmc);
                dialog->commands_active = false;
            }
            return;
        }
        if (notification == CBN_DROPDOWN) {
            populate_original_list(*dialog, tmc);
            return;
        }
        if (notification == CBN_EDITCHANGE) {
            ensure_icon_bar_commands_active(*dialog, tmc);
            refresh_font_control_value(*dialog, tmc, found->second, true);
            OpusX64TraceRibbon("combo-edit", notification, tmc,
                               found->second.value,
                               dialog->commands_active, 0, 0, 0);
            invoke_dialog_proc(*dialog, kDlmChange, tmc);
            return;
        }
        if (notification == CBN_SELCHANGE) {
            ensure_icon_bar_commands_active(*dialog, tmc);
            const Word selection = found->second.value;
            if (selection < found->second.entries.size()) {
                dialog->controls[static_cast<Tmc>(tmc + 1)].value =
                    static_cast<Word>(selection);
                found->second.value = static_cast<Word>(selection);
                found->second.text = selected_list_text(found->second);
                refresh_font_control_value(*dialog, tmc, found->second,
                                           false);
                OpusX64TraceRibbon("combo-select", notification, tmc,
                                   found->second.value,
                                   static_cast<int>(selection), 0, 0, 0);
                invoke_dialog_proc(*dialog, kDlmClick,
                                   static_cast<Tmc>(tmc + 1));
                invoke_dialog_proc(*dialog, kDlmChange, tmc);
            }
            return;
        }
        if (notification == CBN_SELENDOK) {
            ensure_icon_bar_commands_active(*dialog, tmc);
            PostMessageW(dialog->window, kWmCommitRibbonSelection, tmc, 0);
            return;
        }
    }

    if (!dialog->commands_active) {
        return;
    }

    if (dialog->hid == kIddNewDoc && tmc == kTmcNewType &&
        notification == EN_CHANGE) {
        invoke_dialog_proc(*dialog, kDlmChange, tmc);
        return;
    }
    if (dialog->hid == kIddNewDoc && tmc == kTmcNewTypeList &&
        (notification == LBN_SELCHANGE || notification == LBN_DBLCLK)) {
        select_new_type(*dialog);
        invoke_dialog_proc(*dialog,
                           notification == LBN_DBLCLK ? kDlmDblClk
                                                     : kDlmClick,
                           tmc);
        return;
    }
    if (dialog->hid == kIddOpen && tmc == kTmcOpenFileName &&
        notification == EN_CHANGE) {
        invoke_dialog_proc(*dialog, kDlmChange, tmc);
        return;
    }
    if (dialog->hid == kIddSaveAs && tmc == kTmcSaveFile &&
        notification == EN_CHANGE) {
        invoke_dialog_proc(*dialog, kDlmChange, tmc);
        return;
    }
    if (notification == BN_CLICKED) {
        if (dialog->hid == kIddNewDoc &&
            (tmc == kTmcRNewDoc || tmc == kTmcRNewDot)) {
            const bool new_template = tmc == kTmcRNewDot;
            dialog->controls[kTmcNewDot].value = new_template;
            dialog->controls[kTmcRNewDoc].value = !new_template;
            dialog->controls[kTmcRNewDot].value = new_template;
            invoke_dialog_proc(*dialog, kDlmClick, tmc);
        } else if (tmc == kTmcOk || tmc == kTmcCancel ||
                   (dialog->hid == kIddOpen &&
                    tmc == kTmcOpenCatalog) ||
                   (dialog->hid == kIddNewDoc &&
                    tmc == kTmcSummary)) {
            finish_native_dialog(*dialog, tmc);
        } else {
            const Word new_value = found == dialog->controls.end()
                                       ? 0
                                       : found->second.value;
            invoke_dialog_proc(*dialog, kDlmClick, tmc, new_value);
        }
    }
}

Tmc run_word95_common_file_dialog(DialogState& dialog) {
    const bool opening = dialog.hid == kIddOpen;
    const bool saving = dialog.hid == kIddSaveAs;
    if (!opening && !saving) {
        return static_cast<Tmc>(-1);
    }

    invoke_dialog_proc(dialog, kDlmInit, dialog.focus);
    dialog.commands_active = true;
    if (opening) {
        read_open_cab(dialog);
    } else {
        read_save_cab(dialog);
    }

    HWND owner = dialog.window != nullptr ?
        GetWindow(dialog.window, GW_OWNER) : nullptr;
    if (owner == nullptr || !IsWindow(owner)) {
        owner = vhWndMsgBoxParent;
    }
#ifdef OPUS_TEST_HOOKS
    if (saving && owner != nullptr) {
        SetPropW(owner, OPUSW("OpusX64SaveAsStage"),
                 reinterpret_cast<HANDLE>(1));
    }
#endif

    std::string initial = opening ?
        dialog.controls[kTmcOpenFileName].text :
        dialog.controls[kTmcSaveFile].text;
    const auto initial_alias =
        g_win95_saved_aliases.find(win95_alias_key(initial));
    if (initial_alias != g_win95_saved_aliases.end()) {
        initial = initial_alias->second;
    }
    establish_open_directory(dialog, initial.empty() ? "*.*" : initial);

    Tmc result = kTmcCancel;
    for (;;) {
        char test_path[32768]{};
        DWORD test_path_length = 0;
#ifdef OPUS_TEST_HOOKS
        test_path_length = GetEnvironmentVariableA(
            "WORD1_TEST_FILE_DIALOG_PATH", test_path,
            static_cast<DWORD>(std::size(test_path)));
#else
        (void)test_path;
#endif
        if (test_path_length >= std::size(test_path)) {
#ifdef OPUS_TEST_HOOKS
            if (saving && owner != nullptr) {
                SetPropW(owner, OPUSW("OpusX64SaveAsStage"),
                         reinterpret_cast<HANDLE>(2));
            }
#endif
            invoke_dialog_proc(dialog, kDlmTerm, kTmcCancel);
            result = kTmcCancel;
            break;
        }
        std::string selected_path =
            test_path_length == 0 ? initial : std::string(test_path);
        const bool selected_from_injection = test_path_length != 0;
        if (selected_path.empty()) {
#ifdef OPUS_TEST_HOOKS
            if (saving && owner != nullptr) {
                SetPropW(owner, OPUSW("OpusX64SaveAsStage"),
                         reinterpret_cast<HANDLE>(2));
            }
#endif
            invoke_dialog_proc(dialog, kDlmTerm, kTmcCancel);
            result = kTmcCancel;
            break;
        }
#ifdef OPUS_TEST_HOOKS
        if (test_path_length != 0) {
            SetEnvironmentVariableA("WORD1_TEST_FILE_DIALOG_PATH", nullptr);
        }
#endif

        const bool save_docx =
            saving && OpusModernPathIsDocx(selected_path.c_str());
        const bool save_odt =
            saving && OpusModernPathIsOdt(selected_path.c_str());
        if (save_docx || save_odt) {
            const char* modern_extension = save_docx ? ".docx" : ".odt";
            const std::size_t selected_slash =
                selected_path.find_last_of("\\/");
            const std::size_t selected_dot = selected_path.find_last_of('.');
            if (selected_dot == std::string::npos ||
                (selected_slash != std::string::npos &&
                 selected_dot < selected_slash)) {
                selected_path += modern_extension;
            } else if (_stricmp(selected_path.c_str() + selected_dot,
                                ".doc") == 0) {
                selected_path.replace(selected_dot, std::string::npos,
                                      modern_extension);
            }
        }
        if ((opening && !import_file_within_limit(selected_path)) ||
            (saving && !safe_dialog_file_path(selected_path, false))) {
            MessageBoxA(owner,
                opening ?
                    "This document is not a regular file or is too large to open safely." :
                    "This is not a safe document file name.",
                opening ? "Open" : "Save As",
                MB_OK | MB_ICONEXCLAMATION);
            if (!selected_from_injection) {
                invoke_dialog_proc(dialog, kDlmTerm, kTmcCancel);
                result = kTmcCancel;
                break;
            }
            continue;
        }
        std::string legacy_path;
        const bool docx = OpusModernPathIsDocx(selected_path.c_str()) != 0;
        const bool odt = OpusModernPathIsOdt(selected_path.c_str()) != 0;
        const bool modern = docx || odt;
        if (!make_win95_staging_path(legacy_path,
                                     modern ? ".TXT" : ".DOC")) {
            MessageBoxA(owner,
                "Word could not prepare a temporary document file.",
                opening ? "Open" : "Save As",
                MB_OK | MB_ICONEXCLAMATION);
            if (!selected_from_injection) {
                invoke_dialog_proc(dialog, kDlmTerm, kTmcCancel);
                result = kTmcCancel;
                break;
            }
            continue;
        }
        const bool staged_open = !opening ||
            (docx ?
                OpusModernDocxToTextFile(selected_path.c_str(),
                                          legacy_path.c_str()) != 0 :
             odt ?
                OpusModernOdtToTextFile(selected_path.c_str(),
                                         legacy_path.c_str()) != 0 :
                CopyFileA(selected_path.c_str(), legacy_path.c_str(), FALSE) != 0);
        if (!staged_open) {
            MessageBoxA(owner,
                        docx ? "Word could not read this DOCX document." :
                        odt ? "Word could not read this OpenDocument file." :
                            "Word could not open the selected file.",
                        "Open", MB_OK | MB_ICONEXCLAMATION);
            if (!selected_from_injection) {
                invoke_dialog_proc(dialog, kDlmTerm, kTmcCancel);
                result = kTmcCancel;
                break;
            }
            continue;
        }

        establish_open_directory(dialog, selected_path);
        if (opening) {
            g_win95_saved_aliases[win95_alias_key(legacy_path)] =
                selected_path;
            auto& edit = dialog.controls[kTmcOpenFileName];
            edit.text = legacy_path;
            auto& read_only = dialog.controls[kTmcOpenReadOnly];
            read_only.value = 0;
            sync_open_cab(dialog);
        } else {
            g_win95_save_alias.active = true;
            g_win95_save_alias.created = true;
            g_win95_save_alias.selected_path = selected_path;
            g_win95_save_alias.legacy_path = legacy_path;
            auto& edit = dialog.controls[kTmcSaveFile];
            edit.text = legacy_path;
            sync_save_cab(dialog);
            if (auto* cab = save_cab(dialog);
                cab != nullptr && modern) {
                cab->format = kSaveFormatRtf;
                cab->quick_save = false;
            }
        }

        if (invoke_dialog_proc(dialog, kDlmTerm, kTmcOk)) {
            result = dialog.dying ? dialog.result_tmc : kTmcOk;
            if (saving && result != kTmcOk) {
                if (g_win95_save_alias.created) {
                    DeleteFileA(g_win95_save_alias.legacy_path.c_str());
                }
                g_win95_save_alias = {};
            }
            break;
        }
        if (dialog.dying) {
            result = dialog.result_tmc;
            if (saving) {
                if (g_win95_save_alias.created) {
                    DeleteFileA(g_win95_save_alias.legacy_path.c_str());
                }
                g_win95_save_alias = {};
            }
            break;
        }
        if (saving) {
            if (g_win95_save_alias.created) {
                DeleteFileA(g_win95_save_alias.legacy_path.c_str());
            }
            g_win95_save_alias = {};
        }
    }

    dialog.commands_active = false;
    invoke_dialog_proc(dialog, kDlmExit, result);
    return result;
}

LRESULT CALLBACK native_dialog_window_proc(const HWND window,
                                           const UINT message,
                                           const WPARAM w_param,
                                           const LPARAM l_param) {
    if (message == WM_NCCREATE) {
        const auto* create = reinterpret_cast<const CREATESTRUCTA*>(l_param);
        SetWindowLongPtrA(window, GWLP_USERDATA,
                          reinterpret_cast<LONG_PTR>(create->lpCreateParams));
    }
    const Hdlg handle = static_cast<Hdlg>(reinterpret_cast<std::uintptr_t>(
        reinterpret_cast<void*>(GetWindowLongPtrA(window, GWLP_USERDATA))));
    switch (message) {
        case WM_COMMAND:
            handle_dialog_command(handle, w_param, l_param);
            return 0;
        case kWmCommitRibbonSelection:
            if (auto* dialog = find_dialog(handle); dialog != nullptr) {
                g_current_dialog = handle;
                g_focus_dialog = handle;
                sync_current_dialog_globals();
                ensure_icon_bar_commands_active(
                    *dialog, static_cast<Tmc>(w_param));
                commit_ribbon_list_selection(
                    *dialog, static_cast<Tmc>(w_param));
            }
            return 0;
        case WM_CLOSE:
            if (auto* dialog = find_dialog(handle); dialog != nullptr) {
                finish_native_dialog(*dialog, kTmcCancel);
            }
            return 0;
        default:
            return DefWindowProcA(window, message, w_param, l_param);
    }
}

}  // namespace

extern "C" {

int OpusWin95SaveAliasMatches(const unsigned char* st_file) {
    const std::string path = counted_path(st_file);
    if (path.empty()) {
        return false;
    }
    if (g_win95_save_alias.active &&
        _stricmp(path.c_str(),
                 g_win95_save_alias.legacy_path.c_str()) == 0) {
        return true;
    }
    return g_win95_saved_aliases.find(win95_alias_key(path)) !=
           g_win95_saved_aliases.end();
}

int OpusWin95DisplayAlias(unsigned char* const st_file) {
    const std::string path = counted_path(st_file);
    if (path.empty()) {
        return false;
    }
    auto saved = g_win95_saved_aliases.find(win95_alias_key(path));
    if (saved == g_win95_saved_aliases.end()) {
        char full_path[32768]{};
        const DWORD length = GetFullPathNameA(
            path.c_str(), static_cast<DWORD>(std::size(full_path)),
            full_path, nullptr);
        if (length == 0 || length >= std::size(full_path)) {
            return false;
        }
        saved = g_win95_saved_aliases.find(win95_alias_key(full_path));
        if (saved == g_win95_saved_aliases.end()) {
            return false;
        }
    }

    const char* name = saved->second.c_str();
    if (const char* separator = std::strrchr(name, '\\');
        separator != nullptr) {
        name = separator + 1;
    }
    const std::size_t length = (std::min)(
        std::strlen(name), static_cast<std::size_t>(119));
    st_file[0] = static_cast<unsigned char>(length);
    std::memcpy(st_file + 1, name, length);
    return true;
}

int OpusFinishWin95SaveAlias(const unsigned char* st_file,
                             const int success, const int doc) {
    const std::string path = counted_path(st_file);
    if (path.empty()) {
        return !g_win95_save_alias.active;
    }
    const std::string key = win95_alias_key(path);

    if (g_win95_save_alias.active &&
        _stricmp(path.c_str(),
                 g_win95_save_alias.legacy_path.c_str()) == 0) {
        bool copied = success != 0;
        if (copied) {
            copied = (OpusModernPathIsDocx(
                          g_win95_save_alias.selected_path.c_str()) ||
                      OpusModernPathIsOdt(
                          g_win95_save_alias.selected_path.c_str())) ?
                OpusSaveDocumentAsDocx(
                    doc, g_win95_save_alias.selected_path.c_str()) != 0 :
                atomic_copy_file(g_win95_save_alias.legacy_path,
                                 g_win95_save_alias.selected_path);
        }
        if (!success) {
            DeleteFileA(g_win95_save_alias.legacy_path.c_str());
        } else if (copied) {
            g_win95_saved_aliases[key] =
                g_win95_save_alias.selected_path;
        }
        g_win95_save_alias = {};
        return copied;
    }

    const auto saved = g_win95_saved_aliases.find(key);
    if (saved == g_win95_saved_aliases.end()) {
        return true;
    }
    if (!success) {
        return false;
    }
    return (OpusModernPathIsDocx(saved->second.c_str()) ||
            OpusModernPathIsOdt(saved->second.c_str())) ?
        OpusSaveDocumentAsDocx(doc, saved->second.c_str()) != 0 :
        atomic_copy_file(path, saved->second);
}

int OpusWin95SaveAliasRequiresRtf(const unsigned char* st_file) {
    const std::string path = counted_path(st_file);
    if (path.empty()) return false;
    if (g_win95_save_alias.active &&
        _stricmp(path.c_str(), g_win95_save_alias.legacy_path.c_str()) == 0)
        return OpusModernPathIsDocx(
                   g_win95_save_alias.selected_path.c_str()) ||
               OpusModernPathIsOdt(
                   g_win95_save_alias.selected_path.c_str());
    const auto saved = g_win95_saved_aliases.find(win95_alias_key(path));
    return saved != g_win95_saved_aliases.end() &&
           (OpusModernPathIsDocx(saved->second.c_str()) ||
            OpusModernPathIsOdt(saved->second.c_str()));
}

int OpusWin95OpenAliasIsDocx(const unsigned char* st_file) {
    const std::string path = counted_path(st_file);
    if (path.empty()) return false;
    const auto saved = g_win95_saved_aliases.find(win95_alias_key(path));
    return saved != g_win95_saved_aliases.end() &&
           (OpusModernPathIsDocx(saved->second.c_str()) ||
            OpusModernPathIsOdt(saved->second.c_str()));
}

struct OpusSdsCompat {
    std::uintptr_t current_segment;
    std::uintptr_t focus_segment;
    void** current_dialog;
    void** focus_dialog;
};

OpusSdsCompat sds = {};
DAC_SDM dac = {};
HWND vhWndMsgBoxParent = nullptr;

extern void** hcabDlgCur;
extern std::uintptr_t wRefDlgCur;

void OpusRegisterOriginalDialogCallbacks(
    OriginalListProc list_font_name, OriginalListProc list_font_size,
    OriginalListProc list_styles, OriginalListProc list_character_color,
    FontValueProc font_name_to_value, FontValueProc font_size_to_value,
    FontNameFromValueProc font_name_from_value) {
    g_list_font_name = list_font_name;
    g_list_font_size = list_font_size;
    g_list_styles = list_styles;
    g_list_character_color = list_character_color;
    g_font_name_to_value = font_name_to_value;
    g_font_size_to_value = font_size_to_value;
    g_font_name_from_value = font_name_from_value;
}

int FInitSdm_sdm21(void*) {
    g_initialized = true;
    dac.dxBorder = GetSystemMetrics(SM_CXBORDER);
    dac.dyBorder = GetSystemMetrics(SM_CYBORDER);
    dac.dyCaption = GetSystemMetrics(SM_CYCAPTION);
    dac.dxVScroll = GetSystemMetrics(SM_CXVSCROLL);
    dac.dxSysFontChar = 8;
    dac.dySysFontChar = 16;
    dac.dySysFontAscent = 12;
    dac.clrWindow = GetSysColor(COLOR_WINDOW);
    dac.clrWindowFrame = GetSysColor(COLOR_WINDOWFRAME);
    dac.clrWindowText = GetSysColor(COLOR_WINDOWTEXT);
    dac.clrButton = GetSysColor(COLOR_BTNFACE);
    dac.clrButtonText = GetSysColor(COLOR_BTNTEXT);
    return true;
}

void EndSdm() {
    for (auto& entry : g_dialogs) {
        if (entry.second.window != nullptr && IsWindow(entry.second.window)) {
            DestroyWindow(entry.second.window);
        }
    }
    g_dialogs.clear();
    g_no_dialog = {};
    g_current_dialog = 0;
    g_focus_dialog = 0;
    g_initialized = false;
    sync_current_dialog_globals();
}

Word FtmeIsSdmMessage(MSG*) { return 0; }
void ChangeColors() {}

Hdlg HdlgStartDlg(DltHeader** dialog_template, Hcab cab, Dli* initializer) {
    Hdlg handle = 0;
    for (unsigned attempt = 0; attempt < 0xfffeu; ++attempt) {
        const Hdlg candidate = g_next_dialog;
        ++g_next_dialog;
        if (g_next_dialog == 0 || g_next_dialog == static_cast<Hdlg>(-1)) {
            g_next_dialog = 1;
        }
        if (g_dialogs.find(candidate) == g_dialogs.end()) {
            handle = candidate;
            break;
        }
    }
    if (handle == 0) {
        return 0;
    }

    DialogState dialog{};
    dialog.handle = handle;
    dialog.template_handle = dialog_template;
    dialog.cab = cab;
    if (dialog_template != nullptr && *dialog_template != nullptr) {
        dialog.hid = (*dialog_template)->hid;
        dialog.focus = (*dialog_template)->tmc_sel_init;
    }
    if (initializer != nullptr) {
        dialog.reference = initializer->reference;
        dialog.visible = true;
        dialog.modal = (initializer->flags & 0x00000001u) != 0;
    }
    dialog.window = create_dialog_host(dialog, initializer);
    g_dialogs.emplace(handle, std::move(dialog));
    g_current_dialog = handle;
    g_focus_dialog = handle;
    sync_current_dialog_globals();
    materialize_icon_bar_template(g_dialogs.at(handle));
    materialize_new_template(g_dialogs.at(handle));
    materialize_open_template(g_dialogs.at(handle));
    materialize_save_as_template(g_dialogs.at(handle));
    materialize_about_template(g_dialogs.at(handle));
    materialize_character_template(g_dialogs.at(handle));
    materialize_apply_style_template(g_dialogs.at(handle));
    materialize_define_style_template(g_dialogs.at(handle));
    return handle;
}

Tmc TmcDoDlgDli(DltHeader** dialog_template, Hcab cab, Dli* initializer) {
    const Hdlg previous_current = g_current_dialog;
    const Hdlg previous_focus = g_focus_dialog;
    const Hdlg handle = HdlgStartDlg(dialog_template, cab, initializer);
    if (handle == 0) {
        return static_cast<Tmc>(-1);
    }
    auto* dialog = find_dialog(handle);
    if (dialog == nullptr) {
        return static_cast<Tmc>(-1);
    }

    if (dialog->modal &&
        (dialog->hid == kIddOpen || dialog->hid == kIddSaveAs)) {
        const Tmc common_result = run_word95_common_file_dialog(*dialog);
        dialog = find_dialog(handle);
        if (dialog != nullptr && dialog->window != nullptr &&
            IsWindow(dialog->window)) {
            DestroyWindow(dialog->window);
        }
        g_dialogs.erase(handle);
        g_current_dialog = find_dialog(previous_current) == nullptr ?
            0 : previous_current;
        g_focus_dialog = find_dialog(previous_focus) == nullptr ?
            0 : previous_focus;
        sync_current_dialog_globals();
        return common_result;
    }

    Tmc result = kTmcCancel;
    HWND owner = nullptr;
    bool restore_owner = false;
    if (dialog->modal && dialog->native_modal && dialog->window != nullptr) {
        owner = GetWindow(dialog->window, GW_OWNER);
        restore_owner = owner != nullptr && IsWindow(owner) &&
                        IsWindowEnabled(owner) != 0;
        if (restore_owner) {
            EnableWindow(owner, false);
        }

        invoke_dialog_proc(*dialog, kDlmInit, dialog->focus);
        dialog->commands_active = true;
        if (!dialog->dying) {
            dialog->visible = true;
            ShowWindow(dialog->window, SW_SHOW);
            UpdateWindow(dialog->window);
            SetActiveWindow(dialog->window);
            const auto focus = dialog->controls.find(
                static_cast<Tmc>(dialog->focus & ~0x8000u));
            if (focus != dialog->controls.end() &&
                focus->second.window != nullptr) {
                SetFocus(focus->second.window);
            }
        }

        MSG message{};
        while (!dialog->dying) {
            const int status = GetMessageA(&message, nullptr, 0, 0);
            if (status <= 0) {
                if (status == 0) {
                    PostQuitMessage(static_cast<int>(message.wParam));
                }
                dialog->result_tmc = kTmcCancel;
                dialog->dying = true;
                break;
            }
            if (!IsDialogMessageA(dialog->window, &message)) {
                TranslateMessage(&message);
                DispatchMessageA(&message);
            }
            dialog = find_dialog(handle);
            if (dialog == nullptr) {
                break;
            }
        }
        if (dialog != nullptr) {
            result = dialog->result_tmc;
            dialog->commands_active = false;
            invoke_dialog_proc(*dialog, kDlmExit, result);
        }
    }

    dialog = find_dialog(handle);
    if (dialog != nullptr && dialog->window != nullptr &&
        IsWindow(dialog->window)) {
        DestroyWindow(dialog->window);
    }
    g_dialogs.erase(handle);
    if (restore_owner && owner != nullptr && IsWindow(owner)) {
        EnableWindow(owner, true);
        SetActiveWindow(owner);
    }
    g_current_dialog = find_dialog(previous_current) == nullptr
                           ? 0
                           : previous_current;
    g_focus_dialog = find_dialog(previous_focus) == nullptr ? 0
                                                            : previous_focus;
    sync_current_dialog_globals();
    return result;
}

Tmc TmcDoDlg_sdm21(DltHeader** dialog_template, Hcab cab,
                    unsigned char* runtime_items) {
    Dli initializer{};
    initializer.flags = 0x00000001u;
    initializer.runtime_items = runtime_items;
    return TmcDoDlgDli(dialog_template, cab, &initializer);
}

void EndDlg(Tmc result) {
    g_dialog.result_tmc = result;
    g_dialog.dying = true;
    g_dialog.visible = false;
    if (g_dialog.window != nullptr && IsWindow(g_dialog.window)) {
        ShowWindow(g_dialog.window, SW_HIDE);
    }
}

int FFreeDlg() {
    const Hdlg handle = g_current_dialog;
    auto found = g_dialogs.find(handle);
    if (found == g_dialogs.end()) {
        return false;
    }
    if (found->second.window != nullptr && IsWindow(found->second.window)) {
        DestroyWindow(found->second.window);
    }
    g_dialogs.erase(found);
    if (g_focus_dialog == handle) {
        g_focus_dialog = 0;
    }
    g_current_dialog = 0;
    sync_current_dialog_globals();
    return true;
}

Word HidOfDlg(Hdlg dialog) {
    const auto* state = find_dialog(dialog == 0 ? g_current_dialog : dialog);
    return state == nullptr ? 0 : state->hid;
}

Hdlg HdlgSetCurDlg(Hdlg dialog) {
    const Hdlg previous = g_current_dialog;
    if (dialog == 0 || find_dialog(dialog) != nullptr) {
        g_current_dialog = dialog;
        sync_current_dialog_globals();
    }
    return previous;
}

Hdlg HdlgSetFocusDlg(Hdlg dialog) {
    const Hdlg previous = g_focus_dialog;
    if (dialog == 0 || find_dialog(dialog) != nullptr) {
        g_focus_dialog = dialog;
        sync_current_dialog_globals();
    }
    return previous;
}

int FKillDlgFocus() {
    HdlgSetFocusDlg(0);
    return true;
}

int FModalDlg(Hdlg dialog) {
    const auto* state = find_dialog(dialog);
    return state != nullptr && state->modal;
}

int FIsDlgDying() { return g_dialog.dying; }
void ClearListError(Hdlg) {}

void ShowDlg(int visible) {
    g_dialog.visible = visible != 0;
    if (g_dialog.window != nullptr && IsWindow(g_dialog.window)) {
        ShowWindow(g_dialog.window, visible ? SW_SHOWNA : SW_HIDE);
    }
}
int FVisibleDlg() { return g_dialog.visible; }
void ResizeDlg(int width, int height) {
    if (g_dialog.template_handle != nullptr &&
        *g_dialog.template_handle != nullptr) {
        (*g_dialog.template_handle)->rec.dx = width;
        (*g_dialog.template_handle)->rec.dy = height;
    }
    if (g_dialog.window != nullptr && IsWindow(g_dialog.window)) {
        SetWindowPos(g_dialog.window, nullptr, 0, 0, (std::max)(1, width),
                     (std::max)(1, height),
                     SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
    }
}
void MoveDlg(int x, int y) {
    if (g_dialog.template_handle != nullptr &&
        *g_dialog.template_handle != nullptr) {
        (*g_dialog.template_handle)->rec.x = x;
        (*g_dialog.template_handle)->rec.y = y;
    }
    if (g_dialog.window != nullptr && IsWindow(g_dialog.window)) {
        SetWindowPos(g_dialog.window, nullptr, x, y, 0, 0,
                     SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    }
}
void SdmScaleRec(Rec*) {}

int FSetDlgSab(Word sab) {
    g_dialog.sab = sab;
    if (g_dialog.hid == kIddSaveAs) {
        set_save_options_visible(g_dialog, true);
    }
    return true;
}
Word SabGetDlg() { return g_dialog.sab; }

void SetTmcVal_sdm21(Tmc tmc, Word value) {
    auto& state = control(tmc);
    state.value = value;
}
Word ValGetTmc(Tmc tmc) {
    const Tmc value_tmc = static_cast<Tmc>(tmc & ~0x8000u);
    const auto found = g_dialog.controls.find(value_tmc);
    if (found == g_dialog.controls.end()) {
        return 0;
    }
    refresh_font_control_value(g_dialog, value_tmc, found->second, true);
    return found->second.value;
}

void SetTmcText_sdm21(Tmc tmc, char* text) {
    auto& state = control(tmc);
    state.text = counted_or_zero_terminated(text);
    if (state.text.size() > state.text_limit) {
        state.text.resize(state.text_limit);
    }
    refresh_font_control_value(g_dialog, tmc, state, false);
}
void GetTmcText_sdm21(Tmc tmc, char* destination, Word capacity) {
    const auto* state = find_control(tmc);
    copy_text(state == nullptr ? std::string{} : state->text, destination,
              capacity);
}
Word CchGetTmcText(Tmc tmc, char* destination, Word capacity) {
    const auto* state = find_control(tmc);
    const std::string empty;
    const auto& text = state == nullptr ? empty : state->text;
    copy_text(text, destination, capacity);
    return static_cast<Word>((std::min)(
        text.size(), static_cast<std::size_t>(0xffffu)));
}
Word CchGetTmc(Tmc tmc) { return CchGetTmcText(tmc, nullptr, 0); }

void GetTmcLargeVal(Tmc tmc, void* destination, Word byte_count) {
    if (destination == nullptr) {
        return;
    }
    const auto* state = find_control(tmc);
    const std::size_t count = state == nullptr
                                  ? 0
                                  : (std::min)(state->large_value.size(),
                                               std::size_t{byte_count});
    if (count != 0) {
        std::memcpy(destination, state->large_value.data(), count);
    }
    if (count < byte_count) {
        std::memset(static_cast<unsigned char*>(destination) + count, 0,
                    byte_count - count);
    }
}
int FSetTmcLargeVal(Tmc tmc, void* source) {
    if (source == nullptr) {
        return false;
    }

    /* Large values are application records; retain one pointer-sized unit when
     * SDM has no generated TM metadata describing a larger record.
     */
    auto& value = control(tmc).large_value;
    value.resize(sizeof(void*));
    std::memcpy(value.data(), source, value.size());
    return true;
}

void SetFocusTmc(Tmc tmc) {
    g_dialog.focus = tmc;
    if (HWND window = ensure_control_window(tmc); window != nullptr) {
        SetFocus(window);
    }
}
Tmc TmcGetFocus() { return g_dialog.focus; }
void SetDefaultTmc(Tmc tmc) { g_dialog.default_tmc = tmc; }
Tmc TmcGetDefault(int) { return g_dialog.default_tmc; }
void SetTmcTxs(Tmc tmc, Dword selection) {
    control(tmc).selection = selection;
}
Dword TxsGetTmc(Tmc tmc) {
    const auto* state = find_control(tmc);
    return state == nullptr ? 0 : state->selection;
}
Word TmvGetTmc(Tmc tmc) {
    const auto* state = find_control(tmc);
    return state != nullptr && !state->text.empty() ? 3 : 1;
}
void RedisplayTmc(Tmc tmc) { populate_original_list(g_dialog, tmc); }

void EnableTmc_sdm21(Tmc tmc, int enabled) {
    auto& state = control(tmc);
    state.enabled = enabled != 0;
}
int FEnabledTmc_sdm21(Tmc tmc) {
    const auto* state = find_control(tmc);
    return state == nullptr || state->enabled;
}
void EnableNoninteractiveTmc(Tmc tmc, int enabled) {
    EnableTmc_sdm21(tmc, enabled);
}
void SetVisibleTmc(Tmc tmc, int visible) {
    auto& state = control(tmc);
    state.visible = visible != 0;
}
int FIsVisibleTmc(Tmc tmc) {
    const auto* state = find_control(tmc);
    return state == nullptr || state->visible;
}
void LimitTextTmc(Tmc tmc, Word limit) {
    auto& state = control(tmc);
    state.text_limit = limit;
    if (state.text.size() > limit) {
        state.text.resize(limit);
    }
}
void CompleteComboTmc(Tmc) {}

void AddListBoxEntry(Tmc tmc, char* entry) {
    auto& state = control(tmc);
    state.entries.emplace_back(counted_or_zero_terminated(entry));
}
void InsertListBoxEntry(Tmc tmc, char* entry, Word index) {
    auto& state = control(tmc);
    auto& entries = state.entries;
    const auto position = (std::min)(entries.size(), std::size_t{index});
    entries.insert(entries.begin() + static_cast<std::ptrdiff_t>(position),
                   counted_or_zero_terminated(entry));
}
void DeleteListBoxEntry(Tmc tmc, Word index) {
    auto& state = control(tmc);
    auto& entries = state.entries;
    if (index < entries.size()) {
        entries.erase(entries.begin() + index);
    }
}
Word CentryListBoxTmc(Tmc tmc) {
    const auto* state = find_control(tmc);
    return static_cast<Word>(state == nullptr
                                 ? 0
                                 : (std::min)(state->entries.size(),
                                              std::size_t{0xffffu}));
}
Word CEntryListBoxTmc(Tmc tmc) { return CentryListBoxTmc(tmc); }
void GetListBoxEntry(Tmc tmc, Word index, char* destination, Word capacity) {
    const auto* state = find_control(tmc);
    copy_text(state == nullptr || index >= state->entries.size()
                  ? std::string{}
                  : state->entries[index],
              destination, capacity);
}
Word CchGetListBoxEntry(Tmc tmc, Word index, char* destination,
                        Word capacity) {
    const auto* state = find_control(tmc);
    const std::string empty;
    const auto& entry = state == nullptr || index >= state->entries.size()
                            ? empty
                            : state->entries[index];
    copy_text(entry, destination, capacity);
    return static_cast<Word>((std::min)(entry.size(), std::size_t{0xffffu}));
}
Word IEntryFindListBox(Tmc tmc, char* entry, Word*) {
    const auto* state = find_control(tmc);
    if (state == nullptr) {
        return static_cast<Word>(-1);
    }
    const std::string sought = counted_or_zero_terminated(entry);
    const auto found = std::find(state->entries.begin(), state->entries.end(),
                                 sought);
    return found == state->entries.end()
               ? static_cast<Word>(-1)
               : static_cast<Word>(found - state->entries.begin());
}
Word CselListBoxTmc(Tmc tmc) {
    const auto* state = find_control(tmc);
    return state == nullptr || state->entries.empty() ? 0 : 1;
}
void StartListBoxUpdate(Tmc) {}
void BeginListBoxUpdate(Tmc, int) {}
void EndListBoxUpdate(Tmc) {}

Hcab HcabFromDlg(int dialog) {
    const auto* state =
        find_dialog(dialog == 0 ? g_current_dialog : static_cast<Hdlg>(dialog));
    return state == nullptr ? nullptr : state->cab;
}
void SaveCabs(void (*callback)(Hcab, Word, Tmc, int), int save) {
    if (callback != nullptr && g_dialog.cab != nullptr) {
        callback(g_dialog.cab, g_dialog.hid, 0, save);
    }
}

void GetTmcRec(Tmc tmc, Rec* rectangle) {
    if (rectangle == nullptr) {
        return;
    }
    const auto* state = find_control(tmc);
    *rectangle = state == nullptr ? Rec{} : state->rectangle;
}

int OpusSdmRenderDialogPreview(unsigned short hid, unsigned int* pixels,
                               int width, int height) {
    return render_dialog_preview(static_cast<Word>(hid), pixels, width,
                                 height);
}

HWND HwndOfTmc(Tmc tmc) { return ensure_control_window(tmc); }
Hdlg HdlgFromHwnd(HWND window) {
    if (window == nullptr) {
        return 0;
    }
    for (const auto& entry : g_dialogs) {
        if (entry.second.window == window ||
            (entry.second.window != nullptr &&
             IsChild(entry.second.window, window))) {
            return entry.first;
        }
    }
    return 0;
}
HWND HwndFromDlg(Hdlg dialog) {
    const auto* state = find_dialog(dialog == 0 ? g_current_dialog : dialog);
    return state == nullptr ? nullptr : state->window;
}
HWND HwndSwapSdmParent(HWND window) {
    return std::exchange(vhWndMsgBoxParent, window);
}
int FIsDlgInteractive() { return !g_noninteractive; }
void SetDlgCaption(char* caption) {
    g_dialog.caption = counted_or_zero_terminated(caption);
    if (g_dialog.window != nullptr && IsWindow(g_dialog.window)) {
        SetWindowTextA(g_dialog.window, g_dialog.caption.c_str());
    }
}

int FSetNoninteractive(Word, Tmc) {
    g_noninteractive = true;
    return true;
}
void EndSdmTranscription() { g_noninteractive = false; }
int FExecutable() { return true; }
void CBTState(int) {}
int FRestoreDlg(int) { return true; }
int FRestoreTmc(Tmc, int) { return true; }

Word IdDoMsgBox(char* text, char* caption, Word flags) {
    return static_cast<Word>(MessageBoxA(vhWndMsgBoxParent, text, caption,
                                         static_cast<UINT>(flags)));
}

Word FlbfFillDirListTmc(char*, char*, Tmc, Tmc, Tmc, Word, Word) {
    return 0x0008;  // flbfListingMade; directory enumeration is UI-backed.
}

}  // extern "C"

namespace {

void set_sds_handle(const Hdlg current, const Hdlg focus) {
    sds.current_dialog =
        reinterpret_cast<void**>(static_cast<std::uintptr_t>(current));
    sds.focus_dialog =
        reinterpret_cast<void**>(static_cast<std::uintptr_t>(focus));
}

}  // namespace
