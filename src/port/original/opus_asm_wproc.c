#define WIN32_LEAN_AND_MEAN
#define NODDE
#include <windows.h>

#include <stddef.h>

/*
 * AMD64 translation of Opus/asm/wprocn.asm.  The assembly was not a simple
 * forwarding thunk: it admitted only a table of messages to each original C
 * procedure and sent every other message directly to DefWindowProc.  That
 * distinction is required on Win64, notably for WM_NCCREATE.
 */
LRESULT CALLBACK AppWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK MwdWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK WwPaneWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK DdeChnlWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK RSBWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK FedtWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK StaticEditWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK StartWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK DeskTopWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK PgPrvwWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK SplitBarWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK StatLineWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK IconBarWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK RulerMarkWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK PromptWndProc(HWND, UINT, WPARAM, LPARAM);

extern HCURSOR vhcIBeam;
extern HCURSOR vhcArrow;
int OurSetCursor(HCURSOR cursor);
void OpusDrawWin95HorizontalRuler(HWND ruler);

#define ARRAY_COUNT(array) (sizeof(array) / sizeof((array)[0]))

enum {
    kWmSystemError = 0x0017,
    kWmSetVisible = 0x0009,
    kWmCbtInit = 0x03f0,
    kWmCbtTerm = 0x03f1,
    kWmCbtSemEv = 0x03f3,
    kWmDdeInitiate = 0x03e0,
    kWmDdeTerminate = 0x03e1,
    kWmDdeAdvise = 0x03e2,
    kWmDdeUnadvise = 0x03e3,
    kWmDdeAck = 0x03e4,
    kWmDdeData = 0x03e5,
    kWmDdeRequest = 0x03e6,
    kWmDdePoke = 0x03e7,
    kWmDdeExecute = 0x03e8,
    kWmOpusX64QuerySelection = WM_APP + 0x351
};

enum MouseCursor { MouseCursorNone, MouseCursorIBeam, MouseCursorArrow };

static int Contains(const UINT* messages, size_t count, const UINT message) {
    size_t index;
    for (index = 0; index < count; ++index) {
        if (messages[index] == message) {
            return TRUE;
        }
    }
    return FALSE;
}

static LRESULT Dispatch(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam,
                        WNDPROC source, const UINT* messages,
                        size_t message_count, enum MouseCursor mouse_cursor) {
    if (Contains(messages, message_count, message)) {
        return source(hwnd, message, wParam, lParam);
    }
    if (message == WM_MOUSEMOVE && mouse_cursor != MouseCursorNone) {
        OurSetCursor(mouse_cursor == MouseCursorIBeam ? vhcIBeam : vhcArrow);
        return FALSE;
    }
    return DefWindowProcA(hwnd, message, wParam, lParam);
}

static const UINT kAppMessages[] = {
    WM_CREATE, WM_INITMENUPOPUP, WM_MENUCHAR, WM_ENTERIDLE, WM_ACTIVATE,
    WM_ACTIVATEAPP, WM_MOUSEACTIVATE, WM_SETCURSOR, WM_TIMER, WM_CLOSE,
    WM_QUERYENDSESSION, WM_ENDSESSION, WM_DESTROY, WM_SIZE, WM_MOVE,
    WM_COMMAND, WM_SYSCOMMAND, WM_SYSCOLORCHANGE, WM_WININICHANGE,
    WM_DEVMODECHANGE, WM_DESTROYCLIPBOARD, WM_RENDERFORMAT,
    WM_PAINTCLIPBOARD, WM_VSCROLLCLIPBOARD, WM_HSCROLLCLIPBOARD,
    WM_SIZECLIPBOARD, WM_ASKCBFORMATNAME, WM_FONTCHANGE, kWmDdeInitiate,
    WM_MENUSELECT, WM_SYSKEYDOWN, kWmCbtInit, kWmCbtTerm, kWmCbtSemEv,
    kWmSystemError, WM_NCLBUTTONDBLCLK};
static const UINT kMwdMessages[] = {
    WM_CREATE, WM_MOVE, WM_SIZE, WM_CLOSE, WM_GETMINMAXINFO, WM_VSCROLL,
    WM_HSCROLL, WM_MOUSEACTIVATE, WM_CHILDACTIVATE, WM_MENUCHAR,
    WM_SYSCOMMAND, WM_INITMENUPOPUP, WM_MENUSELECT, WM_ENTERIDLE};
static const UINT kWwPaneMessages[] = {
    WM_CREATE, WM_SIZE, WM_SETFOCUS, WM_KILLFOCUS, WM_PAINT,
    WM_MOUSEACTIVATE, WM_SETCURSOR, WM_MOUSEMOVE, WM_LBUTTONDOWN,
    WM_LBUTTONDBLCLK, WM_RBUTTONDOWN, WM_SYSCOMMAND, WM_MENUCHAR,
    kWmOpusX64QuerySelection};
static const UINT kDdeMessages[] = {
    kWmDdeTerminate, kWmDdeAdvise, kWmDdeUnadvise, kWmDdeAck,
    kWmDdeData, kWmDdeRequest, kWmDdePoke, kWmDdeExecute};
static const UINT kRsbMessages[] = {WM_NCCREATE, WM_CREATE, WM_ERASEBKGND,
                                  WM_PAINT, WM_LBUTTONDOWN,
                                  WM_LBUTTONDBLCLK, WM_SETCURSOR};
static const UINT kFedtMessages[] = {
    WM_NCCREATE, WM_DESTROY, WM_SIZE, WM_SETFOCUS, WM_KILLFOCUS, WM_KEYDOWN,
    WM_CHAR, WM_KEYUP, WM_LBUTTONDBLCLK, WM_LBUTTONDOWN, WM_SETREDRAW,
    WM_ERASEBKGND, WM_PAINT, WM_GETTEXTLENGTH, WM_GETTEXT, WM_SETTEXT,
    EM_SETHANDLE, WM_GETDLGCODE, WM_COPY, WM_CUT, WM_CLEAR, WM_PASTE,
    EM_SETSEL, EM_GETSEL, EM_GETLINECOUNT, EM_REPLACESEL, EM_GETHANDLE};
static const UINT kStaticEditMessages[] = {
    WM_CREATE, WM_DESTROY, WM_PAINT, WM_SETFOCUS, WM_KILLFOCUS, WM_COMMAND,
    WM_GETDLGCODE, WM_SETCURSOR, WM_KEYDOWN, WM_MOUSEMOVE, WM_LBUTTONDOWN,
    WM_LBUTTONUP, WM_ENABLE};
static const UINT kStartMessages[] = {WM_NCPAINT, WM_DESTROY};
static const UINT kDesktopMessages[] = {WM_SIZE};
static const UINT kPreviewMessages[] = {
    WM_MOUSEMOVE, WM_CREATE, WM_SIZE, WM_ERASEBKGND, WM_PAINT, WM_VSCROLL,
    WM_LBUTTONDBLCLK, WM_LBUTTONDOWN, WM_CLOSE, WM_SYSCOMMAND};
static const UINT kSplitBarMessages[] = {WM_PAINT};
static const UINT kStatLineMessages[] = {WM_CREATE, WM_LBUTTONDBLCLK, WM_PAINT,
                                       WM_LBUTTONDOWN};
static const UINT kIconBarMessages[] = {
    WM_NCDESTROY, WM_LBUTTONDOWN, WM_LBUTTONDBLCLK, WM_SYSCOMMAND,
    WM_SYSKEYDOWN, WM_KEYDOWN, WM_PAINT, kWmSetVisible, WM_MOVE, WM_SIZE,
    WM_ENABLE};
static const UINT kRulerMarkMessages[] = {WM_LBUTTONDBLCLK, WM_CREATE, WM_PAINT,
                                        WM_LBUTTONDOWN};
static const UINT kPromptMessages[] = {
    WM_SYSCOMMAND, WM_CREATE, WM_PAINT, WM_DESTROY, WM_TIMER, WM_KEYDOWN,
    WM_CHAR, WM_SETFOCUS, WM_SIZE, WM_KILLFOCUS};

LRESULT CALLBACK NatAppWndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    if (m == kWmDdeInitiate) {
        /* The legacy DDE server exposes document contents and macro commands
         * to any process in the desktop session.  The modern port does not
         * accept inbound DDE conversations. */
        return 0;
    }
    return Dispatch(h, m, w, l, AppWndProc, kAppMessages,
                    ARRAY_COUNT(kAppMessages), MouseCursorNone);
}
LRESULT CALLBACK NatMwdWndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    return Dispatch(h, m, w, l, MwdWndProc, kMwdMessages,
                    ARRAY_COUNT(kMwdMessages), MouseCursorNone);
}
LRESULT CALLBACK NatWwPaneWndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    return Dispatch(h, m, w, l, WwPaneWndProc, kWwPaneMessages,
                    ARRAY_COUNT(kWwPaneMessages), MouseCursorNone);
}
LRESULT CALLBACK NatDdeChnlWndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    return Dispatch(h, m, w, l, DdeChnlWndProc, kDdeMessages,
                    ARRAY_COUNT(kDdeMessages), MouseCursorNone);
}
LRESULT CALLBACK NatRSBWndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    return Dispatch(h, m, w, l, RSBWndProc, kRsbMessages,
                    ARRAY_COUNT(kRsbMessages), MouseCursorNone);
}
LRESULT CALLBACK NatFedtWndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    return Dispatch(h, m, w, l, FedtWndProc, kFedtMessages,
                    ARRAY_COUNT(kFedtMessages), MouseCursorIBeam);
}
LRESULT CALLBACK NatStaticEditWndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    return Dispatch(h, m, w, l, StaticEditWndProc, kStaticEditMessages,
                    ARRAY_COUNT(kStaticEditMessages), MouseCursorIBeam);
}
LRESULT CALLBACK NatStartWndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    return Dispatch(h, m, w, l, StartWndProc, kStartMessages,
                    ARRAY_COUNT(kStartMessages), MouseCursorArrow);
}
LRESULT CALLBACK NatDeskTopWndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    return Dispatch(h, m, w, l, DeskTopWndProc, kDesktopMessages,
                    ARRAY_COUNT(kDesktopMessages), MouseCursorArrow);
}
LRESULT CALLBACK NatPgPrvwWndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    return Dispatch(h, m, w, l, PgPrvwWndProc, kPreviewMessages,
                    ARRAY_COUNT(kPreviewMessages), MouseCursorArrow);
}
LRESULT CALLBACK NatSplitBarWndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    return Dispatch(h, m, w, l, SplitBarWndProc, kSplitBarMessages,
                    ARRAY_COUNT(kSplitBarMessages), MouseCursorArrow);
}
LRESULT CALLBACK NatStatLineWndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    return Dispatch(h, m, w, l, StatLineWndProc, kStatLineMessages,
                    ARRAY_COUNT(kStatLineMessages), MouseCursorArrow);
}
LRESULT CALLBACK NatIconBarWndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    return Dispatch(h, m, w, l, IconBarWndProc, kIconBarMessages,
                    ARRAY_COUNT(kIconBarMessages), MouseCursorArrow);
}
LRESULT CALLBACK NatRulerMarkWndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    LRESULT result = Dispatch(h, m, w, l, RulerMarkWndProc,
                              kRulerMarkMessages,
                              ARRAY_COUNT(kRulerMarkMessages),
                              MouseCursorArrow);
    if (m == WM_PAINT) {
        OpusDrawWin95HorizontalRuler(h);
    }
    return result;
}
LRESULT CALLBACK NatPromptWndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    return Dispatch(h, m, w, l, PromptWndProc, kPromptMessages,
                    ARRAY_COUNT(kPromptMessages), MouseCursorArrow);
}
