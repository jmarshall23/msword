#include "opus_x64_compat.h"

#include <cctype>
#include <cstring>

namespace {

bool has_extension(const char* path, const char* extension) {
    if (path == nullptr || extension == nullptr) return false;
    const std::size_t path_length = std::strlen(path);
    const std::size_t extension_length = std::strlen(extension);
    if (path_length < extension_length) return false;
    path += path_length - extension_length;
    for (std::size_t index = 0; index < extension_length; ++index) {
        if (std::tolower(static_cast<unsigned char>(path[index])) !=
            std::tolower(static_cast<unsigned char>(extension[index])))
            return false;
    }
    return true;
}

void copy_text(char* output, const char* text, const int capacity) {
    if (output == nullptr || capacity <= 0) return;
    std::size_t length = std::strlen(text);
    if (length >= static_cast<std::size_t>(capacity))
        length = static_cast<std::size_t>(capacity - 1);
    std::memcpy(output, text, length);
    output[length] = '\0';
}

}  // namespace

extern "C" int OpusModernPathIsDocx(const char* path) {
    return has_extension(path, ".docx");
}

extern "C" int OpusModernPathIsOdt(const char* path) {
    return has_extension(path, ".odt");
}

extern "C" int OpusModernDocxToRtfFile(const char*, const char*) {
    return false;
}

extern "C" int OpusModernDocxToTextFile(const char*, const char*) {
    return false;
}

extern "C" int OpusModernOdtToRtfFile(const char*, const char*) {
    return false;
}

extern "C" int OpusModernOdtToTextFile(const char*, const char*) {
    return false;
}

extern "C" int OpusModernPendingDocxRunCount() {
    return 0;
}

extern "C" int OpusModernPendingDocxParagraphCount() {
    return 0;
}

extern "C" int OpusModernPendingDocxTableCount() {
    return 0;
}

extern "C" int OpusModernGetPendingDocxTable(
    int, long*, long*, int*, int*, int*) {
    return false;
}

extern "C" int OpusModernGetPendingDocxParagraphRange(int, long*, long*) {
    return false;
}

extern "C" int OpusModernGetPendingDocxPage(int*, int*, int*, int*, int*,
                                             int*) {
    return false;
}

extern "C" int OpusModernGetPendingDocxRun(
    int, long*, long*, int*, int*, int*, int*, int*, int*, int*, int*, int*,
    char*, int, int*, char*, int) {
    return false;
}

extern "C" int OpusModernGetPendingDocxParagraph(
    int, long*, long*, int*, int*, int*, int*, int*, int*, int*, int*, int*,
    int*) {
    return false;
}

extern "C" void OpusModernClearPendingDocxFormatting() {
}

extern "C" int OpusModernBindPendingDocxUnicode(int) {
    return false;
}

extern "C" void OpusUnicodeForgetDocument(int) {
}

extern "C" unsigned int OpusUnicodeScalarAt(int, long) {
    return 0;
}

extern "C" unsigned int OpusUnicodeLanguageAt(int, long) {
    return 0;
}

extern "C" int OpusUnicodeHasRange(int, long, int) {
    return false;
}

extern "C" void OpusUnicodeOnReplace(int, long, long, long) {
}

extern "C" void OpusUnicodeOnReplaceCps(int, long, long, int, long, long) {
}

extern "C" int OpusUnicodeSetInputLanguage(const char*) {
    return true;
}

extern "C" int OpusUnicodeGetInputLanguage(char* language, int capacity) {
    if (language == nullptr || capacity <= 0) return false;
    copy_text(language, "auto", capacity);
    return true;
}

extern "C" int OpusUnicodeLegacyByteForScalar(unsigned int scalar) {
    return scalar <= 0xff ? static_cast<int>(scalar) : '?';
}

extern "C" int OpusUnicodeSetScalar(int, long, unsigned int) {
    return false;
}

extern "C" int OpusUnicodeTextToUtf8(int, long, const char* bytes, int length,
                                      int, char* output, int capacity) {
    if (bytes == nullptr || length < 0 || capacity < 0) return -1;
    if (length >= capacity) return -1;
    if (output != nullptr && capacity > 0) {
        std::memcpy(output, bytes, static_cast<std::size_t>(length));
        output[length] = '\0';
    }
    return length;
}

extern "C" int OpusUnicodeLanguageTagAt(int, long, char* language,
                                         int capacity) {
    if (language == nullptr || capacity <= 0) return false;
    copy_text(language, "en-US", capacity);
    return true;
}

extern "C" int OpusUnicodeClipboardToLegacy(HANDLE, HANDLE* legacy_handle) {
    if (legacy_handle != nullptr) *legacy_handle = nullptr;
    return false;
}

extern "C" HANDLE OpusUnicodeCreateClipboardHandle(int, long, HANDLE) {
    return nullptr;
}

extern "C" int OpusUnicodeBindPendingClipboard(int) {
    return false;
}

extern "C" BOOL OpusUnicodeExtTextOut(
    HDC, int, int, UINT, const RECT*, int, long,
    const char* bytes, UINT length, const int* advances) {
    (void)bytes;
    (void)length;
    (void)advances;
    return FALSE;
}

extern "C" int OpusPdfSnapshotBegin(int, int, int, int, int, int) {
    return false;
}

extern "C" int OpusPdfSnapshotAddParagraph(int, int, int, int, int, int, int,
                                            int, int, int, int) {
    return false;
}

extern "C" int OpusPdfSnapshotAddRun(const char*, int, const char*, int, int,
                                      int, int, int, int, int, int, int) {
    return false;
}

extern "C" int OpusPdfSnapshotAddRunUtf8(const char*, int, const char*, int,
                                          int, int, int, int, int, int, int,
                                          int, const char*, int) {
    return false;
}

extern "C" int OpusPdfSnapshotExportDialog(HWND) {
    return false;
}

extern "C" int OpusDocxSnapshotExportPath(const char*) {
    return false;
}

extern "C" int OpusModernRtfFileToDocx(const char*, const char*) {
    return false;
}

extern "C" int OpusModernRtfFileToOdt(const char*, const char*) {
    return false;
}

extern "C" int OpusModernRtfFileToPdf(const char*, const char*) {
    return false;
}

extern "C" int OpusExportRtfToPdfDialog(HWND, const char*) {
    return false;
}

extern "C" int OpusExportTextToPdfDialog(HWND, const char*, int) {
    return false;
}
