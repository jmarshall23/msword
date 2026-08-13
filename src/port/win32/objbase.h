#pragma once

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef LONG HRESULT;

typedef struct _GUID {
    DWORD Data1;
    WORD Data2;
    WORD Data3;
    BYTE Data4[8];
} GUID;

#ifndef S_OK
#define S_OK ((HRESULT)0)
#endif
#ifndef E_FAIL
#define E_FAIL ((HRESULT)0x80004005L)
#endif

#ifndef SUCCEEDED
#define SUCCEEDED(result) (((HRESULT)(result)) >= 0)
#endif
#ifndef FAILED
#define FAILED(result) (((HRESULT)(result)) < 0)
#endif

HRESULT CoCreateGuid(GUID* guid);

#ifdef __cplusplus
}
#endif
