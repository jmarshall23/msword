#include "windows.h"

#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define OPUS_GDI_MAGIC 0x47444932u
#define OPUS_LOGICAL_PIXELS_PER_INCH 96
#define OPUS_DESIGN_UNITS_PER_EM 1000
#define OPUS_GET_SET_PAPER_BINS 29
#define OPUS_ENUM_PAPER_BINS 31
#define OPUS_SET_CHARSET 772
/* Small fixed list; replace with region bands if callers exceed it. */
#define OPUS_MAX_EXCLUDED_CLIP_RECTS 8

enum GdiKind {
    kGdiKindDc,
    kGdiKindBitmap,
    kGdiKindBrush,
    kGdiKindPen,
    kGdiKindFont,
    kGdiKindRegion,
    kGdiKindMetafile,
};

struct BitmapData {
    LONG width;
    LONG height;
    WORD planes;
    WORD bits_per_pixel;
    LONG width_bytes;
    unsigned char* bits;
    size_t bits_size;
};

struct PenData {
    int style;
    int width;
    COLORREF color;
};

struct FontData {
    LOGFONTW logical;
};

struct BrushData {
    COLORREF color;
    HBITMAP pattern;
};

struct RegionData {
    RECT bounds;
};

struct DcState {
    HBITMAP bitmap;
    HBRUSH brush;
    HPEN pen;
    HFONT font;
    COLORREF text_color;
    COLORREF background_color;
    int background_mode;
    int map_mode;
    int raster_operation;
    int stretch_mode;
    RECT clip;
    RECT excluded_clips[OPUS_MAX_EXCLUDED_CLIP_RECTS];
    size_t excluded_clip_count;
    POINT current_position;
    POINT viewport_origin;
    SIZE viewport_extent;
    POINT window_origin;
    SIZE window_extent;
};

struct GdiObject {
    DWORD magic;
    enum GdiKind kind;
    BOOL stock;
    BOOL selected;
    struct BitmapData bitmap;
    struct BrushData brush;
    struct PenData pen;
    struct FontData font;
    struct RegionData region;
    struct DcState dc;
    BOOL metafile_context;
    struct DcState* saved;
    size_t saved_size;
    size_t saved_capacity;
};

struct FontFace {
    const char* name;
    BYTE charset;
    BYTE pitch_family;
    BOOL fixed_pitch;
};

static pthread_mutex_t g_gdi_lock = PTHREAD_MUTEX_INITIALIZER;
static struct GdiObject g_white_brush;
static struct GdiObject g_ltgray_brush;
static struct GdiObject g_gray_brush;
static struct GdiObject g_dkgray_brush;
static struct GdiObject g_black_brush;
static struct GdiObject g_black_pen;
static struct GdiObject g_white_pen;
static struct GdiObject g_system_font;
static struct GdiObject g_default_gui_font;
static struct GdiObject g_null_bitmap;
static unsigned char g_null_bitmap_bits[4];

static const struct FontFace kFontFaces[] = {
    {"Tms Rmn", ANSI_CHARSET, VARIABLE_PITCH | FF_ROMAN, FALSE},
    {"Helv", ANSI_CHARSET, VARIABLE_PITCH | FF_SWISS, FALSE},
    {"Courier", ANSI_CHARSET, FIXED_PITCH | FF_MODERN, TRUE},
    {"Symbol", SYMBOL_CHARSET, VARIABLE_PITCH | FF_DONTCARE, FALSE},
};

#include "font-golden.inc"

static int max_int(int left, int right) {
    return left > right ? left : right;
}

static int min_int(int left, int right) {
    return left < right ? left : right;
}

static LONG max_long(LONG left, LONG right) {
    return left > right ? left : right;
}

static int rect_region_type(const RECT* rect) {
    return rect->right <= rect->left || rect->bottom <= rect->top ? NULLREGION
                                                                 : SIMPLEREGION;
}

static BOOL rects_intersect(const RECT* left, const RECT* right) {
    return left->left < right->right && right->left < left->right &&
           left->top < right->bottom && right->top < left->bottom;
}

static BOOL rect_contains_rect(const RECT* outer, const RECT* inner) {
    return outer->left <= inner->left && outer->top <= inner->top &&
           outer->right >= inner->right && outer->bottom >= inner->bottom;
}

static BOOL point_in_rect(const RECT* rect, int x, int y) {
    return x >= rect->left && x < rect->right && y >= rect->top &&
           y < rect->bottom;
}

static BOOL pixel_allowed(const struct DcState* dc, int x, int y) {
    if (!point_in_rect(&dc->clip, x, y)) return FALSE;
    size_t index;
    for (index = 0; index < dc->excluded_clip_count; ++index) {
        if (point_in_rect(&dc->excluded_clips[index], x, y)) return FALSE;
    }
    return TRUE;
}

static int clip_region_type(const struct DcState* dc) {
    if (rect_region_type(&dc->clip) == NULLREGION) return NULLREGION;
    BOOL complex = FALSE;
    size_t index;
    for (index = 0; index < dc->excluded_clip_count; ++index) {
        if (!rects_intersect(&dc->clip, &dc->excluded_clips[index])) continue;
        if (rect_contains_rect(&dc->excluded_clips[index], &dc->clip)) {
            return NULLREGION;
        }
        complex = TRUE;
    }
    return complex ? COMPLEXREGION : SIMPLEREGION;
}

static void lock_gdi(void) {
    pthread_mutex_lock(&g_gdi_lock);
}

static void unlock_gdi(void) {
    pthread_mutex_unlock(&g_gdi_lock);
}

static void init_dc_state(struct DcState* state) {
    memset(state, 0, sizeof(*state));
    state->text_color = RGB(0, 0, 0);
    state->background_color = RGB(255, 255, 255);
    state->background_mode = OPAQUE;
    state->map_mode = MM_TEXT;
    state->raster_operation = R2_COPYPEN;
    state->stretch_mode = COLORONCOLOR;
    state->viewport_extent.cx = 1;
    state->viewport_extent.cy = 1;
    state->window_extent.cx = 1;
    state->window_extent.cy = 1;
}

static void init_gdi_object(struct GdiObject* object, enum GdiKind kind,
                            BOOL stock) {
    memset(object, 0, sizeof(*object));
    object->magic = OPUS_GDI_MAGIC;
    object->kind = kind;
    object->stock = stock;
    object->bitmap.width = 1;
    object->bitmap.height = 1;
    object->bitmap.planes = 1;
    object->bitmap.bits_per_pixel = 32;
    object->bitmap.width_bytes = 4;
    object->brush.color = RGB(255, 255, 255);
    object->pen.style = PS_SOLID;
    object->pen.width = 1;
    object->pen.color = RGB(0, 0, 0);
    init_dc_state(&object->dc);
}

static struct GdiObject* alloc_gdi_object(enum GdiKind kind) {
    struct GdiObject* object = (struct GdiObject*)calloc(1, sizeof(*object));
    if (object == NULL) return NULL;
    init_gdi_object(object, kind, FALSE);
    return object;
}

static void free_gdi_object(struct GdiObject* object) {
    if (object == NULL || object->stock) return;
    free(object->bitmap.bits);
    free(object->saved);
    free(object);
}

static struct GdiObject* object_from_handle(HGDIOBJ handle) {
    struct GdiObject* object = (struct GdiObject*)handle;
    return object != NULL && object->magic == OPUS_GDI_MAGIC ? object : NULL;
}

static struct GdiObject* dc_from_handle(HDC handle) {
    struct GdiObject* object = object_from_handle(handle);
    return object != NULL && object->kind == kGdiKindDc ? object : NULL;
}

static int bitmap_stride(int width, UINT bits_per_pixel) {
    return (int)(((width * bits_per_pixel + 31u) / 32u) * 4u);
}

static int ascii_lower(int ch) {
    return ch >= 'A' && ch <= 'Z' ? ch - 'A' + 'a' : ch;
}

static BOOL equal_face_name(const char* left, const char* right) {
    if (left == NULL || right == NULL) return FALSE;
    while (*left != '\0' && *right != '\0') {
        if (ascii_lower((unsigned char)*left) !=
            ascii_lower((unsigned char)*right)) {
            return FALSE;
        }
        ++left;
        ++right;
    }
    return *left == '\0' && *right == '\0';
}

static void copy_face_name_a(char* destination, const char* source) {
    size_t index = 0;
    for (; index + 1 < LF_FACESIZE && source[index] != '\0'; ++index) {
        destination[index] = source[index];
    }
    destination[index] = '\0';
}

static void copy_face_name_w(WCHAR* destination, const char* source) {
    size_t index = 0;
    for (; index + 1 < LF_FACESIZE && source[index] != '\0'; ++index) {
        destination[index] = (WCHAR)(unsigned char)source[index];
    }
    destination[index] = 0;
}

static void narrow_face_name(const WCHAR* source, char* destination) {
    size_t index = 0;
    for (; index + 1 < LF_FACESIZE && source[index] != 0; ++index) {
        destination[index] = (char)(source[index] & 0xff);
    }
    destination[index] = '\0';
}

static const struct FontFace* face_for_name(const char* name,
                                            BYTE pitch_family) {
    size_t i;
    for (i = 0; i < sizeof(kFontFaces) / sizeof(kFontFaces[0]); ++i) {
        if (equal_face_name(name, kFontFaces[i].name)) return &kFontFaces[i];
    }
    if ((pitch_family & FIXED_PITCH) != 0) return &kFontFaces[2];
    return &kFontFaces[1];
}

static const struct FontFace* face_for_font(const struct GdiObject* font) {
    char face_name[LF_FACESIZE] = {0};
    if (font == NULL || font->kind != kGdiKindFont) return &kFontFaces[1];
    narrow_face_name(font->font.logical.lfFaceName, face_name);
    return face_for_name(face_name, font->font.logical.lfPitchAndFamily);
}

static int font_logical_height(const LOGFONTW* logical) {
    return logical->lfHeight == 0 ? -16 : logical->lfHeight;
}

static int font_em_pixels(const LOGFONTW* logical) {
    const int height = font_logical_height(logical);
    if (height < 0) return -height;
    return max_int(1, height - max_int(1, height / 5));
}

static int advance_units(const struct FontFace* face, unsigned char ch) {
    if (face->fixed_pitch) return 600;
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
            return face->pitch_family == (VARIABLE_PITCH | FF_ROMAN) ? 520
                                                                      : 560;
    }
}

static int scaled_advance(const struct FontFace* face, unsigned char ch,
                          int em_pixels) {
    return (advance_units(face, ch) * em_pixels +
            OPUS_DESIGN_UNITS_PER_EM / 2) /
           OPUS_DESIGN_UNITS_PER_EM;
}

static int font_point_size(const LOGFONTW* logical) {
    const int em = font_em_pixels(logical);
    return (em * 72 + OPUS_LOGICAL_PIXELS_PER_INCH / 2) /
           OPUS_LOGICAL_PIXELS_PER_INCH;
}

static const struct FontMetricRow* font_metric_row(
    const struct FontFace* face, const LOGFONTW* logical) {
    const int points = font_point_size(logical);
    size_t index;
    for (index = 0; index < sizeof(kFontMetricRows) / sizeof(kFontMetricRows[0]);
         ++index) {
        if (kFontMetricRows[index].points == points &&
            equal_face_name(kFontMetricRows[index].face, face->name)) {
            return &kFontMetricRows[index];
        }
    }
    return NULL;
}

static int font_advance(const struct FontFace* face, const LOGFONTW* logical,
                        unsigned char ch) {
    const struct FontMetricRow* row = font_metric_row(face, logical);
    if (row != NULL && ch >= 32 && ch < 127) return row->widths[ch - 32];
    return scaled_advance(face, ch, font_em_pixels(logical));
}

static LOGFONTW selected_logical_font(HDC device_context) {
    struct GdiObject* dc = dc_from_handle(device_context);
    struct GdiObject* font = dc != NULL ? object_from_handle(dc->dc.font) : NULL;
    return font != NULL && font->kind == kGdiKindFont
               ? font->font.logical
               : g_default_gui_font.font.logical;
}

static struct GdiObject* selected_font_object(HDC device_context) {
    struct GdiObject* dc = dc_from_handle(device_context);
    struct GdiObject* font = dc != NULL ? object_from_handle(dc->dc.font) : NULL;
    return font != NULL && font->kind == kGdiKindFont ? font : NULL;
}

static void fill_text_metric(const LOGFONTW* logical,
                             const struct FontFace* face,
                             TEXTMETRICA* metric) {
    memset(metric, 0, sizeof(*metric));
    const int requested_height = font_logical_height(logical);
    const int em = font_em_pixels(logical);
    const int leading = requested_height > 0 ? max_int(0, requested_height - em)
                                             : max_int(1, em / 5);
    const struct FontMetricRow* row = font_metric_row(face, logical);
    metric->tmHeight =
        row != NULL ? row->ascent + row->descent
                    : (requested_height > 0 ? requested_height : em + leading);
    metric->tmInternalLeading = leading;
    metric->tmAscent = row != NULL ? row->ascent : (metric->tmHeight * 4) / 5;
    metric->tmDescent =
        row != NULL ? row->descent : metric->tmHeight - metric->tmAscent;
    metric->tmAveCharWidth = font_advance(face, logical, 'n');
    metric->tmMaxCharWidth = font_advance(face, logical, 'W');
    metric->tmWeight = logical->lfWeight;
    metric->tmItalic = logical->lfItalic;
    metric->tmUnderlined = logical->lfUnderline;
    metric->tmStruckOut = logical->lfStrikeOut;
    metric->tmFirstChar = 32;
    metric->tmLastChar = 255;
    metric->tmDefaultChar = '?';
    metric->tmBreakChar = ' ';
    metric->tmPitchAndFamily =
        (BYTE)((face->fixed_pitch ? 0 : TMPF_FIXED_PITCH) | TMPF_VECTOR |
               TMPF_TRUETYPE | (face->pitch_family & 0xf0));
    metric->tmCharSet = face->charset;
    metric->tmOverhang = row != NULL ? row->overhang : 0;
    metric->tmDigitizedAspectX = OPUS_LOGICAL_PIXELS_PER_INCH;
    metric->tmDigitizedAspectY = OPUS_LOGICAL_PIXELS_PER_INCH;
}

static void make_enum_font(const struct FontFace* face, LOGFONTA* logical,
                           TEXTMETRICA* metric) {
    LOGFONTW wide;
    memset(logical, 0, sizeof(*logical));
    logical->lfHeight = -MulDiv(10, OPUS_LOGICAL_PIXELS_PER_INCH, 72);
    logical->lfCharSet = face->charset;
    logical->lfPitchAndFamily = face->pitch_family;
    copy_face_name_a(logical->lfFaceName, face->name);
    memset(&wide, 0, sizeof(wide));
    wide.lfHeight = logical->lfHeight;
    wide.lfCharSet = logical->lfCharSet;
    wide.lfPitchAndFamily = logical->lfPitchAndFamily;
    copy_face_name_w(wide.lfFaceName, face->name);
    fill_text_metric(&wide, face, metric);
}

static HBITMAP create_bitmap(int width, int height, UINT planes,
                             UINT bits_per_pixel, const void* bits) {
    if (width <= 0 || height == 0 || planes == 0 || bits_per_pixel == 0) {
        return NULL;
    }
    struct GdiObject* object = alloc_gdi_object(kGdiKindBitmap);
    if (object == NULL) return NULL;
    object->bitmap.width = width;
    object->bitmap.height = height < 0 ? -height : height;
    object->bitmap.planes = (WORD)planes;
    object->bitmap.bits_per_pixel = (WORD)bits_per_pixel;
    object->bitmap.width_bytes = bitmap_stride(width, bits_per_pixel);
    object->bitmap.bits_size =
        (size_t)object->bitmap.width_bytes * (size_t)object->bitmap.height;
    object->bitmap.bits = (unsigned char*)calloc(1, object->bitmap.bits_size);
    if (object->bitmap.bits == NULL) {
        free_gdi_object(object);
        return NULL;
    }
    if (bits != NULL) {
        memcpy(object->bitmap.bits, bits, object->bitmap.bits_size);
    }
    return (HBITMAP)object;
}

static HGDIOBJ select_object(struct GdiObject* dc, struct GdiObject* object) {
    HGDIOBJ* slot = NULL;
    switch (object->kind) {
        case kGdiKindBitmap: slot = (HGDIOBJ*)&dc->dc.bitmap; break;
        case kGdiKindBrush: slot = (HGDIOBJ*)&dc->dc.brush; break;
        case kGdiKindPen: slot = (HGDIOBJ*)&dc->dc.pen; break;
        case kGdiKindFont: slot = (HGDIOBJ*)&dc->dc.font; break;
        case kGdiKindRegion:
        case kGdiKindMetafile:
        case kGdiKindDc: return NULL;
    }
    HGDIOBJ previous = *slot;
    struct GdiObject* previous_object = object_from_handle(previous);
    if (previous_object != NULL) previous_object->selected = FALSE;
    *slot = (HGDIOBJ)object;
    object->selected = TRUE;
    return previous;
}

static void mark_selected(struct DcState* state, BOOL selected) {
    HGDIOBJ handles[] = {(HGDIOBJ)state->bitmap, (HGDIOBJ)state->brush,
                         (HGDIOBJ)state->pen, (HGDIOBJ)state->font};
    size_t i;
    for (i = 0; i < sizeof(handles) / sizeof(handles[0]); ++i) {
        struct GdiObject* object = object_from_handle(handles[i]);
        if (object != NULL) object->selected = selected;
    }
}

static void init_stock_objects(void) {
    static BOOL initialized = FALSE;
    if (initialized) return;
    initialized = TRUE;
    init_gdi_object(&g_white_brush, kGdiKindBrush, TRUE);
    init_gdi_object(&g_ltgray_brush, kGdiKindBrush, TRUE);
    init_gdi_object(&g_gray_brush, kGdiKindBrush, TRUE);
    init_gdi_object(&g_dkgray_brush, kGdiKindBrush, TRUE);
    init_gdi_object(&g_black_brush, kGdiKindBrush, TRUE);
    init_gdi_object(&g_black_pen, kGdiKindPen, TRUE);
    init_gdi_object(&g_white_pen, kGdiKindPen, TRUE);
    init_gdi_object(&g_system_font, kGdiKindFont, TRUE);
    init_gdi_object(&g_default_gui_font, kGdiKindFont, TRUE);
    init_gdi_object(&g_null_bitmap, kGdiKindBitmap, TRUE);
    g_null_bitmap.bitmap.bits = g_null_bitmap_bits;
    g_null_bitmap.bitmap.bits_size = sizeof(g_null_bitmap_bits);
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
    copy_face_name_w(g_system_font.font.logical.lfFaceName, "Helv");
    g_default_gui_font.font.logical = g_system_font.font.logical;
}

static BOOL pixel_at(const struct BitmapData* bitmap, int x, int y,
                     COLORREF* color) {
    if (x < 0 || y < 0 || x >= bitmap->width || y >= bitmap->height) {
        return FALSE;
    }
    const size_t offset = (size_t)y * (size_t)bitmap->width_bytes;
    if (bitmap->bits_per_pixel == 32) {
        DWORD value = 0;
        memcpy(&value, bitmap->bits + offset + (size_t)x * 4, sizeof(value));
        *color = value & 0x00ffffffu;
        return TRUE;
    }
    if (bitmap->bits_per_pixel == 24) {
        const unsigned char* pixel = bitmap->bits + offset + (size_t)x * 3;
        *color = RGB(pixel[2], pixel[1], pixel[0]);
        return TRUE;
    }
    if (bitmap->bits_per_pixel == 1) {
        const unsigned char byte = bitmap->bits[offset + (size_t)x / 8];
        const BOOL set = (byte & (0x80u >> (x % 8))) != 0;
        *color = set ? RGB(255, 255, 255) : RGB(0, 0, 0);
        return TRUE;
    }
    return FALSE;
}

static COLORREF pixel_or_black(const struct BitmapData* bitmap, int x, int y) {
    COLORREF color = 0;
    return pixel_at(bitmap, x, y, &color) ? color : 0;
}

static BOOL put_pixel(struct BitmapData* bitmap, int x, int y, COLORREF color) {
    if (x < 0 || y < 0 || x >= bitmap->width || y >= bitmap->height) {
        return FALSE;
    }
    color &= 0x00ffffffu;
    const size_t offset = (size_t)y * (size_t)bitmap->width_bytes;
    if (bitmap->bits_per_pixel == 32) {
        memcpy(bitmap->bits + offset + (size_t)x * 4, &color, sizeof(color));
        return TRUE;
    }
    if (bitmap->bits_per_pixel == 24) {
        unsigned char* pixel = bitmap->bits + offset + (size_t)x * 3;
        pixel[0] = GetBValue(color);
        pixel[1] = GetGValue(color);
        pixel[2] = GetRValue(color);
        return TRUE;
    }
    if (bitmap->bits_per_pixel == 1) {
        unsigned char* byte = bitmap->bits + offset + (size_t)x / 8;
        const unsigned char mask = (unsigned char)(0x80u >> (x % 8));
        if ((color & 0x00ffffffu) != 0) {
            *byte |= mask;
        } else {
            *byte &= (unsigned char)~mask;
        }
        return TRUE;
    }
    return FALSE;
}

static struct GdiObject* bitmap_from_dc(struct GdiObject* dc) {
    struct GdiObject* bitmap = dc != NULL ? object_from_handle(dc->dc.bitmap)
                                          : NULL;
    return bitmap != NULL && bitmap->kind == kGdiKindBitmap ? bitmap : NULL;
}

static COLORREF brush_pixel_color(struct GdiObject* brush, int x, int y) {
    if (brush == NULL || brush->kind != kGdiKindBrush) {
        return RGB(255, 255, 255);
    }
    struct GdiObject* pattern = object_from_handle(brush->brush.pattern);
    if (pattern != NULL && pattern->kind == kGdiKindBitmap) {
        COLORREF color = 0;
        const int px =
            pattern->bitmap.width == 0 ? 0 : x % pattern->bitmap.width;
        const int py =
            pattern->bitmap.height == 0 ? 0 : y % pattern->bitmap.height;
        if (pixel_at(&pattern->bitmap, px, py, &color)) return color;
    }
    return brush->brush.color;
}

static COLORREF pattern_color(struct GdiObject* dc, int x, int y) {
    return brush_pixel_color(object_from_handle(dc->dc.brush), x, y);
}

static COLORREF pen_color(struct GdiObject* dc) {
    struct GdiObject* pen = object_from_handle(dc->dc.pen);
    return pen != NULL && pen->kind == kGdiKindPen ? pen->pen.color
                                                   : RGB(0, 0, 0);
}

static BOOL put_pixel_clipped(struct GdiObject* dc, struct GdiObject* bitmap,
                              int x, int y, COLORREF color) {
    if (!pixel_allowed(&dc->dc, x, y)) return FALSE;
    return put_pixel(&bitmap->bitmap, x, y, color);
}

static int valid_map_mode(int map_mode) {
    switch (map_mode) {
        case MM_TEXT:
        case MM_LOMETRIC:
        case MM_HIMETRIC:
        case MM_LOENGLISH:
        case MM_HIENGLISH:
        case MM_TWIPS:
        case MM_ISOTROPIC:
        case MM_ANISOTROPIC:
            return TRUE;
        default:
            return FALSE;
    }
}

static COLORREF eval_rop(DWORD rop, COLORREF destination, COLORREF source,
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
        case ROP_PDSxxn:
            return (~(pattern ^ destination ^ source)) & 0x00ffffffu;
        case ROP_PSo: return pattern | source;
        case ROP_PSno: return pattern | ((~source) & 0x00ffffffu);
        case ROP_Pn: return (~pattern) & 0x00ffffffu;
        case ROP_DPSnao:
            return destination | (pattern & ((~source) & 0x00ffffffu));
        case ROP_DSnx: return destination ^ ((~source) & 0x00ffffffu);
        case ROP_DPo: return destination | pattern;
        default: return destination;
    }
}

static BOOL source_rop(DWORD rop) {
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
            return TRUE;
        default:
            return FALSE;
    }
}

static void trace_escape(int escape, int count) {
    char message[96] = {0};
    snprintf(message, sizeof(message),
             "TRACE: Escape unsupported escape=%d count=%d\n", escape, count);
    OutputDebugStringA(message);
}

static WORD read_u16(const unsigned char* bytes, size_t offset) {
    return (WORD)(bytes[offset] | (bytes[offset + 1] << 8));
}

static DWORD read_u32(const unsigned char* bytes, size_t offset) {
    return (DWORD)read_u16(bytes, offset) |
           ((DWORD)read_u16(bytes, offset + 2) << 16u);
}

static LONG read_i32(const unsigned char* bytes, size_t offset) {
    return (LONG)read_u32(bytes, offset);
}

HBITMAP OpusGdiCreateBitmapFromBmpBytes(const unsigned char* bytes,
                                        unsigned int size) {
    if (bytes == NULL || size < 54 || bytes[0] != 'B' || bytes[1] != 'M') {
        return NULL;
    }
    const DWORD bits_offset = read_u32(bytes, 10);
    const LONG width = read_i32(bytes, 18);
    const LONG height = read_i32(bytes, 22);
    const WORD planes = read_u16(bytes, 26);
    const WORD bits_per_pixel = read_u16(bytes, 28);
    const size_t byte_count =
        (size_t)bitmap_stride(width, bits_per_pixel) *
        (size_t)(height < 0 ? -height : height);
    if (bits_offset >= size || bits_offset + byte_count > size) return NULL;
    return create_bitmap(width, height, planes, bits_per_pixel,
                         bytes + bits_offset);
}

HGDIOBJ GetStockObject(int object) {
    HGDIOBJ result = NULL;
    lock_gdi();
    init_stock_objects();
    switch (object) {
        case WHITE_BRUSH: result = &g_white_brush; break;
        case LTGRAY_BRUSH: result = &g_ltgray_brush; break;
        case GRAY_BRUSH: result = &g_gray_brush; break;
        case DKGRAY_BRUSH: result = &g_dkgray_brush; break;
        case BLACK_BRUSH: result = &g_black_brush; break;
        case WHITE_PEN: result = &g_white_pen; break;
        case BLACK_PEN: result = &g_black_pen; break;
        case SYSTEM_FONT: result = &g_system_font; break;
        case DEFAULT_GUI_FONT: result = &g_default_gui_font; break;
        default: result = NULL; break;
    }
    unlock_gdi();
    return result;
}

HDC CreateCompatibleDC(HDC device_context) {
    (void)device_context;
    struct GdiObject* object = alloc_gdi_object(kGdiKindDc);
    if (object == NULL) return NULL;
    object->dc.brush = (HBRUSH)GetStockObject(WHITE_BRUSH);
    object->dc.pen = (HPEN)GetStockObject(BLACK_PEN);
    object->dc.font = (HFONT)GetStockObject(SYSTEM_FONT);
    object->dc.bitmap = (HBITMAP)&g_null_bitmap;
    object->dc.clip.left = 0;
    object->dc.clip.top = 0;
    object->dc.clip.right = 640;
    object->dc.clip.bottom = 480;
    return (HDC)object;
}

HDC CreateDCA(LPCSTR driver, LPCSTR device, LPCSTR output,
              LPCVOID init_data) {
    (void)driver;
    (void)device;
    (void)output;
    (void)init_data;
    return CreateCompatibleDC(NULL);
}

HDC CreateICA(LPCSTR driver, LPCSTR device, LPCSTR output,
              LPCVOID init_data) {
    (void)driver;
    (void)device;
    (void)output;
    (void)init_data;
    return CreateCompatibleDC(NULL);
}

HDC CreateMetaFile(LPSTR file_name) {
    (void)file_name;
    HDC device_context = CreateCompatibleDC(NULL);
    struct GdiObject* dc = dc_from_handle(device_context);
    if (dc != NULL) dc->metafile_context = TRUE;
    return device_context;
}

HMETAFILE CloseMetaFile(HDC device_context) {
    HMETAFILE result = NULL;
    lock_gdi();
    struct GdiObject* dc = dc_from_handle(device_context);
    if (dc != NULL && dc->metafile_context) {
        dc->metafile_context = FALSE;
        dc->kind = kGdiKindMetafile;
        result = (HMETAFILE)dc;
    }
    unlock_gdi();
    return result;
}

BOOL DeleteDC(HDC device_context) {
    BOOL result = FALSE;
    lock_gdi();
    struct GdiObject* dc = dc_from_handle(device_context);
    if (dc != NULL && !dc->stock) {
        mark_selected(&dc->dc, FALSE);
        dc->magic = 0;
        free_gdi_object(dc);
        result = TRUE;
    }
    unlock_gdi();
    return result;
}

HBITMAP CreateBitmap(int width, int height, UINT planes, UINT bits_per_pixel,
                     LPCVOID bits) {
    return create_bitmap(width, height, planes, bits_per_pixel, bits);
}

HBITMAP CreateBitmapIndirect(const BITMAP* bitmap) {
    if (bitmap == NULL) return NULL;
    return create_bitmap(bitmap->bmWidth, bitmap->bmHeight, bitmap->bmPlanes,
                         bitmap->bmBitsPixel, bitmap->bmBits);
}

HBITMAP CreateCompatibleBitmap(HDC device_context, int width, int height) {
    (void)device_context;
    return create_bitmap(width, height, 1, 32, NULL);
}

HBITMAP CreateDIBitmap(HDC device_context, const BITMAPINFOHEADER* header,
                       DWORD init, LPCVOID bits, const BITMAPINFO* info,
                       UINT usage) {
    (void)device_context;
    (void)info;
    (void)usage;
    if (header == NULL) return NULL;
    return create_bitmap(header->biWidth, header->biHeight, header->biPlanes,
                         header->biBitCount,
                         (init & CBM_INIT) != 0 ? bits : NULL);
}

HBRUSH CreateSolidBrush(DWORD color) {
    struct GdiObject* object = alloc_gdi_object(kGdiKindBrush);
    if (object == NULL) return NULL;
    object->brush.color = color;
    return (HBRUSH)object;
}

HBRUSH CreatePatternBrush(HBITMAP bitmap) {
    if (object_from_handle(bitmap) == NULL) return NULL;
    struct GdiObject* object = alloc_gdi_object(kGdiKindBrush);
    if (object == NULL) return NULL;
    object->brush.pattern = bitmap;
    return (HBRUSH)object;
}

HPEN CreatePen(int style, int width, DWORD color) {
    struct GdiObject* object = alloc_gdi_object(kGdiKindPen);
    if (object == NULL) return NULL;
    object->pen.style = style;
    object->pen.width = width;
    object->pen.color = color;
    return (HPEN)object;
}

HPEN CreatePenIndirect(const LOGPEN* log_pen) {
    if (log_pen == NULL) return NULL;
    return CreatePen(log_pen->lopnStyle, log_pen->lopnWidth.x,
                     log_pen->lopnColor);
}

HRGN CreateRectRgn(int left, int top, int right, int bottom) {
    struct GdiObject* object = alloc_gdi_object(kGdiKindRegion);
    if (object == NULL) return NULL;
    object->region.bounds.left = left;
    object->region.bounds.top = top;
    object->region.bounds.right = right;
    object->region.bounds.bottom = bottom;
    return (HRGN)object;
}

HFONT CreateFontIndirectW(const LOGFONTW* logical_font) {
    struct GdiObject* object = alloc_gdi_object(kGdiKindFont);
    if (object == NULL) return NULL;
    if (logical_font != NULL) object->font.logical = *logical_font;
    const struct FontFace* face = face_for_font(object);
    copy_face_name_w(object->font.logical.lfFaceName, face->name);
    object->font.logical.lfCharSet = face->charset;
    object->font.logical.lfPitchAndFamily = face->pitch_family;
    return (HFONT)object;
}

HFONT CreateFontIndirectA(const LOGFONTA* logical_font) {
    LOGFONTW wide;
    memset(&wide, 0, sizeof(wide));
    if (logical_font != NULL) {
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
        size_t i;
        for (i = 0; i < LF_FACESIZE; ++i) {
            wide.lfFaceName[i] =
                (WCHAR)(unsigned char)logical_font->lfFaceName[i];
        }
    }
    return CreateFontIndirectW(&wide);
}

BOOL GetTextMetricsA(HDC device_context, LPTEXTMETRICA text_metric) {
    if (dc_from_handle(device_context) == NULL || text_metric == NULL) {
        return FALSE;
    }
    const LOGFONTW logical = selected_logical_font(device_context);
    fill_text_metric(&logical, face_for_font(selected_font_object(device_context)),
                     text_metric);
    return TRUE;
}

BOOL GetTextMetricsW(HDC device_context, LPTEXTMETRICW text_metric) {
    return GetTextMetricsA(device_context, (LPTEXTMETRICA)text_metric);
}

BOOL GetTextExtentPoint32A(HDC device_context, LPCSTR text, int count,
                           SIZE* size) {
    if (dc_from_handle(device_context) == NULL || text == NULL || count < 0 ||
        size == NULL) {
        return FALSE;
    }
    const LOGFONTW logical = selected_logical_font(device_context);
    const struct FontFace* face = face_for_font(selected_font_object(device_context));
    int width = 0;
    int index;
    for (index = 0; index < count; ++index) {
        width += font_advance(face, &logical, (unsigned char)text[index]);
    }
    TEXTMETRICA metric;
    fill_text_metric(&logical, face, &metric);
    size->cx = width;
    size->cy = metric.tmHeight;
    return TRUE;
}

static int wide_text_count(LPCWSTR text, int count) {
    if (count >= 0) return count;
    int length = 0;
    while (text[length] != 0) ++length;
    return length;
}

static int narrow_text_count(LPCSTR text, int count) {
    if (count >= 0) return count;
    int length = 0;
    while (text[length] != 0) ++length;
    return length;
}

static WCHAR* widen_text(LPCSTR text, int count) {
    WCHAR* wide = (WCHAR*)calloc((size_t)count, sizeof(wide[0]));
    if (wide == NULL) return NULL;
    int index;
    for (index = 0; index < count; ++index) {
        wide[index] = (unsigned char)text[index];
    }
    return wide;
}

static BOOL text_extent_w(HDC device_context, LPCWSTR text, int count,
                          SIZE* size) {
    if (dc_from_handle(device_context) == NULL || text == NULL || count < 0 ||
        size == NULL) {
        return FALSE;
    }
    const LOGFONTW logical = selected_logical_font(device_context);
    const struct FontFace* face =
        face_for_font(selected_font_object(device_context));
    int width = 0;
    int index;
    for (index = 0; index < count; ++index) {
        width += font_advance(face, &logical,
                              (unsigned char)(text[index] & 0xff));
    }
    TEXTMETRICA metric;
    fill_text_metric(&logical, face, &metric);
    size->cx = width;
    size->cy = metric.tmHeight;
    return TRUE;
}

static void fill_text_background(struct GdiObject* dc, struct GdiObject* bitmap,
                                 int x, int y, SIZE size) {
    const int left = max_int(x, dc->dc.clip.left);
    const int top = max_int(y, dc->dc.clip.top);
    const int right =
        min_int(min_int(x + size.cx, dc->dc.clip.right), bitmap->bitmap.width);
    const int bottom =
        min_int(min_int(y + size.cy, dc->dc.clip.bottom), bitmap->bitmap.height);
    int py;
    for (py = top; py < bottom; ++py) {
        int px;
        for (px = left; px < right; ++px) {
            put_pixel_clipped(dc, bitmap, px, py, dc->dc.background_color);
        }
    }
}

static void draw_text_cell(struct GdiObject* dc, struct GdiObject* bitmap,
                           int x, int y, int advance, int height,
                           COLORREF color) {
    if (advance <= 0 || height <= 0) return;
    const int stroke_right = x + max_int(1, advance - 1);
    const int stroke_bottom = y + max_int(1, height - 1);
    int py;
    for (py = y + 1; py < stroke_bottom; ++py) {
        put_pixel_clipped(dc, bitmap, x, py, color);
        put_pixel_clipped(dc, bitmap, stroke_right - 1, py, color);
    }
    int px;
    for (px = x; px < stroke_right; ++px) {
        put_pixel_clipped(dc, bitmap, px, y + 1, color);
        put_pixel_clipped(dc, bitmap, px, stroke_bottom - 1, color);
    }
}

static void fill_color_rect(struct GdiObject* dc, struct GdiObject* bitmap,
                            const RECT* rect, COLORREF color) {
    const int left = max_int(rect->left, dc->dc.clip.left);
    const int top = max_int(rect->top, dc->dc.clip.top);
    const int right = min_int(min_int(rect->right, dc->dc.clip.right),
                              bitmap->bitmap.width);
    const int bottom = min_int(min_int(rect->bottom, dc->dc.clip.bottom),
                               bitmap->bitmap.height);
    int py;
    for (py = top; py < bottom; ++py) {
        int px;
        for (px = left; px < right; ++px) {
            put_pixel_clipped(dc, bitmap, px, py, color);
        }
    }
}

BOOL TextOutW(HDC device_context, int x, int y, LPCWSTR text, int count) {
    struct GdiObject* dc = dc_from_handle(device_context);
    if (dc == NULL || text == NULL || count < 0) return FALSE;
    SIZE size;
    if (!text_extent_w(device_context, text, count, &size)) return FALSE;
    struct GdiObject* bitmap = bitmap_from_dc(dc);
    if (bitmap == NULL) return TRUE;
    if (dc->dc.background_mode == OPAQUE) {
        fill_text_background(dc, bitmap, x, y, size);
    }
    const LOGFONTW logical = selected_logical_font(device_context);
    const struct FontFace* face =
        face_for_font(selected_font_object(device_context));
    int cursor = x;
    int index;
    for (index = 0; index < count; ++index) {
        const int ch = (int)(text[index] & 0xff);
        const int advance = font_advance(face, &logical, (unsigned char)ch);
        if (ch != ' ' && ch != '\t') {
            draw_text_cell(dc, bitmap, cursor, y, advance, size.cy,
                           dc->dc.text_color);
        }
        cursor += advance;
    }
    return TRUE;
}

BOOL TextOutA(HDC device_context, int x, int y, LPCSTR text, int count) {
    if (text == NULL) return FALSE;
    const int length = narrow_text_count(text, count);
    if (length == 0) return dc_from_handle(device_context) != NULL;
    WCHAR* wide = widen_text(text, length);
    if (wide == NULL) return FALSE;
    const BOOL result = TextOutW(device_context, x, y, wide, length);
    free(wide);
    return result;
}

BOOL TextOut(HDC device_context, int x, int y, LPCSTR text, int count) {
    return TextOutA(device_context, x, y, text, count);
}

BOOL ExtTextOutA(HDC device_context, int x, int y, UINT options,
                 const RECT* rect, LPCSTR text, UINT count, const int* dx) {
    (void)dx;
    struct GdiObject* dc = dc_from_handle(device_context);
    if (dc == NULL || text == NULL) return FALSE;
    struct GdiObject* bitmap = bitmap_from_dc(dc);
    if (bitmap != NULL && (options & ETO_OPAQUE) != 0 && rect != NULL) {
        fill_color_rect(dc, bitmap, rect, dc->dc.background_color);
    }
    return TextOutA(device_context, x, y, text, (int)count);
}

BOOL ExtTextOut(HDC device_context, int x, int y, UINT options,
                const RECT* rect, LPCSTR text, UINT count, const int* dx) {
    return ExtTextOutA(device_context, x, y, options, rect, text, count, dx);
}

int DrawTextW(HDC device_context, LPCWSTR text, int count, RECT* rect,
              UINT format) {
    if (rect == NULL || text == NULL) return 0;
    const int length = wide_text_count(text, count);
    SIZE size;
    if (!text_extent_w(device_context, text, length, &size)) return 0;
    int x = rect->left;
    int y = rect->top;
    if ((format & DT_CENTER) != 0) {
        x = rect->left + (rect->right - rect->left - size.cx) / 2;
    }
    if ((format & DT_VCENTER) != 0) {
        y = rect->top + (rect->bottom - rect->top - size.cy) / 2;
    }
    return TextOutW(device_context, x, y, text, length) ? size.cy : 0;
}

BOOL GetCharWidthA(HDC device_context, UINT first_char, UINT last_char,
                   LPINT buffer) {
    if (dc_from_handle(device_context) == NULL || buffer == NULL ||
        last_char < first_char) {
        return FALSE;
    }
    const LOGFONTW logical = selected_logical_font(device_context);
    const struct FontFace* face = face_for_font(selected_font_object(device_context));
    UINT ch;
    for (ch = first_char; ch <= last_char; ++ch) {
        buffer[ch - first_char] = font_advance(face, &logical, (unsigned char)ch);
    }
    return TRUE;
}

int EnumFontsA(HDC device_context, LPCSTR face_name,
               FONTENUMPROCA enum_font_proc, LPARAM parameter) {
    (void)device_context;
    if (enum_font_proc == NULL) return 0;
    int count = 0;
    size_t i;
    for (i = 0; i < sizeof(kFontFaces) / sizeof(kFontFaces[0]); ++i) {
        if (face_name != NULL && face_name[0] != '\0' &&
            !equal_face_name(face_name, kFontFaces[i].name)) {
            continue;
        }
        LOGFONTA logical;
        TEXTMETRICA metric;
        make_enum_font(&kFontFaces[i], &logical, &metric);
        ++count;
        if (enum_font_proc(&logical, &metric, TRUETYPE_FONTTYPE, parameter) ==
            0) {
            break;
        }
    }
    return count;
}

int EnumFontFamiliesExA(HDC device_context, LPLOGFONTA logfont,
                        FONTENUMPROCA enum_font_proc, LPARAM parameter,
                        DWORD flags) {
    (void)flags;
    const char* face_name =
        logfont != NULL && logfont->lfFaceName[0] != '\0'
            ? logfont->lfFaceName
            : NULL;
    return EnumFontsA(device_context, face_name, enum_font_proc, parameter);
}

HGDIOBJ SelectObject(HDC device_context, HGDIOBJ object) {
    HGDIOBJ result = NULL;
    lock_gdi();
    struct GdiObject* dc = dc_from_handle(device_context);
    struct GdiObject* gdi_object = object_from_handle(object);
    if (dc != NULL && gdi_object != NULL) {
        result = select_object(dc, gdi_object);
    }
    unlock_gdi();
    return result;
}

HGDIOBJ GetCurrentObject(HDC device_context, UINT object_type) {
    HGDIOBJ result = NULL;
    lock_gdi();
    struct GdiObject* dc = dc_from_handle(device_context);
    if (dc != NULL) {
        switch (object_type) {
            case OBJ_BITMAP: result = dc->dc.bitmap; break;
            case OBJ_BRUSH: result = dc->dc.brush; break;
            case OBJ_PEN: result = dc->dc.pen; break;
            case OBJ_FONT: result = dc->dc.font; break;
            default: result = NULL; break;
        }
    }
    unlock_gdi();
    return result;
}

BOOL DeleteObject(HGDIOBJ object) {
    BOOL result = FALSE;
    lock_gdi();
    struct GdiObject* gdi_object = object_from_handle(object);
    if (gdi_object != NULL && !gdi_object->stock && !gdi_object->selected) {
        gdi_object->magic = 0;
        free_gdi_object(gdi_object);
        result = TRUE;
    }
    unlock_gdi();
    return result;
}

int SaveDC(HDC device_context) {
    int result = 0;
    lock_gdi();
    struct GdiObject* dc = dc_from_handle(device_context);
    if (dc != NULL) {
        if (dc->saved_size == dc->saved_capacity) {
            const size_t capacity =
                dc->saved_capacity == 0 ? 4 : dc->saved_capacity * 2;
            struct DcState* saved = (struct DcState*)realloc(
                dc->saved, capacity * sizeof(dc->saved[0]));
            if (saved == NULL) {
                unlock_gdi();
                return 0;
            }
            dc->saved = saved;
            dc->saved_capacity = capacity;
        }
        dc->saved[dc->saved_size++] = dc->dc;
        result = (int)dc->saved_size;
    }
    unlock_gdi();
    return result;
}

BOOL RestoreDC(HDC device_context, int saved_dc) {
    BOOL result = FALSE;
    lock_gdi();
    struct GdiObject* dc = dc_from_handle(device_context);
    if (dc == NULL || dc->saved_size == 0) {
        unlock_gdi();
        return FALSE;
    }
    size_t index = 0;
    if (saved_dc < 0) {
        const int relative = (int)dc->saved_size + saved_dc;
        if (relative < 0) {
            unlock_gdi();
            return FALSE;
        }
        index = (size_t)relative;
    } else {
        if (saved_dc == 0 || (size_t)saved_dc > dc->saved_size) {
            unlock_gdi();
            return FALSE;
        }
        index = (size_t)(saved_dc - 1);
    }
    mark_selected(&dc->dc, FALSE);
    dc->dc = dc->saved[index];
    mark_selected(&dc->dc, TRUE);
    dc->saved_size = index;
    result = TRUE;
    unlock_gdi();
    return result;
}

int GetObjectA(HANDLE object, int buffer_size, LPVOID object_data) {
    struct GdiObject* bitmap = object_from_handle(object);
    if (bitmap == NULL || bitmap->kind != kGdiKindBitmap ||
        object_data == NULL || buffer_size < (int)sizeof(BITMAP)) {
        return 0;
    }
    BITMAP info;
    memset(&info, 0, sizeof(info));
    info.bmWidth = bitmap->bitmap.width;
    info.bmHeight = bitmap->bitmap.height;
    info.bmWidthBytes = bitmap->bitmap.width_bytes;
    info.bmPlanes = bitmap->bitmap.planes;
    info.bmBitsPixel = bitmap->bitmap.bits_per_pixel;
    info.bmBits = bitmap->bitmap.bits;
    memcpy(object_data, &info, sizeof(info));
    return (int)sizeof(info);
}

LONG GetBitmapBits(HBITMAP bitmap_handle, LONG count, LPVOID bits) {
    struct GdiObject* bitmap = object_from_handle(bitmap_handle);
    if (bitmap == NULL || bitmap->kind != kGdiKindBitmap || bits == NULL ||
        count <= 0) {
        return 0;
    }
    const LONG copied =
        (LONG)(bitmap->bitmap.bits_size < (size_t)count
                   ? bitmap->bitmap.bits_size
                   : (size_t)count);
    memcpy(bits, bitmap->bitmap.bits, (size_t)copied);
    return copied;
}

BOOL GetBitmapDimensionEx(HBITMAP bitmap_handle, SIZE* size) {
    struct GdiObject* bitmap = object_from_handle(bitmap_handle);
    if (bitmap == NULL || bitmap->kind != kGdiKindBitmap || size == NULL) {
        return FALSE;
    }
    size->cx = bitmap->bitmap.width;
    size->cy = bitmap->bitmap.height;
    return TRUE;
}

BOOL SetBitmapDimensionEx(HBITMAP bitmap_handle, int width, int height,
                          SIZE* previous) {
    struct GdiObject* bitmap = object_from_handle(bitmap_handle);
    if (bitmap == NULL || bitmap->kind != kGdiKindBitmap) return FALSE;
    if (previous != NULL) {
        previous->cx = bitmap->bitmap.width;
        previous->cy = bitmap->bitmap.height;
    }
    bitmap->bitmap.width = width;
    bitmap->bitmap.height = height;
    return TRUE;
}

int SetStretchBltMode(HDC device_context, int stretch_mode) {
    struct GdiObject* dc = dc_from_handle(device_context);
    if (dc == NULL) return 0;
    const int previous = dc->dc.stretch_mode;
    dc->dc.stretch_mode = stretch_mode;
    return previous;
}

int SetROP2(HDC device_context, int draw_mode) {
    struct GdiObject* dc = dc_from_handle(device_context);
    if (dc == NULL) return 0;
    const int previous = dc->dc.raster_operation;
    dc->dc.raster_operation = draw_mode;
    return previous;
}

int SetBkMode(HDC device_context, int background_mode) {
    if (background_mode != OPAQUE && background_mode != TRANSPARENT) return 0;
    struct GdiObject* dc = dc_from_handle(device_context);
    if (dc == NULL) return 0;
    const int previous = dc->dc.background_mode;
    dc->dc.background_mode = background_mode;
    return previous;
}

COLORREF GetBkColor(HDC device_context) {
    struct GdiObject* dc = dc_from_handle(device_context);
    return dc != NULL ? dc->dc.background_color : CLR_INVALID;
}

COLORREF SetBkColor(HDC device_context, COLORREF color) {
    struct GdiObject* dc = dc_from_handle(device_context);
    if (dc == NULL) return CLR_INVALID;
    const COLORREF previous = dc->dc.background_color;
    dc->dc.background_color = color & 0x00ffffffu;
    return previous;
}

COLORREF GetTextColor(HDC device_context) {
    struct GdiObject* dc = dc_from_handle(device_context);
    return dc != NULL ? dc->dc.text_color : CLR_INVALID;
}

COLORREF SetTextColor(HDC device_context, COLORREF color) {
    struct GdiObject* dc = dc_from_handle(device_context);
    if (dc == NULL) return CLR_INVALID;
    const COLORREF previous = dc->dc.text_color;
    dc->dc.text_color = color & 0x00ffffffu;
    return previous;
}

int SetMapMode(HDC device_context, int map_mode) {
    if (!valid_map_mode(map_mode)) return 0;
    struct GdiObject* dc = dc_from_handle(device_context);
    if (dc == NULL) return 0;
    const int previous = dc->dc.map_mode;
    dc->dc.map_mode = map_mode;
    return previous;
}

BOOL GetViewportExtEx(HDC device_context, SIZE* size) {
    struct GdiObject* dc = dc_from_handle(device_context);
    if (dc == NULL || size == NULL) return FALSE;
    *size = dc->dc.viewport_extent;
    return TRUE;
}

BOOL GetViewportOrgEx(HDC device_context, POINT* point) {
    struct GdiObject* dc = dc_from_handle(device_context);
    if (dc == NULL || point == NULL) return FALSE;
    *point = dc->dc.viewport_origin;
    return TRUE;
}

BOOL SetViewportExtEx(HDC device_context, int width, int height,
                      SIZE* previous) {
    struct GdiObject* dc = dc_from_handle(device_context);
    if (dc == NULL) return FALSE;
    if (previous != NULL) *previous = dc->dc.viewport_extent;
    dc->dc.viewport_extent.cx = width;
    dc->dc.viewport_extent.cy = height;
    return TRUE;
}

BOOL SetViewportOrgEx(HDC device_context, int x, int y, POINT* previous) {
    struct GdiObject* dc = dc_from_handle(device_context);
    if (dc == NULL) return FALSE;
    if (previous != NULL) *previous = dc->dc.viewport_origin;
    dc->dc.viewport_origin.x = x;
    dc->dc.viewport_origin.y = y;
    return TRUE;
}

BOOL SetWindowExtEx(HDC device_context, int width, int height, SIZE* previous) {
    struct GdiObject* dc = dc_from_handle(device_context);
    if (dc == NULL) return FALSE;
    if (previous != NULL) *previous = dc->dc.window_extent;
    dc->dc.window_extent.cx = width;
    dc->dc.window_extent.cy = height;
    return TRUE;
}

BOOL SetWindowOrgEx(HDC device_context, int x, int y, POINT* previous) {
    struct GdiObject* dc = dc_from_handle(device_context);
    if (dc == NULL) return FALSE;
    if (previous != NULL) *previous = dc->dc.window_origin;
    dc->dc.window_origin.x = x;
    dc->dc.window_origin.y = y;
    return TRUE;
}

BOOL MoveToEx(HDC device_context, int x, int y, POINT* previous) {
    struct GdiObject* dc = dc_from_handle(device_context);
    if (dc == NULL) return FALSE;
    if (previous != NULL) *previous = dc->dc.current_position;
    dc->dc.current_position.x = x;
    dc->dc.current_position.y = y;
    return TRUE;
}

static void draw_line_pixels(struct GdiObject* dc, struct GdiObject* bitmap,
                             POINT start, POINT end, COLORREF color) {
    const int dx = end.x - start.x;
    const int dy = end.y - start.y;
    const int steps = max_int(abs(dx), abs(dy));
    if (steps == 0) {
        put_pixel_clipped(dc, bitmap, start.x, start.y, color);
        return;
    }
    int step;
    for (step = 0; step <= steps; ++step) {
        const int px = start.x + (dx * step) / steps;
        const int py = start.y + (dy * step) / steps;
        put_pixel_clipped(dc, bitmap, px, py, color);
    }
}

BOOL LineTo(HDC device_context, int x, int y) {
    struct GdiObject* dc = dc_from_handle(device_context);
    struct GdiObject* bitmap = bitmap_from_dc(dc);
    if (dc == NULL) return FALSE;
    const POINT start = dc->dc.current_position;
    dc->dc.current_position.x = x;
    dc->dc.current_position.y = y;
    if (bitmap == NULL) return TRUE;
    POINT end;
    end.x = x;
    end.y = y;
    draw_line_pixels(dc, bitmap, start, end, pen_color(dc));
    return TRUE;
}

int FillRect(HDC device_context, const RECT* rect, HBRUSH brush_handle) {
    struct GdiObject* dc = dc_from_handle(device_context);
    struct GdiObject* bitmap = bitmap_from_dc(dc);
    struct GdiObject* brush = object_from_handle(brush_handle);
    if (dc == NULL || rect == NULL || brush == NULL ||
        brush->kind != kGdiKindBrush) {
        return 0;
    }
    if (bitmap == NULL) return 1;
    const int left = max_int(rect->left, dc->dc.clip.left);
    const int top = max_int(rect->top, dc->dc.clip.top);
    const int right =
        min_int(min_int(rect->right, dc->dc.clip.right), bitmap->bitmap.width);
    const int bottom = min_int(min_int(rect->bottom, dc->dc.clip.bottom),
                               bitmap->bitmap.height);
    int py;
    for (py = top; py < bottom; ++py) {
        int px;
        for (px = left; px < right; ++px) {
            put_pixel_clipped(dc, bitmap, px, py,
                              brush_pixel_color(brush, px, py));
        }
    }
    return 1;
}

int FrameRect(HDC device_context, const RECT* rect, HBRUSH brush) {
    if (rect == NULL) return 0;
    RECT edge;
    edge = *rect;
    edge.bottom = edge.top + 1;
    if (!FillRect(device_context, &edge, brush)) return 0;
    edge = *rect;
    edge.top = edge.bottom - 1;
    if (!FillRect(device_context, &edge, brush)) return 0;
    edge = *rect;
    edge.right = edge.left + 1;
    if (!FillRect(device_context, &edge, brush)) return 0;
    edge = *rect;
    edge.left = edge.right - 1;
    return FillRect(device_context, &edge, brush);
}

static BOOL fill_edge(HDC device_context, const RECT* rect, HBRUSH brush,
                      UINT flags, UINT side) {
    RECT edge;
    if ((flags & side) == 0) return TRUE;
    edge = *rect;
    switch (side) {
        case BF_LEFT: edge.right = edge.left + 1; break;
        case BF_TOP: edge.bottom = edge.top + 1; break;
        case BF_RIGHT: edge.left = edge.right - 1; break;
        case BF_BOTTOM: edge.top = edge.bottom - 1; break;
        default: return FALSE;
    }
    return FillRect(device_context, &edge, brush) != 0;
}

BOOL DrawEdge(HDC device_context, RECT* rect, UINT edge, UINT flags) {
    if (rect == NULL) return FALSE;
    const COLORREF light = edge == EDGE_SUNKEN ? RGB(128, 128, 128)
                                               : RGB(255, 255, 255);
    const COLORREF shadow = edge == EDGE_SUNKEN ? RGB(255, 255, 255)
                                                : RGB(128, 128, 128);
    if (edge != EDGE_RAISED && edge != EDGE_SUNKEN) return FALSE;
    HBRUSH light_brush = CreateSolidBrush(light);
    HBRUSH shadow_brush = CreateSolidBrush(shadow);
    BOOL result = light_brush != NULL && shadow_brush != NULL &&
                  fill_edge(device_context, rect, light_brush, flags,
                            BF_LEFT) &&
                  fill_edge(device_context, rect, light_brush, flags,
                            BF_TOP) &&
                  fill_edge(device_context, rect, shadow_brush, flags,
                            BF_RIGHT) &&
                  fill_edge(device_context, rect, shadow_brush, flags,
                            BF_BOTTOM);
    DeleteObject(light_brush);
    DeleteObject(shadow_brush);
    return result;
}

BOOL Rectangle(HDC device_context, int left, int top, int right, int bottom) {
    struct GdiObject* dc = dc_from_handle(device_context);
    if (dc == NULL) return FALSE;
    struct GdiObject* brush = object_from_handle(dc->dc.brush);
    struct GdiObject* bitmap = bitmap_from_dc(dc);
    if (bitmap == NULL) return TRUE;
    if (right <= left || bottom <= top) return TRUE;
    if (brush != NULL && brush->kind == kGdiKindBrush &&
        right - left > 2 && bottom - top > 2) {
        RECT interior;
        interior.left = left + 1;
        interior.top = top + 1;
        interior.right = right - 1;
        interior.bottom = bottom - 1;
        FillRect(device_context, &interior, (HBRUSH)brush);
    }
    const COLORREF color = pen_color(dc);
    int x;
    int y;
    for (x = left; x < right; ++x) {
        put_pixel_clipped(dc, bitmap, x, top, color);
        put_pixel_clipped(dc, bitmap, x, bottom - 1, color);
    }
    for (y = top; y < bottom; ++y) {
        put_pixel_clipped(dc, bitmap, left, y, color);
        put_pixel_clipped(dc, bitmap, right - 1, y, color);
    }
    return TRUE;
}

static BOOL ellipse_contains(int x, int y, int left, int top, int right,
                             int bottom) {
    const long width = right - left;
    const long height = bottom - top;
    const long center_x2 = left + right - 1;
    const long center_y2 = top + bottom - 1;
    const long dx2 = 2L * x - center_x2;
    const long dy2 = 2L * y - center_y2;
    const long width2 = width * width;
    const long height2 = height * height;
    return dx2 * dx2 * height2 + dy2 * dy2 * width2 <= width2 * height2;
}

BOOL Ellipse(HDC device_context, int left, int top, int right, int bottom) {
    struct GdiObject* dc = dc_from_handle(device_context);
    if (dc == NULL) return FALSE;
    struct GdiObject* bitmap = bitmap_from_dc(dc);
    if (bitmap == NULL) return TRUE;
    if (right <= left || bottom <= top) return TRUE;
    struct GdiObject* brush = object_from_handle(dc->dc.brush);
    const COLORREF outline = pen_color(dc);
    const int draw_left = max_int(left, dc->dc.clip.left);
    const int draw_top = max_int(top, dc->dc.clip.top);
    const int draw_right =
        min_int(min_int(right, dc->dc.clip.right), bitmap->bitmap.width);
    const int draw_bottom =
        min_int(min_int(bottom, dc->dc.clip.bottom), bitmap->bitmap.height);
    int y;
    for (y = draw_top; y < draw_bottom; ++y) {
        int x;
        for (x = draw_left; x < draw_right; ++x) {
            if (!ellipse_contains(x, y, left, top, right, bottom)) continue;
            if (!ellipse_contains(x - 1, y, left, top, right, bottom) ||
                !ellipse_contains(x + 1, y, left, top, right, bottom) ||
                !ellipse_contains(x, y - 1, left, top, right, bottom) ||
                !ellipse_contains(x, y + 1, left, top, right, bottom)) {
                put_pixel_clipped(dc, bitmap, x, y, outline);
            } else if (brush != NULL && brush->kind == kGdiKindBrush) {
                put_pixel_clipped(dc, bitmap, x, y,
                                  brush_pixel_color(brush, x, y));
            }
        }
    }
    return TRUE;
}

static BOOL point_in_polygon(const POINT* points, int count, double x,
                             double y) {
    BOOL inside = FALSE;
    int i;
    int j = count - 1;
    for (i = 0; i < count; ++i) {
        const double yi = points[i].y;
        const double yj = points[j].y;
        if ((yi > y) != (yj > y)) {
            const double xi = points[i].x;
            const double xj = points[j].x;
            const double edge_x = xi + (y - yi) * (xj - xi) / (yj - yi);
            if (x < edge_x) inside = !inside;
        }
        j = i;
    }
    return inside;
}

BOOL Polygon(HDC device_context, const POINT* points, int count) {
    struct GdiObject* dc = dc_from_handle(device_context);
    if (dc == NULL || points == NULL || count < 2) return FALSE;
    struct GdiObject* bitmap = bitmap_from_dc(dc);
    if (bitmap == NULL) return TRUE;
    struct GdiObject* brush = object_from_handle(dc->dc.brush);
    int left = points[0].x;
    int top = points[0].y;
    int right = points[0].x + 1;
    int bottom = points[0].y + 1;
    int i;
    for (i = 1; i < count; ++i) {
        left = min_int(left, points[i].x);
        top = min_int(top, points[i].y);
        right = max_int(right, points[i].x + 1);
        bottom = max_int(bottom, points[i].y + 1);
    }
    left = max_int(left, dc->dc.clip.left);
    top = max_int(top, dc->dc.clip.top);
    right = min_int(min_int(right, dc->dc.clip.right), bitmap->bitmap.width);
    bottom = min_int(min_int(bottom, dc->dc.clip.bottom), bitmap->bitmap.height);
    if (brush != NULL && brush->kind == kGdiKindBrush) {
        int y;
        for (y = top; y < bottom; ++y) {
            int x;
            for (x = left; x < right; ++x) {
                if (point_in_polygon(points, count, x + 0.5, y + 0.5)) {
                    put_pixel_clipped(dc, bitmap, x, y,
                                      brush_pixel_color(brush, x, y));
                }
            }
        }
    }
    const COLORREF outline = pen_color(dc);
    for (i = 0; i < count; ++i) {
        draw_line_pixels(dc, bitmap, points[i], points[(i + 1) % count],
                         outline);
    }
    return TRUE;
}

COLORREF GetPixel(HDC device_context, int x, int y) {
    struct GdiObject* bitmap = bitmap_from_dc(dc_from_handle(device_context));
    COLORREF color = 0;
    if (bitmap == NULL) return CLR_INVALID;
    return pixel_at(&bitmap->bitmap, x, y, &color) ? color : CLR_INVALID;
}

COLORREF SetPixel(HDC device_context, int x, int y, COLORREF color) {
    struct GdiObject* bitmap = bitmap_from_dc(dc_from_handle(device_context));
    if (bitmap == NULL) return CLR_INVALID;
    return put_pixel(&bitmap->bitmap, x, y, color) ? (color & 0x00ffffffu)
                                                   : CLR_INVALID;
}

int IntersectClipRect(HDC device_context, int left, int top, int right,
                      int bottom) {
    int result = ERROR;
    lock_gdi();
    struct GdiObject* dc = dc_from_handle(device_context);
    if (dc != NULL) {
        dc->dc.clip.left = max_int(dc->dc.clip.left, left);
        dc->dc.clip.top = max_int(dc->dc.clip.top, top);
        dc->dc.clip.right = min_int(dc->dc.clip.right, right);
        dc->dc.clip.bottom = min_int(dc->dc.clip.bottom, bottom);
        result = clip_region_type(&dc->dc);
    }
    unlock_gdi();
    return result;
}

int ExcludeClipRect(HDC device_context, int left, int top, int right,
                    int bottom) {
    int result = ERROR;
    lock_gdi();
    struct GdiObject* dc = dc_from_handle(device_context);
    if (dc != NULL) {
        RECT excluded;
        excluded.left = left;
        excluded.top = top;
        excluded.right = right;
        excluded.bottom = bottom;
        if (rect_region_type(&excluded) != NULLREGION &&
            rects_intersect(&dc->dc.clip, &excluded)) {
            if (dc->dc.excluded_clip_count >= OPUS_MAX_EXCLUDED_CLIP_RECTS) {
                unlock_gdi();
                return ERROR;
            }
            dc->dc.excluded_clips[dc->dc.excluded_clip_count++] = excluded;
        }
        result = clip_region_type(&dc->dc);
    }
    unlock_gdi();
    return result;
}

BOOL PatBlt(HDC device_context, int x, int y, int width, int height,
            DWORD raster_operation) {
    struct GdiObject* dc = dc_from_handle(device_context);
    struct GdiObject* destination = bitmap_from_dc(dc);
    if (dc == NULL) return FALSE;
    if (destination == NULL) return TRUE;
    const int left = max_int(x, dc->dc.clip.left);
    const int top = max_int(y, dc->dc.clip.top);
    const int right =
        min_int(min_int(x + width, dc->dc.clip.right), destination->bitmap.width);
    const int bottom =
        min_int(min_int(y + height, dc->dc.clip.bottom), destination->bitmap.height);
    if (right <= left || bottom <= top) return TRUE;
    int py;
    for (py = top; py < bottom; ++py) {
        int px;
        for (px = left; px < right; ++px) {
            if (!pixel_allowed(&dc->dc, px, py)) continue;
            const COLORREF old = pixel_or_black(&destination->bitmap, px, py);
            put_pixel(&destination->bitmap, px, py,
                      eval_rop(raster_operation, old, 0,
                               pattern_color(dc, px, py)));
        }
    }
    return TRUE;
}

BOOL BitBlt(HDC destination_dc, int x_destination, int y_destination, int width,
            int height, HDC source_dc, int x_source, int y_source,
            DWORD raster_operation) {
    struct GdiObject* destination_context = dc_from_handle(destination_dc);
    struct GdiObject* destination = bitmap_from_dc(destination_context);
    struct GdiObject* source_context = dc_from_handle(source_dc);
    struct GdiObject* source = bitmap_from_dc(source_context);
    if (destination_context == NULL) return FALSE;
    if (destination == NULL) return TRUE;
    if (source_rop(raster_operation) && source == NULL) return FALSE;
    const int left = max_int(x_destination, destination_context->dc.clip.left);
    const int top = max_int(y_destination, destination_context->dc.clip.top);
    const int right = min_int(
        min_int(x_destination + width, destination_context->dc.clip.right),
        destination->bitmap.width);
    const int bottom = min_int(
        min_int(y_destination + height, destination_context->dc.clip.bottom),
        destination->bitmap.height);
    if (right <= left || bottom <= top) return TRUE;

    const size_t source_count = (size_t)(right - left) * (size_t)(bottom - top);
    COLORREF* source_pixels = NULL;
    if (source != NULL) {
        source_pixels = (COLORREF*)calloc(source_count, sizeof(source_pixels[0]));
        if (source_pixels == NULL) return FALSE;
        size_t index = 0;
        int py;
        for (py = top; py < bottom; ++py) {
            int px;
            for (px = left; px < right; ++px) {
                const int sx = x_source + (px - x_destination);
                const int sy = y_source + (py - y_destination);
                source_pixels[index++] = pixel_or_black(&source->bitmap, sx, sy);
            }
        }
    }

    size_t index = 0;
    int py;
    for (py = top; py < bottom; ++py) {
        int px;
        for (px = left; px < right; ++px) {
            const COLORREF source_color =
                source != NULL ? source_pixels[index++] : 0;
            if (!pixel_allowed(&destination_context->dc, px, py)) continue;
            const COLORREF old = pixel_or_black(&destination->bitmap, px, py);
            put_pixel(&destination->bitmap, px, py,
                      eval_rop(raster_operation, old, source_color,
                               pattern_color(destination_context, px, py)));
        }
    }
    free(source_pixels);
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
    struct GdiObject* destination_context = dc_from_handle(destination);
    struct GdiObject* destination_bitmap = bitmap_from_dc(destination_context);
    struct GdiObject* source_bitmap = bitmap_from_dc(dc_from_handle(source));
    if (destination_context == NULL) return FALSE;
    if (destination_bitmap == NULL) return TRUE;
    if (source_rop(raster_operation) && source_bitmap == NULL) return FALSE;
    const int left = max_int(x_destination, destination_context->dc.clip.left);
    const int top = max_int(y_destination, destination_context->dc.clip.top);
    const int right = min_int(
        min_int(x_destination + width_destination,
                destination_context->dc.clip.right),
        destination_bitmap->bitmap.width);
    const int bottom = min_int(
        min_int(y_destination + height_destination,
                destination_context->dc.clip.bottom),
        destination_bitmap->bitmap.height);
    if (right <= left || bottom <= top) return TRUE;
    int py;
    for (py = top; py < bottom; ++py) {
        int px;
        for (px = left; px < right; ++px) {
            if (!pixel_allowed(&destination_context->dc, px, py)) continue;
            const int sx = x_source + ((px - x_destination) * width_source) /
                                          width_destination;
            const int sy = y_source + ((py - y_destination) * height_source) /
                                          height_destination;
            const COLORREF source_color =
                source_bitmap != NULL ? pixel_or_black(&source_bitmap->bitmap, sx, sy)
                                      : 0;
            const COLORREF old =
                pixel_or_black(&destination_bitmap->bitmap, px, py);
            put_pixel(&destination_bitmap->bitmap, px, py,
                      eval_rop(raster_operation, old, source_color,
                               pattern_color(destination_context, px, py)));
        }
    }
    return TRUE;
}

int GetDIBits(HDC device_context, HBITMAP bitmap_handle, UINT start_scan,
              UINT scan_lines, LPVOID bits, BITMAPINFO* bitmap_info,
              UINT usage) {
    (void)device_context;
    (void)usage;
    struct GdiObject* bitmap = object_from_handle(bitmap_handle);
    if (bitmap == NULL || bitmap->kind != kGdiKindBitmap) return 0;
    if (bitmap_info != NULL) {
        bitmap_info->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bitmap_info->bmiHeader.biWidth = bitmap->bitmap.width;
        bitmap_info->bmiHeader.biHeight = bitmap->bitmap.height;
        bitmap_info->bmiHeader.biPlanes = bitmap->bitmap.planes;
        bitmap_info->bmiHeader.biBitCount = bitmap->bitmap.bits_per_pixel;
        bitmap_info->bmiHeader.biSizeImage = (DWORD)bitmap->bitmap.bits_size;
    }
    if (bits == NULL) return (int)bitmap->bitmap.height;
    if (start_scan >= (UINT)bitmap->bitmap.height) return 0;
    const UINT rows =
        min_int(scan_lines,
                (UINT)max_long(0, bitmap->bitmap.height - start_scan));
    const size_t row_bytes = (size_t)bitmap->bitmap.width_bytes;
    memcpy(bits, bitmap->bitmap.bits + row_bytes * start_scan, row_bytes * rows);
    return (int)rows;
}

int GetDeviceCaps(HDC device_context, int index) {
    if (dc_from_handle(device_context) == NULL) return 0;
    switch (index) {
        case HORZRES: return 640;
        case VERTRES: return 480;
        case HORZSIZE: return 169;
        case VERTSIZE: return 127;
        case NUMCOLORS: return 256;
        case BITSPIXEL: return 32;
        case PLANES: return 1;
        case LOGPIXELSX:
        case LOGPIXELSY:
            return OPUS_LOGICAL_PIXELS_PER_INCH;
        case RASTERCAPS:
            return RC_BITBLT | RC_SCALING;
        case TEXTCAPS:
            return TC_SA_CONTIN | TC_RA_ABLE | TC_VA_ABLE;
        default:
            return 0;
    }
}

int Escape(HDC device_context, int escape, int count, LPSTR input,
           LPSTR output) {
    (void)device_context;
    (void)input;
    trace_escape(escape, count);
    switch (escape) {
        case STARTDOC:
            return SP_ERROR;
        case NEXTBAND:
            if (output != NULL) memset(output, 0, sizeof(RECT));
            return 0;
        case QUERYESCSUPPORT:
        case GETPHYSPAGESIZE:
        case GETPRINTINGOFFSET:
        case GETSCALINGFACTOR:
        case OPUS_SET_CHARSET:
        case OPUS_GET_SET_PAPER_BINS:
        case OPUS_ENUM_PAPER_BINS:
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
