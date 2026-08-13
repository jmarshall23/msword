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
typedef intptr_t LONG_PTR;
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
typedef POINT* LPPOINT;

typedef struct tagRECT {
    LONG left;
    LONG top;
    LONG right;
    LONG bottom;
} RECT;
typedef RECT* LPRECT;

typedef struct tagMETAFILEPICT {
    int mm;
    int xExt;
    int yExt;
    HANDLE hMF;
} METAFILEPICT;
typedef METAFILEPICT* LPMETAFILEPICT;

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
    BYTE szPathName[120];
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
#ifndef MB_OKCANCEL
#define MB_OKCANCEL 0x0001
#endif
#ifndef MB_YESNOCANCEL
#define MB_YESNOCANCEL 0x0003
#endif
#ifndef MB_YESNO
#define MB_YESNO 0x0004
#endif
#ifndef MB_ICONQUESTION
#define MB_ICONQUESTION 0x0020
#endif
#ifndef MB_ICONEXCLAMATION
#define MB_ICONEXCLAMATION 0x0030
#endif
#ifndef MB_DEFBUTTON2
#define MB_DEFBUTTON2 0x0100
#endif
#ifndef MB_APPLMODAL
#define MB_APPLMODAL 0x0000
#endif

#ifndef IDOK
#define IDOK 1
#endif
#ifndef IDCANCEL
#define IDCANCEL 2
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

#ifndef WS_POPUP
#define WS_POPUP 0x80000000u
#endif
#ifndef WS_CHILD
#define WS_CHILD 0x40000000u
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

#ifndef WS_EX_DLGMODALFRAME
#define WS_EX_DLGMODALFRAME 0x00000001
#endif
#ifndef WS_EX_CONTROLPARENT
#define WS_EX_CONTROLPARENT 0x00010000
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

#ifndef SW_HIDE
#define SW_HIDE 0
#endif
#ifndef SW_SHOWNORMAL
#define SW_SHOWNORMAL 1
#endif
#ifndef SW_SHOW
#define SW_SHOW 5
#endif
#ifndef SHOW_OPENWINDOW
#define SHOW_OPENWINDOW 1
#endif
#ifndef SW_SHOWNA
#define SW_SHOWNA 8
#endif

#ifndef PATINVERT
#define PATINVERT ((DWORD)0x005A0049)
#endif
#ifndef PATCOPY
#define PATCOPY ((DWORD)0x00F00021)
#endif
#ifndef SRCCOPY
#define SRCCOPY ((DWORD)0x00CC0020)
#endif
#ifndef DSTINVERT
#define DSTINVERT ((DWORD)0x00550009)
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

#ifndef MM_ANISOTROPIC
#define MM_ANISOTROPIC 8
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
#ifndef ANSI_CHARSET
#define ANSI_CHARSET 0
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
#ifndef ASPECT_FILTERING
#define ASPECT_FILTERING 0x00000001
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

#ifndef GW_OWNER
#define GW_OWNER 4
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
#ifndef SC_RESTORE
#define SC_RESTORE 0xf120
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
#ifndef CB_INSERTSTRING
#define CB_INSERTSTRING 0x014a
#endif
#ifndef CB_RESETCONTENT
#define CB_RESETCONTENT 0x014b
#endif
#ifndef CB_SETCURSEL
#define CB_SETCURSEL 0x014e
#endif
#ifndef CB_ERR
#define CB_ERR (-1)
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
#ifndef WM_ERASEBKGND
#define WM_ERASEBKGND 0x0014
#endif
#ifndef WM_QUEUESYNC
#define WM_QUEUESYNC 0x0023
#endif
#ifndef WM_GETDLGCODE
#define WM_GETDLGCODE 0x0087
#endif
#ifndef WM_CHAR
#define WM_CHAR 0x0102
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
#ifndef FORMAT_MESSAGE_FROM_SYSTEM
#define FORMAT_MESSAGE_FROM_SYSTEM 0x00001000
#endif

#ifndef CP_ACP
#define CP_ACP 0
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
HMODULE GetModuleHandleA(LPCSTR module_name);
DWORD GetTempPathA(DWORD buffer_length, LPSTR buffer);
UINT GetTempFileNameA(LPCSTR path_name, LPCSTR prefix, UINT unique, LPSTR file_name);
DWORD GetCurrentProcessId(void);
DWORD CharUpperBuffA(LPSTR text, DWORD length);
UINT RegisterClipboardFormatA(LPCSTR name);
BOOL MessageBeep(UINT type);
int MessageBoxA(HWND window, LPCSTR text, LPCSTR caption, UINT type);
BOOL GetStringTypeA(DWORD locale, DWORD type, LPCSTR source, int count,
                    WORD* char_type);
int MulDiv(int number, int numerator, int denominator);
LPSTR lstrcpynA(LPSTR destination, LPCSTR source, int count);
DWORD GetEnvironmentVariableA(LPCSTR name, LPSTR buffer, DWORD size);
BOOL SetEnvironmentVariableA(LPCSTR name, LPCSTR value);

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

ATOM RegisterClassExA(const WNDCLASSEXA* window_class);
HCURSOR LoadCursorA(HINSTANCE instance, LPCSTR cursor_name);
BOOL IsWindow(HWND window);
BOOL IsChild(HWND parent, HWND window);
HWND GetActiveWindow(void);
BOOL AdjustWindowRectEx(LPRECT rectangle, DWORD style, BOOL menu,
                        DWORD extended_style);
BOOL GetWindowRect(HWND window, LPRECT rectangle);
BOOL GetClientRect(HWND window, LPRECT rectangle);
BOOL ClientToScreen(HWND window, LPPOINT point);
BOOL ScreenToClient(HWND window, LPPOINT point);
BOOL InvalidateRect(HWND window, const RECT* rectangle, BOOL erase);
BOOL SystemParametersInfoA(UINT action, UINT parameter, LPVOID data, UINT flags);
BOOL ShowWindow(HWND window, int command_show);
BOOL EnableWindow(HWND window, BOOL enable);
BOOL IsWindowEnabled(HWND window);
BOOL UpdateWindow(HWND window);
HWND SetActiveWindow(HWND window);
HWND SetFocus(HWND window);
HWND SetCapture(HWND window);
BOOL ReleaseCapture(void);
HWND GetWindow(HWND window, UINT command);
HWND GetParent(HWND window);
int GetSystemMetrics(int index);
BOOL SetWindowPos(HWND window, HWND insert_after, int x, int y, int cx, int cy,
                  UINT flags);
LONG_PTR GetWindowLongPtrA(HWND window, int index);
LONG_PTR SetWindowLongPtrA(HWND window, int index, LONG_PTR new_long);
LONG GetWindowLongA(HWND window, int index);
LONG SetWindowLongA(HWND window, int index, LONG new_long);
LRESULT DefWindowProcA(HWND window, UINT message, WPARAM wparam,
                       LPARAM lparam);
BOOL DestroyWindow(HWND window);
DWORD GetSysColor(int index);
HWND CreateWindowExA(DWORD extended_style, LPCSTR class_name,
                     LPCSTR window_name, DWORD style, int x, int y, int width,
                     int height, HWND parent, HMENU menu, HINSTANCE instance,
                     LPVOID parameter);
LRESULT SendMessageA(HWND window, UINT message, WPARAM wparam, LPARAM lparam);
BOOL PostMessageW(HWND window, UINT message, WPARAM wparam, LPARAM lparam);
BOOL TranslateMessage(const MSG* message);
LRESULT DispatchMessageA(const MSG* message);
BOOL PeekMessageA(LPMSG message, HWND window, UINT filter_min, UINT filter_max,
                  UINT remove_message);
BOOL GetMessageA(LPMSG message, HWND window, UINT filter_min, UINT filter_max);
BOOL IsDialogMessageA(HWND dialog, LPMSG message);
VOID PostQuitMessage(int exit_code);
HGDIOBJ GetStockObject(int object);
HGDIOBJ SelectObject(HDC device_context, HGDIOBJ object);
HFONT CreateFontIndirectA(const LOGFONTA* logical_font);
BOOL GetTextMetricsA(HDC device_context, LPTEXTMETRICA text_metric);
HBITMAP CreateBitmapIndirect(const BITMAP* bitmap);
HBITMAP CreateBitmap(int width, int height, UINT planes, UINT bits_per_pixel,
                     LPCVOID bits);
HDC CreateCompatibleDC(HDC device_context);
HRGN CreateRectRgn(int left, int top, int right, int bottom);
int GetObjectA(HANDLE object, int buffer_size, LPVOID object_data);
LONG GetBitmapBits(HBITMAP bitmap, LONG count, LPVOID bits);
DWORD GetBitmapDimension(HBITMAP bitmap);
int GetClassNameA(HWND window, LPSTR class_name, int max_count);
BOOL SetWindowTextA(HWND window, LPCSTR text);
int GetWindowTextLengthA(HWND window);
int GetWindowTextA(HWND window, LPSTR text, int max_count);
int GetWindowTextLengthW(HWND window);
int GetWindowTextW(HWND window, LPWSTR text, int max_count);
BOOL GetComboBoxInfo(HWND combo_box, PCOMBOBOXINFO combo_box_info);
int WideCharToMultiByte(UINT code_page, DWORD flags, LPCWSTR wide_char,
                        int wide_char_count, LPSTR multi_byte, int multi_byte_count,
                        LPCSTR default_char, BOOL* used_default_char);
HDC GetDC(HWND window);
int ReleaseDC(HWND window, HDC device_context);
int EnumFontFamiliesExA(HDC device_context, LPLOGFONTA logfont,
                        FONTENUMPROCA enum_font_proc, LPARAM parameter,
                        DWORD flags);
SHORT GetKeyState(int virtual_key);
BOOL PatBlt(HDC device_context, int x, int y, int width, int height,
            DWORD raster_operation);
BOOL SetCursorPos(int x, int y);
HWND FindWindowA(LPCSTR class_name, LPCSTR window_name);
BOOL IsIconic(HWND window);
BOOL OpenIcon(HWND window);
BOOL IsClipboardFormatAvailable(UINT format);
HFILE OpenFile(LPSTR file_name, LPOFSTRUCT reopen_buffer, UINT style);
HDC BeginPaint(HWND window, LPPAINTSTRUCT paint);
VOID EndPaint(HWND window, LPPAINTSTRUCT paint);
HWND GetClipboardOwner(void);
BOOL OpenClipboard(HWND window);
BOOL EmptyClipboard(void);
HANDLE GetClipboardData(UINT format);
HANDLE SetClipboardData(UINT format, HANDLE memory);
BOOL CloseClipboard(void);
int SetMapMode(HDC device_context, int map_mode);
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
#define PostMessage PostMessageW
#endif
#ifndef FindWindow
#define FindWindow FindWindowA
#endif
#ifndef CreateFontIndirect
#define CreateFontIndirect CreateFontIndirectA
#endif
#ifndef GetTextMetrics
#define GetTextMetrics GetTextMetricsA
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
