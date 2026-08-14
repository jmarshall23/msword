#include <stddef.h>

/*
 * AMD64 translation of Opus/asm/resn.asm:LRghEngine and its four exported
 * descriptor-table entry points. The original segment/word address
 * calculation becomes ordinary native array indexing; the double-indirect
 * movable-handle dereference remains unchanged.
 */

extern void** mpdochdod[];
extern void** mpfnhfcb[];
extern void** mpwwhwwd[];
extern void** mpmwhmwd[];

static void* DescriptorFromTable(void*** table, const int index) {
    void** handle = table[index];
    return handle == NULL ? NULL : *handle;
}

void* N_PdodDoc(const int doc) {
    return DescriptorFromTable(mpdochdod, doc);
}

void* N_PfcbFn(const int fn) {
    return DescriptorFromTable(mpfnhfcb, fn);
}

void* N_PwwdWw(const int ww) {
    return DescriptorFromTable(mpwwhwwd, ww);
}

void** N_HwwdWw(const int ww) {
    return mpwwhwwd[ww];
}

void* N_PmwdMw(const int mw) {
    return DescriptorFromTable(mpmwhmwd, mw);
}
