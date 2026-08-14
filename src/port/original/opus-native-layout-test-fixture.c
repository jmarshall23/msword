#include "word.h"
DEBUGASSERTSZ
#include "heap.h"
#include "doc.h"
#include "file.h"
#include "disp.h"

#include <string.h>

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
