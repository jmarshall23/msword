#include "windows.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct EnumState {
    int count;
    bool saw_tms;
    bool saw_helv;
    bool saw_courier;
    bool saw_symbol;
};

static int parse_int(const char *text, int *value) {
    char *end = NULL;
    long parsed = strtol(text, &end, 10);
    if (end == text || *end != '\0') return 0;
    *value = (int)parsed;
    return 1;
}

static int parse_widths(char *text, int widths[95]) {
    int count = 0;
    char *save = NULL;
    for (char *token = strtok_r(text, ",", &save); token != NULL;
         token = strtok_r(NULL, ",", &save)) {
        if (count == 95 || !parse_int(token, &widths[count])) return 0;
        ++count;
    }
    return count == 95;
}

static int CALLBACK collect_font(const LOGFONTA *logical,
                                 const TEXTMETRICA *metric, DWORD type,
                                 LPARAM parameter) {
    (void)metric;
    (void)type;
    struct EnumState *state = (struct EnumState *)parameter;
    ++state->count;
    if (strcmp(logical->lfFaceName, "Tms Rmn") == 0) state->saw_tms = true;
    if (strcmp(logical->lfFaceName, "Helv") == 0) state->saw_helv = true;
    if (strcmp(logical->lfFaceName, "Courier") == 0) state->saw_courier = true;
    if (strcmp(logical->lfFaceName, "Symbol") == 0) state->saw_symbol = true;
    return 1;
}

static int CALLBACK stop_after_first(const LOGFONTA *logical,
                                     const TEXTMETRICA *metric, DWORD type,
                                     LPARAM parameter) {
    (void)logical;
    (void)metric;
    (void)type;
    ++*(int *)parameter;
    return 0;
}

static HFONT CreateTestFont(const char *face, int points) {
    LOGFONTA logical = {0};
    logical.lfHeight = -MulDiv(points, 96, 72);
    logical.lfCharSet = DEFAULT_CHARSET;
    logical.lfPitchAndFamily = DEFAULT_PITCH;
    strncpy(logical.lfFaceName, face, LF_FACESIZE - 1);
    return CreateFontIndirectA(&logical);
}

static bool SelectFace(HDC dc, const char *face, int points) {
    HFONT font = CreateTestFont(face, points);
    return font != NULL && SelectObject(dc, font) != NULL;
}

static bool CheckGoldenMetrics(HDC dc, const char *path) {
    FILE *input = fopen(path, "r");
    if (input == NULL) return false;

    char line[2048];
    if (fgets(line, sizeof(line), input) == NULL) {
        fclose(input);
        return false;
    }

    int rows = 0;
    while (fgets(line, sizeof(line), input) != NULL) {
        char *newline = strchr(line, '\n');
        if (newline != NULL) *newline = '\0';
        if (line[0] == '\0') continue;

        char *fields[6];
        char *save = NULL;
        char *token = strtok_r(line, "\t", &save);
        for (int index = 0; index < 6; ++index) {
            if (token == NULL) {
                fclose(input);
                return false;
            }
            fields[index] = token;
            token = strtok_r(NULL, "\t", &save);
        }
        if (token != NULL) {
            fclose(input);
            return false;
        }

        int widths[95];
        int points = 0;
        int ascent = 0;
        int descent = 0;
        int overhang = 0;
        if (!parse_int(fields[1], &points) || !parse_int(fields[2], &ascent) ||
            !parse_int(fields[3], &descent) ||
            !parse_int(fields[4], &overhang) ||
            !parse_widths(fields[5], widths)) {
            fclose(input);
            return false;
        }

        if (!SelectFace(dc, fields[0], points)) {
            fclose(input);
            return false;
        }
        TEXTMETRICA metric = {0};
        if (!GetTextMetricsA(dc, &metric) || metric.tmAscent != ascent ||
            metric.tmDescent != descent || metric.tmOverhang != overhang) {
            fclose(input);
            return false;
        }

        for (int ch = 32; ch < 127; ++ch) {
            const char text = (char)ch;
            SIZE size = {0};
            if (!GetTextExtentPoint32A(dc, &text, 1, &size)) {
                fclose(input);
                return false;
            }
            if (size.cx - metric.tmOverhang != widths[ch - 32]) {
                fclose(input);
                return false;
            }
        }
        ++rows;
    }

    fclose(input);
    return rows == 28;
}

int main(int argc, char **argv) {
    HDC dc = CreateCompatibleDC(NULL);
    if (dc == NULL) return 1;

    if (!SelectFace(dc, "Tms Rmn", 10)) return 2;

    const char text[] = "Metric additivity 123";
    SIZE full = {0};
    if (!GetTextExtentPoint32A(dc, text, (int)strlen(text), &full)) {
        return 3;
    }
    int singles = 0;
    for (const char *ch = text; *ch != '\0'; ++ch) {
        SIZE single = {0};
        if (!GetTextExtentPoint32A(dc, ch, 1, &single)) return 4;
        singles += single.cx;
    }
    if (full.cx != singles) return 5;

    TEXTMETRICA metric = {0};
    if (!GetTextMetricsA(dc, &metric)) return 6;
    if (metric.tmHeight != metric.tmAscent + metric.tmDescent ||
        metric.tmOverhang != 0 ||
        metric.tmHeight - metric.tmInternalLeading != MulDiv(10, 96, 72)) {
        return 7;
    }

    int widths[3] = {0};
    if (!GetCharWidthA(dc, 'A', 'C', widths) || widths[0] <= 0 ||
        widths[0] != widths[1]) {
        return 8;
    }

    if (!SelectFace(dc, "Courier", 10) || !GetTextMetricsA(dc, &metric)) {
        return 9;
    }
    if ((metric.tmPitchAndFamily & TMPF_FIXED_PITCH) != 0) return 10;

    const char *faces[] = {"Tms Rmn", "Helv", "Symbol"};
    for (size_t index = 0; index < sizeof(faces) / sizeof(faces[0]); ++index) {
        if (!SelectFace(dc, faces[index], 10) || !GetTextMetricsA(dc, &metric)) {
            return 11;
        }
        if ((metric.tmPitchAndFamily & TMPF_FIXED_PITCH) == 0) return 12;
    }

    struct EnumState state = {0};
    if (EnumFontsA(dc, NULL, collect_font, (LPARAM)&state) != 4) {
        return 13;
    }
    if (state.count != 4 || !state.saw_tms || !state.saw_helv ||
        !state.saw_courier || !state.saw_symbol) {
        return 14;
    }

    int stopped = 0;
    if (EnumFontsA(dc, NULL, stop_after_first, (LPARAM)&stopped) != 1 ||
        stopped != 1) {
        return 15;
    }

    LOGFONTA filter = {0};
    strcpy(filter.lfFaceName, "Courier");
    struct EnumState filtered = {0};
    if (EnumFontFamiliesExA(dc, &filter, collect_font, (LPARAM)&filtered, 0) !=
            1 ||
        !filtered.saw_courier) {
        return 16;
    }

    if (GetDeviceCaps(dc, LOGPIXELSX) != 96 ||
        GetDeviceCaps(dc, BITSPIXEL) != 32 ||
        (GetDeviceCaps(dc, RASTERCAPS) & RC_BITBLT) == 0 ||
        (GetDeviceCaps(dc, TEXTCAPS) & TC_SA_CONTIN) == 0) {
        return 17;
    }

    if (argc > 1 && !CheckGoldenMetrics(dc, argv[1])) return 18;

    DeleteDC(dc);
    return 0;
}
