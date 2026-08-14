/*
 * AMD64 entry adapters for layout2n.asm routines whose authoritative C
 * implementations are retained in wordtech/layout2.c.  The C file is built
 * with DEBUGORNOTWIN so Microsoft's C fallbacks are present; these adapters
 * replace the assembly-visible and p-code-visible entry names.
 */

typedef struct LBS LBS;
typedef struct PLLR PLLR;
typedef struct LR LR;

int C_CopyHdtLrs(int ihdt, LBS* plbs, int yl, LBS* plbs_abs);
int C_CopyLrs(PLLR** hpllr_from, LBS* plbs_to, int ilr_mac, int yl,
              int copy_ignore, int xl);
int C_ReplaceInPllr(PLLR** hpllr, int ilr, LR* plr);
int C_CacheParaL(int doc, long cp, int outline);
int C_CacheSectL(int doc, long cp, int outline);
int C_PushLbs(LBS* from, LBS* to);
int C_PopLbs(LBS* id, LBS* to);
int C_CopyLbs(LBS* from, LBS* to);

int N_CopyHdtLrs(int ihdt, LBS* plbs, int yl, LBS* plbs_abs) {
    C_CopyHdtLrs(ihdt, plbs, yl, plbs_abs);
    return 0;
}

int CopyHdtLrs(int ihdt, LBS* plbs, int yl, LBS* plbs_abs) {
    return N_CopyHdtLrs(ihdt, plbs, yl, plbs_abs);
}

int N_CopyLrs(PLLR** hpllr_from, LBS* plbs_to, int ilr_mac, int yl,
              int copy_ignore, int xl) {
    C_CopyLrs(hpllr_from, plbs_to, ilr_mac, yl, copy_ignore, xl);
    return 0;
}

int CopyLrs(PLLR** hpllr_from, LBS* plbs_to, int ilr_mac, int yl,
            int copy_ignore, int xl) {
    return N_CopyLrs(hpllr_from, plbs_to, ilr_mac, yl, copy_ignore, xl);
}

int N_ReplaceInPllr(PLLR** hpllr, int ilr, LR* plr) {
    C_ReplaceInPllr(hpllr, ilr, plr);
    return 0;
}

int N_ReplaceinPllr(PLLR** hpllr, int ilr, LR* plr) {
    return N_ReplaceInPllr(hpllr, ilr, plr);
}

int ReplaceInPllr(PLLR** hpllr, int ilr, LR* plr) {
    return N_ReplaceInPllr(hpllr, ilr, plr);
}

int N_CacheParaL(int doc, long cp, int outline) {
    C_CacheParaL(doc, cp, outline);
    return 0;
}

int CacheParaL(int doc, long cp, int outline) {
    return N_CacheParaL(doc, cp, outline);
}

int N_CacheSectL(int doc, long cp, int outline) {
    C_CacheSectL(doc, cp, outline);
    return 0;
}

int CacheSectL(int doc, long cp, int outline) {
    return N_CacheSectL(doc, cp, outline);
}

int N_PushLbs(LBS* from, LBS* to) {
    C_PushLbs(from, to);
    return 0;
}

int PushLbs(LBS* from, LBS* to) {
    return N_PushLbs(from, to);
}

int N_PopLbs(LBS* id, LBS* to) {
    C_PopLbs(id, to);
    return 0;
}

int PopLbs(LBS* id, LBS* to) {
    return N_PopLbs(id, to);
}

int N_CopyLbs(LBS* from, LBS* to) {
    C_CopyLbs(from, to);
    return 0;
}

int CopyLbs(LBS* from, LBS* to) {
    return N_CopyLbs(from, to);
}
