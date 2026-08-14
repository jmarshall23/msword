#include "windows.h"

#include <stdio.h>
#include <string.h>

#define PATTERN "OpusGlobalHandleRoundTrip"
#define PATTERN_LEN ((SIZE_T)sizeof(PATTERN))

static int fail(const char* message) {
    fprintf(stderr, "handle-check: %s\n", message);
    return 1;
}

int main(void) {
    HANDLE memory = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, PATTERN_LEN);
    if (memory == NULL) return fail("GlobalAlloc returned NULL");
    if (GlobalSize(memory) != PATTERN_LEN) return fail("GlobalSize mismatch");

    char* bytes = (char*)GlobalLock(memory);
    if (bytes == NULL) return fail("GlobalLock returned NULL");
    memcpy(bytes, PATTERN, PATTERN_LEN);

    if (GlobalUnlock(memory) != FALSE || GetLastError() != ERROR_SUCCESS) {
        return fail("GlobalUnlock did not report final unlock");
    }

    bytes = (char*)GlobalLock(memory);
    if (bytes == NULL) return fail("second GlobalLock returned NULL");
    if (memcmp(bytes, PATTERN, PATTERN_LEN) != 0) {
        return fail("pattern did not survive unlock and relock");
    }

    DWORD token = GlobalHandle(bytes);
    if (HIWORD(token) == 0) return fail("GlobalHandle did not find the pointer");

    GlobalUnlock(memory);

    if (GlobalFree(memory) != NULL) return fail("GlobalFree did not free the handle");
    if (GlobalLock(memory) != NULL || GetLastError() != ERROR_INVALID_HANDLE) {
        return fail("GlobalLock after GlobalFree did not fail cleanly");
    }
    if (GlobalFree(memory) == NULL || GetLastError() != ERROR_INVALID_HANDLE) {
        return fail("second GlobalFree did not report ERROR_INVALID_HANDLE");
    }

    printf("handle-check: OK\n");
    return 0;
}
