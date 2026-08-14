#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * AMD64 translation of the accumulator boundary supplied by Word's 8087
 * math package and the transcendental portion of NATTRANS.ASM.  The public
 * NUM representation used by the original C code is an IEEE-754 double, so
 * the segmented accumulator can be represented directly without changing
 * mathapi.c or any of its callers.
 */

typedef struct OpusNum {
    union {
        char bytes[8];
        double value;
    } num;
} OpusNum;

typedef struct OpusUnpackedString {
    char digits[15];
    char count;
    uint32_t exponent_and_sign;
} OpusUnpackedString;

typedef char opus_num_must_be_8_bytes[(sizeof(OpusNum) == 8) ? 1 : -1];

static double accumulator = 0.0;
static int math_error = 0;

enum {
    kMathOverflow = 0x0001,
    kMathUnderflow = 0x0002,
    kMathDivideByZero = 0x0004,
    kMathTranscendental = 0x0008
};

static void record_result(double result) {
    accumulator = result;
    if (isinf(result)) {
        math_error |= kMathOverflow;
    } else if (isnan(result)) {
        math_error |= kMathTranscendental;
    } else if (result != 0.0 && fpclassify(result) == FP_SUBNORMAL) {
        math_error |= kMathUnderflow;
    }
}

/* The original diagnostic reports whether the optional 8087 path is active.
 * AMD64 always has hardware floating-point support. */
int f8087 = 1;
int fError = 0;

int InitMathPack(void) {
    accumulator = 0.0;
    math_error = 0;
    fError = 0;
    return 1;
}

int FInitMathPack(void) { return InitMathPack(); }
void TermMathPack(void) {}

int DError(int replacement) {
    int previous = math_error;
    math_error = replacement;
    fError = replacement != 0;
    return previous;
}

void Dld(const OpusNum *number) {
    accumulator = number == NULL ? 0.0 : number->num.value;
}

void Dst(OpusNum *number) {
    if (number != NULL) {
        number->num.value = accumulator;
    }
}

void DClr(void) { accumulator = 0.0; }

void DLdC(int index) {
    static const double constants[] = {
        1.0, 2.0, -1.0, 10.0, 100.0,
        2.30258509299404568402, 0.5, -2.0e60
    };
    if (index < 0 ||
        index >= (int)(sizeof(constants) / sizeof(constants[0]))) {
        accumulator = 0.0;
        math_error |= kMathTranscendental;
        return;
    }
    accumulator = constants[index];
}

void DFloat(unsigned int value) {
    accumulator = (double)value;
}

void DAdd(const OpusNum *operand) {
    record_result(accumulator + operand->num.value);
}

void DSub(const OpusNum *operand) {
    record_result(accumulator - operand->num.value);
}

void DMul(const OpusNum *operand) {
    record_result(accumulator * operand->num.value);
}

void DDiv(const OpusNum *operand) {
    if (operand->num.value == 0.0) {
        math_error |= kMathDivideByZero;
        record_result(copysign(INFINITY, accumulator));
        return;
    }
    record_result(accumulator / operand->num.value);
}

void DNeg(void) { accumulator = -accumulator; }

int DCond(void) {
    if (isnan(accumulator)) {
        math_error |= kMathTranscendental;
        return 0;
    }
    return accumulator < 0.0 ? -1 : accumulator > 0.0 ? 1 : 0;
}

void DInt(void) { accumulator = trunc(accumulator); }

unsigned int Fix(void) {
    double fixed = trunc(accumulator);
    if (!isfinite(fixed) || fixed < 0.0 || fixed > 65535.0) {
        math_error |= kMathOverflow;
        return fixed < 0.0 ? 0u : 65535u;
    }
    return (unsigned int)fixed;
}

void TenTo(int exponent) {
    record_result(pow(10.0, (double)exponent));
}

void Exp(void) { record_result(exp(accumulator)); }

void Ln(void) {
    if (accumulator <= 0.0) {
        math_error |= kMathTranscendental;
        accumulator = NAN;
        return;
    }
    record_result(log(accumulator));
}

void Sqr(void) {
    if (accumulator < 0.0) {
        math_error |= kMathTranscendental;
        accumulator = NAN;
        return;
    }
    record_result(sqrt(accumulator));
}

void SQR(void) { Sqr(); }

void Unpack(OpusUnpackedString *unpacked) {
    int negative;
    double magnitude;
    char scientific[32] = {0};
    const char *exponent_marker;
    int decimal_exponent;
    int count;
    const char *cursor;

    if (unpacked == NULL) {
        return;
    }
    memset(unpacked, 0, sizeof(*unpacked));

    negative = signbit(accumulator) != 0;
    magnitude = fabs(accumulator);
    if (magnitude == 0.0 || !isfinite(magnitude)) {
        unpacked->digits[0] = '0';
        unpacked->count = 1;
        unpacked->exponent_and_sign =
            0x4000u | (negative ? 0x8000u : 0u);
        if (!isfinite(magnitude)) {
            math_error |= kMathTranscendental;
        }
        return;
    }

    snprintf(scientific, sizeof(scientific), "%.14e", magnitude);
    exponent_marker = strchr(scientific, 'e');
    decimal_exponent =
        exponent_marker == NULL ? 0 : atoi(exponent_marker + 1);

    count = 0;
    for (cursor = scientific;
         cursor != exponent_marker && *cursor != '\0' && count < 15;
         ++cursor) {
        if (*cursor >= '0' && *cursor <= '9') {
            unpacked->digits[count++] = *cursor;
        }
    }
    while (count > 1 && unpacked->digits[count - 1] == '0') {
        --count;
    }
    unpacked->count = (char)count;
    unpacked->exponent_and_sign =
        (uint32_t)(0x4000 + decimal_exponent + 1) |
        (negative ? 0x8000u : 0u);
}

/* token.c contains the historical mixed-case spelling. */
int CNumInt(int value);
int CnumInt(int value) { return CNumInt(value); }
