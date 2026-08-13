#pragma once

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef NULL
#ifdef __cplusplus
#define NULL 0
#else
#define NULL ((void*)0)
#endif
#endif

#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FAR
#define FAR
#endif
#ifndef NEAR
#define NEAR
#endif
#ifndef PASCAL
#define PASCAL
#endif
#ifndef CALLBACK
#define CALLBACK
#endif

typedef void VOID;
typedef char CHAR;
typedef uint8_t BYTE;
typedef int BOOL;
typedef int16_t SHORT;
typedef uint16_t WORD;
typedef uint32_t UINT;
typedef int32_t LONG;
typedef uint32_t DWORD;
typedef uint64_t ULONGLONG;
typedef uintptr_t DWORD_PTR;
typedef uintptr_t UINT_PTR;
typedef uintptr_t ULONG_PTR;
typedef uintptr_t SIZE_T;
typedef intptr_t LPARAM;
typedef uintptr_t WPARAM;
typedef intptr_t LRESULT;
typedef void* LPVOID;
typedef const void* LPCVOID;
typedef char* LPSTR;
typedef const char* LPCSTR;
#ifdef __cplusplus
typedef char16_t WCHAR;
#else
typedef uint16_t WCHAR;
#endif
typedef WCHAR* LPWSTR;
typedef const WCHAR* LPCWSTR;
typedef WCHAR* PWSTR;
typedef const WCHAR* PCWSTR;

#ifdef __cplusplus
static_assert(sizeof(WCHAR) == 2, "");
#else
typedef char OpusWcharMustBe16Bits[(sizeof(WCHAR) == 2) ? 1 : -1];
#endif

typedef void* HANDLE;
typedef HANDLE HINSTANCE;
typedef HANDLE HMODULE;
typedef HANDLE HWND;
typedef HANDLE HMENU;
typedef HANDLE HDC;
typedef HANDLE HGDIOBJ;
typedef HANDLE HBITMAP;
typedef HANDLE HBRUSH;
typedef HANDLE HFONT;
typedef HANDLE HPEN;
typedef HANDLE HRGN;
typedef HANDLE HICON;
typedef HANDLE HCURSOR;
typedef HANDLE FARPROC;
typedef WORD ATOM;
typedef int HFILE;

typedef struct RTL_SRWLOCK {
    LPVOID Ptr;
} SRWLOCK, *PSRWLOCK;

typedef struct tagPOINT {
    LONG x;
    LONG y;
} POINT;

typedef struct tagRECT {
    LONG left;
    LONG top;
    LONG right;
    LONG bottom;
} RECT;

typedef struct tagSIZE {
    LONG cx;
    LONG cy;
} SIZE;

typedef struct tagMSG {
    HWND hwnd;
    UINT message;
    WPARAM wParam;
    LPARAM lParam;
    DWORD time;
    POINT pt;
} MSG;
typedef MSG* LPMSG;

typedef struct tagBITMAPCOREHEADER {
    DWORD bcSize;
    WORD bcWidth;
    WORD bcHeight;
    WORD bcPlanes;
    WORD bcBitCount;
} BITMAPCOREHEADER;

typedef struct tagBITMAP {
    LONG bmType;
    LONG bmWidth;
    LONG bmHeight;
    LONG bmWidthBytes;
    WORD bmPlanes;
    WORD bmBitsPixel;
    LPVOID bmBits;
} BITMAP;

#ifndef MAX_PATH
#define MAX_PATH 260
#endif

typedef struct _FILETIME {
    DWORD dwLowDateTime;
    DWORD dwHighDateTime;
} FILETIME;

typedef struct _WIN32_FILE_ATTRIBUTE_DATA {
    DWORD dwFileAttributes;
    FILETIME ftCreationTime;
    FILETIME ftLastAccessTime;
    FILETIME ftLastWriteTime;
    DWORD nFileSizeHigh;
    DWORD nFileSizeLow;
} WIN32_FILE_ATTRIBUTE_DATA;

typedef struct _WIN32_FIND_DATAA {
    DWORD dwFileAttributes;
    FILETIME ftCreationTime;
    FILETIME ftLastAccessTime;
    FILETIME ftLastWriteTime;
    DWORD nFileSizeHigh;
    DWORD nFileSizeLow;
    DWORD dwReserved0;
    DWORD dwReserved1;
    CHAR cFileName[MAX_PATH];
    CHAR cAlternateFileName[14];
} WIN32_FIND_DATAA, *LPWIN32_FIND_DATAA;

#ifndef LOWORD
#define LOWORD(value) ((WORD)((DWORD_PTR)(value) & 0xffffu))
#endif
#ifndef HIWORD
#define HIWORD(value) ((WORD)(((DWORD_PTR)(value) >> 16u) & 0xffffu))
#endif
#ifndef MAKELONG
#define MAKELONG(low, high) \
    ((LONG)(((WORD)(low)) | ((DWORD)((WORD)(high)) << 16u)))
#endif

#ifndef MB_OK
#define MB_OK 0x0000
#endif
#ifndef MB_ICONEXCLAMATION
#define MB_ICONEXCLAMATION 0x0030
#endif

#ifndef MF_CHANGE
#define MF_CHANGE 0x0080
#endif
#ifndef MF_INSERT
#define MF_INSERT 0x0000
#endif
#ifndef MF_APPEND
#define MF_APPEND 0x0100
#endif
#ifndef MF_DELETE
#define MF_DELETE 0x0200
#endif
#ifndef MF_BYPOSITION
#define MF_BYPOSITION 0x0400
#endif
#ifndef MF_SEPARATOR
#define MF_SEPARATOR 0x0800
#endif
#ifndef MF_REMOVE
#define MF_REMOVE 0x1000
#endif
#ifndef MF_BYCOMMAND
#define MF_BYCOMMAND 0x0000
#endif
#ifndef MF_STRING
#define MF_STRING 0x0000
#endif
#ifndef MF_POPUP
#define MF_POPUP 0x0010
#endif

#ifndef OBJ_FONT
#define OBJ_FONT 6
#endif

#ifndef FORMAT_MESSAGE_IGNORE_INSERTS
#define FORMAT_MESSAGE_IGNORE_INSERTS 0x00000200
#endif
#ifndef FORMAT_MESSAGE_FROM_SYSTEM
#define FORMAT_MESSAGE_FROM_SYSTEM 0x00001000
#endif

#ifndef GENERIC_READ
#define GENERIC_READ 0x80000000u
#endif
#ifndef GENERIC_WRITE
#define GENERIC_WRITE 0x40000000u
#endif
#ifndef FILE_APPEND_DATA
#define FILE_APPEND_DATA 0x00000004
#endif
#ifndef FILE_SHARE_READ
#define FILE_SHARE_READ 0x00000001
#endif
#ifndef FILE_SHARE_WRITE
#define FILE_SHARE_WRITE 0x00000002
#endif
#ifndef CREATE_NEW
#define CREATE_NEW 1
#endif
#ifndef CREATE_ALWAYS
#define CREATE_ALWAYS 2
#endif
#ifndef OPEN_EXISTING
#define OPEN_EXISTING 3
#endif
#ifndef OPEN_ALWAYS
#define OPEN_ALWAYS 4
#endif
#ifndef FILE_ATTRIBUTE_NORMAL
#define FILE_ATTRIBUTE_NORMAL 0x00000080
#endif
#ifndef FILE_ATTRIBUTE_READONLY
#define FILE_ATTRIBUTE_READONLY 0x00000001
#endif
#ifndef FILE_ATTRIBUTE_HIDDEN
#define FILE_ATTRIBUTE_HIDDEN 0x00000002
#endif
#ifndef FILE_ATTRIBUTE_SYSTEM
#define FILE_ATTRIBUTE_SYSTEM 0x00000004
#endif
#ifndef FILE_ATTRIBUTE_DIRECTORY
#define FILE_ATTRIBUTE_DIRECTORY 0x00000010
#endif
#ifndef FILE_ATTRIBUTE_ARCHIVE
#define FILE_ATTRIBUTE_ARCHIVE 0x00000020
#endif
#ifndef FILE_ATTRIBUTE_REPARSE_POINT
#define FILE_ATTRIBUTE_REPARSE_POINT 0x00000400
#endif
#ifndef INVALID_FILE_ATTRIBUTES
#define INVALID_FILE_ATTRIBUTES ((DWORD)-1)
#endif
#ifndef FILE_BEGIN
#define FILE_BEGIN 0
#endif
#ifndef GetFileExInfoStandard
#define GetFileExInfoStandard 0
#endif
#ifndef REPLACEFILE_WRITE_THROUGH
#define REPLACEFILE_WRITE_THROUGH 0x00000001
#endif
#ifndef MOVEFILE_REPLACE_EXISTING
#define MOVEFILE_REPLACE_EXISTING 0x00000001
#endif
#ifndef MOVEFILE_WRITE_THROUGH
#define MOVEFILE_WRITE_THROUGH 0x00000008
#endif
#ifndef INVALID_HANDLE_VALUE
#define INVALID_HANDLE_VALUE ((HANDLE)(intptr_t)-1)
#endif

#ifndef ERROR_FILE_NOT_FOUND
#define ERROR_FILE_NOT_FOUND 2
#endif
#ifndef ERROR_PATH_NOT_FOUND
#define ERROR_PATH_NOT_FOUND 3
#endif
#ifndef ERROR_TOO_MANY_OPEN_FILES
#define ERROR_TOO_MANY_OPEN_FILES 4
#endif
#ifndef ERROR_ACCESS_DENIED
#define ERROR_ACCESS_DENIED 5
#endif
#ifndef ERROR_INVALID_HANDLE
#define ERROR_INVALID_HANDLE 6
#endif
#ifndef ERROR_NO_MORE_FILES
#define ERROR_NO_MORE_FILES 18
#endif
#ifndef ERROR_NOT_SAME_DEVICE
#define ERROR_NOT_SAME_DEVICE 17
#endif
#ifndef ERROR_WRITE_PROTECT
#define ERROR_WRITE_PROTECT 19
#endif
#ifndef ERROR_NOT_READY
#define ERROR_NOT_READY 21
#endif
#ifndef ERROR_SHARING_VIOLATION
#define ERROR_SHARING_VIOLATION 32
#endif
#ifndef ERROR_HANDLE_DISK_FULL
#define ERROR_HANDLE_DISK_FULL 39
#endif
#ifndef ERROR_FILE_EXISTS
#define ERROR_FILE_EXISTS 80
#endif
#ifndef ERROR_DISK_FULL
#define ERROR_DISK_FULL 112
#endif
#ifndef ERROR_ALREADY_EXISTS
#define ERROR_ALREADY_EXISTS 183
#endif
#ifndef ERROR_CLASS_ALREADY_EXISTS
#define ERROR_CLASS_ALREADY_EXISTS 1410
#endif

#ifndef HEAP_ZERO_MEMORY
#define HEAP_ZERO_MEMORY 0x00000008
#endif
#ifndef EXCEPTION_NONCONTINUABLE
#define EXCEPTION_NONCONTINUABLE 0x00000001
#endif
#ifndef SRWLOCK_INIT
#define SRWLOCK_INIT { NULL }
#endif

#ifndef LOCALE_USER_DEFAULT
#define LOCALE_USER_DEFAULT 0x0400
#endif
#ifndef CT_CTYPE1
#define CT_CTYPE1 0x0001
#endif
#ifndef C1_UPPER
#define C1_UPPER 0x0001
#endif
#ifndef C1_LOWER
#define C1_LOWER 0x0002
#endif
#ifndef C1_DIGIT
#define C1_DIGIT 0x0004
#endif
#ifndef C1_ALPHA
#define C1_ALPHA 0x0100
#endif

FARPROC GetProcAddress(HMODULE module, LPCSTR name);
HMODULE GetModuleHandleW(LPCWSTR module_name);
DWORD GetTempPathA(DWORD buffer_length, LPSTR buffer);
UINT GetTempFileNameA(LPCSTR path_name, LPCSTR prefix, UINT unique, LPSTR file_name);
DWORD GetCurrentProcessId(void);
DWORD CharUpperBuffA(LPSTR text, DWORD length);
UINT RegisterClipboardFormatA(LPCSTR name);
BOOL MessageBeep(UINT type);
BOOL GetStringTypeA(DWORD locale, DWORD type, LPCSTR source, int count,
                    WORD* char_type);

VOID OutputDebugStringA(LPCSTR text);
DWORD FormatMessageA(DWORD flags, LPCVOID source, DWORD message_id,
                     DWORD language_id, LPSTR buffer, DWORD size,
                     LPVOID arguments);
DWORD GetModuleFileNameA(HMODULE module, LPSTR file_name, DWORD size);
HANDLE GetCurrentProcess(void);
BOOL TerminateProcess(HANDLE process, UINT exit_code);
VOID RaiseException(DWORD code, DWORD flags, DWORD argument_count,
                    const ULONG_PTR* arguments);

BOOL AppendMenuA(HMENU menu, UINT flags, UINT_PTR new_item, LPCSTR new_item_text);
BOOL DeleteMenu(HMENU menu, UINT position, UINT flags);
BOOL RemoveMenu(HMENU menu, UINT position, UINT flags);
BOOL ModifyMenuA(HMENU menu, UINT position, UINT flags, UINT_PTR new_item,
                 LPCSTR new_item_text);
BOOL InsertMenuA(HMENU menu, UINT position, UINT flags, UINT_PTR new_item,
                 LPCSTR new_item_text);
UINT GetMenuItemID(HMENU menu, int position);

HGDIOBJ GetCurrentObject(HDC device_context, UINT object_type);
BOOL GetBitmapDimensionEx(HBITMAP bitmap, SIZE* size);
BOOL SetBitmapDimensionEx(HBITMAP bitmap, int width, int height, SIZE* previous);
BOOL GetViewportExtEx(HDC device_context, SIZE* size);
BOOL GetViewportOrgEx(HDC device_context, POINT* point);
BOOL SetViewportExtEx(HDC device_context, int width, int height, SIZE* previous);
BOOL SetViewportOrgEx(HDC device_context, int x, int y, POINT* previous);
BOOL SetWindowExtEx(HDC device_context, int width, int height, SIZE* previous);
BOOL SetWindowOrgEx(HDC device_context, int x, int y, POINT* previous);
BOOL GetTextExtentPoint32A(HDC device_context, LPCSTR text, int count, SIZE* size);
BOOL MoveToEx(HDC device_context, int x, int y, POINT* previous);

HANDLE CreateFileA(LPCSTR file_name, DWORD desired_access, DWORD share_mode,
                   LPVOID security_attributes, DWORD creation_disposition,
                   DWORD flags_and_attributes, HANDLE template_file);
HANDLE CreateFileW(LPCWSTR file_name, DWORD desired_access, DWORD share_mode,
                   LPVOID security_attributes, DWORD creation_disposition,
                   DWORD flags_and_attributes, HANDLE template_file);
BOOL ReadFile(HANDLE file, LPVOID buffer, DWORD bytes_to_read, DWORD* bytes_read,
              LPVOID overlapped);
BOOL WriteFile(HANDLE file, LPCVOID buffer, DWORD bytes_to_write,
               DWORD* bytes_written, LPVOID overlapped);
BOOL FlushFileBuffers(HANDLE file);
BOOL CloseHandle(HANDLE object);
DWORD GetLastError(void);
DWORD GetFileAttributesA(LPCSTR file_name);
BOOL GetFileAttributesExA(LPCSTR file_name, int info_level_id,
                          LPVOID file_information);
BOOL CreateDirectoryA(LPCSTR path_name, LPVOID security_attributes);
BOOL RemoveDirectoryA(LPCSTR path_name);
BOOL DeleteFileA(LPCSTR file_name);
BOOL CopyFileA(LPCSTR existing_file_name, LPCSTR new_file_name,
               BOOL fail_if_exists);
BOOL ReplaceFileA(LPCSTR replaced_file_name, LPCSTR replacement_file_name,
                  LPCSTR backup_file_name, DWORD replace_flags,
                  LPVOID exclude, LPVOID reserved);
BOOL MoveFileA(LPCSTR existing_file_name, LPCSTR new_file_name);
BOOL MoveFileExA(LPCSTR existing_file_name, LPCSTR new_file_name, DWORD flags);
HANDLE FindFirstFileA(LPCSTR file_name, LPWIN32_FIND_DATAA find_file_data);
BOOL FindNextFileA(HANDLE find_file, LPWIN32_FIND_DATAA find_file_data);
BOOL FindClose(HANDLE find_file);

HANDLE GetProcessHeap(void);
LPVOID HeapAlloc(HANDLE heap, DWORD flags, SIZE_T bytes);
LPVOID HeapReAlloc(HANDLE heap, DWORD flags, LPVOID memory, SIZE_T bytes);
BOOL HeapFree(HANDLE heap, DWORD flags, LPVOID memory);
SIZE_T HeapSize(HANDLE heap, DWORD flags, LPCVOID memory);
SIZE_T HeapCompact(HANDLE heap, DWORD flags);

VOID AcquireSRWLockShared(PSRWLOCK lock);
VOID ReleaseSRWLockShared(PSRWLOCK lock);
VOID AcquireSRWLockExclusive(PSRWLOCK lock);
VOID ReleaseSRWLockExclusive(PSRWLOCK lock);

#ifdef __cplusplus
}
#endif
