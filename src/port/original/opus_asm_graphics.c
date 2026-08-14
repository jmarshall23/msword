#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* AMD64 translations of the active GRBITX.ASM and GRHC.ASM entry points. */
int SlowStrBltRow(void* picture, unsigned char* source,
                  unsigned char* destination);
void C_GetMonoStrBltInfo(void* picture, unsigned int* values);
int C_ReadTIFFLine(void** picture_handle, unsigned char* plane);

void StrBltRow(void* picture, unsigned char* source,
               unsigned char* destination) {
    (void)SlowStrBltRow(picture, source, destination);
}

void MonoStrBltRow(void* picture, const unsigned char* source,
                   unsigned char* destination, const unsigned int row,
                   const unsigned char* patterns) {
    unsigned int values[7] = {0};
    unsigned int input_planes;
    unsigned int output_planes;
    size_t plane_bytes;
    unsigned int source_width;
    unsigned int destination_width;
    unsigned int normal_repeat;
    unsigned int repeat_delta;
    size_t destination_bytes;
    const unsigned char* pattern_row;
    unsigned int remainder = 0;
    unsigned int destination_bit = 0;
    unsigned int source_bit;
    unsigned int repeat;
    unsigned int color;
    unsigned int plane;
    unsigned char pattern;
    unsigned char byte;
    unsigned int bit_in_byte;

    if (picture == NULL || source == NULL || destination == NULL ||
        patterns == NULL) {
        return;
    }

    C_GetMonoStrBltInfo(picture, values);
    input_planes = values[0];
    output_planes = values[1];
    plane_bytes = values[2];
    source_width = values[3];
    destination_width = values[4];
    normal_repeat = values[5];
    repeat_delta = values[6];

    if (input_planes == 0 || input_planes > 4 || output_planes != 1 ||
        source_width == 0) {
        return;
    }

    destination_bytes = ((size_t)destination_width + 7u) / 8u;
    memset(destination, 0, destination_bytes);

    pattern_row = patterns + ((row & 7u) * 16u);
    for (source_bit = 0; source_bit < source_width &&
         destination_bit < destination_width;
         ++source_bit) {
        repeat = normal_repeat;
        color = 0;
        remainder += repeat_delta;
        if (remainder >= source_width) {
            ++repeat;
            remainder -= source_width;
        }

        for (plane = input_planes; plane-- > 0;) {
            byte = source[(size_t)plane * plane_bytes + source_bit / 8u];
            color = (color << 1u) |
                    ((byte >> (7u - (source_bit & 7u))) & 1u);
        }

        pattern = pattern_row[color & 0x0fu];
        while (repeat-- > 0 && destination_bit < destination_width) {
            bit_in_byte = destination_bit & 7u;
            if (((pattern >> (7u - bit_in_byte)) & 1u) != 0) {
                destination[destination_bit / 8u] |=
                    (unsigned char)(0x80u >> bit_in_byte);
            }
            ++destination_bit;
        }
    }
}

void ReadTIFFLineNat(void** picture_handle, unsigned char* plane) {
    (void)C_ReadTIFFLine(picture_handle, plane);
}

unsigned int UDiv(const unsigned int numerator,
                  const unsigned int denominator,
                  unsigned int* remainder) {
    if (denominator == 0) {
        if (remainder != NULL) {
            *remainder = 0;
        }
        return 0xffffu;
    }
    if (remainder != NULL) {
        *remainder = numerator % denominator;
    }
    return numerator / denominator;
}
