#pragma once

#include <stddef.h>

#ifdef __cplusplus
static_assert(sizeof(void*) == 8 || sizeof(void*) == 4,
              "Opus handle layout requires 32-bit or 64-bit pointers");
#else
_Static_assert(sizeof(void*) == 8 || sizeof(void*) == 4,
               "Opus handle layout requires 32-bit or 64-bit pointers");
#endif

#ifdef __cplusplus
extern "C" {
#endif

void** OpusHAllocateCb(size_t byte_count);
void OpusFreeH(void** handle);
void OpusFreePh(void*** handle_address);
int OpusFChngSizeHCb(void** handle, size_t byte_count, int allow_shrink);
int OpusFChngSizePhqLcb(void*** handle_address, size_t byte_count);
size_t OpusCbOfH(void** handle);
size_t OpusHeapBytesUsed(void);
void* OpusDerefH(void** handle);
void* OpusHpAlloc(size_t byte_count);
void OpusFreeHp(void* pointer);
unsigned HcCompactH(void** handle);
void** HExpandHc(unsigned compact_handle);
int OpusPrcTokenFromHandle(void** handle);
void** OpusPrcHandleFromToken(int token);

#ifdef __cplusplus
}
#endif

/* Opus movable handles remain pointer-to-pointer values. The handle itself is
 * stable while its native allocation may move during resize. */
#define HAllocateCb(cb) OpusHAllocateCb((size_t)(cb))
#define HAllocateCw(cw) OpusHAllocateCb((size_t)(cw) * sizeof(int))
#define FreeH(h) OpusFreeH((void**)(h))
#define FreePh(ph) OpusFreePh((void***)(ph))
#define CbOfH(h) ((uns)OpusCbOfH((void**)(h)))
#define CwOfPfgr(h) ((CbOfH(h) + 3u) >> 1u)
#define FChngSizeHCb(h, cb, shrink) \
    OpusFChngSizeHCb((void**)(h), (size_t)(cb), (shrink))
#define FChngSizeHCw(h, cw, shrink) \
    OpusFChngSizeHCb((void**)(h), (size_t)(cw) * sizeof(int), (shrink))

#define HqAllocLcb(cb) ((HQ)OpusHAllocateCb((size_t)(cb)))
#define FreeHq(hq) OpusFreeH((void**)(hq))
#define FChngSizePhqLcb(phq, cb) \
    OpusFChngSizePhqLcb((void***)(phq), (size_t)(cb))
#define HpOfHq(hq) ((CHAR HUGE*)OpusDerefH((void**)(hq)))
#define LpFromHq(hq) HpOfHq(hq)
#define LpLockHq(hq) HpOfHq(hq)
#define UnlockHq(hq) ((void)0)
#define CbOfHq(hq) ((uns)OpusCbOfH((void**)(hq)))

#define HpAlloc(cb) ((CHAR HUGE*)OpusHpAlloc((size_t)(cb)))
#define FreeHp(hp) OpusFreeHp((void*)(hp))
#define LpFromHp(hp) ((void*)(hp))
#define LpLockHp(hp) ((void*)(hp))
#define UnlockHp(hp) ((void)0)
#define HpFromPch(pch) ((HP)(pch))

#define FreezeHp() ((void)0)
#define MeltHp() ((void)0)
#define MeltHpToZero() ((void)0)
#define CkHeap() ((void)0)
#define ShakeHeap() ((void)0)

#define cbMaxSbDds ((size_t)-1)
#define lcbMaxFarObj ((long)0x7fffffffL)
/* Algorithms that choose a graphics swath still need the historical target
 * chunk size even though native allocations are no longer segmented. */
#define cbLmemHeap ((size_t)65536u)

struct SBHI
    {
    SB sb;
    uns fHasFixed : 1;
    uns fHasHeap : 1;
    uns ch : 12;
    uns : 2;
    };

struct PLSBHI
    {
    int isbhiMac;
    int isbhiMax;
    int cb;
    int brgsbhi;
    struct SBHI rgsbhi[1];
    };

struct PLF
    {
    int isbMac;
    int isbMax;
    int cbSb;
    int brgsb;
    int fExternal;
    int iMac;
    int iMax;
    int cbFoo;
    int cFooChunk;
    SB rgsb[1];
    };

#define cbSBHI (sizeof(struct SBHI))
#define cbPLFBase offset(PLF, rgsb)

CHAR HUGE* HpInPlf();
struct PLF** HplfInit();
SB SbAllocEmmCb();

/* The Win64 stack is committed/grown by the OS and cannot be measured using
 * the old 16-bit SS offset arithmetic. */
#define ReturnOnNoStack(cbNeeded, wRet, fReport) ((void)0)
