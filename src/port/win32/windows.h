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
#ifndef INVALID_HANDLE_VALUE
#define INVALID_HANDLE_VALUE ((HANDLE)(intptr_t)-1)
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
