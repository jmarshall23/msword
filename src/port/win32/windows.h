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
#ifndef WINAPI
#define WINAPI
#endif
#ifndef __cdecl
#define __cdecl
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
typedef uint64_t DWORDLONG;
typedef uint64_t ULONGLONG;
typedef uint64_t DWORD64;
typedef uint64_t ULONG64;
typedef uint32_t ULONG;
typedef uint16_t USHORT;
typedef uintptr_t DWORD_PTR;
typedef uintptr_t UINT_PTR;
typedef uintptr_t ULONG_PTR;
typedef uintptr_t SIZE_T;
typedef intptr_t LPARAM;
typedef intptr_t LONG_PTR;
typedef uintptr_t WPARAM;
typedef intptr_t LRESULT;
typedef intptr_t INT_PTR;
typedef LONG HRESULT;
typedef void* LPVOID;
typedef void* PVOID;
typedef const void* LPCVOID;
typedef char* LPSTR;
typedef const char* LPCSTR;
typedef const CHAR* LPCCH;
typedef CHAR* PCHAR;
typedef int FAR *LPINT;
#ifdef __cplusplus
typedef char16_t WCHAR;
#else
typedef uint16_t WCHAR;
#endif
typedef WCHAR* LPWSTR;
typedef WCHAR* LPWCH;
typedef const WCHAR* LPCWSTR;
typedef WCHAR* PWSTR;
typedef const WCHAR* PCWSTR;

#ifdef __cplusplus
static_assert(sizeof(WCHAR) == 2, "");
#else
typedef char OpusWcharMustBe16Bits[(sizeof(WCHAR) == 2) ? 1 : -1];
#endif

typedef void* HANDLE;
typedef HANDLE HLOCAL;
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
typedef HANDLE HMETAFILE;
typedef HANDLE HICON;
typedef HANDLE HCURSOR;
typedef int(FAR PASCAL *FARPROC)();
typedef WORD ATOM;
typedef int HFILE;

typedef struct RTL_SRWLOCK {
    LPVOID Ptr;
} SRWLOCK, *PSRWLOCK;

typedef struct tagPOINT {
    LONG x;
    LONG y;
} POINT;
typedef POINT* LPPOINT;

typedef struct tagLOGPEN {
    WORD lopnStyle;
    POINT lopnWidth;
    DWORD lopnColor;
} LOGPEN;
typedef LOGPEN* PLOGPEN;
typedef LOGPEN* NPLOGPEN;
typedef LOGPEN* LPLOGPEN;

typedef struct tagRECT {
    LONG left;
    LONG top;
    LONG right;
    LONG bottom;
} RECT;
typedef RECT* LPRECT;

typedef struct _MEMORYSTATUSEX {
    DWORD dwLength;
    DWORD dwMemoryLoad;
    DWORDLONG ullTotalPhys;
    DWORDLONG ullAvailPhys;
    DWORDLONG ullTotalPageFile;
    DWORDLONG ullAvailPageFile;
    DWORDLONG ullTotalVirtual;
    DWORDLONG ullAvailVirtual;
    DWORDLONG ullAvailExtendedVirtual;
} MEMORYSTATUSEX, *LPMEMORYSTATUSEX;

typedef struct tagMETAFILEPICT {
    int mm;
    int xExt;
    int yExt;
    HANDLE hMF;
} METAFILEPICT;
typedef METAFILEPICT* LPMETAFILEPICT;
typedef struct tagHANDLETABLE {
    HANDLE objectHandle[1];
} HANDLETABLE;
typedef HANDLETABLE* PHANDLETABLE;
typedef HANDLETABLE* LPHANDLETABLE;
typedef struct tagMETARECORD {
    DWORD rdSize;
    WORD rdFunction;
    WORD rdParm[1];
} METARECORD;
typedef METARECORD* PMETARECORD;
typedef METARECORD* LPMETARECORD;

typedef struct tagMENUINFO {
    DWORD cbSize;
    DWORD fMask;
    DWORD dwStyle;
    UINT cyMax;
    HBRUSH hbrBack;
    DWORD dwContextHelpID;
    ULONG_PTR dwMenuData;
} MENUINFO, *LPMENUINFO;

typedef struct tagPAINTSTRUCT {
    HDC hdc;
    BOOL fErase;
    RECT rcPaint;
    BOOL fRestore;
    BOOL fIncUpdate;
    BYTE rgbReserved[16];
} PAINTSTRUCT;
typedef PAINTSTRUCT* LPPAINTSTRUCT;

typedef struct tagOFSTRUCT {
    BYTE cBytes;
    BYTE fFixedDisk;
    WORD nErrCode;
    BYTE reserved[4];
    CHAR szPathName[120];
} OFSTRUCT;
typedef OFSTRUCT* LPOFSTRUCT;

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
#define OPUS_WIN32_HAS_BITMAP_TYPES 1
typedef BITMAPCOREHEADER* LPBITMAPCOREHEADER;

typedef struct tagBITMAPFILEHEADER {
    WORD bfType;
    DWORD bfSize;
    WORD bfReserved1;
    WORD bfReserved2;
    DWORD bfOffBits;
} BITMAPFILEHEADER, *PBITMAPFILEHEADER, *LPBITMAPFILEHEADER;

typedef struct tagRGBTRIPLE {
    BYTE rgbtBlue;
    BYTE rgbtGreen;
    BYTE rgbtRed;
} RGBTRIPLE;

typedef struct tagRGBQUAD {
    BYTE rgbBlue;
    BYTE rgbGreen;
    BYTE rgbRed;
    BYTE rgbReserved;
} RGBQUAD;

typedef struct tagBITMAPINFOHEADER {
    DWORD biSize;
    LONG biWidth;
    LONG biHeight;
    WORD biPlanes;
    WORD biBitCount;
    DWORD biCompression;
    DWORD biSizeImage;
    LONG biXPelsPerMeter;
    LONG biYPelsPerMeter;
    DWORD biClrUsed;
    DWORD biClrImportant;
} BITMAPINFOHEADER;
typedef BITMAPINFOHEADER* LPBITMAPINFOHEADER;

typedef struct tagBITMAPINFO {
    BITMAPINFOHEADER bmiHeader;
    RGBQUAD bmiColors[1];
} BITMAPINFO;
typedef BITMAPINFO* LPBITMAPINFO;

typedef struct tagBITMAP {
    LONG bmType;
    LONG bmWidth;
    LONG bmHeight;
    LONG bmWidthBytes;
    WORD bmPlanes;
    WORD bmBitsPixel;
    LPVOID bmBits;
} BITMAP;
typedef BITMAP* LPBITMAP;

typedef LRESULT(CALLBACK* WNDPROC)(HWND, UINT, WPARAM, LPARAM);
typedef BOOL(CALLBACK* WNDENUMPROC)(HWND, LPARAM);

typedef struct tagWNDCLASSA {
    UINT style;
    WNDPROC lpfnWndProc;
    int cbClsExtra;
    int cbWndExtra;
    HINSTANCE hInstance;
    HICON hIcon;
    HCURSOR hCursor;
    HBRUSH hbrBackground;
    LPCSTR lpszMenuName;
    LPCSTR lpszClassName;
} WNDCLASSA, WNDCLASS, *PWNDCLASSA, *LPWNDCLASSA, *PWNDCLASS, *LPWNDCLASS;

typedef struct tagWNDCLASSEXA {
    UINT cbSize;
    UINT style;
    WNDPROC lpfnWndProc;
    int cbClsExtra;
    int cbWndExtra;
    HINSTANCE hInstance;
    HICON hIcon;
    HCURSOR hCursor;
    HBRUSH hbrBackground;
    LPCSTR lpszMenuName;
    LPCSTR lpszClassName;
    HICON hIconSm;
} WNDCLASSEXA, *LPWNDCLASSEXA;

typedef struct tagWNDCLASSEXW {
    UINT cbSize;
    UINT style;
    WNDPROC lpfnWndProc;
    int cbClsExtra;
    int cbWndExtra;
    HINSTANCE hInstance;
    HICON hIcon;
    HCURSOR hCursor;
    HBRUSH hbrBackground;
    LPCWSTR lpszMenuName;
    LPCWSTR lpszClassName;
    HICON hIconSm;
} WNDCLASSEXW, *LPWNDCLASSEXW;

typedef struct tagCREATESTRUCTA {
    LPVOID lpCreateParams;
    HINSTANCE hInstance;
    HMENU hMenu;
    HWND hwndParent;
    int cy;
    int cx;
    int y;
    int x;
    LONG style;
    LPCSTR lpszName;
    LPCSTR lpszClass;
    DWORD dwExStyle;
} CREATESTRUCTA, *LPCREATESTRUCTA;

typedef struct tagCOMBOBOXINFO {
    DWORD cbSize;
    RECT rcItem;
    RECT rcButton;
    DWORD stateButton;
    HWND hwndCombo;
    HWND hwndItem;
    HWND hwndList;
} COMBOBOXINFO, *PCOMBOBOXINFO;

typedef struct tagGUITHREADINFO {
    DWORD cbSize;
    DWORD flags;
    HWND hwndActive;
    HWND hwndFocus;
    HWND hwndCapture;
    HWND hwndMenuOwner;
    HWND hwndMoveSize;
    HWND hwndCaret;
    RECT rcCaret;
} GUITHREADINFO, *PGUITHREADINFO;

typedef struct tagMOUSEINPUT {
    LONG dx;
    LONG dy;
    DWORD mouseData;
    DWORD dwFlags;
    DWORD time;
    ULONG_PTR dwExtraInfo;
} MOUSEINPUT;

typedef struct tagKEYBDINPUT {
    WORD wVk;
    WORD wScan;
    DWORD dwFlags;
    DWORD time;
    ULONG_PTR dwExtraInfo;
} KEYBDINPUT;

typedef struct tagHARDWAREINPUT {
    DWORD uMsg;
    WORD wParamL;
    WORD wParamH;
} HARDWAREINPUT;

typedef struct tagINPUT {
    DWORD type;
    union {
        MOUSEINPUT mi;
        KEYBDINPUT ki;
        HARDWAREINPUT hi;
    };
} INPUT, *PINPUT;

typedef struct tagSTARTUPINFOW {
    DWORD cb;
    LPWSTR lpReserved;
    LPWSTR lpDesktop;
    LPWSTR lpTitle;
    DWORD dwX;
    DWORD dwY;
    DWORD dwXSize;
    DWORD dwYSize;
    DWORD dwXCountChars;
    DWORD dwYCountChars;
    DWORD dwFillAttribute;
    DWORD dwFlags;
    WORD wShowWindow;
    WORD cbReserved2;
    BYTE* lpReserved2;
    HANDLE hStdInput;
    HANDLE hStdOutput;
    HANDLE hStdError;
} STARTUPINFOW, *LPSTARTUPINFOW;

typedef struct tagPROCESS_INFORMATION {
    HANDLE hProcess;
    HANDLE hThread;
    DWORD dwProcessId;
    DWORD dwThreadId;
} PROCESS_INFORMATION, *LPPROCESS_INFORMATION;

#ifndef LF_FACESIZE
#define LF_FACESIZE 32
#endif

typedef struct tagLOGFONTA {
    LONG lfHeight;
    LONG lfWidth;
    LONG lfEscapement;
    LONG lfOrientation;
    LONG lfWeight;
    BYTE lfItalic;
    BYTE lfUnderline;
    BYTE lfStrikeOut;
    BYTE lfCharSet;
    BYTE lfOutPrecision;
    BYTE lfClipPrecision;
    BYTE lfQuality;
    BYTE lfPitchAndFamily;
    CHAR lfFaceName[LF_FACESIZE];
} LOGFONTA, *LPLOGFONTA;

typedef struct tagLOGFONTW {
    LONG lfHeight;
    LONG lfWidth;
    LONG lfEscapement;
    LONG lfOrientation;
    LONG lfWeight;
    BYTE lfItalic;
    BYTE lfUnderline;
    BYTE lfStrikeOut;
    BYTE lfCharSet;
    BYTE lfOutPrecision;
    BYTE lfClipPrecision;
    BYTE lfQuality;
    BYTE lfPitchAndFamily;
    WCHAR lfFaceName[LF_FACESIZE];
} LOGFONTW, *LPLOGFONTW;

typedef struct tagTEXTMETRICA {
    LONG tmHeight;
    LONG tmAscent;
    LONG tmDescent;
    LONG tmInternalLeading;
    LONG tmExternalLeading;
    LONG tmAveCharWidth;
    LONG tmMaxCharWidth;
    LONG tmWeight;
    BYTE tmItalic;
    BYTE tmUnderlined;
    BYTE tmStruckOut;
    BYTE tmFirstChar;
    BYTE tmLastChar;
    BYTE tmDefaultChar;
    BYTE tmBreakChar;
    BYTE tmPitchAndFamily;
    BYTE tmCharSet;
    LONG tmOverhang;
    LONG tmDigitizedAspectX;
    LONG tmDigitizedAspectY;
} TEXTMETRICA, *LPTEXTMETRICA;
typedef TEXTMETRICA TEXTMETRIC;
typedef LPTEXTMETRICA LPTEXTMETRIC;

typedef TEXTMETRICA TEXTMETRICW, *LPTEXTMETRICW;
typedef LOGFONTA LOGFONT;
typedef LPLOGFONTA LPLOGFONT;

typedef int(CALLBACK* FONTENUMPROCA)(const LOGFONTA*, const TEXTMETRICA*,
                                     DWORD, LPARAM);

#ifndef MAX_PATH
#define MAX_PATH 260
#endif

typedef struct _FILETIME {
    DWORD dwLowDateTime;
    DWORD dwHighDateTime;
} FILETIME;

typedef union _ULARGE_INTEGER {
    struct {
        DWORD LowPart;
        DWORD HighPart;
    };
    ULONGLONG QuadPart;
} ULARGE_INTEGER;

typedef struct _SYSTEMTIME {
    WORD wYear;
    WORD wMonth;
    WORD wDayOfWeek;
    WORD wDay;
    WORD wHour;
    WORD wMinute;
    WORD wSecond;
    WORD wMilliseconds;
} SYSTEMTIME;

typedef struct _EXCEPTION_RECORD {
    DWORD ExceptionCode;
    DWORD ExceptionFlags;
    struct _EXCEPTION_RECORD* ExceptionRecord;
    PVOID ExceptionAddress;
    DWORD NumberParameters;
    ULONG_PTR ExceptionInformation[15];
} EXCEPTION_RECORD, *PEXCEPTION_RECORD;

typedef struct _CONTEXT {
    DWORD64 Rax;
    DWORD64 Rbx;
    DWORD64 Rcx;
    DWORD64 Rdx;
    DWORD64 Rsi;
    DWORD64 Rdi;
    DWORD64 Rbp;
    DWORD64 Rsp;
    DWORD64 R8;
    DWORD64 R9;
    DWORD64 R10;
    DWORD64 R11;
    DWORD64 Rip;
} CONTEXT, *PCONTEXT;

typedef struct _EXCEPTION_POINTERS {
    PEXCEPTION_RECORD ExceptionRecord;
    PCONTEXT ContextRecord;
} EXCEPTION_POINTERS, *PEXCEPTION_POINTERS;

typedef LONG(CALLBACK* PTOP_LEVEL_EXCEPTION_FILTER)(PEXCEPTION_POINTERS);
typedef LONG(CALLBACK* PVECTORED_EXCEPTION_HANDLER)(PEXCEPTION_POINTERS);

typedef struct _RUNTIME_FUNCTION {
    DWORD BeginAddress;
    DWORD EndAddress;
    DWORD UnwindData;
} RUNTIME_FUNCTION, *PRUNTIME_FUNCTION;

typedef struct _IMAGE_DOS_HEADER {
    LONG e_lfanew;
} IMAGE_DOS_HEADER;

typedef struct _IMAGE_OPTIONAL_HEADER64 {
    DWORD SizeOfImage;
} IMAGE_OPTIONAL_HEADER64;

typedef struct _IMAGE_NT_HEADERS64 {
    DWORD Signature;
    IMAGE_OPTIONAL_HEADER64 OptionalHeader;
} IMAGE_NT_HEADERS64;

typedef struct tagEVENTMSG {
    UINT message;
    UINT paramL;
    UINT paramH;
    DWORD time;
    HWND hwnd;
} EVENTMSG;

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
#ifndef MAKELPARAM
#define MAKELPARAM(low, high) ((LPARAM)MAKELONG((low), (high)))
#endif
#ifndef MAKEWPARAM
#define MAKEWPARAM(low, high) ((WPARAM)MAKELONG((low), (high)))
#endif
#ifndef _countof
#define _countof(array) (sizeof(array) / sizeof((array)[0]))
#endif

#ifndef MB_OK
#define MB_OK 0x0000
#endif
#ifndef MB_OKCANCEL
#define MB_OKCANCEL 0x0001
#endif
#ifndef MB_ABORTRETRYIGNORE
#define MB_ABORTRETRYIGNORE 0x0002
#endif
#ifndef MB_YESNOCANCEL
#define MB_YESNOCANCEL 0x0003
#endif
#ifndef MB_YESNO
#define MB_YESNO 0x0004
#endif
#ifndef MB_RETRYCANCEL
#define MB_RETRYCANCEL 0x0005
#endif
#ifndef MB_ICONHAND
#define MB_ICONHAND 0x0010
#endif
#ifndef MB_ICONQUESTION
#define MB_ICONQUESTION 0x0020
#endif
#ifndef MB_ICONEXCLAMATION
#define MB_ICONEXCLAMATION 0x0030
#endif
#ifndef MB_ICONASTERISK
#define MB_ICONASTERISK 0x0040
#endif
#ifndef MB_DEFBUTTON2
#define MB_DEFBUTTON2 0x0100
#endif
#ifndef MB_DEFBUTTON1
#define MB_DEFBUTTON1 0x0000
#endif
#ifndef MB_DEFBUTTON3
#define MB_DEFBUTTON3 0x0200
#endif
#ifndef MB_APPLMODAL
#define MB_APPLMODAL 0x0000
#endif
#ifndef MB_SYSTEMMODAL
#define MB_SYSTEMMODAL 0x1000
#endif
#ifndef MB_TYPEMASK
#define MB_TYPEMASK 0x000f
#endif
#ifndef MB_ICONMASK
#define MB_ICONMASK 0x00f0
#endif
#ifndef MB_DEFMASK
#define MB_DEFMASK 0x0f00
#endif
#ifndef MB_MODEMASK
#define MB_MODEMASK 0x3000
#endif

#ifndef IDOK
#define IDOK 1
#endif
#ifndef IDCANCEL
#define IDCANCEL 2
#endif
#ifndef IDABORT
#define IDABORT 3
#endif
#ifndef IDRETRY
#define IDRETRY 4
#endif
#ifndef IDIGNORE
#define IDIGNORE 5
#endif
#ifndef IDYES
#define IDYES 6
#endif
#ifndef IDNO
#define IDNO 7
#endif

#ifndef WM_SETFONT
#define WM_SETFONT 0x0030
#endif
#ifndef WM_APP
#define WM_APP 0x8000
#endif
#ifndef WM_CLOSE
#define WM_CLOSE 0x0010
#endif
#ifndef WM_NCCREATE
#define WM_NCCREATE 0x0081
#endif
#ifndef WM_NCDESTROY
#define WM_NCDESTROY 0x0082
#endif
#ifndef WM_NCCALCSIZE
#define WM_NCCALCSIZE 0x0083
#endif
#ifndef WM_COMMAND
#define WM_COMMAND 0x0111
#endif
#ifndef WM_SYSCOMMAND
#define WM_SYSCOMMAND 0x0112
#endif
#ifndef WM_KEYDOWN
#define WM_KEYDOWN 0x0100
#endif
#ifndef WM_KEYUP
#define WM_KEYUP 0x0101
#endif
#ifndef WM_SYSKEYUP
#define WM_SYSKEYUP 0x0105
#endif
#ifndef WM_MOUSEMOVE
#define WM_MOUSEMOVE 0x0200
#endif
#ifndef WM_LBUTTONDOWN
#define WM_LBUTTONDOWN 0x0201
#endif
#ifndef WM_LBUTTONUP
#define WM_LBUTTONUP 0x0202
#endif

#ifndef BN_CLICKED
#define BN_CLICKED 0
#endif
#ifndef CBN_SELCHANGE
#define CBN_SELCHANGE 1
#endif
#ifndef CBN_DROPDOWN
#define CBN_DROPDOWN 7
#endif
#ifndef CBN_SETFOCUS
#define CBN_SETFOCUS 3
#endif
#ifndef CBN_KILLFOCUS
#define CBN_KILLFOCUS 4
#endif
#ifndef CBN_EDITCHANGE
#define CBN_EDITCHANGE 5
#endif
#ifndef CBN_SELENDOK
#define CBN_SELENDOK 9
#endif
#ifndef EN_CHANGE
#define EN_CHANGE 0x0300
#endif
#ifndef LBN_SELCHANGE
#define LBN_SELCHANGE 1
#endif
#ifndef LBN_DBLCLK
#define LBN_DBLCLK 2
#endif

#ifndef CS_DBLCLKS
#define CS_DBLCLKS 0x0008
#endif

#ifndef MAKEINTRESOURCEA
#define MAKEINTRESOURCEA(id) ((LPSTR)(ULONG_PTR)((WORD)(id)))
#endif
#ifndef MAKEINTRESOURCEW
#define MAKEINTRESOURCEW(id) ((LPWSTR)(ULONG_PTR)((WORD)(id)))
#endif
#ifndef MAKEINTRESOURCE
#define MAKEINTRESOURCE MAKEINTRESOURCEA
#endif
#ifndef IDC_ARROW
#define IDC_ARROW MAKEINTRESOURCEA(32512)
#endif

#ifndef VK_LBUTTON
#define VK_LBUTTON 0x01
#endif
#ifndef VK_RBUTTON
#define VK_RBUTTON 0x02
#endif
#ifndef VK_CANCEL
#define VK_CANCEL 0x03
#endif
#ifndef VK_BACK
#define VK_BACK 0x08
#endif
#ifndef VK_TAB
#define VK_TAB 0x09
#endif
#ifndef VK_CLEAR
#define VK_CLEAR 0x0c
#endif
#ifndef VK_RETURN
#define VK_RETURN 0x0d
#endif
#ifndef MK_SHIFT
#define MK_SHIFT 0x0004
#endif
#ifndef MK_LBUTTON
#define MK_LBUTTON 0x0001
#endif
#ifndef WM_MOVE
#define WM_MOVE 0x0003
#endif
#ifndef WM_SHOWWINDOW
#define WM_SHOWWINDOW 0x0018
#endif
#ifndef WM_QUIT
#define WM_QUIT 0x0012
#endif
#ifndef WM_QUERYENDSESSION
#define WM_QUERYENDSESSION 0x0011
#endif
#ifndef WM_SYSCOLORCHANGE
#define WM_SYSCOLORCHANGE 0x0015
#endif
#ifndef WM_ENDSESSION
#define WM_ENDSESSION 0x0016
#endif
#ifndef WM_DEVMODECHANGE
#define WM_DEVMODECHANGE 0x001b
#endif
#ifndef WM_FONTCHANGE
#define WM_FONTCHANGE 0x001d
#endif
#ifndef WM_NCLBUTTONDOWN
#define WM_NCLBUTTONDOWN 0x00a1
#endif
#ifndef WM_NCRBUTTONDOWN
#define WM_NCRBUTTONDOWN 0x00a4
#endif
#ifndef WM_NCMBUTTONDOWN
#define WM_NCMBUTTONDOWN 0x00a7
#endif
#ifndef WM_NCLBUTTONUP
#define WM_NCLBUTTONUP 0x00a2
#endif
#ifndef WM_NCRBUTTONUP
#define WM_NCRBUTTONUP 0x00a5
#endif
#ifndef WM_NCMBUTTONUP
#define WM_NCMBUTTONUP 0x00a8
#endif
#ifndef WM_NCLBUTTONDBLCLK
#define WM_NCLBUTTONDBLCLK 0x00a3
#endif
#ifndef WM_NCMBUTTONDBLCLK
#define WM_NCMBUTTONDBLCLK 0x00a9
#endif
#ifndef WM_MOUSEACTIVATE
#define WM_MOUSEACTIVATE 0x0021
#endif
#ifndef WM_PAINTICON
#define WM_PAINTICON 0x0026
#endif
#ifndef WM_KEYFIRST
#define WM_KEYFIRST 0x0100
#endif
#ifndef WM_KEYLAST
#define WM_KEYLAST 0x0108
#endif
#ifndef WM_HSCROLL
#define WM_HSCROLL 0x0114
#endif
#ifndef WM_SYSCHAR
#define WM_SYSCHAR 0x0106
#endif
#ifndef WM_SYSDEADCHAR
#define WM_SYSDEADCHAR 0x0107
#endif
#ifndef WM_RBUTTONDOWN
#define WM_RBUTTONDOWN 0x0204
#endif
#ifndef WM_MBUTTONDOWN
#define WM_MBUTTONDOWN 0x0207
#endif
#ifndef WM_RBUTTONUP
#define WM_RBUTTONUP 0x0205
#endif
#ifndef WM_MBUTTONUP
#define WM_MBUTTONUP 0x0208
#endif
#ifndef WM_RENDERFORMAT
#define WM_RENDERFORMAT 0x0305
#endif
#ifndef WM_DESTROYCLIPBOARD
#define WM_DESTROYCLIPBOARD 0x0307
#endif
#ifndef WM_PAINTCLIPBOARD
#define WM_PAINTCLIPBOARD 0x0309
#endif
#ifndef WM_VSCROLLCLIPBOARD
#define WM_VSCROLLCLIPBOARD 0x030a
#endif
#ifndef WM_SIZECLIPBOARD
#define WM_SIZECLIPBOARD 0x030b
#endif
#ifndef WM_ASKCBFORMATNAME
#define WM_ASKCBFORMATNAME 0x030c
#endif
#ifndef WM_HSCROLLCLIPBOARD
#define WM_HSCROLLCLIPBOARD 0x030e
#endif
#ifndef VK_SHIFT
#define VK_SHIFT 0x10
#endif
#ifndef VK_CONTROL
#define VK_CONTROL 0x11
#endif
#ifndef VK_MENU
#define VK_MENU 0x12
#endif
#ifndef VK_PAUSE
#define VK_PAUSE 0x13
#endif
#ifndef VK_CAPITAL
#define VK_CAPITAL 0x14
#endif
#ifndef VK_ESCAPE
#define VK_ESCAPE 0x1b
#endif
#ifndef VK_SPACE
#define VK_SPACE 0x20
#endif
#ifndef VK_PRIOR
#define VK_PRIOR 0x21
#endif
#ifndef VK_NEXT
#define VK_NEXT 0x22
#endif
#ifndef VK_END
#define VK_END 0x23
#endif
#ifndef VK_HOME
#define VK_HOME 0x24
#endif
#ifndef VK_LEFT
#define VK_LEFT 0x25
#endif
#ifndef VK_UP
#define VK_UP 0x26
#endif
#ifndef VK_RIGHT
#define VK_RIGHT 0x27
#endif
#ifndef VK_DOWN
#define VK_DOWN 0x28
#endif
#ifndef VK_PRINT
#define VK_PRINT 0x2a
#endif
#ifndef VK_INSERT
#define VK_INSERT 0x2d
#endif
#ifndef VK_DELETE
#define VK_DELETE 0x2e
#endif
#ifndef VK_HELP
#define VK_HELP 0x2f
#endif
#ifndef VK_NUMPAD0
#define VK_NUMPAD0 0x60
#endif
#ifndef VK_NUMPAD1
#define VK_NUMPAD1 0x61
#endif
#ifndef VK_NUMPAD2
#define VK_NUMPAD2 0x62
#endif
#ifndef VK_NUMPAD3
#define VK_NUMPAD3 0x63
#endif
#ifndef VK_NUMPAD4
#define VK_NUMPAD4 0x64
#endif
#ifndef VK_NUMPAD5
#define VK_NUMPAD5 0x65
#endif
#ifndef VK_NUMPAD6
#define VK_NUMPAD6 0x66
#endif
#ifndef VK_NUMPAD7
#define VK_NUMPAD7 0x67
#endif
#ifndef VK_NUMPAD8
#define VK_NUMPAD8 0x68
#endif
#ifndef VK_NUMPAD9
#define VK_NUMPAD9 0x69
#endif
#ifndef VK_MULTIPLY
#define VK_MULTIPLY 0x6a
#endif
#ifndef VK_ADD
#define VK_ADD 0x6b
#endif
#ifndef VK_SUBTRACT
#define VK_SUBTRACT 0x6d
#endif
#ifndef VK_DECIMAL
#define VK_DECIMAL 0x6e
#endif
#ifndef VK_DIVIDE
#define VK_DIVIDE 0x6f
#endif
#ifndef VK_F1
#define VK_F1 0x70
#endif
#ifndef VK_F2
#define VK_F2 0x71
#endif
#ifndef VK_F3
#define VK_F3 0x72
#endif
#ifndef VK_F4
#define VK_F4 0x73
#endif
#ifndef VK_F5
#define VK_F5 0x74
#endif
#ifndef VK_F6
#define VK_F6 0x75
#endif
#ifndef VK_F7
#define VK_F7 0x76
#endif
#ifndef VK_F8
#define VK_F8 0x77
#endif
#ifndef VK_F9
#define VK_F9 0x78
#endif
#ifndef VK_F10
#define VK_F10 0x79
#endif
#ifndef VK_F11
#define VK_F11 0x7a
#endif
#ifndef VK_F12
#define VK_F12 0x7b
#endif
#ifndef VK_F13
#define VK_F13 0x7c
#endif
#ifndef VK_F14
#define VK_F14 0x7d
#endif
#ifndef VK_F15
#define VK_F15 0x7e
#endif
#ifndef VK_F16
#define VK_F16 0x7f
#endif
#ifndef VK_NUMLOCK
#define VK_NUMLOCK 0x90
#endif

#ifndef COLOR_BTNFACE
#define COLOR_BTNFACE 15
#endif
#ifndef COLOR_WINDOW
#define COLOR_WINDOW 5
#endif
#ifndef COLOR_WINDOWFRAME
#define COLOR_WINDOWFRAME 6
#endif
#ifndef COLOR_WINDOWTEXT
#define COLOR_WINDOWTEXT 8
#endif
#ifndef COLOR_BTNTEXT
#define COLOR_BTNTEXT 18
#endif
#ifndef COLOR_MENU
#define COLOR_MENU 4
#endif
#ifndef COLOR_APPWORKSPACE
#define COLOR_APPWORKSPACE 12
#endif
#ifndef COLOR_HIGHLIGHT
#define COLOR_HIGHLIGHT 13
#endif
#ifndef COLOR_HIGHLIGHTTEXT
#define COLOR_HIGHLIGHTTEXT 14
#endif

#ifndef CS_VREDRAW
#define CS_VREDRAW 0x0001
#endif
#ifndef CS_HREDRAW
#define CS_HREDRAW 0x0002
#endif
#ifndef CS_OWNDC
#define CS_OWNDC 0x0020
#endif
#ifndef CS_BYTEALIGNCLIENT
#define CS_BYTEALIGNCLIENT 0x1000
#endif

#ifndef WS_POPUP
#define WS_POPUP 0x80000000u
#endif
#ifndef WS_OVERLAPPED
#define WS_OVERLAPPED 0x00000000u
#endif
#ifndef WS_CHILD
#define WS_CHILD 0x40000000u
#endif
#ifndef WS_CHILDWINDOW
#define WS_CHILDWINDOW WS_CHILD
#endif
#ifndef WS_MAXIMIZE
#define WS_MAXIMIZE 0x01000000u
#endif
#ifndef WS_MINIMIZE
#define WS_MINIMIZE 0x20000000u
#endif
#ifndef WS_VISIBLE
#define WS_VISIBLE 0x10000000u
#endif
#ifndef WS_CLIPSIBLINGS
#define WS_CLIPSIBLINGS 0x04000000u
#endif
#ifndef WS_CLIPCHILDREN
#define WS_CLIPCHILDREN 0x02000000u
#endif
#ifndef WS_CAPTION
#define WS_CAPTION 0x00c00000u
#endif
#ifndef WS_SYSMENU
#define WS_SYSMENU 0x00080000u
#endif
#ifndef WS_GROUP
#define WS_GROUP 0x00020000u
#endif
#ifndef WS_TABSTOP
#define WS_TABSTOP 0x00010000u
#endif
#ifndef WS_VSCROLL
#define WS_VSCROLL 0x00200000u
#endif
#ifndef WS_BORDER
#define WS_BORDER 0x00800000u
#endif
#ifndef WS_DLGFRAME
#define WS_DLGFRAME 0x00400000u
#endif
#ifndef WS_THICKFRAME
#define WS_THICKFRAME 0x00040000u
#endif
#ifndef WS_TILED
#define WS_TILED WS_OVERLAPPED
#endif
#ifndef WS_MINIMIZEBOX
#define WS_MINIMIZEBOX 0x00020000u
#endif
#ifndef WS_MAXIMIZEBOX
#define WS_MAXIMIZEBOX 0x00010000u
#endif
#ifndef WS_TILEDWINDOW
#define WS_TILEDWINDOW (WS_TILED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX)
#endif

#ifndef WS_EX_DLGMODALFRAME
#define WS_EX_DLGMODALFRAME 0x00000001
#endif
#ifndef WS_EX_CONTROLPARENT
#define WS_EX_CONTROLPARENT 0x00010000
#endif
#ifndef WS_EX_NOACTIVATE
#define WS_EX_NOACTIVATE 0x08000000
#endif

#ifndef GWL_STYLE
#define GWL_STYLE (-16)
#endif
#ifndef GWL_EXSTYLE
#define GWL_EXSTYLE (-20)
#endif
#ifndef GWLP_USERDATA
#define GWLP_USERDATA (-21)
#endif
#ifndef GWLP_WNDPROC
#define GWLP_WNDPROC (-4)
#endif
#ifndef GWLP_HINSTANCE
#define GWLP_HINSTANCE (-6)
#endif
#ifndef GA_ROOT
#define GA_ROOT 2
#endif

#ifndef SW_HIDE
#define SW_HIDE 0
#endif
#ifndef SW_SHOWMINNOACTIVE
#define SW_SHOWMINNOACTIVE 7
#endif
#ifndef SW_SHOWMINIMIZED
#define SW_SHOWMINIMIZED 2
#endif
#ifndef SW_SHOWMAXIMIZED
#define SW_SHOWMAXIMIZED 3
#endif
#ifndef SW_SHOWNORMAL
#define SW_SHOWNORMAL 1
#endif
#ifndef SW_SHOW
#define SW_SHOW 5
#endif
#ifndef SW_SHOWNOACTIVATE
#define SW_SHOWNOACTIVATE 4
#endif
#ifndef SHOW_OPENWINDOW
#define SHOW_OPENWINDOW 1
#endif
#ifndef SHOW_OPENNOACTIVATE
#define SHOW_OPENNOACTIVATE 4
#endif
#ifndef SW_SHOWNA
#define SW_SHOWNA 8
#endif

#ifndef WM_VSCROLL
#define WM_VSCROLL 0x0115
#endif
#ifndef WM_NCPAINT
#define WM_NCPAINT 0x0085
#endif
#ifndef WM_NCMOUSEMOVE
#define WM_NCMOUSEMOVE 0x00a0
#endif
#ifndef WM_NCHITTEST
#define WM_NCHITTEST 0x0084
#endif
#ifndef WM_CANCELMODE
#define WM_CANCELMODE 0x001f
#endif
#ifndef WM_CAPTURECHANGED
#define WM_CAPTURECHANGED 0x0215
#endif
#ifndef WM_UNICHAR
#define WM_UNICHAR 0x0109
#endif
#ifndef WM_IME_COMPOSITION
#define WM_IME_COMPOSITION 0x010f
#endif
#ifndef WM_MOUSEWHEEL
#define WM_MOUSEWHEEL 0x020a
#endif
#ifndef WM_CONTEXTMENU
#define WM_CONTEXTMENU 0x007b
#endif
#ifndef WM_TIMER
#define WM_TIMER 0x0113
#endif

#ifndef WH_JOURNALPLAYBACK
#define WH_JOURNALPLAYBACK 1
#endif
#ifndef HC_GETNEXT
#define HC_GETNEXT 1
#endif
#ifndef HC_SKIP
#define HC_SKIP 2
#endif
#ifndef WH_MSGFILTER
#define WH_MSGFILTER (-1)
#endif

#ifndef RDW_INVALIDATE
#define RDW_INVALIDATE 0x0001
#endif
#ifndef RDW_ERASE
#define RDW_ERASE 0x0004
#endif
#ifndef RDW_ALLCHILDREN
#define RDW_ALLCHILDREN 0x0080
#endif
#ifndef RDW_UPDATENOW
#define RDW_UPDATENOW 0x0100
#endif
#ifndef RDW_FRAME
#define RDW_FRAME 0x0400
#endif

#ifndef DT_CENTER
#define DT_CENTER 0x0001
#endif
#ifndef DT_VCENTER
#define DT_VCENTER 0x0004
#endif
#ifndef DT_SINGLELINE
#define DT_SINGLELINE 0x0020
#endif

#ifndef SBS_SIZEBOX
#define SBS_SIZEBOX 0x0008L
#endif

#ifndef PATINVERT
#define PATINVERT ((DWORD)0x005A0049)
#endif
#ifndef PATCOPY
#define PATCOPY ((DWORD)0x00F00021)
#endif
#ifndef BLACKNESS
#define BLACKNESS ((DWORD)0x00000042)
#endif
#ifndef SRCCOPY
#define SRCCOPY ((DWORD)0x00CC0020)
#endif
#ifndef DSTINVERT
#define DSTINVERT ((DWORD)0x00550009)
#endif
#ifndef WHITENESS
#define WHITENESS ((DWORD)0x00FF0062)
#endif
#ifndef ROP_PSo
#define ROP_PSo ((DWORD)0x00FC008A)
#endif
#ifndef ROP_PSno
#define ROP_PSno ((DWORD)0x00F3022A)
#endif
#ifndef ROP_DPSxx
#define ROP_DPSxx ((DWORD)0x00960169)
#endif
#ifndef ROP_PDSxxn
#define ROP_PDSxxn ((DWORD)0x00690145)
#endif
#ifndef ROP_Pn
#define ROP_Pn ((DWORD)0x000F0001)
#endif
#ifndef ROP_DPSnao
#define ROP_DPSnao ((DWORD)0x00B00E05)
#endif
#ifndef ROP_DSnx
#define ROP_DSnx ((DWORD)0x00990066)
#endif
#ifndef ROP_DPo
#define ROP_DPo ((DWORD)0x00FA0089)
#endif

#ifndef VERTRES
#define VERTRES 10
#endif
#ifndef HORZSIZE
#define HORZSIZE 4
#endif
#ifndef VERTSIZE
#define VERTSIZE 6
#endif
#ifndef HORZRES
#define HORZRES 8
#endif
#ifndef NUMCOLORS
#define NUMCOLORS 24
#endif
#ifndef LOGPIXELSX
#define LOGPIXELSX 88
#endif
#ifndef LOGPIXELSY
#define LOGPIXELSY 90
#endif

#ifndef SM_CYHSCROLL
#define SM_CYHSCROLL 3
#endif
#ifndef SM_CXSCREEN
#define SM_CXSCREEN 0
#endif
#ifndef SM_CYSCREEN
#define SM_CYSCREEN 1
#endif
#ifndef SM_CYMENU
#define SM_CYMENU 15
#endif
#ifndef SM_CXFULLSCREEN
#define SM_CXFULLSCREEN 16
#endif
#ifndef SM_MOUSEPRESENT
#define SM_MOUSEPRESENT 19
#endif
#ifndef SM_CYVSCROLL
#define SM_CYVSCROLL 20
#endif
#ifndef SM_CYFRAME
#define SM_CYFRAME 33
#endif
#ifndef SM_CXFRAME
#define SM_CXFRAME 32
#endif
#ifndef SM_CXSMICON
#define SM_CXSMICON 49
#endif

#ifndef CW_USEDEFAULT
#define CW_USEDEFAULT 0x8000
#endif
#ifndef IMAGE_ICON
#define IMAGE_ICON 1
#endif
#ifndef IMAGE_BITMAP
#define IMAGE_BITMAP 0
#endif
#ifndef LR_DEFAULTCOLOR
#define LR_DEFAULTCOLOR 0
#endif
#ifndef LR_CREATEDIBSECTION
#define LR_CREATEDIBSECTION 0x00002000
#endif
#ifndef DI_NORMAL
#define DI_NORMAL 0x0003
#endif

#ifndef ETO_OPAQUE
#define ETO_OPAQUE 2
#endif
#ifndef ETO_CLIPPED
#define ETO_CLIPPED 4
#endif
#ifndef ETO_GRAYED
#define ETO_GRAYED 1
#endif

#ifndef FIXED_PITCH
#define FIXED_PITCH 1
#endif
#ifndef VARIABLE_PITCH
#define VARIABLE_PITCH 2
#endif
#ifndef DEFAULT_PITCH
#define DEFAULT_PITCH 0
#endif
#ifndef RASTER_FONTTYPE
#define RASTER_FONTTYPE 0x0001
#endif
#ifndef DEVICE_FONTTYPE
#define DEVICE_FONTTYPE 0x0002
#endif
#ifndef TRUETYPE_FONTTYPE
#define TRUETYPE_FONTTYPE 0x0004
#endif
#ifndef TMPF_FIXED_PITCH
#define TMPF_FIXED_PITCH 0x01
#endif
#ifndef TMPF_VECTOR
#define TMPF_VECTOR 0x02
#endif
#ifndef TMPF_TRUETYPE
#define TMPF_TRUETYPE 0x04
#endif
#ifndef FF_ROMAN
#define FF_ROMAN (1 << 4)
#endif
#ifndef FF_DONTCARE
#define FF_DONTCARE (0 << 4)
#endif
#ifndef FF_SWISS
#define FF_SWISS (2 << 4)
#endif
#ifndef FF_MODERN
#define FF_MODERN (3 << 4)
#endif
#ifndef FF_DECORATIVE
#define FF_DECORATIVE (5 << 4)
#endif
#ifndef FF_SCRIPT
#define FF_SCRIPT (4 << 4)
#endif
#ifndef FW_NORMAL
#define FW_NORMAL 400
#endif
#ifndef FW_BOLD
#define FW_BOLD 700
#endif

#ifndef LTGRAY_BRUSH
#define LTGRAY_BRUSH 1
#endif
#ifndef GRAY_BRUSH
#define GRAY_BRUSH 2
#endif
#ifndef DKGRAY_BRUSH
#define DKGRAY_BRUSH 3
#endif
#ifndef BLACK_BRUSH
#define BLACK_BRUSH 4
#endif
#ifndef NULL_BRUSH
#define NULL_BRUSH 5
#endif
#ifndef HOLLOW_BRUSH
#define HOLLOW_BRUSH NULL_BRUSH
#endif
#ifndef WHITE_PEN
#define WHITE_PEN 6
#endif
#ifndef NULL_PEN
#define NULL_PEN 8
#endif
#ifndef ANSI_FIXED_FONT
#define ANSI_FIXED_FONT 11
#endif
#ifndef ANSI_VAR_FONT
#define ANSI_VAR_FONT 12
#endif

#ifndef IDC_IBEAM
#define IDC_IBEAM MAKEINTRESOURCE(32513)
#endif
#ifndef IDC_WAIT
#define IDC_WAIT MAKEINTRESOURCE(32514)
#endif

#ifndef MM_ANISOTROPIC
#define MM_ANISOTROPIC 8
#endif
#ifndef MM_TEXT
#define MM_TEXT 1
#endif
#ifndef MM_ISOTROPIC
#define MM_ISOTROPIC 7
#endif
#ifndef MM_LOMETRIC
#define MM_LOMETRIC 2
#endif
#ifndef TRANSPARENT
#define TRANSPARENT 1
#endif
#ifndef OPAQUE
#define OPAQUE 2
#endif
#ifndef DRAWPATTERNRECT
#define DRAWPATTERNRECT 25
#endif
#ifndef GETPHYSPAGESIZE
#define GETPHYSPAGESIZE 12
#endif
#ifndef GETPRINTINGOFFSET
#define GETPRINTINGOFFSET 13
#endif
#ifndef GETSCALINGFACTOR
#define GETSCALINGFACTOR 14
#endif
#ifndef PASSTHROUGH
#define PASSTHROUGH 19
#endif
#ifndef NEXTBAND
#define NEXTBAND 3
#endif
#ifndef SETABORTPROC
#define SETABORTPROC 9
#endif
#ifndef STARTDOC
#define STARTDOC 10
#endif
#ifndef ENDDOC
#define ENDDOC 11
#endif
#ifndef DRAFTMODE
#define DRAFTMODE 7
#endif
#ifndef ABORTDOC
#define ABORTDOC 2
#endif
#ifndef QUERYESCSUPPORT
#define QUERYESCSUPPORT 8
#endif
#ifndef BANDINFO
#define BANDINFO 24
#endif
#ifndef SRCAND
#define SRCAND ((DWORD)0x008800C6)
#endif
#ifndef SRCINVERT
#define SRCINVERT ((DWORD)0x00660046)
#endif
#ifndef BLACKONWHITE
#define BLACKONWHITE 1
#endif
#ifndef WHITEONBLACK
#define WHITEONBLACK 2
#endif
#ifndef COLORONCOLOR
#define COLORONCOLOR 3
#endif
#ifndef BITSPIXEL
#define BITSPIXEL 12
#endif
#ifndef PLANES
#define PLANES 14
#endif
#ifndef TEXTCAPS
#define TEXTCAPS 34
#endif
#ifndef RASTERCAPS
#define RASTERCAPS 38
#endif
#ifndef RC_BITBLT
#define RC_BITBLT 1
#endif
#ifndef RC_SCALING
#define RC_SCALING 4
#endif
#ifndef TC_SA_DOUBLE
#define TC_SA_DOUBLE 0x0040
#endif
#ifndef TC_SA_INTEGER
#define TC_SA_INTEGER 0x0080
#endif
#ifndef TC_SA_CONTIN
#define TC_SA_CONTIN 0x0100
#endif
#ifndef TC_RA_ABLE
#define TC_RA_ABLE 0x2000
#endif
#ifndef TC_VA_ABLE
#define TC_VA_ABLE 0x4000
#endif
#ifndef MM_HIMETRIC
#define MM_HIMETRIC 3
#endif
#ifndef MM_HIENGLISH
#define MM_HIENGLISH 5
#endif
#ifndef MM_LOENGLISH
#define MM_LOENGLISH 4
#endif
#ifndef MM_TWIPS
#define MM_TWIPS 6
#endif
#ifndef SP_NOTREPORTED
#define SP_NOTREPORTED 0x4000
#endif
#ifndef SP_ERROR
#define SP_ERROR (-1)
#endif
#ifndef SP_APPABORT
#define SP_APPABORT (-2)
#endif
#ifndef SP_USERABORT
#define SP_USERABORT (-3)
#endif
#ifndef SP_OUTOFDISK
#define SP_OUTOFDISK (-4)
#endif
#ifndef SP_OUTOFMEMORY
#define SP_OUTOFMEMORY (-5)
#endif
#ifndef ANSI_CHARSET
#define ANSI_CHARSET 0
#endif
#ifndef SYMBOL_CHARSET
#define SYMBOL_CHARSET 2
#endif
#ifndef BI_RGB
#define BI_RGB 0L
#endif
#ifndef DIB_RGB_COLORS
#define DIB_RGB_COLORS 0
#endif
#ifndef CBM_INIT
#define CBM_INIT 0x04
#endif
#ifndef RGB
#define RGB(r, g, b) \
    ((DWORD)(((BYTE)(r) | ((WORD)(g) << 8)) | (((DWORD)(BYTE)(b)) << 16)))
#endif
#ifndef GetRValue
#define GetRValue(rgb) ((BYTE)(rgb))
#endif
#ifndef GetGValue
#define GetGValue(rgb) ((BYTE)(((WORD)(rgb)) >> 8))
#endif
#ifndef GetBValue
#define GetBValue(rgb) ((BYTE)((rgb) >> 16))
#endif
#ifndef CLR_INVALID
#define CLR_INVALID ((COLORREF)-1)
#endif
#ifndef ASPECT_FILTERING
#define ASPECT_FILTERING 0x00000001
#endif
#ifndef BF_LEFT
#define BF_LEFT 0x0001
#endif
#ifndef BF_TOP
#define BF_TOP 0x0002
#endif
#ifndef BF_RIGHT
#define BF_RIGHT 0x0004
#endif
#ifndef BF_BOTTOM
#define BF_BOTTOM 0x0008
#endif
#ifndef BDR_RAISEDINNER
#define BDR_RAISEDINNER 0x0004
#endif
#ifndef BDR_SUNKENOUTER
#define BDR_SUNKENOUTER 0x0002
#endif
#ifndef UNICODE_NOCHAR
#define UNICODE_NOCHAR 0xffff
#endif
#ifndef META_SETBKCOLOR
#define META_SETBKCOLOR 0x0201
#endif
#ifndef META_SETTEXTCOLOR
#define META_SETTEXTCOLOR 0x0209
#endif
#ifndef META_SETWINDOWORG
#define META_SETWINDOWORG 0x020b
#endif
#ifndef META_SETWINDOWEXT
#define META_SETWINDOWEXT 0x020c
#endif
#ifndef META_STRETCHDIB
#define META_STRETCHDIB 0x0f43
#endif
#ifndef NULLREGION
#define NULLREGION 1
#endif
#ifndef ERROR
#define ERROR 0
#endif
#ifndef SIMPLEREGION
#define SIMPLEREGION 2
#endif
#ifndef COMPLEXREGION
#define COMPLEXREGION 3
#endif
#ifndef RGN_AND
#define RGN_AND 1
#endif
#ifndef RGN_OR
#define RGN_OR 2
#endif
#ifndef RGN_XOR
#define RGN_XOR 3
#endif
#ifndef RGN_DIFF
#define RGN_DIFF 4
#endif

#ifndef SWP_NOSIZE
#define SWP_NOSIZE 0x0001
#endif
#ifndef SWP_NOMOVE
#define SWP_NOMOVE 0x0002
#endif
#ifndef SWP_NOZORDER
#define SWP_NOZORDER 0x0004
#endif
#ifndef SWP_NOREDRAW
#define SWP_NOREDRAW 0x0008
#endif
#ifndef SWP_NOACTIVATE
#define SWP_NOACTIVATE 0x0010
#endif
#ifndef SWP_SHOWWINDOW
#define SWP_SHOWWINDOW 0x0040
#endif

#ifndef GW_OWNER
#define GW_OWNER 4
#endif
#ifndef GW_CHILD
#define GW_CHILD 5
#endif
#ifndef GW_HWNDNEXT
#define GW_HWNDNEXT 2
#endif
#ifndef GW_HWNDFIRST
#define GW_HWNDFIRST 0
#endif
#ifndef GW_HWNDPREV
#define GW_HWNDPREV 3
#endif

#ifndef WS_SIZEBOX
#define WS_SIZEBOX 0x00040000L
#endif
#ifndef SBS_VERT
#define SBS_VERT 0x0001L
#endif
#ifndef SBS_HORZ
#define SBS_HORZ 0x0000L
#endif
#ifndef HIDE_WINDOW
#define HIDE_WINDOW 0
#endif
#ifndef PS_SOLID
#define PS_SOLID 0
#endif
#ifndef OEM_CHARSET
#define OEM_CHARSET 255
#endif
#ifndef HTSYSMENU
#define HTSYSMENU 3
#endif
#ifndef HTMENU
#define HTMENU 5
#endif
#ifndef SM_CYVTHUMB
#define SM_CYVTHUMB 9
#endif
#ifndef SM_CXHTHUMB
#define SM_CXHTHUMB 10
#endif
#ifndef SM_CXICON
#define SM_CXICON 11
#endif
#ifndef SM_CYICON
#define SM_CYICON 12
#endif
#ifndef SM_CXCURSOR
#define SM_CXCURSOR 13
#endif
#ifndef SM_CYCURSOR
#define SM_CYCURSOR 14
#endif
#ifndef SM_CXDLGFRAME
#define SM_CXDLGFRAME 7
#endif
#ifndef SM_CYDLGFRAME
#define SM_CYDLGFRAME 8
#endif
#ifndef SM_CURSORLEVEL
#define SM_CURSORLEVEL 25
#endif

#ifndef SC_SIZE
#define SC_SIZE 0xf000
#endif
#ifndef SC_MOVE
#define SC_MOVE 0xf010
#endif
#ifndef SC_MINIMIZE
#define SC_MINIMIZE 0xf020
#endif
#ifndef SC_MAXIMIZE
#define SC_MAXIMIZE 0xf030
#endif
#ifndef SC_NEXTWINDOW
#define SC_NEXTWINDOW 0xf040
#endif
#ifndef SC_PREVWINDOW
#define SC_PREVWINDOW 0xf050
#endif
#ifndef SC_CLOSE
#define SC_CLOSE 0xf060
#endif
#ifndef SC_VSCROLL
#define SC_VSCROLL 0xf070
#endif
#ifndef SC_HSCROLL
#define SC_HSCROLL 0xf080
#endif
#ifndef SC_MOUSEMENU
#define SC_MOUSEMENU 0xf090
#endif
#ifndef SC_KEYMENU
#define SC_KEYMENU 0xf100
#endif
#ifndef SC_ARRANGE
#define SC_ARRANGE 0xf110
#endif
#ifndef SC_RESTORE
#define SC_RESTORE 0xf120
#endif
#ifndef SC_ZOOM
#define SC_ZOOM SC_MAXIMIZE
#endif
#ifndef HTSIZEFIRST
#define HTSIZEFIRST 10
#endif
#ifndef HTREDUCE
#define HTREDUCE 8
#endif
#ifndef HTCLIENT
#define HTCLIENT 1
#endif
#ifndef HTTRANSPARENT
#define HTTRANSPARENT (-1)
#endif
#ifndef HTZOOM
#define HTZOOM 9
#endif
#ifndef HTCAPTION
#define HTCAPTION 2
#endif
#ifndef HTSIZE
#define HTSIZE 4
#endif
#ifndef HTHSCROLL
#define HTHSCROLL 6
#endif
#ifndef HTVSCROLL
#define HTVSCROLL 7
#endif
#ifndef HTLEFT
#define HTLEFT 10
#endif
#ifndef HTRIGHT
#define HTRIGHT 11
#endif
#ifndef HTTOP
#define HTTOP 12
#endif
#ifndef HTTOPLEFT
#define HTTOPLEFT 13
#endif
#ifndef HTTOPRIGHT
#define HTTOPRIGHT 14
#endif
#ifndef HTBOTTOM
#define HTBOTTOM 15
#endif
#ifndef HTBOTTOMLEFT
#define HTBOTTOMLEFT 16
#endif
#ifndef HTBOTTOMRIGHT
#define HTBOTTOMRIGHT 17
#endif
#ifndef NOTSRCCOPY
#define NOTSRCCOPY ((DWORD)0x00330008)
#endif
#ifndef R2_NOTXORPEN
#define R2_NOTXORPEN 10
#endif
#ifndef R2_COPYPEN
#define R2_COPYPEN 13
#endif
#ifndef COLOR_SCROLLBAR
#define COLOR_SCROLLBAR 0
#endif
#ifndef COLOR_CAPTIONTEXT
#define COLOR_CAPTIONTEXT 9
#endif
#ifndef COLOR_BTNSHADOW
#define COLOR_BTNSHADOW 16
#endif
#ifndef COLOR_GRAYTEXT
#define COLOR_GRAYTEXT 17
#endif
#ifndef EDGE_RAISED
#define EDGE_RAISED 0x0005
#endif
#ifndef EDGE_SUNKEN
#define EDGE_SUNKEN 0x000a
#endif
#ifndef BF_RECT
#define BF_RECT 0x000f
#endif
#ifndef COLORREF
typedef DWORD COLORREF;
#endif
#ifndef LPCREATESTRUCT
#define LPCREATESTRUCT LPCREATESTRUCTA
#endif
#ifndef OBM_OLD_UPARROW
#define OBM_OLD_UPARROW 32765
#endif
#ifndef OBM_OLD_CLOSE
#define OBM_OLD_CLOSE 32767
#endif
#ifndef OBM_CLOSE
#define OBM_CLOSE 32754
#endif
#ifndef OBM_OLD_DNARROW
#define OBM_OLD_DNARROW 32764
#endif
#ifndef OBM_OLD_LFARROW
#define OBM_OLD_LFARROW 32762
#endif
#ifndef OBM_OLD_RGARROW
#define OBM_OLD_RGARROW 32763
#endif
#ifndef OBM_BTSIZE
#define OBM_BTSIZE 32761
#endif
#ifndef OBM_UPARROW
#define OBM_UPARROW 32753
#endif
#ifndef OBM_DNARROW
#define OBM_DNARROW 32752
#endif
#ifndef OBM_RGARROW
#define OBM_RGARROW 32751
#endif
#ifndef OBM_LFARROW
#define OBM_LFARROW 32750
#endif
#ifndef OBM_UPARROWD
#define OBM_UPARROWD 32743
#endif
#ifndef OBM_DNARROWD
#define OBM_DNARROWD 32742
#endif
#ifndef OBM_RGARROWD
#define OBM_RGARROWD 32741
#endif
#ifndef OBM_LFARROWD
#define OBM_LFARROWD 32740
#endif

#ifndef SB_HORZ
#define SB_HORZ 0
#endif
#ifndef SB_VERT
#define SB_VERT 1
#endif
#ifndef SB_LINEUP
#define SB_LINEUP 0
#endif
#ifndef SB_LINEDOWN
#define SB_LINEDOWN 1
#endif
#ifndef SB_PAGEUP
#define SB_PAGEUP 2
#endif
#ifndef SB_PAGEDOWN
#define SB_PAGEDOWN 3
#endif
#ifndef SB_THUMBPOSITION
#define SB_THUMBPOSITION 4
#endif
#ifndef SB_THUMBTRACK
#define SB_THUMBTRACK 5
#endif

#ifndef SM_CXBORDER
#define SM_CXBORDER 5
#endif
#ifndef SM_CYBORDER
#define SM_CYBORDER 6
#endif
#ifndef SM_CYCAPTION
#define SM_CYCAPTION 4
#endif
#ifndef SM_CXVSCROLL
#define SM_CXVSCROLL 2
#endif

#ifndef SPI_GETWORKAREA
#define SPI_GETWORKAREA 0x0030
#endif
#ifndef PM_NOREMOVE
#define PM_NOREMOVE 0
#endif
#ifndef PM_REMOVE
#define PM_REMOVE 1
#endif
#ifndef PM_NOYIELD
#define PM_NOYIELD 2
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
#ifndef MF_GRAYED
#define MF_GRAYED 0x0001
#endif
#ifndef MF_DISABLED
#define MF_DISABLED 0x0002
#endif
#ifndef MF_ENABLED
#define MF_ENABLED 0x0000
#endif
#ifndef MF_CHECKED
#define MF_CHECKED 0x0008
#endif
#ifndef MF_UNCHECKED
#define MF_UNCHECKED 0x0000
#endif
#ifndef MF_BITMAP
#define MF_BITMAP 0x0004
#endif
#ifndef MF_STRING
#define MF_STRING 0x0000
#endif
#ifndef MF_POPUP
#define MF_POPUP 0x0010
#endif
#ifndef MF_HILITE
#define MF_HILITE 0x0080
#endif
#ifndef MF_UNHILITE
#define MF_UNHILITE 0x0000
#endif
#ifndef MF_SYSMENU
#define MF_SYSMENU 0x2000
#endif
#ifndef MIM_BACKGROUND
#define MIM_BACKGROUND 0x00000002
#endif
#ifndef MIM_APPLYTOSUBMENUS
#define MIM_APPLYTOSUBMENUS 0x80000000
#endif

#ifndef CF_TEXT
#define CF_TEXT 1
#endif
#ifndef CF_BITMAP
#define CF_BITMAP 2
#endif
#ifndef CF_METAFILEPICT
#define CF_METAFILEPICT 3
#endif
#ifndef CF_TIFF
#define CF_TIFF 6
#endif
#ifndef CF_DIB
#define CF_DIB 8
#endif
#ifndef CF_UNICODETEXT
#define CF_UNICODETEXT 13
#endif
#ifndef CF_OWNERDISPLAY
#define CF_OWNERDISPLAY 0x80
#endif

#ifndef OF_REOPEN
#define OF_REOPEN 0x8000
#endif
#ifndef OF_EXIST
#define OF_EXIST 0x4000
#endif
#ifndef OF_PROMPT
#define OF_PROMPT 0x2000
#endif
#ifndef OF_CREATE
#define OF_CREATE 0x1000
#endif
#ifndef OF_CANCEL
#define OF_CANCEL 0x0800
#endif
#ifndef OF_VERIFY
#define OF_VERIFY 0x0400
#endif
#ifndef OF_DELETE
#define OF_DELETE 0x0200
#endif
#ifndef OF_PARSE
#define OF_PARSE 0x0100
#endif
#ifndef OF_READ
#define OF_READ 0
#endif
#ifndef OF_WRITE
#define OF_WRITE 1
#endif
#ifndef OF_READWRITE
#define OF_READWRITE 2
#endif

#ifndef OBJ_PEN
#define OBJ_PEN 1
#endif
#ifndef OBJ_BRUSH
#define OBJ_BRUSH 2
#endif
#ifndef OBJ_DC
#define OBJ_DC 3
#endif
#ifndef OBJ_METADC
#define OBJ_METADC 4
#endif
#ifndef OBJ_BITMAP
#define OBJ_BITMAP 5
#endif
#ifndef OBJ_FONT
#define OBJ_FONT 6
#endif
#ifndef DEFAULT_GUI_FONT
#define DEFAULT_GUI_FONT 17
#endif
#ifndef WHITE_BRUSH
#define WHITE_BRUSH 0
#endif
#ifndef BLACK_PEN
#define BLACK_PEN 7
#endif
#ifndef SYSTEM_FONT
#define SYSTEM_FONT 13
#endif

#ifndef GMEM_FIXED
#define GMEM_FIXED 0x0000
#endif
#ifndef GMEM_MOVEABLE
#define GMEM_MOVEABLE 0x0002
#endif
#ifndef GMEM_ZEROINIT
#define GMEM_ZEROINIT 0x0040
#endif
#ifndef GMEM_SHARE
#define GMEM_SHARE 0x2000
#endif
#ifndef GMEM_DDESHARE
#define GMEM_DDESHARE 0x2000
#endif
#ifndef GMEM_LOWER
#define GMEM_LOWER 0x1000
#endif
#ifndef GMEM_NOT_BANKED
#define GMEM_NOT_BANKED 0x1000
#endif
#ifndef GMEM_MODIFY
#define GMEM_MODIFY 0x0080
#endif
#ifndef GMEM_DISCARDABLE
#define GMEM_DISCARDABLE 0x0F00
#endif
#ifndef GMEM_LOCKCOUNT
#define GMEM_LOCKCOUNT 0x00FF
#endif
#ifndef GMEM_INVALID_HANDLE
#define GMEM_INVALID_HANDLE 0x8000
#endif
#ifndef GHND
#define GHND (GMEM_MOVEABLE | GMEM_ZEROINIT)
#endif
#ifndef GPTR
#define GPTR (GMEM_FIXED | GMEM_ZEROINIT)
#endif

#ifndef SS_LEFT
#define SS_LEFT 0x00000000
#endif
#ifndef SS_CENTER
#define SS_CENTER 0x00000001
#endif
#ifndef SS_RIGHT
#define SS_RIGHT 0x00000002
#endif
#ifndef SS_ETCHEDHORZ
#define SS_ETCHEDHORZ 0x00000010
#endif
#ifndef BS_PUSHBUTTON
#define BS_PUSHBUTTON 0x00000000
#endif
#ifndef BS_DEFPUSHBUTTON
#define BS_DEFPUSHBUTTON 0x00000001
#endif
#ifndef BS_AUTOCHECKBOX
#define BS_AUTOCHECKBOX 0x00000003
#endif
#ifndef BS_GROUPBOX
#define BS_GROUPBOX 0x00000007
#endif
#ifndef BS_AUTORADIOBUTTON
#define BS_AUTORADIOBUTTON 0x00000009
#endif
#ifndef ES_AUTOHSCROLL
#define ES_AUTOHSCROLL 0x0080
#endif
#ifndef LBS_NOTIFY
#define LBS_NOTIFY 0x0001
#endif
#ifndef LBS_SORT
#define LBS_SORT 0x0002
#endif
#ifndef LBS_NOINTEGRALHEIGHT
#define LBS_NOINTEGRALHEIGHT 0x0100
#endif
#ifndef CBS_DROPDOWN
#define CBS_DROPDOWN 0x0002
#endif
#ifndef CBS_DROPDOWNLIST
#define CBS_DROPDOWNLIST 0x0003
#endif
#ifndef CBS_AUTOHSCROLL
#define CBS_AUTOHSCROLL 0x0040
#endif
#ifndef EM_SETSEL
#define EM_SETSEL 0x00b1
#endif
#ifndef EM_GETSEL
#define EM_GETSEL 0x00b0
#endif
#ifndef EM_GETLINECOUNT
#define EM_GETLINECOUNT 0x00ba
#endif
#ifndef EM_GETHANDLE
#define EM_GETHANDLE 0x00bd
#endif
#ifndef EM_REPLACESEL
#define EM_REPLACESEL 0x00c2
#endif
#ifndef CB_ADDSTRING
#define CB_ADDSTRING 0x0143
#endif
#ifndef CB_GETCOUNT
#define CB_GETCOUNT 0x0146
#endif
#ifndef CB_DELETESTRING
#define CB_DELETESTRING 0x0144
#endif
#ifndef CB_GETCURSEL
#define CB_GETCURSEL 0x0147
#endif
#ifndef CB_GETLBTEXT
#define CB_GETLBTEXT 0x0148
#endif
#ifndef CB_GETLBTEXTLEN
#define CB_GETLBTEXTLEN 0x0149
#endif
#ifndef CB_GETITEMHEIGHT
#define CB_GETITEMHEIGHT 0x0154
#endif
#ifndef CB_INSERTSTRING
#define CB_INSERTSTRING 0x014a
#endif
#ifndef CB_FINDSTRINGEXACT
#define CB_FINDSTRINGEXACT 0x0158
#endif
#ifndef CB_RESETCONTENT
#define CB_RESETCONTENT 0x014b
#endif
#ifndef CB_SETCURSEL
#define CB_SETCURSEL 0x014e
#endif
#ifndef CB_SETEDITSEL
#define CB_SETEDITSEL 0x0142
#endif
#ifndef CB_SHOWDROPDOWN
#define CB_SHOWDROPDOWN 0x014f
#endif
#ifndef CB_ERR
#define CB_ERR (-1)
#endif
#ifndef TPM_LEFTALIGN
#define TPM_LEFTALIGN 0x0000
#endif
#ifndef TPM_TOPALIGN
#define TPM_TOPALIGN 0x0000
#endif
#ifndef TPM_RIGHTBUTTON
#define TPM_RIGHTBUTTON 0x0002
#endif
#ifndef TPM_RETURNCMD
#define TPM_RETURNCMD 0x0100
#endif
#ifndef MK_CONTROL
#define MK_CONTROL 0x0008
#endif
#ifndef WHEEL_DELTA
#define WHEEL_DELTA 120
#endif
#ifndef GET_WHEEL_DELTA_WPARAM
#define GET_WHEEL_DELTA_WPARAM(wparam) ((short)HIWORD(wparam))
#endif
#ifndef GET_KEYSTATE_WPARAM
#define GET_KEYSTATE_WPARAM(wparam) LOWORD(wparam)
#endif
#ifndef LB_ADDSTRING
#define LB_ADDSTRING 0x0180
#endif
#ifndef LB_SETCURSEL
#define LB_SETCURSEL 0x0186
#endif
#ifndef LB_GETCURSEL
#define LB_GETCURSEL 0x0188
#endif
#ifndef LB_GETTEXT
#define LB_GETTEXT 0x0189
#endif
#ifndef LB_GETTEXTLEN
#define LB_GETTEXTLEN 0x018a
#endif
#ifndef LB_GETITEMRECT
#define LB_GETITEMRECT 0x0198
#endif
#ifndef LB_GETTOPINDEX
#define LB_GETTOPINDEX 0x018e
#endif
#ifndef LB_SETTOPINDEX
#define LB_SETTOPINDEX 0x0197
#endif
#ifndef LB_RESETCONTENT
#define LB_RESETCONTENT 0x0184
#endif
#ifndef LB_ERR
#define LB_ERR (-1)
#endif
#ifndef BM_GETCHECK
#define BM_GETCHECK 0x00f0
#endif
#ifndef BM_SETCHECK
#define BM_SETCHECK 0x00f1
#endif
#ifndef BST_UNCHECKED
#define BST_UNCHECKED 0x0000
#endif
#ifndef BST_CHECKED
#define BST_CHECKED 0x0001
#endif

#ifndef WM_USER
#define WM_USER 0x0400
#endif
#ifndef WM_APP
#define WM_APP 0x8000
#endif
#ifndef WM_NULL
#define WM_NULL 0x0000
#endif
#ifndef WM_CREATE
#define WM_CREATE 0x0001
#endif
#ifndef WM_DESTROY
#define WM_DESTROY 0x0002
#endif
#ifndef WM_SIZE
#define WM_SIZE 0x0005
#endif
#ifndef WM_SETFOCUS
#define WM_SETFOCUS 0x0007
#endif
#ifndef WM_KILLFOCUS
#define WM_KILLFOCUS 0x0008
#endif
#ifndef WM_ENABLE
#define WM_ENABLE 0x000a
#endif
#ifndef WM_SETREDRAW
#define WM_SETREDRAW 0x000b
#endif
#ifndef WM_SETTEXT
#define WM_SETTEXT 0x000c
#endif
#ifndef WM_GETTEXT
#define WM_GETTEXT 0x000d
#endif
#ifndef WM_GETTEXTLENGTH
#define WM_GETTEXTLENGTH 0x000e
#endif
#ifndef WM_PAINT
#define WM_PAINT 0x000f
#endif
#ifndef WM_ACTIVATE
#define WM_ACTIVATE 0x0006
#endif
#ifndef WM_ACTIVATEAPP
#define WM_ACTIVATEAPP 0x001c
#endif
#ifndef WM_CHILDACTIVATE
#define WM_CHILDACTIVATE 0x0022
#endif
#ifndef WM_GETMINMAXINFO
#define WM_GETMINMAXINFO 0x0024
#endif
#ifndef WM_ERASEBKGND
#define WM_ERASEBKGND 0x0014
#endif
#ifndef WM_WININICHANGE
#define WM_WININICHANGE 0x001a
#endif
#ifndef WM_SETCURSOR
#define WM_SETCURSOR 0x0020
#endif
#ifndef WM_QUEUESYNC
#define WM_QUEUESYNC 0x0023
#endif
#ifndef WM_GETDLGCODE
#define WM_GETDLGCODE 0x0087
#endif
#ifndef WM_NCACTIVATE
#define WM_NCACTIVATE 0x0086
#endif
#ifndef WM_INITMENUPOPUP
#define WM_INITMENUPOPUP 0x0117
#endif
#ifndef WM_MENUSELECT
#define WM_MENUSELECT 0x011F
#endif
#ifndef WM_MENUCHAR
#define WM_MENUCHAR 0x0120
#endif
#ifndef WM_ENTERIDLE
#define WM_ENTERIDLE 0x0121
#endif
#ifndef MSGF_DIALOGBOX
#define MSGF_DIALOGBOX 0
#endif
#ifndef MSGF_MESSAGEBOX
#define MSGF_MESSAGEBOX 1
#endif
#ifndef MSGF_MENU
#define MSGF_MENU 2
#endif
#ifndef SIZEICONIC
#define SIZEICONIC 1
#endif
#ifndef SIZE_RESTORED
#define SIZE_RESTORED 0
#endif
#ifndef SIZE_MAXIMIZED
#define SIZE_MAXIMIZED 2
#endif
#ifndef MA_ACTIVATE
#define MA_ACTIVATE 1
#endif
#ifndef MA_ACTIVATEANDEAT
#define MA_ACTIVATEANDEAT 2
#endif
#ifndef MA_NOACTIVATE
#define MA_NOACTIVATE 3
#endif
#ifndef WM_CHAR
#define WM_CHAR 0x0102
#endif
#ifndef WM_KEYDOWN
#define WM_KEYDOWN 0x0100
#endif
#ifndef WM_KEYUP
#define WM_KEYUP 0x0101
#endif
#ifndef WM_SYSKEYDOWN
#define WM_SYSKEYDOWN 0x0104
#endif
#ifndef WM_MOUSEFIRST
#define WM_MOUSEFIRST 0x0200
#endif
#ifndef WM_LBUTTONDBLCLK
#define WM_LBUTTONDBLCLK 0x0203
#endif
#ifndef WM_MOUSELAST
#define WM_MOUSELAST 0x0209
#endif
#ifndef WM_CUT
#define WM_CUT 0x0300
#endif
#ifndef WM_COPY
#define WM_COPY 0x0301
#endif
#ifndef WM_PASTE
#define WM_PASTE 0x0302
#endif
#ifndef WM_CLEAR
#define WM_CLEAR 0x0303
#endif
#ifndef WM_CTLCOLOREDIT
#define WM_CTLCOLOREDIT 0x0133
#endif
#ifndef EN_ERRSPACE
#define EN_ERRSPACE 0x0500
#endif
#ifndef EM_SETHANDLE
#define EM_SETHANDLE 0x00bc
#endif
#ifndef DLGC_WANTARROWS
#define DLGC_WANTARROWS 0x0001
#endif
#ifndef DLGC_HASSETSEL
#define DLGC_HASSETSEL 0x0008
#endif
#ifndef DLGC_WANTCHARS
#define DLGC_WANTCHARS 0x0080
#endif

#ifndef FORMAT_MESSAGE_IGNORE_INSERTS
#define FORMAT_MESSAGE_IGNORE_INSERTS 0x00000200
#endif
#ifndef SWP_HIDEWINDOW
#define SWP_HIDEWINDOW 0x0080
#endif
#ifndef SWP_FRAMECHANGED
#define SWP_FRAMECHANGED 0x0020
#endif
#ifndef HWND_TOP
#define HWND_TOP ((HWND)0)
#endif
#ifndef SM_CYFULLSCREEN
#define SM_CYFULLSCREEN 17
#endif
#ifndef FORMAT_MESSAGE_FROM_SYSTEM
#define FORMAT_MESSAGE_FROM_SYSTEM 0x00001000
#endif

#ifndef CP_ACP
#define CP_ACP 0
#endif

#ifndef WAIT_OBJECT_0
#define WAIT_OBJECT_0 0
#endif
#ifndef SW_RESTORE
#define SW_RESTORE 9
#endif
#ifndef INPUT_MOUSE
#define INPUT_MOUSE 0
#endif
#ifndef INPUT_KEYBOARD
#define INPUT_KEYBOARD 1
#endif
#ifndef MOUSEEVENTF_LEFTDOWN
#define MOUSEEVENTF_LEFTDOWN 0x0002
#endif
#ifndef MOUSEEVENTF_LEFTUP
#define MOUSEEVENTF_LEFTUP 0x0004
#endif
#ifndef KEYEVENTF_KEYUP
#define KEYEVENTF_KEYUP 0x0002
#endif
#ifndef MAPVK_VK_TO_VSC
#define MAPVK_VK_TO_VSC 0
#endif
#ifndef GUI_INMENUMODE
#define GUI_INMENUMODE 0x00000004
#endif
#ifndef SMTO_ABORTIFHUNG
#define SMTO_ABORTIFHUNG 0x0002
#endif
#ifndef SMTO_BLOCK
#define SMTO_BLOCK 0x0001
#endif
#ifndef CAPTUREBLT
#define CAPTUREBLT ((DWORD)0x40000000)
#endif
#ifndef DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2
#define DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 ((HANDLE)-4)
#endif
#ifndef CREATE_UNICODE_ENVIRONMENT
#define CREATE_UNICODE_ENVIRONMENT 0x00000400
#endif
#ifndef STILL_ACTIVE
#define STILL_ACTIVE 259
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
#ifndef FILE_TYPE_DISK
#define FILE_TYPE_DISK 0x0001
#endif
#ifndef DRIVE_REMOTE
#define DRIVE_REMOTE 4
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
#ifndef DEFAULT_CHARSET
#define DEFAULT_CHARSET 1
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

#ifndef ERROR_SUCCESS
#define ERROR_SUCCESS 0
#endif
#ifndef ERROR_INVALID_PARAMETER
#define ERROR_INVALID_PARAMETER 87
#endif
#ifndef HFILE_ERROR
#define HFILE_ERROR ((HFILE)-1)
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
#ifndef ERROR_NOT_ENOUGH_MEMORY
#define ERROR_NOT_ENOUGH_MEMORY 8
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
#ifndef EXCEPTION_ACCESS_VIOLATION
#define EXCEPTION_ACCESS_VIOLATION 0xc0000005
#endif
#ifndef EXCEPTION_INT_DIVIDE_BY_ZERO
#define EXCEPTION_INT_DIVIDE_BY_ZERO 0xc0000094
#endif
#ifndef EXCEPTION_EXECUTE_HANDLER
#define EXCEPTION_EXECUTE_HANDLER 1
#endif
#ifndef EXCEPTION_CONTINUE_SEARCH
#define EXCEPTION_CONTINUE_SEARCH 0
#endif
#ifndef IMAGE_FILE_MACHINE_AMD64
#define IMAGE_FILE_MACHINE_AMD64 0x8664
#endif
#ifndef LOAD_LIBRARY_SEARCH_APPLICATION_DIR
#define LOAD_LIBRARY_SEARCH_APPLICATION_DIR 0x00000200
#endif
#ifndef LOAD_LIBRARY_SEARCH_USER_DIRS
#define LOAD_LIBRARY_SEARCH_USER_DIRS 0x00000400
#endif
#ifndef LOAD_LIBRARY_SEARCH_SYSTEM32
#define LOAD_LIBRARY_SEARCH_SYSTEM32 0x00000800
#endif
#ifndef BASE_SEARCH_PATH_ENABLE_SAFE_SEARCHMODE
#define BASE_SEARCH_PATH_ENABLE_SAFE_SEARCHMODE 0x00000001
#endif
#ifndef BASE_SEARCH_PATH_PERMANENT
#define BASE_SEARCH_PATH_PERMANENT 0x00008000
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
HMODULE LoadLibraryA(LPCSTR module_name);
HMODULE LoadLibraryExW(LPCWSTR module_name, HANDLE file, DWORD flags);
BOOL FreeLibrary(HMODULE module);
HMODULE GetModuleHandleW(LPCWSTR module_name);
HMODULE GetModuleHandleA(LPCSTR module_name);
HANDLE GetCurrentThread(void);
#ifndef LoadLibrary
#define LoadLibrary LoadLibraryA
#endif
#ifndef GetModuleHandle
#define GetModuleHandle GetModuleHandleA
#endif
DWORD GetTempPathA(DWORD buffer_length, LPSTR buffer);
UINT GetTempFileNameA(LPCSTR path_name, LPCSTR prefix, UINT unique, LPSTR file_name);
DWORD GetCurrentProcessId(void);
DWORD GetCurrentThreadId(void);
DWORD CharUpperBuffA(LPSTR text, DWORD length);
UINT RegisterClipboardFormatA(LPCSTR name);
BOOL MessageBeep(UINT type);
int MessageBoxA(HWND window, LPCSTR text, LPCSTR caption, UINT type);
BOOL GetStringTypeA(DWORD locale, DWORD type, LPCSTR source, int count,
                    WORD* char_type);
int MulDiv(int number, int numerator, int denominator);
LPSTR lstrcpynA(LPSTR destination, LPCSTR source, int count);
LPSTR CharLowerA(LPSTR string);
DWORD GetEnvironmentVariableA(LPCSTR name, LPSTR buffer, DWORD size);
DWORD GetEnvironmentVariableW(LPCWSTR name, LPWSTR buffer, DWORD size);
BOOL SetEnvironmentVariableA(LPCSTR name, LPCSTR value);
LPWCH GetEnvironmentStringsW(void);
BOOL FreeEnvironmentStringsW(LPWCH environment);

VOID OutputDebugStringA(LPCSTR text);
DWORD FormatMessageA(DWORD flags, LPCVOID source, DWORD message_id,
                     DWORD language_id, LPSTR buffer, DWORD size,
                     LPVOID arguments);
DWORD GetModuleFileNameA(HMODULE module, LPSTR file_name, DWORD size);
HANDLE GetCurrentProcess(void);
BOOL TerminateProcess(HANDLE process, UINT exit_code);
BOOL GetExitCodeProcess(HANDLE process, DWORD* exit_code);
BOOL CreateProcessW(LPCWSTR application_name, LPWSTR command_line,
                    LPVOID process_attributes, LPVOID thread_attributes,
                    BOOL inherit_handles, DWORD creation_flags,
                    LPVOID environment, LPCWSTR current_directory,
                    LPSTARTUPINFOW startup_info,
                    LPPROCESS_INFORMATION process_information);
VOID RaiseException(DWORD code, DWORD flags, DWORD argument_count,
                    const ULONG_PTR* arguments);

BOOL AppendMenuA(HMENU menu, UINT flags, UINT_PTR new_item, LPCSTR new_item_text);
BOOL AppendMenuW(HMENU menu, UINT flags, UINT_PTR new_item, LPCWSTR new_item_text);
BOOL DeleteMenu(HMENU menu, UINT position, UINT flags);
BOOL RemoveMenu(HMENU menu, UINT position, UINT flags);
BOOL ModifyMenuA(HMENU menu, UINT position, UINT flags, UINT_PTR new_item,
                 LPCSTR new_item_text);
BOOL ModifyMenuW(HMENU menu, UINT position, UINT flags, UINT_PTR new_item,
                 LPCWSTR new_item_text);
BOOL InsertMenuA(HMENU menu, UINT position, UINT flags, UINT_PTR new_item,
                 LPCSTR new_item_text);
BOOL InsertMenuW(HMENU menu, UINT position, UINT flags, UINT_PTR new_item,
                 LPCWSTR new_item_text);
UINT GetMenuItemID(HMENU menu, int position);
HMENU GetSubMenu(HMENU menu, int position);
HMENU GetMenu(HWND window);
int GetMenuItemCount(HMENU menu);
int GetMenuStringW(HMENU menu, UINT item, LPWSTR string, int max_count,
                   UINT flags);
BOOL SetMenuInfo(HMENU menu, const MENUINFO* menu_info);
UINT GetMenuState(HMENU menu, UINT id, UINT flags);
HMENU CreateMenu(void);
HMENU CreatePopupMenu(void);
BOOL IsMenu(HMENU menu);
BOOL DestroyMenu(HMENU menu);
BOOL SetMenuItemBitmaps(HMENU menu, UINT position, UINT flags,
                        HBITMAP unchecked_bitmap, HBITMAP checked_bitmap);
DWORD CheckMenuItem(HMENU menu, UINT id_check_item, UINT check);
BOOL CheckMenuRadioItem(HMENU menu, UINT first, UINT last, UINT check,
                        UINT flags);
UINT TrackPopupMenu(HMENU menu, UINT flags, int x, int y, int reserved,
                    HWND window, const RECT* rect);

ATOM RegisterClassA(const WNDCLASSA* window_class);
#define RegisterClass RegisterClassA
ATOM RegisterClassExA(const WNDCLASSEXA* window_class);
ATOM RegisterClassExW(const WNDCLASSEXW* window_class);
HCURSOR LoadCursorA(HINSTANCE instance, LPCSTR cursor_name);
HCURSOR LoadCursorW(HINSTANCE instance, LPCWSTR cursor_name);
#define LoadCursor LoadCursorA
BOOL IsWindow(HWND window);
BOOL IsChild(HWND parent, HWND window);
HWND GetActiveWindow(void);
BOOL AdjustWindowRectEx(LPRECT rectangle, DWORD style, BOOL menu,
                        DWORD extended_style);
BOOL GetWindowRect(HWND window, LPRECT rectangle);
BOOL GetClientRect(HWND window, LPRECT rectangle);
BOOL ClientToScreen(HWND window, LPPOINT point);
BOOL ScreenToClient(HWND window, LPPOINT point);
BOOL IntersectRect(LPRECT destination, const RECT* source1, const RECT* source2);
BOOL OffsetRect(LPRECT rectangle, int dx, int dy);
BOOL InvalidateRect(HWND window, const RECT* rectangle, BOOL erase);
BOOL SystemParametersInfoA(UINT action, UINT parameter, LPVOID data, UINT flags);
BOOL ShowWindow(HWND window, int command_show);
BOOL MoveWindow(HWND window, int x, int y, int width, int height, BOOL repaint);
BOOL EnableWindow(HWND window, BOOL enable);
BOOL IsWindowEnabled(HWND window);
BOOL UpdateWindow(HWND window);
HWND SetActiveWindow(HWND window);
HWND SetFocus(HWND window);
HCURSOR SetCursor(HCURSOR cursor);
HWND SetCapture(HWND window);
HWND GetCapture(void);
BOOL ReleaseCapture(void);
HWND GetWindow(HWND window, UINT command);
HWND GetTopWindow(HWND window);
HWND GetParent(HWND window);
HWND GetAncestor(HWND window, UINT flags);
HWND GetFocus(void);
int GetDlgCtrlID(HWND control);
BOOL IsZoomed(HWND window);
BOOL DrawMenuBar(HWND window);
int GetSystemMetrics(int index);
BOOL SetWindowPos(HWND window, HWND insert_after, int x, int y, int cx, int cy,
                  UINT flags);
LONG_PTR GetWindowLongPtrA(HWND window, int index);
LONG_PTR GetWindowLongPtrW(HWND window, int index);
LONG_PTR SetWindowLongPtrA(HWND window, int index, LONG_PTR new_long);
LONG_PTR SetWindowLongPtrW(HWND window, int index, LONG_PTR new_long);
#ifndef GetWindowLongPtr
#define GetWindowLongPtr GetWindowLongPtrA
#endif
#ifndef SetWindowLongPtr
#define SetWindowLongPtr SetWindowLongPtrA
#endif
LONG GetWindowLongA(HWND window, int index);
LONG SetWindowLongA(HWND window, int index, LONG new_long);
WORD GetWindowWord(HWND window, int index);
WORD SetWindowWord(HWND window, int index, WORD new_word);
int GetClassNameW(HWND window, LPWSTR class_name, int max_count);
LPWSTR lstrcpyW(LPWSTR destination, LPCWSTR source);
int lstrcmpW(LPCWSTR first, LPCWSTR second);
int lstrcmpiW(LPCWSTR first, LPCWSTR second);
BOOL SetWindowTextW(HWND window, LPCWSTR text);
HANDLE GetPropA(HWND window, LPCSTR string);
HANDLE GetPropW(HWND window, LPCWSTR string);
BOOL SetPropW(HWND window, LPCWSTR string, HANDLE data);
HANDLE RemovePropW(HWND window, LPCWSTR string);
BOOL IsWindowVisible(HWND window);
BOOL RedrawWindow(HWND window, const RECT* update_rect, HRGN update_region,
                  UINT flags);
LRESULT DefWindowProcA(HWND window, UINT message, WPARAM wparam,
                       LPARAM lparam);
LRESULT DefWindowProcW(HWND window, UINT message, WPARAM wparam,
                       LPARAM lparam);
LRESULT CallWindowProcW(WNDPROC previous, HWND window, UINT message,
                        WPARAM wparam, LPARAM lparam);
BOOL DestroyWindow(HWND window);
DWORD GetSysColor(int index);
HBRUSH GetSysColorBrush(int index);
BOOL EnumWindows(WNDENUMPROC enum_func, LPARAM parameter);
DWORD GetWindowThreadProcessId(HWND window, DWORD* process_id);
BOOL SetProcessDpiAwarenessContext(HANDLE value);
HWND CreateWindowExA(DWORD extended_style, LPCSTR class_name,
                     LPCSTR window_name, DWORD style, int x, int y, int width,
                     int height, HWND parent, HMENU menu, HINSTANCE instance,
                     LPVOID parameter);
HWND CreateWindowExW(DWORD extended_style, LPCWSTR class_name,
                     LPCWSTR window_name, DWORD style, int x, int y, int width,
                     int height, HWND parent, HMENU menu, HINSTANCE instance,
                     LPVOID parameter);
HWND FindWindowExW(HWND parent, HWND child_after, LPCWSTR class_name,
                   LPCWSTR window_name);
LRESULT SendMessageA(HWND window, UINT message, WPARAM wparam, LPARAM lparam);
LRESULT SendMessageW(HWND window, UINT message, WPARAM wparam, LPARAM lparam);
LRESULT SendMessageTimeoutW(HWND window, UINT message, WPARAM wparam,
                            LPARAM lparam, UINT flags, UINT timeout,
                            DWORD_PTR* result);
BOOL PostMessageA(HWND window, UINT message, WPARAM wparam, LPARAM lparam);
BOOL PostMessageW(HWND window, UINT message, WPARAM wparam, LPARAM lparam);
BOOL TranslateMessage(const MSG* message);
LRESULT DispatchMessageA(const MSG* message);
BOOL PeekMessageA(LPMSG message, HWND window, UINT filter_min, UINT filter_max,
                  UINT remove_message);
BOOL GetMessageA(LPMSG message, HWND window, UINT filter_min, UINT filter_max);
BOOL IsDialogMessageA(HWND dialog, LPMSG message);
VOID PostQuitMessage(int exit_code);
BOOL WaitMessage(void);
BOOL EnumChildWindows(HWND parent, WNDENUMPROC enum_func, LPARAM parameter);
BOOL EnumThreadWindows(DWORD thread_id, WNDENUMPROC enum_func, LPARAM parameter);
UINT_PTR SetTimer(HWND window, UINT_PTR event, UINT elapsed, LPVOID timer_func);
BOOL KillTimer(HWND window, UINT_PTR event);
HWND GetDlgItem(HWND dialog, int id);
HWND GetForegroundWindow(void);
BOOL SetForegroundWindow(HWND window);
BOOL BringWindowToTop(HWND window);
BOOL AttachThreadInput(DWORD id_attach, DWORD id_attach_to, BOOL attach);
BOOL GetGUIThreadInfo(DWORD thread_id, PGUITHREADINFO gui);
UINT MapVirtualKeyW(UINT code, UINT map_type);
UINT SendInput(UINT inputs, INPUT* input, int size);
BOOL IsHungAppWindow(HWND window);
HGDIOBJ GetStockObject(int object);
HGDIOBJ SelectObject(HDC device_context, HGDIOBJ object);
HFONT CreateFontIndirectA(const LOGFONTA* logical_font);
HFONT CreateFontIndirectW(const LOGFONTW* logical_font);
BOOL GetTextMetricsA(HDC device_context, LPTEXTMETRICA text_metric);
BOOL GetTextMetricsW(HDC device_context, LPTEXTMETRICW text_metric);
BOOL GetCharWidthA(HDC device_context, UINT first_char, UINT last_char,
                   LPINT buffer);
HBITMAP CreateBitmapIndirect(const BITMAP* bitmap);
HBITMAP CreateBitmap(int width, int height, UINT planes, UINT bits_per_pixel,
                     LPCVOID bits);
HBITMAP CreateDIBitmap(HDC device_context, const BITMAPINFOHEADER* header,
                       DWORD init, LPCVOID bits, const BITMAPINFO* info,
                       UINT usage);
HDC CreateCompatibleDC(HDC device_context);
HBITMAP CreateCompatibleBitmap(HDC device_context, int width, int height);
BOOL DeleteDC(HDC device_context);
HBRUSH CreatePatternBrush(HBITMAP bitmap);
HDC CreateDCA(LPCSTR driver, LPCSTR device, LPCSTR output, LPCVOID init_data);
HDC CreateICA(LPCSTR driver, LPCSTR device, LPCSTR output, LPCVOID init_data);
#define CreateDC CreateDCA
#define CreateIC CreateICA
HMENU GetSystemMenu(HWND window, BOOL revert);
BOOL ChangeMenu(HMENU menu, UINT position, LPCSTR new_item, UINT item_id, UINT flags);
HANDLE LoadImageA(HINSTANCE instance, LPCSTR name, UINT type, int desired_x,
                  int desired_y, UINT load_flags);
HANDLE LoadImageW(HINSTANCE instance, LPCWSTR name, UINT type, int desired_x,
                  int desired_y, UINT load_flags);
#define LoadImage LoadImageA
HICON LoadIconA(HINSTANCE instance, LPCSTR icon_name);
HICON LoadIconW(HINSTANCE instance, LPCWSTR icon_name);
BOOL DrawIconEx(HDC device_context, int x, int y, HICON icon, int width,
                int height, UINT step_if_ani_cur, HBRUSH flicker_free_draw,
                UINT flags);
BOOL DestroyIcon(HICON icon);
HRGN CreateRectRgn(int left, int top, int right, int bottom);
int GetObjectA(HANDLE object, int buffer_size, LPVOID object_data);
LONG GetBitmapBits(HBITMAP bitmap, LONG count, LPVOID bits);
DWORD GetBitmapDimension(HBITMAP bitmap);
int GetClassNameA(HWND window, LPSTR class_name, int max_count);
BOOL SetWindowTextA(HWND window, LPCSTR text);
BOOL GlobalMemoryStatusEx(LPMEMORYSTATUSEX buffer);
int GetWindowTextLengthA(HWND window);
int GetWindowTextA(HWND window, LPSTR text, int max_count);
int GetWindowTextLengthW(HWND window);
int GetWindowTextW(HWND window, LPWSTR text, int max_count);
BOOL GetComboBoxInfo(HWND combo_box, PCOMBOBOXINFO combo_box_info);
int MultiByteToWideChar(UINT code_page, DWORD flags, LPCSTR multi_byte,
                        int multi_byte_count, LPWSTR wide_char,
                        int wide_char_count);
int WideCharToMultiByte(UINT code_page, DWORD flags, LPCWSTR wide_char,
                        int wide_char_count, LPSTR multi_byte, int multi_byte_count,
                        LPCSTR default_char, BOOL* used_default_char);
HDC GetDC(HWND window);
int ReleaseDC(HWND window, HDC device_context);
int EnumFontFamiliesExA(HDC device_context, LPLOGFONTA logfont,
                        FONTENUMPROCA enum_font_proc, LPARAM parameter,
                        DWORD flags);
int EnumFontsA(HDC device_context, LPCSTR face_name,
               FONTENUMPROCA enum_font_proc, LPARAM parameter);
SHORT GetKeyState(int virtual_key);
BOOL PatBlt(HDC device_context, int x, int y, int width, int height,
            DWORD raster_operation);
BOOL SetCursorPos(int x, int y);
BOOL GetCursorPos(LPPOINT point);
HWND FindWindowA(LPCSTR class_name, LPCSTR window_name);
BOOL IsIconic(HWND window);
BOOL OpenIcon(HWND window);
BOOL IsClipboardFormatAvailable(UINT format);
HFILE OpenFile(LPCSTR file_name, LPOFSTRUCT reopen_buffer, UINT style);
int _lclose(HFILE file);
UINT _lread(HFILE file, LPVOID buffer, UINT bytes);
UINT _lwrite(HFILE file, LPCCH buffer, UINT bytes);
LONG _llseek(HFILE file, LONG offset, int origin);
HDC BeginPaint(HWND window, LPPAINTSTRUCT paint);
VOID EndPaint(HWND window, LPPAINTSTRUCT paint);
HWND GetClipboardOwner(void);
BOOL OpenClipboard(HWND window);
BOOL EmptyClipboard(void);
HANDLE GetClipboardData(UINT format);
HANDLE SetClipboardData(UINT format, HANDLE memory);
BOOL CloseClipboard(void);
int SetMapMode(HDC device_context, int map_mode);
HBRUSH CreateSolidBrush(DWORD color);
HPEN CreatePen(int style, int width, DWORD color);
HPEN CreatePenIndirect(const LOGPEN* log_pen);
HDC CreateMetaFile(LPSTR file_name);
HMETAFILE CloseMetaFile(HDC device_context);
HBITMAP LoadBitmapA(HANDLE instance, LPSTR bitmap_name);
HBITMAP LoadBitmapW(HANDLE instance, LPWSTR bitmap_name);
int GetDeviceCaps(HDC device_context, int index);
int SetStretchBltMode(HDC device_context, int stretch_mode);
int SetROP2(HDC device_context, int draw_mode);
int Escape(HDC device_context, int escape, int count, LPSTR input, LPSTR output);
HWND WindowFromPoint(POINT point);
DWORD SetViewportExt(HDC device_context, int x, int y);
DWORD SetWindowExt(HDC device_context, int x, int y);
VOID SetScrollRange(HWND window, int bar, int minimum, int maximum,
                    BOOL redraw);
int SetScrollPos(HWND window, int bar, int position, BOOL redraw);
HANDLE GlobalAlloc(UINT flags, SIZE_T bytes);
LPVOID GlobalLock(HANDLE memory);
BOOL GlobalUnlock(HANDLE memory);
HANDLE GlobalFree(HANDLE memory);
HANDLE GlobalReAlloc(HANDLE memory, SIZE_T bytes, UINT flags);
SIZE_T GlobalSize(HANDLE memory);
DWORD GlobalHandle(LPCVOID memory);
DWORD GlobalCompact(DWORD minimum_free);
LPVOID GlobalWire(HANDLE memory);
UINT GlobalFlags(HANDLE memory);
HLOCAL LocalAlloc(UINT flags, SIZE_T bytes);
HLOCAL LocalFree(HLOCAL memory);
HLOCAL LocalReAlloc(HLOCAL memory, SIZE_T bytes, UINT flags);

#ifndef CreateWindow
#define CreateWindow(class_name, window_name, style, x, y, width, height,       \
                     parent, menu, instance, parameter)                        \
    CreateWindowExA(0, (class_name), (window_name), (style), (x), (y), (width), \
                    (height), (parent), (menu), (instance), (parameter))
#endif
#ifndef GetModuleHandle
#define GetModuleHandle GetModuleHandleA
#endif
#ifndef GetObject
#define GetObject GetObjectA
#endif
#ifndef GetWindowLong
#define GetWindowLong GetWindowLongA
#endif
#ifndef SetWindowLong
#define SetWindowLong SetWindowLongA
#endif
#ifndef GetWindowText
#define GetWindowText(window, text, max_count) \
    GetWindowTextA((HWND)(window), (text), (max_count))
#endif
#ifndef GetWindowTextLength
#define GetWindowTextLength(window) GetWindowTextLengthA((HWND)(window))
#endif
#ifndef SendMessage
#define SendMessage(hwnd, message, wparam, lparam) \
    SendMessageA((HWND)(hwnd), (message), (WPARAM)(wparam), (LPARAM)(lparam))
#endif
#ifndef GetMessage
#define GetMessage(message, window, filter_min, filter_max) \
    GetMessageA((message), (HWND)(window), (filter_min), (filter_max))
#endif
#ifndef DefWindowProc
#define DefWindowProc(window, message, wparam, lparam) \
    DefWindowProcA((HWND)(window), (message), (WPARAM)(wparam), (LPARAM)(lparam))
#endif
#ifndef GetNextWindow
#define GetNextWindow GetWindow
#endif
#ifndef DispatchMessage
#define DispatchMessage DispatchMessageA
#endif
#ifndef PeekMessage
#define PeekMessage PeekMessageA
#endif
#ifndef PostMessage
#define PostMessage(window, message, wparam, lparam) \
    PostMessageA((HWND)(window), (message), (WPARAM)(wparam), (LPARAM)(lparam))
#endif
#ifndef FindWindow
#define FindWindow FindWindowA
#endif
#ifndef CreateFontIndirect
#define CreateFontIndirect CreateFontIndirectA
#endif
#ifndef LoadBitmap
#define LoadBitmap LoadBitmapA
#endif
#ifndef LoadIcon
#define LoadIcon LoadIconA
#endif
#ifndef GetTextMetrics
#define GetTextMetrics GetTextMetricsA
#endif
#ifndef GetCharWidth
#define GetCharWidth GetCharWidthA
#endif
#ifndef EnumFonts
#define EnumFonts EnumFontsA
#endif

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
BOOL PtInRect(const RECT* rect, POINT point);
BOOL LineTo(HDC device_context, int x, int y);
int SaveDC(HDC device_context);
BOOL RestoreDC(HDC device_context, int saved_dc);
int IntersectClipRect(HDC device_context, int left, int top, int right, int bottom);
int ExcludeClipRect(HDC device_context, int left, int top, int right, int bottom);
COLORREF SetPixel(HDC device_context, int x, int y, COLORREF color);
COLORREF GetPixel(HDC device_context, int x, int y);
int GetDIBits(HDC device_context, HBITMAP bitmap, UINT start_scan,
              UINT scan_lines, LPVOID bits, BITMAPINFO* bitmap_info,
              UINT usage);
BOOL DeleteObject(HGDIOBJ object);
BOOL StretchBlt(HDC destination, int x_destination, int y_destination,
                int width_destination, int height_destination, HDC source,
                int x_source, int y_source, int width_source, int height_source,
                DWORD raster_operation);
BOOL BitBlt(HDC destination, int x_destination, int y_destination, int width,
            int height, HDC source, int x_source, int y_source,
            DWORD raster_operation);
int SetBkMode(HDC device_context, int background_mode);
COLORREF SetTextColor(HDC device_context, COLORREF color);
int DrawTextW(HDC device_context, LPCWSTR text, int count, RECT* rect,
              UINT format);
BOOL TextOutW(HDC device_context, int x, int y, LPCWSTR text, int count);
int FillRect(HDC device_context, const RECT* rect, HBRUSH brush);
int FrameRect(HDC device_context, const RECT* rect, HBRUSH brush);
BOOL Rectangle(HDC device_context, int left, int top, int right, int bottom);
BOOL Ellipse(HDC device_context, int left, int top, int right, int bottom);
BOOL Polygon(HDC device_context, const POINT* points, int count);
BOOL DrawEdge(HDC device_context, RECT* rect, UINT edge, UINT flags);
ULONGLONG GetTickCount64(void);
VOID Sleep(DWORD milliseconds);

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
DWORD WaitForSingleObject(HANDLE handle, DWORD milliseconds);
DWORD GetLastError(void);
VOID SetLastError(DWORD error);
DWORD GetFileType(HANDLE file);
VOID GetLocalTime(SYSTEMTIME* system_time);
DWORD GetFileAttributesA(LPCSTR file_name);
BOOL GetFileAttributesExA(LPCSTR file_name, int info_level_id,
                          LPVOID file_information);
BOOL CreateDirectoryA(LPCSTR path_name, LPVOID security_attributes);
BOOL RemoveDirectoryA(LPCSTR path_name);
BOOL DeleteFileA(LPCSTR file_name);
DWORD GetCurrentDirectoryA(DWORD buffer_length, LPSTR buffer);
DWORD GetFullPathNameA(LPCSTR file_name, DWORD buffer_length, LPSTR buffer,
                       LPSTR* file_part);
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
BOOL FileTimeToLocalFileTime(const FILETIME* file_time, FILETIME* local_file_time);
BOOL FileTimeToDosDateTime(const FILETIME* file_time, WORD* fat_date, WORD* fat_time);
BOOL GetDiskFreeSpaceExA(LPCSTR directory_name, ULARGE_INTEGER* free_bytes_available,
                         ULARGE_INTEGER* total_number_of_bytes,
                         ULARGE_INTEGER* total_number_of_free_bytes);
UINT GetDriveTypeA(LPCSTR root_path_name);
LRESULT CallNextHookEx(HANDLE hook, int code, WPARAM wparam, LPARAM lparam);
BOOL SetKeyboardState(BYTE* key_state);
USHORT CaptureStackBackTrace(DWORD frames_to_skip, DWORD frames_to_capture,
                             PVOID* back_trace, DWORD* back_trace_hash);
PTOP_LEVEL_EXCEPTION_FILTER SetUnhandledExceptionFilter(
    PTOP_LEVEL_EXCEPTION_FILTER filter);
PVOID AddVectoredExceptionHandler(ULONG first, PVECTORED_EXCEPTION_HANDLER handler);
VOID GetCurrentThreadStackLimits(ULONG_PTR* low_limit, ULONG_PTR* high_limit);
PRUNTIME_FUNCTION RtlLookupFunctionEntry(DWORD64 control_pc, DWORD64* image_base,
                                         PVOID history_table);
PVOID RtlVirtualUnwind(DWORD handler_type, DWORD64 image_base, DWORD64 control_pc,
                       PRUNTIME_FUNCTION function_entry, PCONTEXT context_record,
                       PVOID* handler_data, DWORD64* establisher_frame,
                       PVOID context_pointers);
LONG InterlockedCompareExchange(volatile LONG* destination, LONG exchange,
                                LONG comparand);
LONG InterlockedExchange(volatile LONG* target, LONG value);
LPWSTR GetCommandLineW(void);
BOOL SetDefaultDllDirectories(DWORD directory_flags);
BOOL SetSearchPathMode(DWORD flags);

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
