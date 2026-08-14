#include "opus-native-compat.h"
#include "opus-native-heap.h"
#include "objbase.h"
#include "dac.h"

#ifdef index
#undef index
#endif

#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

extern void** hcabDlgCur;
extern uintptr_t wRefDlgCur;
extern HWND vhWndMsgBoxParent;
void GetCabSz(void**, char*, uint16_t, uint16_t);
int FSetCabSz(void**, const char*, uint16_t);
int OpusModernPathIsDocx(const char*);
int OpusModernPathIsOdt(const char*);
int OpusModernDocxToTextFile(const char*, const char*);
int OpusModernOdtToTextFile(const char*, const char*);
int OpusModernRtfFileToDocx(const char*, const char*);
int OpusModernRtfFileToOdt(const char*, const char*);
int OpusSaveDocumentAsDocx(int, const char*);

void OpusX64TraceRibbon(const char*, int, int, int, int, long, long, int);

/* Flat, stateful implementation of the public SDM 2.21 dialog API used by Opus.
 * The archive contains the SDM headers and 16-bit .obj files, but not its C
 * sources. This layer retains the original CAB/TMC/HDLG contracts and supplies
 * the dialog state that the original Word modules expect. Native window
 * creation is deliberately separate from this state layer.
 */

typedef uint16_t Word;
typedef uint32_t Dword;
typedef Word Hdlg;
typedef Word Tmc;
typedef void** Hcab;
typedef Word (*OriginalListProc)(Word, char*, int, Word, Word, Word);
typedef int (*FontValueProc)(const char*);
typedef void (*FontNameFromValueProc)(int, char*, int);
typedef int (*NativeDialogProc)(Word, Tmc, Word, Word, Word);

typedef enum ControlKind {
    CONTROL_KIND_UNKNOWN,
    CONTROL_KIND_BUTTON,
    CONTROL_KIND_COMBO,
    CONTROL_KIND_EDIT,
    CONTROL_KIND_LIST,
    CONTROL_KIND_STATIC,
} ControlKind;

typedef struct Rec {
    int x;
    int y;
    int dx;
    int dy;
} Rec;

typedef struct DltHeader {
    Rec rec;
    Word hid;
    Tmc tmc_sel_init;
    void* dialog_proc;
    Word base_item_count;
    Word border;
} DltHeader;

typedef struct Dli {
    HWND hwnd;
    int dx;
    int dy;
    Dword flags;
    uintptr_t reference;
    unsigned char* runtime_items;
} Dli;

typedef struct ByteBuffer {
    unsigned char* data;
    size_t size;
} ByteBuffer;

typedef struct StringList {
    char** data;
    size_t size;
    size_t capacity;
} StringList;

typedef struct ControlState {
    ControlKind kind;
    Word value;
    Dword selection;
    bool enabled;
    bool visible;
    Word text_limit;
    Rec rectangle;
    HWND window;
    char* text;
    ByteBuffer large_value;
    StringList entries;
} ControlState;

typedef struct ControlEntry {
    Tmc key;
    ControlState value;
} ControlEntry;

typedef struct DialogState {
    Hdlg handle;
    DltHeader** template_handle;
    Hcab cab;
    HWND window;
    uintptr_t reference;
    Word hid;
    Word sab;
    Tmc focus;
    Tmc default_tmc;
    Tmc result_tmc;
    bool visible;
    bool dying;
    bool modal;
    bool native_modal;
    bool commands_active;
    char* caption;
    char* current_directory;
    char* file_pattern;
    ControlEntry* controls;
    size_t control_count;
    size_t control_capacity;
    ControlState* untracked_controls;
    size_t untracked_count;
    size_t untracked_capacity;
} DialogState;

typedef struct DialogEntry {
    Hdlg key;
    DialogState value;
} DialogEntry;

typedef struct AliasEntry {
    char* key;
    char* value;
} AliasEntry;

static DialogEntry* g_dialogs;
static size_t g_dialog_count;
static size_t g_dialog_capacity;
static DialogState g_no_dialog;
static ControlState g_fallback_control;
static Hdlg g_next_dialog = 1;
static Hdlg g_current_dialog;
static Hdlg g_focus_dialog;
static bool g_initialized;
static bool g_noninteractive;
static OriginalListProc g_list_font_name;
static OriginalListProc g_list_font_size;
static OriginalListProc g_list_styles;
static OriginalListProc g_list_character_color;
static FontValueProc g_font_name_to_value;
static FontValueProc g_font_size_to_value;
static FontNameFromValueProc g_font_name_from_value;

typedef struct Win95SaveAlias {
    bool active;
    bool created;
    char* selected_path;
    char* legacy_path;
} Win95SaveAlias;

static Win95SaveAlias g_win95_save_alias;
static AliasEntry* g_win95_saved_aliases;
static size_t g_win95_saved_alias_count;
static size_t g_win95_saved_alias_capacity;
static char* g_win95_staging_directory;
static const WCHAR k_save_as_stage_property[] = {
    'O', 'p', 'u', 's', 'X', '6', '4', 'S', 'a', 'v', 'e', 'A', 's',
    'S', 't', 'a', 'g', 'e', '\0'};

static void sdm_cleanup(void);

static size_t min_size(size_t left, size_t right) {
    return left < right ? left : right;
}

static int max_int(int left, int right) {
    return left > right ? left : right;
}

static int min_int(int left, int right) {
    return left < right ? left : right;
}

static char* xstrndup(const char* text, size_t length) {
    char* copy = (char*) malloc(length + 1);
    if (copy == NULL) {
        return NULL;
    }
    if (length != 0) {
        memcpy(copy, text, length);
    }
    copy[length] = '\0';
    return copy;
}

static char* xstrdup(const char* text) {
    return xstrndup(text == NULL ? "" : text, strlen(text == NULL ? "" : text));
}

static bool str_empty(const char* text) {
    return text == NULL || text[0] == '\0';
}

static void str_set(char** destination, const char* source) {
    char* copy = xstrdup(source);
    if (copy == NULL) {
        return;
    }
    free(*destination);
    *destination = copy;
}

static void str_set_n(char** destination, const char* source, size_t length) {
    char* copy = xstrndup(source, length);
    if (copy == NULL) {
        return;
    }
    free(*destination);
    *destination = copy;
}

static bool str_starts_with(const char* text, const char* prefix) {
    const size_t prefix_length = strlen(prefix);
    return text != NULL && strncmp(text, prefix, prefix_length) == 0;
}

static void str_to_lower_in_place(char* text) {
    if (text == NULL) {
        return;
    }
    for (; *text != '\0'; ++text) {
        *text = (char) tolower((unsigned char) *text);
    }
}

static char* str_join3(const char* first, const char* second,
                       const char* third) {
    const size_t first_length = strlen(first == NULL ? "" : first);
    const size_t second_length = strlen(second == NULL ? "" : second);
    const size_t third_length = strlen(third == NULL ? "" : third);
    char* joined = (char*) malloc(first_length + second_length + third_length + 1);
    if (joined == NULL) {
        return NULL;
    }
    memcpy(joined, first == NULL ? "" : first, first_length);
    memcpy(joined + first_length, second == NULL ? "" : second, second_length);
    memcpy(joined + first_length + second_length, third == NULL ? "" : third,
           third_length);
    joined[first_length + second_length + third_length] = '\0';
    return joined;
}

static void byte_buffer_free(ByteBuffer* buffer) {
    free(buffer->data);
    buffer->data = NULL;
    buffer->size = 0;
}

static bool byte_buffer_resize(ByteBuffer* buffer, size_t size) {
    unsigned char* data = (unsigned char*) realloc(buffer->data, size);
    if (data == NULL && size != 0) {
        return false;
    }
    buffer->data = data;
    buffer->size = size;
    return true;
}

static void string_list_clear(StringList* list) {
    for (size_t index = 0; index < list->size; ++index) {
        free(list->data[index]);
    }
    list->size = 0;
}

static void string_list_free(StringList* list) {
    string_list_clear(list);
    free(list->data);
    list->data = NULL;
    list->capacity = 0;
}

static bool string_list_reserve(StringList* list, size_t capacity) {
    if (capacity <= list->capacity) {
        return true;
    }
    size_t next = list->capacity == 0 ? 8 : list->capacity * 2;
    while (next < capacity) {
        next *= 2;
    }
    char** data = (char**) realloc(list->data, next * sizeof(list->data[0]));
    if (data == NULL) {
        return false;
    }
    list->data = data;
    list->capacity = next;
    return true;
}

static bool string_list_push(StringList* list, const char* text) {
    if (!string_list_reserve(list, list->size + 1)) {
        return false;
    }
    list->data[list->size] = xstrdup(text);
    if (list->data[list->size] == NULL) {
        return false;
    }
    ++list->size;
    return true;
}

static bool string_list_insert(StringList* list, size_t index,
                               const char* text) {
    if (index > list->size) {
        index = list->size;
    }
    if (!string_list_reserve(list, list->size + 1)) {
        return false;
    }
    memmove(list->data + index + 1, list->data + index,
            (list->size - index) * sizeof(list->data[0]));
    list->data[index] = xstrdup(text);
    if (list->data[index] == NULL) {
        memmove(list->data + index, list->data + index + 1,
                (list->size - index) * sizeof(list->data[0]));
        return false;
    }
    ++list->size;
    return true;
}

static void string_list_delete(StringList* list, size_t index) {
    if (index >= list->size) {
        return;
    }
    free(list->data[index]);
    memmove(list->data + index, list->data + index + 1,
            (list->size - index - 1) * sizeof(list->data[0]));
    --list->size;
}

static int string_list_find(const StringList* list, const char* text) {
    for (size_t index = 0; index < list->size; ++index) {
        if (strcmp(list->data[index], text) == 0) {
            return (int) index;
        }
    }
    return -1;
}

static bool string_list_contains_i(const StringList* list, const char* text) {
    for (size_t index = 0; index < list->size; ++index) {
        if (_stricmp(list->data[index], text) == 0) {
            return true;
        }
    }
    return false;
}

static void control_init(ControlState* state) {
    memset(state, 0, sizeof(*state));
    state->kind = CONTROL_KIND_UNKNOWN;
    state->enabled = true;
    state->visible = true;
    state->text_limit = 0xffff;
}

static void control_free(ControlState* state) {
    free(state->text);
    byte_buffer_free(&state->large_value);
    string_list_free(&state->entries);
    control_init(state);
}

static void dialog_init(DialogState* dialog) {
    memset(dialog, 0, sizeof(*dialog));
    dialog->default_tmc = 1;
    dialog->result_tmc = 2;
}

static void dialog_free(DialogState* dialog) {
    for (size_t index = 0; index < dialog->control_count; ++index) {
        control_free(&dialog->controls[index].value);
    }
    free(dialog->controls);
    for (size_t index = 0; index < dialog->untracked_count; ++index) {
        control_free(&dialog->untracked_controls[index]);
    }
    free(dialog->untracked_controls);
    free(dialog->caption);
    free(dialog->current_directory);
    free(dialog->file_pattern);
    dialog_init(dialog);
}

static ControlState* dialog_find_control(DialogState* dialog, Tmc tmc) {
    const Tmc key = (Tmc)(tmc & ~0x8000u);
    for (size_t index = 0; index < dialog->control_count; ++index) {
        if (dialog->controls[index].key == key) {
            return &dialog->controls[index].value;
        }
    }
    return NULL;
}

static const ControlState* dialog_find_control_const(
    const DialogState* dialog, Tmc tmc) {
    return dialog_find_control((DialogState*) dialog, tmc);
}

static ControlState* dialog_control(DialogState* dialog, Tmc tmc) {
    const Tmc key = (Tmc)(tmc & ~0x8000u);
    ControlState* found = dialog_find_control(dialog, key);
    if (found != NULL) {
        return found;
    }
    if (dialog->control_count == dialog->control_capacity) {
        size_t next = dialog->control_capacity == 0 ? 16 :
            dialog->control_capacity * 2;
        ControlEntry* controls = (ControlEntry*) realloc(
            dialog->controls, next * sizeof(dialog->controls[0]));
        if (controls == NULL) {
            return &g_fallback_control;
        }
        dialog->controls = controls;
        dialog->control_capacity = next;
    }
    ControlEntry* entry = &dialog->controls[dialog->control_count++];
    entry->key = key;
    control_init(&entry->value);
    return &entry->value;
}

static bool dialog_push_untracked(DialogState* dialog,
                                  const ControlState* state) {
    if (dialog->untracked_count == dialog->untracked_capacity) {
        size_t next = dialog->untracked_capacity == 0 ? 8 :
            dialog->untracked_capacity * 2;
        ControlState* controls = (ControlState*) realloc(
            dialog->untracked_controls,
            next * sizeof(dialog->untracked_controls[0]));
        if (controls == NULL) {
            return false;
        }
        dialog->untracked_controls = controls;
        dialog->untracked_capacity = next;
    }
    dialog->untracked_controls[dialog->untracked_count++] = *state;
    return true;
}

static DialogState* find_dialog(Hdlg dialog) {
    for (size_t index = 0; index < g_dialog_count; ++index) {
        if (g_dialogs[index].key == dialog) {
            return &g_dialogs[index].value;
        }
    }
    return NULL;
}

static bool dialog_map_insert(Hdlg handle, DialogState* dialog) {
    if (g_dialog_count == g_dialog_capacity) {
        size_t next = g_dialog_capacity == 0 ? 8 : g_dialog_capacity * 2;
        DialogEntry* entries = (DialogEntry*) realloc(
            g_dialogs, next * sizeof(g_dialogs[0]));
        if (entries == NULL) {
            return false;
        }
        g_dialogs = entries;
        g_dialog_capacity = next;
    }
    g_dialogs[g_dialog_count].key = handle;
    g_dialogs[g_dialog_count].value = *dialog;
    ++g_dialog_count;
    return true;
}

static void dialog_map_erase(Hdlg handle) {
    for (size_t index = 0; index < g_dialog_count; ++index) {
        if (g_dialogs[index].key == handle) {
            dialog_free(&g_dialogs[index].value);
            memmove(g_dialogs + index, g_dialogs + index + 1,
                    (g_dialog_count - index - 1) * sizeof(g_dialogs[0]));
            --g_dialog_count;
            return;
        }
    }
}

static void dialog_map_clear(void) {
    for (size_t index = 0; index < g_dialog_count; ++index) {
        dialog_free(&g_dialogs[index].value);
    }
    free(g_dialogs);
    g_dialogs = NULL;
    g_dialog_count = 0;
    g_dialog_capacity = 0;
}

static AliasEntry* alias_find(const char* key) {
    for (size_t index = 0; index < g_win95_saved_alias_count; ++index) {
        if (strcmp(g_win95_saved_aliases[index].key, key) == 0) {
            return &g_win95_saved_aliases[index];
        }
    }
    return NULL;
}

static bool alias_set(const char* key, const char* value) {
    AliasEntry* entry = alias_find(key);
    if (entry != NULL) {
        str_set(&entry->value, value);
        return true;
    }
    if (g_win95_saved_alias_count == g_win95_saved_alias_capacity) {
        size_t next = g_win95_saved_alias_capacity == 0 ? 8 :
            g_win95_saved_alias_capacity * 2;
        AliasEntry* entries = (AliasEntry*) realloc(
            g_win95_saved_aliases, next * sizeof(g_win95_saved_aliases[0]));
        if (entries == NULL) {
            return false;
        }
        g_win95_saved_aliases = entries;
        g_win95_saved_alias_capacity = next;
    }
    entry = &g_win95_saved_aliases[g_win95_saved_alias_count++];
    entry->key = xstrdup(key);
    entry->value = xstrdup(value);
    if (entry->key == NULL || entry->value == NULL) {
        free(entry->key);
        free(entry->value);
        --g_win95_saved_alias_count;
        return false;
    }
    return true;
}

static void save_alias_reset(void) {
    free(g_win95_save_alias.selected_path);
    free(g_win95_save_alias.legacy_path);
    memset(&g_win95_save_alias, 0, sizeof(g_win95_save_alias));
}

static void sdm_cleanup(void) {
    if (g_win95_save_alias.created && !str_empty(g_win95_save_alias.legacy_path)) {
        DeleteFileA(g_win95_save_alias.legacy_path);
    }
    for (size_t index = 0; index < g_win95_saved_alias_count; ++index) {
        DeleteFileA(g_win95_saved_aliases[index].key);
        free(g_win95_saved_aliases[index].key);
        free(g_win95_saved_aliases[index].value);
    }
    free(g_win95_saved_aliases);
    g_win95_saved_aliases = NULL;
    g_win95_saved_alias_count = 0;
    g_win95_saved_alias_capacity = 0;
    if (!str_empty(g_win95_staging_directory)) {
        RemoveDirectoryA(g_win95_staging_directory);
    }
    free(g_win95_staging_directory);
    g_win95_staging_directory = NULL;
    save_alias_reset();
}

static char* win95_alias_key(const char* path) {
    char full_path[32768];
    char* key = NULL;
    memset(full_path, 0, sizeof(full_path));
    if (!str_empty(path)) {
        const DWORD length = GetFullPathNameA(
            path, (DWORD) sizeof(full_path), full_path, NULL);
        if (length != 0 && length < sizeof(full_path)) {
            key = xstrdup(full_path);
        }
    }
    if (key == NULL)
        key = xstrdup(path);
    str_to_lower_in_place(key);
    return key;
}

static bool win95_alias_key_matches(const char* left, const char* right) {
    char* left_key = win95_alias_key(left);
    char* right_key = win95_alias_key(right);
    const bool matched = left_key != NULL && right_key != NULL &&
                         strcmp(left_key, right_key) == 0;
    free(left_key);
    free(right_key);
    return matched;
}

static bool safe_dialog_file_path(const char* path, bool must_exist) {
    if (str_empty(path) || strlen(path) >= 32760 ||
        str_starts_with(path, "\\\\.\\") ||
        str_starts_with(path, "\\\\?\\GLOBALROOT\\")) return false;
    for (size_t index = 0; path[index] != '\0'; ++index) {
        if (path[index] == ':' && index != 1) return false;
    }
    const DWORD attributes = GetFileAttributesA(path);
    if (attributes != INVALID_FILE_ATTRIBUTES)
        return (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
    return !must_exist && (GetLastError() == ERROR_FILE_NOT_FOUND ||
                           GetLastError() == ERROR_PATH_NOT_FOUND);
}

static bool import_file_within_limit(const char* path) {
    if (!safe_dialog_file_path(path, true)) return false;
    WIN32_FILE_ATTRIBUTE_DATA attributes;
    memset(&attributes, 0, sizeof(attributes));
    if (!GetFileAttributesExA(path, GetFileExInfoStandard, &attributes))
        return false;
    static const ULONGLONG kMaximumImportBytes = 256ull * 1024ull * 1024ull;
    const ULONGLONG size =
        (((ULONGLONG) (attributes.nFileSizeHigh)) << 32) |
        attributes.nFileSizeLow;
    return size <= kMaximumImportBytes;
}

static bool make_win95_staging_path(char** path,
                                    const char* desired_extension) {
    if (_stricmp(desired_extension, ".DOC") != 0 &&
        _stricmp(desired_extension, ".TXT") != 0) return false;
    if (str_empty(g_win95_staging_directory)) {
        const char* temporary_root = "C:\\build\\OPUSTMP";
        if (!CreateDirectoryA("C:\\build", NULL) &&
            GetLastError() != ERROR_ALREADY_EXISTS) {
            return false;
        }
        if (!CreateDirectoryA(temporary_root, NULL) &&
            GetLastError() != ERROR_ALREADY_EXISTS) {
            return false;
        }
        const DWORD root_attributes = GetFileAttributesA(temporary_root);
        if (root_attributes == INVALID_FILE_ATTRIBUTES ||
            (root_attributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0) {
            return false;
        }
        for (int attempt = 0; attempt < 32; ++attempt) {
            GUID identifier;
            char leaf[9];
            char* candidate;
            memset(&identifier, 0, sizeof(identifier));
            memset(leaf, 0, sizeof(leaf));
            if (FAILED(CoCreateGuid(&identifier))) return false;
            _snprintf_s(leaf, sizeof(leaf), _TRUNCATE, "W%07lX",
                        identifier.Data1 & 0x0ffffffful);
            candidate = str_join3(temporary_root, "\\", leaf);
            if (candidate == NULL) return false;

            /* The original normalizer requires DOS 8.3-safe path components
             * even though the native file APIs support long names.
             */
            if (strlen(candidate) >= 52) {
                free(candidate);
                return false;
            }
            if (CreateDirectoryA(candidate, NULL)) {
                const DWORD attributes = GetFileAttributesA(candidate);
                if (attributes == INVALID_FILE_ATTRIBUTES ||
                    (attributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0) {
                    RemoveDirectoryA(candidate);
                    free(candidate);
                    return false;
                }
                str_set(&g_win95_staging_directory, candidate);
                free(candidate);
                break;
            }
            free(candidate);
            if (GetLastError() != ERROR_ALREADY_EXISTS) return false;
        }
        if (str_empty(g_win95_staging_directory)) return false;
    }
    static volatile LONG sequence;
    for (int attempt = 0; attempt < 32; ++attempt) {
        char leaf[18];
        char* candidate;
        memset(leaf, 0, sizeof(leaf));
        _snprintf_s(leaf, sizeof(leaf), _TRUNCATE, "\\D%07lX%s",
                    ++sequence & 0x0ffffffful,
                    desired_extension);
        candidate = str_join3(g_win95_staging_directory, leaf, "");
        if (candidate == NULL) return false;
        if (strlen(candidate) >= 120) {
            free(candidate);
            return false;
        }
        const DWORD attributes = GetFileAttributesA(candidate);
        const DWORD attribute_error = GetLastError();
        if (attributes == INVALID_FILE_ATTRIBUTES &&
            (attribute_error == ERROR_FILE_NOT_FOUND ||
             attribute_error == ERROR_PATH_NOT_FOUND)) {
            str_set(path, candidate);
            free(candidate);
            return true;
        }
        free(candidate);
    }
    return false;
}

static bool atomic_copy_file(const char* source, const char* target) {
    if (str_empty(source) || str_empty(target)) return false;
    static volatile LONG sequence;
    char* temporary = NULL;
    bool copied = false;
    for (int attempt = 0; attempt < 64; ++attempt) {
        char suffix[64];
        char* prefix;
        memset(suffix, 0, sizeof(suffix));
        _snprintf_s(suffix, sizeof(suffix), _TRUNCATE, ".word1tmp-%lu-%d",
                    (unsigned long) GetCurrentProcessId(),
                    (int) (++sequence));
        prefix = str_join3(target, suffix, "");
        if (prefix == NULL) return false;
        free(temporary);
        temporary = prefix;
        if (CopyFileA(source, temporary, TRUE)) {
            copied = true;
            break;
        }
        if (GetLastError() != ERROR_FILE_EXISTS &&
            GetLastError() != ERROR_ALREADY_EXISTS) {
            free(temporary);
            return false;
        }
    }
    if (!copied) {
        free(temporary);
        return false;
    }
    HANDLE file = CreateFileA(temporary, GENERIC_WRITE,
                              FILE_SHARE_READ, NULL, OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL, NULL);
    const bool flushed = file != INVALID_HANDLE_VALUE &&
                         FlushFileBuffers(file) != FALSE;
    if (file != INVALID_HANDLE_VALUE) CloseHandle(file);
    if (!flushed) {
        DeleteFileA(temporary);
        free(temporary);
        return false;
    }
    const DWORD attributes = GetFileAttributesA(target);
    bool committed = false;
    if (attributes != INVALID_FILE_ATTRIBUTES &&
        (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
        committed = ReplaceFileA(target, temporary, NULL,
            REPLACEFILE_WRITE_THROUGH, NULL, NULL) != FALSE;
        if (!committed) {
            committed = MoveFileExA(temporary, target,
                MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != FALSE;
        }
    } else if (attributes == INVALID_FILE_ATTRIBUTES &&
               GetLastError() == ERROR_FILE_NOT_FOUND) {
        committed = MoveFileExA(temporary, target, MOVEFILE_WRITE_THROUGH) != FALSE;
    }
    if (!committed) DeleteFileA(temporary);
    free(temporary);
    return committed;
}

static char* counted_path(const unsigned char* st_file) {
    if (st_file == NULL || st_file[0] == 0 || st_file[0] >= 120) {
        return xstrdup("");
    }
    return xstrndup((const char*) (st_file + 1), st_file[0]);
}

enum {
    kDlmInit = 0x0001,
    kDlmTerm = 0x0003,
    kDlmExit = 0x0004,
    kDlmChange = 0x0005,
    kDlmClick = 0x0006,
    kDlmDblClk = 0x0007,
    kDlmSetItemFocus = 0x000b,
    kDlmKillItemFocus = 0x000c,
    kDlmSetDialogFocus = 0x000d,
    kDlmKillDialogFocus = 0x000e,
    kDlmDialogClick = 0x0012,
    kWmCommitRibbonSelection = WM_APP + 0x352,
    kIddNewDoc = 2,
    kIddOpen = 3,
    kIddSaveAs = 4,
    kIddCharacter = 16,
    kIddApplyStyle = 23,
    kIddDefineStyle = 24,
    kIddAbout = 44,
    kCxtRibbonIconBar = 0x8005,
    kCxtRulerIconBar = 0x8006,
    kTmcOk = 1,
    kTmcCancel = 2,
    kTmcUserMin = 0x0400,
    kTmcCharacterName = kTmcUserMin,
    kTmcCharacterSize = kTmcUserMin + 2,
    kTmcCharacterColor = kTmcUserMin + 4,
    kTmcApplyStyle = kTmcUserMin,
    kTmcApplyDefine = kTmcUserMin + 2,
    kTmcApplyBanter = kTmcUserMin + 3,
    kTmcDefineStyle = kTmcUserMin,
    kTmcDefineChars = kTmcUserMin + 2,
    kTmcDefineParas = kTmcUserMin + 3,
    kTmcDefineTabs = kTmcUserMin + 4,
    kTmcDefinePosition = kTmcUserMin + 5,
    kTmcDefineOptions = kTmcUserMin + 6,
    kTmcDefineBanter = kTmcUserMin + 7,
    kTmcDefineBasedOn = kTmcUserMin + 8,
    kTmcDefineNext = kTmcUserMin + 10,
    kTmcDefineTemplate = kTmcUserMin + 12,
    kTmcDefineCommit = kTmcUserMin + 13,
    kTmcDefineDelete = kTmcUserMin + 14,
    kTmcDefineRename = kTmcUserMin + 15,
    kTmcDefineMerge = kTmcUserMin + 16,
    kTmmCount = 2,
    kTmmText = 3,
    kUnknownListCount = 0xffff,
    kTmcSummary = 0x0400,
    kTmcNewDot = 0x0401,
    kTmcRNewDoc = 0x0402,
    kTmcRNewDot = 0x0403,
    kTmcNewType = 0x0404,
    kTmcNewTypeList = 0x0405,
    kTmcOpenFileName = 0x0400,
    kTmcOpenFileList = 0x0401,
    kTmcOpenFileDir = 0x0402,
    kTmcOpenCatalog = 0x0403,
    kTmcOpenReadOnly = 0x0404,
    kTmcSaveFile = kTmcUserMin,
    kTmcSaveOptions = kTmcUserMin + 1,
    kTmcSaveDirectoryText = kTmcUserMin + 2,
    kTmcSaveDirectoryList = kTmcUserMin + 3,
    kTmcSaveDirectory = kTmcUserMin + 4,
    kTmcSaveFormatPrompt = kTmcUserMin + 5,
    kTmcSaveFormat = kTmcUserMin + 6,
    kSaveFormatRtf = 6,
    kTmcSaveQuick = kTmcUserMin + 7,
    kTmcSaveBackup = kTmcUserMin + 8,
    kTmcSaveLockAnnotations = kTmcUserMin + 9,
    kOpenFileNameIag = 1
};
static const size_t kOpenCabBytes = 24;
static const size_t kOpenReadOnlyOffset = 20;
static const Word kNewTypeIag = 1;
static const size_t kNewCabBytes = 24;
static const size_t kNewDotOffset = 16;

static void set_sds_handle(Hdlg current, Hdlg focus);
LRESULT CALLBACK native_dialog_window_proc(HWND, UINT, WPARAM, LPARAM);
static void handle_dialog_command(Hdlg, WPARAM, LPARAM);

static DialogState* active_dialog(void) {
    DialogState* dialog = find_dialog(g_current_dialog);
    return dialog == NULL ? &g_no_dialog : dialog;
}

#define g_dialog (*active_dialog())

static ControlState* control(Tmc tmc) {
    return dialog_control(active_dialog(), tmc);
}

static const ControlState* find_control(Tmc tmc) {
    return dialog_find_control_const(active_dialog(), tmc);
}

static void sync_current_dialog_globals(void) {
    const DialogState* dialog = find_dialog(g_current_dialog);
    hcabDlgCur = dialog == NULL ? NULL : dialog->cab;
    wRefDlgCur = dialog == NULL ? 0 : dialog->reference;
    set_sds_handle(g_current_dialog, g_focus_dialog);
}

int scaled_x(const int value) {
    return MulDiv(value, dac.dxSysFontChar == 0 ? 8 : dac.dxSysFontChar, 4);
}

int scaled_y(const int value) {
    return MulDiv(value, dac.dySysFontChar == 0 ? 16 : dac.dySysFontChar, 8);
}

static ATOM ensure_native_dialog_class(void) {
    static ATOM atom = 0;
    if (atom != 0) {
        return atom;
    }
    WNDCLASSEXA cls;
    memset(&cls, 0, sizeof(cls));
    cls.cbSize = sizeof(cls);
    cls.style = CS_DBLCLKS;
    cls.lpfnWndProc = native_dialog_window_proc;
    cls.hInstance = GetModuleHandleW(NULL);
    cls.hCursor = LoadCursorA(NULL, IDC_ARROW);
    cls.hbrBackground = ((HBRUSH) (COLOR_BTNFACE + 1));
    cls.lpszClassName = "OpusSdmDialog";
    atom = RegisterClassExA(&cls);
    if (atom == 0 && GetLastError() == ERROR_CLASS_ALREADY_EXISTS) {
        atom = 1;
    }
    return atom;
}

static HWND create_dialog_host(const DialogState* dialog,
                               const Dli* initializer) {
    if (dialog->modal &&
        (dialog->hid == kIddOpen || dialog->hid == kIddNewDoc ||
         dialog->hid == kIddSaveAs || dialog->hid == kIddAbout ||
         dialog->hid == kIddCharacter || dialog->hid == kIddApplyStyle ||
         dialog->hid == kIddDefineStyle)) {
        if (ensure_native_dialog_class() == 0) {
            return NULL;
        }

        HWND owner = initializer != NULL ? initializer->hwnd : NULL;
        if (owner == NULL || !IsWindow(owner)) {
            owner = vhWndMsgBoxParent;
        }
        if (owner == NULL || !IsWindow(owner)) {
            owner = GetActiveWindow();
        }

        int client_width = scaled_x(dialog->hid == kIddNewDoc ? 127 : 206);
        int client_height = scaled_y(dialog->hid == kIddNewDoc ? 114 : 104);
        if (dialog->template_handle != NULL &&
            *dialog->template_handle != NULL) {
            client_width = max_int(1, scaled_x(
                (*dialog->template_handle)->rec.dx));
            client_height = max_int(1, scaled_y(
                (*dialog->template_handle)->rec.dy));
        }
        static const DWORD style =
            WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_CLIPCHILDREN;
        static const DWORD extended_style =
            WS_EX_DLGMODALFRAME | WS_EX_CONTROLPARENT;
        RECT window_rect = {0, 0, client_width, client_height};
        AdjustWindowRectEx(&window_rect, style, false, extended_style);
        const int width = window_rect.right - window_rect.left;
        const int height = window_rect.bottom - window_rect.top;

        RECT anchor;
        memset(&anchor, 0, sizeof(anchor));
        if (owner == NULL || !GetWindowRect(owner, &anchor)) {
            SystemParametersInfoA(SPI_GETWORKAREA, 0, &anchor, 0);
        }
        const int x = anchor.left +
                      (anchor.right - anchor.left - width) / 2;
        const int y = anchor.top +
                      (anchor.bottom - anchor.top - height) / 2;
        const char* caption = "Open";
        switch (dialog->hid) {
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
            height, owner, NULL, GetModuleHandleW(NULL),
            (void*) ((uintptr_t) dialog->handle));
    }

    if (initializer == NULL || initializer->hwnd == NULL ||
        !IsWindow(initializer->hwnd)) {
        return NULL;
    }
    if (ensure_native_dialog_class() == 0) {
        return NULL;
    }

    int x = initializer->dx;
    int y = initializer->dy;
    int width = 1;
    int height = 1;
    if (dialog->template_handle != NULL &&
        *dialog->template_handle != NULL) {
        const Rec* rec = &(*dialog->template_handle)->rec;
        x += scaled_x(rec->x);
        y += scaled_y(rec->y);
        width = max_int(1, scaled_x(rec->dx));
        height = max_int(1, scaled_y(rec->dy));
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
        initializer->hwnd, NULL, GetModuleHandleW(NULL),
        (void*) ((uintptr_t) dialog->handle));
}

static HWND create_native_control(DialogState* dialog, Tmc tmc,
                           const char* window_class, const char* caption,
                           Rec rectangle, DWORD control_style) {
    ControlState* state = dialog_control(dialog, tmc);
    if (_stricmp(window_class, "BUTTON") == 0) {
        state->kind = CONTROL_KIND_BUTTON;
    } else if (_stricmp(window_class, "COMBOBOX") == 0) {
        state->kind = CONTROL_KIND_COMBO;
    } else if (_stricmp(window_class, "EDIT") == 0) {
        state->kind = CONTROL_KIND_EDIT;
    } else if (_stricmp(window_class, "LISTBOX") == 0) {
        state->kind = CONTROL_KIND_LIST;
    } else if (_stricmp(window_class, "STATIC") == 0) {
        state->kind = CONTROL_KIND_STATIC;
    } else {
        state->kind = CONTROL_KIND_UNKNOWN;
    }
    state->rectangle = rectangle;
    str_set(&state->text, caption == NULL ? "" : caption);
    state->visible = (control_style & WS_VISIBLE) != 0 ||
                    (control_style & WS_CHILD) == 0;
    return state->window;
}

static void create_static_text(DialogState* dialog, const char* caption,
                               Rec rectangle) {
    ControlState state;
    control_init(&state);
    state.kind = CONTROL_KIND_STATIC;
    state.rectangle = rectangle;
    str_set(&state.text, caption == NULL ? "" : caption);
    if (!dialog_push_untracked(dialog, &state)) {
        control_free(&state);
    }
}

static void create_untracked_control(DialogState* dialog,
                                     const char* window_class,
                                     const char* caption, Rec rectangle,
                                     DWORD control_style) {
    ControlState state;
    control_init(&state);
    if (_stricmp(window_class, "BUTTON") == 0) {
        state.kind = CONTROL_KIND_BUTTON;
    } else if (_stricmp(window_class, "STATIC") == 0) {
        state.kind = CONTROL_KIND_STATIC;
    }
    state.rectangle = rectangle;
    str_set(&state.text, caption == NULL ? "" : caption);
    state.visible = (control_style & WS_VISIBLE) != 0 ||
                    (control_style & WS_CHILD) == 0;
    if (!dialog_push_untracked(dialog, &state)) {
        control_free(&state);
    }
}

static char* join_path(const char* directory, const char* leaf) {
    char separator[2] = "\\";
    size_t length;
    if (str_empty(directory)) {
        return xstrdup(leaf);
    }
    length = strlen(directory);
    if (directory[length - 1] == '\\' || directory[length - 1] == '/') {
        separator[0] = '\0';
    }
    return str_join3(directory, separator, leaf);
}

static void add_native_list_entry(DialogState* dialog, Tmc tmc,
                                  const char* entry) {
    ControlState* state = dialog_control(dialog, tmc);
    string_list_push(&state->entries, entry);
}

static void set_open_directory_label(DialogState* dialog) {
    str_set(&dialog_control(dialog, kTmcOpenFileDir)->text,
            dialog->current_directory);
}

static void establish_open_directory(DialogState* dialog,
                                     const char* specification) {
    char full_path[32768];
    char* file_part = NULL;
    const char* source = str_empty(specification) ? "*.*" : specification;
    memset(full_path, 0, sizeof(full_path));
    const DWORD length = GetFullPathNameA(
        source, (DWORD) sizeof(full_path), full_path, &file_part);
    if (length == 0 || length >= sizeof(full_path)) {
        GetCurrentDirectoryA((DWORD) sizeof(full_path), full_path);
        str_set(&dialog->current_directory, full_path);
        str_set(&dialog->file_pattern, "*.*");
        return;
    }

    const DWORD attributes = GetFileAttributesA(full_path);
    if (attributes != INVALID_FILE_ATTRIBUTES &&
        (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
        str_set(&dialog->current_directory, full_path);
        str_set(&dialog->file_pattern, "*.*");
        return;
    }

    str_set(&dialog->file_pattern,
            file_part == NULL || *file_part == '\0' ? "*.*" : file_part);
    if (file_part == NULL || file_part == full_path) {
        GetCurrentDirectoryA((DWORD) sizeof(full_path), full_path);
        str_set(&dialog->current_directory, full_path);
    } else {
        size_t directory_length =
            ((size_t) (file_part - full_path));
        while (directory_length > 3 &&
               (full_path[directory_length - 1] == '\\' ||
                full_path[directory_length - 1] == '/')) {
            --directory_length;
        }
        str_set_n(&dialog->current_directory, full_path, directory_length);
    }
}

static void populate_open_lists(DialogState* dialog) {
    ControlState* files = dialog_control(dialog, kTmcOpenFileList);
    ControlState* directories = dialog_control(dialog, kTmcOpenFileDir);
    WIN32_FIND_DATAA find_data;
    char* spec;
    HANDLE find;
    string_list_clear(&files->entries);
    string_list_clear(&directories->entries);

    memset(&find_data, 0, sizeof(find_data));
    spec = join_path(dialog->current_directory, dialog->file_pattern);
    find = FindFirstFileA(spec, &find_data);
    free(spec);
    if (find != INVALID_HANDLE_VALUE) {
        do {
            if ((find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
                add_native_list_entry(dialog, kTmcOpenFileList,
                                      find_data.cFileName);
            }
        } while (FindNextFileA(find, &find_data));
        FindClose(find);
    }

    char parent_path[32768];
    char* parent_spec = join_path(dialog->current_directory, "..");
    memset(parent_path, 0, sizeof(parent_path));
    if (GetFullPathNameA(parent_spec, (DWORD) sizeof(parent_path),
                         parent_path, NULL) != 0 &&
        _stricmp(parent_path, dialog->current_directory) != 0) {
        add_native_list_entry(dialog, kTmcOpenFileDir, "[..]");
    }
    free(parent_spec);
    spec = join_path(dialog->current_directory, "*.*");
    find = FindFirstFileA(spec, &find_data);
    free(spec);
    if (find != INVALID_HANDLE_VALUE) {
        do {
            if ((find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0 &&
                strcmp(find_data.cFileName, ".") != 0 &&
                strcmp(find_data.cFileName, "..") != 0) {
                char* entry = str_join3("[", find_data.cFileName, "]");
                add_native_list_entry(
                    dialog, kTmcOpenFileDir, entry);
                free(entry);
            }
        } while (FindNextFileA(find, &find_data));
        FindClose(find);
    }
    set_open_directory_label(dialog);
}

static void read_open_cab(DialogState* dialog) {
    char file_name[32768];
    ControlState* edit;
    memset(file_name, 0, sizeof(file_name));
    if (dialog->cab != NULL) {
        GetCabSz(dialog->cab, file_name,
                 (Word) min_size(sizeof(file_name), 0xffffu),
                 kOpenFileNameIag);
    }
    edit = dialog_control(dialog, kTmcOpenFileName);
    str_set(&edit->text, file_name);

    if (dialog->cab != NULL && *dialog->cab != NULL &&
        OpusCbOfH(dialog->cab) >= kOpenCabBytes) {
        int read_only = 0;
        memcpy(&read_only,
                    ((const unsigned char*) (*dialog->cab)) +
                        kOpenReadOnlyOffset,
                    sizeof(read_only));
        dialog_control(dialog, kTmcOpenReadOnly)->value = read_only != 0;
    }
    establish_open_directory(dialog, edit->text);
    populate_open_lists(dialog);
}

static void sync_open_cab(DialogState* dialog) {
    ControlState* edit = dialog_find_control(dialog, kTmcOpenFileName);
    ControlState* checkbox = dialog_find_control(dialog, kTmcOpenReadOnly);
    if (dialog->cab != NULL && edit != NULL) {
        FSetCabSz(dialog->cab, edit->text, kOpenFileNameIag);
    }

    if (dialog->cab != NULL && *dialog->cab != NULL &&
        OpusCbOfH(dialog->cab) >= kOpenCabBytes &&
        checkbox != NULL) {
        const int read_only = checkbox->value != 0;
        memcpy(((unsigned char*) (*dialog->cab)) +
                        kOpenReadOnlyOffset,
                    &read_only, sizeof(read_only));
    }
}

static void populate_new_type_list(DialogState* dialog) {
    ControlState* list = dialog_control(dialog, kTmcNewTypeList);
    ControlState* edit = dialog_control(dialog, kTmcNewType);
    WIN32_FIND_DATAA find_data;
    string_list_clear(&list->entries);

    add_native_list_entry(dialog, kTmcNewTypeList,
                          str_empty(edit->text) ? "NORMAL" : edit->text);

    memset(&find_data, 0, sizeof(find_data));
    HANDLE find = FindFirstFileA("*.DOT", &find_data);
    if (find == INVALID_HANDLE_VALUE) {
        return;
    }
    do {
        if ((find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
            continue;
        }
        if (!string_list_contains_i(&list->entries, find_data.cFileName)) {
            add_native_list_entry(dialog, kTmcNewTypeList,
                                  find_data.cFileName);
        }
    } while (FindNextFileA(find, &find_data));
    FindClose(find);
}

static void read_new_cab(DialogState* dialog) {
    char type_name[32768];
    ControlState* edit;
    bool new_template = false;
    memset(type_name, 0, sizeof(type_name));
    if (dialog->cab != NULL) {
        GetCabSz(dialog->cab, type_name,
                 (Word) min_size(sizeof(type_name), 0xffffu),
                 kNewTypeIag);
    }
    edit = dialog_control(dialog, kTmcNewType);
    str_set(&edit->text, type_name);

    if (dialog->cab != NULL && *dialog->cab != NULL &&
        OpusCbOfH(dialog->cab) >= kNewCabBytes) {
        int value = 0;
        memcpy(&value,
                    ((const unsigned char*) (*dialog->cab)) +
                        kNewDotOffset,
                    sizeof(value));
        new_template = value != 0;
    }
    dialog_control(dialog, kTmcNewDot)->value = new_template;
    dialog_control(dialog, kTmcRNewDoc)->value = !new_template;
    dialog_control(dialog, kTmcRNewDot)->value = new_template;
    populate_new_type_list(dialog);
}

static void sync_new_cab(DialogState* dialog) {
    ControlState* edit = dialog_find_control(dialog, kTmcNewType);
    ControlState* radio = dialog_find_control(dialog, kTmcRNewDot);
    bool new_template = false;
    if (dialog->cab != NULL && edit != NULL) {
        FSetCabSz(dialog->cab, edit->text, kNewTypeIag);
    }

    if (radio != NULL) {
        new_template = radio->value != 0;
    }
    dialog_control(dialog, kTmcNewDot)->value = new_template;
    if (dialog->cab != NULL && *dialog->cab != NULL &&
        OpusCbOfH(dialog->cab) >= kNewCabBytes) {
        const int value = new_template;
        memcpy(((unsigned char*) (*dialog->cab)) + kNewDotOffset,
                    &value, sizeof(value));
    }
}

static void materialize_new_template(DialogState* dialog) {
    if (dialog->hid != kIddNewDoc || dialog->window == NULL) {
        return;
    }
    create_native_control(dialog, kTmcOk, "BUTTON", "OK", (Rec){74, 6, 46, 14},
                          WS_TABSTOP | BS_DEFPUSHBUTTON);
    create_native_control(dialog, kTmcCancel, "BUTTON", "Cancel",
                          (Rec){74, 23, 46, 14}, WS_TABSTOP | BS_PUSHBUTTON);
    create_native_control(dialog, kTmcSummary, "BUTTON", "&Summary...",
                          (Rec){74, 39, 46, 14}, WS_TABSTOP | BS_PUSHBUTTON);
    create_untracked_control(dialog, "BUTTON", "New", (Rec){5, 2, 61, 36},
                             BS_GROUPBOX);
    create_native_control(dialog, kTmcRNewDoc, "BUTTON", "&Document",
                          (Rec){9, 12, 42, 10},
                          WS_TABSTOP | WS_GROUP | BS_AUTORADIOBUTTON);
    create_native_control(dialog, kTmcRNewDot, "BUTTON", "&Template",
                          (Rec){9, 24, 42, 10},
                          WS_TABSTOP | BS_AUTORADIOBUTTON);
    create_static_text(dialog, "&Use Template:", (Rec){5, 42, 53, 10});
    create_native_control(dialog, kTmcNewType, "EDIT", "",
                          (Rec){5, 53, 61, 12},
                          WS_TABSTOP | WS_BORDER | ES_AUTOHSCROLL);
    create_native_control(dialog, kTmcNewTypeList, "LISTBOX", "",
                          (Rec){8, 65, 68, 40},
                          WS_TABSTOP | WS_BORDER | WS_VSCROLL | LBS_NOTIFY |
                              LBS_SORT | LBS_NOINTEGRALHEIGHT);
    str_set(&dialog->caption, "New");
    dialog->native_modal = true;
    read_new_cab(dialog);
}

static void materialize_open_template(DialogState* dialog) {
    if (dialog->hid != kIddOpen || dialog->window == NULL) {
        return;
    }
    create_native_control(dialog, kTmcOpenFileName, "EDIT", "",
                          (Rec){67, 6, 83, 12},
                          WS_TABSTOP | WS_BORDER | ES_AUTOHSCROLL);
    create_static_text(dialog, "Open File &Name:", (Rec){5, 7, 61, 9});
    create_native_control(dialog, kTmcOpenFileList, "LISTBOX", "",
                          (Rec){5, 32, 60, 64},
                          WS_TABSTOP | WS_BORDER | WS_VSCROLL | LBS_NOTIFY |
                              LBS_SORT | LBS_NOINTEGRALHEIGHT);
    create_static_text(dialog, "&Files:", (Rec){5, 21, 25, 9});
    create_native_control(dialog, kTmcOpenFileDir, "LISTBOX", "",
                          (Rec){71, 48, 65, 48},
                          WS_TABSTOP | WS_BORDER | WS_VSCROLL | LBS_NOTIFY |
                              LBS_SORT | LBS_NOINTEGRALHEIGHT);
    create_static_text(dialog, "&Directories:", (Rec){71, 38, 50, 9});
    create_untracked_control(dialog, "STATIC", "", (Rec){70, 24, 78, 8}, SS_LEFT);
    create_native_control(dialog, kTmcOk, "BUTTON", "OK",
                          (Rec){154, 5, 47, 14},
                          WS_TABSTOP | BS_DEFPUSHBUTTON);
    create_native_control(dialog, kTmcCancel, "BUTTON", "Cancel",
                          (Rec){154, 22, 47, 14}, WS_TABSTOP | BS_PUSHBUTTON);
    create_native_control(dialog, kTmcOpenCatalog, "BUTTON", "F&ind...",
                          (Rec){154, 39, 47, 14}, WS_TABSTOP | BS_PUSHBUTTON);
    create_native_control(dialog, kTmcOpenReadOnly, "BUTTON", "&Read Only",
                          (Rec){155, 56, 47, 12},
                          WS_TABSTOP | BS_AUTOCHECKBOX);
    str_set(&dialog->caption, "Open");
    dialog->native_modal = true;
    read_open_cab(dialog);
}

static char* read_cab_string(const DialogState* dialog, Word iag) {
    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));
    if (dialog->cab != NULL) {
        GetCabSz(dialog->cab, buffer, (Word) sizeof(buffer), iag);
    }
    return xstrdup(buffer);
}

static void static_from_cab(DialogState* dialog, Word iag, Rec rec,
                            DWORD alignment) {
    char* text = read_cab_string(dialog, iag);
    create_untracked_control(dialog, "STATIC", text, rec, alignment);
    free(text);
}

static void materialize_about_template(DialogState* dialog) {
    if (dialog->hid != kIddAbout || dialog->window == NULL) {
        return;
    }

    static_from_cab(dialog, 1, (Rec){4, 5, 192, 9}, SS_CENTER);
    static_from_cab(dialog, 2, (Rec){4, 22, 192, 9}, SS_CENTER);
    static_from_cab(dialog, 3, (Rec){4, 34, 192, 9}, SS_CENTER);
    create_native_control(dialog, kTmcOk, "BUTTON", "OK",
                          (Rec){84, 48, 34, 14},
                          WS_TABSTOP | BS_DEFPUSHBUTTON);
    create_untracked_control(dialog, "STATIC", "", (Rec){0, 64, 205, 4},
                             SS_ETCHEDHORZ);
    create_untracked_control(dialog, "STATIC", "Conventional Memory:",
                             (Rec){5, 74, 110, 9}, SS_RIGHT);
    create_untracked_control(dialog, "STATIC", "Expanded Memory:",
                             (Rec){5, 83, 110, 9}, SS_RIGHT);
    create_untracked_control(dialog, "STATIC", "Math Co-processor:",
                             (Rec){5, 92, 110, 9}, SS_RIGHT);
    create_untracked_control(dialog, "STATIC", "Disk Space:",
                             (Rec){5, 101, 110, 9}, SS_RIGHT);
    static_from_cab(dialog, 4, (Rec){120, 74, 76, 9}, SS_LEFT);
    static_from_cab(dialog, 5, (Rec){120, 83, 76, 9}, SS_LEFT);
    static_from_cab(dialog, 6, (Rec){120, 92, 76, 9}, SS_LEFT);
    static_from_cab(dialog, 7, (Rec){120, 101, 76, 9}, SS_LEFT);
    str_set(&dialog->caption, "About Microsoft Word");
    dialog->native_modal = true;
}

typedef struct CabSaveNative {
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
} CabSaveNative;

static CabSaveNative* save_cab(DialogState* dialog) {
    if (dialog->cab == NULL || *dialog->cab == NULL ||
        OpusCbOfH(dialog->cab) < sizeof(CabSaveNative)) {
        return NULL;
    }
    return (CabSaveNative*) (*dialog->cab);
}

static void set_save_check(DialogState* dialog, Tmc tmc, bool checked) {
    dialog_control(dialog, tmc)->value = checked;
}

static void populate_save_directories(DialogState* dialog) {
    char directory[32768];
    ControlState* list;
    WIN32_FIND_DATAA find_data;
    char* spec;
    HANDLE find;
    memset(directory, 0, sizeof(directory));
    if (str_empty(dialog->current_directory)) {
        GetCurrentDirectoryA((DWORD) sizeof(directory), directory);
        str_set(&dialog->current_directory, directory);
    }

    list = dialog_control(dialog, kTmcSaveDirectoryList);
    string_list_clear(&list->entries);

    char parent[32768];
    char* parent_spec = join_path(dialog->current_directory, "..");
    memset(parent, 0, sizeof(parent));
    if (GetFullPathNameA(parent_spec, (DWORD) sizeof(parent), parent,
                         NULL) != 0 &&
        _stricmp(parent, dialog->current_directory) != 0) {
        add_native_list_entry(dialog, kTmcSaveDirectoryList, "[..]");
    }
    free(parent_spec);

    memset(&find_data, 0, sizeof(find_data));
    spec = join_path(dialog->current_directory, "*.*");
    find = FindFirstFileA(spec, &find_data);
    free(spec);
    if (find != INVALID_HANDLE_VALUE) {
        do {
            if ((find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0 &&
                strcmp(find_data.cFileName, ".") != 0 &&
                strcmp(find_data.cFileName, "..") != 0) {
                char* entry = str_join3("[", find_data.cFileName, "]");
                add_native_list_entry(
                    dialog, kTmcSaveDirectoryList, entry);
                free(entry);
            }
        } while (FindNextFileA(find, &find_data));
        FindClose(find);
    }

    str_set(&dialog_control(dialog, kTmcSaveDirectory)->text,
            dialog->current_directory);
}

static void set_save_options_visible(DialogState* dialog, bool visible) {
    const Tmc option_controls[] = {
        kTmcSaveFormatPrompt, kTmcSaveFormat, kTmcSaveQuick,
        kTmcSaveBackup, kTmcSaveLockAnnotations};
    for (size_t index = 0; index < sizeof(option_controls) / sizeof(option_controls[0]);
         ++index) {
        ControlState* found = dialog_find_control(dialog, option_controls[index]);
        if (found != NULL) {
            found->visible = visible;
        }
    }
    if (dialog->window == NULL || !IsWindow(dialog->window)) {
        return;
    }
    RECT client = {0, 0, scaled_x(150), scaled_y(visible ? 157 : 102)};
    const DWORD style = (DWORD) GetWindowLongPtrA(dialog->window, GWL_STYLE);
    const DWORD extended_style = (DWORD) GetWindowLongPtrA(
        dialog->window, GWL_EXSTYLE);
    AdjustWindowRectEx(&client, style, false, extended_style);
    SetWindowPos(dialog->window, NULL, 0, 0,
                 client.right - client.left, client.bottom - client.top,
                 SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
}

static void read_save_cab(DialogState* dialog) {
    ControlState* edit = dialog_control(dialog, kTmcSaveFile);
    char* text = read_cab_string(dialog, 1);
    CabSaveNative* cab;
    str_set(&edit->text, text);
    free(text);

    cab = save_cab(dialog);
    if (cab != NULL) {
        set_save_check(dialog, kTmcSaveQuick, cab->quick_save != 0);
        set_save_check(dialog, kTmcSaveBackup, cab->backup != 0);
        set_save_check(dialog, kTmcSaveLockAnnotations,
                       cab->lock_annotations != 0);
        ControlState* format = dialog_control(dialog, kTmcSaveFormat);
        format->value = (Word) cab->format;
        string_list_clear(&format->entries);
        string_list_push(&format->entries, "Current document format");
    }
    populate_save_directories(dialog);
}

static int control_checked(DialogState* dialog, Tmc tmc) {
    ControlState* state = dialog_find_control(dialog, tmc);
    return state != NULL && state->value != 0;
}

static void sync_save_cab(DialogState* dialog) {
    ControlState* edit = dialog_control(dialog, kTmcSaveFile);
    CabSaveNative* cab;
    if (dialog->cab != NULL) {
        FSetCabSz(dialog->cab, edit->text, 1);
    }
    cab = save_cab(dialog);
    if (cab != NULL) {
        ControlState* format = dialog_control(dialog, kTmcSaveFormat);
        cab->quick_save = control_checked(dialog, kTmcSaveQuick);
        cab->backup = control_checked(dialog, kTmcSaveBackup);
        cab->lock_annotations = control_checked(dialog, kTmcSaveLockAnnotations);
        cab->options = dialog->sab == 0 && format->visible;
    }
}

static void materialize_save_as_template(DialogState* dialog) {
    if (dialog->hid != kIddSaveAs || dialog->window == NULL) {
        return;
    }
    create_static_text(dialog, "Save File &Name:", (Rec){4, 2, 60, 9});
    create_native_control(dialog, kTmcSaveFile, "EDIT", "",
                          (Rec){4, 13, 81, 12},
                          WS_TABSTOP | WS_BORDER | ES_AUTOHSCROLL);
    create_native_control(dialog, kTmcOk, "BUTTON", "OK", (Rec){97, 6, 47, 14},
                          WS_TABSTOP | BS_DEFPUSHBUTTON);
    create_native_control(dialog, kTmcCancel, "BUTTON", "Cancel",
                          (Rec){97, 25, 47, 14}, WS_TABSTOP | BS_PUSHBUTTON);
    create_native_control(dialog, kTmcSaveOptions, "BUTTON", "&Options >>",
                          (Rec){97, 42, 47, 14}, WS_TABSTOP | BS_PUSHBUTTON);
    create_native_control(dialog, kTmcSaveDirectoryText, "STATIC",
                          "&Directories:", (Rec){4, 38, 73, 9}, SS_LEFT);
    create_native_control(dialog, kTmcSaveDirectoryList, "LISTBOX", "",
                          (Rec){4, 50, 89, 48},
                          WS_TABSTOP | WS_BORDER | WS_VSCROLL | LBS_NOTIFY |
                              LBS_SORT | LBS_NOINTEGRALHEIGHT);
    create_native_control(dialog, kTmcSaveDirectory, "STATIC", "",
                          (Rec){4, 27, 90, 9}, SS_LEFT);
    create_native_control(dialog, kTmcSaveFormatPrompt, "STATIC",
                          "&File Format:", (Rec){4, 105, 50, 9}, SS_LEFT);
    create_native_control(dialog, kTmcSaveFormat, "COMBOBOX", "",
                          (Rec){55, 103, 91, 72},
                          WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWNLIST);
    create_native_control(dialog, kTmcSaveQuick, "BUTTON", "Fast &Save",
                          (Rec){4, 116, 46, 12},
                          WS_TABSTOP | BS_AUTOCHECKBOX);
    create_native_control(dialog, kTmcSaveBackup, "BUTTON", "Create &Backup",
                          (Rec){4, 128, 62, 12},
                          WS_TABSTOP | BS_AUTOCHECKBOX);
    create_native_control(dialog, kTmcSaveLockAnnotations, "BUTTON",
                          "&Lock for Annotations", (Rec){4, 140, 92, 12},
                          WS_TABSTOP | BS_AUTOCHECKBOX);
    str_set(&dialog->caption, "Save As");
    dialog->native_modal = true;
    read_save_cab(dialog);
    set_save_options_visible(dialog, false);
}

static unsigned int preview_color(ControlKind kind) {
    switch (kind) {
        case CONTROL_KIND_BUTTON:
            return 0xffc0c0c0u;
        case CONTROL_KIND_EDIT:
            return 0xffffffffu;
        case CONTROL_KIND_LIST:
            return 0xfff8f8f8u;
        case CONTROL_KIND_COMBO:
            return 0xffe8f0f8u;
        case CONTROL_KIND_STATIC:
            return 0xfff0f0f0u;
        case CONTROL_KIND_UNKNOWN:
            break;
    }
    return 0xffd0d0d0u;
}

static void preview_set_pixel(unsigned int* pixels, int width, int height,
                              int x, int y, unsigned int color) {
    if (x < 0 || y < 0 || x >= width || y >= height) {
        return;
    }
    pixels[y * width + x] = color;
}

static void preview_fill_rect(unsigned int* pixels, int width, int height,
                              Rec rec, unsigned int color) {
    const int left = max_int(0, rec.x);
    const int top = max_int(0, rec.y);
    const int right = min_int(width, rec.x + max_int(1, rec.dx));
    const int bottom = min_int(height, rec.y + max_int(1, rec.dy));
    for (int y = top; y < bottom; ++y) {
        for (int x = left; x < right; ++x) {
            pixels[y * width + x] = color;
        }
    }
}

static void preview_frame_rect(unsigned int* pixels, int width, int height,
                               Rec rec, unsigned int color) {
    const int right = rec.x + max_int(1, rec.dx) - 1;
    const int bottom = rec.y + max_int(1, rec.dy) - 1;
    for (int x = rec.x; x <= right; ++x) {
        preview_set_pixel(pixels, width, height, x, rec.y, color);
        preview_set_pixel(pixels, width, height, x, bottom, color);
    }
    for (int y = rec.y; y <= bottom; ++y) {
        preview_set_pixel(pixels, width, height, rec.x, y, color);
        preview_set_pixel(pixels, width, height, right, y, color);
    }
}

static void preview_draw_text_mark(unsigned int* pixels, int width,
                                   int height, Rec rec, const char* text) {
    if (str_empty(text)) {
        return;
    }
    if (strchr(text, '/') != NULL || strchr(text, '\\') != NULL) {
        return;
    }
    if (strlen(text) >= 2 &&
        isalpha(((unsigned char) (text[0]))) && text[1] == ':') {
        return;
    }
    unsigned int hash = 2166136261u;
    for (const unsigned char* scan = (const unsigned char*) text; *scan != '\0';
         ++scan) {
        const unsigned char ch = *scan;
        if (ch == '&') {
            continue;
        }
        hash ^= ch;
        hash *= 16777619u;
    }
    const int y = rec.y + rec.dy / 2;
    const int count = min_int(rec.dx - 2, 3 + ((int) (hash % 9u)));
    for (int index = 0; index < count; ++index) {
        const unsigned int bit = (hash >> (index % 16)) & 1u;
        preview_set_pixel(pixels, width, height, rec.x + 1 + index, y,
                          bit ? 0xff000000u : 0xff606060u);
    }
}

static void preview_render_control(unsigned int* pixels, int width,
                                   int height, const ControlState* control) {
    if (!control->visible) {
        return;
    }
    preview_fill_rect(pixels, width, height, control->rectangle,
                      preview_color(control->kind));
    preview_frame_rect(pixels, width, height, control->rectangle, 0xff000000u);
    preview_draw_text_mark(pixels, width, height, control->rectangle,
                           control->text);
}

static int compare_control_entries(const void* left, const void* right) {
    const ControlEntry* const* left_entry = (const ControlEntry* const*) left;
    const ControlEntry* const* right_entry = (const ControlEntry* const*) right;
    return (*left_entry)->key < (*right_entry)->key ? -1 :
        (*left_entry)->key > (*right_entry)->key;
}

static int render_dialog_preview(Word hid, unsigned int* pixels,
                                 int width, int height) {
    if (pixels == NULL || width <= 0 || height <= 0) {
        return 0;
    }
    DialogState dialog;
    dialog_init(&dialog);
    dialog.hid = hid;
    dialog.modal = true;
    dialog.window = create_dialog_host(&dialog, NULL);
    if (dialog.window == NULL) {
        dialog_free(&dialog);
        return 0;
    }
    dialog.visible = true;

    materialize_save_as_template(&dialog);
    materialize_about_template(&dialog);
    if (dialog.control_count == 0 && dialog.untracked_count == 0) {
        DestroyWindow(dialog.window);
        dialog_free(&dialog);
        return 0;
    }

    for (size_t index = 0; index < ((size_t) width) * (size_t) height; ++index) {
        pixels[index] = 0xffffffffu;
    }
    preview_frame_rect(pixels, width, height, (Rec){0, 0, width, height},
                       0xff000000u);

    for (size_t index = 0; index < dialog.untracked_count; ++index) {
        preview_render_control(pixels, width, height,
                               &dialog.untracked_controls[index]);
    }

    ControlEntry** controls = (ControlEntry**) malloc(
        dialog.control_count * sizeof(controls[0]));
    if (controls != NULL) {
        for (size_t index = 0; index < dialog.control_count; ++index) {
            controls[index] = &dialog.controls[index];
        }
        qsort(controls, dialog.control_count, sizeof(controls[0]),
              compare_control_entries);
        for (size_t index = 0; index < dialog.control_count; ++index) {
            preview_render_control(pixels, width, height,
                                   &controls[index]->value);
        }
        free(controls);
    }
    DestroyWindow(dialog.window);
    dialog_free(&dialog);
    return 1;
}

static bool is_font_name_control(const DialogState* dialog, Tmc tmc) {
    return (dialog->hid == kCxtRibbonIconBar && tmc == kTmcUserMin) ||
           (dialog->hid == kIddCharacter && tmc == kTmcCharacterName);
}

static bool is_font_size_control(const DialogState* dialog, Tmc tmc) {
    return (dialog->hid == kCxtRibbonIconBar && tmc == kTmcUserMin + 1) ||
           (dialog->hid == kIddCharacter && tmc == kTmcCharacterSize);
}

static void refresh_font_control_value(DialogState* dialog, Tmc raw_tmc,
                                       ControlState* state, bool ignored) {
    (void) ignored;
    const Tmc tmc = ((Tmc) (raw_tmc & ~0x8000u));

    if (is_font_name_control(dialog, tmc) &&
        g_font_name_to_value != NULL && !str_empty(state->text)) {
        state->value = (Word) g_font_name_to_value(state->text);
    } else if (is_font_size_control(dialog, tmc) &&
               g_font_size_to_value != NULL && !str_empty(state->text)) {
        state->value = (Word) g_font_size_to_value(state->text);
    }
}

int CALLBACK collect_system_font(const LOGFONTA* logical_font,
                                 const TEXTMETRICA* metrics, DWORD type,
                                 LPARAM parameter) {
    StringList* fonts = (StringList*) parameter;
    (void) metrics;
    (void) type;
    if (logical_font == NULL || logical_font->lfFaceName[0] == '\0' ||
        logical_font->lfFaceName[0] == '@') {
        return 1;
    }
    string_list_push(fonts, logical_font->lfFaceName);
    return 1;
}

static int compare_string_i(const void* left, const void* right) {
    const char* const* left_string = (const char* const*) left;
    const char* const* right_string = (const char* const*) right;
    return _stricmp(*left_string, *right_string);
}

static void string_list_unique_i(StringList* list) {
    size_t write = 0;
    for (size_t read = 0; read < list->size; ++read) {
        if (write != 0 && _stricmp(list->data[write - 1], list->data[read]) == 0) {
            free(list->data[read]);
            continue;
        }
        list->data[write++] = list->data[read];
    }
    list->size = write;
}

static void installed_windows_fonts(StringList* fonts) {
    const HDC dc = GetDC(NULL);
    if (dc != NULL) {
        LOGFONTA logical_font;
        memset(&logical_font, 0, sizeof(logical_font));
        logical_font.lfCharSet = DEFAULT_CHARSET;
        EnumFontFamiliesExA(dc, &logical_font,
                            ((FONTENUMPROCA) (collect_system_font)),
                            ((LPARAM) fonts), 0);
        ReleaseDC(NULL, dc);
    }
    qsort(fonts->data, fonts->size, sizeof(fonts->data[0]), compare_string_i);
    string_list_unique_i(fonts);
}

static void replace_list_entries(DialogState* dialog, Tmc tmc,
                                 StringList* entries) {
    ControlState* state = dialog_control(dialog, tmc);
    string_list_free(&state->entries);
    state->entries = *entries;
    memset(entries, 0, sizeof(*entries));
}

static bool populate_windows_font_control(DialogState* dialog, Tmc tmc) {
    if (is_font_name_control(dialog, tmc)) {
        StringList fonts = {0};
        installed_windows_fonts(&fonts);
        replace_list_entries(dialog, tmc, &fonts);
        return dialog_control(dialog, tmc)->entries.size != 0;
    }
    if (is_font_size_control(dialog, tmc)) {
        static const char* sizes[] = {
            "8",  "9",  "10", "11", "12", "14", "16", "18",
            "20", "22", "24", "26", "28", "36", "48", "72"};
        StringList size_list = {0};
        for (size_t index = 0; index < sizeof(sizes) / sizeof(sizes[0]); ++index) {
            string_list_push(&size_list, sizes[index]);
        }
        replace_list_entries(dialog, tmc, &size_list);
        return true;
    }
    return false;
}

typedef struct OriginalListBinding {
    OriginalListProc proc;
    Word parameter;
} OriginalListBinding;

static OriginalListBinding original_list_binding(const DialogState* dialog,
                                                 Tmc raw_tmc) {
    OriginalListBinding binding = {NULL, 0};
    const Tmc tmc = ((Tmc) (raw_tmc & ~0x8000u));
    if (dialog->hid == kCxtRibbonIconBar) {
        if (tmc == kTmcUserMin) {
            binding.proc = g_list_font_name;
            return binding;
        }
        if (tmc == kTmcUserMin + 1) {
            binding.proc = g_list_font_size;
            binding.parameter = kTmcUserMin;
            return binding;
        }
    } else if (dialog->hid == kCxtRulerIconBar && tmc == kTmcUserMin) {
        binding.proc = g_list_styles;
        return binding;
    } else if (dialog->hid == kIddCharacter) {
        if (tmc == kTmcCharacterName) {
            binding.proc = g_list_font_name;
            return binding;
        }
        if (tmc == kTmcCharacterSize) {
            binding.proc = g_list_font_size;
            binding.parameter = kTmcCharacterName;
            return binding;
        }
        if (tmc == kTmcCharacterColor) {
            binding.proc = g_list_character_color;
            return binding;
        }
    } else if (dialog->hid == kIddApplyStyle && tmc == kTmcApplyStyle) {
        binding.proc = g_list_styles;
        return binding;
    } else if (dialog->hid == kIddDefineStyle &&
               (tmc == kTmcDefineStyle || tmc == kTmcDefineBasedOn ||
                tmc == kTmcDefineNext)) {
        binding.proc = g_list_styles;
        return binding;
    }
    return binding;
}

static bool populate_original_list(DialogState* dialog, Tmc raw_tmc) {
    const Tmc tmc = ((Tmc) (raw_tmc & ~0x8000u));
    if (populate_windows_font_control(dialog, tmc)) {
        return true;
    }
    const OriginalListBinding binding = original_list_binding(dialog, tmc);
    if (binding.proc == NULL) {
        return false;
    }

    ControlState* state = dialog_control(dialog, tmc);
    string_list_clear(&state->entries);

    const Hdlg previous_current = g_current_dialog;
    g_current_dialog = dialog->handle;
    sync_current_dialog_globals();

    char buffer[512];
    memset(buffer, 0, sizeof(buffer));
    const Word reported_count =
        binding.proc(kTmmCount, buffer, 0, 0, tmc, binding.parameter);
    const unsigned limit = reported_count == kUnknownListCount
                               ? 4096u
                               : ((unsigned) (reported_count));
    for (unsigned index = 0; index < limit; ++index) {
        buffer[0] = '\0';
        if (binding.proc(kTmmText, buffer, ((int) (index)), 0, tmc,
                         binding.parameter) == 0) {
            break;
        }
        string_list_push(&state->entries, buffer);
    }

    g_current_dialog = previous_current;
    sync_current_dialog_globals();
    return state->entries.size != 0;
}

static void read_style_cab(DialogState* dialog, Tmc tmc) {
    char style_name[256];
    memset(style_name, 0, sizeof(style_name));
    if (dialog->cab != NULL) {
        /* Pointer arguments begin at iag 1 in the native, aligned CAB layout
         * used by CABAPPLYSTYLE and CABDEFINESTYLE.
         */
        GetCabSz(dialog->cab, style_name, (Word) sizeof(style_name), 1);
    }
    str_set(&dialog_control(dialog, tmc)->text, style_name);
}

static void sync_style_cab(DialogState* dialog, Tmc tmc) {
    ControlState* state = dialog_find_control(dialog, tmc);
    if (state == NULL) {
        return;
    }
    if (dialog->cab != NULL) {
        FSetCabSz(dialog->cab, state->text, 1);
    }
}

typedef struct CabCharacterNative {
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
} CabCharacterNative;

static CabCharacterNative* character_cab(DialogState* dialog) {
    if (dialog->cab == NULL || *dialog->cab == NULL ||
        OpusCbOfH(dialog->cab) < sizeof(CabCharacterNative)) {
        return NULL;
    }
    return (CabCharacterNative*) (*dialog->cab);
}

static void set_native_check(DialogState* dialog, Tmc tmc, int value) {
    dialog_control(dialog, tmc)->value = (Word) value;
}

static void read_character_cab(DialogState* dialog) {
    CabCharacterNative* cab = character_cab(dialog);
    ControlState* font;
    ControlState* size;
    ControlState* color;
    if (cab == NULL) {
        return;
    }
    char font_name[LF_FACESIZE];
    memset(font_name, 0, sizeof(font_name));
    if (g_font_name_from_value != NULL) {
        g_font_name_from_value(cab->ftc, font_name, (int) sizeof(font_name));
    }
    font = dialog_control(dialog, kTmcCharacterName);
    str_set(&font->text, font_name);
    font->value = (Word) cab->ftc;

    char size_text[32];
    memset(size_text, 0, sizeof(size_text));
    if (cab->hps >= 0 && cab->hps != 0x8001) {
        if ((cab->hps & 1) == 0) {
            snprintf(size_text, sizeof(size_text), "%d", cab->hps / 2);
        } else {
            snprintf(size_text, sizeof(size_text), "%d.5", cab->hps / 2);
        }
    }
    size = dialog_control(dialog, kTmcCharacterSize);
    str_set(&size->text, size_text);
    size->value = (Word) cab->hps;

    color = dialog_control(dialog, kTmcCharacterColor);
    if (cab->color >= 0 &&
        ((size_t) cab->color) < color->entries.size) {
        color->value = (Word) cab->color;
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
    set_native_check(dialog, ((Tmc) (kTmcUserMin + 13 + position)), 1);
    const int spacing = cab->spacing >= 0 && cab->spacing <= 2
                            ? cab->spacing
                            : 0;
    set_native_check(dialog, ((Tmc) (kTmcUserMin + 18 + spacing)), 1);
}

static const char* control_text(DialogState* dialog, Tmc tmc) {
    ControlState* state = dialog_control(dialog, tmc);
    return state->text == NULL ? "" : state->text;
}

static void sync_character_cab(DialogState* dialog) {
    CabCharacterNative* cab = character_cab(dialog);
    if (cab == NULL) {
        return;
    }
    if (g_font_name_to_value != NULL) {
        const int ftc =
            g_font_name_to_value(control_text(dialog, kTmcCharacterName));
        if (ftc >= 0) {
            cab->ftc = ftc;
        }
    }
    if (g_font_size_to_value != NULL) {
        const int hps =
            g_font_size_to_value(control_text(dialog, kTmcCharacterSize));
        if (hps >= 0) {
            cab->hps = hps;
        }
    }
    cab->color = dialog_control(dialog, kTmcCharacterColor)->value;
    cab->bold = control_checked(dialog, kTmcUserMin + 5);
    cab->italic = control_checked(dialog, kTmcUserMin + 6);
    cab->small_caps = control_checked(dialog, kTmcUserMin + 7);
    cab->hidden = control_checked(dialog, kTmcUserMin + 8);
    cab->underline = control_checked(dialog, kTmcUserMin + 9);
    cab->word_underline = control_checked(dialog, kTmcUserMin + 10);
    cab->double_underline = control_checked(dialog, kTmcUserMin + 11);
    for (int index = 0; index < 3; ++index) {
        if (control_checked(dialog, (Tmc) (kTmcUserMin + 13 + index))) {
            cab->position = index;
        }
        if (control_checked(dialog, (Tmc) (kTmcUserMin + 18 + index))) {
            cab->spacing = index;
        }
    }
}

static void materialize_character_template(DialogState* dialog) {
    if (dialog->hid != kIddCharacter || dialog->window == NULL) {
        return;
    }
    static const DWORD combo_style =
        WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWN | CBS_AUTOHSCROLL;
    create_static_text(dialog, "Character", (Rec){4, 4, 44, 9});
    create_static_text(dialog, "&Font:", (Rec){4, 14, 35, 9});
    create_native_control(dialog, kTmcCharacterName, "COMBOBOX", "",
                          (Rec){4, 24, 80, 68}, combo_style);
    create_static_text(dialog, "&Points:", (Rec){88, 14, 35, 9});
    create_native_control(dialog, kTmcCharacterSize, "COMBOBOX", "",
                          (Rec){88, 24, 40, 68}, combo_style);
    create_static_text(dialog, "Co&lor:", (Rec){4, 38, 35, 9});
    create_native_control(dialog, kTmcCharacterColor, "COMBOBOX", "",
                          (Rec){4, 48, 44, 72},
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
    for (size_t index = 0; index < sizeof(checks) / sizeof(checks[0]); ++index) {
        create_native_control(dialog, checks[index].tmc, "BUTTON",
                              checks[index].caption, checks[index].rec,
                              WS_TABSTOP | BS_AUTOCHECKBOX);
    }

    create_untracked_control(dialog, "BUTTON", "Position", (Rec){84, 44, 89, 48},
                             BS_GROUPBOX);
    create_native_control(dialog, kTmcUserMin + 13, "BUTTON", "&Normal",
                          (Rec){87, 54, 45, 12},
                          WS_TABSTOP | WS_GROUP | BS_AUTORADIOBUTTON);
    create_native_control(dialog, kTmcUserMin + 14, "BUTTON", "&Superscript",
                          (Rec){87, 65, 54, 12},
                          WS_TABSTOP | BS_AUTORADIOBUTTON);
    create_native_control(dialog, kTmcUserMin + 15, "BUTTON", "Subsc&ript",
                          (Rec){87, 76, 50, 12},
                          WS_TABSTOP | BS_AUTORADIOBUTTON);
    create_static_text(dialog, "B&y:", (Rec){141, 67, 13, 9});
    create_native_control(dialog, kTmcUserMin + 16, "EDIT", "",
                          (Rec){141, 77, 30, 12},
                          WS_TABSTOP | WS_BORDER | ES_AUTOHSCROLL);

    create_untracked_control(dialog, "BUTTON", "Character Spacing",
                             (Rec){84, 96, 89, 49}, BS_GROUPBOX);
    create_native_control(dialog, kTmcUserMin + 18, "BUTTON", "N&ormal",
                          (Rec){87, 107, 45, 12},
                          WS_TABSTOP | WS_GROUP | BS_AUTORADIOBUTTON);
    create_native_control(dialog, kTmcUserMin + 19, "BUTTON", "&Expanded",
                          (Rec){87, 119, 46, 12},
                          WS_TABSTOP | BS_AUTORADIOBUTTON);
    create_native_control(dialog, kTmcUserMin + 20, "BUTTON", "&Condensed",
                          (Rec){87, 130, 50, 12},
                          WS_TABSTOP | BS_AUTORADIOBUTTON);
    create_static_text(dialog, "By&:", (Rec){141, 120, 12, 9});
    create_native_control(dialog, kTmcUserMin + 21, "EDIT", "",
                          (Rec){141, 130, 30, 12},
                          WS_TABSTOP | WS_BORDER | ES_AUTOHSCROLL);
    create_native_control(dialog, kTmcOk, "BUTTON", "OK", (Rec){140, 6, 34, 14},
                          WS_TABSTOP | BS_DEFPUSHBUTTON);
    create_native_control(dialog, kTmcCancel, "BUTTON", "Cancel",
                          (Rec){140, 23, 34, 14}, WS_TABSTOP | BS_PUSHBUTTON);
    str_set(&dialog->caption, "Character");
    dialog->native_modal = true;
    populate_original_list(dialog, kTmcCharacterName);
    populate_original_list(dialog, kTmcCharacterSize);
    populate_original_list(dialog, kTmcCharacterColor);
    read_character_cab(dialog);
}

static void materialize_apply_style_template(DialogState* dialog) {
    if (dialog->hid != kIddApplyStyle || dialog->window == NULL) {
        return;
    }
    create_static_text(dialog, "&Style Name:", (Rec){4, 3, 55, 9});
    create_native_control(dialog, kTmcApplyStyle, "COMBOBOX", "",
                          (Rec){4, 13, 96, 60},
                          WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWN |
                              CBS_AUTOHSCROLL);
    create_native_control(dialog, kTmcOk, "BUTTON", "OK", (Rec){104, 6, 40, 14},
                          WS_TABSTOP | BS_DEFPUSHBUTTON);
    create_native_control(dialog, kTmcCancel, "BUTTON", "Cancel",
                          (Rec){104, 23, 40, 14}, WS_TABSTOP | BS_PUSHBUTTON);
    create_native_control(dialog, kTmcApplyDefine, "BUTTON", "&Define...",
                          (Rec){103, 39, 42, 14}, WS_TABSTOP | BS_PUSHBUTTON);
    create_native_control(dialog, kTmcApplyBanter, "STATIC", "",
                          (Rec){4, 75, 141, 24}, SS_LEFT);
    str_set(&dialog->caption, "Apply Style");
    dialog->native_modal = true;
    read_style_cab(dialog, kTmcApplyStyle);
    populate_original_list(dialog, kTmcApplyStyle);
}

static void materialize_define_style_template(DialogState* dialog) {
    if (dialog->hid != kIddDefineStyle || dialog->window == NULL) {
        return;
    }
    create_static_text(dialog, "Define &Style Name:", (Rec){4, 3, 78, 10});
    create_native_control(dialog, kTmcDefineStyle, "COMBOBOX", "",
                          (Rec){4, 14, 76, 60},
                          WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWN |
                              CBS_AUTOHSCROLL);
    create_native_control(dialog, kTmcDefineChars, "BUTTON", "&Character...",
                          (Rec){85, 14, 52, 14}, WS_TABSTOP | BS_PUSHBUTTON);
    create_native_control(dialog, kTmcDefineParas, "BUTTON", "&Paragraph...",
                          (Rec){85, 30, 52, 14}, WS_TABSTOP | BS_PUSHBUTTON);
    create_native_control(dialog, kTmcDefineTabs, "BUTTON", "&Tabs...",
                          (Rec){85, 46, 52, 14}, WS_TABSTOP | BS_PUSHBUTTON);
    create_native_control(dialog, kTmcDefinePosition, "BUTTON", "Pos&ition...",
                          (Rec){85, 62, 52, 14}, WS_TABSTOP | BS_PUSHBUTTON);
    create_native_control(dialog, kTmcOk, "BUTTON", "OK", (Rec){140, 14, 45, 14},
                          WS_TABSTOP | BS_DEFPUSHBUTTON);
    create_native_control(dialog, kTmcCancel, "BUTTON", "Cancel",
                          (Rec){140, 30, 45, 14}, WS_TABSTOP | BS_PUSHBUTTON);
    create_native_control(dialog, kTmcDefineOptions, "BUTTON", "&Options >>",
                          (Rec){140, 62, 45, 14}, WS_TABSTOP | BS_PUSHBUTTON);
    create_native_control(dialog, kTmcDefineBanter, "STATIC", "",
                          (Rec){4, 76, 180, 24}, SS_LEFT);
    create_native_control(dialog, kTmcDefineCommit, "BUTTON", "&Define",
                          (Rec){4, 129, 43, 14}, WS_TABSTOP | BS_PUSHBUTTON);
    create_native_control(dialog, kTmcDefineDelete, "BUTTON", "De&lete",
                          (Rec){49, 129, 43, 14}, WS_TABSTOP | BS_PUSHBUTTON);
    create_native_control(dialog, kTmcDefineRename, "BUTTON", "&Rename...",
                          (Rec){94, 129, 43, 14}, WS_TABSTOP | BS_PUSHBUTTON);
    create_native_control(dialog, kTmcDefineMerge, "BUTTON", "&Merge...",
                          (Rec){139, 129, 43, 14}, WS_TABSTOP | BS_PUSHBUTTON);
    str_set(&dialog->caption, "Define Style");
    dialog->native_modal = true;
    read_style_cab(dialog, kTmcDefineStyle);
    populate_original_list(dialog, kTmcDefineStyle);
}

static void materialize_icon_bar_template(DialogState* dialog) {
    static const DWORD combo_style =
        WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWN | CBS_AUTOHSCROLL;

    if (dialog->hid == kCxtRibbonIconBar) {
        const bool win3 = (*dialog->template_handle)->rec.dx == 160;
        if (win3) {
            create_static_text(dialog, "Font:", (Rec){4, 3, 20, 8});
            create_native_control(dialog, kTmcUserMin, "COMBOBOX", "",
                                  (Rec){26, 1, 76, 67}, combo_style);
            create_static_text(dialog, "Pts:", (Rec){108, 3, 14, 8});
            create_native_control(dialog, kTmcUserMin + 1, "COMBOBOX", "",
                                  (Rec){127, 1, 28, 67}, combo_style);
        } else {
            create_static_text(dialog, "Font:", (Rec){4, 3, 20, 8});
            create_native_control(dialog, kTmcUserMin, "COMBOBOX", "",
                                  (Rec){29, 1, 80, 68}, combo_style);
            create_static_text(dialog, "Pts:", (Rec){115, 3, 16, 8});
            create_native_control(dialog, kTmcUserMin + 1, "COMBOBOX", "",
                                  (Rec){134, 1, 32, 68}, combo_style);
        }
    } else if (dialog->hid == kCxtRulerIconBar) {
        const bool win3 = (*dialog->template_handle)->rec.dx == 102;
        if (win3) {
            create_static_text(dialog, "Style:", (Rec){4, 3, 20, 8});
            create_native_control(dialog, kTmcUserMin, "COMBOBOX", "",
                                  (Rec){26, 1, 76, 68}, combo_style);
        } else {
            create_static_text(dialog, "Style:", (Rec){4, 3, 24, 8});
            create_native_control(dialog, kTmcUserMin, "COMBOBOX", "",
                                  (Rec){29, 1, 80, 68}, combo_style);
        }
    }
    if (dialog->hid == kCxtRibbonIconBar) {
        populate_original_list(dialog, kTmcUserMin);
        populate_original_list(dialog, kTmcUserMin + 1);
    }
}

static HWND ensure_control_window(Tmc raw_tmc) {
    ControlState* state = control(raw_tmc);
    if (state->window != NULL && IsWindow(state->window)) {
        return state->window;
    }
    return NULL;
}

static void copy_text(const char* source, char* destination, Word capacity) {
    if (destination == NULL || capacity == 0) {
        return;
    }
    const size_t count = min_size(strlen(source == NULL ? "" : source),
                                  (size_t) (capacity - 1));
    memcpy(destination, source == NULL ? "" : source, count);
    destination[count] = '\0';
}

static char* counted_or_zero_terminated(const char* text) {
    return xstrdup(text == NULL ? "" : text);
}

static bool invoke_dialog_proc(DialogState* dialog, Word message, Tmc tmc,
                               Word new_value, Word old_value,
                               Word parameter) {
    if (dialog->template_handle == NULL ||
        *dialog->template_handle == NULL ||
        (*dialog->template_handle)->dialog_proc == NULL) {
        return true;
    }
    const NativeDialogProc proc =
        (NativeDialogProc) (*dialog->template_handle)->dialog_proc;
    return proc(message, tmc, new_value, old_value, parameter) != 0;
}

static bool invoke_dialog_proc_simple(DialogState* dialog, Word message,
                                      Tmc tmc) {
    return invoke_dialog_proc(dialog, message, tmc, 0, 0, 0);
}

static void ensure_icon_bar_commands_active(DialogState* dialog, Tmc tmc) {
    if (dialog->hid != kCxtRibbonIconBar || dialog->commands_active) {
        return;
    }
    dialog->commands_active = true;
    invoke_dialog_proc_simple(dialog, kDlmSetDialogFocus, tmc);
    invoke_dialog_proc_simple(dialog, kDlmSetItemFocus, tmc);
}

static void commit_ribbon_list_selection(DialogState* dialog, Tmc tmc) {
    ControlState* state;
    if (dialog->hid != kCxtRibbonIconBar || !dialog->commands_active) {
        return;
    }

    /* The Win16 SDM/icon-bar loop applied a combo selection when its dialog
     * focus was terminated. A native Win32 drop-down keeps focus on the combo
     * after the list closes, so that termination never reliably occurs.
     * CBN_SELENDOK identifies the completed choice, after all intermediate
     * CBN_SELCHANGE notifications, and is the safe point to send the same
     * original callbacks. Typed edit text still commits on CBN_KILLFOCUS.
     */
    state = dialog_control(dialog, tmc);
    OpusX64TraceRibbon("commit-begin", kDlmKillDialogFocus, tmc,
                       state->value, dialog->commands_active,
                       0, 0, 0);
    const bool item_result =
        invoke_dialog_proc_simple(dialog, kDlmKillItemFocus, tmc);
    const bool dialog_result =
        invoke_dialog_proc_simple(dialog, kDlmKillDialogFocus, tmc);
    dialog->commands_active = false;

    /* Win16 SDM returned focus to the document when an icon-bar session ended.
     * A native combo otherwise retains focus, forcing the user to click the
     * document; that mouse click reloads the properties at the caret and
     * discards the just-applied insertion font and size. Route the focus return
     * through Microsoft's original FDlgIb handler.
     */
    const bool focus_result =
        invoke_dialog_proc_simple(dialog, kDlmDialogClick, tmc);
    OpusX64TraceRibbon("commit-end", kDlmKillDialogFocus, tmc,
                       item_result, dialog_result, focus_result, 0, 0);
}

static const char* selected_list_text(const ControlState* control_state) {
    if (control_state->value >= control_state->entries.size) {
        return "";
    }
    return control_state->entries.data[control_state->value];
}

static void select_new_type(DialogState* dialog) {
    ControlState* found = dialog_find_control(dialog, kTmcNewTypeList);
    if (found == NULL) {
        return;
    }
    const char* selected = selected_list_text(found);
    if (str_empty(selected)) {
        return;
    }
    str_set(&dialog_control(dialog, kTmcNewType)->text, selected);
}

static void select_open_file(DialogState* dialog) {
    ControlState* found = dialog_find_control(dialog, kTmcOpenFileList);
    char* selected_path;
    if (found == NULL) {
        return;
    }
    const char* selected = selected_list_text(found);
    if (str_empty(selected)) {
        return;
    }
    selected_path = join_path(dialog->current_directory, selected);
    str_set(&dialog_control(dialog, kTmcOpenFileName)->text, selected_path);
    free(selected_path);
}

static void enter_open_directory(DialogState* dialog) {
    ControlState* found = dialog_find_control(dialog, kTmcOpenFileDir);
    const char* selected;
    char* leaf;
    char* destination;
    char* edit_text;
    char full_path[32768];
    if (found == NULL) {
        return;
    }
    selected = selected_list_text(found);
    if (strlen(selected) < 2 || selected[0] != '[' ||
        selected[strlen(selected) - 1] != ']') {
        return;
    }
    leaf = xstrndup(selected + 1, strlen(selected) - 2);
    destination = join_path(dialog->current_directory, leaf);
    memset(full_path, 0, sizeof(full_path));
    if (GetFullPathNameA(destination, (DWORD) sizeof(full_path), full_path,
                         NULL) == 0) {
        free(leaf);
        free(destination);
        return;
    }
    str_set(&dialog->current_directory, full_path);
    populate_open_lists(dialog);
    edit_text = join_path(dialog->current_directory, dialog->file_pattern);
    str_set(&dialog_control(dialog, kTmcOpenFileName)->text, edit_text);
    free(edit_text);
    free(leaf);
    free(destination);
}

static void enter_save_directory(DialogState* dialog) {
    ControlState* found = dialog_find_control(dialog, kTmcSaveDirectoryList);
    ControlState* edit;
    const char* selected;
    char* leaf;
    char* destination;
    char full_path[32768];
    if (found == NULL) {
        return;
    }
    selected = selected_list_text(found);
    if (strlen(selected) < 2 || selected[0] != '[' ||
        selected[strlen(selected) - 1] != ']') {
        return;
    }
    leaf = xstrndup(selected + 1, strlen(selected) - 2);
    destination = join_path(dialog->current_directory, leaf);
    memset(full_path, 0, sizeof(full_path));
    if (GetFullPathNameA(destination, (DWORD) sizeof(full_path), full_path,
                         NULL) == 0) {
        free(leaf);
        free(destination);
        return;
    }
    str_set(&dialog->current_directory, full_path);
    populate_save_directories(dialog);

    edit = dialog_control(dialog, kTmcSaveFile);
    {
        const char* last_slash = strrchr(edit->text == NULL ? "" : edit->text, '\\');
        const char* last_forward = strrchr(edit->text == NULL ? "" : edit->text, '/');
        const char* file_name = last_slash > last_forward ? last_slash : last_forward;
        char* new_text = NULL;
        file_name = file_name == NULL ? edit->text : file_name + 1;
        if (!str_empty(file_name)) {
            new_text = join_path(dialog->current_directory, file_name);
            str_set(&edit->text, new_text);
            free(new_text);
        } else {
            str_set(&edit->text, "");
        }
    }
    free(leaf);
    free(destination);
}

static void finish_native_dialog(DialogState* dialog, Tmc result) {
    if (dialog->hid == kIddOpen) {
        sync_open_cab(dialog);
    } else if (dialog->hid == kIddSaveAs) {
        sync_save_cab(dialog);
    } else if (dialog->hid == kIddNewDoc) {
        sync_new_cab(dialog);
    } else if (dialog->hid == kIddApplyStyle) {
        sync_style_cab(dialog, kTmcApplyStyle);
    } else if (dialog->hid == kIddDefineStyle) {
        sync_style_cab(dialog, kTmcDefineStyle);
    } else if (dialog->hid == kIddCharacter) {
        sync_character_cab(dialog);
    }
    if (!invoke_dialog_proc_simple(dialog, kDlmTerm, result)) {
        return;
    }
    dialog->result_tmc = result;
    dialog->dying = true;
    dialog->visible = false;
    if (dialog->window != NULL && IsWindow(dialog->window)) {
        ShowWindow(dialog->window, SW_HIDE);
    }
}

static void handle_dialog_command(Hdlg handle, WPARAM w_param,
                                  LPARAM l_param) {
    DialogState* dialog = find_dialog(handle);
    if (dialog == NULL || dialog->dying) {
        return;
    }
    const Tmc tmc = (Tmc) (LOWORD(w_param) & ~0x8000u);
    const Word notification = HIWORD(w_param);
    ControlState* found = dialog_find_control(dialog, tmc);
    (void)l_param;
    g_current_dialog = handle;
    g_focus_dialog = handle;
    sync_current_dialog_globals();

    if (dialog->hid == kIddSaveAs && tmc == kTmcSaveDirectoryList &&
        (notification == LBN_SELCHANGE || notification == LBN_DBLCLK)) {
        if (found == NULL) {
            return;
        }
        if (notification == LBN_DBLCLK) {
            enter_save_directory(dialog);
        }
        invoke_dialog_proc(dialog,
                           notification == LBN_DBLCLK ? kDlmDblClk : kDlmClick,
                           tmc, found->value, 0, 0);
        return;
    }
    if (dialog->hid == kIddOpen && tmc == kTmcOpenFileList &&
        (notification == LBN_SELCHANGE || notification == LBN_DBLCLK)) {
        select_open_file(dialog);
        invoke_dialog_proc_simple(dialog,
                                  notification == LBN_DBLCLK ? kDlmDblClk :
                                      kDlmClick,
                                  tmc);
        if (notification == LBN_DBLCLK && !dialog->dying) {
            finish_native_dialog(dialog, kTmcOk);
        }
        return;
    }
    if (dialog->hid == kIddOpen && tmc == kTmcOpenFileDir &&
        (notification == LBN_SELCHANGE || notification == LBN_DBLCLK)) {
        invoke_dialog_proc_simple(dialog,
                                  notification == LBN_DBLCLK ? kDlmDblClk :
                                      kDlmClick,
                                  tmc);
        if (notification == LBN_DBLCLK && !dialog->dying) {
            enter_open_directory(dialog);
        }
        return;
    }
    if (notification == BN_CLICKED &&
        (tmc == kTmcOk || tmc == kTmcCancel ||
         (dialog->hid == kIddOpen && tmc == kTmcOpenCatalog))) {
        finish_native_dialog(dialog, tmc);
        return;
    }

    if (found != NULL && found->kind == CONTROL_KIND_COMBO) {
        OpusX64TraceRibbon("combo", notification, tmc,
                           found->value, dialog->commands_active,
                           0, 0, 0);
        if (notification == CBN_SETFOCUS) {
            if (!dialog->commands_active) {
                dialog->commands_active = true;
                invoke_dialog_proc_simple(dialog, kDlmSetDialogFocus, tmc);
            }
            invoke_dialog_proc_simple(dialog, kDlmSetItemFocus, tmc);
            return;
        }
        if (notification == CBN_KILLFOCUS) {
            if (dialog->commands_active) {
                invoke_dialog_proc_simple(dialog, kDlmKillItemFocus, tmc);
                invoke_dialog_proc_simple(dialog, kDlmKillDialogFocus, tmc);
                dialog->commands_active = false;
            }
            return;
        }
        if (notification == CBN_DROPDOWN) {
            populate_original_list(dialog, tmc);
            return;
        }
        if (notification == CBN_EDITCHANGE) {
            ensure_icon_bar_commands_active(dialog, tmc);
            refresh_font_control_value(dialog, tmc, found, true);
            OpusX64TraceRibbon("combo-edit", notification, tmc,
                               found->value,
                               dialog->commands_active, 0, 0, 0);
            invoke_dialog_proc_simple(dialog, kDlmChange, tmc);
            return;
        }
        if (notification == CBN_SELCHANGE) {
            ensure_icon_bar_commands_active(dialog, tmc);
            const Word selection = found->value;
            if (selection < found->entries.size) {
                const char* selected = selected_list_text(found);
                dialog_control(dialog, (Tmc) (tmc + 1))->value = selection;
                found->value = selection;
                str_set(&found->text, selected);
                refresh_font_control_value(dialog, tmc, found, false);
                OpusX64TraceRibbon("combo-select", notification, tmc,
                                   found->value,
                                   ((int) (selection)), 0, 0, 0);
                invoke_dialog_proc_simple(dialog, kDlmClick,
                                          (Tmc) (tmc + 1));
                invoke_dialog_proc_simple(dialog, kDlmChange, tmc);
            }
            return;
        }
        if (notification == CBN_SELENDOK) {
            ensure_icon_bar_commands_active(dialog, tmc);
            PostMessageW(dialog->window, kWmCommitRibbonSelection, tmc, 0);
            return;
        }
    }

    if (!dialog->commands_active) {
        return;
    }

    if (dialog->hid == kIddNewDoc && tmc == kTmcNewType &&
        notification == EN_CHANGE) {
        invoke_dialog_proc_simple(dialog, kDlmChange, tmc);
        return;
    }
    if (dialog->hid == kIddNewDoc && tmc == kTmcNewTypeList &&
        (notification == LBN_SELCHANGE || notification == LBN_DBLCLK)) {
        select_new_type(dialog);
        invoke_dialog_proc_simple(dialog,
                                  notification == LBN_DBLCLK ? kDlmDblClk :
                                      kDlmClick,
                                  tmc);
        return;
    }
    if (dialog->hid == kIddOpen && tmc == kTmcOpenFileName &&
        notification == EN_CHANGE) {
        invoke_dialog_proc_simple(dialog, kDlmChange, tmc);
        return;
    }
    if (dialog->hid == kIddSaveAs && tmc == kTmcSaveFile &&
        notification == EN_CHANGE) {
        invoke_dialog_proc_simple(dialog, kDlmChange, tmc);
        return;
    }
    if (notification == BN_CLICKED) {
        if (dialog->hid == kIddNewDoc &&
            (tmc == kTmcRNewDoc || tmc == kTmcRNewDot)) {
            const bool new_template = tmc == kTmcRNewDot;
            dialog_control(dialog, kTmcNewDot)->value = new_template;
            dialog_control(dialog, kTmcRNewDoc)->value = !new_template;
            dialog_control(dialog, kTmcRNewDot)->value = new_template;
            invoke_dialog_proc_simple(dialog, kDlmClick, tmc);
        } else if (tmc == kTmcOk || tmc == kTmcCancel ||
                   (dialog->hid == kIddOpen &&
                    tmc == kTmcOpenCatalog) ||
                   (dialog->hid == kIddNewDoc &&
                    tmc == kTmcSummary)) {
            finish_native_dialog(dialog, tmc);
        } else {
            const Word new_value = found == NULL ? 0 : found->value;
            invoke_dialog_proc(dialog, kDlmClick, tmc, new_value, 0, 0);
        }
    }
}

static void replace_extension(char** path, size_t dot, const char* extension) {
    char* prefix = xstrndup(*path, dot);
    char* replaced = str_join3(prefix, extension, "");
    free(prefix);
    str_set(path, replaced);
    free(replaced);
}

static Tmc run_word95_common_file_dialog(DialogState* dialog) {
    const bool opening = dialog->hid == kIddOpen;
    const bool saving = dialog->hid == kIddSaveAs;
    char* initial;
    char* initial_key;
    AliasEntry* initial_alias;
    Tmc result = kTmcCancel;
    if (!opening && !saving) {
        return ((Tmc) (-1));
    }

    invoke_dialog_proc_simple(dialog, kDlmInit, dialog->focus);
    dialog->commands_active = true;
    if (opening) {
        read_open_cab(dialog);
    } else {
        read_save_cab(dialog);
    }

    HWND owner = dialog->window != NULL ?
        GetWindow(dialog->window, GW_OWNER) : NULL;
    if (owner == NULL || !IsWindow(owner)) {
        owner = vhWndMsgBoxParent;
    }
#ifdef OPUS_TEST_HOOKS
    if (saving && owner != NULL) {
        SetPropW(owner, k_save_as_stage_property,
                 ((HANDLE) (1)));
    }
#endif

    initial = xstrdup(opening ?
        control_text(dialog, kTmcOpenFileName) :
        control_text(dialog, kTmcSaveFile));
    initial_key = win95_alias_key(initial);
    initial_alias = alias_find(initial_key);
    free(initial_key);
    if (initial_alias != NULL) {
        str_set(&initial, initial_alias->value);
    }
    establish_open_directory(dialog, str_empty(initial) ? "*.*" : initial);

    for (;;) {
        char test_path[32768];
        DWORD test_path_length = 0;
        char* selected_path = NULL;
        char* legacy_path = NULL;
        bool selected_from_injection;
        bool docx;
        bool odt;
        bool modern;
        bool staged_open;
        memset(test_path, 0, sizeof(test_path));
#ifdef OPUS_TEST_HOOKS
        test_path_length = GetEnvironmentVariableA(
            "WORD1_TEST_FILE_DIALOG_PATH", test_path,
            (DWORD) sizeof(test_path));
#else
        (void)test_path;
#endif
        if (test_path_length >= sizeof(test_path)) {
#ifdef OPUS_TEST_HOOKS
            if (saving && owner != NULL) {
                SetPropW(owner, k_save_as_stage_property,
                         ((HANDLE) (2)));
            }
#endif
            invoke_dialog_proc_simple(dialog, kDlmTerm, kTmcCancel);
            result = kTmcCancel;
            break;
        }
        selected_path = xstrdup(test_path_length == 0 ? initial : test_path);
        selected_from_injection = test_path_length != 0;
        if (str_empty(selected_path)) {
#ifdef OPUS_TEST_HOOKS
            if (saving && owner != NULL) {
                SetPropW(owner, k_save_as_stage_property,
                         ((HANDLE) (2)));
            }
#endif
            invoke_dialog_proc_simple(dialog, kDlmTerm, kTmcCancel);
            result = kTmcCancel;
            free(selected_path);
            break;
        }
#ifdef OPUS_TEST_HOOKS
        if (test_path_length != 0) {
            SetEnvironmentVariableA("WORD1_TEST_FILE_DIALOG_PATH", NULL);
        }
#endif

        const bool save_docx = saving && OpusModernPathIsDocx(selected_path);
        const bool save_odt = saving && OpusModernPathIsOdt(selected_path);
        if (save_docx || save_odt) {
            const char* modern_extension = save_docx ? ".docx" : ".odt";
            const char* slash1 = strrchr(selected_path, '\\');
            const char* slash2 = strrchr(selected_path, '/');
            const char* slash = slash1 > slash2 ? slash1 : slash2;
            const char* dot = strrchr(selected_path, '.');
            if (dot == NULL || (slash != NULL && dot < slash)) {
                char* appended = str_join3(selected_path, modern_extension, "");
                str_set(&selected_path, appended);
                free(appended);
            } else if (_stricmp(dot, ".doc") == 0) {
                replace_extension(&selected_path, (size_t)(dot - selected_path),
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
                invoke_dialog_proc_simple(dialog, kDlmTerm, kTmcCancel);
                result = kTmcCancel;
                free(selected_path);
                break;
            }
            free(selected_path);
            continue;
        }
        docx = OpusModernPathIsDocx(selected_path) != 0;
        odt = OpusModernPathIsOdt(selected_path) != 0;
        modern = docx || odt;
        if (!make_win95_staging_path(&legacy_path,
                                     modern ? ".TXT" : ".DOC")) {
            MessageBoxA(owner,
                "Word could not prepare a temporary document file.",
                opening ? "Open" : "Save As",
                MB_OK | MB_ICONEXCLAMATION);
            if (!selected_from_injection) {
                invoke_dialog_proc_simple(dialog, kDlmTerm, kTmcCancel);
                result = kTmcCancel;
                free(selected_path);
                break;
            }
            free(selected_path);
            continue;
        }
        staged_open = !opening ||
            (docx ?
                OpusModernDocxToTextFile(selected_path, legacy_path) != 0 :
             odt ?
                OpusModernOdtToTextFile(selected_path, legacy_path) != 0 :
                CopyFileA(selected_path, legacy_path, FALSE) != 0);
        if (!staged_open) {
            MessageBoxA(owner,
                        docx ? "Word could not read this DOCX document." :
                        odt ? "Word could not read this OpenDocument file." :
                            "Word could not open the selected file.",
                        "Open", MB_OK | MB_ICONEXCLAMATION);
            if (!selected_from_injection) {
                invoke_dialog_proc_simple(dialog, kDlmTerm, kTmcCancel);
                result = kTmcCancel;
                free(selected_path);
                free(legacy_path);
                break;
            }
            free(selected_path);
            free(legacy_path);
            continue;
        }

        establish_open_directory(dialog, selected_path);
        if (opening) {
            char* key = win95_alias_key(legacy_path);
            alias_set(key, selected_path);
            free(key);
            str_set(&dialog_control(dialog, kTmcOpenFileName)->text,
                    legacy_path);
            dialog_control(dialog, kTmcOpenReadOnly)->value = 0;
            sync_open_cab(dialog);
        } else {
            g_win95_save_alias.active = true;
            g_win95_save_alias.created = true;
            str_set(&g_win95_save_alias.selected_path, selected_path);
            str_set(&g_win95_save_alias.legacy_path, legacy_path);
            str_set(&dialog_control(dialog, kTmcSaveFile)->text,
                    legacy_path);
            sync_save_cab(dialog);
            CabSaveNative* cab = save_cab(dialog);
            if (cab != NULL && modern) {
                cab->format = kSaveFormatRtf;
                cab->quick_save = false;
            }
        }

        if (invoke_dialog_proc_simple(dialog, kDlmTerm, kTmcOk)) {
            result = dialog->dying ? dialog->result_tmc : kTmcOk;
#ifdef OPUS_TEST_HOOKS
            if (saving && owner != NULL && result == kTmcOk) {
                SetPropW(owner, k_save_as_stage_property,
                         ((HANDLE) (3)));
            }
#endif
            if (saving && result != kTmcOk) {
                if (g_win95_save_alias.created) {
                    DeleteFileA(g_win95_save_alias.legacy_path);
                }
                save_alias_reset();
            }
            free(selected_path);
            free(legacy_path);
            break;
        }
#ifdef OPUS_TEST_HOOKS
        if (saving && selected_from_injection && g_win95_save_alias.active) {
            str_set(&dialog_control(dialog, kTmcSaveFile)->text,
                    legacy_path);
            sync_save_cab(dialog);
            result = kTmcOk;
            if (owner != NULL) {
                SetPropW(owner, k_save_as_stage_property,
                         ((HANDLE) (3)));
            }
            free(selected_path);
            free(legacy_path);
            break;
        }
#endif
        if (dialog->dying) {
            result = dialog->result_tmc;
            if (saving) {
                if (g_win95_save_alias.created) {
                    DeleteFileA(g_win95_save_alias.legacy_path);
                }
                save_alias_reset();
            }
            free(selected_path);
            free(legacy_path);
            break;
        }
        if (saving) {
            if (g_win95_save_alias.created) {
                DeleteFileA(g_win95_save_alias.legacy_path);
            }
            save_alias_reset();
        }
        free(selected_path);
        free(legacy_path);
    }

    free(initial);
    dialog->commands_active = false;
    invoke_dialog_proc_simple(dialog, kDlmExit, result);
    return result;
}

LRESULT CALLBACK native_dialog_window_proc(const HWND window,
                                           const UINT message,
                                           const WPARAM w_param,
                                           const LPARAM l_param) {
    const Hdlg handle = (Hdlg) (uintptr_t) GetWindowLongPtrA(window, GWLP_USERDATA);
    if (message == WM_NCCREATE) {
        const CREATESTRUCTA* create = (const CREATESTRUCTA*) l_param;
        SetWindowLongPtrA(window, GWLP_USERDATA,
                          ((LONG_PTR) (create->lpCreateParams)));
    }
    switch (message) {
        case WM_COMMAND:
            handle_dialog_command(handle, w_param, l_param);
            return 0;
        case kWmCommitRibbonSelection:
        {
            DialogState* dialog = find_dialog(handle);
            if (dialog != NULL) {
                g_current_dialog = handle;
                g_focus_dialog = handle;
                sync_current_dialog_globals();
                ensure_icon_bar_commands_active(
                    dialog, ((Tmc) (w_param)));
                commit_ribbon_list_selection(
                    dialog, ((Tmc) (w_param)));
            }
            return 0;
        }
        case WM_CLOSE:
        {
            DialogState* dialog = find_dialog(handle);
            if (dialog != NULL) {
                finish_native_dialog(dialog, kTmcCancel);
            }
            return 0;
        }
        default:
            return DefWindowProcA(window, message, w_param, l_param);
    }
}

int OpusWin95SaveAliasMatches(const unsigned char* st_file) {
    char* path = counted_path(st_file);
    char* key;
    int result;
    if (str_empty(path)) {
        free(path);
        return false;
    }
    if (g_win95_save_alias.active &&
        win95_alias_key_matches(path, g_win95_save_alias.legacy_path)) {
        free(path);
        return true;
    }
    key = win95_alias_key(path);
    result = alias_find(key) != NULL;
    free(key);
    free(path);
    return result;
}

int OpusWin95SaveAliasActiveMatches(const unsigned char* st_file) {
    char* path = counted_path(st_file);
    int result = !str_empty(path) && g_win95_save_alias.active &&
        win95_alias_key_matches(path, g_win95_save_alias.legacy_path);
    free(path);
    return result;
}

int OpusWin95DisplayAlias(unsigned char* const st_file) {
    char* path = counted_path(st_file);
    char* key;
    AliasEntry* saved;
    if (str_empty(path)) {
        free(path);
        return false;
    }
    key = win95_alias_key(path);
    saved = alias_find(key);
    free(key);
    if (saved == NULL) {
        char full_path[32768];
        memset(full_path, 0, sizeof(full_path));
        const DWORD length = GetFullPathNameA(
            path, (DWORD) sizeof(full_path), full_path, NULL);
        if (length == 0 || length >= sizeof(full_path)) {
            free(path);
            return false;
        }
        key = win95_alias_key(full_path);
        saved = alias_find(key);
        free(key);
        if (saved == NULL) {
            free(path);
            return false;
        }
    }

    const char* name = saved->value;
    const char* separator = strrchr(name, '\\');
    if (separator != NULL) {
        name = separator + 1;
    }
    const size_t length = min_size(strlen(name), 119);
    st_file[0] = ((unsigned char) (length));
    memcpy(st_file + 1, name, length);
    free(path);
    return true;
}

int OpusFinishWin95SaveAlias(const unsigned char* st_file,
                             const int success, const int doc) {
    char* path = counted_path(st_file);
    char* key;
    AliasEntry* saved;
    int result;
    if (str_empty(path)) {
        free(path);
        return !g_win95_save_alias.active;
    }
    key = win95_alias_key(path);

    if (g_win95_save_alias.active &&
        win95_alias_key_matches(path, g_win95_save_alias.legacy_path)) {
        bool copied = success != 0;
        if (copied) {
            copied = (OpusModernPathIsDocx(
                          g_win95_save_alias.selected_path) ||
                      OpusModernPathIsOdt(
                          g_win95_save_alias.selected_path)) ?
                OpusSaveDocumentAsDocx(
                    doc, g_win95_save_alias.selected_path) != 0 :
                atomic_copy_file(g_win95_save_alias.legacy_path,
                                 g_win95_save_alias.selected_path);
        }
        if (!success) {
            DeleteFileA(g_win95_save_alias.legacy_path);
        } else if (copied) {
            alias_set(key, g_win95_save_alias.selected_path);
        }
        save_alias_reset();
        free(key);
        free(path);
        return copied;
    }

    saved = alias_find(key);
    if (saved == NULL) {
        free(key);
        free(path);
        return true;
    }
    if (!success) {
        free(key);
        free(path);
        return false;
    }
    result = (OpusModernPathIsDocx(saved->value) ||
              OpusModernPathIsOdt(saved->value)) ?
        OpusSaveDocumentAsDocx(doc, saved->value) != 0 :
        atomic_copy_file(path, saved->value);
    free(key);
    free(path);
    return result;
}

int OpusWin95SaveAliasRequiresRtf(const unsigned char* st_file) {
    char* path = counted_path(st_file);
    char* key;
    AliasEntry* saved;
    int result;
    if (str_empty(path)) {
        free(path);
        return false;
    }
    if (g_win95_save_alias.active &&
        win95_alias_key_matches(path, g_win95_save_alias.legacy_path)) {
        result = OpusModernPathIsDocx(g_win95_save_alias.selected_path) ||
            OpusModernPathIsOdt(g_win95_save_alias.selected_path);
        free(path);
        return result;
    }
    key = win95_alias_key(path);
    saved = alias_find(key);
    result = saved != NULL &&
        (OpusModernPathIsDocx(saved->value) ||
         OpusModernPathIsOdt(saved->value));
    free(key);
    free(path);
    return result;
}

int OpusWin95OpenAliasIsDocx(const unsigned char* st_file) {
    char* path = counted_path(st_file);
    char* key;
    AliasEntry* saved;
    int result;
    if (str_empty(path)) {
        free(path);
        return false;
    }
    key = win95_alias_key(path);
    saved = alias_find(key);
    result = saved != NULL &&
        (OpusModernPathIsDocx(saved->value) ||
         OpusModernPathIsOdt(saved->value));
    free(key);
    free(path);
    return result;
}

typedef struct OpusSdsCompat {
    uintptr_t current_segment;
    uintptr_t focus_segment;
    void** current_dialog;
    void** focus_dialog;
} OpusSdsCompat;

OpusSdsCompat sds = {0};
DAC_SDM dac = {0};
HWND vhWndMsgBoxParent = NULL;

extern void** hcabDlgCur;
extern uintptr_t wRefDlgCur;

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

int FInitSdm_sdm21(void* unused) {
    static bool cleanup_registered;
    (void) unused;
    if (!cleanup_registered) {
        atexit(sdm_cleanup);
        cleanup_registered = true;
    }
    dialog_init(&g_no_dialog);
    control_init(&g_fallback_control);
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
    for (size_t index = 0; index < g_dialog_count; ++index) {
        if (g_dialogs[index].value.window != NULL &&
            IsWindow(g_dialogs[index].value.window)) {
            DestroyWindow(g_dialogs[index].value.window);
        }
    }
    dialog_map_clear();
    dialog_free(&g_no_dialog);
    g_current_dialog = 0;
    g_focus_dialog = 0;
    g_initialized = false;
    sync_current_dialog_globals();
}

Word FtmeIsSdmMessage(MSG* message) {
    (void) message;
    return 0;
}
void ChangeColors() {}

Hdlg HdlgStartDlg(DltHeader** dialog_template, Hcab cab, Dli* initializer) {
    Hdlg handle = 0;
    for (unsigned attempt = 0; attempt < 0xfffeu; ++attempt) {
        const Hdlg candidate = g_next_dialog;
        ++g_next_dialog;
        if (g_next_dialog == 0 || g_next_dialog == ((Hdlg) (-1))) {
            g_next_dialog = 1;
        }
        if (find_dialog(candidate) == NULL) {
            handle = candidate;
            break;
        }
    }
    if (handle == 0) {
        return 0;
    }

    DialogState dialog;
    DialogState* stored;
    dialog_init(&dialog);
    dialog.handle = handle;
    dialog.template_handle = dialog_template;
    dialog.cab = cab;
    if (dialog_template != NULL && *dialog_template != NULL) {
        dialog.hid = (*dialog_template)->hid;
        dialog.focus = (*dialog_template)->tmc_sel_init;
    }
    if (initializer != NULL) {
        dialog.reference = initializer->reference;
        dialog.visible = true;
        dialog.modal = (initializer->flags & 0x00000001u) != 0;
    }
    dialog.window = create_dialog_host(&dialog, initializer);
    if (!dialog_map_insert(handle, &dialog)) {
        dialog_free(&dialog);
        return 0;
    }
    g_current_dialog = handle;
    g_focus_dialog = handle;
    sync_current_dialog_globals();
    stored = find_dialog(handle);
    materialize_icon_bar_template(stored);
    materialize_new_template(stored);
    materialize_open_template(stored);
    materialize_save_as_template(stored);
    materialize_about_template(stored);
    materialize_character_template(stored);
    materialize_apply_style_template(stored);
    materialize_define_style_template(stored);
    return handle;
}

Tmc TmcDoDlgDli(DltHeader** dialog_template, Hcab cab, Dli* initializer) {
    const Hdlg previous_current = g_current_dialog;
    const Hdlg previous_focus = g_focus_dialog;
    const Hdlg handle = HdlgStartDlg(dialog_template, cab, initializer);
    if (handle == 0) {
        return ((Tmc) (-1));
    }
    DialogState* dialog = find_dialog(handle);
    if (dialog == NULL) {
        return ((Tmc) (-1));
    }

    if (dialog->modal &&
        (dialog->hid == kIddOpen || dialog->hid == kIddSaveAs)) {
        const Tmc common_result = run_word95_common_file_dialog(dialog);
        dialog = find_dialog(handle);
        if (dialog != NULL && dialog->window != NULL &&
            IsWindow(dialog->window)) {
            DestroyWindow(dialog->window);
        }
        dialog_map_erase(handle);
        g_current_dialog = find_dialog(previous_current) == NULL ?
            0 : previous_current;
        g_focus_dialog = find_dialog(previous_focus) == NULL ?
            0 : previous_focus;
        sync_current_dialog_globals();
        return common_result;
    }

    Tmc result = kTmcCancel;
    HWND owner = NULL;
    bool restore_owner = false;
    if (dialog->modal && dialog->native_modal && dialog->window != NULL) {
        owner = GetWindow(dialog->window, GW_OWNER);
        restore_owner = owner != NULL && IsWindow(owner) &&
                        IsWindowEnabled(owner) != 0;
        if (restore_owner) {
            EnableWindow(owner, false);
        }

        invoke_dialog_proc_simple(dialog, kDlmInit, dialog->focus);
        dialog->commands_active = true;
        if (!dialog->dying) {
            dialog->visible = true;
            ShowWindow(dialog->window, SW_SHOW);
            UpdateWindow(dialog->window);
            SetActiveWindow(dialog->window);
            ControlState* focus = dialog_find_control(dialog,
                ((Tmc) (dialog->focus & ~0x8000u)));
            if (focus != NULL && focus->window != NULL) {
                SetFocus(focus->window);
            }
        }

        MSG message;
        memset(&message, 0, sizeof(message));
        while (!dialog->dying) {
            const int status = GetMessageA(&message, NULL, 0, 0);
            if (status <= 0) {
                if (status == 0) {
                    PostQuitMessage(((int) (message.wParam)));
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
            if (dialog == NULL) {
                break;
            }
        }
        if (dialog != NULL) {
            result = dialog->result_tmc;
            dialog->commands_active = false;
            invoke_dialog_proc_simple(dialog, kDlmExit, result);
        }
    }

    dialog = find_dialog(handle);
    if (dialog != NULL && dialog->window != NULL &&
        IsWindow(dialog->window)) {
        DestroyWindow(dialog->window);
    }
    dialog_map_erase(handle);
    if (restore_owner && owner != NULL && IsWindow(owner)) {
        EnableWindow(owner, true);
        SetActiveWindow(owner);
    }
    g_current_dialog = find_dialog(previous_current) == NULL
                           ? 0
                           : previous_current;
    g_focus_dialog = find_dialog(previous_focus) == NULL ? 0
                                                            : previous_focus;
    sync_current_dialog_globals();
    return result;
}

Tmc TmcDoDlg_sdm21(DltHeader** dialog_template, Hcab cab,
                    unsigned char* runtime_items) {
    Dli initializer;
    memset(&initializer, 0, sizeof(initializer));
    initializer.flags = 0x00000001u;
    initializer.runtime_items = runtime_items;
    return TmcDoDlgDli(dialog_template, cab, &initializer);
}

void EndDlg(Tmc result) {
    g_dialog.result_tmc = result;
    g_dialog.dying = true;
    g_dialog.visible = false;
    if (g_dialog.window != NULL && IsWindow(g_dialog.window)) {
        ShowWindow(g_dialog.window, SW_HIDE);
    }
}

int FFreeDlg() {
    const Hdlg handle = g_current_dialog;
    DialogState* found = find_dialog(handle);
    if (found == NULL) {
        return false;
    }
    if (found->window != NULL && IsWindow(found->window)) {
        DestroyWindow(found->window);
    }
    dialog_map_erase(handle);
    if (g_focus_dialog == handle) {
        g_focus_dialog = 0;
    }
    g_current_dialog = 0;
    sync_current_dialog_globals();
    return true;
}

Word HidOfDlg(Hdlg dialog) {
    const DialogState* state = find_dialog(dialog == 0 ? g_current_dialog : dialog);
    return state == NULL ? 0 : state->hid;
}

Hdlg HdlgSetCurDlg(Hdlg dialog) {
    const Hdlg previous = g_current_dialog;
    if (dialog == 0 || find_dialog(dialog) != NULL) {
        g_current_dialog = dialog;
        sync_current_dialog_globals();
    }
    return previous;
}

Hdlg HdlgSetFocusDlg(Hdlg dialog) {
    const Hdlg previous = g_focus_dialog;
    if (dialog == 0 || find_dialog(dialog) != NULL) {
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
    const DialogState* state = find_dialog(dialog);
    return state != NULL && state->modal;
}

int FIsDlgDying() { return g_dialog.dying; }
void ClearListError(Hdlg dialog) {
    (void) dialog;
}

void ShowDlg(int visible) {
    g_dialog.visible = visible != 0;
    if (g_dialog.window != NULL && IsWindow(g_dialog.window)) {
        ShowWindow(g_dialog.window, visible ? SW_SHOWNA : SW_HIDE);
    }
}
int FVisibleDlg() { return g_dialog.visible; }
void ResizeDlg(int width, int height) {
    if (g_dialog.template_handle != NULL &&
        *g_dialog.template_handle != NULL) {
        (*g_dialog.template_handle)->rec.dx = width;
        (*g_dialog.template_handle)->rec.dy = height;
    }
    if (g_dialog.window != NULL && IsWindow(g_dialog.window)) {
        SetWindowPos(g_dialog.window, NULL, 0, 0, max_int(1, width),
                     max_int(1, height),
                     SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
    }
}
void MoveDlg(int x, int y) {
    if (g_dialog.template_handle != NULL &&
        *g_dialog.template_handle != NULL) {
        (*g_dialog.template_handle)->rec.x = x;
        (*g_dialog.template_handle)->rec.y = y;
    }
    if (g_dialog.window != NULL && IsWindow(g_dialog.window)) {
        SetWindowPos(g_dialog.window, NULL, x, y, 0, 0,
                     SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    }
}
void SdmScaleRec(Rec* rec) {
    (void) rec;
}

int FSetDlgSab(Word sab) {
    g_dialog.sab = sab;
    if (g_dialog.hid == kIddSaveAs) {
        set_save_options_visible(active_dialog(), true);
    }
    return true;
}
Word SabGetDlg() { return g_dialog.sab; }

void SetTmcVal_sdm21(Tmc tmc, Word value) {
    control(tmc)->value = value;
}
Word ValGetTmc(Tmc tmc) {
    const Tmc value_tmc = ((Tmc) (tmc & ~0x8000u));
    ControlState* found = dialog_find_control(active_dialog(), value_tmc);
    if (found == NULL) {
        return 0;
    }
    refresh_font_control_value(active_dialog(), value_tmc, found, true);
    return found->value;
}

void SetTmcText_sdm21(Tmc tmc, char* text) {
    ControlState* state = control(tmc);
    char* copied = counted_or_zero_terminated(text);
    if (strlen(copied) > state->text_limit) {
        copied[state->text_limit] = '\0';
    }
    str_set(&state->text, copied);
    free(copied);
    refresh_font_control_value(active_dialog(), tmc, state, false);
}
void GetTmcText_sdm21(Tmc tmc, char* destination, Word capacity) {
    const ControlState* state = find_control(tmc);
    copy_text(state == NULL ? "" : state->text, destination, capacity);
}
Word CchGetTmcText(Tmc tmc, char* destination, Word capacity) {
    const ControlState* state = find_control(tmc);
    const char* text = state == NULL ? "" : state->text;
    copy_text(text, destination, capacity);
    return (Word) min_size(strlen(text == NULL ? "" : text), 0xffffu);
}
Word CchGetTmc(Tmc tmc) { return CchGetTmcText(tmc, NULL, 0); }

void GetTmcLargeVal(Tmc tmc, void* destination, Word byte_count) {
    if (destination == NULL) {
        return;
    }
    const ControlState* state = find_control(tmc);
    const size_t count = state == NULL
                                  ? 0
                                  : min_size(state->large_value.size,
                                             byte_count);
    if (count != 0) {
        memcpy(destination, state->large_value.data, count);
    }
    if (count < byte_count) {
        memset(((unsigned char*) (destination)) + count, 0,
                    byte_count - count);
    }
}
int FSetTmcLargeVal(Tmc tmc, void* source) {
    if (source == NULL) {
        return false;
    }

    /* Large values are application records; retain one pointer-sized unit when
     * SDM has no generated TM metadata describing a larger record.
     */
    ByteBuffer* value = &control(tmc)->large_value;
    if (!byte_buffer_resize(value, sizeof(void*))) {
        return false;
    }
    memcpy(value->data, source, value->size);
    return true;
}

void SetFocusTmc(Tmc tmc) {
    g_dialog.focus = tmc;
    HWND window = ensure_control_window(tmc);
    if (window != NULL) {
        SetFocus(window);
    }
}
Tmc TmcGetFocus() { return g_dialog.focus; }
void SetDefaultTmc(Tmc tmc) { g_dialog.default_tmc = tmc; }
Tmc TmcGetDefault(int unused) {
    (void) unused;
    return g_dialog.default_tmc;
}
void SetTmcTxs(Tmc tmc, Dword selection) {
    control(tmc)->selection = selection;
}
Dword TxsGetTmc(Tmc tmc) {
    const ControlState* state = find_control(tmc);
    return state == NULL ? 0 : state->selection;
}
Word TmvGetTmc(Tmc tmc) {
    const ControlState* state = find_control(tmc);
    return state != NULL && !str_empty(state->text) ? 3 : 1;
}
void RedisplayTmc(Tmc tmc) { populate_original_list(active_dialog(), tmc); }

void EnableTmc_sdm21(Tmc tmc, int enabled) {
    control(tmc)->enabled = enabled != 0;
}
int FEnabledTmc_sdm21(Tmc tmc) {
    const ControlState* state = find_control(tmc);
    return state == NULL || state->enabled;
}
void EnableNoninteractiveTmc(Tmc tmc, int enabled) {
    EnableTmc_sdm21(tmc, enabled);
}
void SetVisibleTmc(Tmc tmc, int visible) {
    control(tmc)->visible = visible != 0;
}
int FIsVisibleTmc(Tmc tmc) {
    const ControlState* state = find_control(tmc);
    return state == NULL || state->visible;
}
void LimitTextTmc(Tmc tmc, Word limit) {
    ControlState* state = control(tmc);
    state->text_limit = limit;
    if (state->text != NULL && strlen(state->text) > limit) {
        state->text[limit] = '\0';
    }
}
void CompleteComboTmc(Tmc tmc) {
    (void) tmc;
}

void AddListBoxEntry(Tmc tmc, char* entry) {
    char* copied = counted_or_zero_terminated(entry);
    string_list_push(&control(tmc)->entries, copied);
    free(copied);
}
void InsertListBoxEntry(Tmc tmc, char* entry, Word index) {
    ControlState* state = control(tmc);
    char* copied = counted_or_zero_terminated(entry);
    string_list_insert(&state->entries, min_size(state->entries.size, index),
                       copied);
    free(copied);
}
void DeleteListBoxEntry(Tmc tmc, Word index) {
    string_list_delete(&control(tmc)->entries, index);
}
Word CentryListBoxTmc(Tmc tmc) {
    const ControlState* state = find_control(tmc);
    return (Word) (state == NULL ? 0 : min_size(state->entries.size, 0xffffu));
}
Word CEntryListBoxTmc(Tmc tmc) { return CentryListBoxTmc(tmc); }
void GetListBoxEntry(Tmc tmc, Word index, char* destination, Word capacity) {
    const ControlState* state = find_control(tmc);
    copy_text(state == NULL || index >= state->entries.size
                  ? ""
                  : state->entries.data[index],
              destination, capacity);
}
Word CchGetListBoxEntry(Tmc tmc, Word index, char* destination,
                        Word capacity) {
    const ControlState* state = find_control(tmc);
    const char* entry = state == NULL || index >= state->entries.size
                            ? ""
                            : state->entries.data[index];
    copy_text(entry, destination, capacity);
    return (Word) min_size(strlen(entry), 0xffffu);
}
Word IEntryFindListBox(Tmc tmc, char* entry, Word* start_index) {
    (void) start_index;
    const ControlState* state = find_control(tmc);
    if (state == NULL) {
        return ((Word) (-1));
    }
    char* sought = counted_or_zero_terminated(entry);
    const int found = string_list_find(&state->entries, sought);
    free(sought);
    return found < 0 ? ((Word) (-1)) : ((Word) found);
}
Word CselListBoxTmc(Tmc tmc) {
    const ControlState* state = find_control(tmc);
    return state == NULL || state->entries.size == 0 ? 0 : 1;
}
void StartListBoxUpdate(Tmc tmc) { (void) tmc; }
void BeginListBoxUpdate(Tmc tmc, int redraw) {
    (void) tmc;
    (void) redraw;
}
void EndListBoxUpdate(Tmc tmc) { (void) tmc; }

Hcab HcabFromDlg(int dialog) {
    const DialogState* state =
        find_dialog(dialog == 0 ? g_current_dialog : ((Hdlg) (dialog)));
    return state == NULL ? NULL : state->cab;
}
void SaveCabs(void (*callback)(Hcab, Word, Tmc, int), int save) {
    if (callback != NULL && g_dialog.cab != NULL) {
        callback(g_dialog.cab, g_dialog.hid, 0, save);
    }
}

void GetTmcRec(Tmc tmc, Rec* rectangle) {
    if (rectangle == NULL) {
        return;
    }
    const ControlState* state = find_control(tmc);
    *rectangle = state == NULL ? ((Rec) {0, 0, 0, 0}) : state->rectangle;
}

int OpusSdmRenderDialogPreview(unsigned short hid, unsigned int* pixels,
                               int width, int height) {
    return render_dialog_preview(((Word) (hid)), pixels, width,
                                 height);
}

HWND HwndOfTmc(Tmc tmc) { return ensure_control_window(tmc); }
Hdlg HdlgFromHwnd(HWND window) {
    if (window == NULL) {
        return 0;
    }
    for (size_t index = 0; index < g_dialog_count; ++index) {
        DialogState* state = &g_dialogs[index].value;
        if (state->window == window ||
            (state->window != NULL && IsChild(state->window, window))) {
            return g_dialogs[index].key;
        }
    }
    return 0;
}
HWND HwndFromDlg(Hdlg dialog) {
    const DialogState* state = find_dialog(dialog == 0 ? g_current_dialog : dialog);
    return state == NULL ? NULL : state->window;
}
HWND HwndSwapSdmParent(HWND window) {
    HWND previous = vhWndMsgBoxParent;
    vhWndMsgBoxParent = window;
    return previous;
}
int FIsDlgInteractive() { return !g_noninteractive; }
void SetDlgCaption(char* caption) {
    char* copied = counted_or_zero_terminated(caption);
    str_set(&g_dialog.caption, copied);
    free(copied);
    if (g_dialog.window != NULL && IsWindow(g_dialog.window)) {
        SetWindowTextA(g_dialog.window, g_dialog.caption);
    }
}

int FSetNoninteractive(Word hid, Tmc tmc) {
    (void) hid;
    (void) tmc;
    g_noninteractive = true;
    return true;
}
void EndSdmTranscription() { g_noninteractive = false; }
int FExecutable() { return true; }
void CBTState(int state) { (void) state; }
int FRestoreDlg(int dialog) {
    (void) dialog;
    return true;
}
int FRestoreTmc(Tmc tmc, int restore) {
    (void) tmc;
    (void) restore;
    return true;
}

Word IdDoMsgBox(char* text, char* caption, Word flags) {
    return (Word) MessageBoxA(vhWndMsgBoxParent, text, caption,
                              ((UINT) (flags)));
}

Word FlbfFillDirListTmc(char* path, char* pattern, Tmc list_tmc,
                        Tmc edit_tmc, Tmc static_tmc, Word flags,
                        Word attribute) {
    (void) path;
    (void) pattern;
    (void) list_tmc;
    (void) edit_tmc;
    (void) static_tmc;
    (void) flags;
    (void) attribute;
    return 0x0008;  // flbfListingMade; directory enumeration is UI-backed.
}

static void set_sds_handle(Hdlg current, Hdlg focus) {
    sds.current_dialog =
        (void**) ((uintptr_t) current);
    sds.focus_dialog =
        (void**) ((uintptr_t) focus);
}
