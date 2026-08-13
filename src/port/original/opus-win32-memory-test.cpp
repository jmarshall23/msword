#include "windows.h"

#include <cstring>

int main() {
    HANDLE memory = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, 16);
    if (memory == nullptr || GlobalSize(memory) != 16) {
        return 1;
    }

    auto* bytes = static_cast<unsigned char*>(GlobalLock(memory));
    if (bytes == nullptr) {
        return 2;
    }
    for (int index = 0; index < 16; ++index) {
        if (bytes[index] != 0) {
            return 3;
        }
    }
    std::memcpy(bytes, "word", 5);
    if (HIWORD(GlobalHandle(bytes)) == 0) {
        return 4;
    }
    if (GlobalUnlock(memory) != FALSE || GetLastError() != ERROR_SUCCESS) {
        return 5;
    }
    if (GlobalUnlock(memory) != FALSE || GetLastError() != ERROR_SUCCESS) {
        return 6;
    }

    memory = GlobalReAlloc(memory, 32, GMEM_MOVEABLE | GMEM_ZEROINIT);
    if (memory == nullptr || GlobalSize(memory) != 32) {
        return 7;
    }
    bytes = static_cast<unsigned char*>(GlobalLock(memory));
    if (bytes == nullptr || std::memcmp(bytes, "word", 5) != 0) {
        return 8;
    }
    for (int index = 16; index < 32; ++index) {
        if (bytes[index] != 0) {
            return 9;
        }
    }
    GlobalUnlock(memory);

    if (GlobalFree(memory) != nullptr) {
        return 10;
    }
    if (GlobalFree(memory) == nullptr || GetLastError() != ERROR_INVALID_HANDLE) {
        return 11;
    }
    return 0;
}
