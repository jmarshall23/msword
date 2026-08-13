#include "windows.h"

#include <array>

bool PixelIs(HDC dc, int x, int y, COLORREF color) {
    return GetPixel(dc, x, y) == (color & 0x00ffffffu);
}

int main() {
    HDC destination = CreateCompatibleDC(nullptr);
    HBITMAP destination_bitmap = CreateCompatibleBitmap(destination, 4, 4);
    if (destination == nullptr || destination_bitmap == nullptr ||
        SelectObject(destination, destination_bitmap) != nullptr) {
        return 1;
    }

    if (!PatBlt(destination, 0, 0, 4, 4, WHITENESS) ||
        !PixelIs(destination, 3, 3, RGB(255, 255, 255))) {
        return 2;
    }
    if (!PatBlt(destination, 1, 1, 2, 2, BLACKNESS) ||
        !PixelIs(destination, 1, 1, RGB(0, 0, 0)) ||
        !PixelIs(destination, 0, 0, RGB(255, 255, 255))) {
        return 3;
    }

    HBRUSH red = CreateSolidBrush(RGB(255, 0, 0));
    HGDIOBJ old_brush = SelectObject(destination, red);
    if (!PatBlt(destination, 0, 0, 1, 1, PATCOPY) ||
        !PixelIs(destination, 0, 0, RGB(255, 0, 0))) {
        return 4;
    }
    if (!PatBlt(destination, 0, 0, 1, 1, DSTINVERT) ||
        !PixelIs(destination, 0, 0, RGB(0, 255, 255))) {
        return 5;
    }
    if (!PatBlt(destination, 0, 0, 1, 1, PATINVERT) ||
        !PixelIs(destination, 0, 0, RGB(255, 255, 255))) {
        return 6;
    }

    HDC source = CreateCompatibleDC(nullptr);
    std::array<COLORREF, 4> pixels{
        RGB(10, 20, 30), RGB(40, 50, 60),
        RGB(70, 80, 90), RGB(100, 110, 120),
    };
    HBITMAP source_bitmap = CreateBitmap(2, 2, 1, 32, pixels.data());
    if (source == nullptr || source_bitmap == nullptr ||
        SelectObject(source, source_bitmap) != nullptr) {
        return 7;
    }
    if (!BitBlt(destination, 0, 0, 2, 2, source, 0, 0, SRCCOPY) ||
        !PixelIs(destination, 1, 1, RGB(100, 110, 120))) {
        return 8;
    }
    if (!BitBlt(destination, 2, 0, 1, 1, source, 0, 0, NOTSRCCOPY) ||
        !PixelIs(destination, 2, 0, RGB(245, 235, 225))) {
        return 9;
    }
    SetPixel(destination, 2, 1, RGB(0x0f, 0xf0, 0xff));
    if (!BitBlt(destination, 2, 1, 1, 1, source, 0, 0, SRCAND) ||
        !PixelIs(destination, 2, 1, RGB(0x0a, 0x10, 0x1e))) {
        return 10;
    }
    SetPixel(destination, 3, 1, RGB(1, 2, 3));
    if (!BitBlt(destination, 3, 1, 1, 1, source, 0, 0, SRCINVERT) ||
        !PixelIs(destination, 3, 1, RGB(11, 22, 29))) {
        return 11;
    }

    if (!StretchBlt(destination, 0, 2, 4, 2, source, 0, 0, 2, 1,
                    SRCCOPY) ||
        !PixelIs(destination, 0, 2, RGB(10, 20, 30)) ||
        !PixelIs(destination, 3, 3, RGB(40, 50, 60))) {
        return 12;
    }
    if (!PatBlt(destination, -2, -2, 3, 3, BLACKNESS) ||
        !PixelIs(destination, 0, 0, RGB(0, 0, 0)) ||
        !PixelIs(destination, 1, 1, RGB(100, 110, 120))) {
        return 13;
    }

    BITMAPINFO info{};
    std::array<COLORREF, 16> out{};
    if (GetDIBits(destination, destination_bitmap, 0, 4, out.data(), &info, 0) !=
            4 ||
        info.bmiHeader.biWidth != 4 || info.bmiHeader.biHeight != 4 ||
        out[0] != RGB(0, 0, 0)) {
        return 14;
    }
    if (SetStretchBltMode(destination, COLORONCOLOR) != COLORONCOLOR ||
        SetROP2(destination, R2_NOTXORPEN) != R2_COPYPEN ||
        SetROP2(destination, R2_COPYPEN) != R2_NOTXORPEN) {
        return 15;
    }

    SelectObject(destination, old_brush);
    DeleteObject(red);
    DeleteDC(source);
    DeleteDC(destination);
    return 0;
}
