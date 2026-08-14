#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int OpusModernDocxToRtfFile(const char *, const char *);
int OpusModernDocxToTextFile(const char *, const char *);
int OpusModernRtfFileToDocx(const char *, const char *);
int OpusModernOdtToRtfFile(const char *, const char *);
int OpusModernOdtToTextFile(const char *, const char *);
int OpusModernRtfFileToOdt(const char *, const char *);
int OpusModernPathIsOdt(const char *);
int OpusModernRtfFileToPdf(const char *, const char *);
int OpusModernBindPendingDocxUnicode(int);
unsigned int OpusUnicodeScalarAt(int, long);
int OpusPdfSnapshotBegin(int, int, int, int, int, int);
int OpusPdfSnapshotAddParagraph(int, int, int, int, int, int, int,
                                int, int, int, int);
int OpusPdfSnapshotAddRun(const char *, int, const char *, int,
                          int, int, int, int, int, int, int, int);

typedef struct FileBytes {
    char *data;
    size_t size;
} FileBytes;

static bool read_file(const char *path, FileBytes *bytes) {
    FILE *file;
    long size;
    size_t read_size;
    bytes->data = NULL;
    bytes->size = 0;
    file = fopen(path, "rb");
    if (file == NULL) return false;
    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return false;
    }
    size = ftell(file);
    if (size < 0 || fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        return false;
    }
    bytes->data = (char *)malloc((size_t)size + 1);
    if (bytes->data == NULL) {
        fclose(file);
        return false;
    }
    read_size = fread(bytes->data, 1, (size_t)size, file);
    fclose(file);
    if (read_size != (size_t)size) {
        free(bytes->data);
        bytes->data = NULL;
        return false;
    }
    bytes->data[read_size] = '\0';
    bytes->size = read_size;
    return true;
}

static bool write_file(const char *path, const char *data, size_t size) {
    FILE *file = fopen(path, "wb");
    bool ok;
    if (file == NULL) return false;
    ok = fwrite(data, 1, size, file) == size;
    fclose(file);
    return ok;
}

static void free_file(FileBytes *bytes) {
    free(bytes->data);
    bytes->data = NULL;
    bytes->size = 0;
}

static bool bytes_starts_with(const FileBytes *bytes, const char *prefix) {
    const size_t prefix_size = strlen(prefix);
    return bytes->size >= prefix_size &&
           memcmp(bytes->data, prefix, prefix_size) == 0;
}

static bool bytes_contains(const FileBytes *bytes, const char *needle) {
    const size_t needle_size = strlen(needle);
    if (needle_size == 0) return true;
    if (bytes->size < needle_size) return false;
    for (size_t index = 0; index <= bytes->size - needle_size; ++index) {
        if (memcmp(bytes->data + index, needle, needle_size) == 0) {
            return true;
        }
    }
    return false;
}

static bool path_with_suffix(char *path, size_t capacity, const char *base,
                             const char *suffix) {
    const int written = snprintf(path, capacity, "%s%s", base, suffix);
    return written > 0 && (size_t)written < capacity;
}

static int base64_value(int character) {
    if (character >= 'A' && character <= 'Z') return character - 'A';
    if (character >= 'a' && character <= 'z') return character - 'a' + 26;
    if (character >= '0' && character <= '9') return character - '0' + 52;
    if (character == '+') return 62;
    if (character == '/') return 63;
    return -1;
}

static bool write_base64_file(const char *path, const char *encoded) {
    const size_t capacity = strlen(encoded) * 3 / 4 + 3;
    unsigned char *output = (unsigned char *)malloc(capacity);
    unsigned accumulator = 0;
    int bits = 0;
    size_t size = 0;
    bool ok;
    if (output == NULL) return false;
    for (const unsigned char *cursor = (const unsigned char *)encoded;
         *cursor != 0; ++cursor) {
        const int value = base64_value(*cursor);
        if (*cursor == '=') break;
        if (value < 0) continue;
        accumulator = (accumulator << 6) | (unsigned)value;
        bits += 6;
        if (bits >= 8) {
            bits -= 8;
            output[size++] = (unsigned char)((accumulator >> bits) & 0xffu);
        }
    }
    ok = write_file(path, (const char *)output, size);
    free(output);
    return ok;
}

int main(const int argument_count, char **arguments) {
    enum { kPathCapacity = MAX_PATH + 64 };
    if (argument_count == 4 && strcmp(arguments[1], "--text") == 0) {
        const bool odt = OpusModernPathIsOdt(arguments[2]) != 0;
        const bool converted = (odt ?
            OpusModernOdtToTextFile(arguments[2], arguments[3]) :
            OpusModernDocxToTextFile(arguments[2], arguments[3])) != 0;
        printf("%s text import %s\n", odt ? "ODT" : "DOCX",
               converted ? "passed" : "failed");
        return converted ? 0 : 4;
    }
    if (argument_count == 3) {
        const bool odt = OpusModernPathIsOdt(arguments[1]) != 0;
        const bool converted = (odt ?
            OpusModernOdtToRtfFile(arguments[1], arguments[2]) :
            OpusModernDocxToRtfFile(arguments[1], arguments[2])) != 0;
        printf("%s import %s\n", odt ? "ODT" : "DOCX",
               converted ? "passed" : "failed");
        return converted ? 0 : 3;
    }
    if (argument_count != 1) {
        fprintf(stderr, "usage: opus_modern_formats_test [input.docx output.rtf]\n");
        return 1;
    }

    char temporary[MAX_PATH] = {0};
    char seed[MAX_PATH] = {0};
    if (GetTempPathA(MAX_PATH, temporary) == 0 ||
        GetTempFileNameA(temporary, "OWF", 0, seed) == 0) {
        return 1;
    }
    DeleteFileA(seed);

    char rtf[kPathCapacity];
    char docx[kPathCapacity];
    char odt[kPathCapacity];
    char roundtrip[kPathCapacity];
    char odt_roundtrip[kPathCapacity];
    char imported_text[kPathCapacity];
    char odt_imported_text[kPathCapacity];
    char compressed_odt[kPathCapacity];
    char compressed_rtf[kPathCapacity];
    char oversized[kPathCapacity];
    char preserved[kPathCapacity];
    char pdf[32768];
    char requested_pdf[32768] = {0};
    const DWORD requested_pdf_length = GetEnvironmentVariableA(
        "WORD1_TEST_KEEP_PDF", requested_pdf, (DWORD)sizeof(requested_pdf));
    const bool keep_pdf = requested_pdf_length > 0 &&
                          requested_pdf_length < sizeof(requested_pdf);
    if (!path_with_suffix(rtf, sizeof(rtf), seed, ".rtf") ||
        !path_with_suffix(docx, sizeof(docx), seed, ".docx") ||
        !path_with_suffix(odt, sizeof(odt), seed, ".odt") ||
        !path_with_suffix(roundtrip, sizeof(roundtrip), seed, ".roundtrip.rtf") ||
        !path_with_suffix(odt_roundtrip, sizeof(odt_roundtrip), seed,
                          ".odt-roundtrip.rtf") ||
        !path_with_suffix(imported_text, sizeof(imported_text), seed,
                          ".unicode.txt") ||
        !path_with_suffix(odt_imported_text, sizeof(odt_imported_text), seed,
                          ".odt-unicode.txt") ||
        !path_with_suffix(compressed_odt, sizeof(compressed_odt), seed,
                          ".compressed.odt") ||
        !path_with_suffix(compressed_rtf, sizeof(compressed_rtf), seed,
                          ".compressed.rtf") ||
        !path_with_suffix(oversized, sizeof(oversized), seed, ".oversized.rtf") ||
        !path_with_suffix(preserved, sizeof(preserved), seed, ".preserved.pdf")) {
        return 1;
    }
    if (keep_pdf) {
        strncpy(pdf, requested_pdf, sizeof(pdf) - 1);
        pdf[sizeof(pdf) - 1] = '\0';
    } else if (!path_with_suffix(pdf, sizeof(pdf), seed, ".pdf")) {
        return 1;
    }
    DeleteFileA(pdf);

    static const char rtf_fixture[] =
        "{\\rtf1\\ansi{\\fonttbl{\\f0 Arial;}}"
        "{\\colortbl;\\red255\\green0\\blue0;}"
        "\\f0\\fs24 Plain {\\b Bold} {\\i Italic} "
        "{\\ul Underline} {\\cf1 Red} "
        "{\\lang1049 \\u1055?\\u1088?\\u1080?\\u1074?\\u1077?\\u1090?}"
        " {\\lang1032 \\u915?\\u949?\\u953?\\u940?}"
        " {\\lang1025 \\u1605?\\u1585?\\u1581?\\u1576?\\u1575?}"
        " {\\lang1041 \\u12371?\\u12435?\\u12395?\\u12385?\\u12399?}"
        "\\par Second paragraph}";
    if (!write_file(rtf, rtf_fixture, sizeof(rtf_fixture) - 1)) {
        return 1;
    }

    const bool written = OpusModernRtfFileToDocx(rtf, docx) != 0;
    const bool read = written && OpusModernDocxToRtfFile(docx, roundtrip) != 0;
    const bool unicode_imported = written &&
        OpusModernDocxToTextFile(docx, imported_text) != 0 &&
        OpusModernBindPendingDocxUnicode(42) != 0;
    bool cyrillic_sidecar = false;
    bool greek_sidecar = false;
    bool arabic_sidecar = false;
    bool japanese_sidecar = false;
    if (unicode_imported) {
        FileBytes imported_bytes;
        if (read_file(imported_text, &imported_bytes)) {
            for (long cp = 0; cp < (long)imported_bytes.size; ++cp) {
                const unsigned int scalar = OpusUnicodeScalarAt(42, cp);
                if (scalar == 1055) cyrillic_sidecar = true;
                if (scalar == 915) greek_sidecar = true;
                if (scalar == 1605) arabic_sidecar = true;
                if (scalar == 12371) japanese_sidecar = true;
            }
            free_file(&imported_bytes);
        }
    }

    const bool odt_written = OpusModernRtfFileToOdt(rtf, odt) != 0;
    const bool odt_read = odt_written &&
        OpusModernOdtToRtfFile(odt, odt_roundtrip) != 0;
    const bool odt_unicode_imported = odt_written &&
        OpusModernOdtToTextFile(odt, odt_imported_text) != 0 &&
        OpusModernBindPendingDocxUnicode(43) != 0;
    bool odt_cyrillic_sidecar = false;
    bool odt_greek_sidecar = false;
    bool odt_arabic_sidecar = false;
    bool odt_japanese_sidecar = false;
    if (odt_unicode_imported) {
        FileBytes imported_bytes;
        if (read_file(odt_imported_text, &imported_bytes)) {
            for (long cp = 0; cp < (long)imported_bytes.size; ++cp) {
                const unsigned int scalar = OpusUnicodeScalarAt(43, cp);
                if (scalar == 1055) odt_cyrillic_sidecar = true;
                if (scalar == 915) odt_greek_sidecar = true;
                if (scalar == 1605) odt_arabic_sidecar = true;
                if (scalar == 12371) odt_japanese_sidecar = true;
            }
            free_file(&imported_bytes);
        }
    }

    static const char compressed_odt_base64[] =
        "UEsDBBQAAAAAADKSC11exjIMJwAAACcAAAAIAAAAbWltZXR5cGVhcHBsaWNhdGlvbi92bmQub2FzaXMub3BlbmRvY3VtZW50LnRl"
        "eHRQSwMEFAAAAAgAMpILXfNZG7MaAQAAmwIAAAsAAABjb250ZW50LnhtbI1STW+EIBD9K4SerdtbQ4A9tPHaQ7c/ABEtiTKGwVb/"
        "fUFc6x42WQ4wH+/Nm5nAz/PQkx/j0YIT9OX5RIlxGhrrOkG/LlXxSs+SQ9tabVgDehqMC4UGF+JLItkhy1lBJ+8YKLTInBoMsqAZ"
        "jMZdWeyIZqtUjgQzh0fZCXvkYlj6h6VX8JHdwqPUGfuihTj4MKpg663Mvhk1BRhiQherBkqetdabZDuVF/QzeHAd3WKtGmy/CJrG"
        "oldScorRxy58sAZJC7HRuPVfY7vvuKka+oamqIYevKBPVXWKh5aSlwfZ6N3tbkvU0Cy7k2QlXxc8yrc4qDeIpiE5hKNyJFupRnEz"
        "jfx4v8QkBl7uYLnZ438fWaG8ES/v/C35B1BLAQIUABQAAAAAADKSC11exjIMJwAAACcAAAAIAAAAAAAAAAAAAACAAQAAAABtaW1l"
        "dHlwZVBLAQIUABQAAAAIADKSC13zWRuzGgEAAJsCAAALAAAAAAAAAAAAAACAAU0AAABjb250ZW50LnhtbFBLBQYAAAAAAgACAG8A"
        "AACQAQAAAAA=";
    const bool compressed_written =
        write_base64_file(compressed_odt, compressed_odt_base64);
    const bool compressed_odt_read = compressed_written &&
        OpusModernOdtToRtfFile(compressed_odt, compressed_rtf) != 0;
    FileBytes compressed_result = {0};
    if (compressed_odt_read) {
        read_file(compressed_rtf, &compressed_result);
    }

    const bool pdf_written = OpusModernRtfFileToPdf(rtf, pdf) != 0;
    write_file(preserved, "ORIGINAL", 8);
    FILE *oversized_file = fopen(oversized, "wb");
    bool oversized_created = oversized_file != NULL;
    if (oversized_created) {
        oversized_created =
            fseek(oversized_file, 64L * 1024L * 1024L, SEEK_SET) == 0 &&
            fputc(0, oversized_file) != EOF;
        fclose(oversized_file);
    }
    FileBytes preserved_bytes = {0};
    const bool oversized_rejected = oversized_created &&
        OpusModernRtfFileToPdf(oversized, preserved) == 0 &&
        read_file(preserved, &preserved_bytes) &&
        preserved_bytes.size == 8 &&
        memcmp(preserved_bytes.data, "ORIGINAL", 8) == 0;
    free_file(&preserved_bytes);

    const bool invalid_snapshot_rejected =
        OpusPdfSnapshotBegin(INT_MAX, INT_MAX, INT_MAX, INT_MAX,
                             INT_MAX, INT_MAX) == 0 &&
        OpusPdfSnapshotBegin(12240, 15840, 1440, 1440, 1440, 1440) != 0 &&
        OpusPdfSnapshotAddParagraph(0, 0, 0, 0, 0, 0, 0,
                                    0, 0, 0, 0) != 0 &&
        OpusPdfSnapshotAddParagraph(0, 0, 0, 0, 0, 0, -240,
                                    0, 0, 0, 0) != 0 &&
        OpusPdfSnapshotAddRun("x", INT_MAX, "Arial", 20,
                              0, 0, 0, 0, 0, 0, 0, 0) == 0;

    FileBytes result = {0};
    FileBytes odt_result = {0};
    FileBytes odt_package = {0};
    FileBytes pdf_data = {0};
    if (read) read_file(roundtrip, &result);
    if (odt_read) read_file(odt_roundtrip, &odt_result);
    if (odt_written) read_file(odt, &odt_package);
    if (pdf_written) read_file(pdf, &pdf_data);

    DeleteFileA(rtf);
    DeleteFileA(docx);
    DeleteFileA(odt);
    DeleteFileA(roundtrip);
    DeleteFileA(odt_roundtrip);
    DeleteFileA(imported_text);
    DeleteFileA(odt_imported_text);
    DeleteFileA(compressed_odt);
    DeleteFileA(compressed_rtf);
    DeleteFileA(oversized);
    DeleteFileA(preserved);
    if (!keep_pdf) DeleteFileA(pdf);

    const bool failed =
        !written || !read || !unicode_imported || !cyrillic_sidecar ||
        !greek_sidecar || !arabic_sidecar || !japanese_sidecar ||
        !odt_written || !odt_read || !odt_unicode_imported ||
        !odt_cyrillic_sidecar || !odt_greek_sidecar ||
        !odt_arabic_sidecar || !odt_japanese_sidecar ||
        !bytes_contains(&odt_result, "Bold") ||
        !bytes_contains(&odt_result, "Second paragraph") ||
        !bytes_contains(&odt_result, "\\u1055") ||
        !bytes_contains(&odt_result, "\\u915") ||
        !bytes_contains(&odt_result, "\\u1605") ||
        !bytes_contains(&odt_result, "\\u12371") ||
        !bytes_contains(&odt_result, "\\b") ||
        !bytes_contains(&odt_result, "\\cf1") ||
        !bytes_starts_with(&odt_package, "PK") ||
        !bytes_contains(&odt_package,
                        "application/vnd.oasis.opendocument.text") ||
        !bytes_contains(&odt_package, "META-INF/manifest.xml") ||
        !compressed_odt_read ||
        !bytes_contains(&compressed_result, "Compressed") ||
        !bytes_contains(&compressed_result, "ODT test") ||
        !bytes_contains(&compressed_result, "\\b") ||
        !bytes_contains(&compressed_result, "\\cf1") ||
        !bytes_contains(&result, "Bold") ||
        !bytes_contains(&result, "Second paragraph") ||
        !bytes_contains(&result, "\\u1055") ||
        !bytes_contains(&result, "\\u915") ||
        !bytes_contains(&result, "\\u1605") ||
        !bytes_contains(&result, "\\u12371") ||
        !bytes_contains(&result, "\\b") ||
        !bytes_contains(&result, "\\cf1") || !pdf_written ||
        !bytes_starts_with(&pdf_data, "%PDF-") ||
        !bytes_contains(&pdf_data, "Plain") ||
        !bytes_contains(&pdf_data, "Bold") ||
        !bytes_contains(&pdf_data, "Second paragraph") ||
        !bytes_contains(&pdf_data, "/Encoding /Identity-H") ||
        !bytes_contains(&pdf_data, "/ToUnicode") ||
        !bytes_contains(&pdf_data, "/Helvetica-Bold") ||
        !bytes_contains(&pdf_data, "xref") ||
        !bytes_contains(&pdf_data, "%%EOF") ||
        !oversized_rejected || !invalid_snapshot_rejected;
    if (failed) {
        fprintf(stderr,
                "modern format round trip failed: docxWrite=%d read=%d "
                "pdf=%d odtWrite=%d odtRead=%d bytes=%zu "
                "oversizedRejected=%d invalidSnapshotRejected=%d\n",
                written, read, pdf_written, odt_written, odt_read,
                result.size, oversized_rejected, invalid_snapshot_rejected);
        free_file(&result);
        free_file(&odt_result);
        free_file(&odt_package);
        free_file(&pdf_data);
        free_file(&compressed_result);
        return 2;
    }

    printf("DOCX/ODT round trips passed (%zu DOCX RTF bytes, %zu ODT RTF bytes)\n",
           result.size, odt_result.size);
    free_file(&result);
    free_file(&odt_result);
    free_file(&odt_package);
    free_file(&pdf_data);
    free_file(&compressed_result);
    return 0;
}
