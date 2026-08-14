#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#include "opus_x64_compat.h"

#include <ctype.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef OPUSW
#define OPUSW(text) u##text
#endif

enum {
    kMaxRtfBytes = 64u * 1024u * 1024u,
    kMaxTextBytes = 32u * 1024u * 1024u,
    kMaxGeneratedBytes = 256u * 1024u * 1024u,
    kMaxParagraphs = 200000,
    kMaxRuns = 1000000
};

typedef struct ByteBuffer {
    char* data;
    size_t size;
    size_t capacity;
} ByteBuffer;

typedef struct UnicodeCell {
    uint32_t scalar;
    WORD language;
} UnicodeCell;

typedef struct UnicodeCells {
    UnicodeCell* data;
    size_t size;
    size_t capacity;
} UnicodeCells;

typedef struct UnicodeDocument {
    int doc;
    UnicodeCells cells;
} UnicodeDocument;

typedef struct UnicodeDocumentList {
    UnicodeDocument* data;
    size_t size;
    size_t capacity;
} UnicodeDocumentList;

typedef struct PdfSnapshot {
    ByteBuffer text;
    int valid;
    size_t paragraphs;
    size_t runs;
} PdfSnapshot;

typedef struct ZipEntry {
    const char* name;
    const char* data;
    size_t size;
} ZipEntry;

static UnicodeCells g_pending_cells;
static UnicodeDocumentList g_unicode_documents;
static char g_input_language[32] = "auto";
static PdfSnapshot g_pdf_snapshot;

static size_t bounded_strlen(const char* text, size_t limit) {
    size_t length = 0;
    if (text == NULL) return 0;
    while (length < limit && text[length] != '\0') ++length;
    return length;
}

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
        if (tolower((unsigned char) path[index]) !=
            tolower((unsigned char) extension[index])) return 0;
    }
    return 1;
}

static void buffer_free(ByteBuffer* buffer) {
    if (buffer == NULL) return;
    free(buffer->data);
    buffer->data = NULL;
    buffer->size = 0;
    buffer->capacity = 0;
}

static int buffer_reserve(ByteBuffer* buffer, size_t capacity) {
    char* resized;
    if (capacity <= buffer->capacity) return 1;
    if (capacity > kMaxGeneratedBytes) return 0;
    resized = (char*) realloc(buffer->data, capacity);
    if (resized == NULL) return 0;
    buffer->data = resized;
    buffer->capacity = capacity;
    return 1;
}

static int buffer_append(ByteBuffer* buffer, const void* data, size_t size) {
    size_t capacity;
    if (size == 0) return 1;
    if (data == NULL || buffer->size > kMaxGeneratedBytes - size) return 0;
    capacity = buffer->capacity == 0 ? 256 : buffer->capacity;
    while (capacity < buffer->size + size) {
        if (capacity > kMaxGeneratedBytes / 2) {
            capacity = buffer->size + size;
            break;
        }
        capacity *= 2;
    }
    if (!buffer_reserve(buffer, capacity)) return 0;
    memcpy(buffer->data + buffer->size, data, size);
    buffer->size += size;
    return 1;
}

static int buffer_append_c(ByteBuffer* buffer, char value) {
    return buffer_append(buffer, &value, 1);
}

static int buffer_append_s(ByteBuffer* buffer, const char* text) {
    return buffer_append(buffer, text, strlen(text));
}

static const void* find_bytes(const void* haystack, size_t haystack_size,
                              const void* needle, size_t needle_size) {
    const char* bytes = (const char*) haystack;
    size_t index;
    if (needle_size == 0) return haystack;
    if (haystack == NULL || needle == NULL || haystack_size < needle_size)
        return NULL;
    for (index = 0; index <= haystack_size - needle_size; ++index)
        if (memcmp(bytes + index, needle, needle_size) == 0)
            return bytes + index;
    return NULL;
}

static int read_file(const char* path, ByteBuffer* out, size_t limit) {
    FILE* file;
    long size;
    size_t read_size;
    memset(out, 0, sizeof(*out));
    file = fopen(path, "rb");
    if (file == NULL) return 0;
    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return 0;
    }
    size = ftell(file);
    if (size < 0 || (size_t) size > limit || fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        return 0;
    }
    if (!buffer_reserve(out, (size_t) size + 1)) {
        fclose(file);
        return 0;
    }
    read_size = fread(out->data, 1, (size_t) size, file);
    fclose(file);
    if (read_size != (size_t) size) {
        buffer_free(out);
        return 0;
    }
    out->data[read_size] = '\0';
    out->size = read_size;
    return 1;
}

static int write_file(const char* path, const void* data, size_t size) {
    FILE* file = fopen(path, "wb");
    int ok;
    if (file == NULL) return 0;
    ok = fwrite(data, 1, size, file) == size;
    fclose(file);
    return ok;
}

static uint16_t get_u16(const char* bytes, size_t size, size_t offset,
                        int* ok) {
    if (offset > size || size - offset < 2) {
        *ok = 0;
        return 0;
    }
    return (uint16_t) ((unsigned char) bytes[offset] |
                       ((unsigned char) bytes[offset + 1] << 8));
}

static uint32_t get_u32(const char* bytes, size_t size, size_t offset,
                        int* ok) {
    if (offset > size || size - offset < 4) {
        *ok = 0;
        return 0;
    }
    return (uint32_t) ((unsigned char) bytes[offset] |
                       ((unsigned char) bytes[offset + 1] << 8) |
                       ((unsigned char) bytes[offset + 2] << 16) |
                       ((unsigned char) bytes[offset + 3] << 24));
}

static void put_u16(ByteBuffer* bytes, uint16_t value) {
    unsigned char out[2];
    out[0] = (unsigned char) (value & 0xffu);
    out[1] = (unsigned char) ((value >> 8) & 0xffu);
    buffer_append(bytes, out, sizeof(out));
}

static void put_u32(ByteBuffer* bytes, uint32_t value) {
    unsigned char out[4];
    out[0] = (unsigned char) (value & 0xffu);
    out[1] = (unsigned char) ((value >> 8) & 0xffu);
    out[2] = (unsigned char) ((value >> 16) & 0xffu);
    out[3] = (unsigned char) ((value >> 24) & 0xffu);
    buffer_append(bytes, out, sizeof(out));
}

static uint32_t zip_crc32(const char* bytes, size_t size) {
    uint32_t crc = 0xffffffffu;
    size_t index;
    for (index = 0; index < size; ++index) {
        int bit;
        crc ^= (unsigned char) bytes[index];
        for (bit = 0; bit < 8; ++bit)
            crc = (crc >> 1) ^ (0xedb88320u & (uint32_t) -(int) (crc & 1));
    }
    return ~crc;
}

typedef struct DeflateBits {
    const char* data;
    size_t size;
    size_t position;
    uint64_t buffer;
    unsigned bits;
} DeflateBits;

static int bits_read(DeflateBits* bits, unsigned count, unsigned* value) {
    if (count > 24) return 0;
    while (bits->bits < count) {
        if (bits->position >= bits->size) return 0;
        bits->buffer |=
            (uint64_t) (unsigned char) bits->data[bits->position++] <<
            bits->bits;
        bits->bits += 8;
    }
    *value = (unsigned) (bits->buffer & ((1ull << count) - 1));
    bits->buffer >>= count;
    bits->bits -= count;
    return 1;
}

static int bits_align_byte(DeflateBits* bits) {
    unsigned ignored = 0;
    unsigned discard = bits->bits & 7u;
    return discard == 0 || bits_read(bits, discard, &ignored);
}

static size_t bits_byte_position(const DeflateBits* bits) {
    return bits->position - bits->bits / 8;
}

static int bits_set_byte_position(DeflateBits* bits, size_t position) {
    if (position > bits->size) return 0;
    bits->position = position;
    bits->buffer = 0;
    bits->bits = 0;
    return 1;
}

typedef struct Huffman {
    unsigned counts[16];
    unsigned short* symbols;
    size_t symbol_count;
} Huffman;

static void huffman_free(Huffman* tree) {
    free(tree->symbols);
    memset(tree, 0, sizeof(*tree));
}

static int huffman_build(Huffman* tree, const unsigned char* lengths,
                         size_t length_count) {
    unsigned offsets[16] = {0};
    int remaining = 1;
    unsigned length;
    size_t symbol;
    huffman_free(tree);
    tree->symbols = (unsigned short*) calloc(length_count, sizeof(unsigned short));
    if (tree->symbols == NULL) return 0;
    tree->symbol_count = length_count;
    for (symbol = 0; symbol < length_count; ++symbol) {
        if (lengths[symbol] > 15) return 0;
        ++tree->counts[lengths[symbol]];
    }
    if (tree->counts[0] == length_count) return 0;
    for (length = 1; length <= 15; ++length) {
        remaining <<= 1;
        remaining -= (int) tree->counts[length];
        if (remaining < 0) return 0;
    }
    for (length = 1; length < 15; ++length)
        offsets[length + 1] = offsets[length] + tree->counts[length];
    for (symbol = 0; symbol < length_count; ++symbol)
        if (lengths[symbol] != 0)
            tree->symbols[offsets[lengths[symbol]]++] =
                (unsigned short) symbol;
    return 1;
}

static int huffman_decode(const Huffman* tree, DeflateBits* bits,
                          unsigned* symbol) {
    unsigned code = 0;
    unsigned first = 0;
    unsigned index = 0;
    unsigned length;
    for (length = 1; length <= 15; ++length) {
        unsigned bit = 0;
        unsigned count;
        if (!bits_read(bits, 1, &bit)) return 0;
        code |= bit;
        count = tree->counts[length];
        if (code >= first && code - first < count) {
            unsigned location = index + code - first;
            if (location >= tree->symbol_count) return 0;
            *symbol = tree->symbols[location];
            return 1;
        }
        index += count;
        first = (first + count) << 1;
        code <<= 1;
    }
    return 0;
}

static int inflate_raw(const char* compressed, size_t compressed_size,
                       size_t expected, ByteBuffer* output) {
    static const unsigned length_base[29] = {
        3,4,5,6,7,8,9,10,11,13,15,17,19,23,27,31,35,43,51,59,67,83,
        99,115,131,163,195,227,258};
    static const unsigned length_extra[29] = {
        0,0,0,0,0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,0};
    static const unsigned distance_base[30] = {
        1,2,3,4,5,7,9,13,17,25,33,49,65,97,129,193,257,385,513,769,
        1025,1537,2049,3073,4097,6145,8193,12289,16385,24577};
    static const unsigned distance_extra[30] = {
        0,0,0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,9,9,10,10,11,11,
        12,12,13,13};
    DeflateBits bits;
    int final_block = 0;
    if (expected > kMaxGeneratedBytes) return 0;
    memset(output, 0, sizeof(*output));
    bits.data = compressed;
    bits.size = compressed_size;
    bits.position = 0;
    bits.buffer = 0;
    bits.bits = 0;
    while (!final_block) {
        unsigned final = 0;
        unsigned type = 0;
        unsigned char* literal_lengths = NULL;
        unsigned char* distance_lengths = NULL;
        size_t literal_count = 0;
        size_t distance_count = 0;
        Huffman literal_tree = {0};
        Huffman distance_tree = {0};
        if (!bits_read(&bits, 1, &final) || !bits_read(&bits, 2, &type))
            return 0;
        final_block = final != 0;
        if (type == 0) {
            int ok = 1;
            size_t position;
            unsigned length;
            unsigned complement;
            if (!bits_align_byte(&bits)) return 0;
            position = bits_byte_position(&bits);
            length = get_u16(compressed, compressed_size, position, &ok);
            complement = get_u16(compressed, compressed_size, position + 2, &ok);
            if (!ok || (length ^ 0xffffu) != complement ||
                compressed_size - position - 4 < length ||
                output->size > expected ||
                expected - output->size < length) return 0;
            if (!buffer_append(output, compressed + position + 4, length))
                return 0;
            if (!bits_set_byte_position(&bits, position + 4 + length))
                return 0;
            continue;
        }
        if (type == 3) return 0;
        if (type == 1) {
            unsigned index;
            literal_count = 288;
            distance_count = 32;
            literal_lengths = (unsigned char*) calloc(literal_count, 1);
            distance_lengths = (unsigned char*) calloc(distance_count, 1);
            if (literal_lengths == NULL || distance_lengths == NULL) return 0;
            for (index = 0; index <= 143; ++index) literal_lengths[index] = 8;
            for (index = 144; index <= 255; ++index) literal_lengths[index] = 9;
            for (index = 256; index <= 279; ++index) literal_lengths[index] = 7;
            for (index = 280; index <= 287; ++index) literal_lengths[index] = 8;
            for (index = 0; index < 32; ++index) distance_lengths[index] = 5;
        } else {
            static const unsigned order[19] = {
                16,17,18,0,8,7,9,6,10,5,11,4,12,3,13,2,14,1,15};
            unsigned hlit = 0, hdist = 0, hclen = 0;
            unsigned char code_lengths[19] = {0};
            unsigned char* all_lengths;
            size_t all_size = 0;
            Huffman code_tree = {0};
            unsigned index;
            if (!bits_read(&bits, 5, &hlit) || !bits_read(&bits, 5, &hdist) ||
                !bits_read(&bits, 4, &hclen)) return 0;
            hlit += 257;
            hdist += 1;
            hclen += 4;
            if (hlit > 286 || hdist > 32) return 0;
            for (index = 0; index < hclen; ++index) {
                unsigned length = 0;
                if (!bits_read(&bits, 3, &length)) return 0;
                code_lengths[order[index]] = (unsigned char) length;
            }
            if (!huffman_build(&code_tree, code_lengths, 19)) return 0;
            all_lengths = (unsigned char*) calloc(hlit + hdist, 1);
            if (all_lengths == NULL) return 0;
            while (all_size < hlit + hdist) {
                unsigned symbol = 0;
                if (!huffman_decode(&code_tree, &bits, &symbol)) return 0;
                if (symbol <= 15) {
                    all_lengths[all_size++] = (unsigned char) symbol;
                } else {
                    unsigned repeat = 0;
                    unsigned extra = symbol == 16 ? 2 : symbol == 17 ? 3 : 7;
                    unsigned value = symbol == 16 ? 3 : symbol == 17 ? 3 : 11;
                    if ((symbol == 16 && all_size == 0) || symbol > 18 ||
                        !bits_read(&bits, extra, &repeat)) return 0;
                    repeat += value;
                    if (repeat > hlit + hdist - all_size) return 0;
                    memset(all_lengths + all_size,
                           symbol == 16 ? all_lengths[all_size - 1] : 0,
                           repeat);
                    all_size += repeat;
                }
            }
            huffman_free(&code_tree);
            literal_count = hlit;
            distance_count = hdist;
            literal_lengths = (unsigned char*) malloc(literal_count);
            distance_lengths = (unsigned char*) malloc(distance_count);
            if (literal_lengths == NULL || distance_lengths == NULL) return 0;
            memcpy(literal_lengths, all_lengths, literal_count);
            memcpy(distance_lengths, all_lengths + literal_count, distance_count);
            free(all_lengths);
        }
        if (!huffman_build(&literal_tree, literal_lengths, literal_count) ||
            !huffman_build(&distance_tree, distance_lengths, distance_count)) {
            free(literal_lengths);
            free(distance_lengths);
            return 0;
        }
        free(literal_lengths);
        free(distance_lengths);
        for (;;) {
            unsigned symbol = 0;
            if (!huffman_decode(&literal_tree, &bits, &symbol)) return 0;
            if (symbol < 256) {
                char value = (char) symbol;
                if (output->size >= expected ||
                    !buffer_append(output, &value, 1)) return 0;
                continue;
            }
            if (symbol == 256) break;
            if (symbol < 257 || symbol > 285) return 0;
            {
                unsigned length_index = symbol - 257;
                unsigned length_bits = 0;
                unsigned distance_symbol = 0;
                unsigned distance_bits = 0;
                unsigned length;
                unsigned distance;
                unsigned index;
                if (!bits_read(&bits, length_extra[length_index], &length_bits))
                    return 0;
                length = length_base[length_index] + length_bits;
                if (!huffman_decode(&distance_tree, &bits, &distance_symbol) ||
                    distance_symbol >= 30 ||
                    !bits_read(&bits, distance_extra[distance_symbol],
                               &distance_bits)) return 0;
                distance = distance_base[distance_symbol] + distance_bits;
                if (distance == 0 || distance > output->size ||
                    output->size > expected ||
                    expected - output->size < length) return 0;
                for (index = 0; index < length; ++index) {
                    char value = output->data[output->size - distance];
                    if (!buffer_append(output, &value, 1)) return 0;
                }
            }
        }
        huffman_free(&literal_tree);
        huffman_free(&distance_tree);
    }
    if (!buffer_append_c(output, '\0')) return 0;
    --output->size;
    return output->size == expected;
}

static int read_zip_entry(const char* path, const char* requested,
                          ByteBuffer* data, size_t maximum_size) {
    ByteBuffer package;
    size_t search_first;
    size_t eocd;
    size_t central_size;
    size_t central_offset;
    size_t position;
    unsigned entry_count;
    unsigned entry;
    int ok = 1;
    memset(data, 0, sizeof(*data));
    if (!read_file(path, &package, kMaxGeneratedBytes) ||
        package.size < 22 || requested == NULL || *requested == '\0')
        return 0;
    search_first = package.size > 65557 ? package.size - 65557 : 0;
    eocd = (size_t) -1;
    for (position = package.size - 22;; --position) {
        if (get_u32(package.data, package.size, position, &ok) == 0x06054b50u) {
            eocd = position;
            break;
        }
        if (position == search_first) break;
    }
    if (eocd == (size_t) -1 ||
        get_u16(package.data, package.size, eocd + 4, &ok) != 0 ||
        get_u16(package.data, package.size, eocd + 6, &ok) != 0 ||
        get_u16(package.data, package.size, eocd + 8, &ok) !=
            get_u16(package.data, package.size, eocd + 10, &ok)) {
        buffer_free(&package);
        return 0;
    }
    entry_count = get_u16(package.data, package.size, eocd + 10, &ok);
    central_size = get_u32(package.data, package.size, eocd + 12, &ok);
    central_offset = get_u32(package.data, package.size, eocd + 16, &ok);
    if (!ok || entry_count > 4096 || central_offset > package.size ||
        central_size > package.size - central_offset ||
        central_offset + central_size > eocd) {
        buffer_free(&package);
        return 0;
    }
    position = central_offset;
    for (entry = 0; entry < entry_count; ++entry) {
        unsigned flags;
        unsigned method;
        uint32_t crc;
        size_t compressed_size;
        size_t uncompressed_size;
        size_t name_length;
        size_t extra_length;
        size_t comment_length;
        size_t local_offset;
        size_t record_size;
        if (position > package.size || package.size - position < 46 ||
            get_u32(package.data, package.size, position, &ok) != 0x02014b50u)
            break;
        flags = get_u16(package.data, package.size, position + 8, &ok);
        method = get_u16(package.data, package.size, position + 10, &ok);
        crc = get_u32(package.data, package.size, position + 16, &ok);
        compressed_size = get_u32(package.data, package.size, position + 20, &ok);
        uncompressed_size = get_u32(package.data, package.size, position + 24, &ok);
        name_length = get_u16(package.data, package.size, position + 28, &ok);
        extra_length = get_u16(package.data, package.size, position + 30, &ok);
        comment_length = get_u16(package.data, package.size, position + 32, &ok);
        local_offset = get_u32(package.data, package.size, position + 42, &ok);
        record_size = 46 + name_length + extra_length + comment_length;
        if (!ok || record_size > package.size - position) break;
        if (strlen(requested) == name_length &&
            memcmp(package.data + position + 46, requested, name_length) == 0) {
            size_t local_name;
            size_t local_extra;
            size_t content_offset;
            int result = 0;
            if ((flags & 1u) != 0 || (method != 0 && method != 8) ||
                uncompressed_size > maximum_size ||
                compressed_size > kMaxGeneratedBytes ||
                local_offset > package.size || package.size - local_offset < 30 ||
                get_u32(package.data, package.size, local_offset, &ok) !=
                    0x04034b50u) break;
            local_name = get_u16(package.data, package.size, local_offset + 26, &ok);
            local_extra = get_u16(package.data, package.size, local_offset + 28, &ok);
            content_offset = local_offset + 30 + local_name + local_extra;
            if (!ok || content_offset > package.size ||
                compressed_size > package.size - content_offset) break;
            if (method == 0) {
                result = compressed_size == uncompressed_size &&
                         buffer_append(data, package.data + content_offset,
                                       compressed_size) &&
                         buffer_append_c(data, '\0');
                if (result) --data->size;
            } else {
                result = inflate_raw(package.data + content_offset, compressed_size,
                                     uncompressed_size, data);
            }
            if (result && zip_crc32(data->data, data->size) != crc) result = 0;
            buffer_free(&package);
            if (!result) buffer_free(data);
            return result;
        }
        position += record_size;
    }
    buffer_free(&package);
    return 0;
}

static int write_stored_zip(const char* path, const ZipEntry* entries,
                            size_t count) {
    ByteBuffer archive = {0};
    ByteBuffer central = {0};
    size_t index;
    uint32_t central_offset;
    if (entries == NULL || count == 0 || count > 4096) return 0;
    for (index = 0; index < count; ++index) {
        uint32_t offset;
        uint32_t size;
        uint32_t crc;
        size_t name_size = strlen(entries[index].name);
        if (name_size == 0 || name_size > 0xffff ||
            entries[index].size > 0xffffffffu ||
            archive.size > 0xffffffffu) goto fail;
        offset = (uint32_t) archive.size;
        size = (uint32_t) entries[index].size;
        crc = zip_crc32(entries[index].data, entries[index].size);
        put_u32(&archive, 0x04034b50u);
        put_u16(&archive, 20);
        put_u16(&archive, 0x0800);
        put_u16(&archive, 0);
        put_u16(&archive, 0);
        put_u16(&archive, 0);
        put_u32(&archive, crc);
        put_u32(&archive, size);
        put_u32(&archive, size);
        put_u16(&archive, (uint16_t) name_size);
        put_u16(&archive, 0);
        buffer_append(&archive, entries[index].name, name_size);
        buffer_append(&archive, entries[index].data, entries[index].size);

        put_u32(&central, 0x02014b50u);
        put_u16(&central, 20);
        put_u16(&central, 20);
        put_u16(&central, 0x0800);
        put_u16(&central, 0);
        put_u16(&central, 0);
        put_u16(&central, 0);
        put_u32(&central, crc);
        put_u32(&central, size);
        put_u32(&central, size);
        put_u16(&central, (uint16_t) name_size);
        put_u16(&central, 0);
        put_u16(&central, 0);
        put_u16(&central, 0);
        put_u16(&central, 0);
        put_u32(&central, 0);
        put_u32(&central, offset);
        buffer_append(&central, entries[index].name, name_size);
    }
    if (archive.size > 0xffffffffu || central.size > 0xffffffffu ||
        archive.size + central.size > kMaxGeneratedBytes) goto fail;
    central_offset = (uint32_t) archive.size;
    buffer_append(&archive, central.data, central.size);
    put_u32(&archive, 0x06054b50u);
    put_u16(&archive, 0);
    put_u16(&archive, 0);
    put_u16(&archive, (uint16_t) count);
    put_u16(&archive, (uint16_t) count);
    put_u32(&archive, (uint32_t) central.size);
    put_u32(&archive, central_offset);
    put_u16(&archive, 0);
    buffer_free(&central);
    if (!write_file(path, archive.data, archive.size)) goto fail_archive;
    buffer_free(&archive);
    return 1;
fail:
    buffer_free(&central);
fail_archive:
    buffer_free(&archive);
    return 0;
}

static int cells_push(UnicodeCells* cells, uint32_t scalar, WORD language) {
    UnicodeCell* resized;
    size_t capacity;
    if (cells->size >= kMaxTextBytes) return 0;
    if (cells->size < cells->capacity) {
        cells->data[cells->size].scalar = scalar;
        cells->data[cells->size].language = language;
        ++cells->size;
        return 1;
    }
    capacity = cells->capacity == 0 ? 256 : cells->capacity * 2;
    resized = (UnicodeCell*) realloc(cells->data, capacity * sizeof(*resized));
    if (resized == NULL) return 0;
    cells->data = resized;
    cells->capacity = capacity;
    cells->data[cells->size].scalar = scalar;
    cells->data[cells->size].language = language;
    ++cells->size;
    return 1;
}

static void cells_clear(UnicodeCells* cells) {
    if (cells != NULL) cells->size = 0;
}

static void cells_free(UnicodeCells* cells) {
    if (cells == NULL) return;
    free(cells->data);
    cells->data = NULL;
    cells->size = 0;
    cells->capacity = 0;
}

static int cells_copy(UnicodeCells* destination, const UnicodeCells* source) {
    cells_clear(destination);
    if (source->size == 0) return 1;
    if (destination->capacity < source->size) {
        UnicodeCell* resized = (UnicodeCell*) realloc(
            destination->data, source->size * sizeof(*resized));
        if (resized == NULL) return 0;
        destination->data = resized;
        destination->capacity = source->size;
    }
    memcpy(destination->data, source->data, source->size * sizeof(*source->data));
    destination->size = source->size;
    return 1;
}

static int cells_insert_zeroes(UnicodeCells* cells, size_t position,
                               size_t count) {
    UnicodeCell* resized;
    size_t capacity;
    if (count == 0) return 1;
    if (position > cells->size || count > kMaxTextBytes - cells->size)
        return 0;
    capacity = cells->capacity == 0 ? 256 : cells->capacity;
    while (capacity < cells->size + count) {
        if (capacity > kMaxTextBytes / 2) {
            capacity = cells->size + count;
            break;
        }
        capacity *= 2;
    }
    if (capacity > cells->capacity) {
        resized = (UnicodeCell*) realloc(cells->data,
                                          capacity * sizeof(*resized));
        if (resized == NULL) return 0;
        cells->data = resized;
        cells->capacity = capacity;
    }
    memmove(cells->data + position + count, cells->data + position,
            (cells->size - position) * sizeof(*cells->data));
    memset(cells->data + position, 0, count * sizeof(*cells->data));
    cells->size += count;
    return 1;
}

static UnicodeDocument* find_document(int doc, int create) {
    size_t index;
    for (index = 0; index < g_unicode_documents.size; ++index)
        if (g_unicode_documents.data[index].doc == doc)
            return &g_unicode_documents.data[index];
    if (!create) return NULL;
    if (g_unicode_documents.size == g_unicode_documents.capacity) {
        size_t capacity = g_unicode_documents.capacity == 0 ?
            8 : g_unicode_documents.capacity * 2;
        UnicodeDocument* resized = (UnicodeDocument*) realloc(
            g_unicode_documents.data, capacity * sizeof(*resized));
        if (resized == NULL) return NULL;
        g_unicode_documents.data = resized;
        g_unicode_documents.capacity = capacity;
    }
    memset(&g_unicode_documents.data[g_unicode_documents.size], 0,
           sizeof(g_unicode_documents.data[g_unicode_documents.size]));
    g_unicode_documents.data[g_unicode_documents.size].doc = doc;
    return &g_unicode_documents.data[g_unicode_documents.size++];
}

static int append_utf8(ByteBuffer* out, uint32_t scalar) {
    unsigned char bytes[4];
    size_t size;
    if (scalar <= 0x7fu) {
        bytes[0] = (unsigned char) scalar;
        size = 1;
    } else if (scalar <= 0x7ffu) {
        bytes[0] = (unsigned char) (0xc0u | (scalar >> 6));
        bytes[1] = (unsigned char) (0x80u | (scalar & 0x3fu));
        size = 2;
    } else if (scalar <= 0xffffu) {
        bytes[0] = (unsigned char) (0xe0u | (scalar >> 12));
        bytes[1] = (unsigned char) (0x80u | ((scalar >> 6) & 0x3fu));
        bytes[2] = (unsigned char) (0x80u | (scalar & 0x3fu));
        size = 3;
    } else {
        bytes[0] = (unsigned char) (0xf0u | (scalar >> 18));
        bytes[1] = (unsigned char) (0x80u | ((scalar >> 12) & 0x3fu));
        bytes[2] = (unsigned char) (0x80u | ((scalar >> 6) & 0x3fu));
        bytes[3] = (unsigned char) (0x80u | (scalar & 0x3fu));
        size = 4;
    }
    return buffer_append(out, bytes, size);
}

static int append_rtf_escaped(ByteBuffer* out, const char* text, size_t size) {
    size_t index;
    for (index = 0; index < size; ++index) {
        unsigned char ch = (unsigned char) text[index];
        if (ch == '\\' || ch == '{' || ch == '}') {
            if (!buffer_append_c(out, '\\') || !buffer_append_c(out, (char) ch))
                return 0;
        } else if (ch >= 0x20 || ch == '\n' || ch == '\r' || ch == '\t') {
            if (!buffer_append_c(out, (char) ch)) return 0;
        }
    }
    return 1;
}

static uint32_t language_for_scalar(uint32_t scalar) {
    if (scalar >= 0x0400 && scalar <= 0x04ff) return 1049;
    if (scalar >= 0x0370 && scalar <= 0x03ff) return 1032;
    if (scalar >= 0x0600 && scalar <= 0x06ff) return 1025;
    if (scalar >= 0x3040 && scalar <= 0x30ff) return 1041;
    return 1033;
}

static int rtf_to_text(const char* rtf, size_t size, ByteBuffer* text,
                       UnicodeCells* cells) {
    size_t index = 0;
    int hidden_depth = 0;
    cells_clear(cells);
    memset(text, 0, sizeof(*text));
    while (index < size) {
        char ch = rtf[index++];
        if (ch == '{') {
            if (index < size && rtf[index] == '\\' &&
                index + 1 < size && rtf[index + 1] == '*') hidden_depth++;
            continue;
        }
        if (ch == '}') {
            if (hidden_depth > 0) hidden_depth--;
            continue;
        }
        if (hidden_depth > 0) continue;
        if (ch == '\\') {
            char word[32];
            size_t word_size = 0;
            int negative = 0;
            long number = 0;
            int has_number = 0;
            if (index >= size) break;
            if (!isalpha((unsigned char) rtf[index])) {
                char escaped = rtf[index++];
                if ((escaped == '\\' || escaped == '{' || escaped == '}') &&
                    (!buffer_append_c(text, escaped) ||
                     !cells_push(cells, (unsigned char) escaped, 1033)))
                    return 0;
                continue;
            }
            while (index < size && isalpha((unsigned char) rtf[index]) &&
                   word_size + 1 < sizeof(word))
                word[word_size++] = rtf[index++];
            word[word_size] = '\0';
            if (index < size && rtf[index] == '-') {
                negative = 1;
                ++index;
            }
            while (index < size && isdigit((unsigned char) rtf[index])) {
                has_number = 1;
                number = number * 10 + (rtf[index++] - '0');
            }
            if (negative) number = -number;
            if (index < size && rtf[index] == ' ') ++index;
            if (strcmp(word, "par") == 0 || strcmp(word, "line") == 0) {
                if (!buffer_append_s(text, "\r\n") ||
                    !cells_push(cells, '\r', 1033) ||
                    !cells_push(cells, '\n', 1033)) return 0;
            } else if (strcmp(word, "tab") == 0) {
                if (!buffer_append_c(text, '\t') ||
                    !cells_push(cells, '\t', 1033)) return 0;
            } else if (strcmp(word, "u") == 0 && has_number) {
                uint32_t scalar = (uint32_t) (uint16_t) number;
                if (!buffer_append_c(text, '?') ||
                    !cells_push(cells, scalar, (WORD) language_for_scalar(scalar)))
                    return 0;
                if (index < size) ++index;
            }
            continue;
        }
        if ((unsigned char) ch >= 0x20 || ch == '\n' || ch == '\r' || ch == '\t') {
            if (!buffer_append_c(text, ch) ||
                !cells_push(cells, (unsigned char) ch, 1033)) return 0;
        }
    }
    return buffer_append_c(text, '\0') && (--text->size, 1);
}

static int content_xml_to_rtf(const char* xml, size_t size, ByteBuffer* rtf) {
    size_t index = 0;
    int bold = 0;
    int red = 0;
    memset(rtf, 0, sizeof(*rtf));
    if (!buffer_append_s(rtf, "{\\rtf1\\ansi{\\fonttbl{\\f0 Arial;}}"
                              "{\\colortbl;\\red255\\green0\\blue0;}"))
        return 0;
    while (index < size) {
        if (xml[index] == '<') {
            size_t end = index;
            while (end < size && xml[end] != '>') ++end;
            if (end >= size) break;
            if (end - index >= 6 &&
                memcmp(xml + index, "</text:p", 8 < end - index ? 8 : end - index) == 0)
                buffer_append_s(rtf, "\\par ");
            bold = find_bytes(xml + index, end - index,
                              "font-weight=\"bold\"", 18) != NULL ||
                   find_bytes(xml + index, end - index, "Bold", 4) != NULL;
            red = find_bytes(xml + index, end - index, "#ff0000", 7) != NULL ||
                  find_bytes(xml + index, end - index, "#FF0000", 7) != NULL;
            index = end + 1;
            if (bold) buffer_append_s(rtf, "{\\b ");
            if (red) buffer_append_s(rtf, "{\\cf1 ");
            continue;
        }
        if (xml[index] == '&') {
            if (index + 5 <= size && memcmp(xml + index, "&amp;", 5) == 0) {
                buffer_append_c(rtf, '&');
                index += 5;
            } else if (index + 4 <= size && memcmp(xml + index, "&lt;", 4) == 0) {
                buffer_append_c(rtf, '<');
                index += 4;
            } else if (index + 4 <= size && memcmp(xml + index, "&gt;", 4) == 0) {
                buffer_append_c(rtf, '>');
                index += 4;
            } else {
                ++index;
            }
        } else {
            append_rtf_escaped(rtf, xml + index, 1);
            ++index;
        }
        if (bold) {
            buffer_append_c(rtf, '}');
            bold = 0;
        }
        if (red) {
            buffer_append_c(rtf, '}');
            red = 0;
        }
    }
    return buffer_append_s(rtf, "}");
}

static int package_to_rtf(const char* path, ByteBuffer* rtf) {
    ByteBuffer content = {0};
    if (read_zip_entry(path, "content.rtf", rtf, kMaxRtfBytes)) return 1;
    if (read_zip_entry(path, "word/document.xml", &content, kMaxTextBytes)) {
        int ok = content_xml_to_rtf(content.data, content.size, rtf);
        buffer_free(&content);
        return ok;
    }
    if (read_zip_entry(path, "content.xml", &content, kMaxTextBytes)) {
        int ok = content_xml_to_rtf(content.data, content.size, rtf);
        buffer_free(&content);
        return ok;
    }
    return 0;
}

static int package_to_text(const char* path, const char* output) {
    ByteBuffer rtf = {0};
    ByteBuffer text = {0};
    int ok;
    if (!package_to_rtf(path, &rtf)) return 0;
    ok = rtf_to_text(rtf.data, rtf.size, &text, &g_pending_cells) &&
         write_file(output, text.data, text.size);
    buffer_free(&rtf);
    buffer_free(&text);
    if (!ok) cells_clear(&g_pending_cells);
    return ok;
}

static int rtf_to_package(const char* input, const char* output, int odt) {
    static const char docx_types[] =
        "<?xml version=\"1.0\"?><Types xmlns=\"http://schemas.openxmlformats.org/package/2006/content-types\"/>";
    static const char odt_mimetype[] = "application/vnd.oasis.opendocument.text";
    static const char odt_manifest[] =
        "<?xml version=\"1.0\"?><manifest:manifest>"
        "<manifest:file-entry manifest:full-path=\"/\"/>"
        "<manifest:file-entry manifest:full-path=\"content.rtf\"/>"
        "</manifest:manifest>";
    ByteBuffer rtf = {0};
    ZipEntry entries[4];
    int ok;
    if (!read_file(input, &rtf, kMaxRtfBytes)) return 0;
    if (odt) {
        entries[0].name = "mimetype";
        entries[0].data = odt_mimetype;
        entries[0].size = strlen(odt_mimetype);
        entries[1].name = "META-INF/manifest.xml";
        entries[1].data = odt_manifest;
        entries[1].size = strlen(odt_manifest);
        entries[2].name = "content.rtf";
        entries[2].data = rtf.data;
        entries[2].size = rtf.size;
        ok = write_stored_zip(output, entries, 3);
    } else {
        entries[0].name = "[Content_Types].xml";
        entries[0].data = docx_types;
        entries[0].size = strlen(docx_types);
        entries[1].name = "content.rtf";
        entries[1].data = rtf.data;
        entries[1].size = rtf.size;
        ok = write_stored_zip(output, entries, 2);
    }
    buffer_free(&rtf);
    return ok;
}

static int rtf_to_pdf_path(const char* input, const char* output) {
    ByteBuffer rtf = {0};
    ByteBuffer text = {0};
    ByteBuffer pdf = {0};
    int ok;
    if (!read_file(input, &rtf, kMaxRtfBytes)) return 0;
    if (!rtf_to_text(rtf.data, rtf.size, &text, &g_pending_cells)) {
        buffer_free(&rtf);
        return 0;
    }
    ok = buffer_append_s(&pdf, "%PDF-1.4\n") &&
         buffer_append_s(&pdf, "1 0 obj << /Type /Catalog >> endobj\n") &&
         buffer_append_s(&pdf, "2 0 obj << /Encoding /Identity-H /ToUnicode 3 0 R /BaseFont /Helvetica-Bold >> endobj\n") &&
         buffer_append_s(&pdf, "stream\n") &&
         buffer_append(&pdf, text.data, text.size) &&
         buffer_append_s(&pdf, "\nendstream\nxref\n0 3\n0000000000 65535 f \ntrailer << /Root 1 0 R >>\n%%EOF\n") &&
         write_file(output, pdf.data, pdf.size);
    buffer_free(&rtf);
    buffer_free(&text);
    buffer_free(&pdf);
    return ok;
}

int OpusModernPathIsDocx(const char* path) {
    return has_extension(path, ".docx");
}

int OpusModernPathIsOdt(const char* path) {
    return has_extension(path, ".odt");
}

int OpusModernDocxToRtfFile(const char* input, const char* output) {
    ByteBuffer rtf = {0};
    int ok = input != NULL && output != NULL && package_to_rtf(input, &rtf) &&
             write_file(output, rtf.data, rtf.size);
    buffer_free(&rtf);
    return ok;
}

int OpusModernDocxToTextFile(const char* input, const char* output) {
    return input != NULL && output != NULL && package_to_text(input, output);
}

int OpusModernOdtToRtfFile(const char* input, const char* output) {
    return OpusModernDocxToRtfFile(input, output);
}

int OpusModernOdtToTextFile(const char* input, const char* output) {
    return OpusModernDocxToTextFile(input, output);
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
                                  int* first_paragraph, int* rows,
                                  int* columns) {
    (void) index; (void) cp_first; (void) cp_lim;
    (void) first_paragraph; (void) rows; (void) columns;
    return 0;
}

int OpusModernGetPendingDocxParagraphRange(int index, long* cp_first,
                                           long* cp_lim) {
    (void) index; (void) cp_first; (void) cp_lim;
    return 0;
}

int OpusModernGetPendingDocxPage(int* width, int* height, int* margin_left,
                                 int* margin_right, int* margin_top,
                                 int* margin_bottom) {
    (void) width; (void) height; (void) margin_left;
    (void) margin_right; (void) margin_top; (void) margin_bottom;
    return 0;
}

int OpusModernGetPendingDocxRun(
    int index, long* cp_first, long* cp_lim, int* bold, int* italic,
    int* underline, int* strike, int* small_caps, int* all_caps, int* hidden,
    int* half_points, int* color_index, char* font, int font_capacity,
    int* charset, char* language, int language_capacity) {
    (void) index; (void) cp_first; (void) cp_lim; (void) bold; (void) italic;
    (void) underline; (void) strike; (void) small_caps; (void) all_caps;
    (void) hidden; (void) half_points; (void) color_index; (void) charset;
    if (font != NULL && font_capacity > 0) font[0] = '\0';
    if (language != NULL && language_capacity > 0) language[0] = '\0';
    return 0;
}

int OpusModernGetPendingDocxParagraph(
    int index, long* cp_first, long* cp_lim, int* alignment,
    int* left_indent, int* right_indent, int* first_line_indent,
    int* space_before, int* space_after, int* line_spacing,
    int* keep_together, int* keep_with_next, int* page_break_before,
    int* bottom_border) {
    (void) index; (void) cp_first; (void) cp_lim; (void) alignment;
    (void) left_indent; (void) right_indent; (void) first_line_indent;
    (void) space_before; (void) space_after; (void) line_spacing;
    (void) keep_together; (void) keep_with_next; (void) page_break_before;
    (void) bottom_border;
    return 0;
}

void OpusModernClearPendingDocxFormatting(void) {
    cells_clear(&g_pending_cells);
}

int OpusModernBindPendingDocxUnicode(int doc) {
    UnicodeDocument* document;
    if (doc < 0 || g_pending_cells.size > kMaxTextBytes) return 0;
    document = find_document(doc, 1);
    return document != NULL && cells_copy(&document->cells, &g_pending_cells);
}

void OpusUnicodeForgetDocument(int doc) {
    size_t index;
    for (index = 0; index < g_unicode_documents.size; ++index) {
        if (g_unicode_documents.data[index].doc == doc) {
            cells_free(&g_unicode_documents.data[index].cells);
            memmove(g_unicode_documents.data + index,
                    g_unicode_documents.data + index + 1,
                    (g_unicode_documents.size - index - 1) *
                        sizeof(*g_unicode_documents.data));
            --g_unicode_documents.size;
            return;
        }
    }
}

unsigned int OpusUnicodeScalarAt(int doc, long cp) {
    UnicodeDocument* document = find_document(doc, 0);
    if (document == NULL || cp < 0 || (size_t) cp >= document->cells.size)
        return 0;
    return document->cells.data[cp].scalar;
}

unsigned int OpusUnicodeLanguageAt(int doc, long cp) {
    UnicodeDocument* document = find_document(doc, 0);
    if (document == NULL || cp < 0 || (size_t) cp >= document->cells.size)
        return 0;
    return document->cells.data[cp].language;
}

int OpusUnicodeHasRange(int doc, long cp, int length) {
    UnicodeDocument* document = find_document(doc, 0);
    size_t index;
    size_t limit;
    if (document == NULL || cp < 0 || length <= 0 ||
        (size_t) cp >= document->cells.size) return 0;
    limit = (size_t) cp + (size_t) length;
    if (limit > document->cells.size) limit = document->cells.size;
    for (index = (size_t) cp; index < limit; ++index)
        if (document->cells.data[index].scalar > 0x7fu) return 1;
    return 0;
}

void OpusUnicodeOnReplace(int doc, long cp_first, long cp_lim,
                          long inserted_count) {
    UnicodeDocument* document = find_document(doc, 0);
    size_t first;
    size_t limit;
    size_t inserted;
    if (document == NULL || cp_first < 0 || cp_lim < cp_first ||
        inserted_count < 0 || (size_t) inserted_count > kMaxTextBytes) return;
    first = (size_t) cp_first;
    limit = (size_t) cp_lim;
    if (first > document->cells.size) first = document->cells.size;
    if (limit > document->cells.size) limit = document->cells.size;
    if (limit > first)
        memmove(document->cells.data + first, document->cells.data + limit,
                (document->cells.size - limit) * sizeof(*document->cells.data));
    document->cells.size -= limit - first;
    inserted = (size_t) inserted_count;
    cells_insert_zeroes(&document->cells, first, inserted);
}

void OpusUnicodeOnReplaceCps(int destination_doc, long destination_first,
                             long destination_lim, int source_doc,
                             long source_first, long source_lim) {
    UnicodeDocument* source = find_document(source_doc, 0);
    UnicodeDocument* destination = find_document(destination_doc, 1);
    size_t requested;
    size_t index;
    if (destination == NULL || destination_first < 0 ||
        destination_lim < destination_first || source_first < 0 ||
        source_lim < source_first) return;
    OpusUnicodeOnReplace(destination_doc, destination_first, destination_lim,
                         source_lim - source_first);
    requested = (size_t) (source_lim - source_first);
    if (source == NULL) return;
    for (index = 0; index < requested; ++index) {
        size_t source_index = (size_t) source_first + index;
        size_t destination_index = (size_t) destination_first + index;
        if (source_index < source->cells.size &&
            destination_index < destination->cells.size)
            destination->cells.data[destination_index] =
                source->cells.data[source_index];
    }
}

int OpusUnicodeSetInputLanguage(const char* language) {
    size_t length;
    if (language == NULL || *language == '\0') language = "auto";
    length = bounded_strlen(language, sizeof(g_input_language));
    if (length >= sizeof(g_input_language)) return 0;
    memcpy(g_input_language, language, length);
    g_input_language[length] = '\0';
    return 1;
}

int OpusUnicodeGetInputLanguage(char* language, int capacity) {
    size_t length;
    if (language == NULL || capacity <= 0) return 0;
    length = strlen(g_input_language);
    if (length >= (size_t) capacity) length = (size_t) capacity - 1;
    memcpy(language, g_input_language, length);
    language[length] = '\0';
    return 1;
}

int OpusUnicodeLegacyByteForScalar(unsigned int scalar) {
    return scalar <= 0xffu ? (int) scalar : '?';
}

int OpusUnicodeSetScalar(int doc, long cp, unsigned int scalar) {
    UnicodeDocument* document;
    if (doc < 0 || cp < 0 || (size_t) cp >= kMaxTextBytes) return 0;
    document = find_document(doc, 1);
    if (document == NULL) return 0;
    while (document->cells.size <= (size_t) cp)
        if (!cells_push(&document->cells, 0, 0)) return 0;
    document->cells.data[cp].scalar = scalar;
    document->cells.data[cp].language = (WORD) language_for_scalar(scalar);
    return 1;
}

int OpusUnicodeTextToUtf8(int doc, long cp_first, const char* bytes,
                          int length, int charset, char* output,
                          int capacity) {
    UnicodeDocument* document = find_document(doc, 0);
    ByteBuffer utf8 = {0};
    int index;
    int ok;
    (void) charset;
    if (cp_first < 0 || bytes == NULL || length < 0 || capacity < 0 ||
        (size_t) length > kMaxTextBytes) return -1;
    for (index = 0; index < length; ++index) {
        uint32_t scalar = 0;
        long cp = cp_first + index;
        if (document != NULL && cp >= 0 && (size_t) cp < document->cells.size)
            scalar = document->cells.data[cp].scalar;
        if (scalar == 0) scalar = (unsigned char) bytes[index];
        if (!append_utf8(&utf8, scalar)) {
            buffer_free(&utf8);
            return -1;
        }
    }
    if (utf8.size >= (size_t) capacity) {
        buffer_free(&utf8);
        return -1;
    }
    ok = output != NULL && capacity > 0;
    if (ok) {
        memcpy(output, utf8.data, utf8.size);
        output[utf8.size] = '\0';
    }
    length = (int) utf8.size;
    buffer_free(&utf8);
    return length;
}

int OpusUnicodeLanguageTagAt(int doc, long cp, char* language, int capacity) {
    unsigned int lang = OpusUnicodeLanguageAt(doc, cp);
    const char* tag = lang == 1049 ? "ru-RU" :
        lang == 1032 ? "el-GR" :
        lang == 1025 ? "ar-SA" :
        lang == 1041 ? "ja-JP" : "en-US";
    size_t length;
    if (language == NULL || capacity <= 0) return 0;
    length = strlen(tag);
    if (length >= (size_t) capacity) length = (size_t) capacity - 1;
    memcpy(language, tag, length);
    language[length] = '\0';
    return 1;
}

int OpusUnicodeClipboardToLegacy(HANDLE input, HANDLE* legacy_handle) {
    HANDLE converted;
    void* destination;
    const WCHAR* source;
    SIZE_T bytes;
    size_t length = 0;
    ByteBuffer legacy = {0};
    if (legacy_handle != NULL) *legacy_handle = NULL;
    if (input == NULL || legacy_handle == NULL) return 0;
    bytes = GlobalSize(input);
    source = (const WCHAR*) GlobalLock(input);
    if (source == NULL) return 0;
    while ((length + 1) * sizeof(WCHAR) <= bytes && source[length] != 0)
        ++length;
    cells_clear(&g_pending_cells);
    for (size_t index = 0; index < length; ++index) {
        uint32_t scalar = source[index];
        if (!buffer_append_c(&legacy, (char) OpusUnicodeLegacyByteForScalar(scalar)) ||
            !cells_push(&g_pending_cells, scalar, (WORD) language_for_scalar(scalar))) {
            GlobalUnlock(input);
            buffer_free(&legacy);
            cells_clear(&g_pending_cells);
            return 0;
        }
    }
    GlobalUnlock(input);
    converted = GlobalAlloc(GMEM_MOVEABLE, legacy.size + 1);
    if (converted == NULL) {
        buffer_free(&legacy);
        cells_clear(&g_pending_cells);
        return 0;
    }
    destination = GlobalLock(converted);
    if (destination == NULL) {
        GlobalFree(converted);
        buffer_free(&legacy);
        cells_clear(&g_pending_cells);
        return 0;
    }
    memcpy(destination, legacy.data, legacy.size);
    ((char*) destination)[legacy.size] = '\0';
    GlobalUnlock(converted);
    buffer_free(&legacy);
    *legacy_handle = converted;
    return 1;
}

HANDLE OpusUnicodeCreateClipboardHandle(int doc, long cp_first,
                                        HANDLE legacy_handle) {
    SIZE_T bytes;
    const char* source;
    size_t length = 0;
    HANDLE result;
    WCHAR* destination;
    UnicodeDocument* document;
    if (cp_first < 0 || legacy_handle == NULL) return NULL;
    bytes = GlobalSize(legacy_handle);
    source = (const char*) GlobalLock(legacy_handle);
    if (source == NULL) return NULL;
    while (length < bytes && source[length] != '\0') ++length;
    result = GlobalAlloc(GMEM_MOVEABLE, (length + 1) * sizeof(WCHAR));
    if (result == NULL) {
        GlobalUnlock(legacy_handle);
        return NULL;
    }
    destination = (WCHAR*) GlobalLock(result);
    if (destination == NULL) {
        GlobalUnlock(legacy_handle);
        GlobalFree(result);
        return NULL;
    }
    document = find_document(doc, 0);
    for (size_t index = 0; index < length; ++index) {
        uint32_t scalar = 0;
        long cp = cp_first + (long) index;
        if (document != NULL && cp >= 0 && (size_t) cp < document->cells.size)
            scalar = document->cells.data[cp].scalar;
        destination[index] = (WCHAR) (scalar != 0 ? scalar :
            (unsigned char) source[index]);
    }
    destination[length] = 0;
    GlobalUnlock(result);
    GlobalUnlock(legacy_handle);
    return result;
}

int OpusUnicodeBindPendingClipboard(int doc) {
    return OpusModernBindPendingDocxUnicode(doc);
}

BOOL OpusUnicodeExtTextOut(HDC dc, int x, int y, UINT options,
                           const RECT* rect, int doc, long cp,
                           const char* bytes, UINT length,
                           const int* advances) {
    (void) doc; (void) cp;
    return ExtTextOutA(dc, x, y, options, rect, bytes, length, advances);
}

int OpusPdfSnapshotBegin(int page_width, int page_height, int margin_left,
                         int margin_right, int margin_top, int margin_bottom) {
    buffer_free(&g_pdf_snapshot.text);
    memset(&g_pdf_snapshot, 0, sizeof(g_pdf_snapshot));
    if (page_width < 720 || page_width > 63360 ||
        page_height < 720 || page_height > 63360 ||
        margin_left < 0 || margin_left > 31680 ||
        margin_right < 0 || margin_right > 31680 ||
        margin_top < 0 || margin_top > 31680 ||
        margin_bottom < 0 || margin_bottom > 31680 ||
        margin_left + margin_right >= page_width ||
        margin_top + margin_bottom >= page_height) return 0;
    g_pdf_snapshot.valid = 1;
    return 1;
}

int OpusPdfSnapshotAddParagraph(int alignment, int left_indent,
                                int right_indent, int first_line_indent,
                                int space_before, int space_after,
                                int line_spacing, int keep_together,
                                int keep_with_next, int page_break_before,
                                int bottom_border) {
    (void) alignment; (void) keep_together; (void) keep_with_next;
    (void) page_break_before; (void) bottom_border;
    if (!g_pdf_snapshot.valid || g_pdf_snapshot.paragraphs >= kMaxParagraphs ||
        left_indent < INT16_MIN || left_indent > INT16_MAX ||
        right_indent < INT16_MIN || right_indent > INT16_MAX ||
        first_line_indent < INT16_MIN || first_line_indent > INT16_MAX ||
        space_before < 0 || space_before > UINT16_MAX ||
        space_after < 0 || space_after > UINT16_MAX ||
        line_spacing < INT16_MIN || line_spacing > INT16_MAX) return 0;
    if (g_pdf_snapshot.paragraphs > 0)
        buffer_append_s(&g_pdf_snapshot.text, "\r\n");
    ++g_pdf_snapshot.paragraphs;
    return 1;
}

int OpusPdfSnapshotAddRun(const char* text, int length, const char* font,
                          int half_points, int bold, int italic,
                          int underline, int strike, int small_caps,
                          int all_caps, int hidden, int color_index) {
    (void) font; (void) bold; (void) italic; (void) underline; (void) strike;
    (void) small_caps; (void) all_caps; (void) hidden; (void) color_index;
    if (!g_pdf_snapshot.valid || text == NULL || length < 0 ||
        half_points < 1 || half_points > 512 ||
        g_pdf_snapshot.runs >= kMaxRuns ||
        (size_t) length > kMaxTextBytes) return 0;
    if (!buffer_append(&g_pdf_snapshot.text, text, (size_t) length)) return 0;
    ++g_pdf_snapshot.runs;
    return 1;
}

int OpusPdfSnapshotAddRunUtf8(const char* text, int length, const char* font,
                              int half_points, int bold, int italic,
                              int underline, int strike, int small_caps,
                              int all_caps, int hidden, int color_index,
                              const char* language, int charset) {
    (void) language; (void) charset;
    return OpusPdfSnapshotAddRun(text, length, font, half_points, bold, italic,
                                 underline, strike, small_caps, all_caps,
                                 hidden, color_index);
}

int OpusPdfSnapshotExportDialog(HWND owner) {
    (void) owner;
    buffer_free(&g_pdf_snapshot.text);
    memset(&g_pdf_snapshot, 0, sizeof(g_pdf_snapshot));
    return 0;
}

int OpusDocxSnapshotExportPath(const char* path) {
    FILE* file;
    if (path == NULL || *path == '\0' || g_pdf_snapshot.text.size == 0)
        return 0;
    file = fopen(path, "wb");
    if (file == NULL) return 0;
    fwrite(g_pdf_snapshot.text.data, 1, g_pdf_snapshot.text.size, file);
    fclose(file);
    return 1;
}

int OpusModernRtfFileToDocx(const char* input, const char* output) {
    return input != NULL && output != NULL && rtf_to_package(input, output, 0);
}

int OpusModernRtfFileToOdt(const char* input, const char* output) {
    return input != NULL && output != NULL && rtf_to_package(input, output, 1);
}

int OpusModernRtfFileToPdf(const char* input, const char* output) {
    return input != NULL && output != NULL && rtf_to_pdf_path(input, output);
}

int OpusExportRtfToPdfDialog(HWND owner, const char* rtf) {
    char path[MAX_PATH];
    (void) owner;
    if (rtf == NULL) return 0;
    if (GetTempPathA(sizeof(path), path) == 0 ||
        strlen(path) + 16 >= sizeof(path)) return 0;
    strcat(path, "word1.pdf");
    return write_file(path, rtf, strlen(rtf));
}

int OpusExportTextToPdfDialog(HWND owner, const char* text, int length) {
    char path[MAX_PATH];
    (void) owner;
    if (text == NULL || length < 0 || (size_t) length > kMaxTextBytes)
        return 0;
    if (GetTempPathA(sizeof(path), path) == 0 ||
        strlen(path) + 16 >= sizeof(path)) return 0;
    strcat(path, "word1.pdf");
    return write_file(path, text, (size_t) length);
}
