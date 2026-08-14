#include "opus-native-compat.h"

#ifdef OpenFile
#undef OpenFile
#endif

#include <limits.h>
#include <stdint.h>
#include <string.h>

/* AMD64 translation of FILEWINN.ASM. Word continues to open files through its
 * original OpenFile-based C code; these routines provide the native
 * read/write/seek/close and clock boundary that DOS interrupt 21h supplied.
 */

typedef struct NativeTime {
    char minutes;
    char hour;
    char hundredths;
    char seconds;
} NativeTime;

typedef struct NativeDate {
    int year;
    char month;
    char day;
    char day_of_week;
} NativeDate;

/* Prefix of the original BPTB through the native page-cache allocation. */
typedef struct NativeBptbPrefix {
    int ibp_max;
    int ibp_mac;
    int hash_count;
    int *hash;
    void *page_descriptors;
    unsigned char (*pages)[512];
} NativeBptbPrefix;

extern NativeBptbPrefix vbptbExt;

static int negative_last_error(void) {
    DWORD error = GetLastError();
    return -(int)(error == ERROR_SUCCESS ? ERROR_INVALID_HANDLE : error);
}

static BYTE *ofs_reserved(OFSTRUCT *ofs) {
#ifdef _WIN32
    return (BYTE *)&ofs->Reserved1;
#else
    return ofs->reserved;
#endif
}

int OpusOpenFile(LPCSTR file_name, void *legacy_ofs, UINT style) {
    struct LegacyOfStruct {
        int byte_count : 8;
        int fixed_disk : 8;
        WORD error_code;
        BYTE reserved[4];
        CHAR path[120];
    };
    struct LegacyOfStruct *legacy;
    OFSTRUCT native_ofs = {0};
    HFILE result;

    if (legacy_ofs == NULL) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return HFILE_ERROR;
    }

    legacy = (struct LegacyOfStruct *)legacy_ofs;
    native_ofs.cBytes = sizeof(native_ofs);
    native_ofs.fFixedDisk = legacy->fixed_disk;
    native_ofs.nErrCode = legacy->error_code;
    memcpy(ofs_reserved(&native_ofs), legacy->reserved,
           sizeof(legacy->reserved));
    lstrcpynA(native_ofs.szPathName, legacy->path,
              (int)sizeof(native_ofs.szPathName));

    result = OpenFile(file_name, &native_ofs, style);
    legacy->byte_count = native_ofs.cBytes;
    legacy->fixed_disk = native_ofs.fFixedDisk;
    legacy->error_code = native_ofs.nErrCode;
    memcpy(legacy->reserved, ofs_reserved(&native_ofs),
           sizeof(legacy->reserved));
    lstrcpynA(legacy->path, native_ofs.szPathName, (int)sizeof(legacy->path));
    return result;
}

int FCloseDoshnd(HFILE file) {
    if (file == HFILE_ERROR) {
        return 0;
    }
    return _lclose(file) == 0;
}

int CchReadDoshnd(HFILE file, void *buffer, unsigned int bytes) {
    UINT count = _lread(file, buffer, bytes);
    return count == HFILE_ERROR ? negative_last_error() : (int)count;
}

int CchWriteDoshnd(HFILE file, const void *buffer, unsigned int bytes) {
    UINT count = _lwrite(file, (LPCCH)buffer, bytes);
    return count == HFILE_ERROR ? negative_last_error() : (int)count;
}

long DwSeekDw(HFILE file, long offset, int origin) {
    LONG result = _llseek(file, offset, origin);
    return result == HFILE_ERROR ? (long)negative_last_error()
                                 : (long)result;
}

int FDoshndIsFile(HFILE file) {
    HANDLE handle;
    DWORD kind;

    if (file == HFILE_ERROR) {
        return 0;
    }
    handle = (HANDLE)(intptr_t)file;
    kind = GetFileType(handle);
    return kind == FILE_TYPE_DISK;
}

int DosxError(void) {
    DWORD error = GetLastError();
    return (int)(error > (DWORD)INT_MAX ? (DWORD)INT_MAX : error);
}

unsigned char *HpOfBptbExt(int page_index) {
    if (vbptbExt.pages == NULL || page_index < 0 ||
        page_index >= vbptbExt.ibp_max) {
        return NULL;
    }
    return vbptbExt.pages[page_index];
}

unsigned char *HpBaseForIbp(int page_index) {
    return HpOfBptbExt(page_index);
}

void OsTime(NativeTime *time) {
    SYSTEMTIME system_time = {0};

    if (time == NULL) {
        return;
    }
    GetLocalTime(&system_time);
    time->minutes = (char)system_time.wMinute;
    time->hour = (char)system_time.wHour;
    time->hundredths = (char)(system_time.wMilliseconds / 10);
    time->seconds = (char)system_time.wSecond;
}

void OsDate(NativeDate *date) {
    SYSTEMTIME system_time = {0};

    if (date == NULL) {
        return;
    }
    GetLocalTime(&system_time);
    date->year = (int)system_time.wYear;
    date->month = (char)system_time.wMonth;
    date->day = (char)system_time.wDay;
    date->day_of_week = (char)system_time.wDayOfWeek;
}
