#include "word.h"
DEBUGASSERTSZ
#include "heap.h"
#include "doc.h"
#include "file.h"
#include "disp.h"

#include <string.h>

typedef char OpusAssertCurrentFibSize[
	((sizeof(CP) == 8 && sizeof(struct FIB) == 768) ||
	 (sizeof(CP) == 4 && sizeof(struct FIB) == 420)) ? 1 : -1];
typedef char OpusAssertCurrentFileHeaderSize[
	((sizeof(CP) == 8 && cbFileHeader == 512) ||
	 (sizeof(CP) == 4 && cbFileHeader == 512)) ? 1 : -1];
typedef char OpusAssertCurrentPlcBaseSize[
	((sizeof(CP) == 8 && sizeof(void *) == 8 && cbPLCBase == 32) ||
	 (sizeof(CP) == 4 && sizeof(void *) == 8 && cbPLCBase == 36) ||
	 (sizeof(CP) == 4 && sizeof(void *) == 4 && cbPLCBase == 28)) ? 1 : -1];
typedef char OpusAssertCurrentPcdSize[
	((sizeof(CP) == 8 && sizeof(struct PCD) == 24) ||
	 (sizeof(CP) == 4 && sizeof(struct PCD) == 12)) ? 1 : -1];
typedef char OpusAssertCurrentSedSize[
	((sizeof(CP) == 8 && sizeof(struct SED) == 16) ||
	 (sizeof(CP) == 4 && sizeof(struct SED) == 8)) ? 1 : -1];
typedef char OpusAssertCurrentKmeSize[
	((sizeof(void *) == 8 && sizeof(KME) == 16) ||
	 (sizeof(void *) == 4 && sizeof(KME) == 8)) ? 1 : -1];
typedef char OpusAssertCurrentKmeWords[
	((sizeof(void *) == 8 && cwKME == 4) ||
	 (sizeof(void *) == 4 && cwKME == 2)) ? 1 : -1];

/* Real original-layout objects used by the C++ runtime test. */
static struct DOD rgdodTest[4];
static struct WWD rgwwdTest[2];

void* OpusTestDod(int slot, int dk, int docMother, int docDot)
{
	struct DOD* pdod;
	if (slot < 0 || slot >= 4)
		return 0;
	pdod = &rgdodTest[slot];
	memset(pdod, 0, sizeof(*pdod));
	pdod->dk = dk;
	pdod->doc = docMother;
	if (pdod->fDoc)
		pdod->docDot = docDot;
	return pdod;
}

void* OpusTestWwd(int slot, int mw)
{
	struct WWD* pwwd;
	if (slot < 0 || slot >= 2)
		return 0;
	pwwd = &rgwwdTest[slot];
	memset(pwwd, 0, sizeof(*pwwd));
	pwwd->mw = mw;
	return pwwd;
}

void OpusTestSetDodCpMac(void* pdod, long cpMac)
{
	((struct DOD*)pdod)->cpMac = cpMac;
}
