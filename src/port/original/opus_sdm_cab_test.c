#include "opus_x64_compat.h"
#include "opus_x64_heap.h"

#include <stdint.h>
#include <string.h>

typedef uint16_t Word;
typedef void** CabHandle;

CabHandle HcabAlloc_sdm21(Word initializer);
void FreeCab(CabHandle cab);
void NinchCab(CabHandle cab);
int FSetCabSz(CabHandle cab, const char* text, Word iag);
void GetCabSz(CabHandle cab, char* text, Word capacity, Word iag);
int FSetCabSt(CabHandle cab, const unsigned char* text, Word iag);
void GetCabStz(CabHandle cab, unsigned char* text, Word capacity, Word iag);
CabHandle HcabDupeCab(CabHandle cab);

enum {
    kCabMinWords = 3,
    kPointerWords = sizeof(void*) / sizeof(Word),
    kHandleBaseIag =
        ((kCabMinWords + kPointerWords - 1) / kPointerWords * kPointerWords) -
        kCabMinWords,
    kHandleCount = 2,
    kArgumentWords = kHandleBaseIag + kHandleCount * kPointerWords + 2,
    kTotalWords = kCabMinWords + kArgumentWords,
    kInitializer = kTotalWords | (kHandleCount << 8)
};

static int fail(int line) {
    return line;
}

int main(void) {
    size_t initial_heap = OpusHeapBytesUsed();
    CabHandle cab = HcabAlloc_sdm21((Word)kInitializer);
    const Word* words;
    const unsigned char counted[] = {4, 'B', 'e', 't', 'a'};
    char sz[16] = {0};
    unsigned char stz[16] = {0};
    Word* mutable_words;
    CabHandle duplicate;

    if (cab == NULL) return fail(__LINE__);

    words = (const Word*)OpusDerefH(cab);
    if (words[0] != kArgumentWords ||
        words[1] != kHandleCount * kPointerWords) {
        return fail(__LINE__);
    }

    if (!FSetCabSz(cab, "Alpha", kHandleBaseIag)) return fail(__LINE__);
    if (!FSetCabSt(cab, counted, kHandleBaseIag + kPointerWords)) {
        return fail(__LINE__);
    }

    GetCabSz(cab, sz, sizeof(sz), kHandleBaseIag);
    if (strcmp(sz, "Alpha") != 0) return fail(__LINE__);

    GetCabStz(cab, stz, sizeof(stz), kHandleBaseIag + kPointerWords);
    if (stz[0] != 4 || memcmp(stz + 1, "Beta", 5) != 0) {
        return fail(__LINE__);
    }

    mutable_words = (Word*)OpusDerefH(cab);
    mutable_words[kCabMinWords + kHandleBaseIag +
                  kHandleCount * kPointerWords] = 42;
    duplicate = HcabDupeCab(cab);
    if (duplicate == NULL) return fail(__LINE__);

    GetCabSz(duplicate, sz, sizeof(sz), kHandleBaseIag);
    if (strcmp(sz, "Alpha") != 0 ||
        ((Word*)OpusDerefH(duplicate))[kCabMinWords + kHandleBaseIag +
                                       kHandleCount * kPointerWords] != 42) {
        return fail(__LINE__);
    }

    NinchCab(cab);
    GetCabSz(cab, sz, sizeof(sz), kHandleBaseIag);
    mutable_words = (Word*)OpusDerefH(cab);
    if (sz[0] != '\0' ||
        mutable_words[kCabMinWords + kHandleBaseIag +
                      kHandleCount * kPointerWords] != 0xffffu) {
        return fail(__LINE__);
    }

    FreeCab(duplicate);
    FreeCab(cab);
    return OpusHeapBytesUsed() == initial_heap ? 0 : fail(__LINE__);
}
