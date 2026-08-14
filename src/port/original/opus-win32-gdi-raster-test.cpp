#include "windows.h"

#include <array>

bool PixelIs(HDC dc, int x, int y, COLORREF color) {
    return GetPixel(dc, x, y) == (color & 0x00ffffffu);
}

int main() {
    HDC destination = CreateCompatibleDC(nullptr);
    HBITMAP destination_bitmap = CreateCompatibleBitmap(destination, 4, 4);
    if (destination == nullptr || destination_bitmap == nullptr ||
        SelectObject(destination, destination_bitmap) == nullptr) {
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
        SelectObject(source, source_bitmap) == nullptr) {
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

    if (SetBkMode(destination, TRANSPARENT) != OPAQUE ||
        SetBkMode(destination, OPAQUE) != TRANSPARENT ||
        SetTextColor(destination, RGB(1, 2, 3)) != RGB(0, 0, 0) ||
        SetTextColor(destination, RGB(4, 5, 6)) != RGB(1, 2, 3) ||
        SetMapMode(destination, MM_ANISOTROPIC) != MM_TEXT ||
        SetMapMode(destination, MM_TEXT) != MM_ANISOTROPIC) {
        return 16;
    }

    RECT text_rect{0, 0, 4, 4};
    if (!PatBlt(destination, 0, 0, 4, 4, BLACKNESS) ||
        !TextOutW(destination, 0, 0, u"A", 1) ||
        !PixelIs(destination, 0, 1, RGB(4, 5, 6)) ||
        !PixelIs(destination, 2, 2, RGB(255, 255, 255)) ||
        DrawTextW(destination, u"B", -1, &text_rect, DT_SINGLELINE) <= 0) {
        return 17;
    }

    SIZE size{};
    POINT point{};
    POINT previous_point{};
    if (!GetViewportExtEx(destination, &size) || size.cx != 1 || size.cy != 1 ||
        !SetViewportExtEx(destination, 7, 8, &size) ||
        size.cx != 1 || size.cy != 1 ||
        !GetViewportExtEx(destination, &size) || size.cx != 7 || size.cy != 8 ||
        !GetViewportOrgEx(destination, &point) || point.x != 0 || point.y != 0 ||
        !SetViewportOrgEx(destination, 2, 3, &previous_point) ||
        previous_point.x != 0 || previous_point.y != 0 ||
        !GetViewportOrgEx(destination, &point) || point.x != 2 || point.y != 3 ||
        !SetWindowExtEx(destination, 9, 10, &size) ||
        size.cx != 1 || size.cy != 1 ||
        !SetWindowOrgEx(destination, 4, 5, &previous_point) ||
        previous_point.x != 0 || previous_point.y != 0) {
        return 18;
    }

    HPEN blue = CreatePen(PS_SOLID, 1, RGB(0, 0, 255));
    HGDIOBJ old_pen = SelectObject(destination, blue);
    if (blue == nullptr || old_pen == nullptr ||
        !MoveToEx(destination, 0, 0, &previous_point) ||
        previous_point.x != 0 || previous_point.y != 0 ||
        !LineTo(destination, 4, 0) ||
        !PixelIs(destination, 0, 0, RGB(0, 0, 255)) ||
        !PixelIs(destination, 3, 0, RGB(0, 0, 255)) ||
        PixelIs(destination, 3, 1, RGB(0, 0, 255)) ||
        !MoveToEx(destination, 1, 0, &previous_point) ||
        previous_point.x != 4 || previous_point.y != 0 ||
        !LineTo(destination, 1, 4) ||
        !PixelIs(destination, 1, 0, RGB(0, 0, 255)) ||
        !PixelIs(destination, 1, 3, RGB(0, 0, 255))) {
        return 19;
    }

    HBRUSH green = CreateSolidBrush(RGB(0, 255, 0));
    RECT fill_rect{0, 0, 2, 2};
    RECT frame_rect{1, 1, 4, 4};
    if (green == nullptr ||
        !PatBlt(destination, 0, 0, 4, 4, WHITENESS) ||
        FillRect(destination, &fill_rect, green) == 0 ||
        !PixelIs(destination, 0, 0, RGB(0, 255, 0)) ||
        !PixelIs(destination, 1, 1, RGB(0, 255, 0)) ||
        !PixelIs(destination, 2, 2, RGB(255, 255, 255)) ||
        FrameRect(destination, &frame_rect, red) == 0 ||
        !PixelIs(destination, 1, 1, RGB(255, 0, 0)) ||
        !PixelIs(destination, 3, 3, RGB(255, 0, 0)) ||
        !PixelIs(destination, 2, 2, RGB(255, 255, 255)) ||
        !Rectangle(destination, 0, 0, 4, 4) ||
        !PixelIs(destination, 0, 0, RGB(0, 0, 255)) ||
        !PixelIs(destination, 3, 3, RGB(0, 0, 255)) ||
        !PixelIs(destination, 1, 1, RGB(255, 0, 0))) {
        return 20;
    }

    RECT edge_rect{0, 0, 4, 4};
    if (!PatBlt(destination, 0, 0, 4, 4, BLACKNESS) ||
        !DrawEdge(destination, &edge_rect, EDGE_RAISED, BF_RECT) ||
        !PixelIs(destination, 0, 0, RGB(255, 255, 255)) ||
        !PixelIs(destination, 3, 3, RGB(128, 128, 128)) ||
        !PixelIs(destination, 1, 1, RGB(0, 0, 0)) ||
        !PatBlt(destination, 0, 0, 4, 4, BLACKNESS) ||
        !DrawEdge(destination, &edge_rect, EDGE_SUNKEN, BF_LEFT | BF_BOTTOM) ||
        !PixelIs(destination, 0, 0, RGB(128, 128, 128)) ||
        !PixelIs(destination, 3, 3, RGB(255, 255, 255)) ||
        !PixelIs(destination, 3, 0, RGB(0, 0, 0))) {
        return 21;
    }

    POINT polygon_points[]{{0, 0}, {4, 0}, {4, 4}, {0, 4}};
    if (!PatBlt(destination, 0, 0, 4, 4, WHITENESS) ||
        !Ellipse(destination, 0, 0, 4, 4) ||
        !PixelIs(destination, 0, 1, RGB(0, 0, 255)) ||
        !PixelIs(destination, 1, 1, RGB(255, 0, 0)) ||
        !PatBlt(destination, 0, 0, 4, 4, WHITENESS) ||
        !Polygon(destination, polygon_points, 4) ||
        !PixelIs(destination, 0, 0, RGB(0, 0, 255)) ||
        !PixelIs(destination, 1, 1, RGB(255, 0, 0))) {
        return 22;
    }

    const int saved_clip = SaveDC(destination);
    if (saved_clip <= 0 ||
        !PatBlt(destination, 0, 0, 4, 4, WHITENESS) ||
        ExcludeClipRect(destination, 1, 1, 3, 3) != COMPLEXREGION ||
        !PatBlt(destination, 0, 0, 4, 4, BLACKNESS) ||
        !PixelIs(destination, 0, 0, RGB(0, 0, 0)) ||
        !PixelIs(destination, 1, 1, RGB(255, 255, 255)) ||
        !PixelIs(destination, 2, 2, RGB(255, 255, 255)) ||
        ExcludeClipRect(destination, 0, 0, 640, 480) != NULLREGION ||
        !RestoreDC(destination, saved_clip) ||
        !PatBlt(destination, 1, 1, 1, 1, BLACKNESS) ||
        !PixelIs(destination, 1, 1, RGB(0, 0, 0))) {
        return 23;
    }

    HRGN region = CreateRectRgn(1, 1, 3, 3);
    if (region == nullptr || !DeleteObject(region) ||
        !PatBlt(destination, 0, 0, 4, 4, WHITENESS) ||
        IntersectClipRect(destination, 1, 1, 3, 3) != SIMPLEREGION ||
        !PatBlt(destination, 0, 0, 4, 4, BLACKNESS) ||
        !PixelIs(destination, 1, 1, RGB(0, 0, 0)) ||
        !PixelIs(destination, 2, 2, RGB(0, 0, 0)) ||
        !PixelIs(destination, 0, 0, RGB(255, 255, 255)) ||
        !PixelIs(destination, 3, 3, RGB(255, 255, 255)) ||
        IntersectClipRect(destination, 3, 3, 4, 4) != NULLREGION ||
        !PatBlt(destination, 1, 1, 1, 1, WHITENESS) ||
        !PixelIs(destination, 1, 1, RGB(0, 0, 0))) {
        return 24;
    }

    SelectObject(destination, old_brush);
    SelectObject(destination, old_pen);
    DeleteObject(green);
    DeleteObject(blue);
    DeleteObject(red);
    DeleteDC(source);
    DeleteDC(destination);
    return 0;
}
