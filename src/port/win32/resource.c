#include "windows.h"

#include <stddef.h>
#include <stdint.h>

#include "opus-win32-resource-arrays.h"

HBITMAP OpusGdiCreateBitmapFromBmpBytes(const unsigned char* bytes,
                                        unsigned int size);

#define OPUS_TOOLBAR_BITMAP_ID 201
#define OPUS_WORD_ICON_ID 301
#define OPUS_DOCUMENT_ICON_ID 302

struct OpusResourceHandle {
    WORD id;
    UINT type;
    const unsigned char* bytes;
    unsigned int size;
};

static struct OpusResourceHandle g_toolbar_bitmap = {
    OPUS_TOOLBAR_BITMAP_ID, IMAGE_BITMAP, kOpusToolbar201Bmp,
    kOpusToolbar201Bmp_size};
static HBITMAP g_toolbar_bitmap_handle = NULL;
static struct OpusResourceHandle g_word_icon = {
    OPUS_WORD_ICON_ID, IMAGE_ICON, kOpusIcon301Ico, kOpusIcon301Ico_size};
static struct OpusResourceHandle g_document_icon = {
    OPUS_DOCUMENT_ICON_ID, IMAGE_ICON, kOpusIcon302Ico,
    kOpusIcon302Ico_size};
static struct OpusResourceHandle g_stock_bitmaps[] = {
    {OBM_OLD_CLOSE, IMAGE_BITMAP, NULL, 0},
    {OBM_OLD_UPARROW, IMAGE_BITMAP, NULL, 0},
    {OBM_OLD_DNARROW, IMAGE_BITMAP, NULL, 0},
    {OBM_OLD_LFARROW, IMAGE_BITMAP, NULL, 0},
    {OBM_OLD_RGARROW, IMAGE_BITMAP, NULL, 0},
    {OBM_BTSIZE, IMAGE_BITMAP, NULL, 0},
    {OBM_CLOSE, IMAGE_BITMAP, NULL, 0},
    {OBM_UPARROW, IMAGE_BITMAP, NULL, 0},
    {OBM_DNARROW, IMAGE_BITMAP, NULL, 0},
    {OBM_LFARROW, IMAGE_BITMAP, NULL, 0},
    {OBM_RGARROW, IMAGE_BITMAP, NULL, 0},
    {OBM_UPARROWD, IMAGE_BITMAP, NULL, 0},
    {OBM_DNARROWD, IMAGE_BITMAP, NULL, 0},
    {OBM_LFARROWD, IMAGE_BITMAP, NULL, 0},
    {OBM_RGARROWD, IMAGE_BITMAP, NULL, 0},
};

static BOOL IsIntegerResource(const void* name) {
    return ((uintptr_t)name >> 16) == 0;
}

static WORD ResourceId(const void* name) {
    return (WORD)(uintptr_t)name;
}

static uint16_t ReadU16(const unsigned char* bytes, size_t offset) {
    return (uint16_t)bytes[offset] | (uint16_t)(bytes[offset + 1] << 8);
}

static uint32_t ReadU32(const unsigned char* bytes, size_t offset) {
    return (uint32_t)ReadU16(bytes, offset) |
           ((uint32_t)ReadU16(bytes, offset + 2) << 16);
}

static int32_t ReadI32(const unsigned char* bytes, size_t offset) {
    return (int32_t)ReadU32(bytes, offset);
}

static BOOL HasToolbarBitmap(void) {
    if (kOpusToolbar201Bmp_size < 54 || kOpusToolbar201Bmp[0] != 'B' ||
        kOpusToolbar201Bmp[1] != 'M') {
        return FALSE;
    }
    const uint32_t info_size = ReadU32(kOpusToolbar201Bmp, 14);
    const uint32_t bits_offset = ReadU32(kOpusToolbar201Bmp, 10);
    return info_size >= sizeof(BITMAPINFOHEADER) &&
           bits_offset < kOpusToolbar201Bmp_size &&
           ReadI32(kOpusToolbar201Bmp, 18) == 340 &&
           ReadI32(kOpusToolbar201Bmp, 22) == 20 &&
           ReadU16(kOpusToolbar201Bmp, 26) == 1 &&
           ReadU16(kOpusToolbar201Bmp, 28) == 24;
}

static BOOL HasIconResource(WORD id) {
    const unsigned char* bytes = NULL;
    unsigned int size = 0;
    if (id == OPUS_WORD_ICON_ID) {
        bytes = kOpusIcon301Ico;
        size = kOpusIcon301Ico_size;
    } else if (id == OPUS_DOCUMENT_ICON_ID) {
        bytes = kOpusIcon302Ico;
        size = kOpusIcon302Ico_size;
    }
    return size >= 6 && ReadU16(bytes, 0) == 0 && ReadU16(bytes, 2) == 1 &&
           ReadU16(bytes, 4) > 0;
}

static HBITMAP LoadStockBitmap(WORD id) {
    size_t i;
    for (i = 0; i < sizeof(g_stock_bitmaps) / sizeof(g_stock_bitmaps[0]); ++i) {
        if (g_stock_bitmaps[i].id == id) {
            return CreateCompatibleBitmap(NULL, 16, 16);
        }
    }
    return NULL;
}

static HANDLE LoadIntegerResource(WORD id, UINT type) {
    if (type == IMAGE_BITMAP && id == OPUS_TOOLBAR_BITMAP_ID &&
        HasToolbarBitmap()) {
        if (g_toolbar_bitmap_handle == NULL) {
            g_toolbar_bitmap_handle = OpusGdiCreateBitmapFromBmpBytes(
                g_toolbar_bitmap.bytes, g_toolbar_bitmap.size);
        }
        return g_toolbar_bitmap_handle;
    }
    if (type == IMAGE_BITMAP) {
        return LoadStockBitmap(id);
    }
    if (type == IMAGE_ICON && id == OPUS_WORD_ICON_ID &&
        HasIconResource(id)) {
        return (HICON)&g_word_icon;
    }
    if (type == IMAGE_ICON && id == OPUS_DOCUMENT_ICON_ID &&
        HasIconResource(id)) {
        return (HICON)&g_document_icon;
    }
    return NULL;
}

static HANDLE LoadResourceByName(const void* name, UINT type) {
    if (name == NULL || !IsIntegerResource(name)) {
        return NULL;
    }
    return LoadIntegerResource(ResourceId(name), type);
}

HANDLE LoadImageA(HINSTANCE instance, LPCSTR name, UINT type, int cx, int cy,
                  UINT flags) {
    (void)instance;
    (void)cx;
    (void)cy;
    (void)flags;
    return LoadResourceByName(name, type);
}

HANDLE LoadImageW(HINSTANCE instance, LPCWSTR name, UINT type, int cx, int cy,
                  UINT flags) {
    (void)instance;
    (void)cx;
    (void)cy;
    (void)flags;
    return LoadResourceByName(name, type);
}

HBITMAP LoadBitmapA(HANDLE instance, LPSTR bitmap_name) {
    return (HBITMAP)LoadImageA((HINSTANCE)instance, bitmap_name, IMAGE_BITMAP,
                               0, 0, 0);
}

HBITMAP LoadBitmapW(HANDLE instance, LPWSTR bitmap_name) {
    return (HBITMAP)LoadImageW((HINSTANCE)instance, bitmap_name, IMAGE_BITMAP,
                               0, 0, 0);
}

HICON LoadIconA(HINSTANCE instance, LPCSTR icon_name) {
    return (HICON)LoadImageA(instance, icon_name, IMAGE_ICON, 0, 0,
                             LR_DEFAULTCOLOR);
}

HICON LoadIconW(HINSTANCE instance, LPCWSTR icon_name) {
    return (HICON)LoadImageW(instance, icon_name, IMAGE_ICON, 0, 0,
                             LR_DEFAULTCOLOR);
}

BOOL DestroyIcon(HICON icon) {
    return icon == (HICON)&g_word_icon || icon == (HICON)&g_document_icon;
}

BOOL DrawIconEx(HDC device_context, int x, int y, HICON icon, int width,
                int height, UINT step, HBRUSH brush, UINT flags) {
    (void)device_context;
    (void)x;
    (void)y;
    (void)width;
    (void)height;
    (void)step;
    (void)brush;
    (void)flags;
    if (icon != (HICON)&g_word_icon && icon != (HICON)&g_document_icon) {
        return FALSE;
    }
    return TRUE;
}
