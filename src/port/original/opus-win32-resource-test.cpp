#include "windows.h"

#include <array>
#include <cassert>

int main() {
    HINSTANCE self = GetModuleHandleW(nullptr);

    HBITMAP toolbar = static_cast<HBITMAP>(LoadImageW(
        self, MAKEINTRESOURCEW(201), IMAGE_BITMAP, 0, 0,
        LR_CREATEDIBSECTION));
    assert(toolbar != nullptr);
    assert(LoadBitmapA(self, MAKEINTRESOURCEA(201)) == toolbar);
    BITMAP bitmap{};
    assert(GetObjectA(toolbar, sizeof(bitmap), &bitmap) == sizeof(bitmap));
    assert(bitmap.bmWidth == 340);
    assert(bitmap.bmHeight == 20);
    assert(bitmap.bmBitsPixel == 24);
    SIZE size{};
    assert(GetBitmapDimensionEx(toolbar, &size));
    assert(size.cx == 340);
    assert(size.cy == 20);
    std::array<unsigned char, 64> bits{};
    assert(GetBitmapBits(toolbar, static_cast<LONG>(bits.size()),
                         bits.data()) == static_cast<LONG>(bits.size()));
    bool has_nonzero_pixel = false;
    for (const unsigned char value : bits) {
        has_nonzero_pixel = has_nonzero_pixel || value != 0;
    }
    assert(has_nonzero_pixel);
    assert(LoadIconA(self, MAKEINTRESOURCEA(301)) != nullptr);
    assert(LoadImageA(self, MAKEINTRESOURCEA(302), IMAGE_ICON, 16, 16,
                      LR_DEFAULTCOLOR) != nullptr);
    assert(LoadBitmapA(nullptr, MAKEINTRESOURCEA(OBM_OLD_CLOSE)) != nullptr);
    assert(LoadBitmapA(self, MAKEINTRESOURCEA(OBM_UPARROW)) != nullptr);

    assert(LoadImageW(self, MAKEINTRESOURCEW(999), IMAGE_BITMAP, 0, 0, 0) ==
           nullptr);
    assert(LoadIconW(self, MAKEINTRESOURCEW(999)) == nullptr);
    assert(LoadImageA(self, "word95-toolbar", IMAGE_BITMAP, 0, 0, 0) ==
           nullptr);
}
