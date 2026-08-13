#pragma once

#include "windows.h"

#ifdef __cplusplus
extern "C" {
#endif

void OpusUser32PushScriptedInput(HWND window, UINT message, WPARAM wparam,
                                 LPARAM lparam);

#ifdef __cplusplus
}
#endif
