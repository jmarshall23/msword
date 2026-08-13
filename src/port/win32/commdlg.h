#pragma once

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct tagOFNA {
    DWORD lStructSize;
    HWND hwndOwner;
    LPCSTR lpstrFilter;
    DWORD nFilterIndex;
    LPSTR lpstrFile;
    DWORD nMaxFile;
    LPCSTR lpstrInitialDir;
    LPCSTR lpstrTitle;
    DWORD Flags;
    LPCSTR lpstrDefExt;
} OPENFILENAMEA, *LPOPENFILENAMEA;

typedef struct tagOFNW {
    DWORD lStructSize;
    HWND hwndOwner;
    LPCWSTR lpstrFilter;
    DWORD nFilterIndex;
    LPWSTR lpstrFile;
    DWORD nMaxFile;
    LPCWSTR lpstrInitialDir;
    LPCWSTR lpstrTitle;
    DWORD Flags;
    LPCWSTR lpstrDefExt;
} OPENFILENAMEW, *LPOPENFILENAMEW;

#ifndef OFN_READONLY
#define OFN_READONLY 0x00000001
#endif
#ifndef OFN_OVERWRITEPROMPT
#define OFN_OVERWRITEPROMPT 0x00000002
#endif
#ifndef OFN_NOCHANGEDIR
#define OFN_NOCHANGEDIR 0x00000008
#endif
#ifndef OFN_PATHMUSTEXIST
#define OFN_PATHMUSTEXIST 0x00000800
#endif
#ifndef OFN_FILEMUSTEXIST
#define OFN_FILEMUSTEXIST 0x00001000
#endif
#ifndef OFN_NOREADONLYRETURN
#define OFN_NOREADONLYRETURN 0x00008000
#endif
#ifndef OFN_EXPLORER
#define OFN_EXPLORER 0x00080000
#endif
#ifndef OFN_LONGNAMES
#define OFN_LONGNAMES 0x00200000
#endif
#ifndef OFN_ENABLESIZING
#define OFN_ENABLESIZING 0x00800000
#endif

BOOL GetOpenFileNameA(LPOPENFILENAMEA dialog);
BOOL GetSaveFileNameA(LPOPENFILENAMEA dialog);
BOOL GetSaveFileNameW(LPOPENFILENAMEW dialog);
DWORD CommDlgExtendedError(void);

#ifdef __cplusplus
}
#endif
