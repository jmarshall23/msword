#include "opus_x64_compat.h"

#ifdef OpenFile
#undef OpenFile
#endif

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <limits>

/* AMD64 translation of FILEWINN.ASM. Word continues to open files through its
 * original OpenFile-based C code; these routines provide the native
 * read/write/seek/close and clock boundary that DOS interrupt 21h supplied.
 */

namespace {

int negative_last_error() noexcept {
    const DWORD error = GetLastError();
    return -static_cast<int>(error == ERROR_SUCCESS ? ERROR_INVALID_HANDLE
                                                     : error);
}

struct NativeTime {
    char minutes;
    char hour;
    char hundredths;
    char seconds;
};

struct NativeDate {
    int year;
    char month;
    char day;
    char day_of_week;
};

/* Prefix of the original BPTB through the native page-cache allocation. */
struct NativeBptbPrefix {
    int ibp_max;
    int ibp_mac;
    int hash_count;
    int* hash;
    void* page_descriptors;
    unsigned char (*pages)[512];
};

BYTE* ofs_reserved(OFSTRUCT& ofs) noexcept {
#ifdef _WIN32
    return ofs.Reserved1;
#else
    return ofs.reserved;
#endif
}

}  // namespace

extern "C" {

extern NativeBptbPrefix vbptbExt;

int OpusOpenFile(const LPCSTR file_name, void* const legacy_ofs,
                 const UINT style) {
    struct LegacyOfStruct {
        int byte_count : 8;
        int fixed_disk : 8;
        WORD error_code;
        BYTE reserved[4];
        CHAR path[120];
    };

    if (legacy_ofs == nullptr) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return HFILE_ERROR;
    }

    auto* const legacy = static_cast<LegacyOfStruct*>(legacy_ofs);
    OFSTRUCT native_ofs{};
    native_ofs.cBytes = sizeof(native_ofs);
    native_ofs.fFixedDisk = legacy->fixed_disk;
    native_ofs.nErrCode = legacy->error_code;
    std::memcpy(ofs_reserved(native_ofs), legacy->reserved,
                sizeof(legacy->reserved));
    lstrcpynA(native_ofs.szPathName, legacy->path,
              static_cast<int>(sizeof(native_ofs.szPathName)));

    const HFILE result = OpenFile(file_name, &native_ofs, style);
    legacy->byte_count = native_ofs.cBytes;
    legacy->fixed_disk = native_ofs.fFixedDisk;
    legacy->error_code = native_ofs.nErrCode;
    std::memcpy(legacy->reserved, ofs_reserved(native_ofs),
                sizeof(legacy->reserved));
    lstrcpynA(legacy->path, native_ofs.szPathName,
              static_cast<int>(sizeof(legacy->path)));
    return result;
}

int FCloseDoshnd(const HFILE file) {
    if (file == HFILE_ERROR) {
        return 0;
    }
    return _lclose(file) == 0;
}

int CchReadDoshnd(const HFILE file, void* buffer, const unsigned int bytes) {
    const UINT count = _lread(file, buffer, bytes);
    return count == HFILE_ERROR ? negative_last_error()
                                : static_cast<int>(count);
}

int CchWriteDoshnd(const HFILE file, const void* buffer,
                   const unsigned int bytes) {
    const UINT count = _lwrite(file, static_cast<LPCCH>(buffer), bytes);
    return count == HFILE_ERROR ? negative_last_error()
                                : static_cast<int>(count);
}

long DwSeekDw(const HFILE file, const long offset, const int origin) {
    const LONG result = _llseek(file, offset, origin);
    return result == HFILE_ERROR ? static_cast<long>(negative_last_error())
                                 : static_cast<long>(result);
}

int FDoshndIsFile(const HFILE file) {
    if (file == HFILE_ERROR) {
        return 0;
    }
    const HANDLE handle = reinterpret_cast<HANDLE>(
        static_cast<std::intptr_t>(file));
    const DWORD kind = GetFileType(handle);
    return kind == FILE_TYPE_DISK;
}

int DosxError() {
    const DWORD error = GetLastError();
    return static_cast<int>((std::min)(
        error, static_cast<DWORD>((std::numeric_limits<int>::max)())));
}

unsigned char* HpOfBptbExt(const int page_index) {
    if (vbptbExt.pages == nullptr || page_index < 0 ||
        page_index >= vbptbExt.ibp_max) {
        return nullptr;
    }
    return vbptbExt.pages[page_index];
}

unsigned char* HpBaseForIbp(const int page_index) {
    return HpOfBptbExt(page_index);
}

void OsTime(NativeTime* time) {
    if (time == nullptr) {
        return;
    }
    SYSTEMTIME system_time{};
    GetLocalTime(&system_time);
    time->minutes = static_cast<char>(system_time.wMinute);
    time->hour = static_cast<char>(system_time.wHour);
    time->hundredths = static_cast<char>(system_time.wMilliseconds / 10);
    time->seconds = static_cast<char>(system_time.wSecond);
}

void OsDate(NativeDate* date) {
    if (date == nullptr) {
        return;
    }
    SYSTEMTIME system_time{};
    GetLocalTime(&system_time);
    date->year = static_cast<int>(system_time.wYear);
    date->month = static_cast<char>(system_time.wMonth);
    date->day = static_cast<char>(system_time.wDay);
    date->day_of_week = static_cast<char>(system_time.wDayOfWeek);
}

}  // extern "C"
