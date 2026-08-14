#include "windows.h"

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#define HEAP_MAGIC 0x48454150u
#define GLOBAL_MAGIC 0x474d454du
#define FILE_MAGIC 0x46494c45u
#define FIND_MAGIC 0x46494e44u

static _Thread_local DWORD g_last_error = ERROR_SUCCESS;

typedef struct HeapBlock {
    void* memory;
    SIZE_T size;
} HeapBlock;

typedef struct GlobalBlock {
    DWORD magic;
    void* memory;
    SIZE_T size;
    UINT flags;
    unsigned lock_count;
    WORD token;
    BOOL freed;
} GlobalBlock;

typedef struct GlobalPointer {
    void* memory;
    GlobalBlock* block;
} GlobalPointer;

typedef struct FileHandle {
    DWORD magic;
    FILE* file;
} FileHandle;

typedef struct HFileEntry {
    int value;
    FileHandle* handle;
} HFileEntry;

typedef struct FindEntry {
    char* path;
    WIN32_FIND_DATAA data;
} FindEntry;

typedef struct FindHandle {
    DWORD magic;
    FindEntry* entries;
    size_t count;
    size_t index;
} FindHandle;

static pthread_mutex_t g_heap_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t g_global_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t g_file_lock = PTHREAD_MUTEX_INITIALIZER;
static HeapBlock* g_heap_blocks;
static size_t g_heap_count;
static size_t g_heap_capacity;
static GlobalPointer* g_global_pointers;
static size_t g_global_count;
static size_t g_global_capacity;
static HFileEntry* g_hfiles;
static size_t g_hfile_count;
static size_t g_hfile_capacity;
static int g_next_hfile = 3;
static WORD g_next_global_token = 1;
static UINT g_next_temp_file = 1;

static DWORD error_from_errno(int value) {
    switch (value) {
        case 0: return ERROR_SUCCESS;
        case ENOENT: return ERROR_FILE_NOT_FOUND;
        case ENOTDIR: return ERROR_PATH_NOT_FOUND;
        case EEXIST: return ERROR_ALREADY_EXISTS;
        case EACCES:
        case EPERM: return ERROR_ACCESS_DENIED;
        case EMFILE:
        case ENFILE: return ERROR_TOO_MANY_OPEN_FILES;
        default: return ERROR_INVALID_PARAMETER;
    }
}

static void set_errno_error(void) { g_last_error = error_from_errno(errno); }

static char* xstrdup(const char* text) {
    if (text == NULL) return NULL;
    size_t length = strlen(text) + 1;
    char* copy = (char*)malloc(length);
    if (copy != NULL) memcpy(copy, text, length);
    return copy;
}

static BOOL is_int_resource_pointer(const void* value) {
    return ((uintptr_t)value >> 16u) == 0;
}

static LPSTR map_ansi_case(LPSTR string, int (*map)(int)) {
    if (string == NULL) return NULL;
    if (is_int_resource_pointer(string)) {
        unsigned char ch = (unsigned char)(uintptr_t)string;
        return (LPSTR)(uintptr_t)(unsigned char)map(ch);
    }
    char* cursor;
    for (cursor = string; *cursor != '\0'; ++cursor) {
        *cursor = (char)map((unsigned char)*cursor);
    }
    return string;
}

static int grow_array(void** array, size_t* capacity, size_t item_size) {
    size_t next = *capacity == 0 ? 16 : *capacity * 2;
    void* resized = realloc(*array, next * item_size);
    if (resized == NULL) {
        g_last_error = ERROR_NOT_ENOUGH_MEMORY;
        return 0;
    }
    *array = resized;
    *capacity = next;
    return 1;
}

static char lower_char(char value) {
    return (char)tolower((unsigned char)value);
}

static int ascii_equal_ci(const char* left, const char* right) {
    while (*left != '\0' && *right != '\0') {
        if (lower_char(*left) != lower_char(*right)) return 0;
        ++left;
        ++right;
    }
    return *left == *right;
}

static char* replace_slashes_dup(const char* path) {
    char* text = xstrdup(path);
    if (text == NULL) return NULL;
    char* cursor;
    for (cursor = text; *cursor != '\0'; ++cursor) {
        if (*cursor == '\\') *cursor = '/';
    }
    return text;
}

static char* dirname_dup(const char* path) {
    char* text = xstrdup(path);
    if (text == NULL) return NULL;
    char* slash = strrchr(text, '/');
    if (slash == NULL) {
        strcpy(text, ".");
    } else if (slash == text) {
        slash[1] = '\0';
    } else {
        *slash = '\0';
    }
    return text;
}

static char* join_path(const char* left, const char* right) {
    if (left == NULL || *left == '\0') return xstrdup(right == NULL ? "" : right);
    if (right == NULL || *right == '\0') return xstrdup(left);
    size_t left_len = strlen(left);
    size_t right_len = strlen(right);
    int slash = left[left_len - 1] == '/';
    char* result = (char*)malloc(left_len + (slash ? 0 : 1) + right_len + 1);
    if (result == NULL) return NULL;
    memcpy(result, left, left_len);
    if (!slash) result[left_len++] = '/';
    memcpy(result + left_len, right, right_len + 1);
    return result;
}

static char* c_drive_root(void) {
    const char* env = getenv("OPUS_C_DRIVE");
    if (env != NULL && *env != '\0') return xstrdup(env);

    char* dir = dirname_dup(__FILE__);
    int i;
    for (i = 0; i < 3 && dir != NULL; ++i) {
        char* parent = dirname_dup(dir);
        free(dir);
        dir = parent;
    }
    return dir;
}

static char* normalize_path(const char* path) {
    if (path == NULL || *path == '\0') return xstrdup("");
    char* text = replace_slashes_dup(path);
    if (text == NULL) return NULL;

    if (strlen(text) >= 3 &&
        lower_char(text[0]) == '/' && lower_char(text[1]) == 'u' &&
        text[2] == '/') {
        char* root = c_drive_root();
        char* result = join_path(root, text + 3);
        free(root);
        free(text);
        return result;
    }

    if (strlen(text) >= 2 && text[1] == ':') {
        if (lower_char(text[0]) != 'c') {
            free(text);
            g_last_error = ERROR_PATH_NOT_FOUND;
            return xstrdup("");
        }
        char* root = c_drive_root();
        const char* rest = text + 2;
        if (*rest == '/') ++rest;
        char* result = join_path(root, rest);
        free(root);
        free(text);
        return result;
    }

    if (text[0] == '/') return text;

    char current[PATH_MAX];
    if (getcwd(current, sizeof(current)) == NULL) {
        set_errno_error();
        free(text);
        return xstrdup("");
    }
    char* result = join_path(current, text);
    free(text);
    return result;
}

static char* resolve_case_path(const char* path) {
    struct stat st;
    if (path == NULL || *path == '\0' || stat(path, &st) == 0) return xstrdup(path);

    char* copy = xstrdup(path);
    if (copy == NULL) return NULL;
    char* result = path[0] == '/' ? xstrdup("/") : xstrdup("");
    char* cursor = copy + (path[0] == '/' ? 1 : 0);

    while (result != NULL && cursor != NULL) {
        char* slash = strchr(cursor, '/');
        if (slash != NULL) *slash = '\0';
        if (*cursor != '\0') {
            char* candidate = join_path(result, cursor);
            if (stat(candidate, &st) == 0) {
                free(result);
                result = candidate;
            } else {
                DIR* dir = opendir(*result == '\0' ? "." : result);
                int matched = 0;
                if (dir != NULL) {
                    struct dirent* entry;
                    while ((entry = readdir(dir)) != NULL) {
                        if (ascii_equal_ci(entry->d_name, cursor)) {
                            free(candidate);
                            candidate = join_path(result, entry->d_name);
                            matched = 1;
                            break;
                        }
                    }
                    closedir(dir);
                }
                free(result);
                result = candidate;
                (void)matched;
            }
        }
        if (slash == NULL) break;
        cursor = slash + 1;
    }
    free(copy);
    return result;
}

static char* host_path(const char* path) {
    char* normal = normalize_path(path);
    char* resolved = resolve_case_path(normal);
    free(normal);
    return resolved;
}

static int match_pattern(const char* name, const char* pattern) {
    size_t n = 0, p = 0, star = (size_t)-1, mark = 0;
    while (name[n] != '\0') {
        if (pattern[p] != '\0' &&
            (pattern[p] == '?' || lower_char(pattern[p]) == lower_char(name[n]))) {
            ++n;
            ++p;
        } else if (pattern[p] == '*') {
            star = p++;
            mark = n;
        } else if (star != (size_t)-1) {
            p = star + 1;
            n = ++mark;
        } else {
            return 0;
        }
    }
    while (pattern[p] == '*') ++p;
    return pattern[p] == '\0';
}

static FILETIME filetime_from_time(time_t seconds) {
    const unsigned long long ticks =
        ((unsigned long long)seconds + 11644473600ull) * 10000000ull;
    FILETIME result;
    result.dwLowDateTime = (DWORD)(ticks & 0xffffffffu);
    result.dwHighDateTime = (DWORD)(ticks >> 32u);
    return result;
}

static DWORD attributes_from_status(const char* path) {
    struct stat st;
    if (path == NULL || stat(path, &st) != 0) return INVALID_FILE_ATTRIBUTES;
    DWORD attributes = FILE_ATTRIBUTE_NORMAL;
    if (S_ISDIR(st.st_mode)) attributes |= FILE_ATTRIBUTE_DIRECTORY;
    return attributes;
}

static void fill_find_data_path(const char* path, WIN32_FIND_DATAA* data) {
    memset(data, 0, sizeof(*data));
    data->dwFileAttributes = attributes_from_status(path);
    struct stat st;
    if (stat(path, &st) == 0 && !S_ISDIR(st.st_mode)) {
        data->nFileSizeHigh = (DWORD)(((unsigned long long)st.st_size) >> 32u);
        data->nFileSizeLow = (DWORD)(((unsigned long long)st.st_size) & 0xffffffffu);
    }
    FILETIME now = filetime_from_time(time(NULL));
    data->ftCreationTime = data->ftLastAccessTime = data->ftLastWriteTime = now;
    const char* slash = strrchr(path, '/');
    lstrcpynA(data->cFileName, slash == NULL ? path : slash + 1, MAX_PATH);
}

static FileHandle* file_from_handle(HANDLE handle) {
    if ((uintptr_t)handle < 4096) return NULL;
    FileHandle* file = (FileHandle*)handle;
    return file != NULL && file->magic == FILE_MAGIC ? file : NULL;
}

static pthread_rwlock_t* lock_from_srw(PSRWLOCK lock) {
    static pthread_mutex_t init_lock = PTHREAD_MUTEX_INITIALIZER;
    if (lock == NULL) return NULL;
    if (lock->Ptr == NULL) {
        pthread_mutex_lock(&init_lock);
        if (lock->Ptr == NULL) {
            pthread_rwlock_t* native = (pthread_rwlock_t*)malloc(sizeof(*native));
            if (native != NULL) {
                pthread_rwlock_init(native, NULL);
                lock->Ptr = native;
            }
        }
        pthread_mutex_unlock(&init_lock);
    }
    return (pthread_rwlock_t*)lock->Ptr;
}

static int supported_code_page(UINT code_page) {
    return code_page == CP_ACP || code_page == CP_UTF8 || code_page == 1252;
}

static size_t narrow_count(LPCSTR text, int count) {
    if (text == NULL || count < -1) return 0;
    return count == -1 ? strlen(text) + 1 : (size_t)count;
}

static size_t wide_count(LPCWSTR text, int count) {
    if (text == NULL || count < -1) return 0;
    if (count != -1) return (size_t)count;
    size_t length = 0;
    while (text[length] != 0) ++length;
    return length + 1;
}

static int append_wchar(WCHAR** output, size_t* count, size_t* capacity, WCHAR value) {
    if (*count == *capacity && !grow_array((void**)output, capacity, sizeof(**output))) return 0;
    (*output)[(*count)++] = value;
    return 1;
}

static int append_char(char** output, size_t* count, size_t* capacity, char value) {
    if (*count == *capacity && !grow_array((void**)output, capacity, sizeof(**output))) return 0;
    (*output)[(*count)++] = value;
    return 1;
}

static int append_wide_utf8(WCHAR** output, size_t* count, size_t* capacity,
                            DWORD code_point) {
    if (code_point <= 0xffffu) {
        return append_wchar(output, count, capacity, (WCHAR)code_point);
    }
    code_point -= 0x10000u;
    return append_wchar(output, count, capacity, (WCHAR)(0xd800u + (code_point >> 10u))) &&
           append_wchar(output, count, capacity, (WCHAR)(0xdc00u + (code_point & 0x3ffu)));
}

static int append_utf8(char** output, size_t* count, size_t* capacity,
                       DWORD code_point) {
    if (code_point <= 0x7fu) {
        return append_char(output, count, capacity, (char)code_point);
    }
    if (code_point <= 0x7ffu) {
        return append_char(output, count, capacity, (char)(0xc0u | (code_point >> 6u))) &&
               append_char(output, count, capacity, (char)(0x80u | (code_point & 0x3fu)));
    }
    if (code_point <= 0xffffu) {
        return append_char(output, count, capacity, (char)(0xe0u | (code_point >> 12u))) &&
               append_char(output, count, capacity, (char)(0x80u | ((code_point >> 6u) & 0x3fu))) &&
               append_char(output, count, capacity, (char)(0x80u | (code_point & 0x3fu)));
    }
    return append_char(output, count, capacity, (char)(0xf0u | (code_point >> 18u))) &&
           append_char(output, count, capacity, (char)(0x80u | ((code_point >> 12u) & 0x3fu))) &&
           append_char(output, count, capacity, (char)(0x80u | ((code_point >> 6u) & 0x3fu))) &&
           append_char(output, count, capacity, (char)(0x80u | (code_point & 0x3fu)));
}

static WCHAR* decode_utf8(LPCSTR text, size_t source_count, size_t* out_count) {
    WCHAR* output = NULL;
    size_t count = 0, capacity = 0;
    size_t index;
    for (index = 0; index < source_count;) {
        unsigned char byte = (unsigned char)text[index++];
        DWORD code_point = 0xfffdu;
        unsigned extra = 0;
        if (byte < 0x80u) {
            code_point = byte;
        } else if ((byte & 0xe0u) == 0xc0u) {
            code_point = byte & 0x1fu;
            extra = 1;
        } else if ((byte & 0xf0u) == 0xe0u) {
            code_point = byte & 0x0fu;
            extra = 2;
        } else if ((byte & 0xf8u) == 0xf0u) {
            code_point = byte & 0x07u;
            extra = 3;
        }
        int valid = extra != 0 || byte < 0x80u;
        unsigned part;
        for (part = 0; part < extra; ++part) {
            if (index >= source_count || (((unsigned char)text[index]) & 0xc0u) != 0x80u) {
                valid = 0;
                break;
            }
            code_point = (code_point << 6u) | (((unsigned char)text[index++]) & 0x3fu);
        }
        if (!append_wide_utf8(&output, &count, &capacity, valid ? code_point : 0xfffdu)) {
            free(output);
            *out_count = 0;
            return NULL;
        }
    }
    *out_count = count;
    return output;
}

static char* encode_utf8(LPCWSTR text, size_t source_count, size_t* out_count,
                         BOOL* used_default) {
    char* output = NULL;
    size_t count = 0, capacity = 0;
    size_t index;
    for (index = 0; index < source_count; ++index) {
        DWORD code_point = text[index];
        if (code_point >= 0xd800u && code_point <= 0xdbffu &&
            index + 1 < source_count && text[index + 1] >= 0xdc00u &&
            text[index + 1] <= 0xdfffu) {
            code_point = 0x10000u + (((code_point - 0xd800u) << 10u) |
                                     ((DWORD)text[++index] - 0xdc00u));
        } else if (code_point >= 0xd800u && code_point <= 0xdfffu) {
            code_point = 0xfffdu;
            if (used_default != NULL) *used_default = TRUE;
        }
        if (!append_utf8(&output, &count, &capacity, code_point)) {
            free(output);
            *out_count = 0;
            return NULL;
        }
    }
    *out_count = count;
    return output;
}

static int add_heap_block(void* memory, SIZE_T size) {
    if (g_heap_count == g_heap_capacity &&
        !grow_array((void**)&g_heap_blocks, &g_heap_capacity, sizeof(*g_heap_blocks))) {
        return 0;
    }
    g_heap_blocks[g_heap_count].memory = memory;
    g_heap_blocks[g_heap_count].size = size;
    ++g_heap_count;
    return 1;
}

static ssize_t find_heap_block(void* memory) {
    size_t i;
    for (i = 0; i < g_heap_count; ++i) {
        if (g_heap_blocks[i].memory == memory) return (ssize_t)i;
    }
    return -1;
}

static int add_global_pointer(void* memory, GlobalBlock* block) {
    if (g_global_count == g_global_capacity &&
        !grow_array((void**)&g_global_pointers, &g_global_capacity, sizeof(*g_global_pointers))) {
        return 0;
    }
    g_global_pointers[g_global_count].memory = memory;
    g_global_pointers[g_global_count].block = block;
    ++g_global_count;
    return 1;
}

static void remove_global_pointer(void* memory) {
    size_t i;
    for (i = 0; i < g_global_count; ++i) {
        if (g_global_pointers[i].memory == memory) {
            g_global_pointers[i] = g_global_pointers[--g_global_count];
            return;
        }
    }
}

static GlobalBlock* find_global_pointer(LPCVOID memory) {
    size_t i;
    for (i = 0; i < g_global_count; ++i) {
        if (g_global_pointers[i].memory == memory) return g_global_pointers[i].block;
    }
    return NULL;
}

static HFileEntry* find_hfile(HFILE file) {
    size_t i;
    for (i = 0; i < g_hfile_count; ++i) {
        if (g_hfiles[i].value == file) return &g_hfiles[i];
    }
    return NULL;
}

DWORD GetLastError(void) { return g_last_error; }
VOID SetLastError(DWORD error) { g_last_error = error; }

int MultiByteToWideChar(UINT code_page, DWORD flags, LPCSTR multi_byte,
                        int multi_byte_count, LPWSTR wide_char,
                        int wide_char_count) {
    (void)flags;
    if (!supported_code_page(code_page) || multi_byte == NULL ||
        multi_byte_count < -1 || wide_char_count < 0) {
        g_last_error = ERROR_INVALID_PARAMETER;
        return 0;
    }

    size_t count = narrow_count(multi_byte, multi_byte_count);
    WCHAR* output = NULL;
    size_t output_count = 0;
    if (code_page == CP_UTF8) {
        output = decode_utf8(multi_byte, count, &output_count);
    } else {
        output = (WCHAR*)malloc((count == 0 ? 1 : count) * sizeof(*output));
        if (output != NULL) {
            output_count = count;
            size_t i;
            for (i = 0; i < count; ++i) output[i] = (unsigned char)multi_byte[i];
        }
    }
    if (output == NULL && count != 0) {
        g_last_error = ERROR_NOT_ENOUGH_MEMORY;
        return 0;
    }

    if (wide_char == NULL || wide_char_count == 0) {
        int result = (int)output_count;
        free(output);
        return result;
    }
    if ((size_t)wide_char_count < output_count) {
        free(output);
        g_last_error = ERROR_INSUFFICIENT_BUFFER;
        return 0;
    }
    memcpy(wide_char, output, output_count * sizeof(*output));
    free(output);
    return (int)output_count;
}

int WideCharToMultiByte(UINT code_page, DWORD flags, LPCWSTR wide_char,
                        int wide_char_count, LPSTR multi_byte,
                        int multi_byte_count, LPCSTR default_char,
                        BOOL* used_default_char) {
    (void)flags;
    if (used_default_char != NULL) *used_default_char = FALSE;
    if (!supported_code_page(code_page) || wide_char == NULL ||
        wide_char_count < -1 || multi_byte_count < 0) {
        g_last_error = ERROR_INVALID_PARAMETER;
        return 0;
    }

    size_t count = wide_count(wide_char, wide_char_count);
    char* output = NULL;
    size_t output_count = 0;
    if (code_page == CP_UTF8) {
        output = encode_utf8(wide_char, count, &output_count, used_default_char);
    } else {
        output = (char*)malloc(count == 0 ? 1 : count);
        if (output != NULL) {
            const char fallback = default_char != NULL ? default_char[0] : '?';
            output_count = count;
            size_t i;
            for (i = 0; i < count; ++i) {
                WCHAR value = wide_char[i];
                if (value <= 0xffu) {
                    output[i] = (char)value;
                } else {
                    output[i] = fallback;
                    if (used_default_char != NULL) *used_default_char = TRUE;
                }
            }
        }
    }
    if (output == NULL && count != 0) {
        g_last_error = ERROR_NOT_ENOUGH_MEMORY;
        return 0;
    }

    if (multi_byte == NULL || multi_byte_count == 0) {
        int result = (int)output_count;
        free(output);
        return result;
    }
    if ((size_t)multi_byte_count < output_count) {
        free(output);
        g_last_error = ERROR_INSUFFICIENT_BUFFER;
        return 0;
    }
    memcpy(multi_byte, output, output_count);
    free(output);
    return (int)output_count;
}

VOID OutputDebugStringA(LPCSTR text) {
    if (text != NULL) fputs(text, stderr);
}

DWORD FormatMessageA(DWORD flags, LPCVOID source, DWORD message_id, DWORD language_id,
                     LPSTR buffer, DWORD size, LPVOID arguments) {
    (void)flags;
    (void)source;
    (void)language_id;
    (void)arguments;
    if (buffer == NULL || size == 0) return 0;
    snprintf(buffer, size, "Win32 error %lu", (unsigned long)message_id);
    buffer[size - 1] = '\0';
    return (DWORD)strlen(buffer);
}

DWORD GetModuleFileNameA(HMODULE module, LPSTR file_name, DWORD size) {
    (void)module;
    if (file_name == NULL || size == 0) return 0;
    char path[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", path, sizeof(path) - 1);
    if (count < 0) {
        if (getcwd(path, sizeof(path)) == NULL) path[0] = '\0';
    } else {
        path[count] = '\0';
    }
    lstrcpynA(file_name, path, (int)size);
    return (DWORD)strlen(file_name);
}

UINT GetProfileIntA(LPCSTR application_name, LPCSTR key_name, int default_value) {
    (void)application_name;
    (void)key_name;
    return (UINT)default_value;
}

DWORD GetProfileStringA(LPCSTR application_name, LPCSTR key_name,
                        LPCSTR default_value, LPSTR returned_string,
                        DWORD size) {
    (void)application_name;
    (void)key_name;
    if (returned_string == NULL || size == 0) return 0;
    lstrcpynA(returned_string, default_value != NULL ? default_value : "",
              (int)size);
    return (DWORD)strlen(returned_string);
}

BOOL WriteProfileStringA(LPCSTR application_name, LPCSTR key_name,
                         LPCSTR string) {
    (void)application_name;
    (void)key_name;
    (void)string;
    return TRUE;
}

LPSTR AnsiUpper(LPSTR string) { return map_ansi_case(string, toupper); }

LPSTR AnsiLower(LPSTR string) { return map_ansi_case(string, tolower); }

LPSTR AnsiNext(LPCSTR current) {
    return current == NULL ? NULL : (LPSTR)(current + 1);
}

LPSTR AnsiPrev(LPCSTR start, LPCSTR current) {
    if (start == NULL || current == NULL || current <= start) return (LPSTR)start;
    return (LPSTR)(current - 1);
}

BOOL AnsiToOem(LPCSTR ansi, LPSTR oem) {
    if (ansi == NULL || oem == NULL) return FALSE;
    if (ansi != oem) strcpy(oem, ansi);
    return TRUE;
}

BOOL OemToAnsi(LPCSTR oem, LPSTR ansi) {
    if (oem == NULL || ansi == NULL) return FALSE;
    if (oem != ansi) strcpy(ansi, oem);
    return TRUE;
}

HANDLE GetCurrentProcess(void) { return (HANDLE)(uintptr_t)2; }
DWORD GetCurrentProcessId(void) { return (DWORD)getpid(); }
DWORD GetCurrentThreadId(void) { return 1; }

BOOL GlobalMemoryStatusEx(LPMEMORYSTATUSEX buffer) {
    if (buffer == NULL || buffer->dwLength != sizeof(MEMORYSTATUSEX)) {
        g_last_error = ERROR_INVALID_PARAMETER;
        return FALSE;
    }
    const DWORDLONG total = 1024ull * 1024ull * 1024ull;
    buffer->dwMemoryLoad = 50;
    buffer->ullTotalPhys = total;
    buffer->ullAvailPhys = total / 2;
    buffer->ullTotalPageFile = total;
    buffer->ullAvailPageFile = total / 2;
    buffer->ullTotalVirtual = total;
    buffer->ullAvailVirtual = total / 2;
    buffer->ullAvailExtendedVirtual = 0;
    return TRUE;
}

BOOL TerminateProcess(HANDLE process, UINT exit_code) {
    (void)process;
    exit((int)exit_code);
}

VOID RaiseException(DWORD code, DWORD flags, DWORD argument_count,
                    const ULONG_PTR* arguments) {
    (void)code;
    (void)flags;
    (void)argument_count;
    (void)arguments;
    abort();
}

USHORT RtlCaptureStackBackTrace(ULONG frames_to_skip, ULONG frames_to_capture,
                                PVOID* back_trace, ULONG* back_trace_hash) {
    return CaptureStackBackTrace(frames_to_skip, frames_to_capture, back_trace,
                                 back_trace_hash);
}

LPSTR lstrcpynA(LPSTR destination, LPCSTR source, int count) {
    if (destination == NULL || count <= 0) return destination;
    if (source == NULL) {
        destination[0] = '\0';
        return destination;
    }
    strncpy(destination, source, (size_t)count);
    destination[count - 1] = '\0';
    return destination;
}

LPWSTR lstrcpyW(LPWSTR destination, LPCWSTR source) {
    if (destination == NULL) return destination;
    WCHAR* out = destination;
    if (source != NULL) {
        while ((*out++ = *source++) != 0) {}
    } else {
        *out = 0;
    }
    return destination;
}

LPSTR CharLowerA(LPSTR string) {
    uintptr_t value = (uintptr_t)string;
    if ((value >> 16u) == 0) {
        return (LPSTR)(uintptr_t)tolower((unsigned char)(value & 0xffu));
    }
    char* cursor;
    for (cursor = string; cursor != NULL && *cursor != '\0'; ++cursor) {
        *cursor = (char)tolower((unsigned char)*cursor);
    }
    return string;
}

DWORD CharUpperBuffA(LPSTR text, DWORD length) {
    if (text == NULL) return 0;
    DWORD index;
    for (index = 0; index < length; ++index) {
        text[index] = (char)toupper((unsigned char)text[index]);
    }
    return length;
}

BOOL GetStringTypeA(DWORD locale, DWORD type, LPCSTR source, int count,
                    WORD* char_type) {
    (void)locale;
    if (type != CT_CTYPE1 || source == NULL || char_type == NULL) return FALSE;
    int index;
    for (index = 0; index < count; ++index) {
        unsigned char ch = (unsigned char)source[index];
        WORD flags = 0;
        BOOL upper = isupper(ch) || (ch >= 0xc0 && ch <= 0xd6) ||
                     (ch >= 0xd8 && ch <= 0xde);
        BOOL lower = islower(ch) || (ch >= 0xdf && ch <= 0xf6) ||
                     (ch >= 0xf8 && ch <= 0xff);
        if (upper) flags |= C1_UPPER | C1_ALPHA;
        if (lower) flags |= C1_LOWER | C1_ALPHA;
        if (isdigit(ch)) flags |= C1_DIGIT;
        char_type[index] = flags;
    }
    return TRUE;
}

int MulDiv(int number, int numerator, int denominator) {
    if (denominator == 0) return number >= 0 ? INT_MAX : INT_MIN;
    long long value = (long long)number * numerator;
    long long adjusted = value >= 0 ? value + denominator / 2 : value - denominator / 2;
    return (int)(adjusted / denominator);
}

HANDLE GetProcessHeap(void) { return (HANDLE)(uintptr_t)1; }

LPVOID HeapAlloc(HANDLE heap, DWORD flags, SIZE_T bytes) {
    (void)heap;
    SIZE_T actual = bytes == 0 ? 1 : bytes;
    void* memory = malloc(actual);
    if (memory == NULL) {
        g_last_error = ERROR_NOT_ENOUGH_MEMORY;
        return NULL;
    }
    if ((flags & HEAP_ZERO_MEMORY) != 0) memset(memory, 0, actual);
    pthread_mutex_lock(&g_heap_lock);
    if (!add_heap_block(memory, actual)) {
        pthread_mutex_unlock(&g_heap_lock);
        free(memory);
        return NULL;
    }
    pthread_mutex_unlock(&g_heap_lock);
    return memory;
}

LPVOID HeapReAlloc(HANDLE heap, DWORD flags, LPVOID memory, SIZE_T bytes) {
    (void)heap;
    if (memory == NULL) return HeapAlloc(NULL, flags, bytes);
    SIZE_T actual = bytes == 0 ? 1 : bytes;
    pthread_mutex_lock(&g_heap_lock);
    ssize_t found = find_heap_block(memory);
    if (found < 0) {
        pthread_mutex_unlock(&g_heap_lock);
        g_last_error = ERROR_INVALID_HANDLE;
        return NULL;
    }
    SIZE_T old_size = g_heap_blocks[found].size;
    void* resized = realloc(memory, actual);
    if (resized == NULL) {
        pthread_mutex_unlock(&g_heap_lock);
        g_last_error = ERROR_NOT_ENOUGH_MEMORY;
        return NULL;
    }
    if ((flags & HEAP_ZERO_MEMORY) != 0 && actual > old_size) {
        memset((unsigned char*)resized + old_size, 0, actual - old_size);
    }
    g_heap_blocks[found].memory = resized;
    g_heap_blocks[found].size = actual;
    pthread_mutex_unlock(&g_heap_lock);
    return resized;
}

BOOL HeapFree(HANDLE heap, DWORD flags, LPVOID memory) {
    (void)heap;
    (void)flags;
    if (memory == NULL) return TRUE;
    pthread_mutex_lock(&g_heap_lock);
    ssize_t found = find_heap_block(memory);
    if (found < 0) {
        pthread_mutex_unlock(&g_heap_lock);
        g_last_error = ERROR_INVALID_HANDLE;
        return FALSE;
    }
    g_heap_blocks[found] = g_heap_blocks[--g_heap_count];
    pthread_mutex_unlock(&g_heap_lock);
    free(memory);
    return TRUE;
}

SIZE_T HeapSize(HANDLE heap, DWORD flags, LPCVOID memory) {
    (void)heap;
    (void)flags;
    pthread_mutex_lock(&g_heap_lock);
    ssize_t found = find_heap_block((void*)memory);
    SIZE_T size = found < 0 ? (SIZE_T)-1 : g_heap_blocks[found].size;
    pthread_mutex_unlock(&g_heap_lock);
    return size;
}

SIZE_T HeapCompact(HANDLE heap, DWORD flags) {
    (void)heap;
    (void)flags;
    return 16u * 1024u * 1024u;
}

HANDLE GlobalAlloc(UINT flags, SIZE_T bytes) {
    GlobalBlock* block = (GlobalBlock*)calloc(1, sizeof(*block));
    if (block == NULL) {
        g_last_error = ERROR_NOT_ENOUGH_MEMORY;
        return NULL;
    }
    block->magic = GLOBAL_MAGIC;
    block->size = bytes;
    block->flags = flags;
    block->memory = malloc(bytes == 0 ? 1 : bytes);
    if (block->memory == NULL) {
        free(block);
        g_last_error = ERROR_NOT_ENOUGH_MEMORY;
        return NULL;
    }
    if ((flags & GMEM_ZEROINIT) != 0) memset(block->memory, 0, bytes == 0 ? 1 : bytes);

    pthread_mutex_lock(&g_global_lock);
    block->token = g_next_global_token++;
    if (g_next_global_token == 0) g_next_global_token = 1;
    if (!add_global_pointer(block->memory, block)) {
        pthread_mutex_unlock(&g_global_lock);
        free(block->memory);
        free(block);
        return NULL;
    }
    pthread_mutex_unlock(&g_global_lock);
    return (HANDLE)block;
}

LPVOID GlobalLock(HANDLE memory) {
    GlobalBlock* block = (GlobalBlock*)memory;
    if (block == NULL || block->magic != GLOBAL_MAGIC || block->freed) {
        g_last_error = ERROR_INVALID_HANDLE;
        return NULL;
    }
    ++block->lock_count;
    return block->memory;
}

BOOL GlobalUnlock(HANDLE memory) {
    GlobalBlock* block = (GlobalBlock*)memory;
    if (block == NULL || block->magic != GLOBAL_MAGIC || block->freed) {
        g_last_error = ERROR_INVALID_HANDLE;
        return FALSE;
    }
    if (block->lock_count == 0) {
        g_last_error = ERROR_SUCCESS;
        return FALSE;
    }
    --block->lock_count;
    g_last_error = block->lock_count == 0 ? ERROR_SUCCESS : ERROR_INVALID_PARAMETER;
    return block->lock_count != 0;
}

HANDLE GlobalFree(HANDLE memory) {
    GlobalBlock* block = (GlobalBlock*)memory;
    if (block == NULL) return NULL;
    if (block->magic != GLOBAL_MAGIC || block->freed) {
        g_last_error = ERROR_INVALID_HANDLE;
        return memory;
    }
    pthread_mutex_lock(&g_global_lock);
    remove_global_pointer(block->memory);
    pthread_mutex_unlock(&g_global_lock);
    free(block->memory);
    block->freed = TRUE;
    block->memory = NULL;
    block->size = 0;
    return NULL;
}

HANDLE GlobalReAlloc(HANDLE memory, SIZE_T bytes, UINT flags) {
    GlobalBlock* block = (GlobalBlock*)memory;
    if (block == NULL || block->magic != GLOBAL_MAGIC || block->freed) {
        g_last_error = ERROR_INVALID_HANDLE;
        return NULL;
    }
    void* resized = realloc(block->memory, bytes == 0 ? 1 : bytes);
    if (resized == NULL) {
        g_last_error = ERROR_NOT_ENOUGH_MEMORY;
        return NULL;
    }
    if ((flags & GMEM_ZEROINIT) != 0 && bytes > block->size) {
        memset((unsigned char*)resized + block->size, 0, bytes - block->size);
    }
    pthread_mutex_lock(&g_global_lock);
    remove_global_pointer(block->memory);
    block->memory = resized;
    block->size = bytes;
    block->flags = flags;
    add_global_pointer(block->memory, block);
    pthread_mutex_unlock(&g_global_lock);
    return memory;
}

SIZE_T GlobalSize(HANDLE memory) {
    GlobalBlock* block = (GlobalBlock*)memory;
    return block == NULL || block->magic != GLOBAL_MAGIC || block->freed ? 0 : block->size;
}

DWORD GlobalHandle(LPCVOID memory) {
    pthread_mutex_lock(&g_global_lock);
    GlobalBlock* block = find_global_pointer(memory);
    DWORD value = block == NULL ? 0 : ((DWORD)block->token << 16u);
    pthread_mutex_unlock(&g_global_lock);
    return value;
}

DWORD GlobalCompact(DWORD min_free) {
    (void)min_free;
    return 16u * 1024u * 1024u;
}

LPVOID GlobalWire(HANDLE memory) { return GlobalLock(memory); }

UINT GlobalFlags(HANDLE memory) {
    GlobalBlock* block = (GlobalBlock*)memory;
    if (block == NULL || block->magic != GLOBAL_MAGIC || block->freed) {
        g_last_error = ERROR_INVALID_HANDLE;
        return GMEM_INVALID_HANDLE;
    }
    return (block->flags & ~GMEM_LOCKCOUNT) |
           (block->lock_count > 0xffu ? 0xffu : block->lock_count);
}

HLOCAL LocalAlloc(UINT flags, SIZE_T bytes) { return (HLOCAL)GlobalAlloc(flags, bytes); }
HLOCAL LocalFree(HLOCAL memory) { return (HLOCAL)GlobalFree(memory); }
HLOCAL LocalReAlloc(HLOCAL memory, SIZE_T bytes, UINT flags) {
    return (HLOCAL)GlobalReAlloc(memory, bytes, flags);
}

HANDLE CreateFileA(LPCSTR file_name, DWORD desired_access, DWORD share_mode,
                   LPVOID security_attributes, DWORD creation_disposition,
                   DWORD flags_and_attributes, HANDLE base_handle) {
    (void)share_mode;
    (void)security_attributes;
    (void)flags_and_attributes;
    (void)base_handle;
    char* path = host_path(file_name);
    if (path == NULL) return INVALID_HANDLE_VALUE;
    BOOL write = (desired_access & (GENERIC_WRITE | FILE_APPEND_DATA)) != 0;
    BOOL read = (desired_access & GENERIC_READ) != 0 || !write;
    struct stat st;
    int exists = stat(path, &st) == 0;
    const char* mode = "rb";
    if (creation_disposition == CREATE_ALWAYS) {
        mode = read ? "w+b" : "wb";
    } else if (creation_disposition == OPEN_ALWAYS) {
        mode = exists ? (read ? "r+b" : "ab") : (read ? "w+b" : "wb");
    } else if (creation_disposition == CREATE_NEW) {
        if (exists) {
            free(path);
            g_last_error = ERROR_FILE_EXISTS;
            return INVALID_HANDLE_VALUE;
        }
        mode = read ? "w+b" : "wb";
    } else if (write) {
        mode = read ? "r+b" : "ab";
    }
    FILE* file = fopen(path, mode);
    free(path);
    if (file == NULL) {
        set_errno_error();
        return INVALID_HANDLE_VALUE;
    }
    if ((desired_access & FILE_APPEND_DATA) != 0) fseek(file, 0, SEEK_END);
    FileHandle* handle = (FileHandle*)malloc(sizeof(*handle));
    if (handle == NULL) {
        fclose(file);
        g_last_error = ERROR_NOT_ENOUGH_MEMORY;
        return INVALID_HANDLE_VALUE;
    }
    handle->magic = FILE_MAGIC;
    handle->file = file;
    return (HANDLE)handle;
}

BOOL ReadFile(HANDLE handle, LPVOID buffer, DWORD bytes_to_read,
              DWORD* bytes_read, LPVOID overlapped) {
    (void)overlapped;
    FileHandle* file = file_from_handle(handle);
    if (file == NULL || buffer == NULL) {
        g_last_error = ERROR_INVALID_HANDLE;
        return FALSE;
    }
    size_t count = fread(buffer, 1, bytes_to_read, file->file);
    if (bytes_read != NULL) *bytes_read = (DWORD)count;
    if (count < bytes_to_read && ferror(file->file)) {
        set_errno_error();
        return FALSE;
    }
    return TRUE;
}

BOOL WriteFile(HANDLE handle, LPCVOID buffer, DWORD bytes_to_write,
               DWORD* bytes_written, LPVOID overlapped) {
    (void)overlapped;
    FileHandle* file = file_from_handle(handle);
    if (file == NULL || buffer == NULL) {
        g_last_error = ERROR_INVALID_HANDLE;
        return FALSE;
    }
    size_t count = fwrite(buffer, 1, bytes_to_write, file->file);
    if (bytes_written != NULL) *bytes_written = (DWORD)count;
    if (count != bytes_to_write) {
        set_errno_error();
        return FALSE;
    }
    return TRUE;
}

BOOL FlushFileBuffers(HANDLE handle) {
    FileHandle* file = file_from_handle(handle);
    return file != NULL && fflush(file->file) == 0;
}

BOOL CloseHandle(HANDLE object) {
    FileHandle* file = file_from_handle(object);
    if (file == NULL) return TRUE;
    int result = fclose(file->file);
    file->magic = 0;
    free(file);
    if (result != 0) {
        set_errno_error();
        return FALSE;
    }
    return TRUE;
}

DWORD WaitForSingleObject(HANDLE handle, DWORD milliseconds) {
    (void)handle;
    (void)milliseconds;
    return WAIT_OBJECT_0;
}

DWORD GetFileType(HANDLE handle) {
    return file_from_handle(handle) != NULL ||
        find_hfile((HFILE)(uintptr_t)handle) != NULL ? FILE_TYPE_DISK : 0;
}

HFILE OpenFile(LPCSTR file_name, LPOFSTRUCT reopen_buffer, UINT style) {
    DWORD access = (style & OF_WRITE) != 0 ? GENERIC_WRITE : GENERIC_READ;
    DWORD disposition = (style & OF_CREATE) != 0 ? CREATE_ALWAYS : OPEN_EXISTING;
    HANDLE handle = CreateFileA(file_name, access, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                NULL, disposition, FILE_ATTRIBUTE_NORMAL, NULL);
    if (handle == INVALID_HANDLE_VALUE) {
        if (reopen_buffer != NULL) reopen_buffer->nErrCode = (WORD)g_last_error;
        return HFILE_ERROR;
    }

    pthread_mutex_lock(&g_file_lock);
    if (g_hfile_count == g_hfile_capacity &&
        !grow_array((void**)&g_hfiles, &g_hfile_capacity, sizeof(*g_hfiles))) {
        pthread_mutex_unlock(&g_file_lock);
        CloseHandle(handle);
        return HFILE_ERROR;
    }
    int value = g_next_hfile++;
    g_hfiles[g_hfile_count].value = value;
    g_hfiles[g_hfile_count].handle = (FileHandle*)handle;
    ++g_hfile_count;
    pthread_mutex_unlock(&g_file_lock);

    if (reopen_buffer != NULL) {
        reopen_buffer->cBytes = sizeof(OFSTRUCT);
        reopen_buffer->fFixedDisk = TRUE;
        reopen_buffer->nErrCode = 0;
        lstrcpynA(reopen_buffer->szPathName, file_name, sizeof(reopen_buffer->szPathName));
    }
    return value;
}

int _lclose(HFILE file) {
    pthread_mutex_lock(&g_file_lock);
    HFileEntry* found = find_hfile(file);
    if (found == NULL) {
        pthread_mutex_unlock(&g_file_lock);
        return HFILE_ERROR;
    }
    FileHandle* handle = found->handle;
    *found = g_hfiles[--g_hfile_count];
    pthread_mutex_unlock(&g_file_lock);
    int result = fclose(handle->file);
    handle->magic = 0;
    free(handle);
    return result == 0 ? 0 : HFILE_ERROR;
}

UINT _lread(HFILE file, LPVOID buffer, UINT bytes) {
    pthread_mutex_lock(&g_file_lock);
    HFileEntry* found = find_hfile(file);
    UINT result = found == NULL ? HFILE_ERROR : (UINT)fread(buffer, 1, bytes, found->handle->file);
    pthread_mutex_unlock(&g_file_lock);
    return result;
}

UINT _lwrite(HFILE file, LPCCH buffer, UINT bytes) {
    pthread_mutex_lock(&g_file_lock);
    HFileEntry* found = find_hfile(file);
    UINT result = found == NULL ? HFILE_ERROR : (UINT)fwrite(buffer, 1, bytes, found->handle->file);
    pthread_mutex_unlock(&g_file_lock);
    return result;
}

LONG _llseek(HFILE file, LONG offset, int origin) {
    pthread_mutex_lock(&g_file_lock);
    HFileEntry* found = find_hfile(file);
    if (found == NULL || fseek(found->handle->file, offset, origin) != 0) {
        pthread_mutex_unlock(&g_file_lock);
        return HFILE_ERROR;
    }
    LONG result = (LONG)ftell(found->handle->file);
    pthread_mutex_unlock(&g_file_lock);
    return result;
}

DWORD GetFileAttributesA(LPCSTR file_name) {
    char* path = host_path(file_name);
    DWORD attributes = attributes_from_status(path);
    free(path);
    if (attributes == INVALID_FILE_ATTRIBUTES) set_errno_error();
    return attributes;
}

BOOL GetFileAttributesExA(LPCSTR file_name, int level, LPVOID file_information) {
    (void)level;
    if (file_information == NULL) return FALSE;
    char* path = host_path(file_name);
    WIN32_FILE_ATTRIBUTE_DATA* data = (WIN32_FILE_ATTRIBUTE_DATA*)file_information;
    memset(data, 0, sizeof(*data));
    data->dwFileAttributes = attributes_from_status(path);
    if (data->dwFileAttributes == INVALID_FILE_ATTRIBUTES) {
        free(path);
        set_errno_error();
        return FALSE;
    }
    struct stat st;
    if (stat(path, &st) == 0 && !S_ISDIR(st.st_mode)) {
        data->nFileSizeHigh = (DWORD)(((unsigned long long)st.st_size) >> 32u);
        data->nFileSizeLow = (DWORD)(((unsigned long long)st.st_size) & 0xffffffffu);
    }
    FILETIME now = filetime_from_time(time(NULL));
    data->ftCreationTime = data->ftLastAccessTime = data->ftLastWriteTime = now;
    free(path);
    return TRUE;
}

BOOL CreateDirectoryA(LPCSTR path_name, LPVOID security_attributes) {
    (void)security_attributes;
    char* path = normalize_path(path_name);
    int result = mkdir(path, 0777);
    free(path);
    if (result == 0) return TRUE;
    set_errno_error();
    return FALSE;
}

BOOL RemoveDirectoryA(LPCSTR path_name) {
    char* path = host_path(path_name);
    int result = rmdir(path);
    free(path);
    if (result == 0) return TRUE;
    set_errno_error();
    return FALSE;
}

BOOL DeleteFileA(LPCSTR file_name) {
    char* path = host_path(file_name);
    int result = unlink(path);
    free(path);
    if (result == 0) return TRUE;
    set_errno_error();
    return FALSE;
}

DWORD GetCurrentDirectoryA(DWORD buffer_length, LPSTR buffer) {
    char current[PATH_MAX];
    if (getcwd(current, sizeof(current)) == NULL) {
        set_errno_error();
        return 0;
    }
    if (buffer != NULL && buffer_length != 0) lstrcpynA(buffer, current, (int)buffer_length);
    return (DWORD)strlen(current);
}

DWORD GetFullPathNameA(LPCSTR file_name, DWORD buffer_length, LPSTR buffer,
                       LPSTR* file_part) {
    char* full = host_path(file_name);
    if (full == NULL) return 0;
    if (buffer != NULL && buffer_length != 0) lstrcpynA(buffer, full, (int)buffer_length);
    if (file_part != NULL && buffer != NULL) {
        char* slash = strrchr(buffer, '/');
        *file_part = slash == NULL ? buffer : slash + 1;
    }
    DWORD length = (DWORD)strlen(full);
    free(full);
    return length;
}

DWORD GetTempPathA(DWORD buffer_length, LPSTR buffer) {
    const char* temp = getenv("TMPDIR");
    const char* path = temp != NULL ? temp : "/tmp/";
    if (buffer != NULL && buffer_length != 0) lstrcpynA(buffer, path, (int)buffer_length);
    return (DWORD)strlen(path);
}

UINT GetTempFileNameA(LPCSTR path_name, LPCSTR prefix, UINT unique,
                      LPSTR file_name) {
    if (path_name == NULL || prefix == NULL || file_name == NULL) {
        g_last_error = ERROR_INVALID_PARAMETER;
        return 0;
    }
    UINT value = unique != 0 ? unique : g_next_temp_file++;
    char suffix[16];
    snprintf(suffix, sizeof(suffix), "%04X.TMP", value & 0xffffu);
    char prefix_text[4] = {0, 0, 0, 0};
    strncpy(prefix_text, prefix, 3);
    char* path = normalize_path(path_name);
    char name[32];
    snprintf(name, sizeof(name), "%s%s", prefix_text, suffix);
    char* full = join_path(path, name);
    free(path);
    if (full == NULL || strlen(full) >= MAX_PATH) {
        free(full);
        g_last_error = ERROR_INSUFFICIENT_BUFFER;
        return 0;
    }
    strcpy(file_name, full);
    if (unique == 0) {
        FILE* file = fopen(full, "wb");
        if (file == NULL) {
            free(full);
            set_errno_error();
            return 0;
        }
        fclose(file);
    }
    free(full);
    return value;
}

BOOL CopyFileA(LPCSTR existing_file_name, LPCSTR new_file_name, BOOL fail_if_exists) {
    char* from = host_path(existing_file_name);
    char* to = normalize_path(new_file_name);
    struct stat st;
    if (fail_if_exists && stat(to, &st) == 0) {
        free(from);
        free(to);
        g_last_error = ERROR_FILE_EXISTS;
        return FALSE;
    }
    FILE* input = fopen(from, "rb");
    FILE* output = input == NULL ? NULL : fopen(to, "wb");
    BOOL ok = TRUE;
    if (input == NULL || output == NULL) {
        ok = FALSE;
        set_errno_error();
    } else {
        char buffer[8192];
        size_t count;
        while ((count = fread(buffer, 1, sizeof(buffer), input)) != 0) {
            if (fwrite(buffer, 1, count, output) != count) {
                ok = FALSE;
                set_errno_error();
                break;
            }
        }
        if (ferror(input)) {
            ok = FALSE;
            set_errno_error();
        }
    }
    if (input != NULL) fclose(input);
    if (output != NULL) fclose(output);
    free(from);
    free(to);
    return ok;
}

BOOL MoveFileA(LPCSTR existing_file_name, LPCSTR new_file_name) {
    char* from = host_path(existing_file_name);
    char* to = normalize_path(new_file_name);
    int result = rename(from, to);
    free(from);
    free(to);
    if (result == 0) return TRUE;
    set_errno_error();
    return FALSE;
}

BOOL MoveFileExA(LPCSTR existing_file_name, LPCSTR new_file_name, DWORD flags) {
    if ((flags & MOVEFILE_REPLACE_EXISTING) != 0) {
        char* to = normalize_path(new_file_name);
        unlink(to);
        free(to);
    }
    return MoveFileA(existing_file_name, new_file_name);
}

BOOL ReplaceFileA(LPCSTR replaced_file_name, LPCSTR replacement_file_name,
                  LPCSTR backup_file_name, DWORD flags, LPVOID exclude,
                  LPVOID reserved) {
    (void)backup_file_name;
    (void)flags;
    (void)exclude;
    (void)reserved;
    return MoveFileExA(replacement_file_name, replaced_file_name, MOVEFILE_REPLACE_EXISTING);
}

HANDLE FindFirstFileA(LPCSTR file_name, LPWIN32_FIND_DATAA find_file_data) {
    char* path = normalize_path(file_name);
    if (path == NULL) return INVALID_HANDLE_VALUE;
    char* slash = strrchr(path, '/');
    char* pattern = slash == NULL ? path : slash + 1;
    char* directory;
    if (slash == NULL) {
        directory = xstrdup(".");
    } else if (slash == path) {
        directory = xstrdup("/");
    } else {
        *slash = '\0';
        directory = resolve_case_path(path);
    }
    FindHandle* find = (FindHandle*)calloc(1, sizeof(*find));
    if (find == NULL) {
        free(directory);
        free(path);
        g_last_error = ERROR_NOT_ENOUGH_MEMORY;
        return INVALID_HANDLE_VALUE;
    }
    find->magic = FIND_MAGIC;

    DIR* dir = opendir(directory);
    if (dir != NULL) {
        struct dirent* entry;
        while ((entry = readdir(dir)) != NULL) {
            if (!match_pattern(entry->d_name, pattern)) continue;
            char* full = join_path(directory, entry->d_name);
            if (full == NULL) continue;
            if (find->count == find->index &&
                !grow_array((void**)&find->entries, &find->index, sizeof(*find->entries))) {
                free(full);
                continue;
            }
            find->entries[find->count].path = full;
            fill_find_data_path(full, &find->entries[find->count].data);
            ++find->count;
        }
        closedir(dir);
    }
    free(directory);
    free(path);
    find->index = 0;
    if (find->count == 0) {
        free(find);
        g_last_error = ERROR_NO_MORE_FILES;
        return INVALID_HANDLE_VALUE;
    }
    if (find_file_data != NULL) *find_file_data = find->entries[0].data;
    return (HANDLE)find;
}

BOOL FindNextFileA(HANDLE find_file, LPWIN32_FIND_DATAA find_file_data) {
    FindHandle* find = (FindHandle*)find_file;
    if (find == NULL || find->magic != FIND_MAGIC || ++find->index >= find->count) {
        g_last_error = ERROR_NO_MORE_FILES;
        return FALSE;
    }
    if (find_file_data != NULL) *find_file_data = find->entries[find->index].data;
    return TRUE;
}

BOOL FindClose(HANDLE find_file) {
    FindHandle* find = (FindHandle*)find_file;
    if (find == NULL || find->magic != FIND_MAGIC) return FALSE;
    size_t i;
    for (i = 0; i < find->count; ++i) free(find->entries[i].path);
    free(find->entries);
    find->magic = 0;
    free(find);
    return TRUE;
}

VOID GetLocalTime(SYSTEMTIME* system_time) {
    if (system_time == NULL) return;
    time_t now = time(NULL);
    struct tm local;
    localtime_r(&now, &local);
    system_time->wYear = (WORD)(local.tm_year + 1900);
    system_time->wMonth = (WORD)(local.tm_mon + 1);
    system_time->wDayOfWeek = (WORD)local.tm_wday;
    system_time->wDay = (WORD)local.tm_mday;
    system_time->wHour = (WORD)local.tm_hour;
    system_time->wMinute = (WORD)local.tm_min;
    system_time->wSecond = (WORD)local.tm_sec;
    system_time->wMilliseconds = 0;
}

ULONGLONG GetTickCount64(void) {
    struct timeval now;
    gettimeofday(&now, NULL);
    return (ULONGLONG)now.tv_sec * 1000ull + (ULONGLONG)now.tv_usec / 1000ull;
}

DWORD GetTickCount(void) { return (DWORD)GetTickCount64(); }

VOID Sleep(DWORD milliseconds) {
#ifdef __EMSCRIPTEN__
    emscripten_sleep(milliseconds);
#else
    usleep(milliseconds * 1000u);
#endif
}

BOOL FileTimeToLocalFileTime(const FILETIME* file_time, FILETIME* local_file_time) {
    if (file_time == NULL || local_file_time == NULL) return FALSE;
    *local_file_time = *file_time;
    return TRUE;
}

BOOL FileTimeToDosDateTime(const FILETIME* file_time, WORD* fat_date, WORD* fat_time) {
    (void)file_time;
    if (fat_date != NULL) *fat_date = 0;
    if (fat_time != NULL) *fat_time = 0;
    return TRUE;
}

BOOL GetDiskFreeSpaceExA(LPCSTR directory_name, ULARGE_INTEGER* free_bytes_available,
                         ULARGE_INTEGER* total_number_of_bytes,
                         ULARGE_INTEGER* total_number_of_free_bytes) {
    (void)directory_name;
    const ULONGLONG bytes = 1024ull * 1024ull * 1024ull;
    if (free_bytes_available != NULL) free_bytes_available->QuadPart = bytes;
    if (total_number_of_bytes != NULL) total_number_of_bytes->QuadPart = bytes;
    if (total_number_of_free_bytes != NULL) total_number_of_free_bytes->QuadPart = bytes;
    return TRUE;
}

UINT GetDriveTypeA(LPCSTR root_path_name) {
    (void)root_path_name;
    return DRIVE_REMOTE;
}

LONG InterlockedCompareExchange(volatile LONG* destination, LONG exchange, LONG comparand) {
    return __sync_val_compare_and_swap(destination, comparand, exchange);
}

LONG InterlockedExchange(volatile LONG* target, LONG value) {
    return __sync_lock_test_and_set(target, value);
}

LPWSTR GetCommandLineW(void) {
    static WCHAR command[] = {'W', 'O', 'R', 'D', '1', 0};
    return command;
}

DWORD GetEnvironmentVariableA(LPCSTR name, LPSTR buffer, DWORD size) {
    if (name == NULL) return 0;
    const char* value = getenv(name);
    if (value == NULL) return 0;
    DWORD needed = (DWORD)strlen(value);
    if (buffer != NULL && size != 0) lstrcpynA(buffer, value, (int)size);
    return needed >= size ? needed + 1 : needed;
}

BOOL SetEnvironmentVariableA(LPCSTR name, LPCSTR value) {
    if (name == NULL) return FALSE;
    if (value == NULL) return unsetenv(name) == 0 ? TRUE : FALSE;
    return setenv(name, value, 1) == 0 ? TRUE : FALSE;
}

USHORT CaptureStackBackTrace(DWORD frames_to_skip, DWORD frames_to_capture,
                             PVOID* back_trace, DWORD* back_trace_hash) {
    (void)frames_to_skip;
    (void)frames_to_capture;
    (void)back_trace;
    (void)back_trace_hash;
    return 0;
}

VOID GetCurrentThreadStackLimits(ULONG_PTR* low_limit, ULONG_PTR* high_limit) {
    if (low_limit != NULL) *low_limit = 0;
    if (high_limit != NULL) *high_limit = ~(ULONG_PTR)0;
}

BOOL SetDefaultDllDirectories(DWORD directory_flags) {
    (void)directory_flags;
    return TRUE;
}

BOOL SetSearchPathMode(DWORD flags) {
    (void)flags;
    return TRUE;
}

PTOP_LEVEL_EXCEPTION_FILTER SetUnhandledExceptionFilter(PTOP_LEVEL_EXCEPTION_FILTER filter) {
    return filter;
}

PVOID AddVectoredExceptionHandler(ULONG first, PVECTORED_EXCEPTION_HANDLER handler) {
    (void)first;
    (void)handler;
    return (PVOID)(uintptr_t)1;
}

VOID AcquireSRWLockShared(PSRWLOCK lock) {
    pthread_rwlock_t* native = lock_from_srw(lock);
    if (native != NULL) pthread_rwlock_rdlock(native);
}

VOID ReleaseSRWLockShared(PSRWLOCK lock) {
    pthread_rwlock_t* native = lock_from_srw(lock);
    if (native != NULL) pthread_rwlock_unlock(native);
}

VOID AcquireSRWLockExclusive(PSRWLOCK lock) {
    pthread_rwlock_t* native = lock_from_srw(lock);
    if (native != NULL) pthread_rwlock_wrlock(native);
}

VOID ReleaseSRWLockExclusive(PSRWLOCK lock) {
    pthread_rwlock_t* native = lock_from_srw(lock);
    if (native != NULL) pthread_rwlock_unlock(native);
}
