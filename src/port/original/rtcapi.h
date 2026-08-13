#pragma once

#include <Windows.h>

#ifdef __cplusplus
extern "C" {
#endif
typedef int(__cdecl* _RTC_error_fnW)(int, LPCWSTR, int, LPCWSTR, LPCWSTR, ...);
_RTC_error_fnW _RTC_SetErrorFuncW(_RTC_error_fnW function);
#ifdef __cplusplus
}
#endif
