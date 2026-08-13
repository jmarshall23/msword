#include "windows.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>

#include "opus-win32-resource-arrays.h"

namespace
{
constexpr WORD kToolbarBitmapId = 201;
constexpr WORD kWordIconId = 301;
constexpr WORD kDocumentIconId = 302;

struct OpusResourceHandle {
    WORD id;
    UINT type;
    const unsigned char* bytes;
    unsigned int size;
};

OpusResourceHandle g_toolbar_bitmap{kToolbarBitmapId, IMAGE_BITMAP,
                                    kOpusToolbar201Bmp,
                                    kOpusToolbar201Bmp_size};
OpusResourceHandle g_word_icon{kWordIconId, IMAGE_ICON, kOpusIcon301Ico,
                               kOpusIcon301Ico_size};
OpusResourceHandle g_document_icon{kDocumentIconId, IMAGE_ICON,
                                   kOpusIcon302Ico, kOpusIcon302Ico_size};
OpusResourceHandle g_stock_bitmaps[] = {
    {OBM_OLD_CLOSE, IMAGE_BITMAP, nullptr, 0},
    {OBM_OLD_UPARROW, IMAGE_BITMAP, nullptr, 0},
    {OBM_OLD_DNARROW, IMAGE_BITMAP, nullptr, 0},
    {OBM_OLD_LFARROW, IMAGE_BITMAP, nullptr, 0},
    {OBM_OLD_RGARROW, IMAGE_BITMAP, nullptr, 0},
    {OBM_BTSIZE, IMAGE_BITMAP, nullptr, 0},
    {OBM_CLOSE, IMAGE_BITMAP, nullptr, 0},
    {OBM_UPARROW, IMAGE_BITMAP, nullptr, 0},
    {OBM_DNARROW, IMAGE_BITMAP, nullptr, 0},
    {OBM_LFARROW, IMAGE_BITMAP, nullptr, 0},
    {OBM_RGARROW, IMAGE_BITMAP, nullptr, 0},
    {OBM_UPARROWD, IMAGE_BITMAP, nullptr, 0},
    {OBM_DNARROWD, IMAGE_BITMAP, nullptr, 0},
    {OBM_LFARROWD, IMAGE_BITMAP, nullptr, 0},
    {OBM_RGARROWD, IMAGE_BITMAP, nullptr, 0},
};

bool IsIntegerResource(const void* name) {
    return (reinterpret_cast<std::uintptr_t>(name) >> 16) == 0;
}

WORD ResourceId(const void* name) {
    return static_cast<WORD>(reinterpret_cast<std::uintptr_t>(name));
}

std::uint16_t ReadU16(const unsigned char* bytes, std::size_t offset) {
    return static_cast<std::uint16_t>(bytes[offset]) |
           static_cast<std::uint16_t>(bytes[offset + 1] << 8);
}

std::uint32_t ReadU32(const unsigned char* bytes, std::size_t offset) {
    return static_cast<std::uint32_t>(ReadU16(bytes, offset)) |
           (static_cast<std::uint32_t>(ReadU16(bytes, offset + 2)) << 16);
}

std::int32_t ReadI32(const unsigned char* bytes, std::size_t offset) {
    return static_cast<std::int32_t>(ReadU32(bytes, offset));
}

bool HasToolbarBitmap() {
    if (kOpusToolbar201Bmp_size < 54 || kOpusToolbar201Bmp[0] != 'B' ||
        kOpusToolbar201Bmp[1] != 'M') {
        return false;
    }
    const std::uint32_t info_size = ReadU32(kOpusToolbar201Bmp, 14);
    const std::uint32_t bits_offset = ReadU32(kOpusToolbar201Bmp, 10);
    return info_size >= sizeof(BITMAPINFOHEADER) &&
           bits_offset < kOpusToolbar201Bmp_size &&
           ReadI32(kOpusToolbar201Bmp, 18) == 340 &&
           ReadI32(kOpusToolbar201Bmp, 22) == 20 &&
           ReadU16(kOpusToolbar201Bmp, 26) == 1 &&
           ReadU16(kOpusToolbar201Bmp, 28) == 24;
}

OpusResourceHandle* BitmapFromHandle(HBITMAP bitmap) {
    if (bitmap == reinterpret_cast<HBITMAP>(&g_toolbar_bitmap)) {
        return &g_toolbar_bitmap;
    }
    for (auto& stock : g_stock_bitmaps) {
        if (bitmap == reinterpret_cast<HBITMAP>(&stock)) {
            return &stock;
        }
    }
    return nullptr;
}

bool FillBitmapInfo(OpusResourceHandle& handle, BITMAP& bitmap) {
    if (handle.bytes == nullptr || handle.size < 54 ||
        handle.bytes[0] != 'B' || handle.bytes[1] != 'M') {
        return false;
    }
    const LONG width = ReadI32(handle.bytes, 18);
    const LONG height = ReadI32(handle.bytes, 22);
    const WORD planes = ReadU16(handle.bytes, 26);
    const WORD bits_per_pixel = ReadU16(handle.bytes, 28);
    if (width <= 0 || height == 0 || planes == 0 || bits_per_pixel == 0) {
        return false;
    }
    const LONG absolute_height = height < 0 ? -height : height;
    const LONG width_bytes = ((width * bits_per_pixel + 31) / 32) * 4;
    bitmap = {};
    bitmap.bmWidth = width;
    bitmap.bmHeight = absolute_height;
    bitmap.bmWidthBytes = width_bytes;
    bitmap.bmPlanes = planes;
    bitmap.bmBitsPixel = bits_per_pixel;
    bitmap.bmBits =
        const_cast<unsigned char*>(handle.bytes + ReadU32(handle.bytes, 10));
    return true;
}

bool HasIconResource(WORD id) {
    const unsigned char* bytes = nullptr;
    unsigned int size = 0;
    if (id == kWordIconId) {
        bytes = kOpusIcon301Ico;
        size = kOpusIcon301Ico_size;
    } else if (id == kDocumentIconId) {
        bytes = kOpusIcon302Ico;
        size = kOpusIcon302Ico_size;
    }
    return size >= 6 && ReadU16(bytes, 0) == 0 && ReadU16(bytes, 2) == 1 &&
           ReadU16(bytes, 4) > 0;
}

HBITMAP LoadStockBitmap(WORD id) {
    for (auto& bitmap : g_stock_bitmaps) {
        if (bitmap.id == id) {
            return reinterpret_cast<HBITMAP>(&bitmap);
        }
    }
    return nullptr;
}

HANDLE LoadIntegerResource(WORD id, UINT type) {
    if (type == IMAGE_BITMAP && id == kToolbarBitmapId && HasToolbarBitmap()) {
        return reinterpret_cast<HBITMAP>(&g_toolbar_bitmap);
    }
    if (type == IMAGE_BITMAP) {
        return LoadStockBitmap(id);
    }
    if (type == IMAGE_ICON && id == kWordIconId && HasIconResource(id)) {
        return reinterpret_cast<HICON>(&g_word_icon);
    }
    if (type == IMAGE_ICON && id == kDocumentIconId && HasIconResource(id)) {
        return reinterpret_cast<HICON>(&g_document_icon);
    }
    return nullptr;
}

HANDLE LoadResourceByName(const void* name, UINT type) {
    if (name == nullptr || !IsIntegerResource(name)) {
        return nullptr;
    }
    return LoadIntegerResource(ResourceId(name), type);
}
}

HANDLE LoadImageA(HINSTANCE, LPCSTR name, UINT type, int, int, UINT) {
    return LoadResourceByName(name, type);
}

HANDLE LoadImageW(HINSTANCE, LPCWSTR name, UINT type, int, int, UINT) {
    return LoadResourceByName(name, type);
}

HBITMAP LoadBitmapA(HANDLE instance, LPSTR bitmap_name) {
    return static_cast<HBITMAP>(
        LoadImageA(static_cast<HINSTANCE>(instance), bitmap_name, IMAGE_BITMAP,
                   0, 0, 0));
}

HBITMAP LoadBitmapW(HANDLE instance, LPWSTR bitmap_name) {
    return static_cast<HBITMAP>(
        LoadImageW(static_cast<HINSTANCE>(instance), bitmap_name, IMAGE_BITMAP,
                   0, 0, 0));
}

HICON LoadIconA(HINSTANCE instance, LPCSTR icon_name) {
    return static_cast<HICON>(
        LoadImageA(instance, icon_name, IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR));
}

HICON LoadIconW(HINSTANCE instance, LPCWSTR icon_name) {
    return static_cast<HICON>(
        LoadImageW(instance, icon_name, IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR));
}

BOOL DestroyIcon(HICON icon) {
    return icon == reinterpret_cast<HICON>(&g_word_icon) ||
           icon == reinterpret_cast<HICON>(&g_document_icon);
}

BOOL DrawIconEx(HDC device_context, int x, int y, HICON icon, int width,
                int height, UINT, HBRUSH, UINT) {
    if (icon != reinterpret_cast<HICON>(&g_word_icon) &&
        icon != reinterpret_cast<HICON>(&g_document_icon)) {
        return FALSE;
    }
    return TRUE;
}

int GetObjectA(HANDLE object, int buffer_size, LPVOID object_data) {
    if (object_data == nullptr || buffer_size < static_cast<int>(sizeof(BITMAP))) {
        return 0;
    }
    auto* bitmap = BitmapFromHandle(static_cast<HBITMAP>(object));
    BITMAP info{};
    if (bitmap == nullptr || !FillBitmapInfo(*bitmap, info)) {
        return 0;
    }
    std::memcpy(object_data, &info, sizeof(info));
    return static_cast<int>(sizeof(info));
}

LONG GetBitmapBits(HBITMAP bitmap, LONG count, LPVOID bits) {
    if (bits == nullptr || count <= 0) {
        return 0;
    }
    auto* resource = BitmapFromHandle(bitmap);
    BITMAP info{};
    if (resource == nullptr || !FillBitmapInfo(*resource, info)) {
        return 0;
    }
    const std::uint32_t bits_offset = ReadU32(resource->bytes, 10);
    const std::size_t available =
        bits_offset < resource->size ? resource->size - bits_offset : 0;
    const auto copied = static_cast<LONG>(
        (std::min)(available, static_cast<std::size_t>(count)));
    std::memcpy(bits, resource->bytes + bits_offset,
                static_cast<std::size_t>(copied));
    return copied;
}

BOOL GetBitmapDimensionEx(HBITMAP bitmap, SIZE* size) {
    if (size == nullptr) {
        return FALSE;
    }
    auto* resource = BitmapFromHandle(bitmap);
    BITMAP info{};
    if (resource == nullptr || !FillBitmapInfo(*resource, info)) {
        return FALSE;
    }
    size->cx = info.bmWidth;
    size->cy = info.bmHeight;
    return TRUE;
}
