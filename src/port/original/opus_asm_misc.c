#include "opus-native-compat.h"
#include "opus-native-heap.h"

#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

struct NativePlaybackEvent {
    unsigned int message;
    unsigned int virtual_key;
};

struct NativePlaybackQueue {
    int current;
    int count;
    struct NativePlaybackEvent events[1];
};

int N_WCompSzSrt(char* first, char* second, int case_sensitive);
extern HANDLE* lphevtHead;
extern HANDLE* lphrgbKeyState;

#ifdef index
#undef index
#endif
char* index(const char* text, const int character) {
    const char* cursor;
    const unsigned char wanted = (unsigned char)character;
    if (text == NULL) {
        return NULL;
    }
    cursor = text;
    for (;;) {
        if ((unsigned char)*cursor == wanted) {
            return (char*)cursor;
        }
        if (*cursor++ == '\0') {
            return NULL;
        }
    }
}

int LbcCmpLbox(unsigned int unused, unsigned char** first,
               unsigned char** second) {
    int comparison;
    (void)unused;
    if (first == NULL || second == NULL || *first == NULL ||
        *second == NULL) {
        return 0;
    }
    comparison = N_WCompSzSrt((char*)(*first + 1), (char*)(*second + 1), 0);
    if (comparison == 0) return 0;   /* lbcEq */
    if (comparison > 0) return 3;    /* lbcGt */
    if (comparison == -2) return 2;  /* lbcLt */
    return 1;                        /* lbcPrefix */
}

long LPushMacroArgs(void* procedure, const int* arguments,
                    const int argument_count) {
    if (procedure == NULL || argument_count < 0 || argument_count > 16 ||
        (argument_count != 0 && arguments == NULL)) {
        return 0;
    }
    switch (argument_count) {
    case 0: {
        typedef long(__cdecl *Proc)(void);
        return ((Proc)procedure)();
    }
    case 1: {
        typedef long(__cdecl *Proc)(int);
        return ((Proc)procedure)(arguments[0]);
    }
    case 2: {
        typedef long(__cdecl *Proc)(int, int);
        return ((Proc)procedure)(arguments[0], arguments[1]);
    }
    case 3: {
        typedef long(__cdecl *Proc)(int, int, int);
        return ((Proc)procedure)(arguments[0], arguments[1], arguments[2]);
    }
    case 4: {
        typedef long(__cdecl *Proc)(int, int, int, int);
        return ((Proc)procedure)(arguments[0], arguments[1], arguments[2],
                                 arguments[3]);
    }
    case 5: {
        typedef long(__cdecl *Proc)(int, int, int, int, int);
        return ((Proc)procedure)(arguments[0], arguments[1], arguments[2],
                                 arguments[3], arguments[4]);
    }
    case 6: {
        typedef long(__cdecl *Proc)(int, int, int, int, int, int);
        return ((Proc)procedure)(arguments[0], arguments[1], arguments[2],
                                 arguments[3], arguments[4], arguments[5]);
    }
    case 7: {
        typedef long(__cdecl *Proc)(int, int, int, int, int, int, int);
        return ((Proc)procedure)(arguments[0], arguments[1], arguments[2],
                                 arguments[3], arguments[4], arguments[5],
                                 arguments[6]);
    }
    case 8: {
        typedef long(__cdecl *Proc)(int, int, int, int, int, int, int, int);
        return ((Proc)procedure)(arguments[0], arguments[1], arguments[2],
                                 arguments[3], arguments[4], arguments[5],
                                 arguments[6], arguments[7]);
    }
    case 9: {
        typedef long(__cdecl *Proc)(int, int, int, int, int, int, int, int,
                                    int);
        return ((Proc)procedure)(arguments[0], arguments[1], arguments[2],
                                 arguments[3], arguments[4], arguments[5],
                                 arguments[6], arguments[7], arguments[8]);
    }
    case 10: {
        typedef long(__cdecl *Proc)(int, int, int, int, int, int, int, int,
                                    int, int);
        return ((Proc)procedure)(arguments[0], arguments[1], arguments[2],
                                 arguments[3], arguments[4], arguments[5],
                                 arguments[6], arguments[7], arguments[8],
                                 arguments[9]);
    }
    case 11: {
        typedef long(__cdecl *Proc)(int, int, int, int, int, int, int, int,
                                    int, int, int);
        return ((Proc)procedure)(arguments[0], arguments[1], arguments[2],
                                 arguments[3], arguments[4], arguments[5],
                                 arguments[6], arguments[7], arguments[8],
                                 arguments[9], arguments[10]);
    }
    case 12: {
        typedef long(__cdecl *Proc)(int, int, int, int, int, int, int, int,
                                    int, int, int, int);
        return ((Proc)procedure)(arguments[0], arguments[1], arguments[2],
                                 arguments[3], arguments[4], arguments[5],
                                 arguments[6], arguments[7], arguments[8],
                                 arguments[9], arguments[10], arguments[11]);
    }
    case 13: {
        typedef long(__cdecl *Proc)(int, int, int, int, int, int, int, int,
                                    int, int, int, int, int);
        return ((Proc)procedure)(arguments[0], arguments[1], arguments[2],
                                 arguments[3], arguments[4], arguments[5],
                                 arguments[6], arguments[7], arguments[8],
                                 arguments[9], arguments[10], arguments[11],
                                 arguments[12]);
    }
    case 14: {
        typedef long(__cdecl *Proc)(int, int, int, int, int, int, int, int,
                                    int, int, int, int, int, int);
        return ((Proc)procedure)(arguments[0], arguments[1], arguments[2],
                                 arguments[3], arguments[4], arguments[5],
                                 arguments[6], arguments[7], arguments[8],
                                 arguments[9], arguments[10], arguments[11],
                                 arguments[12], arguments[13]);
    }
    case 15: {
        typedef long(__cdecl *Proc)(int, int, int, int, int, int, int, int,
                                    int, int, int, int, int, int, int);
        return ((Proc)procedure)(arguments[0], arguments[1], arguments[2],
                                 arguments[3], arguments[4], arguments[5],
                                 arguments[6], arguments[7], arguments[8],
                                 arguments[9], arguments[10], arguments[11],
                                 arguments[12], arguments[13], arguments[14]);
    }
    case 16: {
        typedef long(__cdecl *Proc)(int, int, int, int, int, int, int, int,
                                    int, int, int, int, int, int, int, int);
        return ((Proc)procedure)(arguments[0], arguments[1], arguments[2],
                                 arguments[3], arguments[4], arguments[5],
                                 arguments[6], arguments[7], arguments[8],
                                 arguments[9], arguments[10], arguments[11],
                                 arguments[12], arguments[13], arguments[14],
                                 arguments[15]);
    }
    default:
        return 0;
    }
}

static int NativeSlotsForDkt(int type) {
    enum { dktLongBridge = 1, dktDoubleBridge = 3, dktStringBridge = 4 };

    if (type == dktStringBridge) {
        return 2;
    }
    if (type == dktLongBridge) {
        return (int)((sizeof(long) + sizeof(int) - 1) / sizeof(int));
    }
    if (type == dktDoubleBridge) {
        return (int)((sizeof(double) + sizeof(int) - 1) / sizeof(int));
    }
    return 1;
}

enum NativeMacroArgumentKind {
    nativeMacroArgumentInteger,
    nativeMacroArgumentDouble
};

struct NativeMacroArgument {
    enum NativeMacroArgumentKind kind;
    uintptr_t integer;
    double number;
};

static long CallMacroArgsTyped6(void* procedure,
                                const struct NativeMacroArgument* native_arguments,
                                const int type_count,
                                const unsigned int double_mask) {
    switch (type_count) {
    case 1:
        switch (double_mask & 1u) {
        case 1u: {
            typedef long(__cdecl *Proc)(double);
            return ((Proc)procedure)(native_arguments[0].number);
        }
        default:
            return 0;
        }
    case 2:
        switch (double_mask & 3u) {
        case 1u: {
            typedef long(__cdecl *Proc)(double, uintptr_t);
            return ((Proc)procedure)(
                native_arguments[0].number,
                native_arguments[1].integer);
        }
        case 2u: {
            typedef long(__cdecl *Proc)(uintptr_t, double);
            return ((Proc)procedure)(
                native_arguments[0].integer,
                native_arguments[1].number);
        }
        case 3u: {
            typedef long(__cdecl *Proc)(double, double);
            return ((Proc)procedure)(
                native_arguments[0].number,
                native_arguments[1].number);
        }
        default:
            return 0;
        }
    case 3:
        switch (double_mask & 7u) {
        case 1u: {
            typedef long(__cdecl *Proc)(double, uintptr_t, uintptr_t);
            return ((Proc)procedure)(
                native_arguments[0].number,
                native_arguments[1].integer,
                native_arguments[2].integer);
        }
        case 2u: {
            typedef long(__cdecl *Proc)(uintptr_t, double, uintptr_t);
            return ((Proc)procedure)(
                native_arguments[0].integer,
                native_arguments[1].number,
                native_arguments[2].integer);
        }
        case 3u: {
            typedef long(__cdecl *Proc)(double, double, uintptr_t);
            return ((Proc)procedure)(
                native_arguments[0].number,
                native_arguments[1].number,
                native_arguments[2].integer);
        }
        case 4u: {
            typedef long(__cdecl *Proc)(uintptr_t, uintptr_t, double);
            return ((Proc)procedure)(
                native_arguments[0].integer,
                native_arguments[1].integer,
                native_arguments[2].number);
        }
        case 5u: {
            typedef long(__cdecl *Proc)(double, uintptr_t, double);
            return ((Proc)procedure)(
                native_arguments[0].number,
                native_arguments[1].integer,
                native_arguments[2].number);
        }
        case 6u: {
            typedef long(__cdecl *Proc)(uintptr_t, double, double);
            return ((Proc)procedure)(
                native_arguments[0].integer,
                native_arguments[1].number,
                native_arguments[2].number);
        }
        case 7u: {
            typedef long(__cdecl *Proc)(double, double, double);
            return ((Proc)procedure)(
                native_arguments[0].number,
                native_arguments[1].number,
                native_arguments[2].number);
        }
        default:
            return 0;
        }
    case 4:
        switch (double_mask & 15u) {
        case 1u: {
            typedef long(__cdecl *Proc)(double, uintptr_t, uintptr_t, uintptr_t);
            return ((Proc)procedure)(
                native_arguments[0].number,
                native_arguments[1].integer,
                native_arguments[2].integer,
                native_arguments[3].integer);
        }
        case 2u: {
            typedef long(__cdecl *Proc)(uintptr_t, double, uintptr_t, uintptr_t);
            return ((Proc)procedure)(
                native_arguments[0].integer,
                native_arguments[1].number,
                native_arguments[2].integer,
                native_arguments[3].integer);
        }
        case 3u: {
            typedef long(__cdecl *Proc)(double, double, uintptr_t, uintptr_t);
            return ((Proc)procedure)(
                native_arguments[0].number,
                native_arguments[1].number,
                native_arguments[2].integer,
                native_arguments[3].integer);
        }
        case 4u: {
            typedef long(__cdecl *Proc)(uintptr_t, uintptr_t, double, uintptr_t);
            return ((Proc)procedure)(
                native_arguments[0].integer,
                native_arguments[1].integer,
                native_arguments[2].number,
                native_arguments[3].integer);
        }
        case 5u: {
            typedef long(__cdecl *Proc)(double, uintptr_t, double, uintptr_t);
            return ((Proc)procedure)(
                native_arguments[0].number,
                native_arguments[1].integer,
                native_arguments[2].number,
                native_arguments[3].integer);
        }
        case 6u: {
            typedef long(__cdecl *Proc)(uintptr_t, double, double, uintptr_t);
            return ((Proc)procedure)(
                native_arguments[0].integer,
                native_arguments[1].number,
                native_arguments[2].number,
                native_arguments[3].integer);
        }
        case 7u: {
            typedef long(__cdecl *Proc)(double, double, double, uintptr_t);
            return ((Proc)procedure)(
                native_arguments[0].number,
                native_arguments[1].number,
                native_arguments[2].number,
                native_arguments[3].integer);
        }
        case 8u: {
            typedef long(__cdecl *Proc)(uintptr_t, uintptr_t, uintptr_t, double);
            return ((Proc)procedure)(
                native_arguments[0].integer,
                native_arguments[1].integer,
                native_arguments[2].integer,
                native_arguments[3].number);
        }
        case 9u: {
            typedef long(__cdecl *Proc)(double, uintptr_t, uintptr_t, double);
            return ((Proc)procedure)(
                native_arguments[0].number,
                native_arguments[1].integer,
                native_arguments[2].integer,
                native_arguments[3].number);
        }
        case 10u: {
            typedef long(__cdecl *Proc)(uintptr_t, double, uintptr_t, double);
            return ((Proc)procedure)(
                native_arguments[0].integer,
                native_arguments[1].number,
                native_arguments[2].integer,
                native_arguments[3].number);
        }
        case 11u: {
            typedef long(__cdecl *Proc)(double, double, uintptr_t, double);
            return ((Proc)procedure)(
                native_arguments[0].number,
                native_arguments[1].number,
                native_arguments[2].integer,
                native_arguments[3].number);
        }
        case 12u: {
            typedef long(__cdecl *Proc)(uintptr_t, uintptr_t, double, double);
            return ((Proc)procedure)(
                native_arguments[0].integer,
                native_arguments[1].integer,
                native_arguments[2].number,
                native_arguments[3].number);
        }
        case 13u: {
            typedef long(__cdecl *Proc)(double, uintptr_t, double, double);
            return ((Proc)procedure)(
                native_arguments[0].number,
                native_arguments[1].integer,
                native_arguments[2].number,
                native_arguments[3].number);
        }
        case 14u: {
            typedef long(__cdecl *Proc)(uintptr_t, double, double, double);
            return ((Proc)procedure)(
                native_arguments[0].integer,
                native_arguments[1].number,
                native_arguments[2].number,
                native_arguments[3].number);
        }
        case 15u: {
            typedef long(__cdecl *Proc)(double, double, double, double);
            return ((Proc)procedure)(
                native_arguments[0].number,
                native_arguments[1].number,
                native_arguments[2].number,
                native_arguments[3].number);
        }
        default:
            return 0;
        }
    case 5:
        switch (double_mask & 31u) {
        case 1u: {
            typedef long(__cdecl *Proc)(double, uintptr_t, uintptr_t, uintptr_t, uintptr_t);
            return ((Proc)procedure)(
                native_arguments[0].number,
                native_arguments[1].integer,
                native_arguments[2].integer,
                native_arguments[3].integer,
                native_arguments[4].integer);
        }
        case 2u: {
            typedef long(__cdecl *Proc)(uintptr_t, double, uintptr_t, uintptr_t, uintptr_t);
            return ((Proc)procedure)(
                native_arguments[0].integer,
                native_arguments[1].number,
                native_arguments[2].integer,
                native_arguments[3].integer,
                native_arguments[4].integer);
        }
        case 3u: {
            typedef long(__cdecl *Proc)(double, double, uintptr_t, uintptr_t, uintptr_t);
            return ((Proc)procedure)(
                native_arguments[0].number,
                native_arguments[1].number,
                native_arguments[2].integer,
                native_arguments[3].integer,
                native_arguments[4].integer);
        }
        case 4u: {
            typedef long(__cdecl *Proc)(uintptr_t, uintptr_t, double, uintptr_t, uintptr_t);
            return ((Proc)procedure)(
                native_arguments[0].integer,
                native_arguments[1].integer,
                native_arguments[2].number,
                native_arguments[3].integer,
                native_arguments[4].integer);
        }
        case 5u: {
            typedef long(__cdecl *Proc)(double, uintptr_t, double, uintptr_t, uintptr_t);
            return ((Proc)procedure)(
                native_arguments[0].number,
                native_arguments[1].integer,
                native_arguments[2].number,
                native_arguments[3].integer,
                native_arguments[4].integer);
        }
        case 6u: {
            typedef long(__cdecl *Proc)(uintptr_t, double, double, uintptr_t, uintptr_t);
            return ((Proc)procedure)(
                native_arguments[0].integer,
                native_arguments[1].number,
                native_arguments[2].number,
                native_arguments[3].integer,
                native_arguments[4].integer);
        }
        case 7u: {
            typedef long(__cdecl *Proc)(double, double, double, uintptr_t, uintptr_t);
            return ((Proc)procedure)(
                native_arguments[0].number,
                native_arguments[1].number,
                native_arguments[2].number,
                native_arguments[3].integer,
                native_arguments[4].integer);
        }
        case 8u: {
            typedef long(__cdecl *Proc)(uintptr_t, uintptr_t, uintptr_t, double, uintptr_t);
            return ((Proc)procedure)(
                native_arguments[0].integer,
                native_arguments[1].integer,
                native_arguments[2].integer,
                native_arguments[3].number,
                native_arguments[4].integer);
        }
        case 9u: {
            typedef long(__cdecl *Proc)(double, uintptr_t, uintptr_t, double, uintptr_t);
            return ((Proc)procedure)(
                native_arguments[0].number,
                native_arguments[1].integer,
                native_arguments[2].integer,
                native_arguments[3].number,
                native_arguments[4].integer);
        }
        case 10u: {
            typedef long(__cdecl *Proc)(uintptr_t, double, uintptr_t, double, uintptr_t);
            return ((Proc)procedure)(
                native_arguments[0].integer,
                native_arguments[1].number,
                native_arguments[2].integer,
                native_arguments[3].number,
                native_arguments[4].integer);
        }
        case 11u: {
            typedef long(__cdecl *Proc)(double, double, uintptr_t, double, uintptr_t);
            return ((Proc)procedure)(
                native_arguments[0].number,
                native_arguments[1].number,
                native_arguments[2].integer,
                native_arguments[3].number,
                native_arguments[4].integer);
        }
        case 12u: {
            typedef long(__cdecl *Proc)(uintptr_t, uintptr_t, double, double, uintptr_t);
            return ((Proc)procedure)(
                native_arguments[0].integer,
                native_arguments[1].integer,
                native_arguments[2].number,
                native_arguments[3].number,
                native_arguments[4].integer);
        }
        case 13u: {
            typedef long(__cdecl *Proc)(double, uintptr_t, double, double, uintptr_t);
            return ((Proc)procedure)(
                native_arguments[0].number,
                native_arguments[1].integer,
                native_arguments[2].number,
                native_arguments[3].number,
                native_arguments[4].integer);
        }
        case 14u: {
            typedef long(__cdecl *Proc)(uintptr_t, double, double, double, uintptr_t);
            return ((Proc)procedure)(
                native_arguments[0].integer,
                native_arguments[1].number,
                native_arguments[2].number,
                native_arguments[3].number,
                native_arguments[4].integer);
        }
        case 15u: {
            typedef long(__cdecl *Proc)(double, double, double, double, uintptr_t);
            return ((Proc)procedure)(
                native_arguments[0].number,
                native_arguments[1].number,
                native_arguments[2].number,
                native_arguments[3].number,
                native_arguments[4].integer);
        }
        case 16u: {
            typedef long(__cdecl *Proc)(uintptr_t, uintptr_t, uintptr_t, uintptr_t, double);
            return ((Proc)procedure)(
                native_arguments[0].integer,
                native_arguments[1].integer,
                native_arguments[2].integer,
                native_arguments[3].integer,
                native_arguments[4].number);
        }
        case 17u: {
            typedef long(__cdecl *Proc)(double, uintptr_t, uintptr_t, uintptr_t, double);
            return ((Proc)procedure)(
                native_arguments[0].number,
                native_arguments[1].integer,
                native_arguments[2].integer,
                native_arguments[3].integer,
                native_arguments[4].number);
        }
        case 18u: {
            typedef long(__cdecl *Proc)(uintptr_t, double, uintptr_t, uintptr_t, double);
            return ((Proc)procedure)(
                native_arguments[0].integer,
                native_arguments[1].number,
                native_arguments[2].integer,
                native_arguments[3].integer,
                native_arguments[4].number);
        }
        case 19u: {
            typedef long(__cdecl *Proc)(double, double, uintptr_t, uintptr_t, double);
            return ((Proc)procedure)(
                native_arguments[0].number,
                native_arguments[1].number,
                native_arguments[2].integer,
                native_arguments[3].integer,
                native_arguments[4].number);
        }
        case 20u: {
            typedef long(__cdecl *Proc)(uintptr_t, uintptr_t, double, uintptr_t, double);
            return ((Proc)procedure)(
                native_arguments[0].integer,
                native_arguments[1].integer,
                native_arguments[2].number,
                native_arguments[3].integer,
                native_arguments[4].number);
        }
        case 21u: {
            typedef long(__cdecl *Proc)(double, uintptr_t, double, uintptr_t, double);
            return ((Proc)procedure)(
                native_arguments[0].number,
                native_arguments[1].integer,
                native_arguments[2].number,
                native_arguments[3].integer,
                native_arguments[4].number);
        }
        case 22u: {
            typedef long(__cdecl *Proc)(uintptr_t, double, double, uintptr_t, double);
            return ((Proc)procedure)(
                native_arguments[0].integer,
                native_arguments[1].number,
                native_arguments[2].number,
                native_arguments[3].integer,
                native_arguments[4].number);
        }
        case 23u: {
            typedef long(__cdecl *Proc)(double, double, double, uintptr_t, double);
            return ((Proc)procedure)(
                native_arguments[0].number,
                native_arguments[1].number,
                native_arguments[2].number,
                native_arguments[3].integer,
                native_arguments[4].number);
        }
        case 24u: {
            typedef long(__cdecl *Proc)(uintptr_t, uintptr_t, uintptr_t, double, double);
            return ((Proc)procedure)(
                native_arguments[0].integer,
                native_arguments[1].integer,
                native_arguments[2].integer,
                native_arguments[3].number,
                native_arguments[4].number);
        }
        case 25u: {
            typedef long(__cdecl *Proc)(double, uintptr_t, uintptr_t, double, double);
            return ((Proc)procedure)(
                native_arguments[0].number,
                native_arguments[1].integer,
                native_arguments[2].integer,
                native_arguments[3].number,
                native_arguments[4].number);
        }
        case 26u: {
            typedef long(__cdecl *Proc)(uintptr_t, double, uintptr_t, double, double);
            return ((Proc)procedure)(
                native_arguments[0].integer,
                native_arguments[1].number,
                native_arguments[2].integer,
                native_arguments[3].number,
                native_arguments[4].number);
        }
        case 27u: {
            typedef long(__cdecl *Proc)(double, double, uintptr_t, double, double);
            return ((Proc)procedure)(
                native_arguments[0].number,
                native_arguments[1].number,
                native_arguments[2].integer,
                native_arguments[3].number,
                native_arguments[4].number);
        }
        case 28u: {
            typedef long(__cdecl *Proc)(uintptr_t, uintptr_t, double, double, double);
            return ((Proc)procedure)(
                native_arguments[0].integer,
                native_arguments[1].integer,
                native_arguments[2].number,
                native_arguments[3].number,
                native_arguments[4].number);
        }
        case 29u: {
            typedef long(__cdecl *Proc)(double, uintptr_t, double, double, double);
            return ((Proc)procedure)(
                native_arguments[0].number,
                native_arguments[1].integer,
                native_arguments[2].number,
                native_arguments[3].number,
                native_arguments[4].number);
        }
        case 30u: {
            typedef long(__cdecl *Proc)(uintptr_t, double, double, double, double);
            return ((Proc)procedure)(
                native_arguments[0].integer,
                native_arguments[1].number,
                native_arguments[2].number,
                native_arguments[3].number,
                native_arguments[4].number);
        }
        case 31u: {
            typedef long(__cdecl *Proc)(double, double, double, double, double);
            return ((Proc)procedure)(
                native_arguments[0].number,
                native_arguments[1].number,
                native_arguments[2].number,
                native_arguments[3].number,
                native_arguments[4].number);
        }
        default:
            return 0;
        }
    case 6:
        switch (double_mask & 63u) {
        case 1u: {
            typedef long(__cdecl *Proc)(double, uintptr_t, uintptr_t, uintptr_t, uintptr_t, uintptr_t);
            return ((Proc)procedure)(
                native_arguments[0].number,
                native_arguments[1].integer,
                native_arguments[2].integer,
                native_arguments[3].integer,
                native_arguments[4].integer,
                native_arguments[5].integer);
        }
        case 2u: {
            typedef long(__cdecl *Proc)(uintptr_t, double, uintptr_t, uintptr_t, uintptr_t, uintptr_t);
            return ((Proc)procedure)(
                native_arguments[0].integer,
                native_arguments[1].number,
                native_arguments[2].integer,
                native_arguments[3].integer,
                native_arguments[4].integer,
                native_arguments[5].integer);
        }
        case 3u: {
            typedef long(__cdecl *Proc)(double, double, uintptr_t, uintptr_t, uintptr_t, uintptr_t);
            return ((Proc)procedure)(
                native_arguments[0].number,
                native_arguments[1].number,
                native_arguments[2].integer,
                native_arguments[3].integer,
                native_arguments[4].integer,
                native_arguments[5].integer);
        }
        case 4u: {
            typedef long(__cdecl *Proc)(uintptr_t, uintptr_t, double, uintptr_t, uintptr_t, uintptr_t);
            return ((Proc)procedure)(
                native_arguments[0].integer,
                native_arguments[1].integer,
                native_arguments[2].number,
                native_arguments[3].integer,
                native_arguments[4].integer,
                native_arguments[5].integer);
        }
        case 5u: {
            typedef long(__cdecl *Proc)(double, uintptr_t, double, uintptr_t, uintptr_t, uintptr_t);
            return ((Proc)procedure)(
                native_arguments[0].number,
                native_arguments[1].integer,
                native_arguments[2].number,
                native_arguments[3].integer,
                native_arguments[4].integer,
                native_arguments[5].integer);
        }
        case 6u: {
            typedef long(__cdecl *Proc)(uintptr_t, double, double, uintptr_t, uintptr_t, uintptr_t);
            return ((Proc)procedure)(
                native_arguments[0].integer,
                native_arguments[1].number,
                native_arguments[2].number,
                native_arguments[3].integer,
                native_arguments[4].integer,
                native_arguments[5].integer);
        }
        case 7u: {
            typedef long(__cdecl *Proc)(double, double, double, uintptr_t, uintptr_t, uintptr_t);
            return ((Proc)procedure)(
                native_arguments[0].number,
                native_arguments[1].number,
                native_arguments[2].number,
                native_arguments[3].integer,
                native_arguments[4].integer,
                native_arguments[5].integer);
        }
        case 8u: {
            typedef long(__cdecl *Proc)(uintptr_t, uintptr_t, uintptr_t, double, uintptr_t, uintptr_t);
            return ((Proc)procedure)(
                native_arguments[0].integer,
                native_arguments[1].integer,
                native_arguments[2].integer,
                native_arguments[3].number,
                native_arguments[4].integer,
                native_arguments[5].integer);
        }
        case 9u: {
            typedef long(__cdecl *Proc)(double, uintptr_t, uintptr_t, double, uintptr_t, uintptr_t);
            return ((Proc)procedure)(
                native_arguments[0].number,
                native_arguments[1].integer,
                native_arguments[2].integer,
                native_arguments[3].number,
                native_arguments[4].integer,
                native_arguments[5].integer);
        }
        case 10u: {
            typedef long(__cdecl *Proc)(uintptr_t, double, uintptr_t, double, uintptr_t, uintptr_t);
            return ((Proc)procedure)(
                native_arguments[0].integer,
                native_arguments[1].number,
                native_arguments[2].integer,
                native_arguments[3].number,
                native_arguments[4].integer,
                native_arguments[5].integer);
        }
        case 11u: {
            typedef long(__cdecl *Proc)(double, double, uintptr_t, double, uintptr_t, uintptr_t);
            return ((Proc)procedure)(
                native_arguments[0].number,
                native_arguments[1].number,
                native_arguments[2].integer,
                native_arguments[3].number,
                native_arguments[4].integer,
                native_arguments[5].integer);
        }
        case 12u: {
            typedef long(__cdecl *Proc)(uintptr_t, uintptr_t, double, double, uintptr_t, uintptr_t);
            return ((Proc)procedure)(
                native_arguments[0].integer,
                native_arguments[1].integer,
                native_arguments[2].number,
                native_arguments[3].number,
                native_arguments[4].integer,
                native_arguments[5].integer);
        }
        case 13u: {
            typedef long(__cdecl *Proc)(double, uintptr_t, double, double, uintptr_t, uintptr_t);
            return ((Proc)procedure)(
                native_arguments[0].number,
                native_arguments[1].integer,
                native_arguments[2].number,
                native_arguments[3].number,
                native_arguments[4].integer,
                native_arguments[5].integer);
        }
        case 14u: {
            typedef long(__cdecl *Proc)(uintptr_t, double, double, double, uintptr_t, uintptr_t);
            return ((Proc)procedure)(
                native_arguments[0].integer,
                native_arguments[1].number,
                native_arguments[2].number,
                native_arguments[3].number,
                native_arguments[4].integer,
                native_arguments[5].integer);
        }
        case 15u: {
            typedef long(__cdecl *Proc)(double, double, double, double, uintptr_t, uintptr_t);
            return ((Proc)procedure)(
                native_arguments[0].number,
                native_arguments[1].number,
                native_arguments[2].number,
                native_arguments[3].number,
                native_arguments[4].integer,
                native_arguments[5].integer);
        }
        case 16u: {
            typedef long(__cdecl *Proc)(uintptr_t, uintptr_t, uintptr_t, uintptr_t, double, uintptr_t);
            return ((Proc)procedure)(
                native_arguments[0].integer,
                native_arguments[1].integer,
                native_arguments[2].integer,
                native_arguments[3].integer,
                native_arguments[4].number,
                native_arguments[5].integer);
        }
        case 17u: {
            typedef long(__cdecl *Proc)(double, uintptr_t, uintptr_t, uintptr_t, double, uintptr_t);
            return ((Proc)procedure)(
                native_arguments[0].number,
                native_arguments[1].integer,
                native_arguments[2].integer,
                native_arguments[3].integer,
                native_arguments[4].number,
                native_arguments[5].integer);
        }
        case 18u: {
            typedef long(__cdecl *Proc)(uintptr_t, double, uintptr_t, uintptr_t, double, uintptr_t);
            return ((Proc)procedure)(
                native_arguments[0].integer,
                native_arguments[1].number,
                native_arguments[2].integer,
                native_arguments[3].integer,
                native_arguments[4].number,
                native_arguments[5].integer);
        }
        case 19u: {
            typedef long(__cdecl *Proc)(double, double, uintptr_t, uintptr_t, double, uintptr_t);
            return ((Proc)procedure)(
                native_arguments[0].number,
                native_arguments[1].number,
                native_arguments[2].integer,
                native_arguments[3].integer,
                native_arguments[4].number,
                native_arguments[5].integer);
        }
        case 20u: {
            typedef long(__cdecl *Proc)(uintptr_t, uintptr_t, double, uintptr_t, double, uintptr_t);
            return ((Proc)procedure)(
                native_arguments[0].integer,
                native_arguments[1].integer,
                native_arguments[2].number,
                native_arguments[3].integer,
                native_arguments[4].number,
                native_arguments[5].integer);
        }
        case 21u: {
            typedef long(__cdecl *Proc)(double, uintptr_t, double, uintptr_t, double, uintptr_t);
            return ((Proc)procedure)(
                native_arguments[0].number,
                native_arguments[1].integer,
                native_arguments[2].number,
                native_arguments[3].integer,
                native_arguments[4].number,
                native_arguments[5].integer);
        }
        case 22u: {
            typedef long(__cdecl *Proc)(uintptr_t, double, double, uintptr_t, double, uintptr_t);
            return ((Proc)procedure)(
                native_arguments[0].integer,
                native_arguments[1].number,
                native_arguments[2].number,
                native_arguments[3].integer,
                native_arguments[4].number,
                native_arguments[5].integer);
        }
        case 23u: {
            typedef long(__cdecl *Proc)(double, double, double, uintptr_t, double, uintptr_t);
            return ((Proc)procedure)(
                native_arguments[0].number,
                native_arguments[1].number,
                native_arguments[2].number,
                native_arguments[3].integer,
                native_arguments[4].number,
                native_arguments[5].integer);
        }
        case 24u: {
            typedef long(__cdecl *Proc)(uintptr_t, uintptr_t, uintptr_t, double, double, uintptr_t);
            return ((Proc)procedure)(
                native_arguments[0].integer,
                native_arguments[1].integer,
                native_arguments[2].integer,
                native_arguments[3].number,
                native_arguments[4].number,
                native_arguments[5].integer);
        }
        case 25u: {
            typedef long(__cdecl *Proc)(double, uintptr_t, uintptr_t, double, double, uintptr_t);
            return ((Proc)procedure)(
                native_arguments[0].number,
                native_arguments[1].integer,
                native_arguments[2].integer,
                native_arguments[3].number,
                native_arguments[4].number,
                native_arguments[5].integer);
        }
        case 26u: {
            typedef long(__cdecl *Proc)(uintptr_t, double, uintptr_t, double, double, uintptr_t);
            return ((Proc)procedure)(
                native_arguments[0].integer,
                native_arguments[1].number,
                native_arguments[2].integer,
                native_arguments[3].number,
                native_arguments[4].number,
                native_arguments[5].integer);
        }
        case 27u: {
            typedef long(__cdecl *Proc)(double, double, uintptr_t, double, double, uintptr_t);
            return ((Proc)procedure)(
                native_arguments[0].number,
                native_arguments[1].number,
                native_arguments[2].integer,
                native_arguments[3].number,
                native_arguments[4].number,
                native_arguments[5].integer);
        }
        case 28u: {
            typedef long(__cdecl *Proc)(uintptr_t, uintptr_t, double, double, double, uintptr_t);
            return ((Proc)procedure)(
                native_arguments[0].integer,
                native_arguments[1].integer,
                native_arguments[2].number,
                native_arguments[3].number,
                native_arguments[4].number,
                native_arguments[5].integer);
        }
        case 29u: {
            typedef long(__cdecl *Proc)(double, uintptr_t, double, double, double, uintptr_t);
            return ((Proc)procedure)(
                native_arguments[0].number,
                native_arguments[1].integer,
                native_arguments[2].number,
                native_arguments[3].number,
                native_arguments[4].number,
                native_arguments[5].integer);
        }
        case 30u: {
            typedef long(__cdecl *Proc)(uintptr_t, double, double, double, double, uintptr_t);
            return ((Proc)procedure)(
                native_arguments[0].integer,
                native_arguments[1].number,
                native_arguments[2].number,
                native_arguments[3].number,
                native_arguments[4].number,
                native_arguments[5].integer);
        }
        case 31u: {
            typedef long(__cdecl *Proc)(double, double, double, double, double, uintptr_t);
            return ((Proc)procedure)(
                native_arguments[0].number,
                native_arguments[1].number,
                native_arguments[2].number,
                native_arguments[3].number,
                native_arguments[4].number,
                native_arguments[5].integer);
        }
        case 32u: {
            typedef long(__cdecl *Proc)(uintptr_t, uintptr_t, uintptr_t, uintptr_t, uintptr_t, double);
            return ((Proc)procedure)(
                native_arguments[0].integer,
                native_arguments[1].integer,
                native_arguments[2].integer,
                native_arguments[3].integer,
                native_arguments[4].integer,
                native_arguments[5].number);
        }
        case 33u: {
            typedef long(__cdecl *Proc)(double, uintptr_t, uintptr_t, uintptr_t, uintptr_t, double);
            return ((Proc)procedure)(
                native_arguments[0].number,
                native_arguments[1].integer,
                native_arguments[2].integer,
                native_arguments[3].integer,
                native_arguments[4].integer,
                native_arguments[5].number);
        }
        case 34u: {
            typedef long(__cdecl *Proc)(uintptr_t, double, uintptr_t, uintptr_t, uintptr_t, double);
            return ((Proc)procedure)(
                native_arguments[0].integer,
                native_arguments[1].number,
                native_arguments[2].integer,
                native_arguments[3].integer,
                native_arguments[4].integer,
                native_arguments[5].number);
        }
        case 35u: {
            typedef long(__cdecl *Proc)(double, double, uintptr_t, uintptr_t, uintptr_t, double);
            return ((Proc)procedure)(
                native_arguments[0].number,
                native_arguments[1].number,
                native_arguments[2].integer,
                native_arguments[3].integer,
                native_arguments[4].integer,
                native_arguments[5].number);
        }
        case 36u: {
            typedef long(__cdecl *Proc)(uintptr_t, uintptr_t, double, uintptr_t, uintptr_t, double);
            return ((Proc)procedure)(
                native_arguments[0].integer,
                native_arguments[1].integer,
                native_arguments[2].number,
                native_arguments[3].integer,
                native_arguments[4].integer,
                native_arguments[5].number);
        }
        case 37u: {
            typedef long(__cdecl *Proc)(double, uintptr_t, double, uintptr_t, uintptr_t, double);
            return ((Proc)procedure)(
                native_arguments[0].number,
                native_arguments[1].integer,
                native_arguments[2].number,
                native_arguments[3].integer,
                native_arguments[4].integer,
                native_arguments[5].number);
        }
        case 38u: {
            typedef long(__cdecl *Proc)(uintptr_t, double, double, uintptr_t, uintptr_t, double);
            return ((Proc)procedure)(
                native_arguments[0].integer,
                native_arguments[1].number,
                native_arguments[2].number,
                native_arguments[3].integer,
                native_arguments[4].integer,
                native_arguments[5].number);
        }
        case 39u: {
            typedef long(__cdecl *Proc)(double, double, double, uintptr_t, uintptr_t, double);
            return ((Proc)procedure)(
                native_arguments[0].number,
                native_arguments[1].number,
                native_arguments[2].number,
                native_arguments[3].integer,
                native_arguments[4].integer,
                native_arguments[5].number);
        }
        case 40u: {
            typedef long(__cdecl *Proc)(uintptr_t, uintptr_t, uintptr_t, double, uintptr_t, double);
            return ((Proc)procedure)(
                native_arguments[0].integer,
                native_arguments[1].integer,
                native_arguments[2].integer,
                native_arguments[3].number,
                native_arguments[4].integer,
                native_arguments[5].number);
        }
        case 41u: {
            typedef long(__cdecl *Proc)(double, uintptr_t, uintptr_t, double, uintptr_t, double);
            return ((Proc)procedure)(
                native_arguments[0].number,
                native_arguments[1].integer,
                native_arguments[2].integer,
                native_arguments[3].number,
                native_arguments[4].integer,
                native_arguments[5].number);
        }
        case 42u: {
            typedef long(__cdecl *Proc)(uintptr_t, double, uintptr_t, double, uintptr_t, double);
            return ((Proc)procedure)(
                native_arguments[0].integer,
                native_arguments[1].number,
                native_arguments[2].integer,
                native_arguments[3].number,
                native_arguments[4].integer,
                native_arguments[5].number);
        }
        case 43u: {
            typedef long(__cdecl *Proc)(double, double, uintptr_t, double, uintptr_t, double);
            return ((Proc)procedure)(
                native_arguments[0].number,
                native_arguments[1].number,
                native_arguments[2].integer,
                native_arguments[3].number,
                native_arguments[4].integer,
                native_arguments[5].number);
        }
        case 44u: {
            typedef long(__cdecl *Proc)(uintptr_t, uintptr_t, double, double, uintptr_t, double);
            return ((Proc)procedure)(
                native_arguments[0].integer,
                native_arguments[1].integer,
                native_arguments[2].number,
                native_arguments[3].number,
                native_arguments[4].integer,
                native_arguments[5].number);
        }
        case 45u: {
            typedef long(__cdecl *Proc)(double, uintptr_t, double, double, uintptr_t, double);
            return ((Proc)procedure)(
                native_arguments[0].number,
                native_arguments[1].integer,
                native_arguments[2].number,
                native_arguments[3].number,
                native_arguments[4].integer,
                native_arguments[5].number);
        }
        case 46u: {
            typedef long(__cdecl *Proc)(uintptr_t, double, double, double, uintptr_t, double);
            return ((Proc)procedure)(
                native_arguments[0].integer,
                native_arguments[1].number,
                native_arguments[2].number,
                native_arguments[3].number,
                native_arguments[4].integer,
                native_arguments[5].number);
        }
        case 47u: {
            typedef long(__cdecl *Proc)(double, double, double, double, uintptr_t, double);
            return ((Proc)procedure)(
                native_arguments[0].number,
                native_arguments[1].number,
                native_arguments[2].number,
                native_arguments[3].number,
                native_arguments[4].integer,
                native_arguments[5].number);
        }
        case 48u: {
            typedef long(__cdecl *Proc)(uintptr_t, uintptr_t, uintptr_t, uintptr_t, double, double);
            return ((Proc)procedure)(
                native_arguments[0].integer,
                native_arguments[1].integer,
                native_arguments[2].integer,
                native_arguments[3].integer,
                native_arguments[4].number,
                native_arguments[5].number);
        }
        case 49u: {
            typedef long(__cdecl *Proc)(double, uintptr_t, uintptr_t, uintptr_t, double, double);
            return ((Proc)procedure)(
                native_arguments[0].number,
                native_arguments[1].integer,
                native_arguments[2].integer,
                native_arguments[3].integer,
                native_arguments[4].number,
                native_arguments[5].number);
        }
        case 50u: {
            typedef long(__cdecl *Proc)(uintptr_t, double, uintptr_t, uintptr_t, double, double);
            return ((Proc)procedure)(
                native_arguments[0].integer,
                native_arguments[1].number,
                native_arguments[2].integer,
                native_arguments[3].integer,
                native_arguments[4].number,
                native_arguments[5].number);
        }
        case 51u: {
            typedef long(__cdecl *Proc)(double, double, uintptr_t, uintptr_t, double, double);
            return ((Proc)procedure)(
                native_arguments[0].number,
                native_arguments[1].number,
                native_arguments[2].integer,
                native_arguments[3].integer,
                native_arguments[4].number,
                native_arguments[5].number);
        }
        case 52u: {
            typedef long(__cdecl *Proc)(uintptr_t, uintptr_t, double, uintptr_t, double, double);
            return ((Proc)procedure)(
                native_arguments[0].integer,
                native_arguments[1].integer,
                native_arguments[2].number,
                native_arguments[3].integer,
                native_arguments[4].number,
                native_arguments[5].number);
        }
        case 53u: {
            typedef long(__cdecl *Proc)(double, uintptr_t, double, uintptr_t, double, double);
            return ((Proc)procedure)(
                native_arguments[0].number,
                native_arguments[1].integer,
                native_arguments[2].number,
                native_arguments[3].integer,
                native_arguments[4].number,
                native_arguments[5].number);
        }
        case 54u: {
            typedef long(__cdecl *Proc)(uintptr_t, double, double, uintptr_t, double, double);
            return ((Proc)procedure)(
                native_arguments[0].integer,
                native_arguments[1].number,
                native_arguments[2].number,
                native_arguments[3].integer,
                native_arguments[4].number,
                native_arguments[5].number);
        }
        case 55u: {
            typedef long(__cdecl *Proc)(double, double, double, uintptr_t, double, double);
            return ((Proc)procedure)(
                native_arguments[0].number,
                native_arguments[1].number,
                native_arguments[2].number,
                native_arguments[3].integer,
                native_arguments[4].number,
                native_arguments[5].number);
        }
        case 56u: {
            typedef long(__cdecl *Proc)(uintptr_t, uintptr_t, uintptr_t, double, double, double);
            return ((Proc)procedure)(
                native_arguments[0].integer,
                native_arguments[1].integer,
                native_arguments[2].integer,
                native_arguments[3].number,
                native_arguments[4].number,
                native_arguments[5].number);
        }
        case 57u: {
            typedef long(__cdecl *Proc)(double, uintptr_t, uintptr_t, double, double, double);
            return ((Proc)procedure)(
                native_arguments[0].number,
                native_arguments[1].integer,
                native_arguments[2].integer,
                native_arguments[3].number,
                native_arguments[4].number,
                native_arguments[5].number);
        }
        case 58u: {
            typedef long(__cdecl *Proc)(uintptr_t, double, uintptr_t, double, double, double);
            return ((Proc)procedure)(
                native_arguments[0].integer,
                native_arguments[1].number,
                native_arguments[2].integer,
                native_arguments[3].number,
                native_arguments[4].number,
                native_arguments[5].number);
        }
        case 59u: {
            typedef long(__cdecl *Proc)(double, double, uintptr_t, double, double, double);
            return ((Proc)procedure)(
                native_arguments[0].number,
                native_arguments[1].number,
                native_arguments[2].integer,
                native_arguments[3].number,
                native_arguments[4].number,
                native_arguments[5].number);
        }
        case 60u: {
            typedef long(__cdecl *Proc)(uintptr_t, uintptr_t, double, double, double, double);
            return ((Proc)procedure)(
                native_arguments[0].integer,
                native_arguments[1].integer,
                native_arguments[2].number,
                native_arguments[3].number,
                native_arguments[4].number,
                native_arguments[5].number);
        }
        case 61u: {
            typedef long(__cdecl *Proc)(double, uintptr_t, double, double, double, double);
            return ((Proc)procedure)(
                native_arguments[0].number,
                native_arguments[1].integer,
                native_arguments[2].number,
                native_arguments[3].number,
                native_arguments[4].number,
                native_arguments[5].number);
        }
        case 62u: {
            typedef long(__cdecl *Proc)(uintptr_t, double, double, double, double, double);
            return ((Proc)procedure)(
                native_arguments[0].integer,
                native_arguments[1].number,
                native_arguments[2].number,
                native_arguments[3].number,
                native_arguments[4].number,
                native_arguments[5].number);
        }
        case 63u: {
            typedef long(__cdecl *Proc)(double, double, double, double, double, double);
            return ((Proc)procedure)(
                native_arguments[0].number,
                native_arguments[1].number,
                native_arguments[2].number,
                native_arguments[3].number,
                native_arguments[4].number,
                native_arguments[5].number);
        }
        default:
            return 0;
        }
    default:
        return 0;
    }
}

static long CallMacroArgsOneDouble7(
    void* procedure, const struct NativeMacroArgument* native_arguments,
    const unsigned int double_mask) {
    switch (double_mask & 127u) {
    case 1u: {
        typedef long(__cdecl *Proc)(double, int, int, int, int, int, int);
        return ((Proc)procedure)(
            native_arguments[0].number,
            native_arguments[1].integer,
            native_arguments[2].integer,
            native_arguments[3].integer,
            native_arguments[4].integer,
            native_arguments[5].integer,
            native_arguments[6].integer);
    }
    case 2u: {
        typedef long(__cdecl *Proc)(int, double, int, int, int, int, int);
        return ((Proc)procedure)(
            native_arguments[0].integer,
            native_arguments[1].number,
            native_arguments[2].integer,
            native_arguments[3].integer,
            native_arguments[4].integer,
            native_arguments[5].integer,
            native_arguments[6].integer);
    }
    case 4u: {
        typedef long(__cdecl *Proc)(int, int, double, int, int, int, int);
        return ((Proc)procedure)(
            native_arguments[0].integer,
            native_arguments[1].integer,
            native_arguments[2].number,
            native_arguments[3].integer,
            native_arguments[4].integer,
            native_arguments[5].integer,
            native_arguments[6].integer);
    }
    case 8u: {
        typedef long(__cdecl *Proc)(int, int, int, double, int, int, int);
        return ((Proc)procedure)(
            native_arguments[0].integer,
            native_arguments[1].integer,
            native_arguments[2].integer,
            native_arguments[3].number,
            native_arguments[4].integer,
            native_arguments[5].integer,
            native_arguments[6].integer);
    }
    case 16u: {
        typedef long(__cdecl *Proc)(int, int, int, int, double, int, int);
        return ((Proc)procedure)(
            native_arguments[0].integer,
            native_arguments[1].integer,
            native_arguments[2].integer,
            native_arguments[3].integer,
            native_arguments[4].number,
            native_arguments[5].integer,
            native_arguments[6].integer);
    }
    case 32u: {
        typedef long(__cdecl *Proc)(int, int, int, int, int, double, int);
        return ((Proc)procedure)(
            native_arguments[0].integer,
            native_arguments[1].integer,
            native_arguments[2].integer,
            native_arguments[3].integer,
            native_arguments[4].integer,
            native_arguments[5].number,
            native_arguments[6].integer);
    }
    case 64u: {
        typedef long(__cdecl *Proc)(int, int, int, int, int, int, double);
        return ((Proc)procedure)(
            native_arguments[0].integer,
            native_arguments[1].integer,
            native_arguments[2].integer,
            native_arguments[3].integer,
            native_arguments[4].integer,
            native_arguments[5].integer,
            native_arguments[6].number);
    }
    default:
        return 0;
    }
}
long LPushMacroArgsTyped(void* procedure, const int* arguments,
                         const int argument_count, const int* types,
                         const int type_count) {
    enum { dktDoubleBridge = 3, dktStringBridge = 4 };
    struct NativeMacroArgument native_arguments[16] = {{0}};
    int argument_index = 0;
    int double_count = 0;
    int single_slot_non_doubles = 1;
    unsigned int double_mask = 0;
    int slot_index = 0;

    if (procedure == NULL || argument_count < 0 || argument_count > 16 ||
        type_count < 0 || type_count > 16 ||
        (argument_count != 0 && arguments == NULL) ||
        (type_count != 0 && types == NULL)) {
        return 0;
    }
    for (argument_index = 0; argument_index < type_count; ++argument_index) {
        int slots = NativeSlotsForDkt(types[argument_index]);
        if (slot_index + slots > argument_count) {
            return 0;
        }
        if (types[argument_index] != dktDoubleBridge && slots != 1) {
            single_slot_non_doubles = 0;
        }
        if (types[argument_index] == dktStringBridge) {
            uint32_t low = (uint32_t)arguments[slot_index];
            uint32_t high = (uint32_t)arguments[slot_index + 1];
#if UINTPTR_MAX > UINT32_MAX
            native_arguments[argument_index].integer =
                ((uintptr_t)high << 32) | (uintptr_t)low;
#else
            (void)high;
            native_arguments[argument_index].integer = (uintptr_t)low;
#endif
        } else if (types[argument_index] == dktDoubleBridge) {
            native_arguments[argument_index].kind = nativeMacroArgumentDouble;
            memcpy(&native_arguments[argument_index].number,
                   arguments + slot_index,
                   sizeof(native_arguments[argument_index].number));
            ++double_count;
            double_mask |= 1u << argument_index;
        } else {
            size_t bytes = (size_t)slots * sizeof(int);
            if (bytes > sizeof(native_arguments[argument_index].integer)) {
                bytes = sizeof(native_arguments[argument_index].integer);
            }
            memcpy(&native_arguments[argument_index].integer,
                   arguments + slot_index, bytes);
        }
        slot_index += slots;
    }
    if (slot_index != argument_count) {
        return 0;
    }
    if (double_count != 0) {
        if (type_count <= 6) {
            return CallMacroArgsTyped6(procedure, native_arguments, type_count,
                                       double_mask);
        }
        if (double_count == 1 && single_slot_non_doubles &&
            type_count == 7) {
            return CallMacroArgsOneDouble7(procedure, native_arguments,
                                           double_mask);
        }
        return 0;
    }
    switch (type_count) {
    case 0: {
        typedef long(__cdecl *Proc)(void);
        return ((Proc)procedure)();
    }
    case 1: {
        typedef long(__cdecl *Proc)(uintptr_t);
        return ((Proc)procedure)(native_arguments[0].integer);
    }
    case 2: {
        typedef long(__cdecl *Proc)(uintptr_t, uintptr_t);
        return ((Proc)procedure)(native_arguments[0].integer,
                                 native_arguments[1].integer);
    }
    case 3: {
        typedef long(__cdecl *Proc)(uintptr_t, uintptr_t, uintptr_t);
        return ((Proc)procedure)(native_arguments[0].integer,
                                 native_arguments[1].integer,
                                 native_arguments[2].integer);
    }
    case 4: {
        typedef long(__cdecl *Proc)(uintptr_t, uintptr_t, uintptr_t,
                                    uintptr_t);
        return ((Proc)procedure)(native_arguments[0].integer,
                                 native_arguments[1].integer,
                                 native_arguments[2].integer,
                                 native_arguments[3].integer);
    }
    case 5: {
        typedef long(__cdecl *Proc)(uintptr_t, uintptr_t, uintptr_t,
                                    uintptr_t, uintptr_t);
        return ((Proc)procedure)(native_arguments[0].integer,
                                 native_arguments[1].integer,
                                 native_arguments[2].integer,
                                 native_arguments[3].integer,
                                 native_arguments[4].integer);
    }
    case 6: {
        typedef long(__cdecl *Proc)(uintptr_t, uintptr_t, uintptr_t,
                                    uintptr_t, uintptr_t, uintptr_t);
        return ((Proc)procedure)(native_arguments[0].integer,
                                 native_arguments[1].integer,
                                 native_arguments[2].integer,
                                 native_arguments[3].integer,
                                 native_arguments[4].integer,
                                 native_arguments[5].integer);
    }
    case 7: {
        typedef long(__cdecl *Proc)(uintptr_t, uintptr_t, uintptr_t,
                                    uintptr_t, uintptr_t, uintptr_t,
                                    uintptr_t);
        return ((Proc)procedure)(native_arguments[0].integer,
                                 native_arguments[1].integer,
                                 native_arguments[2].integer,
                                 native_arguments[3].integer,
                                 native_arguments[4].integer,
                                 native_arguments[5].integer,
                                 native_arguments[6].integer);
    }
    case 8: {
        typedef long(__cdecl *Proc)(uintptr_t, uintptr_t, uintptr_t,
                                    uintptr_t, uintptr_t, uintptr_t,
                                    uintptr_t, uintptr_t);
        return ((Proc)procedure)(native_arguments[0].integer,
                                 native_arguments[1].integer,
                                 native_arguments[2].integer,
                                 native_arguments[3].integer,
                                 native_arguments[4].integer,
                                 native_arguments[5].integer,
                                 native_arguments[6].integer,
                                 native_arguments[7].integer);
    }
    case 9: {
        typedef long(__cdecl *Proc)(uintptr_t, uintptr_t, uintptr_t,
                                    uintptr_t, uintptr_t, uintptr_t,
                                    uintptr_t, uintptr_t, uintptr_t);
        return ((Proc)procedure)(native_arguments[0].integer,
                                 native_arguments[1].integer,
                                 native_arguments[2].integer,
                                 native_arguments[3].integer,
                                 native_arguments[4].integer,
                                 native_arguments[5].integer,
                                 native_arguments[6].integer,
                                 native_arguments[7].integer,
                                 native_arguments[8].integer);
    }
    case 10: {
        typedef long(__cdecl *Proc)(uintptr_t, uintptr_t, uintptr_t,
                                    uintptr_t, uintptr_t, uintptr_t,
                                    uintptr_t, uintptr_t, uintptr_t,
                                    uintptr_t);
        return ((Proc)procedure)(native_arguments[0].integer,
                                 native_arguments[1].integer,
                                 native_arguments[2].integer,
                                 native_arguments[3].integer,
                                 native_arguments[4].integer,
                                 native_arguments[5].integer,
                                 native_arguments[6].integer,
                                 native_arguments[7].integer,
                                 native_arguments[8].integer,
                                 native_arguments[9].integer);
    }
    case 11: {
        typedef long(__cdecl *Proc)(uintptr_t, uintptr_t, uintptr_t,
                                    uintptr_t, uintptr_t, uintptr_t,
                                    uintptr_t, uintptr_t, uintptr_t,
                                    uintptr_t, uintptr_t);
        return ((Proc)procedure)(native_arguments[0].integer,
                                 native_arguments[1].integer,
                                 native_arguments[2].integer,
                                 native_arguments[3].integer,
                                 native_arguments[4].integer,
                                 native_arguments[5].integer,
                                 native_arguments[6].integer,
                                 native_arguments[7].integer,
                                 native_arguments[8].integer,
                                 native_arguments[9].integer,
                                 native_arguments[10].integer);
    }
    case 12: {
        typedef long(__cdecl *Proc)(uintptr_t, uintptr_t, uintptr_t,
                                    uintptr_t, uintptr_t, uintptr_t,
                                    uintptr_t, uintptr_t, uintptr_t,
                                    uintptr_t, uintptr_t, uintptr_t);
        return ((Proc)procedure)(native_arguments[0].integer,
                                 native_arguments[1].integer,
                                 native_arguments[2].integer,
                                 native_arguments[3].integer,
                                 native_arguments[4].integer,
                                 native_arguments[5].integer,
                                 native_arguments[6].integer,
                                 native_arguments[7].integer,
                                 native_arguments[8].integer,
                                 native_arguments[9].integer,
                                 native_arguments[10].integer,
                                 native_arguments[11].integer);
    }
    case 13: {
        typedef long(__cdecl *Proc)(uintptr_t, uintptr_t, uintptr_t,
                                    uintptr_t, uintptr_t, uintptr_t,
                                    uintptr_t, uintptr_t, uintptr_t,
                                    uintptr_t, uintptr_t, uintptr_t,
                                    uintptr_t);
        return ((Proc)procedure)(native_arguments[0].integer,
                                 native_arguments[1].integer,
                                 native_arguments[2].integer,
                                 native_arguments[3].integer,
                                 native_arguments[4].integer,
                                 native_arguments[5].integer,
                                 native_arguments[6].integer,
                                 native_arguments[7].integer,
                                 native_arguments[8].integer,
                                 native_arguments[9].integer,
                                 native_arguments[10].integer,
                                 native_arguments[11].integer,
                                 native_arguments[12].integer);
    }
    case 14: {
        typedef long(__cdecl *Proc)(uintptr_t, uintptr_t, uintptr_t,
                                    uintptr_t, uintptr_t, uintptr_t,
                                    uintptr_t, uintptr_t, uintptr_t,
                                    uintptr_t, uintptr_t, uintptr_t,
                                    uintptr_t, uintptr_t);
        return ((Proc)procedure)(native_arguments[0].integer,
                                 native_arguments[1].integer,
                                 native_arguments[2].integer,
                                 native_arguments[3].integer,
                                 native_arguments[4].integer,
                                 native_arguments[5].integer,
                                 native_arguments[6].integer,
                                 native_arguments[7].integer,
                                 native_arguments[8].integer,
                                 native_arguments[9].integer,
                                 native_arguments[10].integer,
                                 native_arguments[11].integer,
                                 native_arguments[12].integer,
                                 native_arguments[13].integer);
    }
    case 15: {
        typedef long(__cdecl *Proc)(uintptr_t, uintptr_t, uintptr_t,
                                    uintptr_t, uintptr_t, uintptr_t,
                                    uintptr_t, uintptr_t, uintptr_t,
                                    uintptr_t, uintptr_t, uintptr_t,
                                    uintptr_t, uintptr_t, uintptr_t);
        return ((Proc)procedure)(native_arguments[0].integer,
                                 native_arguments[1].integer,
                                 native_arguments[2].integer,
                                 native_arguments[3].integer,
                                 native_arguments[4].integer,
                                 native_arguments[5].integer,
                                 native_arguments[6].integer,
                                 native_arguments[7].integer,
                                 native_arguments[8].integer,
                                 native_arguments[9].integer,
                                 native_arguments[10].integer,
                                 native_arguments[11].integer,
                                 native_arguments[12].integer,
                                 native_arguments[13].integer,
                                 native_arguments[14].integer);
    }
    case 16: {
        typedef long(__cdecl *Proc)(uintptr_t, uintptr_t, uintptr_t,
                                    uintptr_t, uintptr_t, uintptr_t,
                                    uintptr_t, uintptr_t, uintptr_t,
                                    uintptr_t, uintptr_t, uintptr_t,
                                    uintptr_t, uintptr_t, uintptr_t,
                                    uintptr_t);
        return ((Proc)procedure)(native_arguments[0].integer,
                                 native_arguments[1].integer,
                                 native_arguments[2].integer,
                                 native_arguments[3].integer,
                                 native_arguments[4].integer,
                                 native_arguments[5].integer,
                                 native_arguments[6].integer,
                                 native_arguments[7].integer,
                                 native_arguments[8].integer,
                                 native_arguments[9].integer,
                                 native_arguments[10].integer,
                                 native_arguments[11].integer,
                                 native_arguments[12].integer,
                                 native_arguments[13].integer,
                                 native_arguments[14].integer,
                                 native_arguments[15].integer);
    }
    default:
        return LPushMacroArgs(procedure, arguments, argument_count);
    }
}

long MemUsed(const int memory_type) {
    size_t used;
    if ((memory_type & 1) == 0) {
        return 0;
    }
    used = OpusHeapBytesUsed();
    if (used > (size_t)LONG_MAX) {
        return LONG_MAX;
    }
    return (long)used;
}

LRESULT CALLBACK PlaybackHook(const int code, const WPARAM parameter,
                              const LPARAM message_pointer) {
    HANDLE queue_handle;
    struct NativePlaybackQueue* queue;
    int finished;
    if (code != HC_GETNEXT && code != HC_SKIP) {
        return CallNextHookEx(NULL, code, parameter, message_pointer);
    }
    if (lphevtHead == NULL || *lphevtHead == NULL) {
        return 0;
    }

    queue_handle = *lphevtHead;
    queue = (struct NativePlaybackQueue*)GlobalLock(queue_handle);
    if (queue == NULL) {
        return 0;
    }

    finished = 0;
    if (code == HC_SKIP) {
        ++queue->current;
        finished = queue->current >= queue->count;
    } else if (message_pointer != 0 && queue->current >= 0 &&
               queue->current < queue->count) {
        EVENTMSG* output = (EVENTMSG*)message_pointer;
        const struct NativePlaybackEvent* event = &queue->events[queue->current];
        output->message = event->message;
        output->paramL = event->virtual_key;
        output->paramH = 1;
        output->time = 0;
        output->hwnd = NULL;
    }
    GlobalUnlock(queue_handle);

    if (finished) {
        GlobalFree(queue_handle);
        *lphevtHead = NULL;
        if (lphrgbKeyState != NULL && *lphrgbKeyState != NULL) {
            HANDLE state_handle = *lphrgbKeyState;
            unsigned char* state =
                (unsigned char*)GlobalLock(state_handle);
            if (state != NULL) {
                SetKeyboardState(state);
                GlobalUnlock(state_handle);
            }
            GlobalFree(state_handle);
            *lphrgbKeyState = NULL;
        }
    }
    return 0;
}
