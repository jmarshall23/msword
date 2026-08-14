#include "opus-native-compat.h"

#if defined(_MSC_VER)
int _chdir(const char *path);
char *_getdcwd(int drive, char *buffer, int maxlen);
#else
#include <unistd.h>
#define _chdir chdir
static char *_getdcwd(int drive, char *buffer, int maxlen) {
    (void)drive;
    return getcwd(buffer, maxlen);
}
#endif

#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* AMD64 translation of the active FILE2N.ASM entry points. */

enum {
    kDaReadOnly = 0x01,
    kDaHidden = 0x02,
    kDaSystem = 0x04,
    kDaVolume = 0x08,
    kDaSubdir = 0x10,
    kDaArchive = 0x20
};

/* Native layout of doslib.h::FINDFILE under the x64 compiler. */
typedef struct NativeFindFile {
    char private_bytes[21];
    char attribute;
    char alignment[2];
    uint32_t time;
    uint32_t date;
    uint32_t byte_count;
    char file_name[13];
} NativeFindFile;

typedef char native_find_file_time_offset[(offsetof(NativeFindFile, time) == 24) ? 1 : -1];
typedef char native_find_file_name_offset[(offsetof(NativeFindFile, file_name) == 36) ? 1 : -1];
typedef char native_find_file_size[(sizeof(NativeFindFile) == 52) ? 1 : -1];

typedef struct SearchState {
    HANDLE handle;
    unsigned int requested_attributes;
} SearchState;

typedef struct SearchNode {
    NativeFindFile *destination;
    SearchState state;
    struct SearchNode *next;
} SearchNode;

static SRWLOCK search_lock = SRWLOCK_INIT;
static SearchNode *searches = NULL;

static int dos_error(DWORD error) {
    switch (error) {
    case ERROR_FILE_NOT_FOUND: return -2;
    case ERROR_PATH_NOT_FOUND: return -3;
    case ERROR_TOO_MANY_OPEN_FILES: return -4;
    case ERROR_ACCESS_DENIED: return -5;
    case ERROR_INVALID_HANDLE: return -6;
    case ERROR_NOT_SAME_DEVICE: return -17;
    case ERROR_WRITE_PROTECT: return -19;
    case ERROR_NOT_READY: return -21;
    case ERROR_SHARING_VIOLATION: return -32;
    case ERROR_DISK_FULL:
    case ERROR_HANDLE_DISK_FULL: return -200;
    default: return -5;
    }
}

static unsigned char opus_attributes(DWORD attributes) {
    unsigned char result = 0;
    if ((attributes & FILE_ATTRIBUTE_READONLY) != 0) result |= kDaReadOnly;
    if ((attributes & FILE_ATTRIBUTE_HIDDEN) != 0) result |= kDaHidden;
    if ((attributes & FILE_ATTRIBUTE_SYSTEM) != 0) result |= kDaSystem;
    if ((attributes & FILE_ATTRIBUTE_DIRECTORY) != 0) result |= kDaSubdir;
    if ((attributes & FILE_ATTRIBUTE_ARCHIVE) != 0) result |= kDaArchive;
    return result;
}

static int accepted(const WIN32_FIND_DATAA *data, unsigned int requested) {
    unsigned char attributes = opus_attributes(data->dwFileAttributes);
    if ((attributes & kDaSubdir) != 0 && (requested & kDaSubdir) == 0) {
        return 0;
    }
    if ((attributes & kDaHidden) != 0 && (requested & kDaHidden) == 0) {
        return 0;
    }
    if ((attributes & kDaSystem) != 0 && (requested & kDaSystem) == 0) {
        return 0;
    }
    return 1;
}

static void copy_find_data(NativeFindFile *destination,
                           const WIN32_FIND_DATAA *data) {
    FILETIME local_file_time = {0};
    WORD dos_date = 0;
    WORD dos_time = 0;
    const char *source;

    destination->attribute = (char)opus_attributes(data->dwFileAttributes);
    if (FileTimeToLocalFileTime(&data->ftLastWriteTime, &local_file_time)) {
        FileTimeToDosDateTime(&local_file_time, &dos_date, &dos_time);
    }
    destination->time = dos_time;
    destination->date = dos_date;
    destination->byte_count = data->nFileSizeLow;

    source = data->cAlternateFileName[0] != '\0'
                 ? data->cAlternateFileName
                 : data->cFileName;
    strncpy(destination->file_name, source,
            sizeof(destination->file_name) - 1);
    destination->file_name[sizeof(destination->file_name) - 1] = '\0';
}

static int advance_search(NativeFindFile *destination, SearchState *state,
                          WIN32_FIND_DATAA data, int have_data) {
    int current_is_valid = have_data;
    for (;;) {
        if (current_is_valid && accepted(&data, state->requested_attributes)) {
            copy_find_data(destination, &data);
            return 0;
        }
        if (!FindNextFileA(state->handle, &data)) {
            return (int)GetLastError();
        }
        current_is_valid = 1;
    }
}

static const char *counted_path(const unsigned char *stz) {
    return stz == NULL ? NULL : (const char *)(stz + 1);
}

static SearchNode **find_search_link(NativeFindFile *destination) {
    SearchNode **link = &searches;
    while (*link != NULL) {
        if ((*link)->destination == destination) {
            return link;
        }
        link = &(*link)->next;
    }
    return link;
}

static void close_and_remove_search(NativeFindFile *destination) {
    SearchNode **link = find_search_link(destination);
    SearchNode *node = *link;
    if (node == NULL) {
        return;
    }
    if (node->state.handle != INVALID_HANDLE_VALUE) {
        FindClose(node->state.handle);
    }
    *link = node->next;
    free(node);
}

unsigned int DaGetFileModeSz(const char *path) {
    DWORD attributes = GetFileAttributesA(path);
    return attributes == INVALID_FILE_ATTRIBUTES
               ? 0xffffu
               : (unsigned int)opus_attributes(attributes);
}

int CchCurSzPathNat(char *path, int drive) {
    enum { kLegacyPathCapacity = 67 };
    char current[MAX_PATH] = {0};
    size_t length;

    if (path == NULL) {
        return 0;
    }
    if (_getdcwd(drive, current, (int)sizeof(current)) == NULL) {
        path[0] = '\0';
        return 0;
    }
    length = strlen(current);
    if (length + 2 > kLegacyPathCapacity) {
        path[0] = '\0';
        return 0;
    }
    memcpy(path, current, length + 1);
    if (length == 2 && path[1] == ':') {
        path[length++] = '\\';
        path[length] = '\0';
    } else if (length != 0 && path[length - 1] != '\\') {
        path[length++] = '\\';
        path[length] = '\0';
    }
    return (int)(length + 1);
}

int FSetCurStzPath(const unsigned char *stz_path) {
    if (stz_path == NULL || stz_path[0] == 0) {
        return 1;
    }
    return _chdir(counted_path(stz_path)) == 0;
}

int FMakeStzPath(const unsigned char *stz_path) {
    if (stz_path == NULL || stz_path[0] == 0) {
        return 0;
    }
    if (CreateDirectoryA(counted_path(stz_path), NULL)) {
        return 1;
    }
    return dos_error(GetLastError());
}

int FRemoveStzPath(const unsigned char *stz_path) {
    if (stz_path == NULL || stz_path[0] == 0) {
        return 0;
    }
    if (RemoveDirectoryA(counted_path(stz_path))) {
        return 1;
    }
    return dos_error(GetLastError());
}

int EcDeleteSzFfname(const char *path) {
    return DeleteFileA(path) ? 0 : dos_error(GetLastError());
}

int EcRenameSzFfname(const char *old_path, const char *new_path) {
    return MoveFileA(old_path, new_path) ? 0 : dos_error(GetLastError());
}

int FFirst(NativeFindFile *destination, const char *pattern,
           unsigned int attributes) {
    WIN32_FIND_DATAA data = {0};
    SearchNode *node;
    int result;

    if (destination == NULL || pattern == NULL) {
        return ERROR_INVALID_PARAMETER;
    }

    AcquireSRWLockExclusive(&search_lock);
    close_and_remove_search(destination);
    ReleaseSRWLockExclusive(&search_lock);

    node = (SearchNode *)calloc(1, sizeof(*node));
    if (node == NULL) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    node->destination = destination;
    node->state.requested_attributes = attributes;
    node->state.handle = FindFirstFileA(pattern, &data);
    if (node->state.handle == INVALID_HANDLE_VALUE) {
        free(node);
        return (int)GetLastError();
    }

    result = advance_search(destination, &node->state, data, 1);
    if (result != 0) {
        FindClose(node->state.handle);
        free(node);
        return result;
    }

    AcquireSRWLockExclusive(&search_lock);
    node->next = searches;
    searches = node;
    ReleaseSRWLockExclusive(&search_lock);
    return 0;
}

int FNext(NativeFindFile *destination) {
    SearchNode **link;
    SearchNode *node;
    WIN32_FIND_DATAA data = {0};
    int result;

    if (destination == NULL) {
        return ERROR_INVALID_PARAMETER;
    }
    AcquireSRWLockExclusive(&search_lock);
    link = find_search_link(destination);
    node = *link;
    if (node == NULL) {
        ReleaseSRWLockExclusive(&search_lock);
        return ERROR_NO_MORE_FILES;
    }

    result = advance_search(destination, &node->state, data, 0);
    if (result != 0) {
        FindClose(node->state.handle);
        *link = node->next;
        free(node);
    }
    ReleaseSRWLockExclusive(&search_lock);
    return result;
}

long LcbDiskFreeSpace(int drive_character) {
    char root[] = "C:\\";
    const char *root_pointer = NULL;
    ULARGE_INTEGER available = {{0}};
    if (drive_character != 0) {
        root[0] = (char)drive_character;
        root_pointer = root;
    }
    if (!GetDiskFreeSpaceExA(root_pointer, &available, NULL, NULL)) {
        return -1L;
    }
    return available.QuadPart > (ULONGLONG)LONG_MAX
               ? LONG_MAX
               : (long)available.QuadPart;
}

unsigned int PnWhoseFcGEFc(long file_offset) {
    return file_offset <= 0
               ? 0u
               : (unsigned int)((file_offset + 511L) >> 9);
}

int FFileRemote(const char *path) {
    char root[] = "C:\\";
    if (path == NULL || path[0] == '\0') {
        return 0;
    }
    if (path[0] == '\\' && path[1] == '\\') {
        return 1;
    }
    root[0] = path[0];
    return GetDriveTypeA(root) == DRIVE_REMOTE;
}
