#pragma once

#include <windows.h>

#ifndef SW_SHOWNORMAL
#define SW_SHOWNORMAL 1
#endif

#ifdef __cplusplus
extern "C" {
#endif

HINSTANCE ShellExecuteA(HWND hwnd, LPCSTR operation, LPCSTR file,
                        LPCSTR parameters, LPCSTR directory, int show_command);

#ifdef __cplusplus
}
#endif
