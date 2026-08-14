#include "windows.h"
#include "DbgHelp.h"
#if defined(_MSC_VER)
#include "rtcapi.h"
#endif
#include "../win32/opusinputscript.h"
#include "opus-native-compat.h"
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef OPUSW
#define OPUSW(text) u##text
#endif

int WINAPI OpusOriginalWinMain(HINSTANCE instance, HINSTANCE previous,
                               LPSTR command_line, int show_command);
typedef unsigned short (*OpusOriginalListProc)(
    unsigned short, char *, int, unsigned short, unsigned short,
    unsigned short);
typedef int (*OpusFontValueProc)(const char *);
typedef void (*OpusFontNameFromValueProc)(int, char *, int);
unsigned short WListFontName(unsigned short, char *, int, unsigned short,
                             unsigned short, unsigned short);
unsigned short WListFontSize(unsigned short, char *, int, unsigned short,
                             unsigned short, unsigned short);
unsigned short WListStyles(unsigned short, char *, int, unsigned short,
                           unsigned short, unsigned short);
unsigned short Look1WListEntbl(unsigned short, char *, int, unsigned short,
                               unsigned short, unsigned short);
int OpusX64FtcFromFontName(const char *);
int OpusX64HpsFromFontSize(const char *);
void OpusX64FontNameFromFtc(int, char *, int);
void OpusRegisterOriginalDialogCallbacks(
    OpusOriginalListProc, OpusOriginalListProc, OpusOriginalListProc,
    OpusOriginalListProc, OpusFontValueProc, OpusFontValueProc,
    OpusFontNameFromValueProc);

enum {
    kWmOpusX64QuerySelection = WM_APP + 0x351,
    kKcControl = 0x100,
    kEditUndo = 2229,
    kEditCut = 2252,
    kEditCopy = 2274,
    kEditPaste = 2297,
    kEditSelectAll = 5106,
    kFileNew = 1813,
    kFileSaveAs = 1897,
    kFileExit = 2095,
    kHelpAbout = 182,
    kExportPdf = 0x7103,
    kComboFont = 0x7502,
    kComboSize = 0x7503
};

static char g_scripted_save_as_output[MAX_PATH];
static char g_scripted_save_as_doc_path[MAX_PATH + 5];
static bool g_scripted_save_as_keep_output;
static ULONGLONG g_scripted_save_as_started;
static unsigned g_scripted_save_as_attempts;
static bool g_scripted_save_as_text_inserted;
static bool g_scripted_save_as_attempted;
static bool g_scripted_save_as_running;
static UINT_PTR g_scripted_save_as_timer;
static BOOL g_last_scripted_save_has_app;
static BOOL g_last_scripted_save_has_pane;
static INT_PTR g_last_scripted_save_stage;
static BOOL g_last_scripted_save_app_alive;
static BOOL g_last_scripted_save_opus_dialog_open;
static BOOL g_last_scripted_save_win_dialog_open;
static BOOL g_last_scripted_save_header;
static DWORD g_last_scripted_save_error;
static BOOL g_last_scripted_save_file_open;
static DWORD g_last_scripted_save_file_error;
static DWORD g_last_scripted_save_file_size;
static DWORD g_last_scripted_save_file_read;
static uint32_t g_last_scripted_save_header0;
static uint32_t g_last_scripted_save_header4;
static uint32_t g_last_scripted_save_header24;
static uint32_t g_last_scripted_save_header48;

typedef struct ControlSearch {
    int control_id;
    HWND result;
} ControlSearch;

static bool OpusWideContains(LPCWSTR text, LPCWSTR needle) {
    if (text == NULL || needle == NULL || *needle == 0) {
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

static LPCWSTR OpusWideFind(LPCWSTR text, LPCWSTR needle) {
    if (text == NULL || needle == NULL || *needle == 0) {
        return NULL;
    }
    for (; *text != 0; ++text) {
        LPCWSTR cursor = text;
        LPCWSTR wanted = needle;
        while (*cursor != 0 && *wanted != 0 && *cursor == *wanted) {
            ++cursor;
            ++wanted;
        }
        if (*wanted == 0) {
            return text;
        }
    }
    return NULL;
}

static bool OpusWideReadArgumentValue(LPCWSTR text, LPCWSTR name, char* value,
                                      DWORD value_size) {
    LPCWSTR found = OpusWideFind(text, name);
    DWORD offset = 0;
    DWORD name_length = 0;
    if (found == NULL || value == NULL || value_size == 0) {
        return false;
    }
    while (name[name_length] != 0) {
        ++name_length;
    }
    found += name_length;
    while (*found != 0 && *found != OPUSW(" ")[0] && offset + 1 < value_size) {
        value[offset++] = (char)(*found & 0xff);
        ++found;
    }
    value[offset] = '\0';
    return offset != 0;
}

static BOOL CALLBACK FindDocumentPaneCallback(HWND window, LPARAM parameter) {
    HWND *result = (HWND *)parameter;
    WCHAR class_name[64] = {0};
    if (GetClassNameW(window, class_name,
                      (int)(sizeof(class_name) / sizeof(class_name[0]))) != 0 &&
        lstrcmpW(class_name, OPUSW("OpusWwd")) == 0) {
        *result = window;
        return FALSE;
    }
    EnumChildWindows(window, FindDocumentPaneCallback, parameter);
    return *result == NULL;
}

static HWND FindDocumentPane(void) {
    HWND result = NULL;
    EnumWindows(FindDocumentPaneCallback, (LPARAM)&result);
    return result;
}

static BOOL CALLBACK FindControlByIdCallback(HWND window, LPARAM parameter);

static BOOL CALLBACK FindControlByIdCallback(HWND window, LPARAM parameter) {
    ControlSearch *search = (ControlSearch *)parameter;
    if (IsWindowVisible(window) && GetDlgCtrlID(window) == search->control_id) {
        search->result = window;
        return FALSE;
    }
    EnumChildWindows(window, FindControlByIdCallback, parameter);
    return search->result == NULL;
}

static HWND FindControlById(const int control_id) {
    ControlSearch search = {control_id, NULL};
    EnumWindows(FindControlByIdCallback, (LPARAM)&search);
    return search.result;
}

static bool ScriptedTypingDocumentMatched(void) {
    const HWND pane = FindDocumentPane();
    if (pane == NULL) {
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

static bool ScriptedClipboardMatched(void) {
    const HWND pane = FindDocumentPane();
    if (pane == NULL) {
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
    for (size_t index = 0; index < sizeof(bindings) / sizeof(bindings[0]);
         ++index) {
        const char key = bindings[index].key;
        const LRESULT command = bindings[index].command;
        if (SendMessageW(pane, kWmOpusX64QuerySelection, 81,
                         kKcControl | key) != command) {
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

static bool ScriptedUnicodeMatched(void) {
    const HWND pane = FindDocumentPane();
    if (pane == NULL) {
        return false;
    }
    const UINT scalars[] = {0x041f, 0x03a9, 0x0645, 0x3053, 0x1f642};
    const LRESULT cp_first = SendMessageW(
        pane, kWmOpusX64QuerySelection, 0, 0);
    for (size_t index = 0; index < sizeof(scalars) / sizeof(scalars[0]);
         ++index) {
        if (SendMessageW(pane, kWmOpusX64QuerySelection, 107,
                         (LPARAM)scalars[index]) == 0) {
            return false;
        }
    }
    const LRESULT cp_after = SendMessageW(pane, kWmOpusX64QuerySelection, 0, 0);
    if (cp_after != cp_first +
                        (LRESULT)(sizeof(scalars) / sizeof(scalars[0]))) {
        return false;
    }
    for (size_t index = 0; index < sizeof(scalars) / sizeof(scalars[0]);
         ++index) {
        if (SendMessageW(pane, kWmOpusX64QuerySelection, 106,
                         cp_first + (LPARAM)index) != (LRESULT)scalars[index]) {
            return false;
        }
    }
    return true;
}

static bool ScriptedAboutMatched(void) {
    const HWND app = FindWindowA("OpusApp", NULL);
    if (app == NULL) {
        return false;
    }
    OpusUser32PushScriptedInput(NULL, WM_COMMAND, 1, 0);
    SendMessageW(app, WM_COMMAND, kHelpAbout, 0);
    return IsWindow(app) && FindWindowA("OpusSdmDialog", NULL) == NULL;
}

static bool ScriptedSelectionMatched(void) {
    const HWND pane = FindDocumentPane();
    if (pane == NULL) {
        return false;
    }
    const char sentence[] = "physical keyboard input line one";
    const size_t length = sizeof(sentence) - 1;
    for (size_t index = 0; index < length; ++index) {
        if (SendMessageW(pane, kWmOpusX64QuerySelection, 107,
                         (LPARAM)(unsigned char)sentence[index]) == 0) {
            return false;
        }
    }
    const LRESULT typed_first =
        SendMessageW(pane, kWmOpusX64QuerySelection, 0, 0);
    const LRESULT typed_lim = SendMessageW(pane, kWmOpusX64QuerySelection, 1, 0);
    const LRESULT typed_ins = SendMessageW(pane, kWmOpusX64QuerySelection, 2, 0);
    if (typed_first != (LRESULT)length ||
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

static bool ScriptedInteractionMatched(void) {
    const HWND app = FindWindowA("OpusApp", NULL);
    if (app == NULL) {
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
    OpusUser32PushScriptedInput(NULL, WM_COMMAND, 2, 0);
    SendMessageW(app, WM_COMMAND, kFileNew, 0);
    return IsWindow(app) && FindWindowA("OpusSdmDialog", NULL) == NULL;
}

static bool InsertText(HWND pane, const char *text) {
    for (; *text != 0; ++text) {
        if (SendMessageW(pane, kWmOpusX64QuerySelection, 107,
                         (LPARAM)(unsigned char)*text) == 0) {
            return false;
        }
    }
    return true;
}

static uint32_t ReadLittleU32(const unsigned char *bytes) {
    return (uint32_t)bytes[0] |
           ((uint32_t)bytes[1] << 8) |
           ((uint32_t)bytes[2] << 16) |
           ((uint32_t)bytes[3] << 24);
}

static bool FileHasNativeDocHeader(const char *path) {
    FILE *file = fopen(path, "rb");
    unsigned char header[512];
    size_t read = 0;
    size_t index;
    bool matched;
    g_last_scripted_save_file_open = file != NULL;
    g_last_scripted_save_file_error = 0;
    g_last_scripted_save_file_size = 0;
    g_last_scripted_save_file_read = 0;
    g_last_scripted_save_header0 = 0;
    g_last_scripted_save_header4 = 0;
    g_last_scripted_save_header24 = 0;
    g_last_scripted_save_header48 = 0;
    if (file == NULL) {
        g_last_scripted_save_file_error = (DWORD)errno;
        return false;
    }
    if (fseek(file, 0, SEEK_END) == 0) {
        long size = ftell(file);
        if (size >= 0) {
            g_last_scripted_save_file_size = (DWORD)size;
        }
        rewind(file);
    }
    read = fread(header, 1, sizeof(header), file);
    matched = read == sizeof(header);
    g_last_scripted_save_file_read = (DWORD)read;
    if (matched) {
        g_last_scripted_save_header0 = ReadLittleU32(header + 0);
        g_last_scripted_save_header4 = ReadLittleU32(header + 4);
        g_last_scripted_save_header24 = ReadLittleU32(header + 24);
        g_last_scripted_save_header48 = ReadLittleU32(header + 48);
        matched = (g_last_scripted_save_header0 == 0xfe37 ||
                   g_last_scripted_save_header0 == 0xa59b) &&
                  g_last_scripted_save_header4 == 33 &&
                  g_last_scripted_save_header24 == 25 &&
                  g_last_scripted_save_header48 == 512;
    }
    for (index = 420; matched && index < sizeof(header); ++index) {
        matched = header[index] == 0;
    }
    fclose(file);
    return matched;
}

static bool PollScriptedSaveStatus(HWND app, const char *doc_path) {
    const INT_PTR stage = (INT_PTR)GetPropW(app, OPUSW("OpusX64SaveAsStage"));
    const BOOL header = FileHasNativeDocHeader(doc_path);
    g_last_scripted_save_stage = stage;
    g_last_scripted_save_app_alive = IsWindow(app);
    g_last_scripted_save_opus_dialog_open =
        FindWindowA("OpusSdmDialog", NULL) != NULL;
    g_last_scripted_save_win_dialog_open = FindWindowA("#32770", NULL) != NULL;
    g_last_scripted_save_header = header;
    return stage == 3 && g_last_scripted_save_app_alive &&
           !g_last_scripted_save_opus_dialog_open &&
           !g_last_scripted_save_win_dialog_open && header;
}

static bool ScriptedSaveCurrentDocumentAs(HWND app, const char *doc_path,
                                          bool *dispatched) {
    if (dispatched != NULL) {
        *dispatched = false;
    }
    if (app == NULL || doc_path == NULL || *doc_path == '\0') {
        return false;
    }
    g_last_scripted_save_stage = -1;
    g_last_scripted_save_app_alive = false;
    g_last_scripted_save_opus_dialog_open = false;
    g_last_scripted_save_win_dialog_open = false;
    g_last_scripted_save_header = false;
    g_last_scripted_save_error = 0;
    DeleteFileA(doc_path);
    if (!SetEnvironmentVariableA("WORD1_TEST_FILE_DIALOG_PATH", doc_path)) {
        g_last_scripted_save_error = GetLastError();
        return false;
    }
    g_last_scripted_save_error = 0;
    RemovePropW(app, OPUSW("OpusX64SaveAsStage"));
    SendMessageW(app, WM_COMMAND, kFileSaveAs, 0);
    if (dispatched != NULL) {
        *dispatched = true;
    }
    SetEnvironmentVariableA("WORD1_TEST_FILE_DIALOG_PATH", NULL);
    return PollScriptedSaveStatus(app, doc_path);
}

static bool ScriptedSaveAsMatched(bool *text_inserted, bool *save_attempted) {
    const HWND app = FindWindowA("OpusApp", NULL);
    const HWND pane = FindDocumentPane();
    char temporary_directory[MAX_PATH] = {0};
    char temporary_seed[MAX_PATH] = {0};
    char doc_path[MAX_PATH + 5] = {0};
    char output_path[MAX_PATH] = {0};
    bool matched;
    bool keep_output = false;
    int path_length;
    DWORD output_path_length;
    DWORD temporary_directory_length;
    g_last_scripted_save_has_app = app != NULL;
    g_last_scripted_save_has_pane = pane != NULL;
    if (app == NULL || pane == NULL) {
        return false;
    }
    if (save_attempted != NULL && *save_attempted &&
        g_scripted_save_as_doc_path[0] != '\0') {
        return PollScriptedSaveStatus(app, g_scripted_save_as_doc_path);
    }
    if ((text_inserted == NULL || !*text_inserted) &&
        !InsertText(pane, "native save as text")) {
        return false;
    }
    if (text_inserted != NULL) {
        *text_inserted = true;
    }
    if (g_scripted_save_as_output[0] != '\0') {
        path_length = snprintf(doc_path, sizeof(doc_path), "%s",
                               g_scripted_save_as_output);
        if (path_length <= 0 || path_length >= (int)sizeof(doc_path)) {
            return false;
        }
        keep_output = true;
    } else {
        output_path_length = GetEnvironmentVariableA(
            "WORD1_TEST_SAVE_AS_OUTPUT", output_path, (DWORD)sizeof(output_path));
        if (output_path_length != 0) {
            if (output_path_length >= (DWORD)sizeof(output_path)) {
                return false;
            }
            path_length = snprintf(doc_path, sizeof(doc_path), "%s", output_path);
            if (path_length <= 0 || path_length >= (int)sizeof(doc_path)) {
                return false;
            }
            keep_output = true;
        }
    }
    if (!keep_output) {
        temporary_directory_length = GetTempPathA(
            (DWORD)sizeof(temporary_directory), temporary_directory);
        if (temporary_directory_length == 0 ||
            temporary_directory_length >= (DWORD)sizeof(temporary_directory) ||
            GetTempFileNameA(temporary_directory, "OWD", 0, temporary_seed) == 0) {
            return false;
        }
        DeleteFileA(temporary_seed);
        path_length = snprintf(doc_path, sizeof(doc_path), "%s.doc",
                               temporary_seed);
        if (path_length <= 0 || path_length >= (int)sizeof(doc_path)) {
            return false;
        }
    }
    snprintf(g_scripted_save_as_doc_path, sizeof(g_scripted_save_as_doc_path),
             "%s", doc_path);
    g_scripted_save_as_keep_output = keep_output;
    matched = ScriptedSaveCurrentDocumentAs(app, doc_path, save_attempted);
    if (save_attempted == NULL && !keep_output) {
        DeleteFileA(doc_path);
        g_scripted_save_as_doc_path[0] = '\0';
    }
    return matched;
}

static void FinishScriptedSaveAs(int exit_code) {
    if (g_scripted_save_as_timer != 0) {
        KillTimer(NULL, g_scripted_save_as_timer);
        g_scripted_save_as_timer = 0;
    }
    if (!g_scripted_save_as_keep_output &&
        g_scripted_save_as_doc_path[0] != '\0') {
        DeleteFileA(g_scripted_save_as_doc_path);
        g_scripted_save_as_doc_path[0] = '\0';
    }
    PostQuitMessage(exit_code);
}

static void CALLBACK ScriptedSaveAsTimer(HWND window, UINT message,
                                         UINT_PTR timer, DWORD time) {
    (void)message;
    (void)time;
    (void)window;
    if (g_scripted_save_as_running) return;
    g_scripted_save_as_running = true;
    if (g_scripted_save_as_started == 0) {
        g_scripted_save_as_started = GetTickCount64();
    }
    ++g_scripted_save_as_attempts;
    if (ScriptedSaveAsMatched(&g_scripted_save_as_text_inserted,
                              &g_scripted_save_as_attempted)) {
        g_scripted_save_as_running = false;
        FinishScriptedSaveAs(0);
        return;
    }
    if (GetTickCount64() - g_scripted_save_as_started < 25000) {
        g_scripted_save_as_running = false;
        return;
    }
    fprintf(stderr,
            "WORD1 x64: scripted Save As timed out attempts=%u app=%d pane=%d "
            "stage=%lld app_alive=%d opus_dialog=%d win_dialog=%d header=%d "
            "last_error=%lu file_open=%d file_error=%lu file_size=%lu "
            "file_read=%lu h0=%08lx h4=%08lx h24=%08lx h48=%08lx "
            "output=\"%s\"\n",
            g_scripted_save_as_attempts, g_last_scripted_save_has_app,
            g_last_scripted_save_has_pane,
            (long long)g_last_scripted_save_stage,
            g_last_scripted_save_app_alive,
            g_last_scripted_save_opus_dialog_open,
            g_last_scripted_save_win_dialog_open,
            g_last_scripted_save_header,
            (unsigned long)g_last_scripted_save_error,
            g_last_scripted_save_file_open,
            (unsigned long)g_last_scripted_save_file_error,
            (unsigned long)g_last_scripted_save_file_size,
            (unsigned long)g_last_scripted_save_file_read,
            (unsigned long)g_last_scripted_save_header0,
            (unsigned long)g_last_scripted_save_header4,
            (unsigned long)g_last_scripted_save_header24,
            (unsigned long)g_last_scripted_save_header48,
            g_scripted_save_as_doc_path[0] != '\0' ?
                g_scripted_save_as_doc_path : g_scripted_save_as_output);
    g_scripted_save_as_running = false;
    FinishScriptedSaveAs(9);
    fflush(stderr);
    exit(9);
}

static void ScheduleScriptedSaveAsTimer(void) {
    g_scripted_save_as_started = 0;
    g_scripted_save_as_attempts = 0;
    g_scripted_save_as_text_inserted = false;
    g_scripted_save_as_attempted = false;
    g_scripted_save_as_running = false;
    g_scripted_save_as_timer = 0;
    g_scripted_save_as_doc_path[0] = '\0';
    g_scripted_save_as_keep_output = false;
    g_last_scripted_save_has_app = false;
    g_last_scripted_save_has_pane = false;
    g_last_scripted_save_stage = -1;
    g_last_scripted_save_app_alive = false;
    g_last_scripted_save_opus_dialog_open = false;
    g_last_scripted_save_win_dialog_open = false;
    g_last_scripted_save_header = false;
    g_last_scripted_save_error = 0;
    g_last_scripted_save_file_open = false;
    g_last_scripted_save_file_error = 0;
    g_last_scripted_save_file_size = 0;
    g_last_scripted_save_file_read = 0;
    g_last_scripted_save_header0 = 0;
    g_last_scripted_save_header4 = 0;
    g_last_scripted_save_header24 = 0;
    g_last_scripted_save_header48 = 0;
    g_scripted_save_as_timer = SetTimer(NULL, 1, 1, ScriptedSaveAsTimer);
}

static bool FileStartsWithPdfHeader(const char *path) {
    HANDLE file = CreateFileA(path, GENERIC_READ,
                              FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                              OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    bool matched;
    if (file == INVALID_HANDLE_VALUE) {
        return false;
    }
    char header[5] = {0};
    DWORD read = 0;
    matched = ReadFile(file, header, sizeof(header), &read, NULL) &&
                         read == sizeof(header) &&
                         memcmp(header, "%PDF-", sizeof(header)) == 0;
    CloseHandle(file);
    return matched;
}

static bool ScriptedPdfExportMatched(void) {
    const HWND app = FindWindowA("OpusApp", NULL);
    const HWND pane = FindDocumentPane();
    bool matched;
    if (app == NULL || pane == NULL) {
        return false;
    }
    char temporary_directory[MAX_PATH] = {0};
    char temporary_seed[MAX_PATH] = {0};
    char pdf_path[MAX_PATH + 5] = {0};
    if (GetTempPathA((DWORD)sizeof(temporary_directory),
                     temporary_directory) == 0 ||
        GetTempFileNameA(temporary_directory, "OWP", 0, temporary_seed) == 0) {
        return false;
    }
    DeleteFileA(temporary_seed);
    const int path_length =
        snprintf(pdf_path, sizeof(pdf_path), "%s.pdf", temporary_seed);
    if (path_length <= 0 || path_length >= (int)sizeof(pdf_path)) {
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
                         (LPARAM)(unsigned char)text[index]) == 0) {
            SetEnvironmentVariableA("WORD1_TEST_PDF_PATH", NULL);
            DeleteFileA(pdf_path);
            return false;
        }
    }
    const LRESULT cp_mac_after =
        SendMessageW(pane, kWmOpusX64QuerySelection, 41, 0);
    SendMessageW(app, WM_COMMAND, kExportPdf, 0);
    SetEnvironmentVariableA("WORD1_TEST_PDF_PATH", NULL);
    const LRESULT stage = SendMessageW(pane, kWmOpusX64QuerySelection, 105, 0);
    matched = cp_mac_after > cp_mac_before && stage == 4 &&
              FileStartsWithPdfHeader(pdf_path);
    DeleteFileA(pdf_path);
    return matched;
}

static bool SelectComboText(const HWND combo, LPCWSTR text) {
    const LRESULT index = SendMessageW(
        combo, CB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)text);
    const HWND parent = GetParent(combo);
    const int control_id = GetDlgCtrlID(combo);
    if (index == CB_ERR || parent == NULL ||
        SendMessageW(combo, CB_SETCURSEL, index, 0) == CB_ERR) {
        return false;
    }
    SendMessageW(parent, WM_COMMAND, MAKEWPARAM(control_id, CBN_SELCHANGE),
                 (LPARAM)combo);
    SendMessageW(parent, WM_COMMAND, MAKEWPARAM(control_id, CBN_SELENDOK),
                 (LPARAM)combo);
    return true;
}

static bool ScriptedFontTypingMatched(void) {
    const HWND pane = FindDocumentPane();
    const HWND font_combo = FindControlById(kComboFont);
    const HWND size_combo = FindControlById(kComboSize);
    if (pane == NULL || font_combo == NULL || size_combo == NULL) {
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

static bool ScriptedUiMatched(void) {
    const HWND app = FindWindowA("OpusApp", NULL);
    if (app == NULL) {
        return false;
    }
    OpusUser32PushScriptedInput(NULL, WM_COMMAND, 1, 0);
    SendMessageW(app, WM_COMMAND, kFileNew, 0);
    WCHAR caption[256] = {0};
    if (!IsWindow(app) ||
        GetWindowTextW(app, caption,
                       (int)(sizeof(caption) / sizeof(caption[0]))) == 0 ||
        !OpusWideContains(caption, OPUSW("Document2"))) {
        return false;
    }
    SendMessageW(app, WM_COMMAND, kFileExit, 0);
    return true;
}

static void WriteCrashText(HANDLE file, const char *text) {
    DWORD written = 0;
    WriteFile(file, text, (DWORD)strlen(text), &written, NULL);
}

static void NarrowDiagnosticWide(LPCWSTR source, char *destination,
                                 size_t destination_size) {
    if (destination_size == 0) {
        return;
    }
    if (source == NULL) {
        source = OPUSW("");
    }
    size_t index = 0;
    while (*source != 0 && index + 1 < destination_size) {
        const WCHAR character = *source++;
        destination[index++] =
            character >= 0x20 && character <= 0x7e ?
                (char)character : '?';
    }
    destination[index] = '\0';
}

static void BuildDiagnosticPath(const char *file_name, char *path,
                                size_t path_size) {
    char module_path[MAX_PATH] = {0};
    GetModuleFileNameA(NULL, module_path, MAX_PATH);
    char *file_part = strrchr(module_path, '\\');
    if (file_part != NULL) {
        *file_part = '\0';
        char *bin_part = strrchr(module_path, '\\');
        if (bin_part != NULL) {
            *bin_part = '\0';
        }
    }
    snprintf(path, path_size, "%s\\build\\%s", module_path, file_name);
}

static void ResetRibbonTrace(void) {
    char trace_path[MAX_PATH] = {0};
    BuildDiagnosticPath("WORD1-ribbon.txt", trace_path, sizeof(trace_path));
    HANDLE file = CreateFileA(trace_path, GENERIC_WRITE,
                              FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                              CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) {
        return;
    }
    char header[128] = {0};
    snprintf(header, sizeof(header), "WORD1 ribbon trace pid=%u\r\n",
             GetCurrentProcessId());
    WriteCrashText(file, header);
    CloseHandle(file);
}

static void WriteCurrentStack(HANDLE file, unsigned frames_to_skip) {
    void *frames[64] = {0};
    const USHORT frame_count =
        CaptureStackBackTrace(frames_to_skip, 64, frames, NULL);
    const uintptr_t module_base = (uintptr_t)GetModuleHandleW(NULL);
    char raw_line[160] = {0};
    for (USHORT index = 0; index < frame_count; ++index) {
        const uintptr_t address = (uintptr_t)frames[index];
        if (address >= module_base) {
            snprintf(raw_line, sizeof(raw_line),
                     "raw #%u 0x%016llX WORD1+0x%llX\r\n", index,
                     (unsigned long long)address,
                     (unsigned long long)(address - module_base));
        } else {
            snprintf(raw_line, sizeof(raw_line),
                     "raw #%u 0x%016llX\r\n", index,
                     (unsigned long long)address);
        }
        WriteCrashText(file, raw_line);
    }
    HANDLE process = GetCurrentProcess();
    SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS);
    if (!SymInitialize(process, NULL, TRUE)) {
        return;
    }

    char line_text[1024] = {0};
    for (USHORT index = 0; index < frame_count; ++index) {
        const DWORD64 address = (DWORD64)(uintptr_t)frames[index];
        char symbol_storage[sizeof(SYMBOL_INFO) + MAX_SYM_NAME] = {0};
        SYMBOL_INFO *symbol = (SYMBOL_INFO *)symbol_storage;
        symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
        symbol->MaxNameLen = MAX_SYM_NAME;
        DWORD64 displacement = 0;
        const BOOL has_symbol =
            SymFromAddr(process, address, &displacement, symbol);

        IMAGEHLP_LINE64 source_line = {0};
        source_line.SizeOfStruct = sizeof(source_line);
        DWORD line_displacement = 0;
        const BOOL has_line = SymGetLineFromAddr64(
            process, address, &line_displacement, &source_line);
        if (has_symbol && has_line) {
            snprintf(line_text, sizeof(line_text),
                     "#%u 0x%016llX %s+0x%llX (%s:%u)\r\n",
                     index, (unsigned long long)address, symbol->Name,
                     (unsigned long long)displacement,
                     source_line.FileName, source_line.LineNumber);
        } else if (has_symbol) {
            snprintf(line_text, sizeof(line_text),
                     "#%u 0x%016llX %s+0x%llX\r\n", index,
                     (unsigned long long)address, symbol->Name,
                     (unsigned long long)displacement);
        } else {
            snprintf(line_text, sizeof(line_text),
                     "#%u 0x%016llX\r\n", index,
                     (unsigned long long)address);
        }
        WriteCrashText(file, line_text);
    }
    SymCleanup(process);
}

static int __cdecl WriteRtcFailure(int error_type, LPCWSTR file_name, int line,
                                   LPCWSTR module_name, LPCWSTR format, ...) {
    char diagnostic_path[MAX_PATH] = {0};
    BuildDiagnosticPath("WORD1-rtc.txt", diagnostic_path,
                        sizeof(diagnostic_path));
    HANDLE file = CreateFileA(diagnostic_path, GENERIC_WRITE, FILE_SHARE_READ,
                              NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL,
                              NULL);
    if (file == INVALID_HANDLE_VALUE) {
        return 0;
    }

    WCHAR message[2048] = {0};
    va_list arguments;
    va_start(arguments, format);
#if defined(_MSC_VER)
    _vsnwprintf_s(message, _countof(message), _TRUNCATE, format, arguments);
#else
    size_t message_index = 0;
    while (format != NULL && format[message_index] != 0 &&
           message_index + 1 < _countof(message)) {
        message[message_index] = format[message_index];
        ++message_index;
    }
    message[message_index] = 0;
#endif
    va_end(arguments);

    char file_name_ansi[MAX_PATH] = {0};
    char module_name_ansi[MAX_PATH] = {0};
    char message_ansi[4096] = {0};
    NarrowDiagnosticWide(file_name, file_name_ansi, sizeof(file_name_ansi));
    NarrowDiagnosticWide(module_name, module_name_ansi,
                         sizeof(module_name_ansi));
    NarrowDiagnosticWide(message, message_ansi, sizeof(message_ansi));

    char header[4096] = {0};
    snprintf(header, sizeof(header),
             "RTC failure %d at %s:%d (%s)\r\n%s\r\n", error_type,
             file_name_ansi, line, module_name_ansi, message_ansi);
    WriteCrashText(file, header);
    WriteCurrentStack(file, 1);
    CloseHandle(file);
    return 0;
}

static LONG WINAPI WriteCrashStack(EXCEPTION_POINTERS *exception) {
    char crash_path[MAX_PATH] = {0};
    BuildDiagnosticPath("WORD1-crash.txt", crash_path, sizeof(crash_path));

    HANDLE file = CreateFileA(crash_path, GENERIC_WRITE, FILE_SHARE_READ,
                              NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL,
                              NULL);
    if (file == INVALID_HANDLE_VALUE) {
        return EXCEPTION_EXECUTE_HANDLER;
    }

    char line_text[1024] = {0};
    snprintf(line_text, sizeof(line_text),
             "Exception 0x%08X at 0x%016llX\r\n",
             exception->ExceptionRecord->ExceptionCode,
             (unsigned long long)(uintptr_t)
                 exception->ExceptionRecord->ExceptionAddress);
    WriteCrashText(file, line_text);

    if (exception->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION &&
        exception->ExceptionRecord->NumberParameters >= 2) {
        const ULONG_PTR operation = exception->ExceptionRecord->ExceptionInformation[0];
        const ULONG_PTR address = exception->ExceptionRecord->ExceptionInformation[1];
        const char* operation_name = operation == 0 ? "read" :
                                     operation == 1 ? "write" :
                                     operation == 8 ? "execute" : "unknown";
        snprintf(line_text, sizeof(line_text),
                 "Access violation: %s at 0x%016llX\r\n",
                 operation_name, (unsigned long long)address);
        WriteCrashText(file, line_text);
    }

#if defined(_M_X64)
    const CONTEXT* fault_context = exception->ContextRecord;
    snprintf(
        line_text, sizeof(line_text),
        "RAX=%016llX RBX=%016llX RCX=%016llX RDX=%016llX\r\n"
        "RSI=%016llX RDI=%016llX RBP=%016llX RSP=%016llX\r\n"
        "R8 =%016llX R9 =%016llX R10=%016llX R11=%016llX\r\n",
        (unsigned long long)fault_context->Rax,
        (unsigned long long)fault_context->Rbx,
        (unsigned long long)fault_context->Rcx,
        (unsigned long long)fault_context->Rdx,
        (unsigned long long)fault_context->Rsi,
        (unsigned long long)fault_context->Rdi,
        (unsigned long long)fault_context->Rbp,
        (unsigned long long)fault_context->Rsp,
        (unsigned long long)fault_context->R8,
        (unsigned long long)fault_context->R9,
        (unsigned long long)fault_context->R10,
        (unsigned long long)fault_context->R11);
    WriteCrashText(file, line_text);
#endif

    const uintptr_t module_base = (uintptr_t)GetModuleHandleW(NULL);
    const IMAGE_DOS_HEADER *dos_header = (const IMAGE_DOS_HEADER *)module_base;
    const IMAGE_NT_HEADERS64 *nt_headers =
        (const IMAGE_NT_HEADERS64 *)(module_base + (uintptr_t)dos_header->e_lfanew);
    const uintptr_t module_limit =
        module_base + nt_headers->OptionalHeader.SizeOfImage;
    ULONG_PTR stack_low = 0;
    ULONG_PTR stack_high = 0;
    GetCurrentThreadStackLimits(&stack_low, &stack_high);

    CONTEXT unwind_context = *exception->ContextRecord;
    for (unsigned index = 0; index < 64 && unwind_context.Rip != 0; ++index) {
        const DWORD64 control_pc = unwind_context.Rip;
        if (control_pc >= module_base && control_pc < module_limit) {
            snprintf(line_text, sizeof(line_text),
                     "unwind #%u WORD1+0x%llX rsp=0x%llX\r\n", index,
                     (unsigned long long)(control_pc - module_base),
                     (unsigned long long)unwind_context.Rsp);
        } else {
            snprintf(line_text, sizeof(line_text),
                     "unwind #%u 0x%016llX rsp=0x%llX\r\n", index,
                     (unsigned long long)control_pc,
                     (unsigned long long)unwind_context.Rsp);
        }
        WriteCrashText(file, line_text);

        DWORD64 image_base = 0;
        PRUNTIME_FUNCTION runtime_function =
            RtlLookupFunctionEntry(control_pc, &image_base, NULL);
        if (runtime_function != NULL) {
            PVOID handler_data = NULL;
            DWORD64 establisher_frame = 0;
            RtlVirtualUnwind(0, image_base, control_pc, runtime_function,
                             &unwind_context, &handler_data,
                             &establisher_frame, NULL);
        } else {
            if (unwind_context.Rsp < stack_low ||
                unwind_context.Rsp + sizeof(DWORD64) > stack_high) {
                break;
            }
            unwind_context.Rip =
                *(const DWORD64 *)unwind_context.Rsp;
            unwind_context.Rsp += sizeof(DWORD64);
        }
    }

    const uintptr_t stack_begin = exception->ContextRecord->Rsp;
    const uintptr_t stack_limit = stack_begin + 8192u;
    const uintptr_t stack_end =
        (uintptr_t)stack_high < stack_limit ? (uintptr_t)stack_high : stack_limit;
    for (uintptr_t cursor = stack_begin;
         cursor + sizeof(uintptr_t) <= stack_end;
         cursor += sizeof(uintptr_t)) {
        const uintptr_t candidate = *(const uintptr_t *)cursor;
        if (candidate >= module_base && candidate < module_limit) {
            snprintf(line_text, sizeof(line_text),
                     "stack+0x%llX: WORD1+0x%llX\r\n",
                     (unsigned long long)(cursor - stack_begin),
                     (unsigned long long)(candidate - module_base));
            WriteCrashText(file, line_text);
        }
    }

#if defined(_WIN32)
    HANDLE process = GetCurrentProcess();
    SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS);
    if (SymInitialize(process, NULL, TRUE)) {
        CONTEXT context = *exception->ContextRecord;
        STACKFRAME64 frame = {0};
        frame.AddrPC.Offset = context.Rip;
        frame.AddrPC.Mode = AddrModeFlat;
        frame.AddrStack.Offset = context.Rsp;
        frame.AddrStack.Mode = AddrModeFlat;
        frame.AddrFrame.Offset = context.Rbp;
        frame.AddrFrame.Mode = AddrModeFlat;

        for (unsigned index = 0; index < 64 && frame.AddrPC.Offset != 0;
             ++index) {
            char symbol_storage[sizeof(SYMBOL_INFO) + MAX_SYM_NAME] = {0};
            SYMBOL_INFO *symbol = (SYMBOL_INFO *)symbol_storage;
            symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
            symbol->MaxNameLen = MAX_SYM_NAME;
            DWORD64 displacement = 0;
            const BOOL has_symbol =
                SymFromAddr(process, frame.AddrPC.Offset, &displacement, symbol);

            IMAGEHLP_LINE64 source_line = {0};
            source_line.SizeOfStruct = sizeof(source_line);
            DWORD line_displacement = 0;
            const BOOL has_line = SymGetLineFromAddr64(
                process, frame.AddrPC.Offset, &line_displacement, &source_line);

            if (has_symbol && has_line) {
                snprintf(line_text, sizeof(line_text),
                         "#%u 0x%016llX %s+0x%llX (%s:%lu)\r\n",
                         index, (unsigned long long)frame.AddrPC.Offset,
                         symbol->Name, (unsigned long long)displacement,
                         source_line.FileName, source_line.LineNumber);
            } else if (has_symbol) {
                snprintf(line_text, sizeof(line_text),
                         "#%u 0x%016llX %s+0x%llX\r\n", index,
                         (unsigned long long)frame.AddrPC.Offset,
                         symbol->Name, (unsigned long long)displacement);
            } else {
                snprintf(line_text, sizeof(line_text),
                         "#%u 0x%016llX\r\n", index,
                         (unsigned long long)frame.AddrPC.Offset);
            }
            WriteCrashText(file, line_text);

            if (!StackWalk64(IMAGE_FILE_MACHINE_AMD64, process,
                             GetCurrentThread(), &frame, &context, NULL,
                             SymFunctionTableAccess64, SymGetModuleBase64,
                             NULL)) {
                break;
            }
        }
        SymCleanup(process);
    }
#endif

    CloseHandle(file);
    return EXCEPTION_EXECUTE_HANDLER;
}

static LONG WINAPI ObserveVectoredException(EXCEPTION_POINTERS *exception) {
    static volatile LONG writing_exception = 0;
    if (exception != NULL && exception->ExceptionRecord != NULL &&
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

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE previous,
                    PWSTR command_line, int show_command) {
#ifdef __EMSCRIPTEN__
    extern long dtickCaret;
    dtickCaret = 0x3fffffffL;
#endif
    const LPCWSTR self_test = OPUSW("--self-test");
    const LPCWSTR scripted_ui_test = OPUSW("--scripted-ui-test");
    const LPCWSTR scripted_key_test = OPUSW("--scripted-key-test");
    const LPCWSTR scripted_typing_test = OPUSW("--scripted-typing-test");
    const LPCWSTR scripted_clipboard_test = OPUSW("--scripted-clipboard-test");
    const LPCWSTR scripted_unicode_test = OPUSW("--scripted-unicode-test");
    const LPCWSTR scripted_about_test = OPUSW("--scripted-about-test");
    const LPCWSTR scripted_selection_test = OPUSW("--scripted-selection-test");
    const LPCWSTR scripted_interaction_test = OPUSW("--scripted-interaction-test");
    const LPCWSTR scripted_save_as_test = OPUSW("--scripted-save-as-test");
    const LPCWSTR scripted_save_as_output =
        OPUSW("--scripted-save-as-output=");
    const LPCWSTR scripted_pdf_export_test =
        OPUSW("--scripted-pdf-export-test");
    const LPCWSTR scripted_font_typing_test =
        OPUSW("--scripted-font-typing-test");
    char save_as_output_probe[MAX_PATH] = {0};
    bool save_as_output_requested =
        GetEnvironmentVariableA("WORD1_TEST_SAVE_AS_OUTPUT",
                                save_as_output_probe,
                                (DWORD)sizeof(save_as_output_probe)) != 0;
    if (OpusWideReadArgumentValue(command_line, scripted_save_as_output,
                                  save_as_output_probe,
                                  (DWORD)sizeof(save_as_output_probe)) ||
        OpusWideReadArgumentValue(GetCommandLineW(), scripted_save_as_output,
                                  save_as_output_probe,
                                  (DWORD)sizeof(save_as_output_probe))) {
        snprintf(g_scripted_save_as_output,
                 sizeof(g_scripted_save_as_output), "%s",
                 save_as_output_probe);
        save_as_output_requested = true;
    }
    if (OpusWideContains(command_line, self_test) ||
        OpusWideContains(GetCommandLineW(), self_test)) {
        return 0;
    }
    const bool run_scripted_key_test =
        OpusWideContains(command_line, scripted_key_test) ||
        OpusWideContains(GetCommandLineW(), scripted_key_test);
    const bool run_scripted_ui_test =
        OpusWideContains(command_line, scripted_ui_test) ||
        OpusWideContains(GetCommandLineW(), scripted_ui_test);
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
        OpusWideContains(GetCommandLineW(), scripted_save_as_test) ||
        save_as_output_requested;
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

    char command_line_ansi[32768] = {0};
    if (command_line != NULL) {
        WideCharToMultiByte(CP_ACP, 0, command_line, -1, command_line_ansi,
                            (int)sizeof(command_line_ansi), NULL, NULL);
    }
    if (run_scripted_key_test) {
        OpusUser32ExpectScriptedChar('a', 2);
        OpusUser32PushScriptedInput(NULL, WM_KEYDOWN, 'A', 1);
        OpusUser32PushScriptedInput(NULL, WM_KEYUP, 'A', 2);
        OpusUser32PushScriptedInput(NULL, WM_KEYDOWN, 'A', 3);
        OpusUser32PushScriptedInput(NULL, WM_KEYUP, 'A', 4);
        OpusUser32PushScriptedInput(NULL, WM_QUIT, 0, 0);
    } else if (run_scripted_typing_test || run_scripted_clipboard_test) {
        OpusUser32ExpectScriptedChar('a', 3);
        OpusUser32PushScriptedInput(NULL, WM_KEYDOWN, 'A', 1);
        OpusUser32PushScriptedInput(NULL, WM_KEYUP, 'A', 2);
        OpusUser32PushScriptedInput(NULL, WM_KEYDOWN, 'A', 3);
        OpusUser32PushScriptedInput(NULL, WM_KEYUP, 'A', 4);
        OpusUser32PushScriptedInput(NULL, WM_KEYDOWN, 'A', 5);
        OpusUser32PushScriptedInput(NULL, WM_KEYUP, 'A', 6);
        OpusUser32PushScriptedInput(NULL, WM_QUIT, 0, 0);
    } else if (run_scripted_ui_test || run_scripted_unicode_test ||
               run_scripted_about_test ||
               run_scripted_selection_test || run_scripted_interaction_test ||
               run_scripted_save_as_test || run_scripted_pdf_export_test ||
               run_scripted_font_typing_test) {
        if (run_scripted_save_as_test && save_as_output_requested) {
            ScheduleScriptedSaveAsTimer();
        } else {
            OpusUser32PushScriptedInput(NULL, WM_QUIT, 0, 0);
        }
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
    if (run_scripted_save_as_test && !save_as_output_requested &&
        !ScriptedSaveAsMatched(NULL, NULL)) return 9;
    if (g_scripted_save_as_output[0] != '\0' &&
        !FileHasNativeDocHeader(g_scripted_save_as_output)) {
        return 13;
    }
    if (run_scripted_pdf_export_test && !ScriptedPdfExportMatched()) return 10;
    if (run_scripted_font_typing_test && !ScriptedFontTypingMatched()) {
        return 11;
    }
    if (run_scripted_ui_test && !ScriptedUiMatched()) return 12;
    return result;
}

#ifndef _WIN32
int main(const int argument_count, char **arguments) {
    static const char save_as_output_prefix[] = "--scripted-save-as-output=";
    size_t length = 1;
    WCHAR *command_line;
    size_t offset = 0;
    for (int argument = 1; argument < argument_count; ++argument) {
        if (arguments[argument] != NULL &&
            strncmp(arguments[argument], save_as_output_prefix,
                    sizeof(save_as_output_prefix) - 1) == 0) {
            snprintf(g_scripted_save_as_output,
                     sizeof(g_scripted_save_as_output), "%s",
                     arguments[argument] + sizeof(save_as_output_prefix) - 1);
        }
        if (argument > 1) {
            ++length;
        }
        if (arguments[argument] != NULL) {
            length += strlen(arguments[argument]);
        }
    }
    command_line = (WCHAR *)calloc(length, sizeof(command_line[0]));
    if (command_line == NULL) {
        return 1;
    }
    for (int argument = 1; argument < argument_count; ++argument) {
        if (argument > 1) {
            command_line[offset++] = OPUSW(" ")[0];
        }
        if (arguments[argument] == NULL) {
            continue;
        }
        for (const unsigned char *text = (const unsigned char *)arguments[argument];
             *text != 0; ++text) {
            command_line[offset++] = (WCHAR)*text;
        }
    }
    command_line[offset] = 0;
    const int result = wWinMain(NULL, NULL, command_line, 0);
    free(command_line);
    return result;
}
#endif
