#include "opus_x64_layout.h"

#include <stddef.h>
#include <string.h>

/*
 * AMD64 translations of the foundational PLC access routines in resn.asm
 * and fetchn.asm. A PLC still uses the original packed header and movable
 * handle; only its segmented pointer calculations become native pointers.
 */

extern struct OpusFastPlcCache {
    void **hplc;
    unsigned char *pchFoo;
    int cbFoo;
    unsigned int bfoo;
} vfpc;

static long *cp_data(void *pplc) {
    return (long *)OpusPlcCpData(pplc);
}

static long adjustment_at(const void *pplc, int index) {
    return index < OpusPlcAdjustIndex(pplc) ? 0L : OpusPlcAdjustment(pplc);
}

static unsigned char *foo_at(void *pplc, int index) {
    unsigned char *cp_data_bytes = (unsigned char *)cp_data(pplc);
    size_t cp_bytes = (size_t)OpusPlcIMax(pplc) * sizeof(long);
    size_t foo_bytes = (size_t)index * (size_t)OpusPlcEntrySize(pplc);
    return cp_data_bytes + cp_bytes + foo_bytes;
}

long *LprgcpForPlc(void *pplc) {
    return cp_data(pplc);
}

long CpPlc(void **hplc, int index) {
    void *pplc = *hplc;
    return cp_data(pplc)[index] + adjustment_at(pplc, index);
}

int PutCpPlc(void **hplc, int index, long cp) {
    void *pplc = *hplc;
    cp_data(pplc)[index] = cp - adjustment_at(pplc, index);
    return 0;
}

int GetPlc(void **hplc, int index, unsigned char *data) {
    void *pplc = *hplc;
    int byte_count = OpusPlcEntrySize(pplc);
    unsigned char *source = foo_at(pplc, index);
    if (byte_count > 0) {
        memmove(data, source, (size_t)byte_count);
    }
    vfpc.hplc = hplc;
    vfpc.pchFoo = data;
    vfpc.cbFoo = byte_count;
    vfpc.bfoo = (unsigned int)(source - (unsigned char *)cp_data(pplc));
    return 0;
}

int PutPlc(void **hplc, int index, const unsigned char *data) {
    void *pplc = *hplc;
    int byte_count = OpusPlcEntrySize(pplc);
    if (byte_count > 0) {
        memmove(foo_at(pplc, index), data, (size_t)byte_count);
    }
    return 0;
}

int PutPlcLastProc(void) {
    unsigned char *base;

    if (vfpc.hplc == NULL || *vfpc.hplc == NULL ||
        vfpc.pchFoo == NULL || vfpc.cbFoo <= 0) {
        return 0;
    }
    base = (unsigned char *)cp_data(*vfpc.hplc);
    memmove(base + vfpc.bfoo, vfpc.pchFoo, (size_t)vfpc.cbFoo);
    return 0;
}

int IInPlc(void **hplc, long cp) {
    void *pplc = *hplc;
    int last = OpusPlcIMac(pplc);
    int low = 0;
    int high = last;
    while (low < high) {
        int middle = low + (high - low + 1) / 2;
        long middle_cp = cp_data(pplc)[middle] + adjustment_at(pplc, middle);
        if (middle_cp <= cp) {
            low = middle;
        } else {
            high = middle - 1;
        }
    }
    if (OpusPlcHasMultipleCps(pplc)) {
        while (low > 0 &&
               cp_data(pplc)[low - 1] + adjustment_at(pplc, low - 1) ==
               cp_data(pplc)[low] + adjustment_at(pplc, low)) {
            --low;
        }
    }
    OpusPlcSetHint(pplc, low);
    return low;
}

int IInPlcCheck(void **hplc, long cp) {
    int last;

    if (hplc == NULL || *hplc == NULL) {
        return -1;
    }
    last = OpusPlcIMac(*hplc);
    if (cp < CpPlc(hplc, 0) || cp >= CpPlc(hplc, last)) {
        return -1;
    }
    return IInPlc(hplc, cp);
}

int IInPlcRef(void **hplc, long cp) {
    int last;
    int index;

    if (hplc == NULL || *hplc == NULL) {
        return -1;
    }
    last = OpusPlcIMac(*hplc);
    if (CpPlc(hplc, 0) >= cp) {
        return 0;
    }
    if (CpPlc(hplc, last) < cp) {
        return -1;
    }
    index = IInPlc(hplc, cp);
    while (index <= last && CpPlc(hplc, index) < cp) {
        ++index;
    }
    return index;
}

int IMacPlc(void **hplc) {
    return hplc == NULL || *hplc == NULL ? 0 : OpusPlcIMac(*hplc);
}

int CompletelyAdjustHplcCps(void **hplc) {
    void *pplc;
    long adjustment;

    if (hplc == NULL || *hplc == NULL) {
        return 0;
    }
    pplc = *hplc;
    adjustment = OpusPlcAdjustment(pplc);
    if (adjustment != 0) {
        long *cps = cp_data(pplc);
        int index;
        for (index = OpusPlcAdjustIndex(pplc);
             index <= OpusPlcIMac(pplc); ++index) {
            cps[index] += adjustment;
        }
    }
    OpusPlcClearAdjustment(pplc);
    return 0;
}

int AddDcpToCps(long *cps, int first, int limit, long adjustment) {
    int index;
    for (index = first; index < limit; ++index) {
        cps[index] += adjustment;
    }
    return 0;
}

int AdjustHplcCpsToLim(void **hplc, int limit) {
    void *pplc;

    if (hplc == NULL || *hplc == NULL ||
        OpusPlcAdjustment(*hplc) == 0) {
        return 0;
    }
    pplc = *hplc;
    AddDcpToCps(cp_data(pplc), OpusPlcAdjustIndex(pplc), limit,
                OpusPlcAdjustment(pplc));
    OpusPlcSetAdjustIndex(pplc, limit);
    if (limit == OpusPlcIMac(pplc) + 1) {
        OpusPlcClearAdjustment(pplc);
    }
    return 0;
}

int BltInPlc(int mode, void **hplc, unsigned int index, int cp_delta,
             int foo_delta, unsigned int count) {
    void *pplc = *hplc;
    unsigned char *cps = (unsigned char *)cp_data(pplc);
    unsigned char *source;
    ptrdiff_t byte_delta;
    size_t byte_count;

    if (mode == 0) {
        source = cps + (size_t)index * sizeof(long);
        byte_delta = (ptrdiff_t)cp_delta * (ptrdiff_t)sizeof(long);
        byte_count = (size_t)count * sizeof(long);
    } else {
        size_t entry_size = (size_t)OpusPlcEntrySize(pplc);
        source = cps + (size_t)OpusPlcIMax(pplc) * sizeof(long) +
                 (size_t)index * entry_size;
        byte_delta = (ptrdiff_t)cp_delta * (ptrdiff_t)sizeof(long) +
                     (ptrdiff_t)foo_delta * (ptrdiff_t)entry_size;
        byte_count = (size_t)count * entry_size;
    }
    if (byte_count != 0) {
        memmove(source + byte_delta, source, byte_count);
    }
    return 0;
}
