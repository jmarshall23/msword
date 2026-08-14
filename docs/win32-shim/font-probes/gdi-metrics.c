/*
 * Win32 oracle for the text metric shape Word uses:
 * dxwChar = GetTextExtent(hdc, &ch, 1) - tm.tmOverhang.
 *
 * GetTextExtentPoint32A is the Win32 spelling available to this standalone probe.
 */
#include <windows.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef OUT_TT_PRECIS
#define OUT_TT_PRECIS 4
#endif

int WINAPI GetTextFaceA(HDC hdc, int count, LPSTR face);

static void dump_metrics(const char* face, int points) {
    HDC hdc = CreateDCA("DISPLAY", NULL, NULL, NULL);
    if (hdc == NULL) {
        printf("ERR no DC\n");
        return;
    }

    int dpi_x = GetDeviceCaps(hdc, LOGPIXELSX);
    int dpi_y = GetDeviceCaps(hdc, LOGPIXELSY);

    LOGFONTA logical;
    memset(&logical, 0, sizeof(logical));
    logical.lfHeight = -MulDiv(points, dpi_y, 72);
    logical.lfCharSet = ANSI_CHARSET;
    logical.lfOutPrecision = OUT_TT_PRECIS;
    lstrcpynA(logical.lfFaceName, face, LF_FACESIZE);

    HFONT font = CreateFontIndirectA(&logical);
    HFONT old_font = SelectObject(hdc, font);

    TEXTMETRICA metric;
    GetTextMetricsA(hdc, &metric);
    char actual_face[LF_FACESIZE];
    GetTextFaceA(hdc, LF_FACESIZE, actual_face);

    printf("GDI face=%s actual=%s pt=%d dpi=%dx%d\n", face, actual_face, points,
           dpi_x, dpi_y);
    printf("GDI tm height=%d ascent=%d descent=%d overhang=%d avgw=%d maxw=%d\n",
           (int)metric.tmHeight, (int)metric.tmAscent, (int)metric.tmDescent,
           (int)metric.tmOverhang, (int)metric.tmAveCharWidth,
           (int)metric.tmMaxCharWidth);

    for (int ch = 32; ch < 127; ++ch) {
        char text = (char)ch;
        SIZE size;
        GetTextExtentPoint32A(hdc, &text, 1, &size);
        printf("GDI adv %d %d\n", ch, (int)(size.cx - metric.tmOverhang));
    }

    SelectObject(hdc, old_font);
    DeleteObject(font);
    DeleteDC(hdc);
}

int main(int argc, char** argv) {
    const char* face = argc > 1 ? argv[1] : "Liberation Serif";
    int points = argc > 2 ? atoi(argv[2]) : 10;
    dump_metrics(face, points);
    return 0;
}
