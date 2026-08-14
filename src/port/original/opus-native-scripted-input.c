#include "windows.h"

#include "../win32/opusinputscript.h"

static UINT g_expected_scripted_char;
static UINT g_expected_scripted_char_count;
static BOOL g_matched_scripted_char = TRUE;

static UINT lower_ascii(UINT value) {
    return value >= 'A' && value <= 'Z' ? value + ('a' - 'A') : value;
}

void OpusUser32PushScriptedInput(HWND window, UINT message, WPARAM wparam,
                                 LPARAM lparam) {
    if (message == WM_KEYDOWN && g_expected_scripted_char_count > 0 &&
        lower_ascii((UINT)wparam) == g_expected_scripted_char) {
        --g_expected_scripted_char_count;
        g_matched_scripted_char = g_expected_scripted_char_count == 0;
    }
    if (message == WM_QUIT) {
        PostQuitMessage((int)wparam);
    } else if (window != NULL) {
        PostMessageA(window, message, wparam, lparam);
    } else {
        PostThreadMessageA(GetCurrentThreadId(), message, wparam, lparam);
    }
}

void OpusUser32ExpectScriptedChar(UINT character, UINT count) {
    g_expected_scripted_char = character;
    g_expected_scripted_char_count = count;
    g_matched_scripted_char = count == 0;
}

BOOL OpusUser32ScriptedCharMatched(void) {
    return g_matched_scripted_char;
}
