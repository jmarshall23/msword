#include "opus_x64_layout.h"

#include <stddef.h>

/* AMD64 translation of Opus/asm/resn2.asm:HpInPl. */

unsigned char* HpInPl(void** hpl, const int index) {
    unsigned char* base;

    if (hpl == NULL || *hpl == NULL) {
        return NULL;
    }
    base = (unsigned char*)OpusPlData(*hpl);
    return base + (size_t)index * (size_t)OpusPlEntrySize(*hpl);
}

unsigned char* PInPl(void** hpl, const int index) {
    return HpInPl(hpl, index);
}
