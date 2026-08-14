#include "opus_x64_layout.h"

/*
 * AMD64 translations of the document-descriptor shortcuts in
 * Opus/asm/fetchn2.asm. Movable handles remain double-indirect, but native
 * pointers replace the original segment:offset address calculations.
 */

extern void** mpdochdod[];

enum { kDocNil = 0 };

static void* DodOrNull(const int doc) {
    void** handle = mpdochdod[doc];
    return handle == NULL ? NULL : *handle;
}

int DocMother(const int doc) {
    void* const pdod = DodOrNull(doc);
    if (pdod == NULL) {
        return kDocNil;
    }
    return OpusDodIsMother(pdod) ? doc : OpusDodMotherDoc(pdod);
}

void* N_PdodMother(const int doc) {
    void* pdod = DodOrNull(doc);
    if (pdod == NULL || OpusDodIsMother(pdod)) {
        return pdod;
    }
    return DodOrNull(OpusDodMotherDoc(pdod));
}

int DocDotMother(int doc) {
    for (;;) {
        void* const pdod = DodOrNull(doc);
        if (pdod == NULL) {
            return kDocNil;
        }
        if (OpusDodIsDocument(pdod)) {
            return OpusDodTemplateDoc(pdod);
        }
        if (OpusDodIsTemplate(pdod)) {
            return doc;
        }
        doc = OpusDodMotherDoc(pdod);
    }
}
