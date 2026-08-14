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

    struct {
        const char* face;
        int bold;
        int italic;
    } cases[] = {
        {"Liberation Serif", 0, 0}, {"Liberation Serif", 1, 0},
        {"Liberation Serif", 0, 1}, {"Liberation Serif", 1, 1},
        {"Script", 0, 1},           {"Modern", 1, 0},
        {"Helv", 0, 1},             {"Tms Rmn", 0, 1},
    };

    for (unsigned index = 0; index < sizeof(cases) / sizeof(cases[0]); ++index) {
        LOGFONTA logical;
        memset(&logical, 0, sizeof(logical));
        logical.lfHeight = -MulDiv(14, GetDeviceCaps(hdc, LOGPIXELSY), 72);
        logical.lfWeight = cases[index].bold ? FW_BOLD : FW_NORMAL;
        logical.lfItalic = (BYTE)cases[index].italic;
        logical.lfCharSet = ANSI_CHARSET;
        lstrcpynA(logical.lfFaceName, cases[index].face, LF_FACESIZE);

        HFONT font = CreateFontIndirectA(&logical);
        HFONT old_font = SelectObject(hdc, font);

        TEXTMETRICA metric;
        GetTextMetricsA(hdc, &metric);
        char actual_face[LF_FACESIZE];
        GetTextFaceA(hdc, LF_FACESIZE, actual_face);

        printf("GDI synth face=%-17s actual=%-20s bold=%d italic=%d overhang=%d tmItalic=%d tmWeight=%d\n",
               cases[index].face, actual_face, cases[index].bold,
               cases[index].italic, (int)metric.tmOverhang, metric.tmItalic,
               (int)metric.tmWeight);

        SelectObject(hdc, old_font);
        DeleteObject(font);
    }

    DeleteDC(hdc);
    return 0;
}
