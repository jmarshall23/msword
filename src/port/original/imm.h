#pragma once

#include <Windows.h>

typedef HANDLE HIMC;

#ifndef GCS_RESULTSTR
#define GCS_RESULTSTR 0x0800
#endif

HIMC ImmGetContext(HWND window);
LONG ImmGetCompositionStringW(HIMC context, DWORD index, LPVOID buffer,
                              DWORD buffer_length);
BOOL ImmReleaseContext(HWND window, HIMC context);
