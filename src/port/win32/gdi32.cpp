#include "windows.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <vector>

namespace {

constexpr DWORD kGdiMagic = 0x47444932u;

enum class GdiKind {
    Dc,
    Bitmap,
    Brush,
    Pen,
    Font,
};

struct BitmapData {
    LONG width = 1;
    LONG height = 1;
    WORD planes = 1;
    WORD bits_per_pixel = 32;
    LONG width_bytes = 4;
    std::vector<unsigned char> bits;
};

struct PenData {
    int style = PS_SOLID;
    int width = 1;
    COLORREF color = RGB(0, 0, 0);
};

struct FontData {
    LOGFONTW logical{};
};

struct BrushData {
    COLORREF color = RGB(255, 255, 255);
    HBITMAP pattern = nullptr;
};

struct DcState {
    HBITMAP bitmap = nullptr;
    HBRUSH brush = nullptr;
    HPEN pen = nullptr;
    HFONT font = nullptr;
    COLORREF text_color = RGB(0, 0, 0);
    COLORREF background_color = RGB(255, 255, 255);
    int background_mode = OPAQUE;
    int raster_operation = R2_COPYPEN;
    int stretch_mode = COLORONCOLOR;
    RECT clip{0, 0, 0, 0};
    POINT viewport_origin{0, 0};
    SIZE viewport_extent{1, 1};
    POINT window_origin{0, 0};
    SIZE window_extent{1, 1};
};

struct GdiObject {
    DWORD magic = kGdiMagic;
    GdiKind kind = GdiKind::Brush;
    bool stock = false;
    bool selected = false;
    BitmapData bitmap;
    BrushData brush;
    PenData pen;
    FontData font;
    DcState dc;
    std::vector<DcState> saved;
};

std::mutex g_gdi_lock;

GdiObject g_white_brush{ kGdiMagic, GdiKind::Brush, true };
GdiObject g_ltgray_brush{ kGdiMagic, GdiKind::Brush, true };
GdiObject g_gray_brush{ kGdiMagic, GdiKind::Brush, true };
GdiObject g_dkgray_brush{ kGdiMagic, GdiKind::Brush, true };
GdiObject g_black_brush{ kGdiMagic, GdiKind::Brush, true };
GdiObject g_black_pen{ kGdiMagic, GdiKind::Pen, true };
GdiObject g_white_pen{ kGdiMagic, GdiKind::Pen, true };
GdiObject g_system_font{ kGdiMagic, GdiKind::Font, true };
GdiObject g_default_gui_font{ kGdiMagic, GdiKind::Font, true };

GdiObject* object_from_handle(HGDIOBJ handle) {
    auto* object = static_cast<GdiObject*>(handle);
    return object != nullptr && object->magic == kGdiMagic ? object : nullptr;
}

GdiObject* dc_from_handle(HDC handle) {
    auto* object = object_from_handle(handle);
    return object != nullptr && object->kind == GdiKind::Dc ? object : nullptr;
}

int bitmap_stride(int width, UINT bits_per_pixel) {
    return static_cast<int>(((width * bits_per_pixel + 31u) / 32u) * 4u);
}

HBITMAP create_bitmap(int width, int height, UINT planes, UINT bits_per_pixel,
                      const void* bits) {
    if (width <= 0 || height == 0 || planes == 0 || bits_per_pixel == 0) {
        return nullptr;
    }
    auto* object = new GdiObject();
    object->kind = GdiKind::Bitmap;
    object->bitmap.width = width;
    object->bitmap.height = height < 0 ? -height : height;
    object->bitmap.planes = static_cast<WORD>(planes);
    object->bitmap.bits_per_pixel = static_cast<WORD>(bits_per_pixel);
    object->bitmap.width_bytes = bitmap_stride(width, bits_per_pixel);
    const auto size = static_cast<std::size_t>(object->bitmap.width_bytes) *
                      static_cast<std::size_t>(object->bitmap.height);
    object->bitmap.bits.resize(size);
    if (bits != nullptr) {
        std::memcpy(object->bitmap.bits.data(), bits, size);
    }
    return static_cast<HBITMAP>(object);
}

HGDIOBJ select_object(GdiObject& dc, GdiObject& object) {
    HGDIOBJ* slot = nullptr;
    switch (object.kind) {
        case GdiKind::Bitmap: slot = reinterpret_cast<HGDIOBJ*>(&dc.dc.bitmap); break;
        case GdiKind::Brush: slot = reinterpret_cast<HGDIOBJ*>(&dc.dc.brush); break;
        case GdiKind::Pen: slot = reinterpret_cast<HGDIOBJ*>(&dc.dc.pen); break;
        case GdiKind::Font: slot = reinterpret_cast<HGDIOBJ*>(&dc.dc.font); break;
        case GdiKind::Dc: return nullptr;
    }
    HGDIOBJ previous = *slot;
    if (auto* previous_object = object_from_handle(previous)) {
        previous_object->selected = false;
    }
    *slot = static_cast<HGDIOBJ>(&object);
    object.selected = true;
    return previous;
}

void mark_selected(DcState& state, bool selected) {
    for (HGDIOBJ handle :
         {static_cast<HGDIOBJ>(state.bitmap), static_cast<HGDIOBJ>(state.brush),
          static_cast<HGDIOBJ>(state.pen), static_cast<HGDIOBJ>(state.font)}) {
        if (auto* object = object_from_handle(handle)) object->selected = selected;
    }
}

void init_stock_objects() {
    static bool initialized = false;
    if (initialized) return;
    initialized = true;
    g_white_brush.brush.color = RGB(255, 255, 255);
    g_ltgray_brush.brush.color = RGB(192, 192, 192);
    g_gray_brush.brush.color = RGB(128, 128, 128);
    g_dkgray_brush.brush.color = RGB(64, 64, 64);
    g_black_brush.brush.color = RGB(0, 0, 0);
    g_black_pen.pen.color = RGB(0, 0, 0);
    g_white_pen.pen.color = RGB(255, 255, 255);
}

}  // namespace

extern "C" {

HBITMAP OpusGdiCreateBitmapFromBmpBytes(const unsigned char* bytes,
                                        unsigned int size) {
    if (bytes == nullptr || size < 54 || bytes[0] != 'B' || bytes[1] != 'M') {
        return nullptr;
    }
    const auto read_u16 = [bytes](std::size_t offset) -> WORD {
        return static_cast<WORD>(bytes[offset] | (bytes[offset + 1] << 8));
    };
    const auto read_u32 = [read_u16](std::size_t offset) -> DWORD {
        return static_cast<DWORD>(read_u16(offset)) |
               (static_cast<DWORD>(read_u16(offset + 2)) << 16u);
    };
    const auto read_i32 = [read_u32](std::size_t offset) -> LONG {
        return static_cast<LONG>(read_u32(offset));
    };
    const DWORD bits_offset = read_u32(10);
    const LONG width = read_i32(18);
    const LONG height = read_i32(22);
    const WORD planes = read_u16(26);
    const WORD bits_per_pixel = read_u16(28);
    if (bits_offset >= size) return nullptr;
    const std::size_t byte_count =
        static_cast<std::size_t>(bitmap_stride(width, bits_per_pixel)) *
        static_cast<std::size_t>(height < 0 ? -height : height);
    if (bits_offset + byte_count > size) return nullptr;
    return create_bitmap(width, height, planes, bits_per_pixel,
                         bytes + bits_offset);
}

HGDIOBJ GetStockObject(int object) {
    std::lock_guard<std::mutex> guard(g_gdi_lock);
    init_stock_objects();
    switch (object) {
        case WHITE_BRUSH: return &g_white_brush;
        case LTGRAY_BRUSH: return &g_ltgray_brush;
        case GRAY_BRUSH: return &g_gray_brush;
        case DKGRAY_BRUSH: return &g_dkgray_brush;
        case BLACK_BRUSH: return &g_black_brush;
        case WHITE_PEN: return &g_white_pen;
        case BLACK_PEN: return &g_black_pen;
        case SYSTEM_FONT: return &g_system_font;
        case DEFAULT_GUI_FONT: return &g_default_gui_font;
        default: return nullptr;
    }
}

HDC CreateCompatibleDC(HDC) {
    auto* object = new GdiObject();
    object->kind = GdiKind::Dc;
    object->dc.brush = static_cast<HBRUSH>(GetStockObject(WHITE_BRUSH));
    object->dc.pen = static_cast<HPEN>(GetStockObject(BLACK_PEN));
    object->dc.font = static_cast<HFONT>(GetStockObject(SYSTEM_FONT));
    object->dc.clip = {0, 0, 640, 480};
    return static_cast<HDC>(object);
}

BOOL DeleteDC(HDC device_context) {
    std::lock_guard<std::mutex> guard(g_gdi_lock);
    auto* dc = dc_from_handle(device_context);
    if (dc == nullptr || dc->stock) return FALSE;
    mark_selected(dc->dc, false);
    dc->magic = 0;
    delete dc;
    return TRUE;
}

HBITMAP CreateBitmap(int width, int height, UINT planes, UINT bits_per_pixel,
                     LPCVOID bits) {
    return create_bitmap(width, height, planes, bits_per_pixel, bits);
}

HBITMAP CreateBitmapIndirect(const BITMAP* bitmap) {
    if (bitmap == nullptr) return nullptr;
    return create_bitmap(bitmap->bmWidth, bitmap->bmHeight, bitmap->bmPlanes,
                         bitmap->bmBitsPixel, bitmap->bmBits);
}

HBITMAP CreateCompatibleBitmap(HDC, int width, int height) {
    return create_bitmap(width, height, 1, 32, nullptr);
}

HBITMAP CreateDIBitmap(HDC, const BITMAPINFOHEADER* header, DWORD init,
                       LPCVOID bits, const BITMAPINFO*, UINT) {
    if (header == nullptr) return nullptr;
    return create_bitmap(header->biWidth, header->biHeight, header->biPlanes,
                         header->biBitCount,
                         (init & CBM_INIT) != 0 ? bits : nullptr);
}

HBRUSH CreateSolidBrush(DWORD color) {
    auto* object = new GdiObject();
    object->kind = GdiKind::Brush;
    object->brush.color = color;
    return static_cast<HBRUSH>(object);
}

HBRUSH CreatePatternBrush(HBITMAP bitmap) {
    if (object_from_handle(bitmap) == nullptr) return nullptr;
    auto* object = new GdiObject();
    object->kind = GdiKind::Brush;
    object->brush.pattern = bitmap;
    return static_cast<HBRUSH>(object);
}

HPEN CreatePen(int style, int width, DWORD color) {
    auto* object = new GdiObject();
    object->kind = GdiKind::Pen;
    object->pen = {style, width, color};
    return static_cast<HPEN>(object);
}

HPEN CreatePenIndirect(const LOGPEN* log_pen) {
    if (log_pen == nullptr) return nullptr;
    return CreatePen(log_pen->lopnStyle, log_pen->lopnWidth.x, log_pen->lopnColor);
}

HFONT CreateFontIndirectW(const LOGFONTW* logical_font) {
    auto* object = new GdiObject();
    object->kind = GdiKind::Font;
    if (logical_font != nullptr) object->font.logical = *logical_font;
    return static_cast<HFONT>(object);
}

HFONT CreateFontIndirectA(const LOGFONTA* logical_font) {
    LOGFONTW wide{};
    if (logical_font != nullptr) {
        wide.lfHeight = logical_font->lfHeight;
        wide.lfWidth = logical_font->lfWidth;
        wide.lfEscapement = logical_font->lfEscapement;
        wide.lfOrientation = logical_font->lfOrientation;
        wide.lfWeight = logical_font->lfWeight;
        wide.lfItalic = logical_font->lfItalic;
        wide.lfUnderline = logical_font->lfUnderline;
        wide.lfStrikeOut = logical_font->lfStrikeOut;
        wide.lfCharSet = logical_font->lfCharSet;
        wide.lfOutPrecision = logical_font->lfOutPrecision;
        wide.lfClipPrecision = logical_font->lfClipPrecision;
        wide.lfQuality = logical_font->lfQuality;
        wide.lfPitchAndFamily = logical_font->lfPitchAndFamily;
        for (std::size_t i = 0; i < LF_FACESIZE; ++i) {
            wide.lfFaceName[i] =
                static_cast<WCHAR>(static_cast<unsigned char>(logical_font->lfFaceName[i]));
        }
    }
    return CreateFontIndirectW(&wide);
}

HGDIOBJ SelectObject(HDC device_context, HGDIOBJ object) {
    std::lock_guard<std::mutex> guard(g_gdi_lock);
    auto* dc = dc_from_handle(device_context);
    auto* gdi_object = object_from_handle(object);
    if (dc == nullptr || gdi_object == nullptr) return nullptr;
    return select_object(*dc, *gdi_object);
}

HGDIOBJ GetCurrentObject(HDC device_context, UINT object_type) {
    std::lock_guard<std::mutex> guard(g_gdi_lock);
    auto* dc = dc_from_handle(device_context);
    if (dc == nullptr) return nullptr;
    switch (object_type) {
        case OBJ_BITMAP: return dc->dc.bitmap;
        case OBJ_BRUSH: return dc->dc.brush;
        case OBJ_PEN: return dc->dc.pen;
        case OBJ_FONT: return dc->dc.font;
        default: return nullptr;
    }
}

BOOL DeleteObject(HGDIOBJ object) {
    std::lock_guard<std::mutex> guard(g_gdi_lock);
    auto* gdi_object = object_from_handle(object);
    if (gdi_object == nullptr || gdi_object->stock || gdi_object->selected) {
        return FALSE;
    }
    gdi_object->magic = 0;
    delete gdi_object;
    return TRUE;
}

int SaveDC(HDC device_context) {
    std::lock_guard<std::mutex> guard(g_gdi_lock);
    auto* dc = dc_from_handle(device_context);
    if (dc == nullptr) return 0;
    dc->saved.push_back(dc->dc);
    return static_cast<int>(dc->saved.size());
}

BOOL RestoreDC(HDC device_context, int saved_dc) {
    std::lock_guard<std::mutex> guard(g_gdi_lock);
    auto* dc = dc_from_handle(device_context);
    if (dc == nullptr || dc->saved.empty()) return FALSE;
    std::size_t index = 0;
    if (saved_dc < 0) {
        const int relative = static_cast<int>(dc->saved.size()) + saved_dc;
        if (relative < 0) return FALSE;
        index = static_cast<std::size_t>(relative);
    } else {
        if (saved_dc == 0 ||
            static_cast<std::size_t>(saved_dc) > dc->saved.size()) {
            return FALSE;
        }
        index = static_cast<std::size_t>(saved_dc - 1);
    }
    mark_selected(dc->dc, false);
    dc->dc = dc->saved[index];
    mark_selected(dc->dc, true);
    dc->saved.resize(index);
    return TRUE;
}

int GetObjectA(HANDLE object, int buffer_size, LPVOID object_data) {
    auto* bitmap = object_from_handle(object);
    if (bitmap == nullptr || bitmap->kind != GdiKind::Bitmap ||
        object_data == nullptr || buffer_size < static_cast<int>(sizeof(BITMAP))) {
        return 0;
    }
    BITMAP info{};
    info.bmWidth = bitmap->bitmap.width;
    info.bmHeight = bitmap->bitmap.height;
    info.bmWidthBytes = bitmap->bitmap.width_bytes;
    info.bmPlanes = bitmap->bitmap.planes;
    info.bmBitsPixel = bitmap->bitmap.bits_per_pixel;
    info.bmBits = bitmap->bitmap.bits.data();
    std::memcpy(object_data, &info, sizeof(info));
    return static_cast<int>(sizeof(info));
}

LONG GetBitmapBits(HBITMAP bitmap_handle, LONG count, LPVOID bits) {
    auto* bitmap = object_from_handle(bitmap_handle);
    if (bitmap == nullptr || bitmap->kind != GdiKind::Bitmap ||
        bits == nullptr || count <= 0) {
        return 0;
    }
    const auto copied = static_cast<LONG>((std::min)(
        bitmap->bitmap.bits.size(), static_cast<std::size_t>(count)));
    std::memcpy(bits, bitmap->bitmap.bits.data(), static_cast<std::size_t>(copied));
    return copied;
}

BOOL GetBitmapDimensionEx(HBITMAP bitmap_handle, SIZE* size) {
    auto* bitmap = object_from_handle(bitmap_handle);
    if (bitmap == nullptr || bitmap->kind != GdiKind::Bitmap || size == nullptr) {
        return FALSE;
    }
    size->cx = bitmap->bitmap.width;
    size->cy = bitmap->bitmap.height;
    return TRUE;
}

BOOL SetBitmapDimensionEx(HBITMAP bitmap_handle, int width, int height,
                          SIZE* previous) {
    auto* bitmap = object_from_handle(bitmap_handle);
    if (bitmap == nullptr || bitmap->kind != GdiKind::Bitmap) return FALSE;
    if (previous != nullptr) {
        previous->cx = bitmap->bitmap.width;
        previous->cy = bitmap->bitmap.height;
    }
    bitmap->bitmap.width = width;
    bitmap->bitmap.height = height;
    return TRUE;
}

}  // extern "C"
