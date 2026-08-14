#include "opus_elx_dispatch.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef struct NativeNum {
    union {
        unsigned char rgb[8];
        double d;
    } num;
} NativeNum;

typedef char native_num_size[(sizeof(NativeNum) == 8) ? 1 : -1];

static NativeNum read_num(const OPUS_ELX_NATIVE_ARG *argument) {
    NativeNum value;
    memcpy(&value, argument->num, sizeof(value));
    return value;
}

#define WITH_FIRST_ARG(index, next)                                            \
    do {                                                                       \
        switch (arguments[(index)].kind) {                                     \
        case opusElxArgInt:                                                    \
            next(int, arguments[(index)].integer);                             \
            break;                                                             \
        case opusElxArgPointer:                                                \
            next(void *, arguments[(index)].pointer);                          \
            break;                                                             \
        case opusElxArgNum:                                                    \
            next(NativeNum, read_num(&arguments[(index)]));                    \
            break;                                                             \
        default:                                                               \
            return false;                                                      \
        }                                                                      \
    } while (0)

#define WITH_NEXT_ARG0(next, ...)                                              \
    do {                                                                       \
        switch (arguments[0].kind) {                                           \
        case opusElxArgInt:                                                    \
            next(__VA_ARGS__, int, arguments[0].integer);                      \
            break;                                                             \
        case opusElxArgPointer:                                                \
            next(__VA_ARGS__, void *, arguments[0].pointer);                   \
            break;                                                             \
        case opusElxArgNum:                                                    \
            next(__VA_ARGS__, NativeNum, read_num(&arguments[0]));             \
            break;                                                             \
        default:                                                               \
            return false;                                                      \
        }                                                                      \
    } while (0)

#define WITH_NEXT_ARG1(next, ...)                                              \
    do {                                                                       \
        switch (arguments[1].kind) {                                           \
        case opusElxArgInt:                                                    \
            next(__VA_ARGS__, int, arguments[1].integer);                      \
            break;                                                             \
        case opusElxArgPointer:                                                \
            next(__VA_ARGS__, void *, arguments[1].pointer);                   \
            break;                                                             \
        case opusElxArgNum:                                                    \
            next(__VA_ARGS__, NativeNum, read_num(&arguments[1]));             \
            break;                                                             \
        default:                                                               \
            return false;                                                      \
        }                                                                      \
    } while (0)

#define WITH_NEXT_ARG2(next, ...)                                              \
    do {                                                                       \
        switch (arguments[2].kind) {                                           \
        case opusElxArgInt:                                                    \
            next(__VA_ARGS__, int, arguments[2].integer);                      \
            break;                                                             \
        case opusElxArgPointer:                                                \
            next(__VA_ARGS__, void *, arguments[2].pointer);                   \
            break;                                                             \
        case opusElxArgNum:                                                    \
            next(__VA_ARGS__, NativeNum, read_num(&arguments[2]));             \
            break;                                                             \
        default:                                                               \
            return false;                                                      \
        }                                                                      \
    } while (0)

#define WITH_NEXT_ARG3(next, ...)                                              \
    do {                                                                       \
        switch (arguments[3].kind) {                                           \
        case opusElxArgInt:                                                    \
            next(__VA_ARGS__, int, arguments[3].integer);                      \
            break;                                                             \
        case opusElxArgPointer:                                                \
            next(__VA_ARGS__, void *, arguments[3].pointer);                   \
            break;                                                             \
        case opusElxArgNum:                                                    \
            next(__VA_ARGS__, NativeNum, read_num(&arguments[3]));             \
            break;                                                             \
        default:                                                               \
            return false;                                                      \
        }                                                                      \
    } while (0)

#define WITH_NEXT_ARG4(next, ...)                                              \
    do {                                                                       \
        switch (arguments[4].kind) {                                           \
        case opusElxArgInt:                                                    \
            next(__VA_ARGS__, int, arguments[4].integer);                      \
            break;                                                             \
        case opusElxArgPointer:                                                \
            next(__VA_ARGS__, void *, arguments[4].pointer);                   \
            break;                                                             \
        case opusElxArgNum:                                                    \
            next(__VA_ARGS__, NativeNum, read_num(&arguments[4]));             \
            break;                                                             \
        default:                                                               \
            return false;                                                      \
        }                                                                      \
    } while (0)

#define WITH_NEXT_ARG5(next, ...)                                              \
    do {                                                                       \
        switch (arguments[5].kind) {                                           \
        case opusElxArgInt:                                                    \
            next(__VA_ARGS__, int, arguments[5].integer);                      \
            break;                                                             \
        case opusElxArgPointer:                                                \
            next(__VA_ARGS__, void *, arguments[5].pointer);                   \
            break;                                                             \
        case opusElxArgNum:                                                    \
            next(__VA_ARGS__, NativeNum, read_num(&arguments[5]));             \
            break;                                                             \
        default:                                                               \
            return false;                                                      \
        }                                                                      \
    } while (0)

#define DISPATCH1(call) WITH_FIRST_ARG(0, call)
#define DISPATCH2(call) WITH_NEXT_ARG0(DISPATCH2_1, call)
#define DISPATCH2_1(call, t0, v0) WITH_NEXT_ARG1(call, t0, v0)
#define DISPATCH3(call) WITH_NEXT_ARG0(DISPATCH3_1, call)
#define DISPATCH3_1(call, t0, v0) WITH_NEXT_ARG1(DISPATCH3_2, call, t0, v0)
#define DISPATCH3_2(call, t0, v0, t1, v1) WITH_NEXT_ARG2(call, t0, v0, t1, v1)
#define DISPATCH4(call) WITH_NEXT_ARG0(DISPATCH4_1, call)
#define DISPATCH4_1(call, t0, v0) WITH_NEXT_ARG1(DISPATCH4_2, call, t0, v0)
#define DISPATCH4_2(call, t0, v0, t1, v1)                                     \
    WITH_NEXT_ARG2(DISPATCH4_3, call, t0, v0, t1, v1)
#define DISPATCH4_3(call, t0, v0, t1, v1, t2, v2)                             \
    WITH_NEXT_ARG3(call, t0, v0, t1, v1, t2, v2)
#define DISPATCH5(call) WITH_NEXT_ARG0(DISPATCH5_1, call)
#define DISPATCH5_1(call, t0, v0) WITH_NEXT_ARG1(DISPATCH5_2, call, t0, v0)
#define DISPATCH5_2(call, t0, v0, t1, v1)                                     \
    WITH_NEXT_ARG2(DISPATCH5_3, call, t0, v0, t1, v1)
#define DISPATCH5_3(call, t0, v0, t1, v1, t2, v2)                             \
    WITH_NEXT_ARG3(DISPATCH5_4, call, t0, v0, t1, v1, t2, v2)
#define DISPATCH5_4(call, t0, v0, t1, v1, t2, v2, t3, v3)                     \
    WITH_NEXT_ARG4(call, t0, v0, t1, v1, t2, v2, t3, v3)
#define DISPATCH6(call) WITH_NEXT_ARG0(DISPATCH6_1, call)
#define DISPATCH6_1(call, t0, v0) WITH_NEXT_ARG1(DISPATCH6_2, call, t0, v0)
#define DISPATCH6_2(call, t0, v0, t1, v1)                                     \
    WITH_NEXT_ARG2(DISPATCH6_3, call, t0, v0, t1, v1)
#define DISPATCH6_3(call, t0, v0, t1, v1, t2, v2)                             \
    WITH_NEXT_ARG3(DISPATCH6_4, call, t0, v0, t1, v1, t2, v2)
#define DISPATCH6_4(call, t0, v0, t1, v1, t2, v2, t3, v3)                     \
    WITH_NEXT_ARG4(DISPATCH6_5, call, t0, v0, t1, v1, t2, v2, t3, v3)
#define DISPATCH6_5(call, t0, v0, t1, v1, t2, v2, t3, v3, t4, v4)             \
    WITH_NEXT_ARG5(call, t0, v0, t1, v1, t2, v2, t3, v3, t4, v4)

static bool invoke_void(void *pfn, const OPUS_ELX_NATIVE_ARG *arguments,
                        int count) {
#define CALL0()                                                                \
    do {                                                                       \
        ((void (*)(void))pfn)();                                               \
        return true;                                                           \
    } while (0)
#define CALL1(t0, v0)                                                          \
    do {                                                                       \
        ((void (*)(t0))pfn)(v0);                                               \
        return true;                                                           \
    } while (0)
#define CALL2(t0, v0, t1, v1)                                                  \
    do {                                                                       \
        ((void (*)(t0, t1))pfn)(v0, v1);                                       \
        return true;                                                           \
    } while (0)
#define CALL3(t0, v0, t1, v1, t2, v2)                                          \
    do {                                                                       \
        ((void (*)(t0, t1, t2))pfn)(v0, v1, v2);                               \
        return true;                                                           \
    } while (0)
#define CALL4(t0, v0, t1, v1, t2, v2, t3, v3)                                  \
    do {                                                                       \
        ((void (*)(t0, t1, t2, t3))pfn)(v0, v1, v2, v3);                       \
        return true;                                                           \
    } while (0)
#define CALL5(t0, v0, t1, v1, t2, v2, t3, v3, t4, v4)                          \
    do {                                                                       \
        ((void (*)(t0, t1, t2, t3, t4))pfn)(v0, v1, v2, v3, v4);               \
        return true;                                                           \
    } while (0)
#define CALL6(t0, v0, t1, v1, t2, v2, t3, v3, t4, v4, t5, v5)                  \
    do {                                                                       \
        ((void (*)(t0, t1, t2, t3, t4, t5))pfn)(v0, v1, v2, v3, v4, v5);       \
        return true;                                                           \
    } while (0)

    switch (count) {
    case 0: CALL0();
    case 1: DISPATCH1(CALL1);
    case 2: DISPATCH2(CALL2);
    case 3: DISPATCH3(CALL3);
    case 4: DISPATCH4(CALL4);
    case 5: DISPATCH5(CALL5);
    case 6: DISPATCH6(CALL6);
    default: return false;
    }

#undef CALL0
#undef CALL1
#undef CALL2
#undef CALL3
#undef CALL4
#undef CALL5
#undef CALL6
}

static bool invoke_num(void *pfn, const OPUS_ELX_NATIVE_ARG *arguments,
                       int count, void *result) {
#define CALL0()                                                                \
    do {                                                                       \
        NativeNum value = ((NativeNum (*)(void))pfn)();                        \
        memcpy(result, &value, sizeof(value));                                 \
        return true;                                                           \
    } while (0)
#define CALL1(t0, v0)                                                          \
    do {                                                                       \
        NativeNum value = ((NativeNum (*)(t0))pfn)(v0);                        \
        memcpy(result, &value, sizeof(value));                                 \
        return true;                                                           \
    } while (0)
#define CALL2(t0, v0, t1, v1)                                                  \
    do {                                                                       \
        NativeNum value = ((NativeNum (*)(t0, t1))pfn)(v0, v1);                \
        memcpy(result, &value, sizeof(value));                                 \
        return true;                                                           \
    } while (0)
#define CALL3(t0, v0, t1, v1, t2, v2)                                          \
    do {                                                                       \
        NativeNum value = ((NativeNum (*)(t0, t1, t2))pfn)(v0, v1, v2);        \
        memcpy(result, &value, sizeof(value));                                 \
        return true;                                                           \
    } while (0)
#define CALL4(t0, v0, t1, v1, t2, v2, t3, v3)                                  \
    do {                                                                       \
        NativeNum value =                                                       \
            ((NativeNum (*)(t0, t1, t2, t3))pfn)(v0, v1, v2, v3);              \
        memcpy(result, &value, sizeof(value));                                 \
        return true;                                                           \
    } while (0)
#define CALL5(t0, v0, t1, v1, t2, v2, t3, v3, t4, v4)                          \
    do {                                                                       \
        NativeNum value =                                                       \
            ((NativeNum (*)(t0, t1, t2, t3, t4))pfn)(v0, v1, v2, v3, v4);      \
        memcpy(result, &value, sizeof(value));                                 \
        return true;                                                           \
    } while (0)
#define CALL6(t0, v0, t1, v1, t2, v2, t3, v3, t4, v4, t5, v5)                  \
    do {                                                                       \
        NativeNum value = ((NativeNum (*)(t0, t1, t2, t3, t4, t5))pfn)(        \
            v0, v1, v2, v3, v4, v5);                                           \
        memcpy(result, &value, sizeof(value));                                 \
        return true;                                                           \
    } while (0)

    switch (count) {
    case 0: CALL0();
    case 1: DISPATCH1(CALL1);
    case 2: DISPATCH2(CALL2);
    case 3: DISPATCH3(CALL3);
    case 4: DISPATCH4(CALL4);
    case 5: DISPATCH5(CALL5);
    case 6: DISPATCH6(CALL6);
    default: return false;
    }

#undef CALL0
#undef CALL1
#undef CALL2
#undef CALL3
#undef CALL4
#undef CALL5
#undef CALL6
}

static bool invoke_int(void *pfn, const OPUS_ELX_NATIVE_ARG *arguments,
                       int count, void *result) {
#define CALL0()                                                                \
    do {                                                                       \
        *(int *)result = ((int (*)(void))pfn)();                               \
        return true;                                                           \
    } while (0)
#define CALL1(t0, v0)                                                          \
    do {                                                                       \
        *(int *)result = ((int (*)(t0))pfn)(v0);                               \
        return true;                                                           \
    } while (0)
#define CALL2(t0, v0, t1, v1)                                                  \
    do {                                                                       \
        *(int *)result = ((int (*)(t0, t1))pfn)(v0, v1);                       \
        return true;                                                           \
    } while (0)
#define CALL3(t0, v0, t1, v1, t2, v2)                                          \
    do {                                                                       \
        *(int *)result = ((int (*)(t0, t1, t2))pfn)(v0, v1, v2);               \
        return true;                                                           \
    } while (0)
#define CALL4(t0, v0, t1, v1, t2, v2, t3, v3)                                  \
    do {                                                                       \
        *(int *)result = ((int (*)(t0, t1, t2, t3))pfn)(v0, v1, v2, v3);       \
        return true;                                                           \
    } while (0)
#define CALL5(t0, v0, t1, v1, t2, v2, t3, v3, t4, v4)                          \
    do {                                                                       \
        *(int *)result =                                                       \
            ((int (*)(t0, t1, t2, t3, t4))pfn)(v0, v1, v2, v3, v4);            \
        return true;                                                           \
    } while (0)
#define CALL6(t0, v0, t1, v1, t2, v2, t3, v3, t4, v4, t5, v5)                  \
    do {                                                                       \
        *(int *)result = ((int (*)(t0, t1, t2, t3, t4, t5))pfn)(               \
            v0, v1, v2, v3, v4, v5);                                           \
        return true;                                                           \
    } while (0)

    switch (count) {
    case 0: CALL0();
    case 1: DISPATCH1(CALL1);
    case 2: DISPATCH2(CALL2);
    case 3: DISPATCH3(CALL3);
    case 4: DISPATCH4(CALL4);
    case 5: DISPATCH5(CALL5);
    case 6: DISPATCH6(CALL6);
    default: return false;
    }

#undef CALL0
#undef CALL1
#undef CALL2
#undef CALL3
#undef CALL4
#undef CALL5
#undef CALL6
}

static bool invoke_uintptr(void *pfn, const OPUS_ELX_NATIVE_ARG *arguments,
                           int count, void *result) {
#define CALL0()                                                                \
    do {                                                                       \
        *(uintptr_t *)result = ((uintptr_t (*)(void))pfn)();                   \
        return true;                                                           \
    } while (0)
#define CALL1(t0, v0)                                                          \
    do {                                                                       \
        *(uintptr_t *)result = ((uintptr_t (*)(t0))pfn)(v0);                   \
        return true;                                                           \
    } while (0)
#define CALL2(t0, v0, t1, v1)                                                  \
    do {                                                                       \
        *(uintptr_t *)result = ((uintptr_t (*)(t0, t1))pfn)(v0, v1);           \
        return true;                                                           \
    } while (0)
#define CALL3(t0, v0, t1, v1, t2, v2)                                          \
    do {                                                                       \
        *(uintptr_t *)result =                                                 \
            ((uintptr_t (*)(t0, t1, t2))pfn)(v0, v1, v2);                     \
        return true;                                                           \
    } while (0)
#define CALL4(t0, v0, t1, v1, t2, v2, t3, v3)                                  \
    do {                                                                       \
        *(uintptr_t *)result =                                                 \
            ((uintptr_t (*)(t0, t1, t2, t3))pfn)(v0, v1, v2, v3);             \
        return true;                                                           \
    } while (0)
#define CALL5(t0, v0, t1, v1, t2, v2, t3, v3, t4, v4)                          \
    do {                                                                       \
        *(uintptr_t *)result =                                                 \
            ((uintptr_t (*)(t0, t1, t2, t3, t4))pfn)(v0, v1, v2, v3, v4);      \
        return true;                                                           \
    } while (0)
#define CALL6(t0, v0, t1, v1, t2, v2, t3, v3, t4, v4, t5, v5)                  \
    do {                                                                       \
        *(uintptr_t *)result =                                                 \
            ((uintptr_t (*)(t0, t1, t2, t3, t4, t5))pfn)(                     \
                v0, v1, v2, v3, v4, v5);                                      \
        return true;                                                           \
    } while (0)

    switch (count) {
    case 0: CALL0();
    case 1: DISPATCH1(CALL1);
    case 2: DISPATCH2(CALL2);
    case 3: DISPATCH3(CALL3);
    case 4: DISPATCH4(CALL4);
    case 5: DISPATCH5(CALL5);
    case 6: DISPATCH6(CALL6);
    default: return false;
    }

#undef CALL0
#undef CALL1
#undef CALL2
#undef CALL3
#undef CALL4
#undef CALL5
#undef CALL6
}

int OpusInvokeElx(void *pfn, int return_kind, int argument_count,
                  const OPUS_ELX_NATIVE_ARG *arguments, void *result) {
    if (pfn == NULL || argument_count < 0 || argument_count > 6 ||
        (argument_count != 0 && arguments == NULL)) {
        return 0;
    }

    switch (return_kind) {
    case 0:
        return invoke_void(pfn, arguments, argument_count) ? 1 : 0;
    case 1:
        return invoke_num(pfn, arguments, argument_count, result) ? 1 : 0;
    case 2:
        return invoke_int(pfn, arguments, argument_count, result) ? 1 : 0;
    case 3:
        return invoke_uintptr(pfn, arguments, argument_count, result) ? 1 : 0;
    default:
        return 0;
    }
}
