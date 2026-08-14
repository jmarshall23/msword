#include "windows.h"

int main(void) {
    HDC dc = CreateCompatibleDC(NULL);
    if (dc == NULL) return 1;

    if (Escape(dc, STARTDOC, 4, (LPSTR)"Test", NULL) != SP_ERROR) {
        return 2;
    }

    int escape = BANDINFO;
    POINT point = {123, 456};
    RECT band = {1, 2, 3, 4};
    if (Escape(dc, QUERYESCSUPPORT, sizeof(escape), (LPSTR)&escape, NULL) != 0 ||
        Escape(dc, GETPHYSPAGESIZE, 0, NULL, (LPSTR)&point) != 0 ||
        point.x != 123 || point.y != 456 ||
        Escape(dc, NEXTBAND, 0, NULL, (LPSTR)&band) != 0 || band.left != 0 ||
        band.top != 0 || band.right != 0 || band.bottom != 0 ||
        Escape(dc, SETABORTPROC, 0, NULL, NULL) != 0 ||
        Escape(dc, ENDDOC, 0, NULL, NULL) != 0 ||
        Escape(dc, ABORTDOC, 0, NULL, NULL) != 0 ||
        Escape(dc, DRAFTMODE, 0, NULL, NULL) != 0) {
        return 3;
    }

    DeleteDC(dc);
    return 0;
}
