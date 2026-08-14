#include "opus_x64_layout.h"

/* AMD64 translation of Opus/asm/resn2.asm:N_PmwdWw. */

extern void** mpwwhwwd[];
extern void** mpmwhmwd[];

static void* DescriptorFromTable(void*** table, const int index) {
    void** handle = table[index];
    return handle == NULL ? NULL : *handle;
}

void* N_PmwdWw(const int ww) {
    void* const pwwd = DescriptorFromTable(mpwwhwwd, ww);
    if (pwwd == NULL) {
        return NULL;
    }
    return DescriptorFromTable(mpmwhmwd, OpusWwdMw(pwwd));
}
