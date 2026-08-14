#include "opus_x64_layout.h"

#include <stdint.h>
#include <stddef.h>
#include <string.h>

/*
 * AMD64 translations of the flat-memory utility and document-range slice of
 * Opus/asm/resn.asm, Opus/asm/resn2.asm, and Opus/asm/fetchn2.asm.  The old
 * routines spent most of their instructions managing far pointers and the
 * 16-bit calling convention; the algorithms below are the C descriptions
 * embedded beside that assembly, with the same public entry names.
 */

extern void **mpdochdod[];

enum {
    kCcpEop = 1,
    kDocNil = 0
};

typedef struct OpusCa {
    long cpFirst;
    long cpLim;
    int doc;
} OpusCa;

typedef struct OpusRect {
    int left;
    int top;
    int right;
    int bottom;
} OpusRect;

static void *dod_or_null(int doc) {
    void **handle = mpdochdod[doc];
    return handle == NULL ? NULL : *handle;
}

static void *move_bytes(const void *source, void *destination,
                        size_t byte_count) {
    if (byte_count != 0) {
        memmove(destination, source, byte_count);
    }
    return (unsigned char *)destination + byte_count;
}

long CpMax(long first, long second) {
    return first > second ? first : second;
}

long CpMin(long first, long second) {
    return first < second ? first : second;
}

unsigned int umax(unsigned int first, unsigned int second) {
    return first > second ? first : second;
}

unsigned int umin(unsigned int first, unsigned int second) {
    return first < second ? first : second;
}

int N_abs(int value) {
    return value < 0 ? -value : value;
}

int NMultDiv(int value, int numerator, int denominator) {
    int64_t product = (int64_t)value * numerator;
    int negative = (product < 0) != (denominator < 0);
    uint64_t magnitude = product < 0 ? (uint64_t)(-product)
                                     : (uint64_t)product;
    uint64_t divisor = denominator < 0
        ? (uint64_t)(-(int64_t)denominator)
        : (uint64_t)denominator;
    uint64_t quotient = UINT16_MAX;
    int result;

    if (divisor != 0) {
        quotient = (magnitude + divisor / 2) / divisor;
    }
    if (quotient > 0x7fffU) {
        quotient = 0x7fffU;
    }
    result = (int)quotient;
    return negative ? -result : result;
}

unsigned int UMultDiv(unsigned int value, unsigned int numerator,
                      unsigned int denominator) {
    uint64_t product;
    uint64_t quotient;

    if (denominator == 0) {
        return 0xffffU;
    }
    product = (uint64_t)value * numerator;
    quotient = (product + (uint64_t)denominator / 2) / denominator;
    return quotient > 0xffffU ? 0xffffU : (unsigned int)quotient;
}

void *bltbyte(const void *source, void *destination, int byte_count) {
    return move_bytes(source, destination, (size_t)byte_count);
}

void *bltbyteNat(const void *source, void *destination, int byte_count) {
    return bltbyte(source, destination, byte_count);
}

void *bltbx(const void *source, void *destination, int byte_count) {
    return bltbyte(source, destination, byte_count);
}

void *bltbxNat(const void *source, void *destination, int byte_count) {
    return bltbyte(source, destination, byte_count);
}

void *blt(const void *source, void *destination, int word_count) {
    return move_bytes(source, destination, (size_t)word_count * sizeof(int));
}

void *bltNat(const void *source, void *destination, int word_count) {
    return blt(source, destination, word_count);
}

void *bltx(const void *source, void *destination, int word_count) {
    return blt(source, destination, word_count);
}

void *bltxNat(const void *source, void *destination, int word_count) {
    return blt(source, destination, word_count);
}

void *bltbcx(int value, void *destination, int byte_count) {
    memset(destination, (unsigned char)value, (size_t)byte_count);
    return destination;
}

void *bltcx(int value, int *destination, int word_count) {
    int index;
    for (index = 0; index < word_count; ++index) {
        destination[index] = value;
    }
    return destination;
}

int FNeRgch(const void *first, const void *second, int byte_count) {
    return byte_count != 0 &&
           memcmp(first, second, (size_t)byte_count) != 0;
}

int FNeRgw(const int *first, const int *second, int word_count) {
    return FNeRgch(first, second, (int)sizeof(int) * word_count);
}

int FNeSt(const unsigned char *first, const unsigned char *second) {
    return first[0] != second[0] || FNeRgch(first + 1, second + 1, first[0]);
}

int CchNonZeroPrefix(const unsigned char *bytes, int count) {
    while (count > 0 && bytes[count - 1] == 0) {
        --count;
    }
    return count;
}

int DrcToRc(const OpusRect *dimensions, OpusRect *rectangle) {
    rectangle->left = dimensions->left;
    rectangle->top = dimensions->top;
    rectangle->right = dimensions->left + dimensions->right;
    rectangle->bottom = dimensions->top + dimensions->bottom;
    return 0;
}

int RcToDrc(const OpusRect *rectangle, OpusRect *dimensions) {
    dimensions->left = rectangle->left;
    dimensions->top = rectangle->top;
    dimensions->right = rectangle->right - rectangle->left;
    dimensions->bottom = rectangle->bottom - rectangle->top;
    return 0;
}

int FEmptyRc(const OpusRect *rectangle) {
    return rectangle->top >= rectangle->bottom ||
           rectangle->left >= rectangle->right;
}

int FSectRc(const OpusRect *first, const OpusRect *second,
            OpusRect *destination) {
    destination->left = first->left > second->left ? first->left : second->left;
    destination->top = first->top > second->top ? first->top : second->top;
    destination->right = first->right < second->right
        ? first->right : second->right;
    destination->bottom = first->bottom < second->bottom
        ? first->bottom : second->bottom;
    if (destination->left > destination->right) {
        destination->left = destination->right;
    }
    if (destination->top > destination->bottom) {
        destination->top = destination->bottom;
    }
    return !FEmptyRc(destination);
}

int FIsectIval(const OpusCa *range, long first, long limit) {
    return range->cpLim >= first && range->cpFirst <= limit;
}

char *StringMap(const char *text, int counted, int location) {
    static _Thread_local unsigned char buffers[16][256];
    static _Thread_local unsigned int next_buffer;
    unsigned char *result;
    size_t count;

    (void)location;
    if (!counted) {
        return (char *)text;
    }
    result = buffers[next_buffer++ % 16];
    count = strlen(text);
    if (count > 254) {
        count = 254;
    }
    result[0] = (unsigned char)count;
    memcpy(result + 1, text, count);
    result[count + 1] = 0;
    return (char *)result;
}

long CpMac2Doc(int doc) {
    void *pdod = dod_or_null(doc);
    return pdod == NULL ? 0 : OpusDodCpMac(pdod);
}

long CpMacDoc(int doc) {
    return CpMac2Doc(doc) - 2 * kCcpEop;
}

long CpMac1Doc(int doc) {
    return CpMac2Doc(doc) - kCcpEop;
}

long CpMacDocEdit(int doc) {
    return CpMac2Doc(doc) - 3 * kCcpEop;
}

OpusCa *PcaSet(OpusCa *range, int doc, long cp_first, long cp_lim) {
    range->doc = doc;
    range->cpFirst = cp_first;
    range->cpLim = cp_lim;
    return range;
}

OpusCa *PcaSetDcp(OpusCa *range, int doc, long cp_first, long dcp) {
    return PcaSet(range, doc, cp_first, cp_first + dcp);
}

OpusCa *PcaSetWholeDoc(OpusCa *range, int doc) {
    return PcaSet(range, doc, 0, CpMacDocEdit(doc));
}

OpusCa *PcaSetNil(OpusCa *range) {
    return PcaSet(range, kDocNil, 0, 0);
}

OpusCa *PcaPoint(OpusCa *range, int doc, long cp) {
    return PcaSet(range, doc, cp, cp);
}

long DcpCa(const OpusCa *range) {
    return range->cpLim - range->cpFirst;
}

int FInCa(int doc, long cp, const OpusCa *range) {
    return range->doc == doc && range->cpFirst <= cp && cp < range->cpLim;
}
