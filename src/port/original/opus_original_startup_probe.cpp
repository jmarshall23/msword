#include "windows.h"
#include "DbgHelp.h"
#if defined(_MSC_VER)
#include "rtcapi.h"
#endif
#include "opusinputscript.h"
#include "opus_x64_compat.h"
#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <vector>

extern "C" int WINAPI OpusOriginalWinMain(HINSTANCE instance,
                                             HINSTANCE previous,
                                             LPSTR command_line,
                                             int show_command);
using OpusOriginalListProc = unsigned short (*)(
    unsigned short, char*, int, unsigned short, unsigned short,
    unsigned short);
using OpusFontValueProc = int (*)(const char*);
using OpusFontNameFromValueProc = void (*)(int, char*, int);
extern "C" unsigned short WListFontName(unsigned short, char*, int,
                                          unsigned short, unsigned short,
                                          unsigned short);
extern "C" unsigned short WListFontSize(unsigned short, char*, int,
                                          unsigned short, unsigned short,
                                          unsigned short);
extern "C" unsigned short WListStyles(unsigned short, char*, int,
                                        unsigned short, unsigned short,
                                        unsigned short);
extern "C" unsigned short Look1WListEntbl(unsigned short, char*, int,
                                            unsigned short, unsigned short,
                                            unsigned short);
extern "C" int OpusX64FtcFromFontName(const char*);
extern "C" int OpusX64HpsFromFontSize(const char*);
extern "C" void OpusX64FontNameFromFtc(int, char*, int);
extern "C" void OpusRegisterOriginalDialogCallbacks(
    OpusOriginalListProc, OpusOriginalListProc, OpusOriginalListProc,
    OpusOriginalListProc, OpusFontValueProc, OpusFontValueProc,
    OpusFontNameFromValueProc);

namespace {

constexpr UINT kWmOpusX64QuerySelection = WM_APP + 0x351;
constexpr LPARAM kKcControl = 0x100;
constexpr LRESULT kEditUndo = 2229;
constexpr LRESULT kEditCut = 2252;
constexpr LRESULT kEditCopy = 2274;
constexpr LRESULT kEditPaste = 2297;
constexpr LRESULT kEditSelectAll = 5106;
constexpr WPARAM kFileNew = 1813;
constexpr WPARAM kFileSaveAs = 1897;
constexpr WPARAM kHelpAbout = 182;
constexpr WPARAM kExportPdf = 0x7103;
constexpr int kComboFont = 0x7502;
constexpr int kComboSize = 0x7503;

struct ControlSearch {
    int control_id;
    HWND result;
};

bool OpusWideContains(LPCWSTR text, LPCWSTR needle) {
    if (text == nullptr || needle == nullptr || *needle == 0) {
        return false;
    }
    for (; *text != 0; ++text) {
        LPCWSTR cursor = text;
        LPCWSTR wanted = needle;
        while (*cursor != 0 && *wanted != 0 && *cursor == *wanted) {
            ++cursor;
            ++wanted;
        }
        if (*wanted == 0) {
            return true;
        }
    }
    return false;
}

BOOL CALLBACK FindDocumentPaneCallback(HWND window, LPARAM parameter) {
    auto* result = reinterpret_cast<HWND*>(parameter);
    WCHAR class_name[64] = {};
    if (GetClassNameW(window, class_name,
                      static_cast<int>(sizeof(class_name) /
                                       sizeof(class_name[0]))) != 0 &&
        lstrcmpW(class_name, OPUSW("OpusWwd")) == 0) {
        *result = window;
        return FALSE;
    }
    EnumChildWindows(window, FindDocumentPaneCallback, parameter);
    return *result == nullptr;
}

HWND FindDocumentPane() {
    HWND result = nullptr;
    EnumWindows(FindDocumentPaneCallback, reinterpret_cast<LPARAM>(&result));
    return result;
}

BOOL CALLBACK FindControlByIdCallback(HWND window, LPARAM parameter);

BOOL CALLBACK FindControlByIdCallback(HWND window, LPARAM parameter) {
    auto* search = reinterpret_cast<ControlSearch*>(parameter);
    if (IsWindowVisible(window) && GetDlgCtrlID(window) == search->control_id) {
        search->result = window;
        return FALSE;
    }
    EnumChildWindows(window, FindControlByIdCallback, parameter);
    return search->result == nullptr;
}

HWND FindControlById(const int control_id) {
    ControlSearch search{control_id, nullptr};
    EnumWindows(FindControlByIdCallback, reinterpret_cast<LPARAM>(&search));
    return search.result;
}

bool ScriptedTypingDocumentMatched() {
    const HWND pane = FindDocumentPane();
    if (pane == nullptr) {
        return false;
    }
    const LRESULT cp_first = SendMessageW(pane, kWmOpusX64QuerySelection, 0, 0);
    const LRESULT cp_lim = SendMessageW(pane, kWmOpusX64QuerySelection, 1, 0);
    const LRESULT is_insertion =
        SendMessageW(pane, kWmOpusX64QuerySelection, 2, 0);
    const LRESULT cp_mac = SendMessageW(pane, kWmOpusX64QuerySelection, 41, 0);
    return cp_first >= 3 && cp_first == cp_lim && is_insertion == 1 &&
           cp_mac >= cp_first;
}

bool ScriptedClipboardMatched() {
    const HWND pane = FindDocumentPane();
    if (pane == nullptr) {
        return false;
    }
    const struct {
        char key;
        LRESULT command;
    } bindings[] = {
        {'A', kEditSelectAll},
        {'C', kEditCopy},
        {'V', kEditPaste},
        {'X', kEditCut},
        {'Z', kEditUndo},
    };
    for (const auto& binding : bindings) {
        if (SendMessageW(pane, kWmOpusX64QuerySelection, 81,
                         kKcControl | binding.key) != binding.command) {
            return false;
        }
    }
    if (SendMessageW(pane, kWmOpusX64QuerySelection, 80,
                     kKcControl | 'A') == 0) {
        return false;
    }
    const LRESULT cp_first = SendMessageW(pane, kWmOpusX64QuerySelection, 0, 0);
    const LRESULT cp_lim = SendMessageW(pane, kWmOpusX64QuerySelection, 1, 0);
    return cp_lim > cp_first;
}

bool ScriptedUnicodeMatched() {
    const HWND pane = FindDocumentPane();
    if (pane == nullptr) {
        return false;
    }
    const UINT scalars[] = {0x041f, 0x03a9, 0x0645, 0x3053, 0x1f642};
    const LRESULT cp_first = SendMessageW(
        pane, kWmOpusX64QuerySelection, 0, 0);
    for (const UINT scalar : scalars) {
        if (SendMessageW(pane, kWmOpusX64QuerySelection, 107,
                         static_cast<LPARAM>(scalar)) == 0) {
            return false;
        }
    }
    const LRESULT cp_after = SendMessageW(pane, kWmOpusX64QuerySelection, 0, 0);
    if (cp_after != cp_first + static_cast<LRESULT>(
            sizeof(scalars) / sizeof(scalars[0]))) {
        return false;
    }
    for (size_t index = 0; index < sizeof(scalars) / sizeof(scalars[0]);
         ++index) {
        if (SendMessageW(pane, kWmOpusX64QuerySelection, 106,
                         cp_first + static_cast<LPARAM>(index)) !=
            static_cast<LRESULT>(scalars[index])) {
            return false;
        }
    }
    return true;
}

bool ScriptedAboutMatched() {
    const HWND app = FindWindowA("OpusApp", nullptr);
    if (app == nullptr) {
        return false;
    }
    OpusUser32PushScriptedInput(nullptr, WM_COMMAND, 1, 0);
    SendMessageW(app, WM_COMMAND, kHelpAbout, 0);
    return IsWindow(app) && FindWindowA("OpusSdmDialog", nullptr) == nullptr;
}

bool ScriptedSelectionMatched() {
    const HWND pane = FindDocumentPane();
    if (pane == nullptr) {
        return false;
    }
    const char sentence[] = "physical keyboard input line one";
    const size_t length = sizeof(sentence) - 1;
    for (size_t index = 0; index < length; ++index) {
        if (SendMessageW(pane, kWmOpusX64QuerySelection, 107,
                         static_cast<LPARAM>(
                             static_cast<unsigned char>(sentence[index]))) == 0) {
            return false;
        }
    }
    const LRESULT typed_first =
        SendMessageW(pane, kWmOpusX64QuerySelection, 0, 0);
    const LRESULT typed_lim = SendMessageW(pane, kWmOpusX64QuerySelection, 1, 0);
    const LRESULT typed_ins = SendMessageW(pane, kWmOpusX64QuerySelection, 2, 0);
    if (typed_first != static_cast<LRESULT>(length) ||
        typed_lim != typed_first || typed_ins != 1) {
        return false;
    }
    SendMessageW(pane, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(250, 10));
    SendMessageW(pane, WM_LBUTTONUP, 0, MAKELPARAM(250, 10));
    const LRESULT clicked_first =
        SendMessageW(pane, kWmOpusX64QuerySelection, 0, 0);
    const LRESULT clicked_lim =
        SendMessageW(pane, kWmOpusX64QuerySelection, 1, 0);
    const LRESULT clicked_ins =
        SendMessageW(pane, kWmOpusX64QuerySelection, 2, 0);
    const LRESULT clicked_double =
        SendMessageW(pane, kWmOpusX64QuerySelection, 5, 0);
    const LRESULT clicked_sk =
        SendMessageW(pane, kWmOpusX64QuerySelection, 6, 0);
    return clicked_first >= 15 && clicked_first <= typed_first &&
           clicked_lim == clicked_first && clicked_ins == 1 &&
           clicked_double == 0 && clicked_sk == 32;
}

bool ScriptedInteractionMatched() {
    const HWND app = FindWindowA("OpusApp", nullptr);
    if (app == nullptr) {
        return false;
    }
    if (IsZoomed(app)) {
        SendMessageW(app, WM_SYSCOMMAND, SC_RESTORE, 0);
        if (IsZoomed(app)) {
            return false;
        }
    }
    SendMessageW(app, WM_SYSCOMMAND, SC_MAXIMIZE, 0);
    if (!IsZoomed(app)) {
        return false;
    }
    SendMessageW(app, WM_SYSCOMMAND, SC_RESTORE, 0);
    if (IsZoomed(app)) {
        return false;
    }
    OpusUser32PushScriptedInput(nullptr, WM_COMMAND, 2, 0);
    SendMessageW(app, WM_COMMAND, kFileNew, 0);
    return IsWindow(app) && FindWindowA("OpusSdmDialog", nullptr) == nullptr;
}

bool ScriptedSaveAsMatched() {
    const HWND app = FindWindowA("OpusApp", nullptr);
    if (app == nullptr) {
        return false;
    }
    RemovePropW(app, OPUSW("OpusX64SaveAsStage"));
    SendMessageW(app, WM_COMMAND, kFileSaveAs, 0);
    const auto stage =
        reinterpret_cast<INT_PTR>(GetPropW(
            app, OPUSW("OpusX64SaveAsStage")));
    return stage == 2 && IsWindow(app) &&
           FindWindowA("OpusSdmDialog", nullptr) == nullptr &&
           FindWindowA("#32770", nullptr) == nullptr;
}

bool FileStartsWithPdfHeader(const char* path) {
    HANDLE file = CreateFileA(path, GENERIC_READ,
                              FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                              OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        return false;
    }
    char header[5] = {};
    DWORD read = 0;
    const bool matched = ReadFile(file, header, sizeof(header), &read,
                                  nullptr) &&
                         read == sizeof(header) &&
                         std::memcmp(header, "%PDF-", sizeof(header)) == 0;
    CloseHandle(file);
    return matched;
}

bool ScriptedPdfExportMatched() {
    const HWND app = FindWindowA("OpusApp", nullptr);
    const HWND pane = FindDocumentPane();
    if (app == nullptr || pane == nullptr) {
        return false;
    }
    char temporary_directory[MAX_PATH] = {};
    char temporary_seed[MAX_PATH] = {};
    char pdf_path[MAX_PATH + 5] = {};
    if (GetTempPathA(static_cast<DWORD>(sizeof(temporary_directory)),
                     temporary_directory) == 0 ||
        GetTempFileNameA(temporary_directory, "OWP", 0, temporary_seed) == 0) {
        return false;
    }
    DeleteFileA(temporary_seed);
    const int path_length =
        std::snprintf(pdf_path, sizeof(pdf_path), "%s.pdf", temporary_seed);
    if (path_length <= 0 ||
        path_length >= static_cast<int>(sizeof(pdf_path))) {
        return false;
    }
    DeleteFileA(pdf_path);
    if (!SetEnvironmentVariableA("WORD1_TEST_PDF_PATH", pdf_path)) {
        return false;
    }

    const char text[] = "pdf export text";
    const LRESULT cp_mac_before =
        SendMessageW(pane, kWmOpusX64QuerySelection, 41, 0);
    for (size_t index = 0; index < sizeof(text) - 1; ++index) {
        if (SendMessageW(pane, kWmOpusX64QuerySelection, 107,
                         static_cast<LPARAM>(
                             static_cast<unsigned char>(text[index]))) == 0) {
            SetEnvironmentVariableA("WORD1_TEST_PDF_PATH", nullptr);
            DeleteFileA(pdf_path);
            return false;
        }
    }
    const LRESULT cp_mac_after =
        SendMessageW(pane, kWmOpusX64QuerySelection, 41, 0);
    SendMessageW(app, WM_COMMAND, kExportPdf, 0);
    SetEnvironmentVariableA("WORD1_TEST_PDF_PATH", nullptr);
    const LRESULT stage = SendMessageW(pane, kWmOpusX64QuerySelection, 105, 0);
    const bool matched = cp_mac_after > cp_mac_before && stage == 4 &&
                         FileStartsWithPdfHeader(pdf_path);
    DeleteFileA(pdf_path);
    return matched;
}

bool SelectComboText(const HWND combo, LPCWSTR text) {
    const LRESULT index = SendMessageW(
        combo, CB_FINDSTRINGEXACT, static_cast<WPARAM>(-1),
        reinterpret_cast<LPARAM>(text));
    const HWND parent = GetParent(combo);
    const int control_id = GetDlgCtrlID(combo);
    if (index == CB_ERR || parent == nullptr ||
        SendMessageW(combo, CB_SETCURSEL, index, 0) == CB_ERR) {
        return false;
    }
    SendMessageW(parent, WM_COMMAND, MAKEWPARAM(control_id, CBN_SELCHANGE),
                 reinterpret_cast<LPARAM>(combo));
    SendMessageW(parent, WM_COMMAND, MAKEWPARAM(control_id, CBN_SELENDOK),
                 reinterpret_cast<LPARAM>(combo));
    return true;
}

bool InsertText(HWND pane, const char* text) {
    for (; *text != 0; ++text) {
        if (SendMessageW(pane, kWmOpusX64QuerySelection, 107,
                         static_cast<LPARAM>(
                             static_cast<unsigned char>(*text))) == 0) {
            return false;
        }
    }
    return true;
}

bool ScriptedFontTypingMatched() {
    const HWND pane = FindDocumentPane();
    const HWND font_combo = FindControlById(kComboFont);
    const HWND size_combo = FindControlById(kComboSize);
    if (pane == nullptr || font_combo == nullptr || size_combo == nullptr) {
        return false;
    }
    const LRESULT initial_ftc =
        SendMessageW(pane, kWmOpusX64QuerySelection, 49, 0);
    const LRESULT initial_hps =
        SendMessageW(pane, kWmOpusX64QuerySelection, 50, 0);
    if (SendMessageW(pane, kWmOpusX64QuerySelection, 54, 195) != 195 ||
        SendMessageW(pane, kWmOpusX64QuerySelection, 58, 0) != 4) {
        return false;
    }

    if (!SelectComboText(font_combo, OPUSW("Courier New")) ||
        !SelectComboText(size_combo, OPUSW("24"))) {
        return false;
    }
    const LRESULT applied_ftc =
        SendMessageW(pane, kWmOpusX64QuerySelection, 49, 0);
    const LRESULT applied_hps =
        SendMessageW(pane, kWmOpusX64QuerySelection, 50, 0);
    const LRESULT cp_before =
        SendMessageW(pane, kWmOpusX64QuerySelection, 0, 0);
    if (!InsertText(pane, "fonttest")) {
        return false;
    }
    const LRESULT inserted_ftc =
        SendMessageW(pane, kWmOpusX64QuerySelection, 51, cp_before);
    const LRESULT inserted_hps =
        SendMessageW(pane, kWmOpusX64QuerySelection, 52, cp_before);
    const LRESULT first_height =
        SendMessageW(pane, kWmOpusX64QuerySelection, 55, cp_before);
    if (applied_ftc < 0 || applied_ftc == initial_ftc ||
        applied_hps != 48 || applied_hps == initial_hps ||
        inserted_ftc != applied_ftc || inserted_hps != applied_hps ||
        first_height <= 0) {
        return false;
    }

    if (!SelectComboText(font_combo, OPUSW("Arial")) ||
        !SelectComboText(size_combo, OPUSW("36"))) {
        return false;
    }
    const LRESULT second_ftc =
        SendMessageW(pane, kWmOpusX64QuerySelection, 49, 0);
    const LRESULT second_hps =
        SendMessageW(pane, kWmOpusX64QuerySelection, 50, 0);
    const LRESULT second_cp =
        SendMessageW(pane, kWmOpusX64QuerySelection, 0, 0);
    if (!InsertText(pane, " secondfont")) {
        return false;
    }
    const LRESULT second_inserted_ftc =
        SendMessageW(pane, kWmOpusX64QuerySelection, 51, second_cp);
    const LRESULT second_inserted_hps =
        SendMessageW(pane, kWmOpusX64QuerySelection, 52, second_cp);
    const LRESULT first_hps_after_second =
        SendMessageW(pane, kWmOpusX64QuerySelection, 52, cp_before);
    const LRESULT second_height =
        SendMessageW(pane, kWmOpusX64QuerySelection, 55, second_cp);
    const LRESULT formatted_chp_hps =
        SendMessageW(pane, kWmOpusX64QuerySelection, 59, second_cp);
    const LRESULT formatted_fcid_hps =
        SendMessageW(pane, kWmOpusX64QuerySelection, 60, second_cp);
    if (second_ftc < 0 || second_ftc == applied_ftc || second_hps != 72 ||
        second_inserted_ftc != second_ftc ||
        second_inserted_hps != second_hps ||
        first_hps_after_second != applied_hps ||
        second_height <= first_height ||
        formatted_chp_hps != 72 || formatted_fcid_hps != 72) {
        return false;
    }

    if (!InsertText(pane, "\r") ||
        !SelectComboText(size_combo, OPUSW("72"))) {
        return false;
    }
    const LRESULT large_cp =
        SendMessageW(pane, kWmOpusX64QuerySelection, 0, 0);
    if (!InsertText(pane, "largeline")) {
        return false;
    }
    const LRESULT large_inserted_ftc =
        SendMessageW(pane, kWmOpusX64QuerySelection, 51, large_cp);
    const LRESULT large_inserted_hps =
        SendMessageW(pane, kWmOpusX64QuerySelection, 52, large_cp);
    const LRESULT large_height =
        SendMessageW(pane, kWmOpusX64QuerySelection, 55, large_cp);
    const LRESULT display_lines =
        SendMessageW(pane, kWmOpusX64QuerySelection, 30, 0);
    const LRESULT cp_mac =
        SendMessageW(pane, kWmOpusX64QuerySelection, 41, 0);
    bool fetch_bytes_match = true;
    for (LRESULT cp = 0; cp < cp_mac; ++cp) {
        const LRESULT raw = SendMessageW(
            pane, kWmOpusX64QuerySelection, 69, cp);
        const LRESULT formatted = SendMessageW(
            pane, kWmOpusX64QuerySelection, 70, cp);
        if (raw != formatted) {
            fetch_bytes_match = false;
            break;
        }
    }
    const LRESULT cache_pages =
        SendMessageW(pane, kWmOpusX64QuerySelection, 71, 0);
    if (large_inserted_ftc != second_ftc || large_inserted_hps != 144 ||
        large_height <= second_height || display_lines < 2 ||
        !fetch_bytes_match || LOWORD(cache_pages) == HIWORD(cache_pages)) {
        return false;
    }

    if (SendMessageW(pane, kWmOpusX64QuerySelection, 80,
                     kKcControl | 'A') == 0 ||
        !SelectComboText(size_combo, OPUSW("48"))) {
        return false;
    }
    const LRESULT selected_first =
        SendMessageW(pane, kWmOpusX64QuerySelection, 0, 0);
    const LRESULT selected_lim =
        SendMessageW(pane, kWmOpusX64QuerySelection, 1, 0);
    const LRESULT selected_first_hps =
        SendMessageW(pane, kWmOpusX64QuerySelection, 52, cp_before);
    const LRESULT selected_second_hps =
        SendMessageW(pane, kWmOpusX64QuerySelection, 52, second_cp);
    const LRESULT selected_height =
        SendMessageW(pane, kWmOpusX64QuerySelection, 55, cp_before);
    return selected_lim > selected_first && selected_first_hps == 96 &&
           selected_second_hps == 96 && selected_height > second_height &&
           SelectComboText(size_combo, OPUSW("72")) &&
           SendMessageW(pane, kWmOpusX64QuerySelection, 52,
                        cp_before) == 144 &&
           SendMessageW(pane, kWmOpusX64QuerySelection, 52,
                        second_cp) == 144;
}

void WriteCrashText(HANDLE file, const char* text) {
    DWORD written = 0;
    WriteFile(file, text, static_cast<DWORD>(std::strlen(text)), &written,
              nullptr);
}

void NarrowDiagnosticWide(LPCWSTR source, char* destination,
                          size_t destination_size) {
    if (destination_size == 0) {
        return;
    }
    if (source == nullptr) {
        source = OPUSW("");
    }
    size_t index = 0;
    while (*source != 0 && index + 1 < destination_size) {
        const WCHAR character = *source++;
        destination[index++] =
            character >= 0x20 && character <= 0x7e ?
                static_cast<char>(character) : '?';
    }
    destination[index] = '\0';
}

void BuildDiagnosticPath(const char* file_name, char* path, size_t path_size) {
    char module_path[MAX_PATH] = {};
    GetModuleFileNameA(nullptr, module_path, MAX_PATH);
    char* file_part = std::strrchr(module_path, '\\');
    if (file_part != nullptr) {
        *file_part = '\0';
        char* bin_part = std::strrchr(module_path, '\\');
        if (bin_part != nullptr) {
            *bin_part = '\0';
        }
    }
    std::snprintf(path, path_size, "%s\\build\\%s", module_path,
                  file_name);
}

void ResetRibbonTrace() {
    char trace_path[MAX_PATH] = {};
    BuildDiagnosticPath("WORD1-ribbon.txt", trace_path, sizeof(trace_path));
    HANDLE file = CreateFileA(trace_path, GENERIC_WRITE,
                              FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                              CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        return;
    }
    char header[128] = {};
    std::snprintf(header, sizeof(header), "WORD1 ribbon trace pid=%lu\r\n",
                  GetCurrentProcessId());
    WriteCrashText(file, header);
    CloseHandle(file);
}

void WriteCurrentStack(HANDLE file, unsigned frames_to_skip) {
    void* frames[64] = {};
    const USHORT frame_count =
        CaptureStackBackTrace(frames_to_skip, 64, frames, nullptr);
    const auto module_base = reinterpret_cast<std::uintptr_t>(
        GetModuleHandleW(nullptr));
    char raw_line[160] = {};
    for (USHORT index = 0; index < frame_count; ++index) {
        const auto address = reinterpret_cast<std::uintptr_t>(frames[index]);
        if (address >= module_base) {
            std::snprintf(raw_line, sizeof(raw_line),
                          "raw #%u 0x%016llX WORD1+0x%llX\r\n", index,
                          static_cast<unsigned long long>(address),
                          static_cast<unsigned long long>(address - module_base));
        } else {
            std::snprintf(raw_line, sizeof(raw_line),
                          "raw #%u 0x%016llX\r\n", index,
                          static_cast<unsigned long long>(address));
        }
        WriteCrashText(file, raw_line);
    }
    HANDLE process = GetCurrentProcess();
    SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS);
    if (!SymInitialize(process, nullptr, TRUE)) {
        return;
    }

    char line_text[1024] = {};
    for (USHORT index = 0; index < frame_count; ++index) {
        const DWORD64 address =
            static_cast<DWORD64>(reinterpret_cast<std::uintptr_t>(frames[index]));
        char symbol_storage[sizeof(SYMBOL_INFO) + MAX_SYM_NAME] = {};
        auto* symbol = reinterpret_cast<SYMBOL_INFO*>(symbol_storage);
        symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
        symbol->MaxNameLen = MAX_SYM_NAME;
        DWORD64 displacement = 0;
        const BOOL has_symbol =
            SymFromAddr(process, address, &displacement, symbol);

        IMAGEHLP_LINE64 source_line = {};
        source_line.SizeOfStruct = sizeof(source_line);
        DWORD line_displacement = 0;
        const BOOL has_line = SymGetLineFromAddr64(
            process, address, &line_displacement, &source_line);
        if (has_symbol && has_line) {
            std::snprintf(line_text, sizeof(line_text),
                          "#%u 0x%016llX %s+0x%llX (%s:%lu)\r\n",
                          index, static_cast<unsigned long long>(address),
                          symbol->Name,
                          static_cast<unsigned long long>(displacement),
                          source_line.FileName, source_line.LineNumber);
        } else if (has_symbol) {
            std::snprintf(line_text, sizeof(line_text),
                          "#%u 0x%016llX %s+0x%llX\r\n", index,
                          static_cast<unsigned long long>(address), symbol->Name,
                          static_cast<unsigned long long>(displacement));
        } else {
            std::snprintf(line_text, sizeof(line_text),
                          "#%u 0x%016llX\r\n", index,
                          static_cast<unsigned long long>(address));
        }
        WriteCrashText(file, line_text);
    }
    SymCleanup(process);
}

int __cdecl WriteRtcFailure(int error_type, LPCWSTR file_name, int line,
                            LPCWSTR module_name, LPCWSTR format, ...) {
    char diagnostic_path[MAX_PATH] = {};
    BuildDiagnosticPath("WORD1-rtc.txt", diagnostic_path,
                        sizeof(diagnostic_path));
    HANDLE file = CreateFileA(diagnostic_path, GENERIC_WRITE, FILE_SHARE_READ,
                              nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL,
                              nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        return 0;
    }

    WCHAR message[2048] = {};
    va_list arguments;
    va_start(arguments, format);
#if defined(_MSC_VER)
    _vsnwprintf_s(message, _countof(message), _TRUNCATE, format, arguments);
#else
    size_t message_index = 0;
    while (format != nullptr && format[message_index] != 0 &&
           message_index + 1 < _countof(message)) {
        message[message_index] = format[message_index];
        ++message_index;
    }
    message[message_index] = 0;
#endif
    va_end(arguments);

    char file_name_ansi[MAX_PATH] = {};
    char module_name_ansi[MAX_PATH] = {};
    char message_ansi[4096] = {};
    NarrowDiagnosticWide(file_name, file_name_ansi, sizeof(file_name_ansi));
    NarrowDiagnosticWide(module_name, module_name_ansi,
                         sizeof(module_name_ansi));
    NarrowDiagnosticWide(message, message_ansi, sizeof(message_ansi));

    char header[4096] = {};
    std::snprintf(header, sizeof(header),
                  "RTC failure %d at %s:%d (%s)\r\n%s\r\n", error_type,
                  file_name_ansi, line, module_name_ansi, message_ansi);
    WriteCrashText(file, header);
    WriteCurrentStack(file, 1);
    CloseHandle(file);
    return 0;
}

LONG WINAPI WriteCrashStack(EXCEPTION_POINTERS* exception) {
    char crash_path[MAX_PATH] = {};
    BuildDiagnosticPath("WORD1-crash.txt", crash_path, sizeof(crash_path));

    HANDLE file = CreateFileA(crash_path, GENERIC_WRITE, FILE_SHARE_READ,
                              nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL,
                              nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        return EXCEPTION_EXECUTE_HANDLER;
    }

    char line_text[1024] = {};
    std::snprintf(line_text, sizeof(line_text),
                  "Exception 0x%08lX at 0x%016llX\r\n",
                  exception->ExceptionRecord->ExceptionCode,
                  static_cast<unsigned long long>(
                      reinterpret_cast<std::uintptr_t>(
                          exception->ExceptionRecord->ExceptionAddress)));
    WriteCrashText(file, line_text);

    if (exception->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION &&
        exception->ExceptionRecord->NumberParameters >= 2) {
        const ULONG_PTR operation = exception->ExceptionRecord->ExceptionInformation[0];
        const ULONG_PTR address = exception->ExceptionRecord->ExceptionInformation[1];
        const char* operation_name = operation == 0 ? "read" :
                                     operation == 1 ? "write" :
                                     operation == 8 ? "execute" : "unknown";
        std::snprintf(line_text, sizeof(line_text),
                      "Access violation: %s at 0x%016llX\r\n",
                      operation_name,
                      static_cast<unsigned long long>(address));
        WriteCrashText(file, line_text);
    }

#if defined(_M_X64)
    const CONTEXT* fault_context = exception->ContextRecord;
    std::snprintf(
        line_text, sizeof(line_text),
        "RAX=%016llX RBX=%016llX RCX=%016llX RDX=%016llX\r\n"
        "RSI=%016llX RDI=%016llX RBP=%016llX RSP=%016llX\r\n"
        "R8 =%016llX R9 =%016llX R10=%016llX R11=%016llX\r\n",
        static_cast<unsigned long long>(fault_context->Rax),
        static_cast<unsigned long long>(fault_context->Rbx),
        static_cast<unsigned long long>(fault_context->Rcx),
        static_cast<unsigned long long>(fault_context->Rdx),
        static_cast<unsigned long long>(fault_context->Rsi),
        static_cast<unsigned long long>(fault_context->Rdi),
        static_cast<unsigned long long>(fault_context->Rbp),
        static_cast<unsigned long long>(fault_context->Rsp),
        static_cast<unsigned long long>(fault_context->R8),
        static_cast<unsigned long long>(fault_context->R9),
        static_cast<unsigned long long>(fault_context->R10),
        static_cast<unsigned long long>(fault_context->R11));
    WriteCrashText(file, line_text);
#endif

    const auto module_base = reinterpret_cast<std::uintptr_t>(
        GetModuleHandleW(nullptr));
    const auto* dos_header = reinterpret_cast<const IMAGE_DOS_HEADER*>(
        module_base);
    const auto* nt_headers = reinterpret_cast<const IMAGE_NT_HEADERS64*>(
        module_base + static_cast<std::uintptr_t>(dos_header->e_lfanew));
    const std::uintptr_t module_limit =
        module_base + nt_headers->OptionalHeader.SizeOfImage;
    ULONG_PTR stack_low = 0;
    ULONG_PTR stack_high = 0;
    GetCurrentThreadStackLimits(&stack_low, &stack_high);

    CONTEXT unwind_context = *exception->ContextRecord;
    for (unsigned index = 0; index < 64 && unwind_context.Rip != 0; ++index) {
        const DWORD64 control_pc = unwind_context.Rip;
        if (control_pc >= module_base && control_pc < module_limit) {
            std::snprintf(line_text, sizeof(line_text),
                          "unwind #%u WORD1+0x%llX rsp=0x%llX\r\n", index,
                          static_cast<unsigned long long>(control_pc - module_base),
                          static_cast<unsigned long long>(unwind_context.Rsp));
        } else {
            std::snprintf(line_text, sizeof(line_text),
                          "unwind #%u 0x%016llX rsp=0x%llX\r\n", index,
                          static_cast<unsigned long long>(control_pc),
                          static_cast<unsigned long long>(unwind_context.Rsp));
        }
        WriteCrashText(file, line_text);

        DWORD64 image_base = 0;
        PRUNTIME_FUNCTION runtime_function =
            RtlLookupFunctionEntry(control_pc, &image_base, nullptr);
        if (runtime_function != nullptr) {
            PVOID handler_data = nullptr;
            DWORD64 establisher_frame = 0;
            RtlVirtualUnwind(0, image_base, control_pc, runtime_function,
                             &unwind_context, &handler_data,
                             &establisher_frame, nullptr);
        } else {
            if (unwind_context.Rsp < stack_low ||
                unwind_context.Rsp + sizeof(DWORD64) > stack_high) {
                break;
            }
            unwind_context.Rip =
                *reinterpret_cast<const DWORD64*>(unwind_context.Rsp);
            unwind_context.Rsp += sizeof(DWORD64);
        }
    }

    const std::uintptr_t stack_begin = exception->ContextRecord->Rsp;
    const std::uintptr_t stack_end = (std::min)(
        static_cast<std::uintptr_t>(stack_high), stack_begin + 8192u);
    for (std::uintptr_t cursor = stack_begin;
         cursor + sizeof(std::uintptr_t) <= stack_end;
         cursor += sizeof(std::uintptr_t)) {
        const auto candidate =
            *reinterpret_cast<const std::uintptr_t*>(cursor);
        if (candidate >= module_base && candidate < module_limit) {
            std::snprintf(line_text, sizeof(line_text),
                          "stack+0x%llX: WORD1+0x%llX\r\n",
                          static_cast<unsigned long long>(cursor - stack_begin),
                          static_cast<unsigned long long>(candidate - module_base));
            WriteCrashText(file, line_text);
        }
    }

#if defined(_WIN32)
    HANDLE process = GetCurrentProcess();
    SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS);
    if (SymInitialize(process, nullptr, TRUE)) {
        CONTEXT context = *exception->ContextRecord;
        STACKFRAME64 frame = {};
        frame.AddrPC.Offset = context.Rip;
        frame.AddrPC.Mode = AddrModeFlat;
        frame.AddrStack.Offset = context.Rsp;
        frame.AddrStack.Mode = AddrModeFlat;
        frame.AddrFrame.Offset = context.Rbp;
        frame.AddrFrame.Mode = AddrModeFlat;

        for (unsigned index = 0; index < 64 && frame.AddrPC.Offset != 0;
             ++index) {
            char symbol_storage[sizeof(SYMBOL_INFO) + MAX_SYM_NAME] = {};
            auto* symbol = reinterpret_cast<SYMBOL_INFO*>(symbol_storage);
            symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
            symbol->MaxNameLen = MAX_SYM_NAME;
            DWORD64 displacement = 0;
            const BOOL has_symbol =
                SymFromAddr(process, frame.AddrPC.Offset, &displacement, symbol);

            IMAGEHLP_LINE64 source_line = {};
            source_line.SizeOfStruct = sizeof(source_line);
            DWORD line_displacement = 0;
            const BOOL has_line = SymGetLineFromAddr64(
                process, frame.AddrPC.Offset, &line_displacement, &source_line);

            if (has_symbol && has_line) {
                std::snprintf(line_text, sizeof(line_text),
                              "#%u 0x%016llX %s+0x%llX (%s:%lu)\r\n",
                              index,
                              static_cast<unsigned long long>(frame.AddrPC.Offset),
                              symbol->Name,
                              static_cast<unsigned long long>(displacement),
                              source_line.FileName, source_line.LineNumber);
            } else if (has_symbol) {
                std::snprintf(line_text, sizeof(line_text),
                              "#%u 0x%016llX %s+0x%llX\r\n", index,
                              static_cast<unsigned long long>(frame.AddrPC.Offset),
                              symbol->Name,
                              static_cast<unsigned long long>(displacement));
            } else {
                std::snprintf(line_text, sizeof(line_text),
                              "#%u 0x%016llX\r\n", index,
                              static_cast<unsigned long long>(frame.AddrPC.Offset));
            }
            WriteCrashText(file, line_text);

            if (!StackWalk64(IMAGE_FILE_MACHINE_AMD64, process,
                             GetCurrentThread(), &frame, &context, nullptr,
                             SymFunctionTableAccess64, SymGetModuleBase64,
                             nullptr)) {
                break;
            }
        }
        SymCleanup(process);
    }
#endif

    CloseHandle(file);
    return EXCEPTION_EXECUTE_HANDLER;
}

LONG WINAPI ObserveVectoredException(EXCEPTION_POINTERS* exception) {
    static volatile LONG writing_exception = 0;
    if (exception != nullptr && exception->ExceptionRecord != nullptr &&
        (exception->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION ||
         exception->ExceptionRecord->ExceptionCode == EXCEPTION_INT_DIVIDE_BY_ZERO ||
         exception->ExceptionRecord->ExceptionCode == 0xC0000374u ||
         exception->ExceptionRecord->ExceptionCode == 0xC0000409u ||
         exception->ExceptionRecord->ExceptionCode == 0xE0421001u) &&
        InterlockedCompareExchange(&writing_exception, 1, 0) == 0) {
        WriteCrashStack(exception);
        InterlockedExchange(&writing_exception, 0);
    }
    return EXCEPTION_CONTINUE_SEARCH;
}

}  // namespace

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE previous,
                    PWSTR command_line, int show_command) {
    const LPCWSTR self_test = OPUSW("--self-test");
    const LPCWSTR scripted_key_test = OPUSW("--scripted-key-test");
    const LPCWSTR scripted_typing_test = OPUSW("--scripted-typing-test");
    const LPCWSTR scripted_clipboard_test = OPUSW("--scripted-clipboard-test");
    const LPCWSTR scripted_unicode_test = OPUSW("--scripted-unicode-test");
    const LPCWSTR scripted_about_test = OPUSW("--scripted-about-test");
    const LPCWSTR scripted_selection_test = OPUSW("--scripted-selection-test");
    const LPCWSTR scripted_interaction_test = OPUSW("--scripted-interaction-test");
    const LPCWSTR scripted_save_as_test = OPUSW("--scripted-save-as-test");
    const LPCWSTR scripted_pdf_export_test =
        OPUSW("--scripted-pdf-export-test");
    const LPCWSTR scripted_font_typing_test =
        OPUSW("--scripted-font-typing-test");
    if (OpusWideContains(command_line, self_test) ||
        OpusWideContains(GetCommandLineW(), self_test)) {
        return 0;
    }
    const bool run_scripted_key_test =
        OpusWideContains(command_line, scripted_key_test) ||
        OpusWideContains(GetCommandLineW(), scripted_key_test);
    const bool run_scripted_typing_test =
        OpusWideContains(command_line, scripted_typing_test) ||
        OpusWideContains(GetCommandLineW(), scripted_typing_test);
    const bool run_scripted_clipboard_test =
        OpusWideContains(command_line, scripted_clipboard_test) ||
        OpusWideContains(GetCommandLineW(), scripted_clipboard_test);
    const bool run_scripted_unicode_test =
        OpusWideContains(command_line, scripted_unicode_test) ||
        OpusWideContains(GetCommandLineW(), scripted_unicode_test);
    const bool run_scripted_about_test =
        OpusWideContains(command_line, scripted_about_test) ||
        OpusWideContains(GetCommandLineW(), scripted_about_test);
    const bool run_scripted_selection_test =
        OpusWideContains(command_line, scripted_selection_test) ||
        OpusWideContains(GetCommandLineW(), scripted_selection_test);
    const bool run_scripted_interaction_test =
        OpusWideContains(command_line, scripted_interaction_test) ||
        OpusWideContains(GetCommandLineW(), scripted_interaction_test);
    const bool run_scripted_save_as_test =
        OpusWideContains(command_line, scripted_save_as_test) ||
        OpusWideContains(GetCommandLineW(), scripted_save_as_test);
    const bool run_scripted_pdf_export_test =
        OpusWideContains(command_line, scripted_pdf_export_test) ||
        OpusWideContains(GetCommandLineW(), scripted_pdf_export_test);
    const bool run_scripted_font_typing_test =
        OpusWideContains(command_line, scripted_font_typing_test) ||
        OpusWideContains(GetCommandLineW(), scripted_font_typing_test);

    /* Exclude the current directory and PATH from DLL resolution. The app
     * directory remains available for intentionally deployed components and
     * system DLLs are resolved only from System32.
     */
    SetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_APPLICATION_DIR |
                             LOAD_LIBRARY_SEARCH_SYSTEM32 |
                             LOAD_LIBRARY_SEARCH_USER_DIRS);
    SetSearchPathMode(BASE_SEARCH_PATH_ENABLE_SAFE_SEARCHMODE |
                      BASE_SEARCH_PATH_PERMANENT);

    SetUnhandledExceptionFilter(WriteCrashStack);
    AddVectoredExceptionHandler(1, ObserveVectoredException);
#if defined(_MSC_VER)
    _RTC_SetErrorFuncW(WriteRtcFailure);
#endif
    ResetRibbonTrace();

    /* The native SDM shim owns the controls, while Microsoft's original
     * callbacks still own every font, point-size, color, and style list.
     */
    OpusRegisterOriginalDialogCallbacks(WListFontName, WListFontSize,
                                        WListStyles, Look1WListEntbl,
                                        OpusX64FtcFromFontName,
                                        OpusX64HpsFromFontSize,
                                        OpusX64FontNameFromFtc);

    char command_line_ansi[32768] = {};
    if (command_line != nullptr) {
        WideCharToMultiByte(CP_ACP, 0, command_line, -1, command_line_ansi,
                            static_cast<int>(sizeof(command_line_ansi)),
                            nullptr, nullptr);
    }
    if (run_scripted_key_test) {
        OpusUser32ExpectScriptedChar('a', 2);
        OpusUser32PushScriptedInput(nullptr, WM_KEYDOWN, 'A', 1);
        OpusUser32PushScriptedInput(nullptr, WM_KEYUP, 'A', 2);
        OpusUser32PushScriptedInput(nullptr, WM_KEYDOWN, 'A', 3);
        OpusUser32PushScriptedInput(nullptr, WM_KEYUP, 'A', 4);
        OpusUser32PushScriptedInput(nullptr, WM_QUIT, 0, 0);
    } else if (run_scripted_typing_test || run_scripted_clipboard_test) {
        OpusUser32ExpectScriptedChar('a', 3);
        OpusUser32PushScriptedInput(nullptr, WM_KEYDOWN, 'A', 1);
        OpusUser32PushScriptedInput(nullptr, WM_KEYUP, 'A', 2);
        OpusUser32PushScriptedInput(nullptr, WM_KEYDOWN, 'A', 3);
        OpusUser32PushScriptedInput(nullptr, WM_KEYUP, 'A', 4);
        OpusUser32PushScriptedInput(nullptr, WM_KEYDOWN, 'A', 5);
        OpusUser32PushScriptedInput(nullptr, WM_KEYUP, 'A', 6);
        OpusUser32PushScriptedInput(nullptr, WM_QUIT, 0, 0);
    } else if (run_scripted_unicode_test || run_scripted_about_test ||
               run_scripted_selection_test || run_scripted_interaction_test ||
               run_scripted_save_as_test || run_scripted_pdf_export_test ||
               run_scripted_font_typing_test) {
        OpusUser32PushScriptedInput(nullptr, WM_QUIT, 0, 0);
    }
    const int result = OpusOriginalWinMain(instance, previous, command_line_ansi,
                                          show_command);
    if ((run_scripted_key_test || run_scripted_typing_test ||
         run_scripted_clipboard_test) &&
        !OpusUser32ScriptedCharMatched()) {
        return 2;
    }
    if ((run_scripted_typing_test || run_scripted_clipboard_test) &&
        !ScriptedTypingDocumentMatched()) {
        return 3;
    }
    if (run_scripted_clipboard_test && !ScriptedClipboardMatched()) return 4;
    if (run_scripted_unicode_test && !ScriptedUnicodeMatched()) return 5;
    if (run_scripted_about_test && !ScriptedAboutMatched()) return 6;
    if (run_scripted_selection_test && !ScriptedSelectionMatched()) return 7;
    if (run_scripted_interaction_test && !ScriptedInteractionMatched()) return 8;
    if (run_scripted_save_as_test && !ScriptedSaveAsMatched()) return 9;
    if (run_scripted_pdf_export_test && !ScriptedPdfExportMatched()) return 10;
    if (run_scripted_font_typing_test && !ScriptedFontTypingMatched()) {
        return 11;
    }
    return result;
}

#ifndef _WIN32
int main(const int argument_count, char** arguments) {
    std::vector<WCHAR> command_line;
    for (int argument = 1; argument < argument_count; ++argument) {
        if (argument > 1) command_line.push_back(OPUSW(" ")[0]);
        if (arguments[argument] == nullptr) continue;
        for (const unsigned char* text =
                 reinterpret_cast<const unsigned char*>(arguments[argument]);
             *text != 0; ++text) {
            command_line.push_back(static_cast<WCHAR>(*text));
        }
    }
    command_line.push_back(0);
    return wWinMain(nullptr, nullptr, command_line.data(), 0);
}
#endif
