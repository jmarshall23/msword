#include "windows.h"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <optional>
#include <mutex>
#include <vector>

namespace {

constexpr DWORD kGdiMagic = 0x47444932u;
constexpr int kLogicalPixelsPerInch = 96;
constexpr int kDesignUnitsPerEm = 1000;
constexpr int kGetSetPaperBins = 29;
constexpr int kEnumPaperBins = 31;
constexpr int kSetCharset = 772;

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

struct FontFace {
    const char* name;
    BYTE charset;
    BYTE pitch_family;
    bool fixed_pitch;
};

constexpr FontFace kFontFaces[] = {
    {"Tms Rmn", ANSI_CHARSET, VARIABLE_PITCH | FF_ROMAN, false},
    {"Helv", ANSI_CHARSET, VARIABLE_PITCH | FF_SWISS, false},
    {"Courier", ANSI_CHARSET, FIXED_PITCH | FF_MODERN, true},
    {"Symbol", SYMBOL_CHARSET, VARIABLE_PITCH | FF_DONTCARE, false},
};

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

int ascii_lower(int ch) {
    return ch >= 'A' && ch <= 'Z' ? ch - 'A' + 'a' : ch;
}

bool equal_face_name(const char* left, const char* right) {
    if (left == nullptr || right == nullptr) return false;
    while (*left != '\0' && *right != '\0') {
        if (ascii_lower(static_cast<unsigned char>(*left)) !=
            ascii_lower(static_cast<unsigned char>(*right))) {
            return false;
        }
        ++left;
        ++right;
    }
    return *left == '\0' && *right == '\0';
}

void copy_face_name(char* destination, const char* source) {
    std::size_t index = 0;
    for (; index + 1 < LF_FACESIZE && source[index] != '\0'; ++index) {
        destination[index] = source[index];
    }
    destination[index] = '\0';
}

void copy_face_name(WCHAR* destination, const char* source) {
    std::size_t index = 0;
    for (; index + 1 < LF_FACESIZE && source[index] != '\0'; ++index) {
        destination[index] =
            static_cast<WCHAR>(static_cast<unsigned char>(source[index]));
    }
    destination[index] = 0;
}

void narrow_face_name(const WCHAR* source, char* destination) {
    std::size_t index = 0;
    for (; index + 1 < LF_FACESIZE && source[index] != 0; ++index) {
        destination[index] = static_cast<char>(source[index] & 0xff);
    }
    destination[index] = '\0';
}

const FontFace& face_for_name(const char* name, BYTE pitch_family) {
    for (const auto& face : kFontFaces) {
        if (equal_face_name(name, face.name)) return face;
    }
    if ((pitch_family & FIXED_PITCH) != 0) return kFontFaces[2];
    return kFontFaces[1];
}

const FontFace& face_for_font(const GdiObject* font) {
    if (font == nullptr || font->kind != GdiKind::Font) return kFontFaces[1];
    char face_name[LF_FACESIZE] = {};
    narrow_face_name(font->font.logical.lfFaceName, face_name);
    return face_for_name(face_name, font->font.logical.lfPitchAndFamily);
}

int font_logical_height(const LOGFONTW& logical) {
    return logical.lfHeight == 0 ? -16 : logical.lfHeight;
}

int font_em_pixels(const LOGFONTW& logical) {
    const int height = font_logical_height(logical);
    if (height < 0) return -height;
    const int leading = (std::max)(1, height / 5);
    return (std::max)(1, height - leading);
}

int advance_units(const FontFace& face, unsigned char ch) {
    if (face.fixed_pitch) return 600;
    if (ch == ' ') return 250;
    if (ch >= '0' && ch <= '9') return 500;
    switch (ch) {
        case 'i':
        case 'j':
        case 'l':
        case 'I':
        case '.':
        case ',':
        case ':':
        case ';':
        case '!':
        case '\'':
            return 280;
        case 'm':
        case 'w':
        case 'M':
        case 'W':
            return 900;
        case 'f':
        case 'r':
        case 't':
            return 360;
        default:
            return face.pitch_family == (VARIABLE_PITCH | FF_ROMAN) ? 520 : 560;
    }
}

int scaled_advance(const FontFace& face, unsigned char ch, int em_pixels) {
    return (advance_units(face, ch) * em_pixels + kDesignUnitsPerEm / 2) /
           kDesignUnitsPerEm;
}

LOGFONTW selected_logical_font(HDC device_context) {
    auto* dc = dc_from_handle(device_context);
    auto* font = dc != nullptr ? object_from_handle(dc->dc.font) : nullptr;
    return font != nullptr && font->kind == GdiKind::Font
               ? font->font.logical
               : g_default_gui_font.font.logical;
}

GdiObject* selected_font_object(HDC device_context) {
    auto* dc = dc_from_handle(device_context);
    auto* font = dc != nullptr ? object_from_handle(dc->dc.font) : nullptr;
    return font != nullptr && font->kind == GdiKind::Font ? font : nullptr;
}

void fill_text_metric(const LOGFONTW& logical, const FontFace& face,
                      TEXTMETRICA& metric) {
    std::memset(&metric, 0, sizeof(metric));
    const int requested_height = font_logical_height(logical);
    const int em = font_em_pixels(logical);
    const int leading = requested_height > 0
                            ? (std::max)(0, requested_height - em)
                            : (std::max)(1, em / 5);
    metric.tmHeight = requested_height > 0 ? requested_height : em + leading;
    metric.tmInternalLeading = leading;
    metric.tmAscent = (metric.tmHeight * 4) / 5;
    metric.tmDescent = metric.tmHeight - metric.tmAscent;
    metric.tmAveCharWidth = scaled_advance(face, 'n', em);
    metric.tmMaxCharWidth = scaled_advance(face, 'W', em);
    metric.tmWeight = logical.lfWeight;
    metric.tmItalic = logical.lfItalic;
    metric.tmUnderlined = logical.lfUnderline;
    metric.tmStruckOut = logical.lfStrikeOut;
    metric.tmFirstChar = 32;
    metric.tmLastChar = 255;
    metric.tmDefaultChar = '?';
    metric.tmBreakChar = ' ';
    metric.tmPitchAndFamily =
        static_cast<BYTE>((face.fixed_pitch ? 0 : TMPF_FIXED_PITCH) |
                          TMPF_VECTOR | TMPF_TRUETYPE |
                          (face.pitch_family & 0xf0));
    metric.tmCharSet = face.charset;
    metric.tmOverhang = 0;
    metric.tmDigitizedAspectX = kLogicalPixelsPerInch;
    metric.tmDigitizedAspectY = kLogicalPixelsPerInch;
}

void make_enum_font(const FontFace& face, LOGFONTA& logical,
                    TEXTMETRICA& metric) {
    std::memset(&logical, 0, sizeof(logical));
    logical.lfHeight = -MulDiv(10, kLogicalPixelsPerInch, 72);
    logical.lfCharSet = face.charset;
    logical.lfPitchAndFamily = face.pitch_family;
    copy_face_name(logical.lfFaceName, face.name);
    LOGFONTW wide{};
    wide.lfHeight = logical.lfHeight;
    wide.lfCharSet = logical.lfCharSet;
    wide.lfPitchAndFamily = logical.lfPitchAndFamily;
    copy_face_name(wide.lfFaceName, face.name);
    fill_text_metric(wide, face, metric);
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
    g_system_font.font.logical.lfHeight = -16;
    g_system_font.font.logical.lfCharSet = ANSI_CHARSET;
    g_system_font.font.logical.lfPitchAndFamily = VARIABLE_PITCH | FF_SWISS;
    copy_face_name(g_system_font.font.logical.lfFaceName, "Helv");
    g_default_gui_font.font.logical = g_system_font.font.logical;
}

std::optional<COLORREF> pixel_at(const BitmapData& bitmap, int x, int y) {
    if (x < 0 || y < 0 || x >= bitmap.width || y >= bitmap.height) {
        return std::nullopt;
    }
    const auto offset = static_cast<std::size_t>(y * bitmap.width_bytes);
    if (bitmap.bits_per_pixel == 32) {
        DWORD value = 0;
        std::memcpy(&value, bitmap.bits.data() + offset + x * 4, sizeof(value));
        return value & 0x00ffffffu;
    }
    if (bitmap.bits_per_pixel == 24) {
        const unsigned char* pixel = bitmap.bits.data() + offset + x * 3;
        return RGB(pixel[2], pixel[1], pixel[0]);
    }
    if (bitmap.bits_per_pixel == 1) {
        const unsigned char byte = bitmap.bits[offset + x / 8];
        const bool set = (byte & (0x80u >> (x % 8))) != 0;
        return set ? RGB(255, 255, 255) : RGB(0, 0, 0);
    }
    return std::nullopt;
}

bool put_pixel(BitmapData& bitmap, int x, int y, COLORREF color) {
    if (x < 0 || y < 0 || x >= bitmap.width || y >= bitmap.height) return false;
    color &= 0x00ffffffu;
    const auto offset = static_cast<std::size_t>(y * bitmap.width_bytes);
    if (bitmap.bits_per_pixel == 32) {
        std::memcpy(bitmap.bits.data() + offset + x * 4, &color, sizeof(color));
        return true;
    }
    if (bitmap.bits_per_pixel == 24) {
        unsigned char* pixel = bitmap.bits.data() + offset + x * 3;
        pixel[0] = GetBValue(color);
        pixel[1] = GetGValue(color);
        pixel[2] = GetRValue(color);
        return true;
    }
    if (bitmap.bits_per_pixel == 1) {
        unsigned char& byte = bitmap.bits[offset + x / 8];
        const unsigned char mask = static_cast<unsigned char>(0x80u >> (x % 8));
        if ((color & 0x00ffffffu) != 0) {
            byte |= mask;
        } else {
            byte &= static_cast<unsigned char>(~mask);
        }
        return true;
    }
    return false;
}

GdiObject* bitmap_from_dc(GdiObject* dc) {
    auto* bitmap = dc != nullptr ? object_from_handle(dc->dc.bitmap) : nullptr;
    return bitmap != nullptr && bitmap->kind == GdiKind::Bitmap ? bitmap : nullptr;
}

COLORREF pattern_color(GdiObject& dc, int x, int y) {
    auto* brush = object_from_handle(dc.dc.brush);
    if (brush == nullptr || brush->kind != GdiKind::Brush) return RGB(255, 255, 255);
    auto* pattern = object_from_handle(brush->brush.pattern);
    if (pattern != nullptr && pattern->kind == GdiKind::Bitmap) {
        const int px = pattern->bitmap.width == 0 ? 0 : x % pattern->bitmap.width;
        const int py = pattern->bitmap.height == 0 ? 0 : y % pattern->bitmap.height;
        if (auto color = pixel_at(pattern->bitmap, px, py)) return *color;
    }
    return brush->brush.color;
}

COLORREF eval_rop(DWORD rop, COLORREF destination, COLORREF source,
                  COLORREF pattern) {
    rop &= ~CAPTUREBLT;
    destination &= 0x00ffffffu;
    source &= 0x00ffffffu;
    pattern &= 0x00ffffffu;
    switch (rop) {
        case BLACKNESS: return RGB(0, 0, 0);
        case WHITENESS: return RGB(255, 255, 255);
        case PATCOPY: return pattern;
        case PATINVERT: return pattern ^ destination;
        case DSTINVERT: return (~destination) & 0x00ffffffu;
        case SRCCOPY: return source;
        case NOTSRCCOPY: return (~source) & 0x00ffffffu;
        case SRCAND: return source & destination;
        case SRCINVERT: return source ^ destination;
        case ROP_DPSxx: return destination ^ pattern ^ source;
        case ROP_PDSxxn: return (~(pattern ^ destination ^ source)) & 0x00ffffffu;
        case ROP_PSo: return pattern | source;
        case ROP_PSno: return pattern | ((~source) & 0x00ffffffu);
        case ROP_Pn: return (~pattern) & 0x00ffffffu;
        case ROP_DPSnao: return destination | (pattern & ((~source) & 0x00ffffffu));
        case ROP_DSnx: return destination ^ ((~source) & 0x00ffffffu);
        case ROP_DPo: return destination | pattern;
        default: return destination;
    }
}

bool source_rop(DWORD rop) {
    rop &= ~CAPTUREBLT;
    switch (rop) {
        case SRCCOPY:
        case NOTSRCCOPY:
        case SRCAND:
        case SRCINVERT:
        case ROP_DPSxx:
        case ROP_PDSxxn:
        case ROP_PSo:
        case ROP_PSno:
        case ROP_DPSnao:
        case ROP_DSnx:
            return true;
        default:
            return false;
    }
}

void trace_escape(int escape, int count) {
    char message[96] = {};
    std::snprintf(message, sizeof(message),
                  "TRACE: Escape unsupported escape=%d count=%d\n", escape,
                  count);
    OutputDebugStringA(message);
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
    const FontFace& face = face_for_font(object);
    copy_face_name(object->font.logical.lfFaceName, face.name);
    object->font.logical.lfCharSet = face.charset;
    object->font.logical.lfPitchAndFamily = face.pitch_family;
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

BOOL GetTextMetricsA(HDC device_context, LPTEXTMETRICA text_metric) {
    if (dc_from_handle(device_context) == nullptr || text_metric == nullptr) {
        return FALSE;
    }
    const LOGFONTW logical = selected_logical_font(device_context);
    fill_text_metric(logical, face_for_font(selected_font_object(device_context)),
                     *text_metric);
    return TRUE;
}

BOOL GetTextMetricsW(HDC device_context, LPTEXTMETRICW text_metric) {
    return GetTextMetricsA(device_context, text_metric);
}

BOOL GetTextExtentPoint32A(HDC device_context, LPCSTR text, int count,
                           SIZE* size) {
    if (dc_from_handle(device_context) == nullptr || text == nullptr || count < 0 ||
        size == nullptr) {
        return FALSE;
    }
    const LOGFONTW logical = selected_logical_font(device_context);
    const FontFace& face = face_for_font(selected_font_object(device_context));
    const int em = font_em_pixels(logical);
    int width = 0;
    for (int index = 0; index < count; ++index) {
        width += scaled_advance(face, static_cast<unsigned char>(text[index]), em);
    }
    TEXTMETRICA metric{};
    fill_text_metric(logical, face, metric);
    size->cx = width;
    size->cy = metric.tmHeight;
    return TRUE;
}

BOOL GetCharWidthA(HDC device_context, UINT first_char, UINT last_char,
                   LPINT buffer) {
    if (dc_from_handle(device_context) == nullptr || buffer == nullptr ||
        last_char < first_char) {
        return FALSE;
    }
    const LOGFONTW logical = selected_logical_font(device_context);
    const FontFace& face = face_for_font(selected_font_object(device_context));
    const int em = font_em_pixels(logical);
    for (UINT ch = first_char; ch <= last_char; ++ch) {
        buffer[ch - first_char] =
            scaled_advance(face, static_cast<unsigned char>(ch), em);
    }
    return TRUE;
}

int EnumFontsA(HDC, LPCSTR face_name, FONTENUMPROCA enum_font_proc,
               LPARAM parameter) {
    if (enum_font_proc == nullptr) return 0;
    int count = 0;
    for (const auto& face : kFontFaces) {
        if (face_name != nullptr && face_name[0] != '\0' &&
            !equal_face_name(face_name, face.name)) {
            continue;
        }
        LOGFONTA logical{};
        TEXTMETRICA metric{};
        make_enum_font(face, logical, metric);
        ++count;
        if (enum_font_proc(&logical, &metric, TRUETYPE_FONTTYPE, parameter) == 0) {
            break;
        }
    }
    return count;
}

int EnumFontFamiliesExA(HDC device_context, LPLOGFONTA logfont,
                        FONTENUMPROCA enum_font_proc, LPARAM parameter,
                        DWORD) {
    const char* face_name =
        logfont != nullptr && logfont->lfFaceName[0] != '\0'
            ? logfont->lfFaceName
            : nullptr;
    return EnumFontsA(device_context, face_name, enum_font_proc, parameter);
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

int SetStretchBltMode(HDC device_context, int stretch_mode) {
    auto* dc = dc_from_handle(device_context);
    if (dc == nullptr) return 0;
    const int previous = dc->dc.stretch_mode;
    dc->dc.stretch_mode = stretch_mode;
    return previous;
}

int SetROP2(HDC device_context, int draw_mode) {
    auto* dc = dc_from_handle(device_context);
    if (dc == nullptr) return 0;
    const int previous = dc->dc.raster_operation;
    dc->dc.raster_operation = draw_mode;
    return previous;
}

COLORREF GetPixel(HDC device_context, int x, int y) {
    auto* bitmap = bitmap_from_dc(dc_from_handle(device_context));
    if (bitmap == nullptr) return CLR_INVALID;
    const auto color = pixel_at(bitmap->bitmap, x, y);
    return color ? *color : CLR_INVALID;
}

COLORREF SetPixel(HDC device_context, int x, int y, COLORREF color) {
    auto* bitmap = bitmap_from_dc(dc_from_handle(device_context));
    if (bitmap == nullptr) return CLR_INVALID;
    return put_pixel(bitmap->bitmap, x, y, color) ? (color & 0x00ffffffu)
                                                  : CLR_INVALID;
}

BOOL PatBlt(HDC device_context, int x, int y, int width, int height,
            DWORD raster_operation) {
    auto* dc = dc_from_handle(device_context);
    auto* destination = bitmap_from_dc(dc);
    if (dc == nullptr) return FALSE;
    if (destination == nullptr) return TRUE;
    const int left = (std::max)(x, dc->dc.clip.left);
    const int top = (std::max)(y, dc->dc.clip.top);
    const int right = (std::min)({x + width, dc->dc.clip.right,
                                  static_cast<int>(destination->bitmap.width)});
    const int bottom = (std::min)({y + height, dc->dc.clip.bottom,
                                   static_cast<int>(destination->bitmap.height)});
    if (right <= left || bottom <= top) return TRUE;
    for (int py = top; py < bottom; ++py) {
        for (int px = left; px < right; ++px) {
            const COLORREF old = pixel_at(destination->bitmap, px, py).value_or(0);
            put_pixel(destination->bitmap, px, py,
                      eval_rop(raster_operation, old, 0, pattern_color(*dc, px, py)));
        }
    }
    return TRUE;
}

BOOL BitBlt(HDC destination_dc, int x_destination, int y_destination, int width,
            int height, HDC source_dc, int x_source, int y_source,
            DWORD raster_operation) {
    auto* destination_context = dc_from_handle(destination_dc);
    auto* destination = bitmap_from_dc(destination_context);
    auto* source_context = dc_from_handle(source_dc);
    auto* source = bitmap_from_dc(source_context);
    if (destination_context == nullptr) return FALSE;
    if (destination == nullptr) return TRUE;
    if (source_rop(raster_operation) && source == nullptr) return FALSE;
    const int left = (std::max)(x_destination, destination_context->dc.clip.left);
    const int top = (std::max)(y_destination, destination_context->dc.clip.top);
    const int right = (std::min)({x_destination + width, destination_context->dc.clip.right,
                                  static_cast<int>(destination->bitmap.width)});
    const int bottom = (std::min)({y_destination + height, destination_context->dc.clip.bottom,
                                   static_cast<int>(destination->bitmap.height)});
    if (right <= left || bottom <= top) return TRUE;
    std::vector<COLORREF> source_pixels;
    if (source != nullptr) {
        source_pixels.reserve(static_cast<std::size_t>((right - left) * (bottom - top)));
        for (int py = top; py < bottom; ++py) {
            for (int px = left; px < right; ++px) {
                const int sx = x_source + (px - x_destination);
                const int sy = y_source + (py - y_destination);
                source_pixels.push_back(pixel_at(source->bitmap, sx, sy).value_or(0));
            }
        }
    }
    std::size_t index = 0;
    for (int py = top; py < bottom; ++py) {
        for (int px = left; px < right; ++px) {
            const COLORREF source_color =
                source != nullptr ? source_pixels[index++] : 0;
            const COLORREF old = pixel_at(destination->bitmap, px, py).value_or(0);
            put_pixel(destination->bitmap, px, py,
                      eval_rop(raster_operation, old, source_color,
                               pattern_color(*destination_context, px, py)));
        }
    }
    return TRUE;
}

BOOL StretchBlt(HDC destination, int x_destination, int y_destination,
                int width_destination, int height_destination, HDC source,
                int x_source, int y_source, int width_source, int height_source,
                DWORD raster_operation) {
    if (width_destination <= 0 || height_destination <= 0 ||
        width_source <= 0 || height_source <= 0) {
        return FALSE;
    }
    auto* destination_context = dc_from_handle(destination);
    auto* destination_bitmap = bitmap_from_dc(destination_context);
    auto* source_bitmap = bitmap_from_dc(dc_from_handle(source));
    if (destination_context == nullptr) return FALSE;
    if (destination_bitmap == nullptr) return TRUE;
    if (source_rop(raster_operation) && source_bitmap == nullptr) return FALSE;
    const int left = (std::max)(x_destination, destination_context->dc.clip.left);
    const int top = (std::max)(y_destination, destination_context->dc.clip.top);
    const int right = (std::min)({x_destination + width_destination,
                                  destination_context->dc.clip.right,
                                  static_cast<int>(destination_bitmap->bitmap.width)});
    const int bottom = (std::min)({y_destination + height_destination,
                                   destination_context->dc.clip.bottom,
                                   static_cast<int>(destination_bitmap->bitmap.height)});
    if (right <= left || bottom <= top) return TRUE;
    for (int py = top; py < bottom; ++py) {
        for (int px = left; px < right; ++px) {
            const int sx = x_source + ((px - x_destination) * width_source) /
                                          width_destination;
            const int sy = y_source + ((py - y_destination) * height_source) /
                                          height_destination;
            const COLORREF source_color =
                source_bitmap != nullptr ? pixel_at(source_bitmap->bitmap, sx, sy).value_or(0) : 0;
            const COLORREF old = pixel_at(destination_bitmap->bitmap, px, py).value_or(0);
            put_pixel(destination_bitmap->bitmap, px, py,
                      eval_rop(raster_operation, old, source_color,
                               pattern_color(*destination_context, px, py)));
        }
    }
    return TRUE;
}

int GetDIBits(HDC, HBITMAP bitmap_handle, UINT start_scan, UINT scan_lines,
              LPVOID bits, BITMAPINFO* bitmap_info, UINT) {
    auto* bitmap = object_from_handle(bitmap_handle);
    if (bitmap == nullptr || bitmap->kind != GdiKind::Bitmap) return 0;
    if (bitmap_info != nullptr) {
        bitmap_info->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bitmap_info->bmiHeader.biWidth = bitmap->bitmap.width;
        bitmap_info->bmiHeader.biHeight = bitmap->bitmap.height;
        bitmap_info->bmiHeader.biPlanes = bitmap->bitmap.planes;
        bitmap_info->bmiHeader.biBitCount = bitmap->bitmap.bits_per_pixel;
        bitmap_info->bmiHeader.biSizeImage =
            static_cast<DWORD>(bitmap->bitmap.bits.size());
    }
    if (bits == nullptr) return static_cast<int>(bitmap->bitmap.height);
    if (start_scan >= static_cast<UINT>(bitmap->bitmap.height)) return 0;
    const UINT rows = (std::min)(scan_lines,
        static_cast<UINT>((std::max<LONG>)(0, bitmap->bitmap.height - start_scan)));
    const auto row_bytes = static_cast<std::size_t>(bitmap->bitmap.width_bytes);
    std::memcpy(bits, bitmap->bitmap.bits.data() + row_bytes * start_scan,
                row_bytes * rows);
    return static_cast<int>(rows);
}

int GetDeviceCaps(HDC device_context, int index) {
    if (dc_from_handle(device_context) == nullptr) return 0;
    switch (index) {
        case HORZRES:
            return 640;
        case VERTRES:
            return 480;
        case HORZSIZE:
            return 169;
        case VERTSIZE:
            return 127;
        case NUMCOLORS:
            return 256;
        case BITSPIXEL:
            return 32;
        case PLANES:
            return 1;
        case LOGPIXELSX:
        case LOGPIXELSY:
            return kLogicalPixelsPerInch;
        case RASTERCAPS:
            return RC_BITBLT | RC_SCALING;
        case TEXTCAPS:
            return TC_SA_CONTIN | TC_RA_ABLE | TC_VA_ABLE;
        default:
            return 0;
    }
}

int Escape(HDC, int escape, int count, LPSTR, LPSTR output) {
    trace_escape(escape, count);
    switch (escape) {
        case STARTDOC:
            return SP_ERROR;
        case NEXTBAND:
            if (output != nullptr) std::memset(output, 0, sizeof(RECT));
            return 0;
        case QUERYESCSUPPORT:
        case GETPHYSPAGESIZE:
        case GETPRINTINGOFFSET:
        case GETSCALINGFACTOR:
        case kSetCharset:
        case kGetSetPaperBins:
        case kEnumPaperBins:
        case SETABORTPROC:
        case ENDDOC:
        case ABORTDOC:
        case DRAFTMODE:
        case BANDINFO:
        case PASSTHROUGH:
        case DRAWPATTERNRECT:
            return 0;
        default:
            return 0;
    }
}

}  // extern "C"
