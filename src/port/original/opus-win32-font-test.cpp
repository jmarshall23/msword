#include "windows.h"

#include <array>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

struct EnumState {
    int count = 0;
    bool saw_tms = false;
    bool saw_helv = false;
    bool saw_courier = false;
    bool saw_symbol = false;
};

int CALLBACK collect_font(const LOGFONTA* logical, const TEXTMETRICA*, DWORD,
                          LPARAM parameter) {
    auto* state = reinterpret_cast<EnumState*>(parameter);
    ++state->count;
    if (std::strcmp(logical->lfFaceName, "Tms Rmn") == 0) state->saw_tms = true;
    if (std::strcmp(logical->lfFaceName, "Helv") == 0) state->saw_helv = true;
    if (std::strcmp(logical->lfFaceName, "Courier") == 0) state->saw_courier = true;
    if (std::strcmp(logical->lfFaceName, "Symbol") == 0) state->saw_symbol = true;
    return 1;
}

int CALLBACK stop_after_first(const LOGFONTA*, const TEXTMETRICA*, DWORD,
                              LPARAM parameter) {
    ++*reinterpret_cast<int*>(parameter);
    return 0;
}

HFONT CreateTestFont(const char* face, int points) {
    LOGFONTA logical{};
    logical.lfHeight = -MulDiv(points, 96, 72);
    logical.lfCharSet = DEFAULT_CHARSET;
    logical.lfPitchAndFamily = DEFAULT_PITCH;
    std::strncpy(logical.lfFaceName, face, LF_FACESIZE - 1);
    return CreateFontIndirectA(&logical);
}

bool SelectFace(HDC dc, const char* face, int points) {
    HFONT font = CreateTestFont(face, points);
    return font != nullptr && SelectObject(dc, font) != nullptr;
}

std::vector<std::string> Split(const std::string& text, char delimiter) {
    std::vector<std::string> parts;
    std::stringstream stream(text);
    std::string part;
    while (std::getline(stream, part, delimiter)) parts.push_back(part);
    return parts;
}

bool CheckGoldenMetrics(HDC dc, const char* path) {
    std::ifstream input(path);
    if (!input) return false;

    std::string line;
    if (!std::getline(input, line)) return false;
    int rows = 0;
    while (std::getline(input, line)) {
        if (line.empty()) continue;
        const std::vector<std::string> fields = Split(line, '\t');
        if (fields.size() != 6) return false;
        const std::vector<std::string> widths = Split(fields[5], ',');
        if (widths.size() != 95) return false;

        const std::string& face = fields[0];
        const int points = std::stoi(fields[1]);
        const int ascent = std::stoi(fields[2]);
        const int descent = std::stoi(fields[3]);
        const int overhang = std::stoi(fields[4]);

        if (!SelectFace(dc, face.c_str(), points)) return false;
        TEXTMETRICA metric{};
        if (!GetTextMetricsA(dc, &metric) || metric.tmAscent != ascent ||
            metric.tmDescent != descent || metric.tmOverhang != overhang) {
            return false;
        }

        for (int ch = 32; ch < 127; ++ch) {
            const char text = static_cast<char>(ch);
            SIZE size{};
            if (!GetTextExtentPoint32A(dc, &text, 1, &size)) return false;
            if (size.cx - metric.tmOverhang != std::stoi(widths[ch - 32])) {
                return false;
            }
        }
        ++rows;
    }
    return rows == 28;
}

int main(int argc, char** argv) {
    HDC dc = CreateCompatibleDC(nullptr);
    if (dc == nullptr) return 1;

    if (!SelectFace(dc, "Tms Rmn", 10)) return 2;

    constexpr char text[] = "Metric additivity 123";
    SIZE full{};
    if (!GetTextExtentPoint32A(dc, text, static_cast<int>(std::strlen(text)),
                               &full)) {
        return 3;
    }
    int singles = 0;
    for (const char* ch = text; *ch != '\0'; ++ch) {
        SIZE single{};
        if (!GetTextExtentPoint32A(dc, ch, 1, &single)) return 4;
        singles += single.cx;
    }
    if (full.cx != singles) return 5;

    TEXTMETRICA metric{};
    if (!GetTextMetricsA(dc, &metric)) return 6;
    if (metric.tmHeight != metric.tmAscent + metric.tmDescent ||
        metric.tmOverhang != 0 ||
        metric.tmHeight - metric.tmInternalLeading != MulDiv(10, 96, 72)) {
        return 7;
    }

    std::array<int, 3> widths{};
    if (!GetCharWidthA(dc, 'A', 'C', widths.data()) || widths[0] <= 0 ||
        widths[0] != widths[1]) {
        return 8;
    }

    if (!SelectFace(dc, "Courier", 10) || !GetTextMetricsA(dc, &metric)) {
        return 9;
    }
    if ((metric.tmPitchAndFamily & TMPF_FIXED_PITCH) != 0) return 10;

    for (const char* face : {"Tms Rmn", "Helv", "Symbol"}) {
        if (!SelectFace(dc, face, 10) || !GetTextMetricsA(dc, &metric)) {
            return 11;
        }
        if ((metric.tmPitchAndFamily & TMPF_FIXED_PITCH) == 0) return 12;
    }

    EnumState state{};
    if (EnumFontsA(dc, nullptr, collect_font, reinterpret_cast<LPARAM>(&state)) != 4) {
        return 13;
    }
    if (state.count != 4 || !state.saw_tms || !state.saw_helv ||
        !state.saw_courier || !state.saw_symbol) {
        return 14;
    }

    int stopped = 0;
    if (EnumFontsA(dc, nullptr, stop_after_first,
                   reinterpret_cast<LPARAM>(&stopped)) != 1 ||
        stopped != 1) {
        return 15;
    }

    LOGFONTA filter{};
    std::strcpy(filter.lfFaceName, "Courier");
    EnumState filtered{};
    if (EnumFontFamiliesExA(dc, &filter, collect_font,
                            reinterpret_cast<LPARAM>(&filtered), 0) != 1 ||
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
