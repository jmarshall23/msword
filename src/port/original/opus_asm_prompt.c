#include <stdint.h>
#include <string.h>

/* AMD64 translation of PROMPTN.ASM.  Pointer members use their native size;
 * the scalar field ordering is identical to prompt.h::PPR. */
typedef struct NativeProgressReport NativeProgressReport;

struct NativeProgressReport {
    int ich;
    int cch;
    unsigned int nLast;
    unsigned int nIncr;
    int abort_check;
    NativeProgressReport** previous;
    char** previous_text;
    short x_pixels;
};

extern NativeProgressReport** vhpprPRPrompt;
int PopToHppr(NativeProgressReport** report);
int AdjustPrompt(int first_character, int character_count, int x_pixels,
                 char* replacement);

static int ClampInt(int value, int low, int high) {
    if (value < low) {
        return low;
    }
    return value > high ? high : value;
}

static int64_t ClampI64(int64_t value, int64_t low, int64_t high) {
    if (value < low) {
        return low;
    }
    return value > high ? high : value;
}

void ChangeProgressReport(NativeProgressReport** report,
                          unsigned int new_value) {
    NativeProgressReport* current;
    unsigned int increment;
    char digits[6];
    int width;
    unsigned int remaining;
    int position;
    char* replacement;

    if (report == NULL) {
        return;
    }
    if (report != vhpprPRPrompt) {
        (void)PopToHppr(report);
    }
    current = *report;
    if (current == NULL || current->nIncr == 0) {
        return;
    }

    increment = current->nIncr;
    new_value = ((new_value + increment / 2u) / increment) * increment;
    if (new_value == current->nLast) {
        return;
    }
    current->nLast = new_value;

    memset(digits, ' ', sizeof(digits));
    width = ClampInt(current->cch, 0, 6);
    remaining = new_value;
    position = 6;
    do {
        if (position <= 6 - width) {
            break;
        }
        digits[--position] = (char)('0' + (int)(remaining % 10u));
        remaining /= 10u;
    } while (remaining != 0);

    replacement = digits + (6 - width);
    (void)AdjustPrompt(current->ich, width, current->x_pixels, replacement);
}

void ProgressReportPercent(NativeProgressReport** report,
                           const long low, const long high,
                           const long value, long* next_value) {
    int64_t range;
    unsigned int increment;
    unsigned int partitions;
    int64_t offset;
    int64_t part;
    int64_t next_partition;
    int64_t numerator;

    if (report == NULL || *report == NULL || high <= low ||
        (*report)->nIncr == 0) {
        return;
    }

    range = (int64_t)high - (int64_t)low;
    increment = (*report)->nIncr;
    partitions = 100u / increment;
    if (partitions == 0) {
        return;
    }

    offset = (int64_t)value - (int64_t)low;
    part = (offset * partitions + range / 2) / range;
    part = ClampI64(part, 0, (int64_t)partitions);
    ChangeProgressReport(report, (unsigned int)part * increment);

    if (next_value != NULL) {
        next_partition = (int64_t)((*report)->nLast / increment) + 1;
        numerator = range * next_partition - range / 2 + partitions - 1;
        *next_value = (long)(numerator / partitions + low);
    }
}
