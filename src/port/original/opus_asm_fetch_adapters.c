/*
 * AMD64 entry adapters for the fetch-family assembly modules.  Microsoft
 * retained equivalent C bodies in wordtech/fetch.c; that file is compiled
 * with DEBUGORNOTWIN for this port, and these wrappers preserve every native
 * symbol spelling used by the original source graph.
 */

#include <stddef.h>

int C_FetchCp(int doc, long cp, int fcm);
int C_CachePara(int doc, long cp);
int C_PnFromPlcbteFc(void** hplcbte, long fc);
int C_BFromFc(void* hpfkp, long fc, long* pfc_first, long* pfc_lim,
              int* pifc, int word3);
int C_MapStc(void* pdod, int stc, void* pchp, void* ppap);
unsigned char* C_HpchFromFc(int fn, long fc);
unsigned char* C_HpchGetPn(int fn, int pn);
int C_IbpCacheFilePage(int fn, int pn);
int C_DoPrmSgc(int prm, unsigned char* properties, int sgc);
int C_ApplyPrlSgc(unsigned char* prl, int count, unsigned char* properties,
                  int sgc, int old_version);
int C_ApplyChpxTransforms(void* chp);
int C_FInTableDocCp(int doc, long cp);
extern unsigned char* vhpchFetch;

int N_FetchCp(int doc, long cp, int fcm) {
    C_FetchCp(doc, cp, fcm);
    return 0;
}

int N_FETCHCP(int doc, long cp, int fcm) {
    return N_FetchCp(doc, cp, fcm);
}

int N_CachePara(int doc, long cp) {
    C_CachePara(doc, cp);
    return 0;
}

int N_PnFromPlcbteFc(void** hplcbte, long fc) {
    return C_PnFromPlcbteFc(hplcbte, fc);
}

int N_BFromFc(void* hpfkp, long fc, long* pfc_first, long* pfc_lim,
              int* pifc, int word3) {
    return C_BFromFc(hpfkp, fc, pfc_first, pfc_lim, pifc, word3);
}

int N_MapStc(void* pdod, int stc, void* pchp, void* ppap) {
    C_MapStc(pdod, stc, pchp, ppap);
    return 0;
}

unsigned char* N_HpchFromFc(int fn, long fc) {
    return C_HpchFromFc(fn, fc);
}

unsigned char* N_HpchGetPn(int fn, int pn) {
    return C_HpchGetPn(fn, pn);
}

int N_IbpCacheFilePage(int fn, int pn) {
    return C_IbpCacheFilePage(fn, pn);
}

int N_DoPrmSgc(int prm, unsigned char* properties, int sgc) {
    C_DoPrmSgc(prm, properties, sgc);
    return 0;
}

int N_ApplyPrlSgc(unsigned char* prl, int count, unsigned char* properties,
                  int sgc, int old_version) {
    C_ApplyPrlSgc(prl, count, properties, sgc, old_version);
    return 0;
}

int N_ApplyChpxTransforms(void* chp) {
    C_ApplyChpxTransforms(chp);
    return 0;
}

int N_FInTableDocCp(int doc, long cp) {
    return C_FInTableDocCp(doc, cp);
}

int FetchCpAndPara(int doc, long cp, int fcm) {
    N_CachePara(doc, cp);
    return N_FetchCp(doc, cp, fcm);
}

typedef struct OpusFetchCa {
    long cpFirst;
    long cpLim;
    int doc;
} OpusFetchCa;

int FetchCpAndParaCa(const OpusFetchCa* range, int fcm) {
    return FetchCpAndPara(range->doc, range->cpFirst, fcm);
}

int ChFetch(int doc, long cp, int fcm) {
    if (fcm != 0x8000) {
        N_CachePara(doc, cp);
    }
    N_FetchCp(doc, cp, fcm);
    return vhpchFetch == NULL ? 0 : *vhpchFetch;
}
