#include "word.h"
DEBUGASSERTSZ
#include "heap.h"
#include "doc.h"
#include "file.h"
#include "disp.h"

#include "opus-native-layout.h"

enum {
	kOpusDiskCpSize = 4,
	kOpusDiskPcdSize = 8,
	kOpusDiskSedSize = 6
};

static unsigned OpusReadU16(const unsigned char* bytes)
{
	return (unsigned)bytes[0] | ((unsigned)bytes[1] << 8);
}

static unsigned long OpusReadU32(const unsigned char* bytes)
{
	return (unsigned long)bytes[0] |
		((unsigned long)bytes[1] << 8) |
		((unsigned long)bytes[2] << 16) |
		((unsigned long)bytes[3] << 24);
}

static void OpusWriteU16(unsigned char* bytes, unsigned value)
{
	bytes[0] = (unsigned char)(value & 0xffu);
	bytes[1] = (unsigned char)((value >> 8) & 0xffu);
}

static void OpusWriteU32(unsigned char* bytes, unsigned long value)
{
	bytes[0] = (unsigned char)(value & 0xffu);
	bytes[1] = (unsigned char)((value >> 8) & 0xffu);
	bytes[2] = (unsigned char)((value >> 16) & 0xffu);
	bytes[3] = (unsigned char)((value >> 24) & 0xffu);
}

static long OpusReadI32(const unsigned char* bytes)
{
	unsigned long value = OpusReadU32(bytes);
	return (value & 0x80000000ul) ? (long)(value | ~0xfffffffful) :
		(long)value;
}

static int OpusReadI16(const unsigned char* bytes)
{
	unsigned value = OpusReadU16(bytes);
	return (value & 0x8000u) ? (int)(value | ~0xffffu) : (int)value;
}

int OpusDodIsMother(const void* pdod)
{
	return ((const struct DOD*)pdod)->fMother != 0;
}

int OpusDodIsDocument(const void* pdod)
{
	return ((const struct DOD*)pdod)->fDoc != 0;
}

int OpusDodIsTemplate(const void* pdod)
{
	return ((const struct DOD*)pdod)->fDot != 0;
}

int OpusDodMotherDoc(const void* pdod)
{
	return ((const struct DOD*)pdod)->doc;
}

int OpusDodTemplateDoc(const void* pdod)
{
	return ((const struct DOD*)pdod)->docDot;
}

long OpusDodCpMac(const void* pdod)
{
	return ((const struct DOD*)pdod)->cpMac;
}

int OpusWwdMw(const void* pwwd)
{
	return ((const struct WWD*)pwwd)->mw;
}

void OpusWwdCoordinateOffsets(const void* pwwdVoid, int* page_x, int* page_y,
		int* window_x, int* window_y)
{
	const struct WWD* pwwd = (const struct WWD*)pwwdVoid;
	*page_x = pwwd->rcePage.xeLeft;
	*page_y = pwwd->rcePage.yeTop;
	*window_x = pwwd->xwMin;
	*window_y = pwwd->ywMin;
}

void* OpusSttbData(void* psttbVoid)
{
	struct STTB* psttb = (struct STTB*)psttbVoid;
	if (psttb->fExternal)
		return HpOfHq(psttb->hqrgbst);
	return psttb->rgbst;
}

int OpusSttbCount(const void* psttb)
{
	return ((const struct STTB*)psttb)->ibstMac;
}

int OpusSttbUsesStyleRules(const void* psttb)
{
	return ((const struct STTB*)psttb)->fStyleRules != 0;
}

size_t OpusSttbDataSize(void** hsttb)
{
	struct STTB* psttb;
	size_t cbHandle;
	if (hsttb == NULL || *hsttb == NULL)
		return 0;
	psttb = (struct STTB*)*hsttb;
	if (psttb->fExternal)
		return (size_t)CbOfHq(psttb->hqrgbst);
	cbHandle = OpusCbOfH(hsttb);
	return cbHandle > offset(STTB, rgbst)
		? cbHandle - offset(STTB, rgbst) : 0;
}

void* OpusPlcCpData(void* pplcVoid)
{
	struct PLC* pplc = (struct PLC*)pplcVoid;
	if (pplc->fExternal)
		return HpOfHq(pplc->hqplce);
	return pplc->rgcp;
}

int OpusPlcIMac(const void* pplc)
{
	return ((const struct PLC*)pplc)->iMac;
}

int OpusPlcIMax(const void* pplc)
{
	return ((const struct PLC*)pplc)->iMax;
}

int OpusPlcEntrySize(const void* pplc)
{
	return ((const struct PLC*)pplc)->cb;
}

int OpusPlcAdjustIndex(const void* pplc)
{
	return ((const struct PLC*)pplc)->icpAdjust;
}

long OpusPlcAdjustment(const void* pplc)
{
	return ((const struct PLC*)pplc)->dcpAdjust;
}

int OpusPlcHasMultipleCps(const void* pplc)
{
	return ((const struct PLC*)pplc)->fMult != 0;
}

void OpusPlcSetHint(void* pplc, int index)
{
	((struct PLC*)pplc)->icpHint = index;
}

void OpusPlcSetAdjustIndex(void* pplc, int index)
{
	((struct PLC*)pplc)->icpAdjust = index;
}

void OpusPlcClearAdjustment(void* pplcVoid)
{
	struct PLC* pplc = (struct PLC*)pplcVoid;
	pplc->icpAdjust = pplc->iMac + 1;
	pplc->dcpAdjust = cp0;
}

void* OpusPlData(void* pplVoid)
{
	struct PL* ppl = (struct PL*)pplVoid;
	char* rgfoo = ((char*)ppl) + ppl->brgfoo;
	if (ppl->fExternal)
		return HpOfHq(*((HQ*)rgfoo));
	return rgfoo;
}

int OpusPlCount(const void* ppl)
{
	return ((const struct PL*)ppl)->iMac;
}

int OpusPlEntrySize(const void* ppl)
{
	return ((const struct PL*)ppl)->cb;
}

int OpusDiskPlcSize(int entry_count, int runtime_entry_size)
{
	return (entry_count + 1) * kOpusDiskCpSize +
		entry_count * runtime_entry_size;
}

int OpusPackPcdDisk(const void* pcdVoid, void* bytesVoid)
{
	const struct PCD* pcd = (const struct PCD*)pcdVoid;
	unsigned char* bytes = (unsigned char*)bytesVoid;
	unsigned flags = 0;
	if (pcd->fNoParaLast)
		flags |= 0x01u;
	if (pcd->fPaphNil)
		flags |= 0x02u;
#ifdef DEBUG
	if (pcd->fNoParaLastValid)
		flags |= 0x80u;
#endif
	bytes[0] = (unsigned char)flags;
	bytes[1] = pcd->fn;
	OpusWriteU32(bytes + 2, (unsigned long)(int32_t)pcd->fc);
	OpusWriteU16(bytes + 6, (unsigned)(uint16_t)pcd->prm);
	return kOpusDiskPcdSize;
}

int OpusUnpackPcdDisk(void* pcdVoid, const void* bytesVoid)
{
	struct PCD* pcd = (struct PCD*)pcdVoid;
	const unsigned char* bytes = (const unsigned char*)bytesVoid;
	memset(pcd, 0, sizeof(*pcd));
	pcd->fNoParaLast = (bytes[0] & 0x01u) != 0;
	pcd->fPaphNil = (bytes[0] & 0x02u) != 0;
#ifdef DEBUG
	pcd->fNoParaLastValid = (bytes[0] & 0x80u) != 0;
#endif
	pcd->fn = bytes[1];
	pcd->fc = (FC)OpusReadI32(bytes + 2);
	pcd->prm = OpusReadI16(bytes + 6);
	return kOpusDiskPcdSize;
}

int OpusPackSedDisk(const void* sedVoid, void* bytesVoid)
{
	const struct SED* sed = (const struct SED*)sedVoid;
	unsigned char* bytes = (unsigned char*)bytesVoid;
	unsigned flags = ((unsigned)sed->fn & 0x3fffu) << 2;
	if (sed->fSpare)
		flags |= 0x01u;
	if (sed->fUnk)
		flags |= 0x02u;
	OpusWriteU16(bytes, flags);
	OpusWriteU32(bytes + 2, (unsigned long)(int32_t)sed->fcSepx);
	return kOpusDiskSedSize;
}

int OpusUnpackSedDisk(void* sedVoid, const void* bytesVoid)
{
	struct SED* sed = (struct SED*)sedVoid;
	const unsigned char* bytes = (const unsigned char*)bytesVoid;
	unsigned flags = OpusReadU16(bytes);
	int fn = (int)((flags >> 2) & 0x3fffu);
	memset(sed, 0, sizeof(*sed));
	if (fn & 0x2000)
		fn |= ~0x3fff;
	sed->fSpare = (flags & 0x01u) != 0;
	sed->fUnk = (flags & 0x02u) != 0;
	sed->fn = fn;
	sed->fcSepx = (FC)OpusReadI32(bytes + 2);
	return kOpusDiskSedSize;
}

int OpusPackPlcDisk(const void* pplcVoid, int entry_count,
		long cp_last_override, void* bytesVoid, int byte_count)
{
	const struct PLC* pplc = (const struct PLC*)pplcVoid;
	unsigned char* bytes = (unsigned char*)bytesVoid;
	const CP* rgcp = (const CP*)OpusPlcCpData((void*)pplcVoid);
	const char* rgfoo = (const char*)rgcp + (size_t)pplc->iMax * sizeof(CP);
	int expected = OpusDiskPlcSize(entry_count, pplc->cb);
	int i;
	if (byte_count < expected)
		return 0;
	for (i = 0; i <= entry_count; ++i)
		{
		long cp = (i == entry_count && cp_last_override != cpNil) ?
			cp_last_override : rgcp[i];
		OpusWriteU32(bytes + i * kOpusDiskCpSize,
			(unsigned long)(int32_t)cp);
		}
	memcpy(bytes + (entry_count + 1) * kOpusDiskCpSize, rgfoo,
		(size_t)entry_count * pplc->cb);
	return expected;
}

int OpusUnpackPlcDisk(void* pplcVoid, int entry_count, const void* bytesVoid,
		int byte_count)
{
	struct PLC* pplc = (struct PLC*)pplcVoid;
	const unsigned char* bytes = (const unsigned char*)bytesVoid;
	CP* rgcp = (CP*)OpusPlcCpData(pplcVoid);
	char* rgfoo = (char*)rgcp + (size_t)pplc->iMax * sizeof(CP);
	int expected = OpusDiskPlcSize(entry_count, pplc->cb);
	int i;
	if (byte_count < expected)
		return 0;
	for (i = 0; i <= entry_count; ++i)
		rgcp[i] = (CP)OpusReadI32(bytes + i * kOpusDiskCpSize);
	memcpy(rgfoo, bytes + (entry_count + 1) * kOpusDiskCpSize,
		(size_t)entry_count * pplc->cb);
	return expected;
}
