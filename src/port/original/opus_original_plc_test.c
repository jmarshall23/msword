#include "word.h"
DEBUGASSERTSZ
#include "heap.h"
#include "doc.h"
#include "file.h"
#include "disp.h"
#include "debug.h"
#include "error.h"
#include "inter.h"
#include "opus-native-layout.h"

#include <string.h>

struct FPC vfpc;
struct MERR vmerr;
struct FTI { int unused; };
struct FTI vfti;
int vfInCommit = 0;
int dypCS = -1;
CHAR stEmpty[] = {0};
struct ITR vitr;

long LPushMacroArgs(void *procedure, const int *arguments, int count)
{
	(void)procedure;
	(void)arguments;
	(void)count;
	return 0;
}

struct DR* N_PdrFetch(struct PLDR** hpldr, int idr, struct DRF* pdrf)
{
	(void)hpldr;
	(void)idr;
	(void)pdrf;
	return 0;
}

void N_FreePdrf(struct DRF* pdrf)
{
	(void)pdrf;
}

int ErrorEidProc(void)
{
	return 0;
}

int SetErrorMatProc(void)
{
	return 0;
}

struct PLC** HplcInit(int cbPlc, unsigned ifooMaxInit, CP cpLim,
	int fExtRgFoo);
int FreeHplc(struct PLC** hplc);
int PutPlc(struct PLC** hplc, int index, const void* data);
int GetPlc(struct PLC** hplc, int index, void* data);
int PutCpPlc(struct PLC** hplc, int index, CP cp);
CP CpPlc(struct PLC** hplc, int index);
int IInPlc(struct PLC** hplc, CP cp);
int IInPlcCheck(struct PLC** hplc, CP cp);
int IInPlcRef(struct PLC** hplc, CP cp);
int PutPlcLastProc(void);

struct TESTFOO
	{
	int first;
	int second;
	};

static int FExercisePlc(int fExternal)
{
	struct TESTFOO foo0 = {11, 22};
	struct TESTFOO foo1 = {33, 44};
	struct TESTFOO copy;
	struct PLC** hplc = HplcInit(sizeof(struct TESTFOO), 3, 100,
		fExternal);
	int result = 0;

	if (hplc == 0)
		return 1;
	if (CpPlc(hplc, 0) != 100)
		result = 2;
	else
		{
		(*hplc)->iMac = 2;
		(*hplc)->icpAdjust = 3;
		PutCpPlc(hplc, 0, 0);
		PutCpPlc(hplc, 1, 10);
		PutCpPlc(hplc, 2, 100);
		PutPlc(hplc, 0, &foo0);
		PutPlc(hplc, 1, &foo1);
		memset(&copy, 0, sizeof(copy));
		GetPlc(hplc, 1, &copy);
		if (copy.first != 33 || copy.second != 44)
			result = 3;
		else if (CpPlc(hplc, 0) != 0 || CpPlc(hplc, 1) != 10 ||
			CpPlc(hplc, 2) != 100)
			result = 4;
		else if (IInPlc(hplc, 7) != 0 || IInPlc(hplc, 10) != 1 ||
			IInPlc(hplc, 99) != 1 || IInPlc(hplc, 100) != 2)
			result = 5;
		else if (IInPlcCheck(hplc, -1) != -1 ||
			IInPlcCheck(hplc, 99) != 1 || IInPlcCheck(hplc, 100) != -1 ||
			IInPlcRef(hplc, 0) != 0 || IInPlcRef(hplc, 7) != 1 ||
			IInPlcRef(hplc, 101) != -1)
			result = 7;
		else
			{
			copy.first = 55;
			copy.second = 66;
			PutPlcLastProc();
			memset(&copy, 0, sizeof(copy));
			GetPlc(hplc, 1, &copy);
			if (copy.first != 55 || copy.second != 66)
				result = 6;
			}
		}
	FreeHplc(hplc);
	return result;
}

static int FExerciseDiskPacking(void)
{
	struct PCD pcd;
	struct PCD pcdCopy;
	struct SED sed;
	struct SED sedCopy;
	unsigned char rgch[32];
	struct TESTFOO foo0 = {11, 22};
	struct TESTFOO foo1 = {33, 44};
	struct PLC** hplc;
	int result = 0;

	memset(&pcd, 0, sizeof(pcd));
	pcd.fNoParaLast = fTrue;
	pcd.fPaphNil = fTrue;
	pcd.fn = 7;
	pcd.fc = (FC)0x12345678L;
	pcd.prm = -2;
	OpusPackPcdDisk(&pcd, rgch);
	if (rgch[0] != 3 || rgch[1] != 7 || rgch[2] != 0x78 ||
			rgch[3] != 0x56 || rgch[4] != 0x34 || rgch[5] != 0x12 ||
			rgch[6] != 0xfe || rgch[7] != 0xff)
		return 21;
	OpusUnpackPcdDisk(&pcdCopy, rgch);
	if (!pcdCopy.fNoParaLast || !pcdCopy.fPaphNil || pcdCopy.fn != 7 ||
			pcdCopy.fc != (FC)0x12345678L || pcdCopy.prm != -2)
		return 22;

	memset(&sed, 0, sizeof(sed));
	sed.fSpare = fTrue;
	sed.fn = -1;
	sed.fcSepx = fcNil;
	OpusPackSedDisk(&sed, rgch);
	if (rgch[0] != 0xfd || rgch[1] != 0xff || rgch[2] != 0xff ||
			rgch[3] != 0xff || rgch[4] != 0xff || rgch[5] != 0xff)
		return 23;
	OpusUnpackSedDisk(&sedCopy, rgch);
	if (!sedCopy.fSpare || sedCopy.fUnk || sedCopy.fn != -1 ||
			sedCopy.fcSepx != fcNil)
		return 24;

	hplc = HplcInit(sizeof(struct TESTFOO), 3, 100, fTrue);
	if (hplc == 0)
		return 25;
	(*hplc)->iMac = 2;
	(*hplc)->icpAdjust = 3;
	PutCpPlc(hplc, 0, 0);
	PutCpPlc(hplc, 1, 10);
	PutCpPlc(hplc, 2, 100);
	PutPlc(hplc, 0, &foo0);
	PutPlc(hplc, 1, &foo1);
	if (OpusDiskPlcSize(2, sizeof(struct TESTFOO)) != 28)
		result = 26;
	else if (!OpusPackPlcDisk(*hplc, 2, 90, rgch, sizeof(rgch)))
		result = 27;
	else if (rgch[0] != 0 || rgch[4] != 10 || rgch[8] != 90 ||
			rgch[12] != 11 || rgch[16] != 22 || rgch[20] != 33 ||
			rgch[24] != 44)
		result = 28;
	FreeHplc(hplc);
	return result;
}

int main(void)
{
	int result = FExercisePlc(fFalse);
	if (result != 0)
		return result;
	result = FExercisePlc(fTrue);
	if (result != 0)
		return result + 10;
	return FExerciseDiskPacking();
}
