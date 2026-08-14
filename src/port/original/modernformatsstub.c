#include "opus_x64_compat.h"

#include <ctype.h>
#include <stddef.h>
#include <string.h>

static int has_extension(const char* path, const char* extension) {
    size_t path_length;
    size_t extension_length;
    size_t index;

    if (path == NULL || extension == NULL) return 0;
    path_length = strlen(path);
    extension_length = strlen(extension);
    if (path_length < extension_length) return 0;
    path += path_length - extension_length;
    for (index = 0; index < extension_length; ++index) {
        if (tolower((unsigned char)path[index]) !=
            tolower((unsigned char)extension[index]))
            return 0;
    }
    return 1;
}

static void copy_text(char* output, const char* text, int capacity) {
    size_t length;

    if (output == NULL || capacity <= 0) return;
    length = strlen(text);
    if (length >= (size_t)capacity) length = (size_t)(capacity - 1);
    memcpy(output, text, length);
    output[length] = '\0';
}

int OpusModernPathIsDocx(const char* path) {
    return has_extension(path, ".docx");
}

int OpusModernPathIsOdt(const char* path) {
    return has_extension(path, ".odt");
}

int OpusModernDocxToRtfFile(const char* input, const char* output) {
    return 0;
}

int OpusModernDocxToTextFile(const char* input, const char* output) {
    return 0;
}

int OpusModernOdtToRtfFile(const char* input, const char* output) {
    return 0;
}

int OpusModernOdtToTextFile(const char* input, const char* output) {
    return 0;
}

int OpusModernPendingDocxRunCount(void) {
    return 0;
}

int OpusModernPendingDocxParagraphCount(void) {
    return 0;
}

int OpusModernPendingDocxTableCount(void) {
    return 0;
}

int OpusModernGetPendingDocxTable(int index, long* cp_first, long* cp_lim,
                                  int* rows, int* columns, int* nesting) {
    return 0;
}

int OpusModernGetPendingDocxParagraphRange(int index, long* cp_first,
                                           long* cp_lim) {
    return 0;
}

int OpusModernGetPendingDocxPage(int* width, int* height, int* margin_left,
                                 int* margin_top, int* margin_right,
                                 int* margin_bottom) {
    return 0;
}

int OpusModernGetPendingDocxRun(int index, long* cp_first, long* cp_lim,
                                int* bold, int* italic, int* underline,
                                int* font_size, int* color, int* language,
                                int* style, int* table, int* reserved,
                                char* font, int font_capacity,
                                int* text_length, char* text,
                                int text_capacity) {
    return 0;
}

int OpusModernGetPendingDocxParagraph(int index, long* cp_first, long* cp_lim,
                                      int* alignment, int* style,
                                      int* list_kind, int* list_level,
                                      int* indent_left, int* indent_first,
                                      int* space_before, int* space_after,
                                      int* reserved, int* table) {
    return 0;
}

void OpusModernClearPendingDocxFormatting(void) {
}

int OpusModernBindPendingDocxUnicode(int doc) {
    return 0;
}

void OpusUnicodeForgetDocument(int doc) {
}

unsigned int OpusUnicodeScalarAt(int doc, long cp) {
    return 0;
}

unsigned int OpusUnicodeLanguageAt(int doc, long cp) {
    return 0;
}

int OpusUnicodeHasRange(int doc, long cp, int length) {
    return 0;
}

void OpusUnicodeOnReplace(int doc, long cp_first, long cp_lim, long delta) {
}

void OpusUnicodeOnReplaceCps(int doc, long old_first, long old_lim, int source,
                             long new_first, long new_lim) {
}

int OpusUnicodeSetInputLanguage(const char* language) {
    return 1;
}

int OpusUnicodeGetInputLanguage(char* language, int capacity) {
    if (language == NULL || capacity <= 0) return 0;
    copy_text(language, "auto", capacity);
    return 1;
}

int OpusUnicodeLegacyByteForScalar(unsigned int scalar) {
    return scalar <= 0xffu ? (int)scalar : '?';
}

int OpusUnicodeSetScalar(int doc, long cp, unsigned int scalar) {
    return 0;
}

int OpusUnicodeTextToUtf8(int doc, long cp, const char* bytes, int length,
                          int flags, char* output, int capacity) {
    if (bytes == NULL || length < 0 || capacity < 0) return -1;
    if (length >= capacity) return -1;
    if (output != NULL && capacity > 0) {
        memcpy(output, bytes, (size_t)length);
        output[length] = '\0';
    }
    return length;
}

int OpusUnicodeLanguageTagAt(int doc, long cp, char* language, int capacity) {
    if (language == NULL || capacity <= 0) return 0;
    copy_text(language, "en-US", capacity);
    return 1;
}

int OpusUnicodeClipboardToLegacy(HANDLE input, HANDLE* legacy_handle) {
    if (legacy_handle != NULL) *legacy_handle = NULL;
    return 0;
}

HANDLE OpusUnicodeCreateClipboardHandle(int doc, long cp, HANDLE source) {
    return NULL;
}

int OpusUnicodeBindPendingClipboard(int doc) {
    return 0;
}

BOOL OpusUnicodeExtTextOut(HDC dc, int x, int y, UINT options,
                           const RECT* rect, int doc, long cp,
                           const char* bytes, UINT length,
                           const int* advances) {
    return FALSE;
}

int OpusPdfSnapshotBegin(int page_width, int page_height, int margin_left,
                         int margin_top, int margin_right,
                         int margin_bottom) {
    return 0;
}

int OpusPdfSnapshotAddParagraph(int cp_first, int cp_lim, int alignment,
                                int style, int list_kind, int list_level,
                                int indent_left, int indent_first,
                                int space_before, int space_after,
                                int table) {
    return 0;
}

int OpusPdfSnapshotAddRun(const char* font, int font_size,
                          const char* language, int cp_first, int cp_lim,
                          int bold, int italic, int underline, int color,
                          int style, int table, int reserved) {
    return 0;
}

int OpusPdfSnapshotAddRunUtf8(const char* font, int font_size,
                              const char* language, int cp_first, int cp_lim,
                              int bold, int italic, int underline, int color,
                              int style, int table, int reserved,
                              const char* text, int text_length) {
    return 0;
}

int OpusPdfSnapshotExportDialog(HWND parent) {
    return 0;
}

int OpusDocxSnapshotExportPath(const char* path) {
    return 0;
}

int OpusModernRtfFileToDocx(const char* input, const char* output) {
    return 0;
}

int OpusModernRtfFileToOdt(const char* input, const char* output) {
    return 0;
}

int OpusModernRtfFileToPdf(const char* input, const char* output) {
    return 0;
}
