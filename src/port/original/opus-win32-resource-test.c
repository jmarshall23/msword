#include "windows.h"

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>

int main(void) {
    HINSTANCE self = GetModuleHandleW(NULL);

    HBITMAP toolbar = (HBITMAP)LoadImageW(
        self, MAKEINTRESOURCEW(201), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
    assert(toolbar != NULL);
    assert(LoadBitmapA(self, MAKEINTRESOURCEA(201)) == toolbar);
    BITMAP bitmap = {0};
    assert(GetObjectA(toolbar, sizeof(bitmap), &bitmap) == sizeof(bitmap));
    assert(bitmap.bmWidth == 340);
    assert(bitmap.bmHeight == 20);
    assert(bitmap.bmBitsPixel == 24);
    SIZE size = {0};
    assert(GetBitmapDimensionEx(toolbar, &size));
    assert(size.cx == 340);
    assert(size.cy == 20);
    unsigned char bits[64] = {0};
    assert(GetBitmapBits(toolbar, (LONG)sizeof(bits), bits) ==
           (LONG)sizeof(bits));
    bool has_nonzero_pixel = false;
    for (size_t index = 0; index < sizeof(bits); ++index) {
        has_nonzero_pixel = has_nonzero_pixel || bits[index] != 0;
    }
    assert(has_nonzero_pixel);
    assert(LoadIconA(self, MAKEINTRESOURCEA(301)) != NULL);
    assert(LoadImageA(self, MAKEINTRESOURCEA(302), IMAGE_ICON, 16, 16,
                      LR_DEFAULTCOLOR) != NULL);
    assert(LoadBitmapA(NULL, MAKEINTRESOURCEA(OBM_OLD_CLOSE)) != NULL);
    assert(LoadBitmapA(self, MAKEINTRESOURCEA(OBM_UPARROW)) != NULL);

    assert(LoadImageW(self, MAKEINTRESOURCEW(999), IMAGE_BITMAP, 0, 0, 0) ==
           NULL);
    assert(LoadIconW(self, MAKEINTRESOURCEW(999)) == NULL);
    assert(LoadImageA(self, "word95-toolbar", IMAGE_BITMAP, 0, 0, 0) == NULL);
}
