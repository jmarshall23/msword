#include "opus_x64_compat.h"

#if defined(_MSC_VER)
extern "C" int _chdir(const char* path);
extern "C" char* _getdcwd(int drive, char* buffer, int maxlen);
#else
#include <unistd.h>
#define _chdir chdir
static char* _getdcwd(int, char* buffer, int maxlen) {
    return getcwd(buffer, maxlen);
}
#endif

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <limits>
#include <mutex>
#include <unordered_map>

/* AMD64 translation of the active FILE2N.ASM entry points. */

namespace {

constexpr unsigned char kDaReadOnly = 0x01;
constexpr unsigned char kDaHidden = 0x02;
constexpr unsigned char kDaSystem = 0x04;
constexpr unsigned char kDaVolume = 0x08;
constexpr unsigned char kDaSubdir = 0x10;
constexpr unsigned char kDaArchive = 0x20;

/* Native layout of doslib.h::FINDFILE under the x64 compiler. */
struct NativeFindFile {
    char private_bytes[21];
    char attribute;
    char alignment[2];
    std::uint32_t time;
    std::uint32_t date;
    std::uint32_t byte_count;
    char file_name[13];
};

static_assert(offsetof(NativeFindFile, time) == 24);
static_assert(offsetof(NativeFindFile, file_name) == 36);
static_assert(sizeof(NativeFindFile) == 52);

struct SearchState {
    HANDLE handle = INVALID_HANDLE_VALUE;
    unsigned int requested_attributes = 0;
};

std::mutex search_mutex;
std::unordered_map<void*, SearchState> searches;

int dos_error(const DWORD error) noexcept {
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

unsigned char opus_attributes(const DWORD attributes) noexcept {
    unsigned char result = 0;
    if ((attributes & FILE_ATTRIBUTE_READONLY) != 0) result |= kDaReadOnly;
    if ((attributes & FILE_ATTRIBUTE_HIDDEN) != 0) result |= kDaHidden;
    if ((attributes & FILE_ATTRIBUTE_SYSTEM) != 0) result |= kDaSystem;
    if ((attributes & FILE_ATTRIBUTE_DIRECTORY) != 0) result |= kDaSubdir;
    if ((attributes & FILE_ATTRIBUTE_ARCHIVE) != 0) result |= kDaArchive;
    return result;
}

bool accepted(const WIN32_FIND_DATAA& data,
              const unsigned int requested) noexcept {
    const unsigned char attributes = opus_attributes(data.dwFileAttributes);
    if ((attributes & kDaSubdir) != 0 && (requested & kDaSubdir) == 0) {
        return false;
    }
    if ((attributes & kDaHidden) != 0 && (requested & kDaHidden) == 0) {
        return false;
    }
    if ((attributes & kDaSystem) != 0 && (requested & kDaSystem) == 0) {
        return false;
    }
    return true;
}

void copy_find_data(NativeFindFile* destination,
                    const WIN32_FIND_DATAA& data) noexcept {
    destination->attribute =
        static_cast<char>(opus_attributes(data.dwFileAttributes));

    FILETIME local_file_time{};
    WORD dos_date = 0;
    WORD dos_time = 0;
    if (FileTimeToLocalFileTime(&data.ftLastWriteTime, &local_file_time)) {
        FileTimeToDosDateTime(&local_file_time, &dos_date, &dos_time);
    }
    destination->time = dos_time;
    destination->date = dos_date;
    destination->byte_count = data.nFileSizeLow;

    const char* source = data.cAlternateFileName[0] != '\0'
                             ? data.cAlternateFileName
                             : data.cFileName;
    std::strncpy(destination->file_name, source,
                 sizeof(destination->file_name) - 1);
    destination->file_name[sizeof(destination->file_name) - 1] = '\0';
}

int advance_search(NativeFindFile* destination, SearchState& state,
                   WIN32_FIND_DATAA data, const bool have_data) {
    bool current_is_valid = have_data;
    for (;;) {
        if (current_is_valid && accepted(data, state.requested_attributes)) {
            copy_find_data(destination, data);
            return 0;
        }
        if (!FindNextFileA(state.handle, &data)) {
            return static_cast<int>(GetLastError());
        }
        current_is_valid = true;
    }
}

const char* counted_path(const unsigned char* stz) noexcept {
    return stz == nullptr ? nullptr
                          : reinterpret_cast<const char*>(stz + 1);
}

}  // namespace

extern "C" {

unsigned int DaGetFileModeSz(const char* path) {
    const DWORD attributes = GetFileAttributesA(path);
    return attributes == INVALID_FILE_ATTRIBUTES
               ? 0xffffu
               : static_cast<unsigned int>(opus_attributes(attributes));
}

int CchCurSzPathNat(char* path, const int drive) {
    if (path == nullptr) {
        return 0;
    }

    /* The original ABI requires callers to provide exactly 67 bytes. Do not
     * copy a modern MAX_PATH-sized current directory into that legacy buffer.
     */
    constexpr int kLegacyPathCapacity = 67;
    char current[MAX_PATH]{};
    if (_getdcwd(drive, current, static_cast<int>(sizeof(current))) ==
        nullptr) {
        path[0] = '\0';
        return 0;
    }
    std::size_t length = std::strlen(current);
    if (length + 2 > kLegacyPathCapacity) {
        path[0] = '\0';
        return 0;
    }
    std::memcpy(path, current, length + 1);
    if (length == 2 && path[1] == ':') {
        path[length++] = '\\';
        path[length] = '\0';
    } else if (length != 0 && path[length - 1] != '\\') {
        path[length++] = '\\';
        path[length] = '\0';
    }
    return static_cast<int>(length + 1);
}

int FSetCurStzPath(const unsigned char* stz_path) {
    if (stz_path == nullptr || stz_path[0] == 0) {
        return 1;
    }
    return _chdir(counted_path(stz_path)) == 0;
}

int FMakeStzPath(const unsigned char* stz_path) {
    if (stz_path == nullptr || stz_path[0] == 0) {
        return 0;
    }
    if (CreateDirectoryA(counted_path(stz_path), nullptr)) {
        return 1;
    }
    return dos_error(GetLastError());
}

int FRemoveStzPath(const unsigned char* stz_path) {
    if (stz_path == nullptr || stz_path[0] == 0) {
        return 0;
    }
    if (RemoveDirectoryA(counted_path(stz_path))) {
        return 1;
    }
    return dos_error(GetLastError());
}

int EcDeleteSzFfname(const char* path) {
    return DeleteFileA(path) ? 0 : dos_error(GetLastError());
}

int EcRenameSzFfname(const char* old_path, const char* new_path) {
    return MoveFileA(old_path, new_path) ? 0 : dos_error(GetLastError());
}

int FFirst(NativeFindFile* destination, const char* pattern,
           const unsigned int attributes) {
    if (destination == nullptr || pattern == nullptr) {
        return ERROR_INVALID_PARAMETER;
    }

    std::lock_guard<std::mutex> lock(search_mutex);
    const auto previous = searches.find(destination);
    if (previous != searches.end()) {
        if (previous->second.handle != INVALID_HANDLE_VALUE) {
            FindClose(previous->second.handle);
        }
        searches.erase(previous);
    }

    WIN32_FIND_DATAA data{};
    SearchState state;
    state.requested_attributes = attributes;
    state.handle = FindFirstFileA(pattern, &data);
    if (state.handle == INVALID_HANDLE_VALUE) {
        return static_cast<int>(GetLastError());
    }

    const int result = advance_search(destination, state, data, true);
    if (result != 0) {
        FindClose(state.handle);
        return result;
    }
    searches.emplace(destination, state);
    return 0;
}

int FNext(NativeFindFile* destination) {
    if (destination == nullptr) {
        return ERROR_INVALID_PARAMETER;
    }
    std::lock_guard<std::mutex> lock(search_mutex);
    const auto found = searches.find(destination);
    if (found == searches.end()) {
        return ERROR_NO_MORE_FILES;
    }

    WIN32_FIND_DATAA data{};
    const int result = advance_search(destination, found->second, data, false);
    if (result != 0) {
        FindClose(found->second.handle);
        searches.erase(found);
    }
    return result;
}

long LcbDiskFreeSpace(const int drive_character) {
    char root[] = "C:\\";
    const char* root_pointer = nullptr;
    if (drive_character != 0) {
        root[0] = static_cast<char>(drive_character);
        root_pointer = root;
    }
    ULARGE_INTEGER available{};
    if (!GetDiskFreeSpaceExA(root_pointer, &available, nullptr, nullptr)) {
        return -1L;
    }
    return static_cast<long>((std::min)(
        available.QuadPart,
        static_cast<ULONGLONG>((std::numeric_limits<LONG>::max)())));
}

unsigned int PnWhoseFcGEFc(const long file_offset) {
    return file_offset <= 0
               ? 0u
               : static_cast<unsigned int>((file_offset + 511L) >> 9);
}

int FFileRemote(const char* path) {
    if (path == nullptr || path[0] == '\0') {
        return 0;
    }
    if (path[0] == '\\' && path[1] == '\\') {
        return 1;
    }
    char root[] = "C:\\";
    root[0] = path[0];
    return GetDriveTypeA(root) == DRIVE_REMOTE;
}

}  // extern "C"
