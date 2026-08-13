#include <commdlg.h>

BOOL GetOpenFileNameA(LPOPENFILENAMEA dialog) {
    (void)dialog;
    return FALSE;
}

BOOL GetSaveFileNameA(LPOPENFILENAMEA dialog) {
    (void)dialog;
    return FALSE;
}

BOOL GetSaveFileNameW(LPOPENFILENAMEW dialog) {
    (void)dialog;
    return FALSE;
}

DWORD CommDlgExtendedError(void) {
    return 0;
}
