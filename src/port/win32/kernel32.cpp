#include "windows.h"

#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <condition_variable>
#include <cctype>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <iostream>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

thread_local DWORD g_last_error = ERROR_SUCCESS;

struct HeapBlock {
    void* memory = nullptr;
    SIZE_T size = 0;
};

struct GlobalBlock {
    DWORD magic = 0x474d454du;
    void* memory = nullptr;
    SIZE_T size = 0;
    UINT flags = 0;
    unsigned lock_count = 0;
    WORD token = 0;
    bool freed = false;
};

struct FileHandle {
    DWORD magic = 0x46494c45u;
    FILE* file = nullptr;
};

struct FindHandle {
    DWORD magic = 0x46494e44u;
    std::vector<std::filesystem::directory_entry> entries;
    std::size_t index = 0;
};

std::mutex g_heap_lock;
std::mutex g_global_lock;
std::mutex g_file_lock;
std::unordered_map<void*, HeapBlock> g_heap_blocks;
std::unordered_map<void*, GlobalBlock*> g_global_by_pointer;
std::unordered_map<int, FileHandle*> g_hfiles;
int g_next_hfile = 3;
WORD g_next_global_token = 1;

DWORD error_from_errno(const int value) {
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

void set_errno_error() { g_last_error = error_from_errno(errno); }

std::string lower_ascii(std::string value) {
    for (char& ch : value) {
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }
    return value;
}

std::filesystem::path c_drive_root() {
    if (const char* root = std::getenv("OPUS_C_DRIVE");
        root != nullptr && *root != '\0') {
        return root;
    }
    return std::filesystem::path(__FILE__).parent_path().parent_path().parent_path()
        .parent_path();
}

std::filesystem::path normalize_path(const char* path) {
    if (path == nullptr || *path == '\0') return {};
    std::string text(path);
    std::replace(text.begin(), text.end(), '\\', '/');
    if (text.size() >= 2 && text[1] == ':') {
        if (std::tolower(static_cast<unsigned char>(text[0])) != 'c') {
            g_last_error = ERROR_PATH_NOT_FOUND;
            return {};
        }
        text.erase(0, text.size() >= 3 && text[2] == '/' ? 3 : 2);
        return c_drive_root() / text;
    }
    if (!text.empty() && text.front() == '/') return text;
    return std::filesystem::current_path() / text;
}

std::filesystem::path resolve_case_path(std::filesystem::path path) {
    if (path.empty() || std::filesystem::exists(path)) return path;
    std::filesystem::path result = path.is_absolute() ? path.root_path() : std::filesystem::path{};
    auto iterator = path.begin();
    if (path.is_absolute() && iterator != path.end()) ++iterator;
    for (; iterator != path.end(); ++iterator) {
        const auto part = *iterator;
        const auto candidate = result / part;
        if (std::filesystem::exists(candidate)) {
            result = candidate;
            continue;
        }
        bool matched = false;
        std::error_code error;
        if (!result.empty() && std::filesystem::is_directory(result, error)) {
            const std::string wanted = lower_ascii(part.string());
            for (const auto& entry : std::filesystem::directory_iterator(result, error)) {
                if (lower_ascii(entry.path().filename().string()) == wanted) {
                    result = entry.path();
                    matched = true;
                    break;
                }
            }
        }
        if (!matched) result = candidate;
    }
    return result;
}

std::filesystem::path host_path(const char* path) {
    return resolve_case_path(normalize_path(path));
}

bool match_pattern(const std::string& name, const std::string& pattern) {
    std::size_t n = 0, p = 0, star = std::string::npos, mark = 0;
    const std::string lower_name = lower_ascii(name);
    const std::string lower_pattern = lower_ascii(pattern);
    while (n < lower_name.size()) {
        if (p < lower_pattern.size() &&
            (lower_pattern[p] == '?' || lower_pattern[p] == lower_name[n])) {
            ++n;
            ++p;
        } else if (p < lower_pattern.size() && lower_pattern[p] == '*') {
            star = p++;
            mark = n;
        } else if (star != std::string::npos) {
            p = star + 1;
            n = ++mark;
        } else {
            return false;
        }
    }
    while (p < lower_pattern.size() && lower_pattern[p] == '*') ++p;
    return p == lower_pattern.size();
}

FILETIME filetime_from_time(std::time_t seconds) {
    constexpr unsigned long long kEpochDelta = 11644473600ull;
    const unsigned long long ticks =
        (static_cast<unsigned long long>(seconds) + kEpochDelta) * 10000000ull;
    return {static_cast<DWORD>(ticks & 0xffffffffu),
            static_cast<DWORD>(ticks >> 32u)};
}

DWORD attributes_from_status(const std::filesystem::path& path) {
    std::error_code error;
    const auto status = std::filesystem::status(path, error);
    if (error || !std::filesystem::exists(status)) return INVALID_FILE_ATTRIBUTES;
    DWORD attributes = FILE_ATTRIBUTE_NORMAL;
    if (std::filesystem::is_directory(status)) attributes |= FILE_ATTRIBUTE_DIRECTORY;
    return attributes;
}

void fill_find_data(const std::filesystem::directory_entry& entry,
                    WIN32_FIND_DATAA* data) {
    *data = {};
    const auto path = entry.path();
    data->dwFileAttributes = attributes_from_status(path);
    std::error_code error;
    if (!entry.is_directory(error)) {
        const auto size = entry.file_size(error);
        if (!error) {
            data->nFileSizeHigh = static_cast<DWORD>(size >> 32u);
            data->nFileSizeLow = static_cast<DWORD>(size & 0xffffffffu);
        }
    }
    const std::time_t now = std::time(nullptr);
    data->ftCreationTime = data->ftLastAccessTime =
        data->ftLastWriteTime = filetime_from_time(now);
    lstrcpynA(data->cFileName, path.filename().string().c_str(), MAX_PATH);
}

FileHandle* file_from_handle(HANDLE handle) {
    auto* file = static_cast<FileHandle*>(handle);
    return file != nullptr && file->magic == 0x46494c45u ? file : nullptr;
}

std::shared_mutex* lock_from_srw(PSRWLOCK lock) {
    static std::mutex init_lock;
    if (lock == nullptr) return nullptr;
    if (lock->Ptr == nullptr) {
        std::lock_guard<std::mutex> guard(init_lock);
        if (lock->Ptr == nullptr) lock->Ptr = new std::shared_mutex();
    }
    return static_cast<std::shared_mutex*>(lock->Ptr);
}

}  // namespace

extern "C" {

DWORD GetLastError(void) { return g_last_error; }
VOID SetLastError(DWORD error) { g_last_error = error; }

VOID OutputDebugStringA(LPCSTR text) {
    if (text != nullptr) std::fputs(text, stderr);
}

DWORD FormatMessageA(DWORD, LPCVOID, DWORD message_id, DWORD, LPSTR buffer,
                     DWORD size, LPVOID) {
    if (buffer == nullptr || size == 0) return 0;
    const std::string text = "Win32 error " + std::to_string(message_id);
    lstrcpynA(buffer, text.c_str(), static_cast<int>(size));
    return static_cast<DWORD>(std::strlen(buffer));
}

DWORD GetModuleFileNameA(HMODULE, LPSTR file_name, DWORD size) {
    if (file_name == nullptr || size == 0) return 0;
    char path[4096]{};
    ssize_t count = readlink("/proc/self/exe", path, sizeof(path) - 1);
    if (count < 0) {
        const std::string fallback = std::filesystem::current_path().string();
        lstrcpynA(file_name, fallback.c_str(), static_cast<int>(size));
    } else {
        path[count] = '\0';
        lstrcpynA(file_name, path, static_cast<int>(size));
    }
    return static_cast<DWORD>(std::strlen(file_name));
}

HANDLE GetCurrentProcess(void) {
    return reinterpret_cast<HANDLE>(static_cast<uintptr_t>(2));
}

DWORD GetCurrentProcessId(void) {
    return static_cast<DWORD>(getpid());
}

BOOL TerminateProcess(HANDLE, UINT exit_code) {
    std::exit(static_cast<int>(exit_code));
}

VOID RaiseException(DWORD code, DWORD, DWORD, const ULONG_PTR*) {
    std::abort();
}

USHORT RtlCaptureStackBackTrace(ULONG frames_to_skip, ULONG frames_to_capture,
                                PVOID* back_trace, ULONG*) {
    return CaptureStackBackTrace(frames_to_skip, frames_to_capture, back_trace,
                                 nullptr);
}

LPSTR lstrcpynA(LPSTR destination, LPCSTR source, int count) {
    if (destination == nullptr || count <= 0) return destination;
    if (source == nullptr) {
        destination[0] = '\0';
        return destination;
    }
    std::strncpy(destination, source, static_cast<std::size_t>(count));
    destination[count - 1] = '\0';
    return destination;
}

LPWSTR lstrcpyW(LPWSTR destination, LPCWSTR source) {
    if (destination == nullptr) return destination;
    WCHAR* out = destination;
    if (source != nullptr) {
        while ((*out++ = *source++) != 0) {}
    } else {
        *out = 0;
    }
    return destination;
}

LPSTR CharLowerA(LPSTR string) {
    const auto value = reinterpret_cast<DWORD_PTR>(string);
    if ((value >> 16u) == 0) {
        return reinterpret_cast<LPSTR>(static_cast<DWORD_PTR>(
            std::tolower(static_cast<unsigned char>(value & 0xffu))));
    }
    for (char* cursor = string; cursor != nullptr && *cursor != '\0'; ++cursor) {
        *cursor = static_cast<char>(
            std::tolower(static_cast<unsigned char>(*cursor)));
    }
    return string;
}

DWORD CharUpperBuffA(LPSTR text, DWORD length) {
    if (text == nullptr) return 0;
    for (DWORD index = 0; index < length; ++index) {
        text[index] = static_cast<char>(
            std::toupper(static_cast<unsigned char>(text[index])));
    }
    return length;
}

BOOL GetStringTypeA(DWORD, DWORD type, LPCSTR source, int count,
                    WORD* char_type) {
    if (type != CT_CTYPE1 || source == nullptr || char_type == nullptr) {
        return FALSE;
    }
    for (int index = 0; index < count; ++index) {
        const auto ch = static_cast<unsigned char>(source[index]);
        WORD flags = 0;
        const bool upper = std::isupper(ch) || (ch >= 0xc0 && ch <= 0xd6) ||
                           (ch >= 0xd8 && ch <= 0xde);
        const bool lower = std::islower(ch) || (ch >= 0xdf && ch <= 0xf6) ||
                           (ch >= 0xf8 && ch <= 0xff);
        if (upper) flags |= C1_UPPER | C1_ALPHA;
        if (lower) flags |= C1_LOWER | C1_ALPHA;
        if (std::isdigit(ch)) flags |= C1_DIGIT;
        char_type[index] = flags;
    }
    return TRUE;
}

int MulDiv(int number, int numerator, int denominator) {
    if (denominator == 0) return number >= 0 ? INT_MAX : INT_MIN;
    const long long value = static_cast<long long>(number) * numerator;
    const long long adjusted =
        value >= 0 ? value + denominator / 2 : value - denominator / 2;
    return static_cast<int>(adjusted / denominator);
}

HANDLE GetProcessHeap(void) { return reinterpret_cast<HANDLE>(static_cast<uintptr_t>(1)); }

LPVOID HeapAlloc(HANDLE, DWORD flags, SIZE_T bytes) {
    const SIZE_T actual = bytes == 0 ? 1 : bytes;
    void* memory = std::malloc(actual);
    if (memory == nullptr) {
        g_last_error = ERROR_NOT_ENOUGH_MEMORY;
        return nullptr;
    }
    if ((flags & HEAP_ZERO_MEMORY) != 0) std::memset(memory, 0, actual);
    std::lock_guard<std::mutex> guard(g_heap_lock);
    g_heap_blocks[memory] = {memory, actual};
    return memory;
}

LPVOID HeapReAlloc(HANDLE, DWORD flags, LPVOID memory, SIZE_T bytes) {
    if (memory == nullptr) return HeapAlloc(nullptr, flags, bytes);
    const SIZE_T actual = bytes == 0 ? 1 : bytes;
    std::lock_guard<std::mutex> guard(g_heap_lock);
    auto found = g_heap_blocks.find(memory);
    if (found == g_heap_blocks.end()) {
        g_last_error = ERROR_INVALID_HANDLE;
        return nullptr;
    }
    const SIZE_T old_size = found->second.size;
    void* resized = std::realloc(memory, actual);
    if (resized == nullptr) {
        g_last_error = ERROR_NOT_ENOUGH_MEMORY;
        return nullptr;
    }
    if ((flags & HEAP_ZERO_MEMORY) != 0 && actual > old_size) {
        std::memset(static_cast<unsigned char*>(resized) + old_size, 0,
                    actual - old_size);
    }
    g_heap_blocks.erase(found);
    g_heap_blocks[resized] = {resized, actual};
    return resized;
}

BOOL HeapFree(HANDLE, DWORD, LPVOID memory) {
    if (memory == nullptr) return TRUE;
    std::lock_guard<std::mutex> guard(g_heap_lock);
    auto found = g_heap_blocks.find(memory);
    if (found == g_heap_blocks.end()) {
        g_last_error = ERROR_INVALID_HANDLE;
        return FALSE;
    }
    std::free(memory);
    g_heap_blocks.erase(found);
    return TRUE;
}

SIZE_T HeapSize(HANDLE, DWORD, LPCVOID memory) {
    std::lock_guard<std::mutex> guard(g_heap_lock);
    const auto found = g_heap_blocks.find(const_cast<LPVOID>(memory));
    if (found == g_heap_blocks.end()) return static_cast<SIZE_T>(-1);
    return found->second.size;
}

SIZE_T HeapCompact(HANDLE, DWORD) { return 16u * 1024u * 1024u; }

HANDLE GlobalAlloc(UINT flags, SIZE_T bytes) {
    auto* block = new GlobalBlock();
    block->size = bytes;
    block->flags = flags;
    block->memory = std::malloc(bytes == 0 ? 1 : bytes);
    if (block->memory == nullptr) {
        delete block;
        g_last_error = ERROR_NOT_ENOUGH_MEMORY;
        return nullptr;
    }
    if ((flags & GMEM_ZEROINIT) != 0) {
        std::memset(block->memory, 0, bytes == 0 ? 1 : bytes);
    }
    std::lock_guard<std::mutex> guard(g_global_lock);
    block->token = g_next_global_token++;
    if (g_next_global_token == 0) g_next_global_token = 1;
    g_global_by_pointer[block->memory] = block;
    return static_cast<HANDLE>(block);
}

LPVOID GlobalLock(HANDLE memory) {
    auto* block = static_cast<GlobalBlock*>(memory);
    if (block == nullptr || block->magic != 0x474d454du || block->freed) {
        g_last_error = ERROR_INVALID_HANDLE;
        return nullptr;
    }
    ++block->lock_count;
    return block->memory;
}

BOOL GlobalUnlock(HANDLE memory) {
    auto* block = static_cast<GlobalBlock*>(memory);
    if (block == nullptr || block->magic != 0x474d454du || block->freed) {
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
    auto* block = static_cast<GlobalBlock*>(memory);
    if (block == nullptr) return nullptr;
    if (block->magic != 0x474d454du || block->freed) {
        g_last_error = ERROR_INVALID_HANDLE;
        return memory;
    }
    {
        std::lock_guard<std::mutex> guard(g_global_lock);
        g_global_by_pointer.erase(block->memory);
    }
    std::free(block->memory);
    block->freed = true;
    block->memory = nullptr;
    block->size = 0;
    return nullptr;
}

HANDLE GlobalReAlloc(HANDLE memory, SIZE_T bytes, UINT flags) {
    auto* block = static_cast<GlobalBlock*>(memory);
    if (block == nullptr || block->magic != 0x474d454du || block->freed) {
        g_last_error = ERROR_INVALID_HANDLE;
        return nullptr;
    }
    void* resized = std::realloc(block->memory, bytes == 0 ? 1 : bytes);
    if (resized == nullptr) {
        g_last_error = ERROR_NOT_ENOUGH_MEMORY;
        return nullptr;
    }
    if ((flags & GMEM_ZEROINIT) != 0 && bytes > block->size) {
        std::memset(static_cast<unsigned char*>(resized) + block->size, 0,
                    bytes - block->size);
    }
    std::lock_guard<std::mutex> guard(g_global_lock);
    g_global_by_pointer.erase(block->memory);
    block->memory = resized;
    block->size = bytes;
    block->flags = flags;
    g_global_by_pointer[block->memory] = block;
    return memory;
}

SIZE_T GlobalSize(HANDLE memory) {
    auto* block = static_cast<GlobalBlock*>(memory);
    return block == nullptr || block->magic != 0x474d454du || block->freed
               ? 0
               : block->size;
}

DWORD GlobalHandle(LPCVOID memory) {
    std::lock_guard<std::mutex> guard(g_global_lock);
    const auto found = g_global_by_pointer.find(const_cast<LPVOID>(memory));
    if (found == g_global_by_pointer.end()) return 0;
    return static_cast<DWORD>(found->second->token) << 16u;
}

DWORD GlobalCompact(DWORD) { return 16u * 1024u * 1024u; }

LPVOID GlobalWire(HANDLE memory) { return GlobalLock(memory); }

UINT GlobalFlags(HANDLE memory) {
    auto* block = static_cast<GlobalBlock*>(memory);
    if (block == nullptr || block->magic != 0x474d454du || block->freed) {
        g_last_error = ERROR_INVALID_HANDLE;
        return GMEM_INVALID_HANDLE;
    }
    return (block->flags & ~GMEM_LOCKCOUNT) |
           (std::min)(block->lock_count, 0xffu);
}

HLOCAL LocalAlloc(UINT flags, SIZE_T bytes) {
    return static_cast<HLOCAL>(GlobalAlloc(flags, bytes));
}

HLOCAL LocalFree(HLOCAL memory) {
    return static_cast<HLOCAL>(GlobalFree(memory));
}

HLOCAL LocalReAlloc(HLOCAL memory, SIZE_T bytes, UINT flags) {
    return static_cast<HLOCAL>(GlobalReAlloc(memory, bytes, flags));
}

HANDLE CreateFileA(LPCSTR file_name, DWORD desired_access, DWORD, LPVOID,
                   DWORD creation_disposition, DWORD, HANDLE) {
    const auto path = host_path(file_name);
    const bool write = (desired_access & (GENERIC_WRITE | FILE_APPEND_DATA)) != 0;
    const bool read = (desired_access & GENERIC_READ) != 0 || !write;
    const char* mode = "rb";
    if (creation_disposition == CREATE_ALWAYS) {
        mode = read ? "w+b" : "wb";
    } else if (creation_disposition == OPEN_ALWAYS) {
        mode = std::filesystem::exists(path) ? (read ? "r+b" : "ab")
                                             : (read ? "w+b" : "wb");
    } else if (creation_disposition == CREATE_NEW) {
        if (std::filesystem::exists(path)) {
            g_last_error = ERROR_FILE_EXISTS;
            return INVALID_HANDLE_VALUE;
        }
        mode = read ? "w+b" : "wb";
    } else if (write) {
        mode = read ? "r+b" : "ab";
    }
    FILE* file = std::fopen(path.string().c_str(), mode);
    if (file == nullptr) {
        set_errno_error();
        return INVALID_HANDLE_VALUE;
    }
    if ((desired_access & FILE_APPEND_DATA) != 0) std::fseek(file, 0, SEEK_END);
    return new FileHandle{0x46494c45u, file};
}

BOOL ReadFile(HANDLE handle, LPVOID buffer, DWORD bytes_to_read,
              DWORD* bytes_read, LPVOID) {
    auto* file = file_from_handle(handle);
    if (file == nullptr || buffer == nullptr) {
        g_last_error = ERROR_INVALID_HANDLE;
        return FALSE;
    }
    const size_t count = std::fread(buffer, 1, bytes_to_read, file->file);
    if (bytes_read != nullptr) *bytes_read = static_cast<DWORD>(count);
    if (count < bytes_to_read && std::ferror(file->file)) {
        set_errno_error();
        return FALSE;
    }
    return TRUE;
}

BOOL WriteFile(HANDLE handle, LPCVOID buffer, DWORD bytes_to_write,
               DWORD* bytes_written, LPVOID) {
    auto* file = file_from_handle(handle);
    if (file == nullptr || buffer == nullptr) {
        g_last_error = ERROR_INVALID_HANDLE;
        return FALSE;
    }
    const size_t count = std::fwrite(buffer, 1, bytes_to_write, file->file);
    if (bytes_written != nullptr) *bytes_written = static_cast<DWORD>(count);
    if (count != bytes_to_write) {
        set_errno_error();
        return FALSE;
    }
    return TRUE;
}

BOOL FlushFileBuffers(HANDLE handle) {
    auto* file = file_from_handle(handle);
    return file != nullptr && std::fflush(file->file) == 0;
}

BOOL CloseHandle(HANDLE object) {
    auto* file = file_from_handle(object);
    if (file == nullptr) return TRUE;
    const int result = std::fclose(file->file);
    file->magic = 0;
    delete file;
    if (result != 0) {
        set_errno_error();
        return FALSE;
    }
    return TRUE;
}

DWORD WaitForSingleObject(HANDLE, DWORD) { return WAIT_OBJECT_0; }

DWORD GetFileType(HANDLE handle) {
    return file_from_handle(handle) == nullptr ? 0 : FILE_TYPE_DISK;
}

HFILE OpenFile(LPCSTR file_name, LPOFSTRUCT reopen_buffer, UINT style) {
    const DWORD access = (style & OF_WRITE) != 0 ? GENERIC_WRITE : GENERIC_READ;
    const DWORD disposition = (style & OF_CREATE) != 0 ? CREATE_ALWAYS : OPEN_EXISTING;
    HANDLE handle = CreateFileA(file_name, access, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                nullptr, disposition, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (handle == INVALID_HANDLE_VALUE) {
        if (reopen_buffer != nullptr) reopen_buffer->nErrCode = static_cast<WORD>(g_last_error);
        return HFILE_ERROR;
    }
    auto* file = static_cast<FileHandle*>(handle);
    std::lock_guard<std::mutex> guard(g_file_lock);
    const int value = g_next_hfile++;
    g_hfiles[value] = file;
    if (reopen_buffer != nullptr) {
        reopen_buffer->cBytes = sizeof(OFSTRUCT);
        reopen_buffer->fFixedDisk = TRUE;
        reopen_buffer->nErrCode = 0;
        lstrcpynA(reopen_buffer->szPathName, file_name, sizeof(reopen_buffer->szPathName));
    }
    return value;
}

int _lclose(HFILE file) {
    std::lock_guard<std::mutex> guard(g_file_lock);
    const auto found = g_hfiles.find(file);
    if (found == g_hfiles.end()) return HFILE_ERROR;
    FileHandle* handle = found->second;
    g_hfiles.erase(found);
    const int result = std::fclose(handle->file);
    handle->magic = 0;
    delete handle;
    return result == 0 ? 0 : HFILE_ERROR;
}

UINT _lread(HFILE file, LPVOID buffer, UINT bytes) {
    std::lock_guard<std::mutex> guard(g_file_lock);
    const auto found = g_hfiles.find(file);
    if (found == g_hfiles.end()) return HFILE_ERROR;
    return static_cast<UINT>(std::fread(buffer, 1, bytes, found->second->file));
}

UINT _lwrite(HFILE file, LPCCH buffer, UINT bytes) {
    std::lock_guard<std::mutex> guard(g_file_lock);
    const auto found = g_hfiles.find(file);
    if (found == g_hfiles.end()) return HFILE_ERROR;
    return static_cast<UINT>(std::fwrite(buffer, 1, bytes, found->second->file));
}

LONG _llseek(HFILE file, LONG offset, int origin) {
    std::lock_guard<std::mutex> guard(g_file_lock);
    const auto found = g_hfiles.find(file);
    if (found == g_hfiles.end()) return HFILE_ERROR;
    if (std::fseek(found->second->file, offset, origin) != 0) return HFILE_ERROR;
    return static_cast<LONG>(std::ftell(found->second->file));
}

DWORD GetFileAttributesA(LPCSTR file_name) {
    const DWORD attributes = attributes_from_status(host_path(file_name));
    if (attributes == INVALID_FILE_ATTRIBUTES) set_errno_error();
    return attributes;
}

BOOL GetFileAttributesExA(LPCSTR file_name, int, LPVOID file_information) {
    if (file_information == nullptr) return FALSE;
    const auto path = host_path(file_name);
    auto* data = static_cast<WIN32_FILE_ATTRIBUTE_DATA*>(file_information);
    *data = {};
    data->dwFileAttributes = attributes_from_status(path);
    if (data->dwFileAttributes == INVALID_FILE_ATTRIBUTES) {
        set_errno_error();
        return FALSE;
    }
    std::error_code error;
    if (!std::filesystem::is_directory(path, error)) {
        const auto size = std::filesystem::file_size(path, error);
        if (!error) {
            data->nFileSizeHigh = static_cast<DWORD>(size >> 32u);
            data->nFileSizeLow = static_cast<DWORD>(size & 0xffffffffu);
        }
    }
    const auto now = filetime_from_time(std::time(nullptr));
    data->ftCreationTime = data->ftLastAccessTime = data->ftLastWriteTime = now;
    return TRUE;
}

BOOL CreateDirectoryA(LPCSTR path_name, LPVOID) {
    if (std::filesystem::create_directory(normalize_path(path_name))) return TRUE;
    set_errno_error();
    return FALSE;
}

BOOL RemoveDirectoryA(LPCSTR path_name) {
    if (std::filesystem::remove(host_path(path_name))) return TRUE;
    set_errno_error();
    return FALSE;
}

BOOL DeleteFileA(LPCSTR file_name) {
    if (std::filesystem::remove(host_path(file_name))) return TRUE;
    set_errno_error();
    return FALSE;
}

DWORD GetCurrentDirectoryA(DWORD buffer_length, LPSTR buffer) {
    const std::string current = std::filesystem::current_path().string();
    if (buffer != nullptr && buffer_length != 0) {
        lstrcpynA(buffer, current.c_str(), static_cast<int>(buffer_length));
    }
    return static_cast<DWORD>(current.size());
}

DWORD GetFullPathNameA(LPCSTR file_name, DWORD buffer_length, LPSTR buffer,
                       LPSTR* file_part) {
    const std::string full = host_path(file_name).lexically_normal().string();
    if (buffer != nullptr && buffer_length != 0) {
        lstrcpynA(buffer, full.c_str(), static_cast<int>(buffer_length));
    }
    if (file_part != nullptr && buffer != nullptr) {
        const char* slash = std::strrchr(buffer, '/');
        *file_part = const_cast<char*>(slash == nullptr ? buffer : slash + 1);
    }
    return static_cast<DWORD>(full.size());
}

DWORD GetTempPathA(DWORD buffer_length, LPSTR buffer) {
    const char* temp = std::getenv("TMPDIR");
    const std::string path = temp != nullptr ? temp : "/tmp/";
    if (buffer != nullptr && buffer_length != 0) {
        lstrcpynA(buffer, path.c_str(), static_cast<int>(buffer_length));
    }
    return static_cast<DWORD>(path.size());
}

BOOL CopyFileA(LPCSTR existing_file_name, LPCSTR new_file_name, BOOL fail_if_exists) {
    const auto from = host_path(existing_file_name);
    const auto to = normalize_path(new_file_name);
    if (fail_if_exists && std::filesystem::exists(to)) {
        g_last_error = ERROR_FILE_EXISTS;
        return FALSE;
    }
    std::error_code error;
    std::filesystem::copy_file(from, to, std::filesystem::copy_options::overwrite_existing, error);
    if (!error) return TRUE;
    g_last_error = error_from_errno(error.value());
    return FALSE;
}

BOOL MoveFileA(LPCSTR existing_file_name, LPCSTR new_file_name) {
    std::error_code error;
    std::filesystem::rename(host_path(existing_file_name), normalize_path(new_file_name), error);
    if (!error) return TRUE;
    g_last_error = error_from_errno(error.value());
    return FALSE;
}

BOOL MoveFileExA(LPCSTR existing_file_name, LPCSTR new_file_name, DWORD flags) {
    if ((flags & MOVEFILE_REPLACE_EXISTING) != 0) {
        std::error_code ignored;
        std::filesystem::remove(normalize_path(new_file_name), ignored);
    }
    return MoveFileA(existing_file_name, new_file_name);
}

BOOL ReplaceFileA(LPCSTR replaced_file_name, LPCSTR replacement_file_name,
                  LPCSTR, DWORD, LPVOID, LPVOID) {
    return MoveFileExA(replacement_file_name, replaced_file_name,
                       MOVEFILE_REPLACE_EXISTING);
}

HANDLE FindFirstFileA(LPCSTR file_name, LPWIN32_FIND_DATAA find_file_data) {
    const auto path = normalize_path(file_name);
    const auto directory = path.parent_path().empty() ? std::filesystem::current_path()
                                                      : resolve_case_path(path.parent_path());
    const std::string pattern = path.filename().string();
    auto* find = new FindHandle();
    std::error_code error;
    for (const auto& entry : std::filesystem::directory_iterator(directory, error)) {
        if (match_pattern(entry.path().filename().string(), pattern)) {
            find->entries.push_back(entry);
        }
    }
    if (error || find->entries.empty()) {
        delete find;
        g_last_error = ERROR_NO_MORE_FILES;
        return INVALID_HANDLE_VALUE;
    }
    fill_find_data(find->entries.front(), find_file_data);
    return find;
}

BOOL FindNextFileA(HANDLE find_file, LPWIN32_FIND_DATAA find_file_data) {
    auto* find = static_cast<FindHandle*>(find_file);
    if (find == nullptr || find->magic != 0x46494e44u ||
        ++find->index >= find->entries.size()) {
        g_last_error = ERROR_NO_MORE_FILES;
        return FALSE;
    }
    fill_find_data(find->entries[find->index], find_file_data);
    return TRUE;
}

BOOL FindClose(HANDLE find_file) {
    auto* find = static_cast<FindHandle*>(find_file);
    if (find == nullptr || find->magic != 0x46494e44u) return FALSE;
    find->magic = 0;
    delete find;
    return TRUE;
}

VOID GetLocalTime(SYSTEMTIME* system_time) {
    if (system_time == nullptr) return;
    const std::time_t now = std::time(nullptr);
    std::tm local{};
    localtime_r(&now, &local);
    system_time->wYear = static_cast<WORD>(local.tm_year + 1900);
    system_time->wMonth = static_cast<WORD>(local.tm_mon + 1);
    system_time->wDayOfWeek = static_cast<WORD>(local.tm_wday);
    system_time->wDay = static_cast<WORD>(local.tm_mday);
    system_time->wHour = static_cast<WORD>(local.tm_hour);
    system_time->wMinute = static_cast<WORD>(local.tm_min);
    system_time->wSecond = static_cast<WORD>(local.tm_sec);
    system_time->wMilliseconds = 0;
}

ULONGLONG GetTickCount64(void) {
    using namespace std::chrono;
    return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

VOID Sleep(DWORD milliseconds) { usleep(milliseconds * 1000u); }

BOOL FileTimeToLocalFileTime(const FILETIME* file_time, FILETIME* local_file_time) {
    if (file_time == nullptr || local_file_time == nullptr) return FALSE;
    *local_file_time = *file_time;
    return TRUE;
}

BOOL FileTimeToDosDateTime(const FILETIME*, WORD* fat_date, WORD* fat_time) {
    if (fat_date != nullptr) *fat_date = 0;
    if (fat_time != nullptr) *fat_time = 0;
    return TRUE;
}

BOOL GetDiskFreeSpaceExA(LPCSTR, ULARGE_INTEGER* free_bytes_available,
                         ULARGE_INTEGER* total_number_of_bytes,
                         ULARGE_INTEGER* total_number_of_free_bytes) {
    constexpr ULONGLONG bytes = 1024ull * 1024ull * 1024ull;
    if (free_bytes_available != nullptr) free_bytes_available->QuadPart = bytes;
    if (total_number_of_bytes != nullptr) total_number_of_bytes->QuadPart = bytes;
    if (total_number_of_free_bytes != nullptr) total_number_of_free_bytes->QuadPart = bytes;
    return TRUE;
}

UINT GetDriveTypeA(LPCSTR) { return DRIVE_REMOTE; }

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
    if (name == nullptr) return 0;
    const char* value = std::getenv(name);
    if (value == nullptr) return 0;
    const DWORD needed = static_cast<DWORD>(std::strlen(value));
    if (buffer != nullptr && size != 0) {
        lstrcpynA(buffer, value, static_cast<int>(size));
    }
    return needed >= size ? needed + 1 : needed;
}

BOOL SetEnvironmentVariableA(LPCSTR name, LPCSTR value) {
    if (name == nullptr) return FALSE;
    if (value == nullptr) return unsetenv(name) == 0 ? TRUE : FALSE;
    return setenv(name, value, 1) == 0 ? TRUE : FALSE;
}

USHORT CaptureStackBackTrace(DWORD, DWORD, PVOID*, DWORD*) { return 0; }
VOID GetCurrentThreadStackLimits(ULONG_PTR* low_limit, ULONG_PTR* high_limit) {
    if (low_limit != nullptr) *low_limit = 0;
    if (high_limit != nullptr) *high_limit = ~(ULONG_PTR)0;
}

BOOL SetDefaultDllDirectories(DWORD) { return TRUE; }
BOOL SetSearchPathMode(DWORD) { return TRUE; }
PTOP_LEVEL_EXCEPTION_FILTER SetUnhandledExceptionFilter(PTOP_LEVEL_EXCEPTION_FILTER filter) {
    return filter;
}
PVOID AddVectoredExceptionHandler(ULONG, PVECTORED_EXCEPTION_HANDLER) {
    return reinterpret_cast<PVOID>(1);
}

VOID AcquireSRWLockShared(PSRWLOCK lock) {
    if (auto* native = lock_from_srw(lock)) native->lock_shared();
}
VOID ReleaseSRWLockShared(PSRWLOCK lock) {
    if (auto* native = lock_from_srw(lock)) native->unlock_shared();
}
VOID AcquireSRWLockExclusive(PSRWLOCK lock) {
    if (auto* native = lock_from_srw(lock)) native->lock();
}
VOID ReleaseSRWLockExclusive(PSRWLOCK lock) {
    if (auto* native = lock_from_srw(lock)) native->unlock();
}

}  // extern "C"
