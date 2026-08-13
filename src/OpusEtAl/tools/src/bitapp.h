#define FALSE	0
#define TRUE	1
#define NULL	0

#define FIG_CURSOR	1
#define FIG_BITMAP	2
#define FIG_ICON	3

#define EVEN_MASK	0xaaaa
#define ODD_MASK	0x5555

#define WRONGPARAM	1
#define BADFILE		2
#define BADFIGURE	3
#define BADEOF		4
#define BADSWITCH	5

#ifdef OPUS_X64_TOOL
#define FAR
#else
#define FAR	far
#endif
#define NEAR
#define LONG	long
#define VOID	void

typedef unsigned char	BYTE;
typedef unsigned short	WORD;
#if defined(OPUS_X64_TOOL) && !defined(_MSC_VER)
#include <stdint.h>
typedef uint32_t DWORD;
#else
typedef unsigned long  DWORD;
#endif
typedef int	  BOOL;
typedef char	 *PSTR;
typedef char NEAR *NPSTR;
typedef char FAR *LPSTR;
typedef int  FAR *LPINT;

#ifdef OPUS_X64_TOOL
#pragma pack(push, 2)
#endif

typedef struct tagBITMAP {
	short      bmType;
	short      bmWidth;
	short      bmHeight;
	short      bmWidthBytes;
	BYTE       bmPlanes;
	BYTE       bmBitsPixel;
#ifdef OPUS_X64_TOOL
	DWORD      bmBits;       /* serialized Win16 far pointer, always ignored */
#else
	LPSTR      bmBits;
#endif
} BITMAP;

#if defined(OPUS_X64_TOOL)
typedef char OPUS_X64_BITMAP_MUST_MATCH_WIN16[(sizeof(BITMAP) == 14) ? 1 : -1];
#endif

typedef struct {
	short      xHotspot;
	short      yHotspot;
	short      cx;
	short      cy;
	short      WidthBytes;
	short      clr;
} RCI;

#ifdef OPUS_X64_TOOL
#pragma pack(pop)
#endif
