#include "opus_x64_compat.h"
#include "opus_x64_heap.h"

#include <limits.h>
#include <stddef.h>

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
