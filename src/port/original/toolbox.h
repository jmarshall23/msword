#pragma once

/*
 * The original toolbox.h was generated from Opus/resource/toolbox.txt and
 * redeclared the complete Win16 API plus Opus's assembly thunks. Modern
 * Windows declarations come from opus-native-compat.h. Custom Opus services are
 * introduced here as their owning modules are migrated.
 */

struct DR;
struct DRF;
struct DOD;
struct FCB;
struct MWD;
struct PLDR;
struct PLC;
struct PT;
struct WWD;

struct DOD* N_PdodDoc(int doc);
struct FCB* N_PfcbFn(int fn);
struct MWD* N_PmwdWw(int ww);
struct WWD* N_PwwdWw(int ww);
struct WWD** N_HwwdWw(int ww);
struct MWD* N_PmwdMw(int mw);

struct DR* N_PdrFetch(struct PLDR** hpldr, int idr, struct DRF* pdrf);
struct DR* N_PdrFetchAndFree(struct PLDR** hpldr, int idr,
                             struct DRF* pdrf);
struct DR* N_PdrFreeAndFetch(struct PLDR** hpldr, int idr,
                             struct DRF* pdrf);
void N_FreePdrf(struct DRF* pdrf);
struct PT N_PtOrigin(struct PLDR** hpldr, int idr);
BOOL N_FInsertInPlc(struct PLC** hplc, int index, long cp, char* data);
void AddDcbToLprgbst(int* offsets, int count, int delta, int threshold);
