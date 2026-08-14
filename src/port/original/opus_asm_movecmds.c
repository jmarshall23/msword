#include "opus_x64_compat.h"
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef int (*OPUS_PFN)();

#define mctCmd 1
#define mctEl 2
#define mctSdm 3

#define OPUS_MAX_SIZE(a, b) ((a) > (b) ? (a) : (b))
#define OPUS_ALIGN_UP(value, alignment) \
	((((value) + (alignment) - 1) / (alignment)) * (alignment))
#define OPUS_POINTER_ALIGN sizeof(void *)
#define OPUS_KME_VALUE_SIZE OPUS_MAX_SIZE(sizeof(void *), sizeof(int))
#define OPUS_EXPECTED_SY_SIZE (2 + sizeof(OPUS_PFN) + 2 + 2 + 1)
#define OPUS_EXPECTED_KME_SIZE \
	OPUS_ALIGN_UP(sizeof(unsigned) + OPUS_KME_VALUE_SIZE, OPUS_POINTER_ALIGN)
#define OPUS_EXPECTED_KMP_OFFSET \
	OPUS_ALIGN_UP(sizeof(void *) + sizeof(int) * 3, OPUS_POINTER_ALIGN)

#pragma pack(push, 1)
typedef struct _OPUS_NATIVE_SY
	{
	uint16_t bsyNext;
	union {
		struct {
			OPUS_PFN pfnCmd;
			uint16_t ucc : 4;
			uint16_t fSetUndo : 1;
			uint16_t fRepeatable : 1;
			uint16_t fWParam : 1;
			uint16_t iidstr : 9;
			};
		struct {
			OPUS_PFN pfnEl;
			uint16_t cagdMin : 4;
			uint16_t cagdMax : 4;
			uint16_t elr : 3;
			uint16_t fStmt : 1;
			uint16_t fDyadic : 1;
			uint16_t spare : 3;
			};
		};
	union {
		uint16_t grf;
		};
	char stName[1];
	} OPUS_NATIVE_SY;

typedef struct _OPUS_NATIVE_SYT
	{
	uint16_t bsy;
	uint16_t bsyMax;
	uint16_t mpwbsy[127];
	char grpsy[0];
	} OPUS_NATIVE_SYT;
#pragma pack(pop)

typedef struct _OPUS_NATIVE_KME
	{
	unsigned kc : 12;
	unsigned kt : 3;
	union {
		int w;
		void *pfn;
		void **hst;
		};
	} OPUS_NATIVE_KME;

typedef struct _OPUS_NATIVE_KMP
	{
	struct _OPUS_NATIVE_KMP **hkmpNext;
	union {
		int grpf;
		struct {
			int fStopHere : 1;
			int fModal : 1;
			int fBeep : 1;
			int fTranslate : 1;
			int fAppModal : 1;
			int : 11;
			};
		};
	int ikmeMac;
	int ikmeMax;
	OPUS_NATIVE_KME rgkme[0];
	} OPUS_NATIVE_KMP;

typedef struct _OPUS_NATIVE_MTM
	{
	unsigned imnu : 4;
	unsigned ibst : 9;
	int fSpare : 1;
	int fUndo : 1;
	int fRemove : 1;
	union {
		unsigned bsy;
		int bcm;
		};
	} OPUS_NATIVE_MTM;

typedef struct _OPUS_NATIVE_MUD
	{
	int imtmMac;
	int imtmMax;
	OPUS_NATIVE_MTM rgmtm[0];
	} OPUS_NATIVE_MUD;

typedef struct _OPUS_NATIVE_STTB
	{
	unsigned bMax;
	int ibstMac;
	int ibstMax;
	union {
		struct {
			int cbExtra : 13;
			int fNoShrink : 1;
			int fExternal : 1;
			int fStyleRules : 1;
			};
		int cbExtraOld;
		};
	union {
		int rgbst[2];
		void *hqrgbst;
		};
	} OPUS_NATIVE_STTB;

typedef struct _OpusNativeSymbolInit
	{
	uint16_t bsy;
	uint16_t bsy_next;
	uint16_t grf;
	uint8_t mct;
	const char *function_name;
	uint8_t cch_name;
	const char *name;
	uint16_t bsy_back;
	uint8_t ucc;
	uint8_t set_undo;
	uint8_t repeatable;
	uint8_t w_param;
	uint16_t iidstr;
	uint8_t cagd_min;
	uint8_t cagd_max;
	uint8_t elr;
	uint8_t statement;
	uint8_t dyadic;
	int cabi;
	int ieldi;
	uint8_t arguments[15];
	} OpusNativeSymbolInit;

typedef struct _OpusNativeKeyInit
	{
	unsigned kc;
	unsigned kt;
	int value;
	} OpusNativeKeyInit;

typedef struct _OpusNativeMenuInit
	{
	unsigned imnu;
	unsigned ibst;
	unsigned checked;
	unsigned disabled;
	unsigned remove;
	int bsy;
	} OpusNativeMenuInit;

typedef struct _OpusNativeCommandAddress
	{
	const char *name;
	OPUS_PFN function;
	} OpusNativeCommandAddress;

#include "opuscmddata.inc"
#include "opuscmdtable.h"

typedef char OpusAssertSySize[(sizeof(OPUS_NATIVE_SY) == OPUS_EXPECTED_SY_SIZE) ? 1 : -1];
typedef char OpusAssertSytSize[(sizeof(OPUS_NATIVE_SYT) == 258) ? 1 : -1];
typedef char OpusAssertKmeSize[(sizeof(OPUS_NATIVE_KME) == OPUS_EXPECTED_KME_SIZE) ? 1 : -1];
typedef char OpusAssertKmpOffset[(offsetof(OPUS_NATIVE_KMP, rgkme) == OPUS_EXPECTED_KMP_OFFSET) ? 1 : -1];
typedef char OpusAssertMtmSize[(sizeof(OPUS_NATIVE_MTM) == 8) ? 1 : -1];
typedef char OpusAssertMudSize[(sizeof(OPUS_NATIVE_MUD) == 8) ? 1 : -1];
typedef char OpusAssertSttbSize[(sizeof(OPUS_NATIVE_STTB) == 24) ? 1 : -1];

static int CompareCommandAddressByName(const void *key, const void *entryRaw)
	{
	const OpusNativeCommandAddress *entry =
		(const OpusNativeCommandAddress *)entryRaw;
	return strcmp((const char *)key, entry->name);
	}

static OPUS_PFN ResolveCommandAddress(const char *name)
	{
	const OpusNativeCommandAddress *entry =
		(const OpusNativeCommandAddress *)bsearch(name, kOpusNativeCommandAddresses,
				sizeof(kOpusNativeCommandAddresses) /
					sizeof(kOpusNativeCommandAddresses[0]),
				sizeof(kOpusNativeCommandAddresses[0]),
				CompareCommandAddressByName);
	return entry != NULL ? entry->function : NULL;
	}

static size_t CopySymbolPayload(OPUS_NATIVE_SY *symbol,
		const OpusNativeSymbolInit *init)
	{
	char *payload = symbol->stName;
	size_t variableBytes = init->cch_name;
	*payload++ = (char)init->cch_name;

	if (init->cch_name != 0)
		{
		memcpy(payload, init->name, init->cch_name);
		payload += init->cch_name;
		}
	else
		{
		unsigned back = init->bsy_back;
		memcpy(payload, &back, sizeof(back));
		payload += sizeof(back);
		variableBytes = sizeof(back);
		}

	if (init->mct == mctSdm)
		{
		memcpy(payload, &init->cabi, sizeof(init->cabi));
		payload += sizeof(init->cabi);
		memcpy(payload, &init->ieldi, sizeof(init->ieldi));
		variableBytes += sizeof(init->cabi) + sizeof(init->ieldi);
		}
	else if (init->mct == mctEl && init->cagd_max != 0)
		{
		memcpy(payload, init->arguments, init->cagd_max);
		variableBytes += init->cagd_max;
		}

	return sizeof(OPUS_NATIVE_SY) + variableBytes;
	}

/* Native replacement for asm/movecmds.asm. Microsoft's original MKCMD parser
 * still owns all command, key, menu, and help-table source data.
 */
void MoveCmds(void *sytRaw, void *kmpRaw, void *mudRaw,
		void *sttbRaw, void *rgbstRaw)
	{
	OPUS_NATIVE_SYT *syt = (OPUS_NATIVE_SYT *)sytRaw;
	OPUS_NATIVE_KMP *kmp = (OPUS_NATIVE_KMP *)kmpRaw;
	OPUS_NATIVE_MUD *mud = (OPUS_NATIVE_MUD *)mudRaw;
	OPUS_NATIVE_STTB *sttb = (OPUS_NATIVE_STTB *)sttbRaw;
	int *rgbst = (int *)rgbstRaw;
	size_t bsy = 0;
	int i;

	syt->bsy = (uint16_t)kOpusNativeBsyMac;
	syt->bsyMax = (uint16_t)kOpusNativeBsyMac;
	memcpy(syt->mpwbsy, kOpusNativeHash, sizeof(kOpusNativeHash));

	for (i = 0; i < (int)(sizeof(kOpusNativeSymbols) /
			sizeof(kOpusNativeSymbols[0])); ++i)
		{
		const OpusNativeSymbolInit *init = &kOpusNativeSymbols[i];
		OPUS_NATIVE_SY *symbol = (OPUS_NATIVE_SY *)(syt->grpsy + bsy);
		if (bsy != init->bsy)
			abort();
		memset(symbol, 0, sizeof(*symbol));
		symbol->bsyNext = init->bsy_next;
		symbol->pfnCmd = ResolveCommandAddress(init->function_name);
		symbol->grf = init->grf;

		if (init->mct == mctCmd || init->mct == mctSdm)
			{
			symbol->ucc = init->ucc;
			symbol->fSetUndo = init->set_undo;
			symbol->fRepeatable = init->repeatable;
			symbol->fWParam = init->w_param;
			symbol->iidstr = init->iidstr;
			}
		else if (init->mct == mctEl)
			{
			symbol->cagdMin = init->cagd_min;
			symbol->cagdMax = init->cagd_max;
			symbol->elr = init->elr;
			symbol->fStmt = init->statement;
			symbol->fDyadic = init->dyadic;
			}

		bsy += CopySymbolPayload(symbol, init);
		}
	if (bsy != (size_t)kOpusNativeBsyMac)
		abort();

	kmp->hkmpNext = NULL;
	kmp->grpf = 0;
	kmp->ikmeMac = (int)(sizeof(kOpusNativeKeys) / sizeof(kOpusNativeKeys[0]));
	kmp->ikmeMax = kmp->ikmeMac;
	for (i = 0; i < kmp->ikmeMac; ++i)
		{
		OPUS_NATIVE_KME *key = &kmp->rgkme[i];
		memset(key, 0, sizeof(*key));
		key->kc = kOpusNativeKeys[i].kc;
		key->kt = kOpusNativeKeys[i].kt;
		key->w = kOpusNativeKeys[i].value;
		}

	mud->imtmMac = (int)(sizeof(kOpusNativeMenus) / sizeof(kOpusNativeMenus[0]));
	mud->imtmMax = mud->imtmMac;
	for (i = 0; i < mud->imtmMac; ++i)
		{
		OPUS_NATIVE_MTM *item = &mud->rgmtm[i];
		const OpusNativeMenuInit *init = &kOpusNativeMenus[i];
		memset(item, 0, sizeof(*item));
		item->imnu = init->imnu;
		item->ibst = init->ibst;
		item->fSpare = init->checked;
		item->fUndo = init->disabled;
		item->fRemove = init->remove;
		item->bsy = (unsigned)init->bsy;
		}

	sttb->bMax = (unsigned)kOpusNativeMenuBMax;
	sttb->ibstMac = kOpusNativeMenuIbstMac;
	sttb->ibstMax = kOpusNativeMenuIbstMax;
	sttb->cbExtraOld = 0;
	sttb->cbExtra = kOpusNativeMenuCbExtra;
	sttb->fNoShrink = 0;
	sttb->fExternal = kOpusNativeMenuExternal;
	sttb->fStyleRules = kOpusNativeMenuStyleRules;

	memcpy(rgbst, kOpusNativeMenuOffsets, sizeof(kOpusNativeMenuOffsets));
	memcpy((unsigned char *)rgbst + sizeof(kOpusNativeMenuOffsets),
			kOpusNativeMenuStrings, sizeof(kOpusNativeMenuStrings));
	}
