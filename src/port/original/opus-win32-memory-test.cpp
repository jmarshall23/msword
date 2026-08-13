#include "windows.h"

#include <cstring>

int main() {
    WCHAR wide[16]{};
    char narrow[16]{};
    BOOL used_default = FALSE;
    const char utf8[] = {'h', static_cast<char>(0xc3), static_cast<char>(0xa9), 0};
    const WCHAR wide_utf8[] = {'h', 0x00e9, 0};
    const WCHAR wide_unmapped[] = {'A', 0x20ac, 0};
    if (MultiByteToWideChar(CP_ACP, 0, "word", -1, nullptr, 0) != 5 ||
        MultiByteToWideChar(CP_ACP, 0, "word", -1, wide, 4) != 0 ||
        GetLastError() != ERROR_INSUFFICIENT_BUFFER ||
        MultiByteToWideChar(CP_ACP, 0, "word", -1, wide, 5) != 5 ||
        wide[0] != 'w' || wide[4] != 0 ||
        MultiByteToWideChar(CP_ACP, 0, "word", 2, wide, 16) != 2 ||
        wide[0] != 'w' || wide[1] != 'o' ||
        MultiByteToWideChar(CP_UTF8, 0, utf8, -1, wide, 16) != 3 ||
        wide[0] != 'h' || wide[1] != 0x00e9 || wide[2] != 0) {
        return 1;
    }

    if (WideCharToMultiByte(CP_ACP, 0, wide_utf8, -1, nullptr, 0, nullptr,
                            nullptr) != 3 ||
        WideCharToMultiByte(CP_ACP, 0, wide_utf8, -1, narrow, 2, nullptr,
                            nullptr) != 0 ||
        GetLastError() != ERROR_INSUFFICIENT_BUFFER ||
        WideCharToMultiByte(CP_UTF8, 0, wide_utf8, -1, narrow, 16, nullptr,
                            nullptr) != 4 ||
        narrow[0] != 'h' || static_cast<unsigned char>(narrow[1]) != 0xc3 ||
        static_cast<unsigned char>(narrow[2]) != 0xa9 || narrow[3] != 0 ||
        WideCharToMultiByte(1252, WC_NO_BEST_FIT_CHARS, wide_unmapped, -1,
                            narrow, 16, "!", &used_default) != 3 ||
        narrow[0] != 'A' || narrow[1] != '!' || narrow[2] != 0 ||
        used_default != TRUE ||
        WideCharToMultiByte(9999, 0, wide_utf8, -1, narrow, 16, nullptr,
                            nullptr) != 0 ||
        GetLastError() != ERROR_INVALID_PARAMETER) {
        return 2;
    }

    HANDLE memory = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, 16);
    if (memory == nullptr || GlobalSize(memory) != 16) {
        return 3;
    }

    auto* bytes = static_cast<unsigned char*>(GlobalLock(memory));
    if (bytes == nullptr) {
        return 4;
    }
    for (int index = 0; index < 16; ++index) {
        if (bytes[index] != 0) {
            return 5;
        }
    }
    std::memcpy(bytes, "word", 5);
    if (HIWORD(GlobalHandle(bytes)) == 0) {
        return 6;
    }
    if (GlobalUnlock(memory) != FALSE || GetLastError() != ERROR_SUCCESS) {
        return 7;
    }
    if (GlobalUnlock(memory) != FALSE || GetLastError() != ERROR_SUCCESS) {
        return 8;
    }

    memory = GlobalReAlloc(memory, 32, GMEM_MOVEABLE | GMEM_ZEROINIT);
    if (memory == nullptr || GlobalSize(memory) != 32) {
        return 9;
    }
    bytes = static_cast<unsigned char*>(GlobalLock(memory));
    if (bytes == nullptr || std::memcmp(bytes, "word", 5) != 0) {
        return 10;
    }
    for (int index = 16; index < 32; ++index) {
        if (bytes[index] != 0) {
            return 11;
        }
    }
    GlobalUnlock(memory);

    if (GlobalFree(memory) != nullptr) {
        return 12;
    }
    if (GlobalFree(memory) == nullptr || GetLastError() != ERROR_INVALID_HANDLE) {
        return 13;
    }
    return 0;
}
