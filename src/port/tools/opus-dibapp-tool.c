#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static uint16_t read_u16(const unsigned char* bytes, size_t offset) {
    return (uint16_t)bytes[offset] | (uint16_t)(bytes[offset + 1] << 8);
}

static uint32_t read_u32(const unsigned char* bytes, size_t offset) {
    return (uint32_t)read_u16(bytes, offset) |
           ((uint32_t)read_u16(bytes, offset + 2) << 16);
}

static int read_file(const char* path, unsigned char** bytes, size_t* size) {
    FILE* file = fopen(path, "rb");
    long length;

    if (file == NULL) return 0;
    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return 0;
    }
    length = ftell(file);
    if (length < 0) {
        fclose(file);
        return 0;
    }
    if (fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        return 0;
    }
    *bytes = (unsigned char*)malloc((size_t)length);
    if (*bytes == NULL && length != 0) {
        fclose(file);
        return 0;
    }
    *size = fread(*bytes, 1, (size_t)length, file);
    if (*size != (size_t)length || ferror(file)) {
        free(*bytes);
        *bytes = NULL;
        *size = 0;
        fclose(file);
        return 0;
    }
    return fclose(file) == 0;
}

int main(int argc, char** argv) {
    unsigned char* bytes = NULL;
    size_t byte_count = 0;
    uint16_t width;
    uint16_t height;
    uint16_t planes;
    uint16_t bit_count;
    size_t palette_bytes;
    size_t bits_offset;
    size_t row_bytes;
    size_t bits_size;
    FILE* output;
    size_t index;
    int ok;

    if (argc != 3) {
        fputs("usage: opus_dibapp_tool input.dib output.hb\n", stderr);
        return 2;
    }

    if (!read_file(argv[1], &bytes, &byte_count)) {
        fprintf(stderr, "cannot open input: %s\n", argv[1]);
        return 1;
    }

    /* The archived files are OS/2 bitmap-array files: a 14-byte BA record,
     * a 14-byte BM record, a 12-byte BITMAPCOREHEADER, a 3-byte-per-color
     * palette, then DWORD-aligned scan lines. */
    if (byte_count < 46 || bytes[0] != 'B' || bytes[1] != 'A' ||
        bytes[14] != 'B' || bytes[15] != 'M' || read_u32(bytes, 28) != 12) {
        fprintf(stderr, "unsupported archived DIB layout: %s\n", argv[1]);
        free(bytes);
        return 1;
    }

    width = read_u16(bytes, 32);
    height = read_u16(bytes, 34);
    planes = read_u16(bytes, 36);
    bit_count = read_u16(bytes, 38);
    if (planes != 1 || (bit_count != 1 && bit_count != 4)) {
        fprintf(stderr, "unsupported DIB bit depth: %s\n", argv[1]);
        free(bytes);
        return 1;
    }

    palette_bytes = ((size_t)1 << bit_count) * 3;
    bits_offset = 40 + palette_bytes;
    row_bytes = (((size_t)width * bit_count + 31) / 32) * 4;
    bits_size = row_bytes * height;
    if (bits_offset + bits_size != byte_count) {
        fprintf(stderr, "DIB size does not match its core header: %s\n",
                argv[1]);
        free(bytes);
        return 1;
    }

    output = fopen(argv[2], "wb");
    if (output == NULL) {
        fprintf(stderr, "cannot open output: %s\n", argv[2]);
        free(bytes);
        return 1;
    }

    fprintf(output,
            "/* THIS IS A GENERATED FILE -- DO NOT EDIT */\n\n"
            "{\n\t{ 12, %u, %u, %u, %u },\n\t{\n",
            width, height, planes, bit_count);
    for (index = 0; index < bits_size; ++index) {
        if (index % 12 == 0) fputs("\t\t", output);
        fprintf(output, "0x%02x", (unsigned)bytes[bits_offset + index]);
        if (index + 1 != bits_size) fputs(", ", output);
        if (index % 12 == 11 || index + 1 == bits_size) fputc('\n', output);
    }
    fprintf(output, "\t},\n\t%zu,\n},\n", bits_size);

    ok = ferror(output) == 0 && fclose(output) == 0;
    free(bytes);
    return ok ? 0 : 1;
}
