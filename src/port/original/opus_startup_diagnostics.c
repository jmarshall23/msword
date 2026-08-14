#include "opus-native-compat.h"

#include <stdio.h>
#include <string.h>

static char g_last_report[512];

void OpusX64ReportSz(const char* message, const char* file, int line) {
    _snprintf_s(g_last_report, sizeof(g_last_report), _TRUNCATE,
                "%s (%s:%d)",
                message != NULL ? message : "startup failure",
                file != NULL ? file : "unknown", line);
    OutputDebugStringA("WORD1 x64: ");
    OutputDebugStringA(g_last_report);
    OutputDebugStringA("\r\n");
}

void OpusX64ReportWin32(const char* message, DWORD error, const char* file,
                        int line) {
    char system_message[256] = {0};
    char* pch;

    FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                   NULL, error, 0, system_message,
                   (DWORD)sizeof(system_message), NULL);
    for (pch = system_message; *pch != '\0'; ++pch)
        if (*pch == '\r' || *pch == '\n') *pch = ' ';
    _snprintf_s(g_last_report, sizeof(g_last_report), _TRUNCATE,
                "%s: Win32 error %lu (%s) (%s:%d)",
                message != NULL ? message : "startup failure",
                (unsigned long)error,
                system_message[0] != '\0' ? system_message : "no system text",
                file != NULL ? file : "unknown", line);
    OutputDebugStringA("WORD1 x64: ");
    OutputDebugStringA(g_last_report);
    OutputDebugStringA("\r\n");
}

const char* OpusX64LastReport(void) {
    return g_last_report;
}

void OpusX64Trace(const char* message) {
    char module_path[MAX_PATH] = {0};
    char trace_path[MAX_PATH] = {0};
    char* file_part;
    char* bin_part;
    HANDLE file;
    DWORD written = 0;
    const char* text;

    GetModuleFileNameA(NULL, module_path, MAX_PATH);
    file_part = strrchr(module_path, '\\');
    if (file_part != NULL) {
        *file_part = '\0';
        bin_part = strrchr(module_path, '\\');
        if (bin_part != NULL) *bin_part = '\0';
    }
    _snprintf_s(trace_path, sizeof(trace_path), _TRUNCATE,
                "%s\\build\\WORD1-port-trace.txt", module_path);
    file = CreateFileA(trace_path, FILE_APPEND_DATA, FILE_SHARE_READ, NULL,
                       OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) return;
    text = message != NULL ? message : "(null)";
    WriteFile(file, text, (DWORD)strlen(text), &written, NULL);
    WriteFile(file, "\r\n", 2, &written, NULL);
    CloseHandle(file);
}
