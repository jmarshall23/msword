#include "windows.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

int main(void) {
    WCHAR wide[16] = {0};
    char narrow[16] = {0};
    BOOL used_default = FALSE;
    const char utf8[] = {'h', (char)0xc3, (char)0xa9, 0};
    const WCHAR wide_utf8[] = {'h', 0x00e9, 0};
    const WCHAR wide_unmapped[] = {'A', 0x20ac, 0};
    if (MultiByteToWideChar(CP_ACP, 0, "word", -1, NULL, 0) != 5 ||
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

    if (WideCharToMultiByte(CP_ACP, 0, wide_utf8, -1, NULL, 0, NULL, NULL) !=
            3 ||
        WideCharToMultiByte(CP_ACP, 0, wide_utf8, -1, narrow, 2, NULL, NULL) !=
            0 ||
        GetLastError() != ERROR_INSUFFICIENT_BUFFER ||
        WideCharToMultiByte(CP_UTF8, 0, wide_utf8, -1, narrow, 16, NULL,
                            NULL) != 4 ||
        narrow[0] != 'h' || (unsigned char)narrow[1] != 0xc3 ||
        (unsigned char)narrow[2] != 0xa9 || narrow[3] != 0 ||
        WideCharToMultiByte(1252, WC_NO_BEST_FIT_CHARS, wide_unmapped, -1,
                            narrow, 16, "!", &used_default) != 3 ||
        narrow[0] != 'A' || narrow[1] != '!' || narrow[2] != 0 ||
        used_default != TRUE ||
        WideCharToMultiByte(9999, 0, wide_utf8, -1, narrow, 16, NULL, NULL) !=
            0 ||
        GetLastError() != ERROR_INVALID_PARAMETER) {
        return 2;
    }

    MEMORYSTATUSEX memory_status = {0};
    memory_status.dwLength = sizeof(memory_status);
    char temporary[MAX_PATH] = {0};
    char generated[MAX_PATH] = {0};
    char explicit_name[MAX_PATH] = {0};
    FILE *generated_file = NULL;
    LPSTR (*ansi_lower)(LPSTR) = AnsiLower;
    LPSTR (*ansi_upper)(LPSTR) = AnsiUpper;
    LPSTR (*ansi_next)(LPCSTR) = AnsiNext;
    LPSTR (*ansi_prev)(LPCSTR, LPCSTR) = AnsiPrev;
    BOOL (*ansi_to_oem)(LPCSTR, LPSTR) = AnsiToOem;
    BOOL (*oem_to_ansi)(LPCSTR, LPSTR) = OemToAnsi;
    if (GetCurrentThreadId() == 0 ||
        GetProfileIntA("WinWord", "NewLook", 2) != 2 ||
        GetProfileStringA("WinWord", "Missing", "fallback", narrow,
                          sizeof(narrow)) != 8 ||
        strcmp(narrow, "fallback") != 0 ||
        !WriteProfileStringA("WinWord", "Missing", "value") ||
        ansi_lower((LPSTR)(uintptr_t)'Q') != (LPSTR)(uintptr_t)'q' ||
        ansi_upper((LPSTR)(uintptr_t)'q') != (LPSTR)(uintptr_t)'Q' ||
        strcpy(narrow, "Az") == NULL || ansi_lower(narrow) != narrow ||
        strcmp(narrow, "az") != 0 || ansi_upper(narrow) != narrow ||
        strcmp(narrow, "AZ") != 0 || ansi_next(narrow) != narrow + 1 ||
        ansi_prev(narrow, narrow + 1) != narrow ||
        !ansi_to_oem("copy", narrow) || strcmp(narrow, "copy") != 0 ||
        !oem_to_ansi("back", narrow) || strcmp(narrow, "back") != 0 ||
        GetTickCount() == 0 || !GlobalMemoryStatusEx(&memory_status) ||
        memory_status.ullTotalPhys == 0 ||
        memory_status.ullAvailPhys > memory_status.ullTotalPhys ||
        GlobalMemoryStatusEx(NULL) ||
        GetLastError() != ERROR_INVALID_PARAMETER ||
        GetTempPathA(MAX_PATH, temporary) == 0 ||
        GetTempFileNameA(temporary, "OWF", 0, generated) == 0 ||
        (generated_file = fopen(generated, "rb")) == NULL ||
        GetTempFileNameA(temporary, "OWF", 0x1234, explicit_name) != 0x1234 ||
        strstr(explicit_name, "OWF1234.TMP") == NULL) {
        return 3;
    }
    fclose(generated_file);
    remove(generated);

    HANDLE memory = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, 16);
    if (memory == NULL || GlobalSize(memory) != 16) {
        return 4;
    }

    unsigned char *bytes = (unsigned char *)GlobalLock(memory);
    if (bytes == NULL) {
        return 5;
    }
    for (int index = 0; index < 16; ++index) {
        if (bytes[index] != 0) {
            return 6;
        }
    }
    memcpy(bytes, "word", 5);
    if (HIWORD(GlobalHandle(bytes)) == 0) {
        return 7;
    }
    if (GlobalUnlock(memory) != FALSE || GetLastError() != ERROR_SUCCESS) {
        return 8;
    }
    if (GlobalUnlock(memory) != FALSE || GetLastError() != ERROR_SUCCESS) {
        return 9;
    }

    memory = GlobalReAlloc(memory, 32, GMEM_MOVEABLE | GMEM_ZEROINIT);
    if (memory == NULL || GlobalSize(memory) != 32) {
        return 10;
    }
    bytes = (unsigned char *)GlobalLock(memory);
    if (bytes == NULL || memcmp(bytes, "word", 5) != 0) {
        return 11;
    }
    for (int index = 16; index < 32; ++index) {
        if (bytes[index] != 0) {
            return 12;
        }
    }
    GlobalUnlock(memory);

    if (GlobalFree(memory) != NULL) {
        return 13;
    }
    if (GlobalFree(memory) == NULL || GetLastError() != ERROR_INVALID_HANDLE) {
        return 14;
    }
    return 0;
}
