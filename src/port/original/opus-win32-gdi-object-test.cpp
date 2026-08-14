#include "windows.h"

int main() {
    HDC dc = CreateCompatibleDC(nullptr);
    if (dc == nullptr) return 1;

    HBRUSH default_brush = static_cast<HBRUSH>(GetCurrentObject(dc, OBJ_BRUSH));
    HPEN default_pen = static_cast<HPEN>(GetCurrentObject(dc, OBJ_PEN));
    HFONT default_font = static_cast<HFONT>(GetCurrentObject(dc, OBJ_FONT));
    HBITMAP default_bitmap =
        static_cast<HBITMAP>(GetCurrentObject(dc, OBJ_BITMAP));
    if (default_brush == nullptr || default_pen == nullptr ||
        default_font == nullptr || default_bitmap == nullptr) {
        return 2;
    }

    HDC device_dc = CreateDCA("DISPLAY", nullptr, nullptr, nullptr);
    HDC info_dc = CreateICA("DISPLAY", nullptr, nullptr, nullptr);
    if (device_dc == nullptr || info_dc == nullptr || device_dc == info_dc ||
        GetCurrentObject(device_dc, OBJ_BRUSH) == nullptr ||
        GetCurrentObject(info_dc, OBJ_PEN) == nullptr ||
        !DeleteDC(device_dc) || !DeleteDC(info_dc)) {
        return 15;
    }

    HDC metafile_dc = CreateMetaFile(nullptr);
    if (metafile_dc == nullptr || CloseMetaFile(dc) != nullptr) return 16;
    HMETAFILE metafile = CloseMetaFile(metafile_dc);
    if (metafile == nullptr || DeleteDC(metafile_dc) != FALSE ||
        !DeleteObject(static_cast<HGDIOBJ>(metafile))) {
        return 17;
    }

    HBRUSH brush = CreateSolidBrush(RGB(10, 20, 30));
    HPEN pen = CreatePen(PS_SOLID, 2, RGB(30, 20, 10));
    LOGFONTA logical{};
    logical.lfHeight = 18;
    HFONT font = CreateFontIndirectA(&logical);
    HBITMAP bitmap = CreateCompatibleBitmap(dc, 8, 4);
    if (brush == nullptr || pen == nullptr || font == nullptr ||
        bitmap == nullptr) {
        return 3;
    }

    if (SelectObject(dc, brush) != default_brush ||
        SelectObject(dc, pen) != default_pen ||
        SelectObject(dc, font) != default_font ||
        SelectObject(dc, bitmap) != default_bitmap) {
        return 4;
    }
    if (DeleteObject(brush) != FALSE) return 5;

    const int saved = SaveDC(dc);
    if (saved <= 0) return 6;

    HBRUSH second_brush = CreateSolidBrush(RGB(1, 2, 3));
    if (SelectObject(dc, second_brush) != brush ||
        GetCurrentObject(dc, OBJ_BRUSH) != second_brush) {
        return 7;
    }
    if (!RestoreDC(dc, saved) || GetCurrentObject(dc, OBJ_BRUSH) != brush) {
        return 8;
    }

    BITMAP info{};
    if (GetObjectA(bitmap, sizeof(info), &info) != sizeof(info) ||
        info.bmWidth != 8 || info.bmHeight != 4 || info.bmBitsPixel != 32) {
        return 9;
    }

    HBITMAP spare_bitmap = CreateCompatibleBitmap(dc, 1, 1);
    if (spare_bitmap == nullptr || SelectObject(dc, spare_bitmap) != bitmap) {
        return 10;
    }
    SelectObject(dc, default_brush);
    SelectObject(dc, default_pen);
    SelectObject(dc, default_font);
    if (!DeleteObject(brush) || !DeleteObject(second_brush) ||
        !DeleteObject(pen) || !DeleteObject(font) || !DeleteObject(bitmap)) {
        return 11;
    }
    if (DeleteObject(GetStockObject(WHITE_BRUSH)) != FALSE) return 12;
    if (!DeleteDC(dc)) return 13;
    if (!DeleteObject(spare_bitmap)) return 14;
    return 0;
}
