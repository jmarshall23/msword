/*
 * Resolves the four startup font names Word loads by default.
 */
#include <windows.h>

#include <stdio.h>
#include <string.h>

int WINAPI GetTextFaceA(HDC hdc, int count, LPSTR face);

int main(void) {
    HDC hdc = CreateDCA("DISPLAY", NULL, NULL, NULL);
    if (hdc == NULL) {
        printf("ERR no DC\n");
        return 1;
    }

    const char* names[] = {"Tms Rmn", "Symbol", "Helv", "Courier"};
    const BYTE pitch_and_family[] = {
        FF_ROMAN | VARIABLE_PITCH,
        FF_DECORATIVE | DEFAULT_PITCH,
        FF_SWISS | VARIABLE_PITCH,
        FF_MODERN | FIXED_PITCH,
    };

    for (unsigned index = 0; index < sizeof(names) / sizeof(names[0]); ++index) {
        LOGFONTA logical;
        memset(&logical, 0, sizeof(logical));
        logical.lfHeight = -MulDiv(14, GetDeviceCaps(hdc, LOGPIXELSY), 72);
        logical.lfWeight = FW_NORMAL;
        logical.lfItalic = 0;
        logical.lfCharSet = ANSI_CHARSET;
        logical.lfPitchAndFamily = pitch_and_family[index];
        lstrcpynA(logical.lfFaceName, names[index], LF_FACESIZE);

        HFONT font = CreateFontIndirectA(&logical);
        HFONT old_font = SelectObject(hdc, font);

        TEXTMETRICA metric;
        GetTextMetricsA(hdc, &metric);
        char actual_face[LF_FACESIZE];
        GetTextFaceA(hdc, LF_FACESIZE, actual_face);

        printf("GDI font-substitution requested=%-10s actual=%-24s charset=%u overhang=%d\n",
               names[index], actual_face, (unsigned)metric.tmCharSet,
               (int)metric.tmOverhang);

        SelectObject(hdc, old_font);
        DeleteObject(font);
    }

    DeleteDC(hdc);
    return 0;
}
