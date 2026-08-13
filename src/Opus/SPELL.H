#define spcDone		98
#define spcAbort	99

#define ilngMax    17
#define cbszLangMax 20
#define cbReplaceWordMax	255
#define cbWordMax   30          /* Maximum size of word strings */
#define cbWordMin   2           /* Minimum size of word strings */


#define udcUpdate      	0	/* User Dictionary Code */
#define udcUser		1
#define udcMax		2

struct SPL	/* Spell library structure */
	{
	CHAR stPath[ichMaxFile];
	HANDLE hLib;
	struct GHD ghdWord;		/* structures for passing strings to/from DLL */
	struct GHD ghdReplaceWord;
	struct GHD ghdT;
	HSTACK hstack;
	HSTACK (FAR PASCAL *lpfnHStackSpell)();
	PFN_CRMGR lpfnFSpellInit;
	PFN_CRMGR lpfnFSpellCleanUp;
	PFN_CRMGR lpfnLLookUpWord;
	PFN_CRMGR lpfnGetAlternates;
	PFN_CRMGR lpfnFSetDictAux;
	PFN_CRMGR lpfnFAddWordToDictAux;
	PFN_CRMGR lpfnFDeleteWordFromDictAux;
	PFN_CRMGR lpfnFAddWordToCacheAux;
	PFN_CRMGR lpfnGhSpellGetrgchCase;
	PFN_CRMGR lpfnResetRepeatWord;
	};

#define szUpdateDictDef	"STDUSER.DIC"

struct SPV	/* Spell pseudo-global variables (on frame) */
	{
	int fSeeSel;
	BOOL fSplCancel;
	BOOL fInScanDoc;
	BOOL fStartScan;
	int LookupError;
	BOOL fCancelTerminate;
	int mpudcfRO [udcMax];
	int  mpilngiLB [ilngMax];	/* maps language code to index in list box */
	int  mpilngfDee [ilngMax];
	BOOL fNoSugg;
	int StatLdADictFn;
	HDLG hDlgSpeller;
	CHAR szT[cbWordMax+2];
	struct FINDFILE FindFile;
	CHAR szUpdateDict[sizeof(szUpdateDictDef)];
	};

#define cbchBuffMax 256
#define ccrBuffMax 20

struct SCD	/* Scan Doc structure */	
	{
	CP cpScanStart;	    	/* defines overall scan for FScanDoc */
	CP cpScanLim;		/* defines overall scan for FScanDoc */
	CP cpTestWord;			/* returned by FGetNextWordFromDoc */
	CP cpLimWord;			/* returned by FGetNextWordFromDoc */
	CHAR szTestWord[cbWordMax+2];	/* returned by FGetNextWordFromDoc */
	int fPrompt;		/* true if prompt line is used during scan */
	int fDirtyDoc;		/* set iff doc is dirtied during spell */
	CP cpScan;			/* cp scan point for FGetNextWordFromDoc */
	CP cpScanLocalMac;		/* Limit cp for FGetNextWordFromDoc */
	BOOL fCheckWrap;   		/* whether to check wraparound at doc end */
	CHAR rgchCase[256];		/* speller array: maps ch to char type bits */
	};



NATIVE SpcScanDocNextWord();
NATIVE FGetNextWordFromDoc();
