#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void OpusUser32PushScriptedInput(HWND window, UINT message, WPARAM wparam,
                                 LPARAM lparam);
void OpusUser32ExpectScriptedChar(UINT character, UINT count);
BOOL OpusUser32ScriptedCharMatched(void);

#ifdef __cplusplus
}
#endif
