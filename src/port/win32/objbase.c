#include <objbase.h>

#include <stdint.h>
#include <time.h>

HRESULT CoCreateGuid(GUID* guid) {
    static uintptr_t counter;
    if (guid == 0) return E_FAIL;
    const uintptr_t seed = (uintptr_t)guid ^ (uintptr_t)time(0) ^ ++counter;
    int index;
    guid->Data1 = (DWORD)seed;
    guid->Data2 = (WORD)(seed >> 16);
    guid->Data3 = (WORD)(seed >> 32);
    for (index = 0; index < 8; ++index)
        guid->Data4[index] = (BYTE)(seed >> ((index % sizeof(seed)) * 8));
    guid->Data4[6] = (BYTE)((guid->Data4[6] & 0x0f) | 0x40);
    guid->Data4[8 - 1] = (BYTE)((guid->Data4[8 - 1] & 0x3f) | 0x80);
    return S_OK;
}
