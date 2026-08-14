#include <stddef.h>

/* Direct AMD64 C translation of Opus/asm/sttbn.asm. */
void AddDcbToLprgbst(int* offsets, const int count, const int delta,
                     const int threshold) {
    int index;

    if (offsets == NULL || count <= 0) {
        return;
    }
    for (index = 0; index < count; ++index) {
        if (offsets[index] >= threshold) {
            offsets[index] += delta;
        }
    }
}
