#define WIN32_LEAN_AND_MEAN
#include "opus_x64_compat.h"
#include <commdlg.h>
#include <msopc.h>
#include <richedit.h>
#include <wrl/client.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cctype>
#include <charconv>
#include <cmath>
#include <cwctype>
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iterator>
#include <limits>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

using Microsoft::WRL::ComPtr;

namespace {

constexpr std::size_t kMaxDocumentXmlBytes = 64u * 1024u * 1024u;
constexpr std::size_t kMaxStylesXmlBytes = 8u * 1024u * 1024u;
constexpr std::size_t kMaxRtfBytes = 64u * 1024u * 1024u;
constexpr std::size_t kMaxTextBytes = 32u * 1024u * 1024u;
constexpr std::size_t kMaxGeneratedBytes = 256u * 1024u * 1024u;
constexpr std::size_t kMaxStyles = 4096;
constexpr std::size_t kMaxParagraphs = 200000;
constexpr std::size_t kMaxRuns = 1000000;
constexpr std::size_t kMaxTableRows = 4096;
constexpr std::size_t kMaxTableColumns = 256;
constexpr std::size_t kMaxTableCells = 262144;
constexpr WCHAR kMsfteditModule[] = OPUSW("Msftedit.dll");
constexpr WCHAR kPdfFilter[] = OPUSW("PDF Files (*.pdf)\0*.pdf\0");
constexpr WCHAR kPdfDialogTitle[] = OPUSW("Export as PDF");
constexpr WCHAR kPdfDefaultExtension[] = OPUSW("pdf");
constexpr WCHAR kPdfTestPathEnv[] = OPUSW("WORD1_TEST_PDF_PATH");
constexpr WCHAR kPdfFailureMessage[] = OPUSW(
    "Word could not create the PDF. Check that the selected folder is writable.");

void require_parse_limit(const bool condition) {
    if (!condition) throw std::length_error("document exceeds import limits");
}

bool parse_bounded_int(std::string_view text, const int minimum,
                       const int maximum, int& result) {
    if (text.empty()) return false;
    int parsed = 0;
    const auto conversion = std::from_chars(
        text.data(), text.data() + text.size(), parsed, 10);
    if (conversion.ec != std::errc{} ||
        conversion.ptr != text.data() + text.size() ||
        parsed < minimum || parsed > maximum) {
        return false;
    }
    result = parsed;
    return true;
}

bool parse_rgb(std::string_view text, unsigned& result) {
    if (text.size() != 6) return false;
    unsigned parsed = 0;
    const auto conversion = std::from_chars(
        text.data(), text.data() + text.size(), parsed, 16);
    if (conversion.ec != std::errc{} ||
        conversion.ptr != text.data() + text.size() || parsed > 0xffffff) {
        return false;
    }
    result = parsed;
    return true;
}

struct ComApartment {
    HRESULT result = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    ~ComApartment() {
        if (result == S_OK || result == S_FALSE) {
            CoUninitialize();
        }
    }
    bool usable() const {
        return SUCCEEDED(result) || result == RPC_E_CHANGED_MODE;
    }
};

std::wstring wide_path(const char* path) {
    if (path == nullptr || *path == '\0') {
        return {};
    }
    const int count = MultiByteToWideChar(CP_ACP, 0, path, -1, nullptr, 0);
    std::wstring result(count > 0 ? static_cast<std::size_t>(count) : 0,
                        L'\0');
    if (count > 1) {
        MultiByteToWideChar(CP_ACP, 0, path, -1, result.data(), count);
        result.resize(static_cast<std::size_t>(count - 1));
    }
    return result;
}

std::wstring utf8_to_wide(std::string_view value) {
    if (value.empty()) {
        return {};
    }
    const int count = MultiByteToWideChar(CP_UTF8, 0, value.data(),
                                           static_cast<int>(value.size()),
                                           nullptr, 0);
    std::wstring result(count > 0 ? static_cast<std::size_t>(count) : 0,
                        L'\0');
    if (count > 0) {
        MultiByteToWideChar(CP_UTF8, 0, value.data(),
                            static_cast<int>(value.size()), result.data(),
                            count);
    }
    return result;
}

std::string wide_to_utf8(std::wstring_view value) {
    if (value.empty()) {
        return {};
    }
    const int count = WideCharToMultiByte(CP_UTF8, 0, value.data(),
                                          static_cast<int>(value.size()),
                                          nullptr, 0, nullptr, nullptr);
    std::string result(count > 0 ? static_cast<std::size_t>(count) : 0, '\0');
    if (count > 0) {
        WideCharToMultiByte(CP_UTF8, 0, value.data(),
                            static_cast<int>(value.size()), result.data(),
                            count, nullptr, nullptr);
    }
    return result;
}

std::string wide_to_ansi(std::wstring_view value) {
    if (value.empty()) return {};
    const int count = WideCharToMultiByte(
        1252, WC_NO_BEST_FIT_CHARS, value.data(),
        static_cast<int>(value.size()), nullptr, 0, "?", nullptr);
    std::string result(count > 0 ? static_cast<std::size_t>(count) : 0, '\0');
    if (count > 0) {
        WideCharToMultiByte(1252, WC_NO_BEST_FIT_CHARS, value.data(),
                            static_cast<int>(value.size()), result.data(),
                            count, "?", nullptr);
    }
    return result;
}

std::wstring ansi_to_wide(std::string_view value) {
    if (value.empty()) return {};
    const int count = MultiByteToWideChar(
        1252, 0, value.data(), static_cast<int>(value.size()), nullptr, 0);
    std::wstring result(count > 0 ? static_cast<std::size_t>(count) : 0,
                        L'\0');
    if (count > 0) {
        MultiByteToWideChar(1252, 0, value.data(),
                            static_cast<int>(value.size()), result.data(),
                            count);
    }
    return result;
}

std::string xml_escape(std::wstring_view text) {
    std::string result;
    for (std::size_t index = 0; index < text.size(); ++index) {
        const wchar_t character = text[index];
        switch (character) {
            case L'&': result += "&amp;"; break;
            case L'<': result += "&lt;"; break;
            case L'>': result += "&gt;"; break;
            case L'\"': result += "&quot;"; break;
            case L'\'': result += "&apos;"; break;
            default:
                if (character >= 0xd800 && character <= 0xdbff &&
                    index + 1 < text.size() && text[index + 1] >= 0xdc00 &&
                    text[index + 1] <= 0xdfff) {
                    result += wide_to_utf8(text.substr(index, 2));
                    ++index;
                } else if (!(character >= 0xd800 && character <= 0xdfff)) {
                    result += wide_to_utf8(text.substr(index, 1));
                } else {
                    result += "&#xFFFD;";
                }
        }
    }
    return result;
}

std::wstring xml_unescape(std::string_view text) {
    std::string decoded;
    for (std::size_t position = 0; position < text.size();) {
        if (text[position] != '&') {
            decoded.push_back(text[position++]);
            continue;
        }
        const std::size_t end = text.find(';', position + 1);
        if (end == std::string_view::npos) {
            decoded.push_back(text[position++]);
            continue;
        }
        const std::string entity(text.substr(position + 1, end - position - 1));
        if (entity == "amp") decoded.push_back('&');
        else if (entity == "lt") decoded.push_back('<');
        else if (entity == "gt") decoded.push_back('>');
        else if (entity == "quot") decoded.push_back('"');
        else if (entity == "apos") decoded.push_back('\'');
        else if (!entity.empty() && entity[0] == '#') {
            unsigned long code = 0;
            char* parse_end = nullptr;
            const char* digits = entity.c_str() + 1;
            int radix = 10;
            if (entity.size() > 2 && (entity[1] == 'x' || entity[1] == 'X')) {
                digits = entity.c_str() + 2;
                radix = 16;
            }
            code = std::strtoul(digits, &parse_end, radix);
            if (parse_end != digits && *parse_end == '\0' &&
                code <= 0x10ffff && !(code >= 0xd800 && code <= 0xdfff)) {
                if (code <= 0xffff) {
                    const wchar_t scalar = static_cast<wchar_t>(code);
                    decoded += wide_to_utf8(std::wstring_view(&scalar, 1));
                } else {
                    const unsigned long scalar = code - 0x10000;
                    const wchar_t pair[2] = {
                        static_cast<wchar_t>(0xd800 + (scalar >> 10)),
                        static_cast<wchar_t>(0xdc00 + (scalar & 0x3ff))};
                    decoded += wide_to_utf8(std::wstring_view(pair, 2));
                }
            }
        } else {
            decoded.append(text.substr(position, end - position + 1));
        }
        position = end + 1;
    }
    return utf8_to_wide(decoded);
}

bool has_extension(const std::string& path, const char* extension) {
    const std::size_t length = std::strlen(extension);
    return path.size() >= length &&
           _stricmp(path.c_str() + path.size() - length, extension) == 0;
}

bool safe_file_path_syntax(std::wstring_view path) {
    if (path.empty() || path.size() >= 32760 ||
        path.starts_with(LR"(\\.\)") ||
        path.starts_with(LR"(\\?\GLOBALROOT\)")) return false;
    for (std::size_t index = 0; index < path.size(); ++index) {
        if (path[index] == L':' && index != 1) return false;
    }
    return true;
}

std::vector<WCHAR> native_file_path(std::wstring_view path) {
    std::vector<WCHAR> result;
    result.reserve(path.size() + 1);
    for (const wchar_t code_unit : path) {
        result.push_back(
            static_cast<WCHAR>(static_cast<std::uint16_t>(code_unit)));
    }
    result.push_back(WCHAR{});
    return result;
}

bool regular_file_within_limit(const std::wstring& path,
                               const std::size_t maximum_size) {
    if (!safe_file_path_syntax(path)) return false;
    const std::vector<WCHAR> native_path = native_file_path(path);
    WIN32_FILE_ATTRIBUTE_DATA attributes{};
    if (!GetFileAttributesExW(native_path.data(), GetFileExInfoStandard,
                              &attributes) ||
        (attributes.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
        return false;
    }
    const ULONGLONG size =
        (static_cast<ULONGLONG>(attributes.nFileSizeHigh) << 32) |
        attributes.nFileSizeLow;
    return size <= maximum_size;
}

bool read_stream(IStream* stream, std::string& data,
                 const std::size_t maximum_size) {
    STATSTG status{};
    if (stream == nullptr || FAILED(stream->Stat(&status, STATFLAG_NONAME)) ||
        status.cbSize.QuadPart > static_cast<ULONGLONG>(maximum_size) ||
        maximum_size > MAXDWORD) {
        return false;
    }
    data.resize(static_cast<std::size_t>(status.cbSize.QuadPart));
    LARGE_INTEGER zero{};
    stream->Seek(zero, STREAM_SEEK_SET, nullptr);
    ULONG read = 0;
    return data.empty() ||
           (SUCCEEDED(stream->Read(data.data(), static_cast<ULONG>(data.size()),
                                   &read)) && read == data.size());
}

bool read_opc_part(const std::wstring& path, const WCHAR* part_name,
                   std::string& data, const std::size_t maximum_size) {
    if (!regular_file_within_limit(path, kMaxGeneratedBytes)) return false;
    ComApartment apartment;
    if (!apartment.usable()) return false;
    ComPtr<IOpcFactory> factory;
    ComPtr<IStream> file;
    ComPtr<IOpcPackage> package;
    ComPtr<IOpcPartSet> parts;
    ComPtr<IOpcPartUri> uri;
    ComPtr<IOpcPart> part;
    ComPtr<IStream> content;
    const std::vector<WCHAR> native_path = native_file_path(path);
    return SUCCEEDED(CoCreateInstance(__uuidof(OpcFactory), nullptr,
                                      CLSCTX_INPROC_SERVER,
                                      IID_PPV_ARGS(&factory))) &&
           SUCCEEDED(factory->CreateStreamOnFile(native_path.data(),
                                                 OPC_STREAM_IO_READ, nullptr,
                                                 FILE_ATTRIBUTE_NORMAL,
                                                 &file)) &&
           SUCCEEDED(factory->ReadPackageFromStream(file.Get(),
                                                    OPC_READ_DEFAULT,
                                                    &package)) &&
           SUCCEEDED(package->GetPartSet(&parts)) &&
           SUCCEEDED(factory->CreatePartUri(part_name, &uri)) &&
           SUCCEEDED(parts->GetPart(uri.Get(), &part)) &&
           SUCCEEDED(part->GetContentStream(&content)) &&
           read_stream(content.Get(), data, maximum_size);
}

std::string tag_attribute(std::string_view tag, std::string_view name) {
    std::size_t position = 0;
    while ((position = tag.find(name, position)) != std::string_view::npos) {
        const bool boundary = position == 0 ||
            std::isspace(static_cast<unsigned char>(tag[position - 1])) ||
            tag[position - 1] == ':';
        std::size_t equals = position + name.size();
        while (equals < tag.size() && std::isspace(
                   static_cast<unsigned char>(tag[equals]))) ++equals;
        if (!boundary || equals >= tag.size() || tag[equals] != '=') {
            position += name.size();
            continue;
        }
        ++equals;
        while (equals < tag.size() && std::isspace(
                   static_cast<unsigned char>(tag[equals]))) ++equals;
        if (equals >= tag.size() || (tag[equals] != '\'' && tag[equals] != '"'))
            return {};
        const char quote = tag[equals++];
        const std::size_t end = tag.find(quote, equals);
        return end == std::string_view::npos ? std::string{} :
            std::string(tag.substr(equals, end - equals));
    }
    return {};
}

std::string local_tag_name(std::string_view tag) {
    std::size_t start = 0;
    while (start < tag.size() && (tag[start] == '<' || tag[start] == '/' ||
           std::isspace(static_cast<unsigned char>(tag[start])))) ++start;
    std::size_t end = start;
    while (end < tag.size() && !std::isspace(static_cast<unsigned char>(tag[end])) &&
           tag[end] != '/' && tag[end] != '>') ++end;
    const std::size_t colon = tag.substr(start, end - start).rfind(':');
    if (colon != std::string_view::npos) start += colon + 1;
    return std::string(tag.substr(start, end - start));
}

struct RunStyle {
    bool bold = false;
    bool italic = false;
    bool underline = false;
    bool strike = false;
    bool small_caps = false;
    bool all_caps = false;
    bool hidden = false;
    int half_points = 20;
    COLORREF color = RGB(0, 0, 0);
    bool auto_color = true;
    std::wstring font = L"Arial";
    std::string language = "en-US";
    int code_page = 1252;
    int charset = ANSI_CHARSET;
    bool operator==(const RunStyle&) const = default;
};

struct LanguageProfile {
    const char* tag;
    int code_page;
    int charset;
    LANGID language_id;
};

constexpr LanguageProfile kLanguageProfiles[] = {
    {"en-US", 1252, ANSI_CHARSET, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US)},
    {"es-ES", 1252, ANSI_CHARSET, MAKELANGID(LANG_SPANISH, SUBLANG_SPANISH_MODERN)},
    {"fr-FR", 1252, ANSI_CHARSET, MAKELANGID(LANG_FRENCH, SUBLANG_FRENCH)},
    {"de-DE", 1252, ANSI_CHARSET, MAKELANGID(LANG_GERMAN, SUBLANG_GERMAN)},
    {"pl-PL", 1250, EASTEUROPE_CHARSET, MAKELANGID(LANG_POLISH, SUBLANG_DEFAULT)},
    {"el-GR", 1253, GREEK_CHARSET, MAKELANGID(LANG_GREEK, SUBLANG_DEFAULT)},
    {"ru-RU", 1251, RUSSIAN_CHARSET, MAKELANGID(LANG_RUSSIAN, SUBLANG_DEFAULT)},
    {"tr-TR", 1254, TURKISH_CHARSET, MAKELANGID(LANG_TURKISH, SUBLANG_DEFAULT)},
    {"he-IL", 1255, HEBREW_CHARSET, MAKELANGID(LANG_HEBREW, SUBLANG_DEFAULT)},
    {"ar-SA", 1256, ARABIC_CHARSET, MAKELANGID(LANG_ARABIC, SUBLANG_ARABIC_SAUDI_ARABIA)},
    {"th-TH", 874, THAI_CHARSET, MAKELANGID(LANG_THAI, SUBLANG_DEFAULT)},
    {"vi-VN", 1258, VIETNAMESE_CHARSET, MAKELANGID(LANG_VIETNAMESE, SUBLANG_DEFAULT)},
    {"ja-JP", 932, SHIFTJIS_CHARSET, MAKELANGID(LANG_JAPANESE, SUBLANG_DEFAULT)},
    {"zh-CN", 936, GB2312_CHARSET, MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED)},
    {"zh-TW", 950, CHINESEBIG5_CHARSET, MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL)},
    {"ko-KR", 949, HANGEUL_CHARSET, MAKELANGID(LANG_KOREAN, SUBLANG_DEFAULT)},
};

const LanguageProfile& default_language_profile() {
    return kLanguageProfiles[0];
}

bool tag_prefix(std::string_view value, std::string_view prefix) {
    if (value.size() < prefix.size()) return false;
    for (std::size_t index = 0; index < prefix.size(); ++index) {
        const unsigned char left = static_cast<unsigned char>(value[index]);
        const unsigned char right = static_cast<unsigned char>(prefix[index]);
        if (std::tolower(left) != std::tolower(right)) return false;
    }
    return value.size() == prefix.size() || value[prefix.size()] == '-';
}

const LanguageProfile& profile_for_tag(std::string_view tag) {
    for (const LanguageProfile& profile : kLanguageProfiles) {
        if (_stricmp(std::string(tag).c_str(), profile.tag) == 0)
            return profile;
    }
    struct Alias { const char* prefix; std::size_t profile; };
    static constexpr Alias aliases[] = {
        {"en", 0}, {"es", 1}, {"fr", 2}, {"de", 3}, {"pl", 4},
        {"cs", 4}, {"sk", 4}, {"hu", 4}, {"ro", 4}, {"el", 5},
        {"ru", 6}, {"uk", 6}, {"bg", 6}, {"sr", 6}, {"tr", 7},
        {"he", 8}, {"iw", 8}, {"ar", 9}, {"fa", 9}, {"ur", 9},
        {"th", 10}, {"vi", 11}, {"ja", 12}, {"zh-CN", 13},
        {"zh-SG", 13}, {"zh", 14}, {"ko", 15},
    };
    for (const Alias& alias : aliases) {
        if (tag_prefix(tag, alias.prefix)) return kLanguageProfiles[alias.profile];
    }
    return default_language_profile();
}

const LanguageProfile& profile_for_language_id(const LANGID language) {
    const WORD primary = PRIMARYLANGID(language);
    for (const LanguageProfile& profile : kLanguageProfiles) {
        if (PRIMARYLANGID(profile.language_id) == primary) return profile;
    }
    return default_language_profile();
}

const LanguageProfile& profile_for_scalar(const std::uint32_t scalar) {
    if (scalar >= 0x0370 && scalar <= 0x03ff) return kLanguageProfiles[5];
    if (scalar >= 0x0400 && scalar <= 0x052f) return kLanguageProfiles[6];
    if (scalar >= 0x0590 && scalar <= 0x05ff) return kLanguageProfiles[8];
    if ((scalar >= 0x0600 && scalar <= 0x08ff) ||
        (scalar >= 0xfb50 && scalar <= 0xfdff) ||
        (scalar >= 0xfe70 && scalar <= 0xfeff)) return kLanguageProfiles[9];
    if (scalar >= 0x0e00 && scalar <= 0x0e7f) return kLanguageProfiles[10];
    if ((scalar >= 0x3040 && scalar <= 0x30ff) ||
        (scalar >= 0x31f0 && scalar <= 0x31ff)) return kLanguageProfiles[12];
    if ((scalar >= 0x3400 && scalar <= 0x9fff) ||
        (scalar >= 0xf900 && scalar <= 0xfaff)) return kLanguageProfiles[13];
    if ((scalar >= 0x1100 && scalar <= 0x11ff) ||
        (scalar >= 0x3130 && scalar <= 0x318f) ||
        (scalar >= 0xac00 && scalar <= 0xd7af)) return kLanguageProfiles[15];
    return default_language_profile();
}

std::uint32_t scalar_at(std::wstring_view text, std::size_t& index) {
    const std::uint32_t first = static_cast<std::uint16_t>(text[index++]);
    if (first >= 0xd800 && first <= 0xdbff && index < text.size()) {
        const std::uint32_t second = static_cast<std::uint16_t>(text[index]);
        if (second >= 0xdc00 && second <= 0xdfff) {
            ++index;
            return 0x10000 + ((first - 0xd800) << 10) + (second - 0xdc00);
        }
    }
    if (first >= 0xd800 && first <= 0xdfff) return 0xfffd;
    return first;
}

std::wstring scalar_to_wide(const std::uint32_t scalar) {
    if (scalar <= 0xffff && !(scalar >= 0xd800 && scalar <= 0xdfff))
        return std::wstring(1, static_cast<wchar_t>(scalar));
    if (scalar <= 0x10ffff) {
        const std::uint32_t value = scalar - 0x10000;
        const wchar_t pair[] = {
            static_cast<wchar_t>(0xd800 + (value >> 10)),
            static_cast<wchar_t>(0xdc00 + (value & 0x3ff))};
        return std::wstring(pair, 2);
    }
    return std::wstring(1, L'\xfffd');
}

char legacy_byte_for_scalar(const std::uint32_t scalar,
                            const LanguageProfile& profile) {
    if (scalar == 0) return '?';
    const std::wstring wide = scalar_to_wide(scalar);
    char bytes[4]{};
    BOOL used_default = FALSE;
    const int count = WideCharToMultiByte(
        profile.code_page, WC_NO_BEST_FIT_CHARS, wide.data(),
        static_cast<int>(wide.size()), bytes, static_cast<int>(sizeof(bytes)),
        "?", &used_default);
    if (count == 1 && !used_default && bytes[0] != '\0') return bytes[0];
    const bool wide_script = profile.code_page == 932 ||
        profile.code_page == 936 || profile.code_page == 949 ||
        profile.code_page == 950 || scalar > 0xffff;
    return wide_script ? 'W' : '?';
}

struct TextRun { RunStyle style; std::wstring text; };
struct DocumentSettings {
    bool valid = false;
    int page_width = 0;
    int page_height = 0;
    int margin_left = 0;
    int margin_right = 0;
    int margin_top = 0;
    int margin_bottom = 0;
};
struct Paragraph {
    int alignment = PFA_LEFT;
    int left_indent = 0;
    int right_indent = 0;
    int first_line_indent = 0;
    int space_before = 0;
    int space_after = 0;
    int line_spacing = 0;
    bool keep_together = false;
    bool keep_with_next = false;
    bool page_break_before = false;
    bool bottom_border = false;
    std::vector<TextRun> runs;
};

std::string_view element_block(std::string_view xml,
                               std::string_view local_name) {
    const std::string opening = "<w:" + std::string(local_name);
    std::size_t start = 0;
    while ((start = xml.find(opening, start)) != std::string_view::npos) {
        const std::size_t boundary = start + opening.size();
        if (boundary < xml.size() &&
            (xml[boundary] == '>' || xml[boundary] == '/' ||
             std::isspace(static_cast<unsigned char>(xml[boundary])))) {
            const std::size_t open_end = xml.find('>', boundary);
            if (open_end == std::string_view::npos) return {};
            if (open_end > start && xml[open_end - 1] == '/')
                return xml.substr(start, open_end - start + 1);
            const std::string closing = "</w:" + std::string(local_name) + ">";
            const std::size_t close = xml.find(closing, open_end + 1);
            if (close == std::string_view::npos) return {};
            return xml.substr(start, close + closing.size() - start);
        }
        start = boundary;
    }
    return {};
}

int property_state(std::string_view properties, const char* local_name) {
    std::size_t position = 0;
    while ((position = properties.find('<', position)) != std::string_view::npos) {
        const std::size_t end = properties.find('>', position + 1);
        if (end == std::string_view::npos) break;
        const std::string_view tag = properties.substr(position, end - position + 1);
        if (local_tag_name(tag) == local_name) {
            const std::string value = tag_attribute(tag, "val");
            return value == "0" || value == "false" || value == "none" ? 0 : 1;
        }
        position = end + 1;
    }
    return -1;
}

bool property_enabled(std::string_view properties, const char* local_name) {
    return property_state(properties, local_name) == 1;
}

std::string first_property_value(std::string_view properties,
                                 const char* local_name,
                                 const char* attribute = "val") {
    std::size_t position = 0;
    while ((position = properties.find('<', position)) != std::string_view::npos) {
        const std::size_t end = properties.find('>', position + 1);
        if (end == std::string_view::npos) break;
        const std::string_view tag = properties.substr(position, end - position + 1);
        if (local_tag_name(tag) == local_name) return tag_attribute(tag, attribute);
        position = end + 1;
    }
    return {};
}

void apply_run_properties(RunStyle& style, std::string_view props) {
    const auto apply_switch = [&](const char* name, bool& target) {
        const int state = property_state(props, name);
        if (state >= 0) target = state != 0;
    };
    apply_switch("b", style.bold);
    apply_switch("i", style.italic);
    apply_switch("u", style.underline);
    apply_switch("strike", style.strike);
    apply_switch("smallCaps", style.small_caps);
    apply_switch("caps", style.all_caps);
    apply_switch("vanish", style.hidden);
    if (const std::string size = first_property_value(props, "sz"); !size.empty()) {
        parse_bounded_int(size, 2, 254, style.half_points);
    }
    std::string font = first_property_value(props, "rFonts", "ascii");
    if (font.empty()) font = first_property_value(props, "rFonts", "hAnsi");
    if (font.empty()) font = first_property_value(props, "rFonts", "eastAsia");
    if (font.empty()) font = first_property_value(props, "rFonts", "cs");
    if (!font.empty()) {
        style.font = xml_unescape(font);
    }
    std::string language = first_property_value(props, "lang", "val");
    if (language.empty())
        language = first_property_value(props, "lang", "eastAsia");
    if (language.empty()) language = first_property_value(props, "lang", "bidi");
    if (!language.empty()) {
        const LanguageProfile& profile = profile_for_tag(language);
        style.language = language;
        style.code_page = profile.code_page;
        style.charset = profile.charset;
    }
    if (const std::string color = first_property_value(props, "color");
        !color.empty() && color != "auto" && color.size() == 6) {
        unsigned rgb = 0;
        if (parse_rgb(color, rgb)) {
            style.color = RGB((rgb >> 16) & 0xff,
                              (rgb >> 8) & 0xff, rgb & 0xff);
            style.auto_color = false;
        }
    } else if (first_property_value(props, "color") == "auto") {
        style.auto_color = true;
        style.color = RGB(0, 0, 0);
    }
}

void apply_paragraph_properties(Paragraph& paragraph, std::string_view props) {
    if (const std::string alignment = first_property_value(props, "jc");
        !alignment.empty()) {
        if (alignment == "center") paragraph.alignment = PFA_CENTER;
        else if (alignment == "right") paragraph.alignment = PFA_RIGHT;
        else if (alignment == "both" || alignment == "distribute")
            paragraph.alignment = PFA_JUSTIFY;
        else paragraph.alignment = PFA_LEFT;
    }
    if (const std::string_view indent = element_block(props, "ind"); !indent.empty()) {
        const std::string left = tag_attribute(indent, "left");
        const std::string right = tag_attribute(indent, "right");
        const std::string first = tag_attribute(indent, "firstLine");
        const std::string hanging = tag_attribute(indent, "hanging");
        parse_bounded_int(left, -31680, 31680, paragraph.left_indent);
        parse_bounded_int(right, -31680, 31680, paragraph.right_indent);
        parse_bounded_int(first, -31680, 31680,
                          paragraph.first_line_indent);
        int hanging_indent = 0;
        if (parse_bounded_int(hanging, 0, 31680, hanging_indent))
            paragraph.first_line_indent = -hanging_indent;
    }
    if (const std::string_view spacing = element_block(props, "spacing");
        !spacing.empty()) {
        const std::string before = tag_attribute(spacing, "before");
        const std::string after = tag_attribute(spacing, "after");
        const std::string line = tag_attribute(spacing, "line");
        parse_bounded_int(before, 0, 31680, paragraph.space_before);
        parse_bounded_int(after, 0, 31680, paragraph.space_after);
        parse_bounded_int(line, 0, 31680, paragraph.line_spacing);
    }
    const auto apply_switch = [&](const char* name, bool& target) {
        const int state = property_state(props, name);
        if (state >= 0) target = state != 0;
    };
    apply_switch("keepLines", paragraph.keep_together);
    apply_switch("keepNext", paragraph.keep_with_next);
    apply_switch("pageBreakBefore", paragraph.page_break_before);
    if (const std::string_view borders = element_block(props, "pBdr");
        !borders.empty()) {
        const std::string_view bottom = element_block(borders, "bottom");
        const std::string value = tag_attribute(bottom, "val");
        paragraph.bottom_border = !bottom.empty() && value != "nil" &&
                                  value != "none";
    }
}

struct StyleDefinition {
    std::string based_on;
    std::string run_properties;
    std::string paragraph_properties;
};

struct StyleCatalog {
    RunStyle default_run;
    Paragraph default_paragraph;
    std::map<std::string, StyleDefinition> definitions;
};

StyleCatalog parse_style_catalog(std::string_view xml) {
    StyleCatalog catalog;
    if (const std::string_view defaults = element_block(xml, "docDefaults");
        !defaults.empty()) {
        const std::string_view run_default = element_block(defaults, "rPrDefault");
        apply_run_properties(catalog.default_run,
                             element_block(run_default, "rPr"));
        const std::string_view paragraph_default =
            element_block(defaults, "pPrDefault");
        apply_paragraph_properties(catalog.default_paragraph,
                                   element_block(paragraph_default, "pPr"));
    }
    std::size_t position = 0;
    while ((position = xml.find("<w:style", position)) != std::string_view::npos) {
        const std::size_t boundary = position + 8;
        if (boundary >= xml.size() ||
            (!std::isspace(static_cast<unsigned char>(xml[boundary])) &&
             xml[boundary] != '>')) {
            position = boundary;
            continue;
        }
        const std::size_t open_end = xml.find('>', boundary);
        const std::size_t close = xml.find("</w:style>", open_end);
        if (open_end == std::string_view::npos || close == std::string_view::npos)
            break;
        const std::string_view opening = xml.substr(position, open_end - position + 1);
        const std::string id = tag_attribute(opening, "styleId");
        const std::string_view block = xml.substr(position, close + 10 - position);
        if (!id.empty()) {
            require_parse_limit(catalog.definitions.size() < kMaxStyles);
            StyleDefinition definition;
            definition.based_on = first_property_value(block, "basedOn");
            if (const std::string_view props = element_block(block, "rPr");
                !props.empty()) definition.run_properties.assign(props);
            if (const std::string_view props = element_block(block, "pPr");
                !props.empty()) definition.paragraph_properties.assign(props);
            catalog.definitions[id] = std::move(definition);
        }
        position = close + 10;
    }
    return catalog;
}

void apply_style(const StyleCatalog& catalog, const std::string& id,
                 RunStyle& run, Paragraph& paragraph, const int depth = 0) {
    if (id.empty() || depth > 16) return;
    const auto found = catalog.definitions.find(id);
    if (found == catalog.definitions.end()) return;
    if (!found->second.based_on.empty() && found->second.based_on != id) {
        apply_style(catalog, found->second.based_on, run, paragraph, depth + 1);
    }
    apply_run_properties(run, found->second.run_properties);
    apply_paragraph_properties(paragraph, found->second.paragraph_properties);
}

RunStyle parse_run_style(std::string_view run, RunStyle style,
                         const StyleCatalog& catalog) {
    const std::string_view props = element_block(run, "rPr");
    if (const std::string character_style =
            first_property_value(props, "rStyle"); !character_style.empty()) {
        Paragraph ignored;
        apply_style(catalog, character_style, style, ignored);
    }
    apply_run_properties(style, props);
    return style;
}

std::wstring parse_run_text(std::string_view run) {
    std::wstring text;
    std::size_t position = 0;
    while ((position = run.find('<', position)) != std::string_view::npos) {
        const std::size_t tag_end = run.find('>', position + 1);
        if (tag_end == std::string_view::npos) break;
        const std::string_view tag = run.substr(position, tag_end - position + 1);
        const std::string name = local_tag_name(tag);
        if (name == "t") {
            const std::size_t close = run.find("</w:t>", tag_end + 1);
            if (close == std::string_view::npos) break;
            text += xml_unescape(run.substr(tag_end + 1, close - tag_end - 1));
            require_parse_limit(text.size() <= kMaxTextBytes);
            position = close + 6;
        } else {
            if (name == "tab") text.push_back(L'\t');
            else if (name == "br") {
                text.push_back(tag_attribute(tag, "type") == "page" ?
                                   L'\f' : L'\n');
            } else if (name == "cr") text.push_back(L'\n');
            else if (name == "noBreakHyphen") text.push_back(L'-');
            else if (name == "softHyphen") text.push_back(L'\x00ad');
            position = tag_end + 1;
        }
    }
    return text;
}

std::vector<Paragraph> parse_paragraphs_flat(
    std::string_view xml, const StyleCatalog& catalog,
    std::size_t& total_paragraphs, std::size_t& total_runs) {
    std::vector<Paragraph> paragraphs;
    std::size_t position = 0;
    while ((position = xml.find("<w:p", position)) != std::string_view::npos) {
        const char next = position + 4 < xml.size() ? xml[position + 4] : '\0';
        if (next != '>' && next != '/' && !std::isspace(static_cast<unsigned char>(next))) {
            position += 4;
            continue;
        }
        const std::size_t open_end = xml.find('>', position);
        const std::size_t close = xml.find("</w:p>", open_end);
        if (open_end == std::string_view::npos || close == std::string_view::npos) break;
        const std::string_view block = xml.substr(open_end + 1, close - open_end - 1);
        Paragraph paragraph = catalog.default_paragraph;
        RunStyle paragraph_run = catalog.default_run;
        apply_style(catalog, "Normal", paragraph_run, paragraph);
        const std::string_view paragraph_properties = element_block(block, "pPr");
        if (const std::string paragraph_style =
                first_property_value(paragraph_properties, "pStyle");
            !paragraph_style.empty() && paragraph_style != "Normal") {
            apply_style(catalog, paragraph_style, paragraph_run, paragraph);
        }
        apply_paragraph_properties(paragraph, paragraph_properties);

        std::size_t run_position = 0;
        while ((run_position = block.find("<w:r", run_position)) != std::string_view::npos) {
            const char run_next = run_position + 4 < block.size() ? block[run_position + 4] : '\0';
            if (run_next != '>' && run_next != '/' &&
                !std::isspace(static_cast<unsigned char>(run_next))) {
                run_position += 4;
                continue;
            }
            const std::size_t run_open_end = block.find('>', run_position);
            const std::size_t run_close = block.find("</w:r>", run_open_end);
            if (run_open_end == std::string_view::npos || run_close == std::string_view::npos)
                break;
            const std::string_view run = block.substr(
                run_position, run_close + 6 - run_position);
            std::wstring text = parse_run_text(run);
            if (!text.empty()) {
                require_parse_limit(total_runs < kMaxRuns);
                paragraph.runs.push_back(
                    {parse_run_style(run, paragraph_run, catalog),
                     std::move(text)});
                ++total_runs;
            }
            run_position = run_close + 6;
        }
        require_parse_limit(total_paragraphs < kMaxParagraphs);
        paragraphs.push_back(std::move(paragraph));
        ++total_paragraphs;
        position = close + 6;
    }
    if (paragraphs.empty()) paragraphs.push_back({});
    return paragraphs;
}

struct TableLayout {
    std::size_t first_paragraph = 0;
    std::size_t paragraph_count = 0;
    int rows = 0;
    int columns = 0;
};

std::size_t find_word_tag(std::string_view xml, std::string_view name,
                          std::size_t position) {
    const std::string opening = "<w:" + std::string(name);
    while ((position = xml.find(opening, position)) != std::string_view::npos) {
        const std::size_t boundary = position + opening.size();
        if (boundary < xml.size() &&
            (xml[boundary] == '>' || xml[boundary] == '/' ||
             std::isspace(static_cast<unsigned char>(xml[boundary])))) {
            return position;
        }
        position = boundary;
    }
    return std::string_view::npos;
}

std::vector<Paragraph> parse_document_xml(
    std::string_view xml, const StyleCatalog& catalog = {},
    std::vector<TableLayout>* table_layouts = nullptr) {
    std::vector<Paragraph> paragraphs;
    if (table_layouts != nullptr) table_layouts->clear();
    std::size_t total_paragraphs = 0;
    std::size_t total_runs = 0;
    std::size_t cursor = 0;
    while (cursor < xml.size()) {
        const std::size_t table_start = find_word_tag(xml, "tbl", cursor);
        if (table_start == std::string_view::npos) {
            const std::string_view remainder = xml.substr(cursor);
            if (find_word_tag(remainder, "p", 0) != std::string_view::npos) {
                std::vector<Paragraph> tail =
                    parse_paragraphs_flat(remainder, catalog,
                                          total_paragraphs, total_runs);
                paragraphs.insert(paragraphs.end(),
                                  std::make_move_iterator(tail.begin()),
                                  std::make_move_iterator(tail.end()));
                require_parse_limit(paragraphs.size() <= kMaxParagraphs);
            }
            break;
        }
        const std::string_view prefix =
            xml.substr(cursor, table_start - cursor);
        if (find_word_tag(prefix, "p", 0) != std::string_view::npos) {
            std::vector<Paragraph> before =
                parse_paragraphs_flat(prefix, catalog,
                                      total_paragraphs, total_runs);
            paragraphs.insert(paragraphs.end(),
                              std::make_move_iterator(before.begin()),
                              std::make_move_iterator(before.end()));
            require_parse_limit(paragraphs.size() <= kMaxParagraphs);
        }
        const std::size_t table_close = xml.find("</w:tbl>", table_start);
        if (table_close == std::string_view::npos) break;
        const std::size_t table_end = table_close + std::strlen("</w:tbl>");
        const std::string_view table =
            xml.substr(table_start, table_end - table_start);
        TableLayout layout;
        layout.first_paragraph = paragraphs.size();
        std::size_t row_cursor = 0;
        std::size_t table_cell_count = 0;
        while (true) {
            const std::size_t row_start = find_word_tag(table, "tr", row_cursor);
            if (row_start == std::string_view::npos) break;
            const std::size_t row_close = table.find("</w:tr>", row_start);
            if (row_close == std::string_view::npos) break;
            const std::size_t row_end = row_close + std::strlen("</w:tr>");
            require_parse_limit(static_cast<std::size_t>(layout.rows) <
                                kMaxTableRows);
            const std::string_view row = table.substr(row_start,
                                                       row_end - row_start);
            std::vector<std::vector<Paragraph>> cells;
            std::size_t cell_cursor = 0;
            while (true) {
                const std::size_t cell_start = find_word_tag(row, "tc", cell_cursor);
                if (cell_start == std::string_view::npos) break;
                const std::size_t cell_close = row.find("</w:tc>", cell_start);
                if (cell_close == std::string_view::npos) break;
                const std::size_t cell_end = cell_close + std::strlen("</w:tc>");
                require_parse_limit(cells.size() < kMaxTableColumns &&
                                    table_cell_count < kMaxTableCells);
                cells.push_back(parse_paragraphs_flat(
                    row.substr(cell_start, cell_end - cell_start), catalog,
                    total_paragraphs, total_runs));
                ++table_cell_count;
                cell_cursor = cell_end;
            }
            std::size_t line_count = 0;
            for (const auto& cell : cells)
                line_count = (std::max)(line_count, cell.size());
            if (!cells.empty()) {
                layout.columns = (std::max)(layout.columns,
                                            static_cast<int>(cells.size()));
                for (std::size_t line = 0; line < line_count; ++line) {
                    for (auto& cell : cells) {
                        if (line < cell.size())
                            paragraphs.push_back(std::move(cell[line]));
                        else
                            paragraphs.push_back({});
                        require_parse_limit(paragraphs.size() <=
                                            kMaxParagraphs);
                    }
                    ++layout.rows;
                }
            }
            row_cursor = row_end;
        }
        layout.paragraph_count = paragraphs.size() - layout.first_paragraph;
        if (layout.rows > 0 && layout.columns > 1 &&
            layout.paragraph_count ==
                static_cast<std::size_t>(layout.rows * layout.columns)) {
            if (table_layouts != nullptr) table_layouts->push_back(layout);
        } else {
            paragraphs.resize(layout.first_paragraph);
            std::vector<Paragraph> flat = parse_paragraphs_flat(
                table, catalog, total_paragraphs, total_runs);
            paragraphs.insert(paragraphs.end(),
                              std::make_move_iterator(flat.begin()),
                              std::make_move_iterator(flat.end()));
            require_parse_limit(paragraphs.size() <= kMaxParagraphs);
        }
        cursor = table_end;
    }
    if (paragraphs.empty()) paragraphs.push_back({});
    return paragraphs;
}

DocumentSettings parse_document_settings(std::string_view xml) {
    DocumentSettings settings;
    const std::string_view section = element_block(xml, "sectPr");
    const std::string_view size = element_block(section, "pgSz");
    const std::string_view margins = element_block(section, "pgMar");
    const auto integer_attribute = [](std::string_view tag, const char* name,
                                      const int minimum, const int maximum) {
        const std::string value = tag_attribute(tag, name);
        int result = 0;
        parse_bounded_int(value, minimum, maximum, result);
        return result;
    };
    settings.page_width = integer_attribute(size, "w", 720, 63360);
    settings.page_height = integer_attribute(size, "h", 720, 63360);
    settings.margin_left = integer_attribute(margins, "left", 0, 31680);
    settings.margin_right = integer_attribute(margins, "right", 0, 31680);
    settings.margin_top = integer_attribute(margins, "top", 0, 31680);
    settings.margin_bottom = integer_attribute(margins, "bottom", 0, 31680);
    settings.valid = settings.page_width > 0 && settings.page_height > 0 &&
        settings.margin_left + settings.margin_right < settings.page_width &&
        settings.margin_top + settings.margin_bottom < settings.page_height;
    return settings;
}

struct PendingRunFormat {
    long cp_first = 0;
    long cp_lim = 0;
    RunStyle style;
};

struct PendingParagraphFormat {
    long cp_first = 0;
    long cp_lim = 0;
    Paragraph paragraph;
};

struct PendingTableFormat {
    long cp_first = 0;
    long cp_lim = 0;
    int first_paragraph = 0;
    int rows = 0;
    int columns = 0;
};

struct UnicodeCell {
    std::uint32_t scalar = 0;
    LANGID language = 0;
};

struct UnicodeDocument {
    std::vector<UnicodeCell> cells;
};

struct PendingDocxImport {
    std::vector<PendingRunFormat> runs;
    std::vector<PendingParagraphFormat> paragraphs;
    std::vector<PendingTableFormat> tables;
    std::vector<UnicodeCell> unicode_cells;
    DocumentSettings settings;
};

PendingDocxImport pending_docx_import;
std::unordered_map<int, UnicodeDocument> unicode_documents;
std::vector<UnicodeCell> pending_clipboard_cells;
std::string input_language = "auto";

struct PendingPdfExport {
    std::vector<Paragraph> paragraphs;
    DocumentSettings settings;
    std::size_t run_count = 0;
    std::size_t text_bytes = 0;
};

PendingPdfExport pending_pdf_export;

RunStyle effective_run_style(const TextRun& run) {
    RunStyle style = run.style;
    if (style.language.empty() || style.language == "en-US") {
        for (std::size_t index = 0; index < run.text.size();) {
            const std::uint32_t scalar = scalar_at(run.text, index);
            const LanguageProfile& detected = profile_for_scalar(scalar);
            if (detected.code_page != 1252) {
                style.language = detected.tag;
                style.code_page = detected.code_page;
                style.charset = detected.charset;
                break;
            }
        }
    }
    return style;
}

void append_legacy_scalar(std::string& result,
                          std::vector<UnicodeCell>* cells,
                          const std::uint32_t scalar,
                          const LanguageProfile& profile) {
    result.push_back(legacy_byte_for_scalar(scalar, profile));
    if (cells != nullptr) cells->push_back({scalar, profile.language_id});
}

std::string legacy_run_text(std::wstring_view text, const RunStyle& style,
                            std::vector<UnicodeCell>* cells) {
    std::string result;
    const LanguageProfile& configured = profile_for_tag(style.language);
    for (std::size_t index = 0; index < text.size();) {
        const std::uint32_t scalar = scalar_at(text, index);
        const LanguageProfile& profile = style.language.empty() ?
            profile_for_scalar(scalar) : configured;
        if (scalar == L'\n') {
            append_legacy_scalar(result, cells, L'\r', profile);
            append_legacy_scalar(result, cells, L'\n', profile);
        } else if (scalar == L'\r') {
            if (result.empty() || result.back() != '\r')
                append_legacy_scalar(result, cells, scalar, profile);
        } else {
            append_legacy_scalar(result, cells, scalar, profile);
        }
    }
    return result;
}

std::string paragraphs_to_text(const std::vector<Paragraph>& paragraphs,
                               PendingDocxImport* pending = nullptr,
                               const std::vector<TableLayout>* tables = nullptr) {
    std::string text;
    if (pending != nullptr) *pending = {};
    for (std::size_t index = 0; index < paragraphs.size(); ++index) {
        require_parse_limit(text.size() <= kMaxTextBytes);
        PendingParagraphFormat paragraph_format;
        paragraph_format.cp_first = static_cast<long>(text.size());
        paragraph_format.paragraph = paragraphs[index];
        paragraph_format.paragraph.runs.clear();
        for (const auto& run : paragraphs[index].runs) {
            PendingRunFormat run_format;
            run_format.cp_first = static_cast<long>(text.size());
            run_format.style = effective_run_style(run);
            std::vector<UnicodeCell>* cells = pending != nullptr ?
                &pending->unicode_cells : nullptr;
            const std::string encoded = legacy_run_text(
                run.text, run_format.style, cells);
            require_parse_limit(encoded.size() <= kMaxTextBytes - text.size());
            text += encoded;
            run_format.cp_lim = static_cast<long>(text.size());
            if (pending != nullptr && run_format.cp_lim > run_format.cp_first)
                pending->runs.push_back(std::move(run_format));
        }
        if (index + 1 < paragraphs.size()) {
            require_parse_limit(text.size() <= kMaxTextBytes - 2);
            text += "\r\n";
            if (pending != nullptr) {
                const LanguageProfile& profile = default_language_profile();
                pending->unicode_cells.push_back({L'\r', profile.language_id});
                pending->unicode_cells.push_back({L'\n', profile.language_id});
            }
        }
        paragraph_format.cp_lim = static_cast<long>(text.size());
        if (pending != nullptr)
            pending->paragraphs.push_back(std::move(paragraph_format));
    }
    if (pending != nullptr && tables != nullptr) {
        for (const TableLayout& table : *tables) {
            if (table.paragraph_count == 0 ||
                table.first_paragraph >= pending->paragraphs.size() ||
                table.first_paragraph + table.paragraph_count >
                    pending->paragraphs.size()) continue;
            PendingTableFormat format;
            format.cp_first =
                pending->paragraphs[table.first_paragraph].cp_first;
            format.cp_lim = pending->paragraphs[
                table.first_paragraph + table.paragraph_count - 1].cp_lim;
            format.first_paragraph = static_cast<int>(table.first_paragraph);
            format.rows = table.rows;
            format.columns = table.columns;
            pending->tables.push_back(format);
        }
    }
    return text;
}

bool load_docx_paragraphs(const char* path,
                          std::vector<Paragraph>& paragraphs,
                          DocumentSettings* settings = nullptr,
                          std::vector<TableLayout>* tables = nullptr) {
    std::string document;
    std::string styles;
    const std::wstring document_path = wide_path(path);
    if (document_path.empty() ||
        !read_opc_part(document_path, OPUSW("/word/document.xml"), document,
                       kMaxDocumentXmlBytes))
        return false;
    read_opc_part(document_path, OPUSW("/word/styles.xml"), styles,
                  kMaxStylesXmlBytes);
    paragraphs = parse_document_xml(document, parse_style_catalog(styles),
                                    tables);
    if (settings != nullptr) *settings = parse_document_settings(document);
    return true;
}

int legacy_color_index(const RunStyle& style) {
    if (style.auto_color) return 0;
    const int red = GetRValue(style.color);
    const int green = GetGValue(style.color);
    const int blue = GetBValue(style.color);
    const int maximum = (std::max)({red, green, blue});
    const int minimum = (std::min)({red, green, blue});
    if (maximum < 64 || maximum - minimum < 32) {
        return maximum > 208 ? 8 : 1;
    }
    const bool high_red = red * 5 >= maximum * 3;
    const bool high_green = green * 5 >= maximum * 3;
    const bool high_blue = blue * 5 >= maximum * 3;
    if (high_red && high_green && !high_blue) return 7;
    if (high_red && high_blue && !high_green) return 5;
    if (high_green && high_blue && !high_red) return 3;
    if (maximum == red) return 6;
    if (maximum == green) return 4;
    return 2;
}

void apply_legacy_color(const int color_index, RunStyle& style) {
    static constexpr std::array<COLORREF, 9> colors = {
        RGB(0, 0, 0), RGB(0, 0, 0), RGB(0, 0, 255),
        RGB(0, 255, 255), RGB(0, 128, 0), RGB(255, 0, 255),
        RGB(255, 0, 0), RGB(255, 255, 0), RGB(255, 255, 255)};
    style.auto_color = color_index <= 0 ||
        color_index >= static_cast<int>(colors.size());
    style.color = style.auto_color ? RGB(0, 0, 0) : colors[color_index];
}

std::string ansi_text_to_rtf(std::string_view text) {
    std::string rtf =
        "{\\rtf1\\ansi\\deff0{\\fonttbl{\\f0 Arial;}}\\f0\\fs24 ";
    static constexpr char hex[] = "0123456789abcdef";
    for (std::size_t index = 0; index < text.size(); ++index) {
        const unsigned char character =
            static_cast<unsigned char>(text[index]);
        if (character == '\r' || character == '\n') {
            if (character == '\r' && index + 1 < text.size() &&
                text[index + 1] == '\n') {
                ++index;
            }
            rtf += "\\par ";
        } else if (character == '\t') {
            rtf += "\\tab ";
        } else if (character == '\f') {
            rtf += "\\page ";
        } else if (character == '\\' || character == '{' ||
                   character == '}') {
            rtf.push_back('\\');
            rtf.push_back(static_cast<char>(character));
        } else if (character >= 0x80) {
            rtf += "\\'";
            rtf.push_back(hex[character >> 4]);
            rtf.push_back(hex[character & 0x0f]);
        } else if (character >= 0x20) {
            rtf.push_back(static_cast<char>(character));
        }
    }
    rtf.push_back('}');
    return rtf;
}

void append_rtf_text(std::string& rtf, std::wstring_view text) {
    for (const wchar_t character : text) {
        if (character == L'\\' || character == L'{' || character == L'}') {
            rtf.push_back('\\');
            rtf.push_back(static_cast<char>(character));
        } else if (character == L'\t') rtf += "\\tab ";
        else if (character == L'\n') rtf += "\\line ";
        else if (character >= 0x20 && character <= 0x7e) {
            rtf.push_back(static_cast<char>(character));
        } else {
            const auto unit = static_cast<std::int16_t>(character);
            rtf += "\\u" + std::to_string(static_cast<int>(unit)) + "?";
        }
    }
}

std::string paragraphs_to_rtf(const std::vector<Paragraph>& paragraphs) {
    std::vector<std::wstring> fonts{L"Arial"};
    std::vector<COLORREF> colors;
    for (const auto& paragraph : paragraphs) for (const auto& run : paragraph.runs) {
        if (std::find(fonts.begin(), fonts.end(), run.style.font) == fonts.end())
            fonts.push_back(run.style.font);
        if (!run.style.auto_color &&
            std::find(colors.begin(), colors.end(), run.style.color) == colors.end())
            colors.push_back(run.style.color);
    }
    std::string rtf = "{\\rtf1\\ansi\\ansicpg1252\\uc1 \\deff0";
    rtf += "{\\fonttbl";
    for (std::size_t index = 0; index < fonts.size(); ++index) {
        int charset = ANSI_CHARSET;
        for (const auto& paragraph : paragraphs) {
            const auto found = std::find_if(
                paragraph.runs.begin(), paragraph.runs.end(),
                [&](const TextRun& run) { return run.style.font == fonts[index]; });
            if (found != paragraph.runs.end()) {
                charset = found->style.charset;
                break;
            }
        }
        rtf += "{\\f" + std::to_string(index) + "\\fnil\\fcharset" +
               std::to_string(charset) + " ";
        append_rtf_text(rtf, fonts[index]);
        rtf += ";}";
    }
    rtf += "}{\\colortbl;";
    for (const COLORREF color : colors) {
        rtf += "\\red" + std::to_string(GetRValue(color)) +
               "\\green" + std::to_string(GetGValue(color)) +
               "\\blue" + std::to_string(GetBValue(color)) + ";";
    }
    rtf += "}";
    for (const auto& paragraph : paragraphs) {
        rtf += "\\pard";
        if (paragraph.alignment == PFA_CENTER) rtf += "\\qc";
        else if (paragraph.alignment == PFA_RIGHT) rtf += "\\qr";
        else if (paragraph.alignment == PFA_JUSTIFY) rtf += "\\qj";
        else rtf += "\\ql";
        rtf.push_back(' ');
        for (const auto& run : paragraph.runs) {
            const auto font = std::find(fonts.begin(), fonts.end(), run.style.font);
            const auto color = std::find(colors.begin(), colors.end(), run.style.color);
            rtf += "{";
            rtf += run.style.bold ? "\\b" : "\\b0";
            rtf += run.style.italic ? "\\i" : "\\i0";
            rtf += run.style.underline ? "\\ul" : "\\ulnone";
            rtf += "\\fs" + std::to_string(run.style.half_points);
            rtf += "\\f" + std::to_string(std::distance(fonts.begin(), font));
            rtf += "\\lang" + std::to_string(
                profile_for_tag(run.style.language).language_id);
            if (!run.style.auto_color)
                rtf += "\\cf" + std::to_string(std::distance(colors.begin(), color) + 1);
            rtf.push_back(' ');
            append_rtf_text(rtf, run.text);
            rtf += "}";
        }
        rtf += "\\par\n";
    }
    rtf += "}";
    return rtf;
}

bool reserve_sibling_temporary_file(const std::wstring& path,
                                    std::wstring& temporary,
                                    HANDLE& file) {
    if (!safe_file_path_syntax(path)) return false;
    static std::atomic<unsigned long> sequence{0};
    for (int attempt = 0; attempt < 64; ++attempt) {
        temporary = path + L".word1tmp-" +
            std::to_wstring(GetCurrentProcessId()) + L"-" +
            std::to_wstring(++sequence);
        const std::vector<WCHAR> native_temporary =
            native_file_path(temporary);
        file = CreateFileW(native_temporary.data(), GENERIC_WRITE, 0, nullptr,
                           CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file != INVALID_HANDLE_VALUE) return true;
        if (GetLastError() != ERROR_FILE_EXISTS &&
            GetLastError() != ERROR_ALREADY_EXISTS) return false;
    }
    return false;
}

bool commit_sibling_temporary_file(const std::wstring& temporary,
                                   const std::wstring& path) {
    const std::vector<WCHAR> native_temporary = native_file_path(temporary);
    const std::vector<WCHAR> native_path = native_file_path(path);
    const DWORD attributes = GetFileAttributesW(native_path.data());
    bool replaced = false;
    if (attributes != INVALID_FILE_ATTRIBUTES &&
        (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
        replaced = ReplaceFileW(native_path.data(), native_temporary.data(),
                                nullptr, REPLACEFILE_WRITE_THROUGH, nullptr,
                                nullptr) != FALSE;
        if (!replaced) {
            replaced = MoveFileExW(
                native_temporary.data(), native_path.data(),
                MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != FALSE;
        }
    } else if (attributes == INVALID_FILE_ATTRIBUTES &&
               GetLastError() == ERROR_FILE_NOT_FOUND) {
        replaced = MoveFileExW(native_temporary.data(), native_path.data(),
                               MOVEFILE_WRITE_THROUGH) != FALSE;
    }
    if (!replaced) DeleteFileW(native_temporary.data());
    return replaced;
}

bool write_bytes(const std::wstring& path, std::string_view bytes) {
    if (bytes.size() > MAXDWORD) return false;
    std::wstring temporary;
    HANDLE file = INVALID_HANDLE_VALUE;
    if (!reserve_sibling_temporary_file(path, temporary, file)) return false;
    std::size_t position = 0;
    bool ok = true;
    while (position < bytes.size()) {
        DWORD written = 0;
        const DWORD requested = static_cast<DWORD>((std::min)(
            bytes.size() - position,
            static_cast<std::size_t>((std::numeric_limits<DWORD>::max)())));
        if (!WriteFile(file, bytes.data() + position, requested, &written,
                       nullptr) || written == 0) {
            ok = false;
            break;
        }
        position += written;
    }
    if (ok) ok = FlushFileBuffers(file) != FALSE;
    CloseHandle(file);
    if (!ok) {
        const std::vector<WCHAR> native_temporary =
            native_file_path(temporary);
        DeleteFileW(native_temporary.data());
        return false;
    }
    return commit_sibling_temporary_file(temporary, path);
}

bool read_bytes(const std::wstring& path, std::string& bytes,
                const std::size_t maximum_size = kMaxRtfBytes) {
    if (!regular_file_within_limit(path, maximum_size)) return false;
    const std::vector<WCHAR> native_path = native_file_path(path);
    HANDLE file = CreateFileW(native_path.data(), GENERIC_READ, FILE_SHARE_READ,
                              nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                              nullptr);
    if (file == INVALID_HANDLE_VALUE) return false;
    LARGE_INTEGER size{};
    bool ok = GetFileSizeEx(file, &size) && size.QuadPart >= 0 &&
              size.QuadPart <= static_cast<LONGLONG>(maximum_size) &&
              maximum_size <= MAXDWORD;
    if (ok) {
        bytes.resize(static_cast<std::size_t>(size.QuadPart));
        DWORD read = 0;
        ok = bytes.empty() || (ReadFile(file, bytes.data(),
            static_cast<DWORD>(bytes.size()), &read, nullptr) && read == bytes.size());
    }
    CloseHandle(file);
    return ok;
}

std::uint16_t zip_u16(std::string_view bytes, const std::size_t offset) {
    if (offset > bytes.size() || bytes.size() - offset < 2)
        throw std::runtime_error("truncated ZIP field");
    return static_cast<std::uint16_t>(
        static_cast<unsigned char>(bytes[offset]) |
        (static_cast<unsigned char>(bytes[offset + 1]) << 8));
}

std::uint32_t zip_u32(std::string_view bytes, const std::size_t offset) {
    if (offset > bytes.size() || bytes.size() - offset < 4)
        throw std::runtime_error("truncated ZIP field");
    return static_cast<std::uint32_t>(
        static_cast<unsigned char>(bytes[offset]) |
        (static_cast<unsigned char>(bytes[offset + 1]) << 8) |
        (static_cast<unsigned char>(bytes[offset + 2]) << 16) |
        (static_cast<unsigned char>(bytes[offset + 3]) << 24));
}

void zip_append_u16(std::string& bytes, const std::uint16_t value) {
    bytes.push_back(static_cast<char>(value & 0xff));
    bytes.push_back(static_cast<char>((value >> 8) & 0xff));
}

void zip_append_u32(std::string& bytes, const std::uint32_t value) {
    bytes.push_back(static_cast<char>(value & 0xff));
    bytes.push_back(static_cast<char>((value >> 8) & 0xff));
    bytes.push_back(static_cast<char>((value >> 16) & 0xff));
    bytes.push_back(static_cast<char>((value >> 24) & 0xff));
}

std::uint32_t zip_crc32(std::string_view bytes) {
    std::uint32_t crc = 0xffffffffu;
    for (const unsigned char byte : bytes) {
        crc ^= byte;
        for (int bit = 0; bit < 8; ++bit)
            crc = (crc >> 1) ^ (0xedb88320u &
                  static_cast<std::uint32_t>(-
                      static_cast<int>(crc & 1)));
    }
    return ~crc;
}

class DeflateBits {
public:
    explicit DeflateBits(std::string_view bytes) : bytes_(bytes) {}
    bool read(const unsigned count, unsigned& value) {
        if (count > 24) return false;
        while (bits_ < count) {
            if (position_ >= bytes_.size()) return false;
            buffer_ |= static_cast<std::uint64_t>(
                static_cast<unsigned char>(bytes_[position_++])) << bits_;
            bits_ += 8;
        }
        value = static_cast<unsigned>(buffer_ & ((1ull << count) - 1));
        buffer_ >>= count;
        bits_ -= count;
        return true;
    }
    bool align_byte() {
        const unsigned discard = bits_ & 7u;
        unsigned ignored = 0;
        return discard == 0 || read(discard, ignored);
    }
    std::size_t byte_position() const { return position_ - bits_ / 8; }
    bool set_byte_position(const std::size_t position) {
        if (position > bytes_.size()) return false;
        position_ = position;
        buffer_ = 0;
        bits_ = 0;
        return true;
    }
    std::string_view bytes() const { return bytes_; }
private:
    std::string_view bytes_;
    std::size_t position_ = 0;
    std::uint64_t buffer_ = 0;
    unsigned bits_ = 0;
};

struct DeflateHuffman {
    std::array<unsigned, 16> counts{};
    std::vector<unsigned short> symbols;

    bool build(const std::vector<unsigned char>& lengths) {
        counts.fill(0);
        symbols.assign(lengths.size(), 0);
        for (const unsigned length : lengths) {
            if (length > 15) return false;
            ++counts[length];
        }
        if (counts[0] == lengths.size()) return false;
        int remaining = 1;
        for (unsigned length = 1; length <= 15; ++length) {
            remaining <<= 1;
            remaining -= static_cast<int>(counts[length]);
            if (remaining < 0) return false;
        }
        std::array<unsigned, 16> offsets{};
        for (unsigned length = 1; length < 15; ++length)
            offsets[length + 1] = offsets[length] + counts[length];
        for (unsigned symbol = 0; symbol < lengths.size(); ++symbol)
            if (lengths[symbol] != 0)
                symbols[offsets[lengths[symbol]]++] =
                    static_cast<unsigned short>(symbol);
        return true;
    }

    bool decode(DeflateBits& bits, unsigned& symbol) const {
        unsigned code = 0;
        unsigned first = 0;
        unsigned index = 0;
        for (unsigned length = 1; length <= 15; ++length) {
            unsigned bit = 0;
            if (!bits.read(1, bit)) return false;
            code |= bit;
            const unsigned count = counts[length];
            if (code >= first && code - first < count) {
                const unsigned location = index + code - first;
                if (location >= symbols.size()) return false;
                symbol = symbols[location];
                return true;
            }
            index += count;
            first = (first + count) << 1;
            code <<= 1;
        }
        return false;
    }
};

bool inflate_raw(std::string_view compressed, const std::size_t expected,
                 std::string& output) {
    static constexpr unsigned length_base[29] = {
        3,4,5,6,7,8,9,10,11,13,15,17,19,23,27,31,35,43,51,59,67,83,
        99,115,131,163,195,227,258};
    static constexpr unsigned length_extra[29] = {
        0,0,0,0,0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,0};
    static constexpr unsigned distance_base[30] = {
        1,2,3,4,5,7,9,13,17,25,33,49,65,97,129,193,257,385,513,769,
        1025,1537,2049,3073,4097,6145,8193,12289,16385,24577};
    static constexpr unsigned distance_extra[30] = {
        0,0,0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,9,9,10,10,11,11,
        12,12,13,13};
    if (expected > kMaxGeneratedBytes) return false;
    output.clear();
    output.reserve(expected);
    DeflateBits bits(compressed);
    bool final_block = false;
    while (!final_block) {
        unsigned final = 0;
        unsigned type = 0;
        if (!bits.read(1, final) || !bits.read(2, type)) return false;
        final_block = final != 0;
        if (type == 0) {
            if (!bits.align_byte()) return false;
            const std::size_t position = bits.byte_position();
            if (position > compressed.size() ||
                compressed.size() - position < 4) return false;
            const unsigned length = zip_u16(compressed, position);
            const unsigned complement = zip_u16(compressed, position + 2);
            if ((length ^ 0xffffu) != complement ||
                compressed.size() - position - 4 < length ||
                output.size() > expected || expected - output.size() < length)
                return false;
            output.append(compressed.substr(position + 4, length));
            if (!bits.set_byte_position(position + 4 + length)) return false;
            continue;
        }
        if (type == 3) return false;

        std::vector<unsigned char> literal_lengths;
        std::vector<unsigned char> distance_lengths;
        if (type == 1) {
            literal_lengths.assign(288, 0);
            for (unsigned index = 0; index <= 143; ++index)
                literal_lengths[index] = 8;
            for (unsigned index = 144; index <= 255; ++index)
                literal_lengths[index] = 9;
            for (unsigned index = 256; index <= 279; ++index)
                literal_lengths[index] = 7;
            for (unsigned index = 280; index <= 287; ++index)
                literal_lengths[index] = 8;
            distance_lengths.assign(32, 5);
        } else {
            unsigned hlit = 0, hdist = 0, hclen = 0;
            if (!bits.read(5, hlit) || !bits.read(5, hdist) ||
                !bits.read(4, hclen)) return false;
            hlit += 257;
            hdist += 1;
            hclen += 4;
            if (hlit > 286 || hdist > 32) return false;
            static constexpr unsigned order[19] = {
                16,17,18,0,8,7,9,6,10,5,11,4,12,3,13,2,14,1,15};
            std::vector<unsigned char> code_lengths(19, 0);
            for (unsigned index = 0; index < hclen; ++index) {
                unsigned length = 0;
                if (!bits.read(3, length)) return false;
                code_lengths[order[index]] = static_cast<unsigned char>(length);
            }
            DeflateHuffman code_tree;
            if (!code_tree.build(code_lengths)) return false;
            std::vector<unsigned char> all_lengths;
            all_lengths.reserve(hlit + hdist);
            while (all_lengths.size() < hlit + hdist) {
                unsigned symbol = 0;
                if (!code_tree.decode(bits, symbol)) return false;
                if (symbol <= 15) {
                    all_lengths.push_back(static_cast<unsigned char>(symbol));
                } else if (symbol == 16) {
                    unsigned repeat = 0;
                    if (all_lengths.empty() || !bits.read(2, repeat)) return false;
                    repeat += 3;
                    if (repeat > hlit + hdist - all_lengths.size()) return false;
                    all_lengths.insert(all_lengths.end(), repeat,
                                       all_lengths.back());
                } else if (symbol == 17 || symbol == 18) {
                    unsigned repeat = 0;
                    const unsigned extra = symbol == 17 ? 3 : 7;
                    if (!bits.read(extra, repeat)) return false;
                    repeat += symbol == 17 ? 3 : 11;
                    if (repeat > hlit + hdist - all_lengths.size()) return false;
                    all_lengths.insert(all_lengths.end(), repeat, 0);
                } else return false;
            }
            literal_lengths.assign(all_lengths.begin(),
                                   all_lengths.begin() + hlit);
            distance_lengths.assign(all_lengths.begin() + hlit,
                                    all_lengths.end());
        }
        DeflateHuffman literal_tree;
        DeflateHuffman distance_tree;
        if (!literal_tree.build(literal_lengths) ||
            !distance_tree.build(distance_lengths)) return false;
        for (;;) {
            unsigned symbol = 0;
            if (!literal_tree.decode(bits, symbol)) return false;
            if (symbol < 256) {
                if (output.size() >= expected) return false;
                output.push_back(static_cast<char>(symbol));
                continue;
            }
            if (symbol == 256) break;
            if (symbol < 257 || symbol > 285) return false;
            const unsigned length_index = symbol - 257;
            unsigned length_bits = 0;
            if (!bits.read(length_extra[length_index], length_bits)) return false;
            const unsigned length = length_base[length_index] + length_bits;
            unsigned distance_symbol = 0;
            if (!distance_tree.decode(bits, distance_symbol) ||
                distance_symbol >= 30) return false;
            unsigned distance_bits = 0;
            if (!bits.read(distance_extra[distance_symbol], distance_bits))
                return false;
            const unsigned distance =
                distance_base[distance_symbol] + distance_bits;
            if (distance == 0 || distance > output.size() ||
                output.size() > expected || expected - output.size() < length)
                return false;
            for (unsigned index = 0; index < length; ++index)
                output.push_back(output[output.size() - distance]);
        }
    }
    return output.size() == expected;
}

bool read_zip_entry(const std::wstring& path, std::string_view requested_name,
                    std::string& data, const std::size_t maximum_size) {
    std::string package;
    if (!read_bytes(path, package, kMaxGeneratedBytes) ||
        package.size() < 22 || requested_name.empty()) return false;
    const std::size_t search_first = package.size() > 65557 ?
        package.size() - 65557 : 0;
    std::size_t eocd = std::string::npos;
    for (std::size_t position = package.size() - 22;; --position) {
        if (zip_u32(package, position) == 0x06054b50u) {
            eocd = position;
            break;
        }
        if (position == search_first) break;
    }
    if (eocd == std::string::npos || zip_u16(package, eocd + 4) != 0 ||
        zip_u16(package, eocd + 6) != 0 ||
        zip_u16(package, eocd + 8) != zip_u16(package, eocd + 10))
        return false;
    const unsigned entry_count = zip_u16(package, eocd + 10);
    if (entry_count > 4096) return false;
    const std::size_t central_size = zip_u32(package, eocd + 12);
    const std::size_t central_offset = zip_u32(package, eocd + 16);
    if (central_offset > package.size() ||
        central_size > package.size() - central_offset ||
        central_offset + central_size > eocd) return false;
    std::size_t position = central_offset;
    for (unsigned entry = 0; entry < entry_count; ++entry) {
        if (position > package.size() || package.size() - position < 46 ||
            zip_u32(package, position) != 0x02014b50u) return false;
        const unsigned flags = zip_u16(package, position + 8);
        const unsigned method = zip_u16(package, position + 10);
        const std::uint32_t crc = zip_u32(package, position + 16);
        const std::size_t compressed_size = zip_u32(package, position + 20);
        const std::size_t uncompressed_size = zip_u32(package, position + 24);
        const std::size_t name_length = zip_u16(package, position + 28);
        const std::size_t extra_length = zip_u16(package, position + 30);
        const std::size_t comment_length = zip_u16(package, position + 32);
        const std::size_t local_offset = zip_u32(package, position + 42);
        const std::size_t record_size = 46 + name_length + extra_length +
                                        comment_length;
        if (record_size > package.size() - position) return false;
        const std::string_view name(package.data() + position + 46,
                                    name_length);
        if (name == requested_name) {
            if ((flags & 1u) != 0 || (method != 0 && method != 8) ||
                uncompressed_size > maximum_size ||
                compressed_size > kMaxGeneratedBytes ||
                local_offset > package.size() ||
                package.size() - local_offset < 30 ||
                zip_u32(package, local_offset) != 0x04034b50u)
                return false;
            const std::size_t local_name = zip_u16(package, local_offset + 26);
            const std::size_t local_extra = zip_u16(package, local_offset + 28);
            const std::size_t content_offset =
                local_offset + 30 + local_name + local_extra;
            if (content_offset > package.size() ||
                compressed_size > package.size() - content_offset)
                return false;
            const std::string_view compressed(
                package.data() + content_offset, compressed_size);
            if (method == 0) {
                if (compressed_size != uncompressed_size) return false;
                data.assign(compressed);
            } else if (!inflate_raw(compressed, uncompressed_size, data)) {
                return false;
            }
            return zip_crc32(data) == crc;
        }
        position += record_size;
    }
    return false;
}

struct ZipWriteEntry {
    std::string name;
    std::string data;
};

bool write_stored_zip(const std::wstring& path,
                      const std::vector<ZipWriteEntry>& entries) {
    if (entries.empty() || entries.size() > 4096) return false;
    std::string archive;
    std::string central;
    archive.reserve(4096);
    for (const ZipWriteEntry& entry : entries) {
        if (entry.name.empty() || entry.name.size() > 0xffff ||
            entry.data.size() > 0xffffffffu ||
            archive.size() > 0xffffffffu) return false;
        const std::uint32_t offset = static_cast<std::uint32_t>(archive.size());
        const std::uint32_t size = static_cast<std::uint32_t>(entry.data.size());
        const std::uint32_t crc = zip_crc32(entry.data);
        zip_append_u32(archive, 0x04034b50u);
        zip_append_u16(archive, 20);
        zip_append_u16(archive, 0x0800);
        zip_append_u16(archive, 0);
        zip_append_u16(archive, 0);
        zip_append_u16(archive, 0);
        zip_append_u32(archive, crc);
        zip_append_u32(archive, size);
        zip_append_u32(archive, size);
        zip_append_u16(archive, static_cast<std::uint16_t>(entry.name.size()));
        zip_append_u16(archive, 0);
        archive += entry.name;
        archive += entry.data;

        zip_append_u32(central, 0x02014b50u);
        zip_append_u16(central, 20);
        zip_append_u16(central, 20);
        zip_append_u16(central, 0x0800);
        zip_append_u16(central, 0);
        zip_append_u16(central, 0);
        zip_append_u16(central, 0);
        zip_append_u32(central, crc);
        zip_append_u32(central, size);
        zip_append_u32(central, size);
        zip_append_u16(central, static_cast<std::uint16_t>(entry.name.size()));
        zip_append_u16(central, 0);
        zip_append_u16(central, 0);
        zip_append_u16(central, 0);
        zip_append_u16(central, 0);
        zip_append_u32(central, 0);
        zip_append_u32(central, offset);
        central += entry.name;
    }
    if (archive.size() > 0xffffffffu || central.size() > 0xffffffffu ||
        archive.size() + central.size() > kMaxGeneratedBytes) return false;
    const std::uint32_t central_offset =
        static_cast<std::uint32_t>(archive.size());
    archive += central;
    zip_append_u32(archive, 0x06054b50u);
    zip_append_u16(archive, 0);
    zip_append_u16(archive, 0);
    zip_append_u16(archive, static_cast<std::uint16_t>(entries.size()));
    zip_append_u16(archive, static_cast<std::uint16_t>(entries.size()));
    zip_append_u32(archive, static_cast<std::uint32_t>(central.size()));
    zip_append_u32(archive, central_offset);
    zip_append_u16(archive, 0);
    return archive.size() <= kMaxGeneratedBytes && write_bytes(path, archive);
}

struct LocalElement {
    std::string_view opening;
    std::string_view inner;
    std::string_view block;
    std::size_t next = 0;
};

bool next_local_element(std::string_view xml, std::string_view local_name,
                        std::size_t start, LocalElement& result) {
    while ((start = xml.find('<', start)) != std::string_view::npos) {
        if (start + 1 >= xml.size()) return false;
        const char kind = xml[start + 1];
        const std::size_t open_end = xml.find('>', start + 1);
        if (open_end == std::string_view::npos) return false;
        const std::string_view opening =
            xml.substr(start, open_end - start + 1);
        if (kind == '/' || kind == '!' || kind == '?' ||
            local_tag_name(opening) != local_name) {
            start = open_end + 1;
            continue;
        }
        if (open_end > start && xml[open_end - 1] == '/') {
            result = {opening, {}, opening, open_end + 1};
            return true;
        }
        std::size_t name_start = start + 1;
        while (name_start < open_end && std::isspace(
                   static_cast<unsigned char>(xml[name_start]))) ++name_start;
        std::size_t name_end = name_start;
        while (name_end < open_end && !std::isspace(
                   static_cast<unsigned char>(xml[name_end])) &&
               xml[name_end] != '/' && xml[name_end] != '>') ++name_end;
        if (name_end == name_start) return false;
        const std::string qualified(xml.substr(name_start,
                                                name_end - name_start));
        const std::string closing = "</" + qualified + ">";
        const std::size_t close = xml.find(closing, open_end + 1);
        if (close == std::string_view::npos) return false;
        const std::size_t block_end = close + closing.size();
        result.opening = opening;
        result.inner = xml.substr(open_end + 1, close - open_end - 1);
        result.block = xml.substr(start, block_end - start);
        result.next = block_end;
        return true;
    }
    return false;
}

std::string_view first_local_block(std::string_view xml,
                                   std::string_view local_name) {
    LocalElement element;
    return next_local_element(xml, local_name, 0, element) ? element.block :
        std::string_view{};
}

bool parse_odf_number(std::string_view source, double& result) {
    if (source.empty() || source.size() > 64) return false;
    const std::string value(source);
    char* end = nullptr;
    result = std::strtod(value.c_str(), &end);
    return end != value.c_str() && std::isfinite(result) &&
           static_cast<std::size_t>(end - value.c_str()) <= value.size();
}

bool parse_odf_length(std::string_view value, int& twips,
                      const int minimum = -31680,
                      const int maximum = 63360) {
    double number = 0.0;
    if (!parse_odf_number(value, number)) return false;
    std::size_t suffix = 0;
    while (suffix < value.size() &&
           (std::isdigit(static_cast<unsigned char>(value[suffix])) ||
            value[suffix] == '+' || value[suffix] == '-' ||
            value[suffix] == '.' || value[suffix] == 'e' ||
            value[suffix] == 'E')) ++suffix;
    const std::string_view unit = value.substr(suffix);
    double scale = 0.0;
    if (unit == "in") scale = 1440.0;
    else if (unit == "cm") scale = 1440.0 / 2.54;
    else if (unit == "mm") scale = 1440.0 / 25.4;
    else if (unit == "pt") scale = 20.0;
    else if (unit == "pc") scale = 240.0;
    else if (unit == "px") scale = 15.0;
    else return false;
    const double converted = number * scale;
    if (!std::isfinite(converted) || converted < minimum ||
        converted > maximum) return false;
    twips = static_cast<int>(std::lround(converted));
    return true;
}

bool odf_truth(std::string_view value) {
    return value == "true" || value == "always" || value == "page" ||
           value == "bold";
}

struct OdfStyleDefinition {
    std::string family;
    std::string parent;
    std::string text_properties;
    std::string paragraph_properties;
};

struct OdfStyleCatalog {
    RunStyle default_run;
    Paragraph default_paragraph;
    DocumentSettings settings;
    std::map<std::string, OdfStyleDefinition> definitions;
};

void apply_odf_text_properties(RunStyle& style, std::string_view properties) {
    LocalElement element;
    if (!next_local_element(properties, "text-properties", 0, element)) return;
    const std::string_view tag = element.opening;
    const std::string weight = tag_attribute(tag, "font-weight");
    if (!weight.empty()) {
        int numeric = 0;
        style.bold = weight == "bold" ||
            (parse_bounded_int(weight, 1, 1000, numeric) && numeric >= 600);
    }
    const std::string italic = tag_attribute(tag, "font-style");
    if (!italic.empty()) style.italic = italic != "normal" && italic != "none";
    const std::string underline = tag_attribute(tag, "text-underline-style");
    if (!underline.empty()) style.underline = underline != "none";
    const std::string strike = tag_attribute(tag, "text-line-through-style");
    if (!strike.empty()) style.strike = strike != "none";
    const std::string variant = tag_attribute(tag, "font-variant");
    if (!variant.empty()) style.small_caps = variant == "small-caps";
    const std::string transform = tag_attribute(tag, "text-transform");
    if (!transform.empty()) style.all_caps = transform == "uppercase";
    const std::string display = tag_attribute(tag, "text-display");
    if (!display.empty()) style.hidden = display == "none";
    const std::string size = tag_attribute(tag, "font-size");
    int size_twips = 0;
    if (parse_odf_length(size, size_twips, 20, 2540))
        style.half_points = (std::clamp)(size_twips / 10, 2, 254);
    std::string font = tag_attribute(tag, "font-family");
    if (font.empty()) font = tag_attribute(tag, "font-name");
    if (font.size() >= 2 && ((font.front() == '\'' && font.back() == '\'') ||
                            (font.front() == '"' && font.back() == '"')))
        font = font.substr(1, font.size() - 2);
    if (!font.empty()) style.font = xml_unescape(font);
    const std::string color = tag_attribute(tag, "color");
    if (color.size() == 7 && color.front() == '#') {
        unsigned rgb = 0;
        if (parse_rgb(std::string_view(color).substr(1), rgb)) {
            style.color = RGB((rgb >> 16) & 0xff, (rgb >> 8) & 0xff,
                              rgb & 0xff);
            style.auto_color = false;
        }
    } else if (color == "auto") {
        style.auto_color = true;
        style.color = RGB(0, 0, 0);
    }
    std::string language = tag_attribute(tag, "language");
    const std::string country = tag_attribute(tag, "country");
    if (!language.empty() && language != "none") {
        if (!country.empty() && country != "none") language += "-" + country;
        const LanguageProfile& profile = profile_for_tag(language);
        style.language = language;
        style.code_page = profile.code_page;
        style.charset = profile.charset;
    }
}

void apply_odf_paragraph_properties(Paragraph& paragraph,
                                    std::string_view properties) {
    LocalElement element;
    if (!next_local_element(properties, "paragraph-properties", 0, element))
        return;
    const std::string_view tag = element.opening;
    const std::string alignment = tag_attribute(tag, "text-align");
    if (!alignment.empty()) {
        if (alignment == "center") paragraph.alignment = PFA_CENTER;
        else if (alignment == "right" || alignment == "end")
            paragraph.alignment = PFA_RIGHT;
        else if (alignment == "justify") paragraph.alignment = PFA_JUSTIFY;
        else paragraph.alignment = PFA_LEFT;
    }
    const std::string common_margin = tag_attribute(tag, "margin");
    if (!common_margin.empty()) {
        parse_odf_length(common_margin, paragraph.left_indent);
        paragraph.right_indent = paragraph.left_indent;
    }
    parse_odf_length(tag_attribute(tag, "margin-left"),
                     paragraph.left_indent);
    parse_odf_length(tag_attribute(tag, "margin-right"),
                     paragraph.right_indent);
    parse_odf_length(tag_attribute(tag, "text-indent"),
                     paragraph.first_line_indent);
    parse_odf_length(tag_attribute(tag, "margin-top"), paragraph.space_before,
                     0, 31680);
    parse_odf_length(tag_attribute(tag, "margin-bottom"), paragraph.space_after,
                     0, 31680);
    const std::string line_height = tag_attribute(tag, "line-height");
    if (!line_height.empty() && line_height.back() == '%') {
        double percent = 0.0;
        if (parse_odf_number(
                std::string_view(line_height).substr(0, line_height.size() - 1),
                percent) && percent >= 10.0 && percent <= 1000.0)
            paragraph.line_spacing = static_cast<int>(std::lround(240.0 *
                                                                  percent / 100.0));
    } else {
        parse_odf_length(line_height, paragraph.line_spacing, 0, 31680);
    }
    const std::string keep = tag_attribute(tag, "keep-together");
    if (!keep.empty()) paragraph.keep_together = keep != "auto" && keep != "false";
    const std::string keep_next = tag_attribute(tag, "keep-with-next");
    if (!keep_next.empty())
        paragraph.keep_with_next = keep_next != "auto" && keep_next != "false";
    const std::string break_before = tag_attribute(tag, "break-before");
    if (!break_before.empty()) paragraph.page_break_before = break_before == "page";
    const std::string border = tag_attribute(tag, "border-bottom");
    if (!border.empty()) paragraph.bottom_border = border != "none";
}

void parse_odf_styles_into(std::string_view xml, OdfStyleCatalog& catalog) {
    std::size_t position = 0;
    LocalElement element;
    while (next_local_element(xml, "default-style", position, element)) {
        const std::string family = tag_attribute(element.opening, "family");
        if (family == "text" || family == "paragraph")
            apply_odf_text_properties(catalog.default_run, element.block);
        if (family == "paragraph")
            apply_odf_paragraph_properties(catalog.default_paragraph,
                                           element.block);
        position = element.next;
    }
    position = 0;
    while (next_local_element(xml, "style", position, element)) {
        const std::string name = tag_attribute(element.opening, "name");
        const std::string family = tag_attribute(element.opening, "family");
        if (!name.empty() && (family == "text" || family == "paragraph")) {
            require_parse_limit(catalog.definitions.size() < kMaxStyles);
            OdfStyleDefinition definition;
            definition.family = family;
            definition.parent = tag_attribute(element.opening,
                                               "parent-style-name");
            if (const std::string_view props =
                    first_local_block(element.inner, "text-properties");
                !props.empty()) definition.text_properties.assign(props);
            if (const std::string_view props =
                    first_local_block(element.inner, "paragraph-properties");
                !props.empty()) definition.paragraph_properties.assign(props);
            catalog.definitions[name] = std::move(definition);
        }
        position = element.next;
    }
    position = 0;
    while (next_local_element(xml, "page-layout", position, element)) {
        LocalElement properties;
        if (next_local_element(element.inner, "page-layout-properties", 0,
                               properties)) {
            DocumentSettings settings;
            parse_odf_length(tag_attribute(properties.opening, "page-width"),
                             settings.page_width, 720, 63360);
            parse_odf_length(tag_attribute(properties.opening, "page-height"),
                             settings.page_height, 720, 63360);
            const std::string common = tag_attribute(properties.opening,
                                                     "margin");
            if (!common.empty()) {
                parse_odf_length(common, settings.margin_left, 0, 31680);
                settings.margin_right = settings.margin_top =
                    settings.margin_bottom = settings.margin_left;
            }
            parse_odf_length(tag_attribute(properties.opening, "margin-left"),
                             settings.margin_left, 0, 31680);
            parse_odf_length(tag_attribute(properties.opening, "margin-right"),
                             settings.margin_right, 0, 31680);
            parse_odf_length(tag_attribute(properties.opening, "margin-top"),
                             settings.margin_top, 0, 31680);
            parse_odf_length(tag_attribute(properties.opening, "margin-bottom"),
                             settings.margin_bottom, 0, 31680);
            settings.valid = settings.page_width > 0 && settings.page_height > 0 &&
                settings.margin_left + settings.margin_right < settings.page_width &&
                settings.margin_top + settings.margin_bottom < settings.page_height;
            if (settings.valid) catalog.settings = settings;
        }
        position = element.next;
    }
}

void apply_odf_style(const OdfStyleCatalog& catalog, const std::string& name,
                     RunStyle& run, Paragraph& paragraph,
                     const int depth = 0) {
    if (name.empty() || depth > 16) return;
    const auto found = catalog.definitions.find(name);
    if (found == catalog.definitions.end()) return;
    if (!found->second.parent.empty() && found->second.parent != name)
        apply_odf_style(catalog, found->second.parent, run, paragraph,
                        depth + 1);
    apply_odf_text_properties(run, found->second.text_properties);
    apply_odf_paragraph_properties(paragraph,
                                   found->second.paragraph_properties);
}

void append_odf_run(Paragraph& paragraph, const RunStyle& style,
                    std::wstring text, std::size_t& total_runs,
                    std::size_t& total_text) {
    if (text.empty()) return;
    require_parse_limit(text.size() <= kMaxTextBytes - total_text);
    total_text += text.size();
    if (!paragraph.runs.empty() && paragraph.runs.back().style == style) {
        paragraph.runs.back().text += text;
        return;
    }
    require_parse_limit(total_runs < kMaxRuns);
    paragraph.runs.push_back({style, std::move(text)});
    ++total_runs;
}

Paragraph parse_odf_paragraph(const LocalElement& element,
                              const OdfStyleCatalog& catalog,
                              std::size_t& total_runs,
                              std::size_t& total_text) {
    Paragraph paragraph = catalog.default_paragraph;
    RunStyle base = catalog.default_run;
    apply_odf_style(catalog, tag_attribute(element.opening, "style-name"),
                    base, paragraph);
    std::vector<RunStyle> styles{base};
    std::size_t position = 0;
    while (position < element.inner.size()) {
        const std::size_t tag_start = element.inner.find('<', position);
        const std::size_t text_end = tag_start == std::string_view::npos ?
            element.inner.size() : tag_start;
        if (text_end > position)
            append_odf_run(paragraph, styles.back(),
                           xml_unescape(element.inner.substr(position,
                                                             text_end - position)),
                           total_runs, total_text);
        if (tag_start == std::string_view::npos) break;
        const std::size_t tag_end = element.inner.find('>', tag_start + 1);
        if (tag_end == std::string_view::npos) break;
        const std::string_view tag = element.inner.substr(
            tag_start, tag_end - tag_start + 1);
        const std::string name = local_tag_name(tag);
        const bool closing = tag.size() > 1 && tag[1] == '/';
        const bool self_closing = tag.size() > 2 && tag[tag.size() - 2] == '/';
        if (name == "span") {
            if (closing) {
                if (styles.size() > 1) styles.pop_back();
            } else {
                RunStyle style = styles.back();
                Paragraph ignored;
                apply_odf_style(catalog, tag_attribute(tag, "style-name"),
                                style, ignored);
                styles.push_back(std::move(style));
                if (self_closing) styles.pop_back();
            }
        } else if (!closing && name == "s") {
            int count = 1;
            const std::string count_text = tag_attribute(tag, "c");
            if (!count_text.empty()) parse_bounded_int(count_text, 1, 100000,
                                                       count);
            append_odf_run(paragraph, styles.back(),
                           std::wstring(static_cast<std::size_t>(count), L' '),
                           total_runs, total_text);
        } else if (!closing && name == "tab") {
            append_odf_run(paragraph, styles.back(), L"\t", total_runs,
                           total_text);
        } else if (!closing && name == "line-break") {
            append_odf_run(paragraph, styles.back(), L"\n", total_runs,
                           total_text);
        } else if (!closing && name == "soft-page-break") {
            append_odf_run(paragraph, styles.back(), L"\f", total_runs,
                           total_text);
        }
        position = tag_end + 1;
    }
    return paragraph;
}

bool load_odt_paragraphs(const char* path,
                         std::vector<Paragraph>& paragraphs,
                         DocumentSettings* settings = nullptr,
                         std::vector<TableLayout>* tables = nullptr) {
    const std::wstring document_path = wide_path(path);
    std::string mimetype;
    std::string content;
    std::string styles;
    if (document_path.empty() ||
        !read_zip_entry(document_path, "mimetype", mimetype, 256) ||
        mimetype != "application/vnd.oasis.opendocument.text" ||
        !read_zip_entry(document_path, "content.xml", content,
                        kMaxDocumentXmlBytes)) return false;
    read_zip_entry(document_path, "styles.xml", styles, kMaxStylesXmlBytes);
    OdfStyleCatalog catalog;
    parse_odf_styles_into(styles, catalog);
    parse_odf_styles_into(content, catalog);
    LocalElement office_text;
    const std::string_view body = next_local_element(content, "text", 0,
                                                      office_text) ?
        office_text.inner : std::string_view(content);
    paragraphs.clear();
    if (tables != nullptr) tables->clear();
    std::size_t position = 0;
    std::size_t total_runs = 0;
    std::size_t total_text = 0;
    while (position < body.size()) {
        LocalElement paragraph_element;
        LocalElement heading_element;
        const bool has_paragraph = next_local_element(body, "p", position,
                                                       paragraph_element);
        const bool has_heading = next_local_element(body, "h", position,
                                                     heading_element);
        if (!has_paragraph && !has_heading) break;
        const LocalElement& chosen = !has_heading ||
            (has_paragraph && paragraph_element.block.data() <
                                heading_element.block.data()) ?
            paragraph_element : heading_element;
        require_parse_limit(paragraphs.size() < kMaxParagraphs);
        paragraphs.push_back(parse_odf_paragraph(chosen, catalog, total_runs,
                                                  total_text));
        position = chosen.next;
    }
    if (paragraphs.empty()) paragraphs.push_back({});
    if (settings != nullptr) *settings = catalog.settings;
    return true;
}

struct StreamCookie { const char* data; LONG length; LONG position; };
DWORD CALLBACK rich_edit_stream_in(DWORD_PTR cookie, LPBYTE buffer,
                                   LONG requested, LONG* copied) {
    if (cookie == 0 || buffer == nullptr || copied == nullptr ||
        requested <= 0) {
        if (copied != nullptr) *copied = 0;
        return 1;
    }
    auto& source = *reinterpret_cast<StreamCookie*>(cookie);
    if (source.data == nullptr || source.position < 0 ||
        source.position > source.length) {
        *copied = 0;
        return 1;
    }
    *copied = (std::min)(requested, source.length - source.position);
    if (*copied > 0) {
        std::memcpy(buffer, source.data + source.position, *copied);
        source.position += *copied;
    }
    return 0;
}

class RichEditDocument {
public:
    bool load(std::string_view rtf) {
        if (rtf.size() > kMaxRtfBytes ||
            rtf.size() > static_cast<std::size_t>(LONG_MAX)) return false;
        module_ = LoadLibraryExW(kMsfteditModule, nullptr,
                                 LOAD_LIBRARY_SEARCH_SYSTEM32);
        if (module_ == nullptr) return false;
        window_ = CreateWindowExW(0, MSFTEDIT_CLASS, nullptr,
                                  WS_POPUP | ES_MULTILINE, 0, 0, 100, 100,
                                  nullptr, nullptr,
                                  GetModuleHandleW(nullptr), nullptr);
        if (window_ == nullptr) return false;
        SendMessageW(window_, EM_EXLIMITTEXT, 0, 0x7ffffffe);
        StreamCookie cookie{rtf.data(), static_cast<LONG>(rtf.size()), 0};
        EDITSTREAM stream{reinterpret_cast<DWORD_PTR>(&cookie), 0,
                          rich_edit_stream_in};
        SendMessageW(window_, EM_STREAMIN, SF_RTF,
                     reinterpret_cast<LPARAM>(&stream));
        return stream.dwError == 0;
    }
    ~RichEditDocument() {
        if (window_ != nullptr) DestroyWindow(window_);
        if (module_ != nullptr) FreeLibrary(module_);
    }
    HWND window() const { return window_; }
    std::wstring text() const {
        const int length = GetWindowTextLengthW(window_);
        if (length <= 0) return {};
        std::basic_string<WCHAR> buffer(
            static_cast<std::size_t>(length) + 1, WCHAR{});
        const int copied = GetWindowTextW(window_, buffer.data(), length + 1);
        if (copied <= 0) return {};
        std::wstring value;
        value.reserve(static_cast<std::size_t>(copied));
        for (int index = 0; index < copied; ++index) {
            value.push_back(static_cast<wchar_t>(
                buffer[static_cast<std::size_t>(index)]));
        }
        return value;
    }
private:
    HMODULE module_ = nullptr;
    HWND window_ = nullptr;
};

RunStyle rich_style_at(HWND rich, LONG position) {
    CHARRANGE range{position, position + 1};
    SendMessageW(rich, EM_EXSETSEL, 0, reinterpret_cast<LPARAM>(&range));
    CHARFORMAT2W format{};
    format.cbSize = sizeof(format);
    SendMessageW(rich, EM_GETCHARFORMAT, SCF_SELECTION,
                 reinterpret_cast<LPARAM>(&format));
    RunStyle style;
    style.bold = (format.dwEffects & CFE_BOLD) != 0;
    style.italic = (format.dwEffects & CFE_ITALIC) != 0;
    style.underline = (format.dwEffects & CFE_UNDERLINE) != 0;
    style.half_points = format.yHeight > 0 ?
        (std::max)(2L, format.yHeight / 10) : 20;
    style.auto_color = (format.dwEffects & CFE_AUTOCOLOR) != 0;
    style.color = format.crTextColor;
    if ((format.dwMask & CFM_FACE) != 0 && format.szFaceName[0] != WCHAR{}) {
        style.font.clear();
        for (std::size_t index = 0;
             index < std::size(format.szFaceName) &&
             format.szFaceName[index] != WCHAR{}; ++index) {
            style.font.push_back(
                static_cast<wchar_t>(format.szFaceName[index]));
        }
    }
    if (format.lcid != 0) {
        const LanguageProfile& profile = profile_for_language_id(
            LANGIDFROMLCID(format.lcid));
        style.language = profile.tag;
        style.code_page = profile.code_page;
        style.charset = profile.charset;
    }
    return style;
}

int rich_alignment_at(HWND rich, LONG position) {
    CHARRANGE range{position, position};
    SendMessageW(rich, EM_EXSETSEL, 0, reinterpret_cast<LPARAM>(&range));
    PARAFORMAT2 format{};
    format.cbSize = sizeof(format);
    format.dwMask = PFM_ALIGNMENT;
    SendMessageW(rich, EM_GETPARAFORMAT, 0, reinterpret_cast<LPARAM>(&format));
    return format.wAlignment;
}

std::vector<Paragraph> paragraphs_from_rich_edit(RichEditDocument& rich) {
    const std::wstring text = rich.text();
    std::vector<Paragraph> paragraphs;
    LONG paragraph_start = 0;
    while (paragraph_start <= static_cast<LONG>(text.size())) {
        LONG paragraph_end = paragraph_start;
        while (paragraph_end < static_cast<LONG>(text.size()) &&
               text[paragraph_end] != L'\r' && text[paragraph_end] != L'\n')
            ++paragraph_end;
        Paragraph paragraph;
        paragraph.alignment = rich_alignment_at(rich.window(), paragraph_start);
        LONG run_start = paragraph_start;
        while (run_start < paragraph_end) {
            RunStyle style = rich_style_at(rich.window(), run_start);
            LONG run_end = run_start + 1;
            while (run_end < paragraph_end &&
                   rich_style_at(rich.window(), run_end) == style) ++run_end;
            paragraph.runs.push_back({style, text.substr(
                static_cast<std::size_t>(run_start),
                static_cast<std::size_t>(run_end - run_start))});
            run_start = run_end;
        }
        paragraphs.push_back(std::move(paragraph));
        if (paragraph_end >= static_cast<LONG>(text.size())) break;
        paragraph_start = paragraph_end + 1;
        if (paragraph_start < static_cast<LONG>(text.size()) &&
            text[paragraph_end] == L'\r' && text[paragraph_start] == L'\n')
            ++paragraph_start;
    }
    return paragraphs;
}

std::string run_text_xml(std::wstring_view text) {
    std::string xml;
    std::wstring ordinary;
    const auto flush = [&]() {
        if (!ordinary.empty()) {
            xml += "<w:t xml:space=\"preserve\">" + xml_escape(ordinary) +
                   "</w:t>";
            ordinary.clear();
        }
    };
    for (const wchar_t character : text) {
        if (character == L'\t' || character == L'\n' || character == L'\r' ||
            character == L'\f') {
            flush();
            if (character == L'\t') xml += "<w:tab/>";
            else if (character == L'\f') xml += "<w:br w:type=\"page\"/>";
            else if (character == L'\n') xml += "<w:br/>";
        } else {
            ordinary.push_back(character);
        }
    }
    flush();
    return xml;
}

std::string document_xml(const std::vector<Paragraph>& paragraphs,
                         const DocumentSettings& settings = {}) {
    std::string xml =
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
        "<w:document xmlns:w=\"http://schemas.openxmlformats.org/wordprocessingml/2006/main\">"
        "<w:body>";
    for (const auto& paragraph : paragraphs) {
        xml += "<w:p>";
        xml += "<w:pPr>";
        if (paragraph.alignment != PFA_LEFT) {
            const char* value = paragraph.alignment == PFA_CENTER ? "center" :
                paragraph.alignment == PFA_RIGHT ? "right" : "both";
            xml += std::string("<w:jc w:val=\"") + value + "\"/>";
        }
        if (paragraph.left_indent != 0 || paragraph.right_indent != 0 ||
            paragraph.first_line_indent != 0) {
            xml += "<w:ind w:left=\"" +
                   std::to_string(paragraph.left_indent) + "\" w:right=\"" +
                   std::to_string(paragraph.right_indent) + "\"";
            if (paragraph.first_line_indent < 0)
                xml += " w:hanging=\"" +
                    std::to_string(-paragraph.first_line_indent) + "\"";
            else
                xml += " w:firstLine=\"" +
                    std::to_string(paragraph.first_line_indent) + "\"";
            xml += "/>";
        }
        if (paragraph.space_before != 0 || paragraph.space_after != 0 ||
            paragraph.line_spacing != 0) {
            xml += "<w:spacing w:before=\"" +
                   std::to_string(paragraph.space_before) +
                   "\" w:after=\"" + std::to_string(paragraph.space_after) +
                   "\"";
            if (paragraph.line_spacing < 0) {
                xml += " w:line=\"" +
                       std::to_string(-paragraph.line_spacing) +
                       "\" w:lineRule=\"exact\"";
            } else if (paragraph.line_spacing > 0) {
                xml += " w:line=\"" +
                       std::to_string(paragraph.line_spacing) +
                       "\" w:lineRule=\"atLeast\"";
            }
            xml += "/>";
        }
        if (paragraph.keep_together) xml += "<w:keepLines/>";
        if (paragraph.keep_with_next) xml += "<w:keepNext/>";
        if (paragraph.page_break_before) xml += "<w:pageBreakBefore/>";
        if (paragraph.bottom_border)
            xml += "<w:pBdr><w:bottom w:val=\"single\" w:sz=\"4\"/></w:pBdr>";
        xml += "</w:pPr>";
        for (const auto& run : paragraph.runs) {
            xml += "<w:r><w:rPr>";
            if (run.style.bold) xml += "<w:b/>";
            if (run.style.italic) xml += "<w:i/>";
            if (run.style.underline) xml += "<w:u w:val=\"single\"/>";
            if (run.style.strike) xml += "<w:strike/>";
            if (run.style.small_caps) xml += "<w:smallCaps/>";
            if (run.style.all_caps) xml += "<w:caps/>";
            if (run.style.hidden) xml += "<w:vanish/>";
            xml += "<w:rFonts w:ascii=\"" + xml_escape(run.style.font) +
                   "\" w:hAnsi=\"" + xml_escape(run.style.font) +
                   "\" w:eastAsia=\"" + xml_escape(run.style.font) +
                   "\" w:cs=\"" + xml_escape(run.style.font) + "\"/>";
            if (!run.style.language.empty()) {
                xml += "<w:lang w:val=\"" + run.style.language +
                       "\" w:eastAsia=\"" + run.style.language +
                       "\" w:bidi=\"" + run.style.language + "\"/>";
            }
            xml += "<w:sz w:val=\"" + std::to_string(run.style.half_points) + "\"/>";
            if (!run.style.auto_color) {
                char color[7]{};
                wsprintfA(color, "%02X%02X%02X", GetRValue(run.style.color),
                          GetGValue(run.style.color), GetBValue(run.style.color));
                xml += std::string("<w:color w:val=\"") + color + "\"/>";
            }
            xml += "</w:rPr>" + run_text_xml(run.text) + "</w:r>";
        }
        xml += "</w:p>";
    }
    const int page_width = settings.valid ? settings.page_width : 12240;
    const int page_height = settings.valid ? settings.page_height : 15840;
    const int margin_left = settings.valid ? settings.margin_left : 1440;
    const int margin_right = settings.valid ? settings.margin_right : 1440;
    const int margin_top = settings.valid ? settings.margin_top : 1440;
    const int margin_bottom = settings.valid ? settings.margin_bottom : 1440;
    xml += "<w:sectPr><w:pgSz w:w=\"" + std::to_string(page_width) +
           "\" w:h=\"" + std::to_string(page_height) +
           "\"/><w:pgMar w:top=\"" + std::to_string(margin_top) +
           "\" w:right=\"" + std::to_string(margin_right) +
           "\" w:bottom=\"" + std::to_string(margin_bottom) +
           "\" w:left=\"" + std::to_string(margin_left) +
           "\"/></w:sectPr></w:body></w:document>";
    return xml;
}

bool add_part(IOpcFactory* factory, IOpcPartSet* parts, const WCHAR* name,
              const WCHAR* content_type, std::string_view data,
              IOpcPart** created = nullptr) {
    ComPtr<IOpcPartUri> uri;
    ComPtr<IOpcPart> part;
    ComPtr<IStream> stream;
    ULONG written = 0;
    if (FAILED(factory->CreatePartUri(name, &uri)) ||
        FAILED(parts->CreatePart(uri.Get(), content_type,
                                 OPC_COMPRESSION_NORMAL, &part)) ||
        FAILED(part->GetContentStream(&stream)) ||
        (!data.empty() && (FAILED(stream->Write(data.data(),
            static_cast<ULONG>(data.size()), &written)) || written != data.size())))
        return false;
    if (created != nullptr) *created = part.Detach();
    return true;
}

bool add_relationship(IOpcRelationshipSet* set, IOpcFactory* factory,
                      const WCHAR* target, const WCHAR* type) {
    ComPtr<IOpcPartUri> uri;
    ComPtr<IOpcRelationship> relationship;
    return SUCCEEDED(factory->CreatePartUri(target, &uri)) &&
           SUCCEEDED(set->CreateRelationship(nullptr, type, uri.Get(),
                                             OPC_URI_TARGET_MODE_INTERNAL,
                                             &relationship));
}

bool write_docx(const std::wstring& path, const std::vector<Paragraph>& paragraphs,
                const DocumentSettings& settings = {}) {
    ComApartment apartment;
    if (!apartment.usable()) return false;
    ComPtr<IOpcFactory> factory;
    ComPtr<IOpcPackage> package;
    ComPtr<IOpcPartSet> parts;
    ComPtr<IOpcPart> main_part;
    ComPtr<IOpcRelationshipSet> package_relationships;
    ComPtr<IOpcRelationshipSet> document_relationships;
    ComPtr<IStream> output;
    if (FAILED(CoCreateInstance(__uuidof(OpcFactory), nullptr,
                                CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory))) ||
        FAILED(factory->CreatePackage(&package)) ||
        FAILED(package->GetPartSet(&parts)) ||
        !add_part(factory.Get(), parts.Get(), OPUSW("/word/document.xml"),
                  OPUSW("application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml"),
                  document_xml(paragraphs, settings), &main_part)) return false;

    const std::string styles =
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
        "<w:styles xmlns:w=\"http://schemas.openxmlformats.org/wordprocessingml/2006/main\">"
        "<w:style w:type=\"paragraph\" w:default=\"1\" w:styleId=\"Normal\">"
        "<w:name w:val=\"Normal\"/><w:qFormat/></w:style></w:styles>";
    const std::string settings_xml =
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
        "<w:settings xmlns:w=\"http://schemas.openxmlformats.org/wordprocessingml/2006/main\"/>";
    const std::string core =
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
        "<cp:coreProperties xmlns:cp=\"http://schemas.openxmlformats.org/package/2006/metadata/core-properties\" "
        "xmlns:dc=\"http://purl.org/dc/elements/1.1/\"><dc:creator>Microsoft Word</dc:creator>"
        "<dc:title>Document</dc:title></cp:coreProperties>";
    const std::string app =
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
        "<Properties xmlns=\"http://schemas.openxmlformats.org/officeDocument/2006/extended-properties\">"
        "<Application>Microsoft Word</Application></Properties>";
    if (!add_part(factory.Get(), parts.Get(), OPUSW("/word/styles.xml"),
                  OPUSW("application/vnd.openxmlformats-officedocument.wordprocessingml.styles+xml"), styles) ||
        !add_part(factory.Get(), parts.Get(), OPUSW("/word/settings.xml"),
                  OPUSW("application/vnd.openxmlformats-officedocument.wordprocessingml.settings+xml"), settings_xml) ||
        !add_part(factory.Get(), parts.Get(), OPUSW("/docProps/core.xml"),
                  OPUSW("application/vnd.openxmlformats-package.core-properties+xml"), core) ||
        !add_part(factory.Get(), parts.Get(), OPUSW("/docProps/app.xml"),
                  OPUSW("application/vnd.openxmlformats-officedocument.extended-properties+xml"), app) ||
        FAILED(package->GetRelationshipSet(&package_relationships)) ||
        !add_relationship(package_relationships.Get(), factory.Get(),
                          OPUSW("/word/document.xml"),
                          OPUSW("http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument")) ||
        !add_relationship(package_relationships.Get(), factory.Get(),
                          OPUSW("/docProps/core.xml"),
                          OPUSW("http://schemas.openxmlformats.org/package/2006/relationships/metadata/core-properties")) ||
        !add_relationship(package_relationships.Get(), factory.Get(),
                          OPUSW("/docProps/app.xml"),
                          OPUSW("http://schemas.openxmlformats.org/officeDocument/2006/relationships/extended-properties")) ||
        FAILED(main_part->GetRelationshipSet(&document_relationships)) ||
        !add_relationship(document_relationships.Get(), factory.Get(),
                          OPUSW("/word/styles.xml"),
                          OPUSW("http://schemas.openxmlformats.org/officeDocument/2006/relationships/styles")) ||
        !add_relationship(document_relationships.Get(), factory.Get(),
                          OPUSW("/word/settings.xml"),
                          OPUSW("http://schemas.openxmlformats.org/officeDocument/2006/relationships/settings")))
        return false;
    if (FAILED(CreateStreamOnHGlobal(nullptr, TRUE, &output)) ||
        FAILED(factory->WritePackageToStream(package.Get(),
                                             OPC_WRITE_DEFAULT,
                                             output.Get()))) return false;
    std::string package_bytes;
    return read_stream(output.Get(), package_bytes, kMaxGeneratedBytes) &&
           write_bytes(path, package_bytes);
}

bool rtf_to_docx(const std::wstring& rtf_path, const std::wstring& docx_path) {
    std::string rtf;
    RichEditDocument rich;
    return read_bytes(rtf_path, rtf) && rich.load(rtf) &&
           write_docx(docx_path, paragraphs_from_rich_edit(rich));
}

std::string odf_length(const int twips) {
    char buffer[48]{};
    std::snprintf(buffer, std::size(buffer), "%.4fin",
                  static_cast<double>(twips) / 1440.0);
    return buffer;
}

std::string odf_color(const COLORREF color) {
    char buffer[8]{};
    std::snprintf(buffer, std::size(buffer), "#%02X%02X%02X",
                  GetRValue(color), GetGValue(color), GetBValue(color));
    return buffer;
}

std::string odf_language_attributes(std::string_view language) {
    if (language.empty()) return {};
    const std::size_t dash = language.find('-');
    std::string result = " fo:language=\"" +
        std::string(language.substr(0, dash)) + "\"";
    if (dash != std::string_view::npos && dash + 1 < language.size())
        result += " fo:country=\"" + std::string(language.substr(dash + 1)) +
                  "\"";
    return result;
}

std::string odf_text_markup(std::wstring_view text) {
    std::string xml;
    std::wstring ordinary;
    const auto flush = [&]() {
        if (!ordinary.empty()) {
            xml += xml_escape(ordinary);
            ordinary.clear();
        }
    };
    std::size_t index = 0;
    while (index < text.size()) {
        const wchar_t character = text[index++];
        if (character == L' ' || character == L'\t' || character == L'\n' ||
            character == L'\r' || character == L'\f') {
            flush();
            if (character == L' ') {
                std::size_t count = 1;
                while (index < text.size() && text[index] == L' ') {
                    ++count;
                    ++index;
                }
                xml += count == 1 ? "<text:s/>" :
                    "<text:s text:c=\"" + std::to_string(count) + "\"/>";
            } else if (character == L'\t') xml += "<text:tab/>";
            else if (character == L'\f') xml += "<text:soft-page-break/>";
            else if (character == L'\n') xml += "<text:line-break/>";
        } else {
            ordinary.push_back(character);
        }
    }
    flush();
    return xml;
}

std::string odf_paragraph_style(const Paragraph& paragraph,
                                const std::size_t index) {
    std::string xml = "<style:style style:name=\"P" +
        std::to_string(index) + "\" style:family=\"paragraph\">";
    xml += "<style:paragraph-properties";
    if (paragraph.alignment != PFA_LEFT) {
        const char* alignment = paragraph.alignment == PFA_CENTER ? "center" :
            paragraph.alignment == PFA_RIGHT ? "right" : "justify";
        xml += std::string(" fo:text-align=\"") + alignment + "\"";
    }
    if (paragraph.left_indent != 0)
        xml += " fo:margin-left=\"" + odf_length(paragraph.left_indent) + "\"";
    if (paragraph.right_indent != 0)
        xml += " fo:margin-right=\"" + odf_length(paragraph.right_indent) + "\"";
    if (paragraph.first_line_indent != 0)
        xml += " fo:text-indent=\"" +
               odf_length(paragraph.first_line_indent) + "\"";
    if (paragraph.space_before != 0)
        xml += " fo:margin-top=\"" + odf_length(paragraph.space_before) + "\"";
    if (paragraph.space_after != 0)
        xml += " fo:margin-bottom=\"" + odf_length(paragraph.space_after) + "\"";
    if (paragraph.line_spacing != 0)
        xml += " fo:line-height=\"" + odf_length(paragraph.line_spacing) + "\"";
    if (paragraph.keep_together) xml += " fo:keep-together=\"always\"";
    if (paragraph.keep_with_next) xml += " fo:keep-with-next=\"always\"";
    if (paragraph.page_break_before) xml += " fo:break-before=\"page\"";
    if (paragraph.bottom_border)
        xml += " fo:border-bottom=\"0.5pt solid #000000\"";
    xml += "/></style:style>";
    return xml;
}

std::string odf_run_style(const RunStyle& style, const std::size_t index) {
    std::string xml = "<style:style style:name=\"T" +
        std::to_string(index) + "\" style:family=\"text\">";
    xml += "<style:text-properties style:font-name=\"" +
           xml_escape(style.font) + "\" fo:font-family=\"" +
           xml_escape(style.font) + "\" fo:font-size=\"" +
           std::to_string((std::max)(2, style.half_points) / 2.0) + "pt\"";
    if (style.bold) xml += " fo:font-weight=\"bold\"";
    if (style.italic) xml += " fo:font-style=\"italic\"";
    if (style.underline)
        xml += " style:text-underline-style=\"solid\"";
    if (style.strike)
        xml += " style:text-line-through-style=\"solid\"";
    if (style.small_caps) xml += " fo:font-variant=\"small-caps\"";
    if (style.all_caps) xml += " fo:text-transform=\"uppercase\"";
    if (style.hidden) xml += " text:display=\"none\"";
    if (!style.auto_color) xml += " fo:color=\"" + odf_color(style.color) + "\"";
    xml += odf_language_attributes(style.language);
    xml += "/></style:style>";
    return xml;
}

std::string odf_content_xml(const std::vector<Paragraph>& paragraphs) {
    std::string styles;
    std::string body;
    std::size_t run_index = 0;
    for (std::size_t paragraph_index = 0;
         paragraph_index < paragraphs.size(); ++paragraph_index) {
        const Paragraph& paragraph = paragraphs[paragraph_index];
        styles += odf_paragraph_style(paragraph, paragraph_index);
        body += "<text:p text:style-name=\"P" +
                std::to_string(paragraph_index) + "\">";
        for (const TextRun& run : paragraph.runs) {
            styles += odf_run_style(run.style, run_index);
            body += "<text:span text:style-name=\"T" +
                    std::to_string(run_index++) + "\">" +
                    odf_text_markup(run.text) + "</text:span>";
        }
        body += "</text:p>";
    }
    return
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
        "<office:document-content office:version=\"1.3\" "
        "xmlns:office=\"urn:oasis:names:tc:opendocument:xmlns:office:1.0\" "
        "xmlns:style=\"urn:oasis:names:tc:opendocument:xmlns:style:1.0\" "
        "xmlns:text=\"urn:oasis:names:tc:opendocument:xmlns:text:1.0\" "
        "xmlns:fo=\"urn:oasis:names:tc:opendocument:xmlns:xsl-fo-compatible:1.0\">"
        "<office:automatic-styles>" + styles +
        "</office:automatic-styles><office:body><office:text>" + body +
        "</office:text></office:body></office:document-content>";
}

std::string odf_styles_xml(const DocumentSettings& source) {
    const int page_width = source.valid ? source.page_width : 12240;
    const int page_height = source.valid ? source.page_height : 15840;
    const int margin_left = source.valid ? source.margin_left : 1440;
    const int margin_right = source.valid ? source.margin_right : 1440;
    const int margin_top = source.valid ? source.margin_top : 1440;
    const int margin_bottom = source.valid ? source.margin_bottom : 1440;
    return
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
        "<office:document-styles office:version=\"1.3\" "
        "xmlns:office=\"urn:oasis:names:tc:opendocument:xmlns:office:1.0\" "
        "xmlns:style=\"urn:oasis:names:tc:opendocument:xmlns:style:1.0\" "
        "xmlns:text=\"urn:oasis:names:tc:opendocument:xmlns:text:1.0\" "
        "xmlns:fo=\"urn:oasis:names:tc:opendocument:xmlns:xsl-fo-compatible:1.0\">"
        "<office:styles><style:default-style style:family=\"paragraph\">"
        "<style:text-properties style:font-name=\"Arial\" "
        "fo:font-family=\"Arial\" fo:font-size=\"10pt\"/>"
        "</style:default-style></office:styles><office:automatic-styles>"
        "<style:page-layout style:name=\"pm1\"><style:page-layout-properties "
        "fo:page-width=\"" + odf_length(page_width) +
        "\" fo:page-height=\"" + odf_length(page_height) +
        "\" fo:margin-left=\"" + odf_length(margin_left) +
        "\" fo:margin-right=\"" + odf_length(margin_right) +
        "\" fo:margin-top=\"" + odf_length(margin_top) +
        "\" fo:margin-bottom=\"" + odf_length(margin_bottom) +
        "\"/></style:page-layout></office:automatic-styles>"
        "<office:master-styles><style:master-page style:name=\"Standard\" "
        "style:page-layout-name=\"pm1\"/></office:master-styles>"
        "</office:document-styles>";
}

bool write_odt(const std::wstring& path,
               const std::vector<Paragraph>& paragraphs,
               const DocumentSettings& settings = {}) {
    static constexpr const char* mime =
        "application/vnd.oasis.opendocument.text";
    const std::string manifest =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
        "<manifest:manifest manifest:version=\"1.3\" "
        "xmlns:manifest=\"urn:oasis:names:tc:opendocument:xmlns:manifest:1.0\">"
        "<manifest:file-entry manifest:full-path=\"/\" "
        "manifest:media-type=\"application/vnd.oasis.opendocument.text\"/>"
        "<manifest:file-entry manifest:full-path=\"content.xml\" "
        "manifest:media-type=\"text/xml\"/>"
        "<manifest:file-entry manifest:full-path=\"styles.xml\" "
        "manifest:media-type=\"text/xml\"/>"
        "<manifest:file-entry manifest:full-path=\"meta.xml\" "
        "manifest:media-type=\"text/xml\"/>"
        "</manifest:manifest>";
    const std::string meta =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
        "<office:document-meta office:version=\"1.3\" "
        "xmlns:office=\"urn:oasis:names:tc:opendocument:xmlns:office:1.0\" "
        "xmlns:meta=\"urn:oasis:names:tc:opendocument:xmlns:meta:1.0\">"
        "<office:meta><meta:generator>Microsoft Word 1.1</meta:generator>"
        "</office:meta></office:document-meta>";
    return write_stored_zip(path, {
        {"mimetype", mime},
        {"content.xml", odf_content_xml(paragraphs)},
        {"styles.xml", odf_styles_xml(settings)},
        {"meta.xml", meta},
        {"META-INF/manifest.xml", manifest}});
}

bool rtf_to_odt(const std::wstring& rtf_path, const std::wstring& odt_path) {
    std::string rtf;
    RichEditDocument rich;
    return read_bytes(rtf_path, rtf) && rich.load(rtf) &&
           write_odt(odt_path, paragraphs_from_rich_edit(rich));
}

struct PdfLineRun {
    RunStyle style;
    std::wstring text;
    double width = 0.0;
};

struct PdfLine {
    std::vector<PdfLineRun> runs;
    double width = 0.0;
    double largest_font = 0.0;
    double left = 72.0;
    double writable_width = 468.0;
    int alignment = PFA_LEFT;
};

std::string pdf_number(const double value) {
    char buffer[64]{};
    std::snprintf(buffer, std::size(buffer), "%.3f", value);
    std::string result = buffer;
    while (result.size() > 1 && result.back() == '0') result.pop_back();
    if (!result.empty() && result.back() == '.') result.pop_back();
    return result;
}

double pdf_font_size(const RunStyle& style) {
    return (std::max)(4.0, static_cast<double>(style.half_points) / 2.0);
}

double pdf_character_width(const wchar_t character, const RunStyle& style) {
    double em = 0.556;
    if (character == L' ') em = 0.278;
    else if (character == L'i' || character == L'l' || character == L'I' ||
             character == L'.' || character == L',' || character == L'!' ||
             character == L'\'' || character == L'`' || character == L'|' ||
             character == L':' || character == L';') em = 0.278;
    else if (character == L'm' || character == L'w' || character == L'M' ||
             character == L'W' || character == L'@' || character == L'%' ||
             character == L'&') em = 0.889;
    else if (character >= L'A' && character <= L'Z') em = 0.667;
    else if (character >= L'a' && character <= L'z') em = 0.500;
    else if (character == L'(' || character == L')' || character == L'[' ||
             character == L']' || character == L'{' || character == L'}')
        em = 0.333;
    if (style.bold) em *= 1.03;
    return em * pdf_font_size(style);
}

std::wstring lower_ascii(std::wstring value) {
    for (wchar_t& character : value) {
        if (character >= L'A' && character <= L'Z') character += L'a' - L'A';
    }
    return value;
}

int pdf_font_resource(const RunStyle& style) {
    const std::wstring font = lower_ascii(style.font);
    int family = 0;
    if (font.find(L"times") != std::wstring::npos ||
        font.find(L"roman") != std::wstring::npos ||
        font.find(L"serif") != std::wstring::npos) {
        family = 1;
    } else if (font.find(L"courier") != std::wstring::npos ||
               font.find(L"mono") != std::wstring::npos) {
        family = 2;
    }
    const int face = (style.bold ? 1 : 0) + (style.italic ? 2 : 0);
    return family * 4 + face + 1;
}

bool pdf_needs_unicode_font(const std::vector<Paragraph>& paragraphs) {
    for (const Paragraph& paragraph : paragraphs) {
        for (const TextRun& run : paragraph.runs) {
            for (const wchar_t character : run.text) {
                if (character > 0xff) return true;
            }
        }
    }
    return false;
}

std::wstring pdf_unicode_font_name(const std::vector<Paragraph>& paragraphs) {
    for (const Paragraph& paragraph : paragraphs) {
        for (const TextRun& run : paragraph.runs) {
            for (std::size_t index = 0; index < run.text.size();) {
                const std::uint32_t scalar = scalar_at(run.text, index);
                const LanguageProfile& profile = profile_for_scalar(scalar);
                if (profile.code_page == 932) return L"Yu Gothic UI";
                if (profile.code_page == 936) return L"Microsoft YaHei UI";
                if (profile.code_page == 950) return L"Microsoft JhengHei UI";
                if (profile.code_page == 949) return L"Malgun Gothic";
                if (scalar > 0xffff) return L"Segoe UI Symbol";
            }
        }
    }
    return L"Segoe UI";
}

std::string pdf_font_identifier(std::wstring_view value) {
    std::string identifier;
    for (const wchar_t character : value) {
        if ((character >= L'A' && character <= L'Z') ||
            (character >= L'a' && character <= L'z') ||
            (character >= L'0' && character <= L'9'))
            identifier.push_back(static_cast<char>(character));
    }
    return identifier.empty() ? "SegoeUI" : identifier;
}

struct PdfUnicodeFont {
    HDC dc = nullptr;
    HFONT font = nullptr;
    HGDIOBJ previous = nullptr;
    std::wstring face;
    std::vector<unsigned char> file;
    std::map<WORD, std::wstring> unicode_by_glyph;
    TEXTMETRICW metrics{};

    ~PdfUnicodeFont() {
        if (dc != nullptr && previous != nullptr) SelectObject(dc, previous);
        if (font != nullptr) DeleteObject(font);
        if (dc != nullptr) DeleteDC(dc);
    }

    bool initialize(const std::wstring& requested) {
        face = requested;
        dc = CreateCompatibleDC(nullptr);
        if (dc == nullptr) return false;
        std::basic_string<WCHAR> requested_face;
        requested_face.reserve(face.size());
        for (const wchar_t code_unit : face) {
            requested_face.push_back(static_cast<WCHAR>(
                static_cast<std::uint16_t>(code_unit)));
        }
        font = CreateFontW(-1000, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                           DEFAULT_CHARSET, OUT_TT_ONLY_PRECIS,
                           CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                           DEFAULT_PITCH | FF_DONTCARE,
                           requested_face.c_str());
        if (font == nullptr) return false;
        previous = SelectObject(dc, font);
        if (previous == nullptr || !GetTextMetricsW(dc, &metrics)) return false;
        WCHAR actual[LF_FACESIZE]{};
        const int copied = GetTextFaceW(
            dc, static_cast<int>(std::size(actual)), actual);
        if (copied > 0) {
            face.clear();
            const std::size_t limit = (std::min)(
                static_cast<std::size_t>(copied), std::size(actual));
            for (std::size_t index = 0;
                 index < limit && actual[index] != WCHAR{}; ++index) {
                face.push_back(static_cast<wchar_t>(actual[index]));
            }
        }
        const DWORD size = GetFontData(dc, 0, 0, nullptr, 0);
        if (size == GDI_ERROR || size == 0 || size > kMaxGeneratedBytes)
            return false;
        file.resize(size);
        return GetFontData(dc, 0, 0, file.data(), size) != GDI_ERROR;
    }

    WORD glyph(std::wstring_view characters) {
        WORD glyphs[2]{0xffff, 0xffff};
        const int count = (std::min)(2, static_cast<int>(characters.size()));
        if (count <= 0) return 0;
        WCHAR glyph_characters[2]{};
        for (int index = 0; index < count; ++index) {
            const std::size_t offset = static_cast<std::size_t>(index);
            glyph_characters[index] = static_cast<WCHAR>(
                static_cast<std::uint16_t>(characters[offset]));
        }
        if (GetGlyphIndicesW(dc, glyph_characters, count,
                             glyphs, GGI_MARK_NONEXISTING_GLYPHS) == GDI_ERROR)
            return 0;
        WORD selected = glyphs[0] == 0xffff ? 0 : glyphs[0];
        if (selected == 0 && count == 2 && glyphs[1] != 0xffff)
            selected = glyphs[1];
        unicode_by_glyph[selected] = std::wstring(characters);
        return selected;
    }
};

std::string pdf_hex_word(const WORD value) {
    static constexpr char digits[] = "0123456789ABCDEF";
    std::string result(4, '0');
    result[0] = digits[(value >> 12) & 0xf];
    result[1] = digits[(value >> 8) & 0xf];
    result[2] = digits[(value >> 4) & 0xf];
    result[3] = digits[value & 0xf];
    return result;
}

std::string pdf_hex_utf16(std::wstring_view value) {
    std::string result;
    for (const wchar_t character : value)
        result += pdf_hex_word(static_cast<WORD>(character));
    return result;
}

std::string pdf_unicode_encoded_text(std::wstring_view text,
                                     PdfUnicodeFont& font) {
    std::string encoded = "<";
    for (std::size_t index = 0; index < text.size();) {
        const std::size_t first = index;
        const std::uint32_t scalar = scalar_at(text, index);
        (void)scalar;
        const std::wstring_view characters = text.substr(first, index - first);
        encoded += pdf_hex_word(font.glyph(characters));
    }
    encoded += ">";
    return encoded;
}

std::string pdf_to_unicode_cmap(const PdfUnicodeFont& font) {
    std::string cmap =
        "/CIDInit /ProcSet findresource begin\n12 dict begin\nbegincmap\n"
        "/CIDSystemInfo << /Registry (Adobe) /Ordering (UCS) /Supplement 0 >> def\n"
        "/CMapName /Word1Unicode def\n/CMapType 2 def\n"
        "1 begincodespacerange\n<0000> <FFFF>\nendcodespacerange\n";
    auto current = font.unicode_by_glyph.begin();
    while (current != font.unicode_by_glyph.end()) {
        const std::size_t count = (std::min)(
            static_cast<std::size_t>(100),
            static_cast<std::size_t>(std::distance(
                current, font.unicode_by_glyph.end())));
        cmap += std::to_string(count) + " beginbfchar\n";
        for (std::size_t index = 0; index < count; ++index, ++current) {
            cmap += "<" + pdf_hex_word(current->first) + "> <" +
                    pdf_hex_utf16(current->second) + ">\n";
        }
        cmap += "endbfchar\n";
    }
    cmap += "endcmap\nCMapName currentdict /CMap defineresource pop\n"
            "end\nend\n";
    return cmap;
}

std::string pdf_encoded_text(std::wstring_view text) {
    std::string encoded;
    for (const wchar_t character : text) {
        char byte = '?';
        BOOL used_default = FALSE;
        if (character >= 0x20) {
            WideCharToMultiByte(1252, WC_NO_BEST_FIT_CHARS, &character, 1,
                                &byte, 1, "?", &used_default);
        } else {
            continue;
        }
        if (byte == '\\' || byte == '(' || byte == ')') encoded.push_back('\\');
        encoded.push_back(byte);
    }
    return encoded;
}

void append_pdf_line(std::string& content, const PdfLine& line,
                     const double baseline, PdfUnicodeFont* unicode_font) {
    double x = line.left;
    if (line.alignment == PFA_CENTER) {
        x += ((std::max)(0.0, line.writable_width - line.width)) / 2.0;
    } else if (line.alignment == PFA_RIGHT) {
        x += (std::max)(0.0, line.writable_width - line.width);
    }
    std::string underlines;
    content += "BT\n1 0 0 1 " + pdf_number(x) + " " +
               pdf_number(baseline) + " Tm\n";
    for (const PdfLineRun& run : line.runs) {
        const double font_size = pdf_font_size(run.style);
        const COLORREF color = run.style.auto_color ? RGB(0, 0, 0) :
                                                     run.style.color;
        const double red = static_cast<double>(GetRValue(color)) / 255.0;
        const double green = static_cast<double>(GetGValue(color)) / 255.0;
        const double blue = static_cast<double>(GetBValue(color)) / 255.0;
        const bool unicode = unicode_font != nullptr &&
            std::any_of(run.text.begin(), run.text.end(),
                        [](const wchar_t character) { return character > 0xff; });
        content += unicode ? "/FU " :
            "/F" + std::to_string(pdf_font_resource(run.style)) + " ";
        content += pdf_number(font_size) + " Tf\n";
        content += pdf_number(red) + " " + pdf_number(green) + " " +
                   pdf_number(blue) + " rg\n";
        content += unicode ? pdf_unicode_encoded_text(run.text, *unicode_font) :
            "(" + pdf_encoded_text(run.text) + ")";
        content += " Tj\n";
        if (run.style.underline && run.width > 0.0) {
            const double underline_y = baseline - (std::max)(1.0, font_size / 12.0);
            underlines += "q\n" + pdf_number(red) + " " + pdf_number(green) +
                          " " + pdf_number(blue) + " RG\n" +
                          pdf_number((std::max)(0.5, font_size / 18.0)) +
                          " w\n" + pdf_number(x) + " " +
                          pdf_number(underline_y) + " m\n" +
                          pdf_number(x + run.width) + " " +
                          pdf_number(underline_y) + " l\nS\nQ\n";
        }
        x += run.width;
    }
    content += "ET\n" + underlines;
}

std::vector<std::string> pdf_page_contents(
    const std::vector<Paragraph>& paragraphs,
    const DocumentSettings& settings, PdfUnicodeFont* unicode_font) {
    const double page_width = settings.valid ?
        static_cast<double>(settings.page_width) / 20.0 : 612.0;
    const double page_height = settings.valid ?
        static_cast<double>(settings.page_height) / 20.0 : 792.0;
    const double margin_left = settings.valid ?
        static_cast<double>(settings.margin_left) / 20.0 : 72.0;
    const double margin_right = settings.valid ?
        static_cast<double>(settings.margin_right) / 20.0 : 72.0;
    const double margin_top = settings.valid ?
        static_cast<double>(settings.margin_top) / 20.0 : 72.0;
    const double margin_bottom = settings.valid ?
        static_cast<double>(settings.margin_bottom) / 20.0 : 72.0;
    const double top = (std::max)(margin_bottom + 36.0,
                                  page_height - margin_top);
    const double bottom = (std::max)(18.0, margin_bottom);
    std::vector<std::string> pages(1);
    double cursor = top;

    const auto begin_new_page = [&]() {
        pages.emplace_back();
        cursor = top;
    };

    bool first_paragraph = true;
    for (const Paragraph& paragraph : paragraphs) {
        if (paragraph.page_break_before && !first_paragraph) begin_new_page();
        first_paragraph = false;
        cursor -= (std::max)(0.0,
            static_cast<double>(paragraph.space_before) / 20.0);
        if (cursor <= bottom + 12.0) begin_new_page();

        PdfLine line;
        bool first_visual_line = true;
        const auto reset_line = [&]() {
            const double first_indent = first_visual_line ?
                static_cast<double>(paragraph.first_line_indent) / 20.0 : 0.0;
            line.alignment = paragraph.alignment;
            line.left = margin_left +
                static_cast<double>(paragraph.left_indent) / 20.0 +
                first_indent;
            const double right = page_width - margin_right -
                static_cast<double>(paragraph.right_indent) / 20.0;
            line.writable_width = (std::max)(36.0, right - line.left);
        };
        reset_line();
        const auto emit_line = [&](const bool force) {
            if (!force && line.runs.empty()) return;
            const double largest_font = line.largest_font > 0.0 ?
                line.largest_font : 12.0;
            double line_height = (std::max)(largest_font * 1.20, 4.8);
            if (paragraph.line_spacing < 0) {
                line_height = (std::max)(4.8,
                    -static_cast<double>(paragraph.line_spacing) / 20.0);
            } else if (paragraph.line_spacing > 0) {
                line_height = (std::max)(line_height,
                    static_cast<double>(paragraph.line_spacing) / 20.0);
            }
            if (cursor - line_height < bottom) begin_new_page();
            const double baseline = cursor - largest_font;
            append_pdf_line(pages.back(), line, baseline, unicode_font);
            cursor -= line_height;
            first_visual_line = false;
            line = PdfLine{};
            reset_line();
        };
        const auto add_character = [&](const wchar_t character,
                                       const RunStyle& style) {
            const double width = pdf_character_width(character, style);
            if (!line.runs.empty() &&
                line.width + width > line.writable_width) {
                emit_line(false);
                if (character == L' ') return;
            }
            if (line.runs.empty() || !(line.runs.back().style == style)) {
                line.runs.push_back({style, {}, 0.0});
            }
            line.runs.back().text.push_back(character);
            line.runs.back().width += width;
            line.width += width;
            line.largest_font =
                (std::max)(line.largest_font, pdf_font_size(style));
        };
        const auto add_word = [&](std::wstring_view word,
                                  const RunStyle& style) {
            double word_width = 0.0;
            for (const wchar_t character : word)
                word_width += pdf_character_width(character, style);
            if (!line.runs.empty() &&
                line.width + word_width > line.writable_width) {
                emit_line(false);
            }
            for (const wchar_t character : word)
                add_character(character, style);
        };

        for (const TextRun& run : paragraph.runs) {
            if (run.style.hidden) continue;
            for (std::size_t position = 0; position < run.text.size();) {
                const wchar_t character = run.text[position];
                if (character == L'\f') {
                    emit_line(false);
                    begin_new_page();
                } else if (character == L'\r' || character == L'\n') {
                    emit_line(true);
                } else if (character == L'\t') {
                    for (int index = 0; index < 4; ++index)
                        add_character(L' ', run.style);
                } else if (character == L' ') {
                    if (!line.runs.empty()) add_character(character, run.style);
                } else if (character >= 0x20) {
                    std::size_t end = position + 1;
                    while (end < run.text.size() && run.text[end] > L' ' &&
                           run.text[end] != L'\f') ++end;
                    add_word(std::wstring_view(run.text).substr(
                                 position, end - position), run.style);
                    position = end;
                    continue;
                }
                ++position;
            }
        }
        emit_line(true);
        if (paragraph.bottom_border) {
            const double border_left = margin_left +
                static_cast<double>(paragraph.left_indent) / 20.0;
            const double border_right = page_width - margin_right -
                static_cast<double>(paragraph.right_indent) / 20.0;
            pages.back() += "q\n0 0 0 RG\n0.5 w\n" +
                pdf_number(border_left) + " " + pdf_number(cursor + 2.0) +
                " m\n" + pdf_number(border_right) + " " +
                pdf_number(cursor + 2.0) + " l\nS\nQ\n";
        }
        cursor -= (std::max)(0.0,
            static_cast<double>(paragraph.space_after) / 20.0);
    }
    if (pages.empty()) pages.emplace_back();
    return pages;
}

bool write_pdf(const std::wstring& path,
               const std::vector<Paragraph>& paragraphs,
               const DocumentSettings& settings = {}) {
    static constexpr std::array<const char*, 12> font_names = {
        "Helvetica", "Helvetica-Bold", "Helvetica-Oblique",
        "Helvetica-BoldOblique", "Times-Roman", "Times-Bold",
        "Times-Italic", "Times-BoldItalic", "Courier", "Courier-Bold",
        "Courier-Oblique", "Courier-BoldOblique"};
    PdfUnicodeFont embedded_unicode;
    PdfUnicodeFont* unicode_font = nullptr;
    if (pdf_needs_unicode_font(paragraphs) &&
        embedded_unicode.initialize(pdf_unicode_font_name(paragraphs)))
        unicode_font = &embedded_unicode;
    const std::vector<std::string> contents =
        pdf_page_contents(paragraphs, settings, unicode_font);
    const double page_width = settings.valid ?
        static_cast<double>(settings.page_width) / 20.0 : 612.0;
    const double page_height = settings.valid ?
        static_cast<double>(settings.page_height) / 20.0 : 792.0;
    constexpr int catalog_object = 1;
    constexpr int pages_object = 2;
    constexpr int first_font_object = 3;
    const int unicode_font_object = first_font_object +
                                    static_cast<int>(font_names.size());
    const int unicode_cid_font_object = unicode_font_object + 1;
    const int unicode_descriptor_object = unicode_font_object + 2;
    const int unicode_file_object = unicode_font_object + 3;
    const int unicode_cmap_object = unicode_font_object + 4;
    const int first_page_object = unicode_font == nullptr ?
        unicode_font_object : unicode_cmap_object + 1;
    const int object_count = first_page_object - 1 +
                             static_cast<int>(contents.size()) * 2;
    std::vector<std::string> objects(static_cast<std::size_t>(object_count + 1));
    objects[catalog_object] = "<< /Type /Catalog /Pages 2 0 R >>";

    std::string kids;
    for (std::size_t index = 0; index < contents.size(); ++index) {
        const int page_object = first_page_object + static_cast<int>(index) * 2;
        kids += std::to_string(page_object) + " 0 R ";
    }
    objects[pages_object] = "<< /Type /Pages /Kids [ " + kids +
        "] /Count " + std::to_string(contents.size()) + " >>";
    for (std::size_t index = 0; index < font_names.size(); ++index) {
        objects[first_font_object + static_cast<int>(index)] =
            "<< /Type /Font /Subtype /Type1 /BaseFont /" +
            std::string(font_names[index]) + " /Encoding /WinAnsiEncoding >>";
    }
    if (unicode_font != nullptr) {
        const std::string base_font = pdf_font_identifier(unicode_font->face);
        const int ascent = unicode_font->metrics.tmAscent;
        const int descent = -unicode_font->metrics.tmDescent;
        const int height = (std::max)(1L, unicode_font->metrics.tmHeight);
        const auto scaled = [height](const int value) {
            return static_cast<int>((static_cast<long long>(value) * 1000) /
                                    height);
        };
        const int scaled_ascent = scaled(ascent);
        const int scaled_descent = scaled(descent);
        objects[unicode_font_object] =
            "<< /Type /Font /Subtype /Type0 /BaseFont /" + base_font +
            " /Encoding /Identity-H /DescendantFonts [" +
            std::to_string(unicode_cid_font_object) +
            " 0 R] /ToUnicode " + std::to_string(unicode_cmap_object) +
            " 0 R >>";
        objects[unicode_cid_font_object] =
            "<< /Type /Font /Subtype /CIDFontType2 /BaseFont /" + base_font +
            " /CIDSystemInfo << /Registry (Adobe) /Ordering (Identity) "
            "/Supplement 0 >> /FontDescriptor " +
            std::to_string(unicode_descriptor_object) +
            " 0 R /DW 1000 /CIDToGIDMap /Identity >>";
        objects[unicode_descriptor_object] =
            "<< /Type /FontDescriptor /FontName /" + base_font +
            " /Flags 32 /FontBBox [-1000 " +
            std::to_string(scaled_descent) + " 3000 " +
            std::to_string(scaled_ascent + 500) + "] /ItalicAngle 0 "
            "/Ascent " + std::to_string(scaled_ascent) + " /Descent " +
            std::to_string(scaled_descent) + " /CapHeight " +
            std::to_string(scaled_ascent) + " /StemV 80 /FontFile2 " +
            std::to_string(unicode_file_object) + " 0 R >>";
        objects[unicode_file_object] =
            "<< /Length " + std::to_string(unicode_font->file.size()) +
            " /Length1 " + std::to_string(unicode_font->file.size()) +
            " >>\nstream\n" +
            std::string(reinterpret_cast<const char*>(unicode_font->file.data()),
                        unicode_font->file.size()) +
            "\nendstream";
        const std::string cmap = pdf_to_unicode_cmap(*unicode_font);
        objects[unicode_cmap_object] =
            "<< /Length " + std::to_string(cmap.size()) +
            " >>\nstream\n" + cmap + "endstream";
    }

    std::string resources = "<< /Font << ";
    for (std::size_t index = 0; index < font_names.size(); ++index) {
        resources += "/F" + std::to_string(index + 1) + " " +
                     std::to_string(first_font_object +
                                    static_cast<int>(index)) + " 0 R ";
    }
    if (unicode_font != nullptr)
        resources += "/FU " + std::to_string(unicode_font_object) + " 0 R ";
    resources += ">> >>";
    for (std::size_t index = 0; index < contents.size(); ++index) {
        const int page_object = first_page_object + static_cast<int>(index) * 2;
        const int content_object = page_object + 1;
        objects[page_object] =
            "<< /Type /Page /Parent 2 0 R /MediaBox [0 0 " +
            pdf_number(page_width) + " " + pdf_number(page_height) + "] "
            "/Resources " + resources + " /Contents " +
            std::to_string(content_object) + " 0 R >>";
        objects[content_object] =
            "<< /Length " + std::to_string(contents[index].size()) +
            " >>\nstream\n" + contents[index] + "endstream";
    }

    std::string pdf = "%PDF-1.4\n%\xe2\xe3\xcf\xd3\n";
    std::vector<std::size_t> offsets(static_cast<std::size_t>(object_count + 1));
    for (int object = 1; object <= object_count; ++object) {
        offsets[object] = pdf.size();
        pdf += std::to_string(object) + " 0 obj\n" + objects[object] +
               "\nendobj\n";
    }
    const std::size_t xref_offset = pdf.size();
    pdf += "xref\n0 " + std::to_string(object_count + 1) +
           "\n0000000000 65535 f \n";
    for (int object = 1; object <= object_count; ++object) {
        char entry[32]{};
        std::snprintf(entry, std::size(entry), "%010llu 00000 n \n",
                      static_cast<unsigned long long>(offsets[object]));
        pdf += entry;
    }
    pdf += "trailer\n<< /Size " + std::to_string(object_count + 1) +
           " /Root 1 0 R >>\nstartxref\n" + std::to_string(xref_offset) +
           "\n%%EOF\n";
    return write_bytes(path, pdf);
}

bool rtf_to_pdf(std::string_view rtf, const std::wstring& path) {
    RichEditDocument rich;
    return rich.load(rtf) &&
           write_pdf(path, paragraphs_from_rich_edit(rich));
}

int export_paragraphs_to_pdf_dialog(
    HWND owner, const std::vector<Paragraph>& paragraphs,
    const DocumentSettings& settings = {}) {
    WCHAR path[32768] = OPUSW("Document.pdf");
    OPENFILENAMEW dialog{};
    dialog.lStructSize = sizeof(dialog);
    dialog.hwndOwner = owner;
    dialog.lpstrFilter = kPdfFilter;
    dialog.lpstrFile = path;
    dialog.nMaxFile = static_cast<DWORD>(std::size(path));
    dialog.lpstrTitle = kPdfDialogTitle;
    dialog.lpstrDefExt = kPdfDefaultExtension;
    dialog.Flags = OFN_EXPLORER | OFN_ENABLESIZING | OFN_LONGNAMES |
                   OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
    DWORD test_path_length = 0;
#ifdef OPUS_TEST_HOOKS
    test_path_length = GetEnvironmentVariableW(
        kPdfTestPathEnv, path, static_cast<DWORD>(std::size(path)));
#endif
    const bool accepted =
        test_path_length > 0 && test_path_length < std::size(path) ?
#ifdef OPUS_TEST_HOOKS
            (SetEnvironmentVariableW(kPdfTestPathEnv, nullptr), true) :
#else
            true :
#endif
            GetSaveFileNameW(&dialog) != FALSE;
    if (!accepted) {
        return CommDlgExtendedError() == 0 ? -1 : false;
    }
    std::wstring selected_path;
    for (const WCHAR* text = path; *text != 0; ++text) {
        selected_path.push_back(static_cast<wchar_t>(*text));
    }
    if (!write_pdf(selected_path, paragraphs, settings)) {
        MessageBoxW(owner, kPdfFailureMessage, kPdfDialogTitle,
                    MB_OK | MB_ICONEXCLAMATION);
        return false;
    }
    return true;
}

int export_rtf_to_pdf_dialog(HWND owner, std::string_view rtf) {
    RichEditDocument rich;
    if (!rich.load(rtf)) return false;
    return export_paragraphs_to_pdf_dialog(
        owner, paragraphs_from_rich_edit(rich));
}

}  // namespace

extern "C" int OpusModernPathIsDocx(const char* path) {
    return path != nullptr && has_extension(path, ".docx");
}

extern "C" int OpusModernPathIsOdt(const char* path) {
    return path != nullptr && has_extension(path, ".odt");
}

extern "C" int OpusModernDocxToRtfFile(const char* docx_path,
                                         const char* rtf_path) try {
    std::vector<Paragraph> paragraphs;
    return load_docx_paragraphs(docx_path, paragraphs) &&
           write_bytes(wide_path(rtf_path), paragraphs_to_rtf(paragraphs));
} catch (...) {
    return false;
}

extern "C" int OpusModernDocxToTextFile(const char* docx_path,
                                           const char* text_path) try {
    std::vector<Paragraph> paragraphs;
    std::vector<TableLayout> tables;
    DocumentSettings settings;
    if (!load_docx_paragraphs(docx_path, paragraphs, &settings, &tables)) {
        pending_docx_import = {};
        return false;
    }
    const std::string text = paragraphs_to_text(paragraphs,
                                                &pending_docx_import, &tables);
    pending_docx_import.settings = settings;
    if (!write_bytes(wide_path(text_path), text)) {
        pending_docx_import = {};
        return false;
    }
    return true;
} catch (...) {
    pending_docx_import = {};
    return false;
}

extern "C" int OpusModernOdtToRtfFile(const char* odt_path,
                                       const char* rtf_path) try {
    std::vector<Paragraph> paragraphs;
    return load_odt_paragraphs(odt_path, paragraphs) &&
           write_bytes(wide_path(rtf_path), paragraphs_to_rtf(paragraphs));
} catch (...) {
    return false;
}

extern "C" int OpusModernOdtToTextFile(const char* odt_path,
                                        const char* text_path) try {
    std::vector<Paragraph> paragraphs;
    std::vector<TableLayout> tables;
    DocumentSettings settings;
    if (!load_odt_paragraphs(odt_path, paragraphs, &settings, &tables)) {
        pending_docx_import = {};
        return false;
    }
    const std::string text = paragraphs_to_text(paragraphs,
                                                &pending_docx_import, &tables);
    pending_docx_import.settings = settings;
    if (!write_bytes(wide_path(text_path), text)) {
        pending_docx_import = {};
        return false;
    }
    return true;
} catch (...) {
    pending_docx_import = {};
    return false;
}

extern "C" int OpusModernPendingDocxRunCount() {
    return static_cast<int>(pending_docx_import.runs.size());
}

extern "C" int OpusModernPendingDocxParagraphCount() {
    return static_cast<int>(pending_docx_import.paragraphs.size());
}

extern "C" int OpusModernPendingDocxTableCount() {
    return static_cast<int>(pending_docx_import.tables.size());
}

extern "C" int OpusModernGetPendingDocxTable(
    const int index, long* cp_first, long* cp_lim, int* first_paragraph,
    int* rows, int* columns) {
    if (index < 0 || static_cast<std::size_t>(index) >=
                         pending_docx_import.tables.size()) return false;
    const PendingTableFormat& record = pending_docx_import.tables[index];
    if (cp_first != nullptr) *cp_first = record.cp_first;
    if (cp_lim != nullptr) *cp_lim = record.cp_lim;
    if (first_paragraph != nullptr)
        *first_paragraph = record.first_paragraph;
    if (rows != nullptr) *rows = record.rows;
    if (columns != nullptr) *columns = record.columns;
    return true;
}

extern "C" int OpusModernGetPendingDocxParagraphRange(
    const int index, long* cp_first, long* cp_lim) {
    if (index < 0 || static_cast<std::size_t>(index) >=
                         pending_docx_import.paragraphs.size()) return false;
    const PendingParagraphFormat& record =
        pending_docx_import.paragraphs[index];
    if (cp_first != nullptr) *cp_first = record.cp_first;
    if (cp_lim != nullptr) *cp_lim = record.cp_lim;
    return true;
}

extern "C" int OpusModernGetPendingDocxPage(
    int* page_width, int* page_height, int* margin_left, int* margin_right,
    int* margin_top, int* margin_bottom) {
    const DocumentSettings& settings = pending_docx_import.settings;
    if (!settings.valid) return false;
    if (page_width != nullptr) *page_width = settings.page_width;
    if (page_height != nullptr) *page_height = settings.page_height;
    if (margin_left != nullptr) *margin_left = settings.margin_left;
    if (margin_right != nullptr) *margin_right = settings.margin_right;
    if (margin_top != nullptr) *margin_top = settings.margin_top;
    if (margin_bottom != nullptr) *margin_bottom = settings.margin_bottom;
    return true;
}

extern "C" int OpusModernGetPendingDocxRun(
    const int index, long* cp_first, long* cp_lim, int* bold, int* italic,
    int* underline, int* strike, int* small_caps, int* all_caps, int* hidden,
    int* half_points, int* color_index, char* font,
    const int font_capacity, int* charset, char* language,
    const int language_capacity) try {
    if (index < 0 || static_cast<std::size_t>(index) >=
                         pending_docx_import.runs.size()) return false;
    const PendingRunFormat& record = pending_docx_import.runs[index];
    if (cp_first != nullptr) *cp_first = record.cp_first;
    if (cp_lim != nullptr) *cp_lim = record.cp_lim;
    if (bold != nullptr) *bold = record.style.bold;
    if (italic != nullptr) *italic = record.style.italic;
    if (underline != nullptr) *underline = record.style.underline;
    if (strike != nullptr) *strike = record.style.strike;
    if (small_caps != nullptr) *small_caps = record.style.small_caps;
    if (all_caps != nullptr) *all_caps = record.style.all_caps;
    if (hidden != nullptr) *hidden = record.style.hidden;
    if (half_points != nullptr) *half_points = record.style.half_points;
    if (color_index != nullptr) *color_index = legacy_color_index(record.style);
    if (font != nullptr && font_capacity > 0) {
        const std::string ansi_font = wide_to_ansi(record.style.font);
        lstrcpynA(font, ansi_font.c_str(), font_capacity);
    }
    if (charset != nullptr) *charset = record.style.charset;
    if (language != nullptr && language_capacity > 0)
        lstrcpynA(language, record.style.language.c_str(), language_capacity);
    return true;
} catch (...) {
    if (font != nullptr && font_capacity > 0) font[0] = '\0';
    if (language != nullptr && language_capacity > 0) language[0] = '\0';
    return false;
}

extern "C" int OpusModernGetPendingDocxParagraph(
    const int index, long* cp_first, long* cp_lim, int* alignment,
    int* left_indent, int* right_indent, int* first_line_indent,
    int* space_before, int* space_after, int* line_spacing,
    int* keep_together, int* keep_with_next, int* page_break_before,
    int* bottom_border) {
    if (index < 0 || static_cast<std::size_t>(index) >=
                         pending_docx_import.paragraphs.size()) return false;
    const PendingParagraphFormat& record =
        pending_docx_import.paragraphs[index];
    if (cp_first != nullptr) *cp_first = record.cp_first;
    if (cp_lim != nullptr) *cp_lim = record.cp_lim;
    if (alignment != nullptr) {
        *alignment = record.paragraph.alignment == PFA_CENTER ? 1 :
            record.paragraph.alignment == PFA_RIGHT ? 2 :
            record.paragraph.alignment == PFA_JUSTIFY ? 3 : 0;
    }
    if (left_indent != nullptr) *left_indent = record.paragraph.left_indent;
    if (right_indent != nullptr) *right_indent = record.paragraph.right_indent;
    if (first_line_indent != nullptr)
        *first_line_indent = record.paragraph.first_line_indent;
    if (space_before != nullptr) *space_before = record.paragraph.space_before;
    if (space_after != nullptr) *space_after = record.paragraph.space_after;
    if (line_spacing != nullptr) *line_spacing = record.paragraph.line_spacing;
    if (keep_together != nullptr) *keep_together = record.paragraph.keep_together;
    if (keep_with_next != nullptr) *keep_with_next = record.paragraph.keep_with_next;
    if (page_break_before != nullptr)
        *page_break_before = record.paragraph.page_break_before;
    if (bottom_border != nullptr) *bottom_border = record.paragraph.bottom_border;
    return true;
}

extern "C" void OpusModernClearPendingDocxFormatting() {
    pending_docx_import = {};
}

extern "C" int OpusModernBindPendingDocxUnicode(const int doc) try {
    if (doc < 0 || pending_docx_import.unicode_cells.size() > kMaxTextBytes)
        return false;
    unicode_documents[doc].cells = pending_docx_import.unicode_cells;
    return true;
} catch (...) {
    return false;
}

extern "C" void OpusUnicodeForgetDocument(const int doc) {
    unicode_documents.erase(doc);
}

extern "C" unsigned int OpusUnicodeScalarAt(const int doc, const long cp) {
    if (cp < 0) return 0;
    const auto found = unicode_documents.find(doc);
    if (found == unicode_documents.end() ||
        static_cast<std::size_t>(cp) >= found->second.cells.size()) return 0;
    return found->second.cells[static_cast<std::size_t>(cp)].scalar;
}

extern "C" unsigned int OpusUnicodeLanguageAt(const int doc, const long cp) {
    if (cp < 0) return 0;
    const auto found = unicode_documents.find(doc);
    if (found == unicode_documents.end() ||
        static_cast<std::size_t>(cp) >= found->second.cells.size()) return 0;
    return found->second.cells[static_cast<std::size_t>(cp)].language;
}

extern "C" int OpusUnicodeHasRange(
    const int doc, const long cp_first, const int length) {
    if (cp_first < 0 || length <= 0) return false;
    const auto found = unicode_documents.find(doc);
    if (found == unicode_documents.end()) return false;
    const std::vector<UnicodeCell>& cells = found->second.cells;
    const std::size_t first = static_cast<std::size_t>(cp_first);
    if (first >= cells.size()) return false;
    const std::size_t count = (std::min)(
        static_cast<std::size_t>(length), cells.size() - first);
    return std::any_of(
        cells.begin() + static_cast<std::ptrdiff_t>(first),
        cells.begin() + static_cast<std::ptrdiff_t>(first + count),
        [](const UnicodeCell& cell) { return cell.scalar != 0; });
}

extern "C" void OpusUnicodeOnReplace(
    const int doc, const long cp_first, const long cp_lim,
    const long inserted_count) try {
    if (cp_first < 0 || cp_lim < cp_first || inserted_count < 0 ||
        inserted_count > static_cast<long>(kMaxTextBytes)) return;
    const auto found = unicode_documents.find(doc);
    if (found == unicode_documents.end()) return;
    std::vector<UnicodeCell>& cells = found->second.cells;
    const std::size_t first = (std::min)(
        static_cast<std::size_t>(cp_first), cells.size());
    const std::size_t limit = (std::min)(
        static_cast<std::size_t>(cp_lim), cells.size());
    if (cells.size() - (limit - first) +
            static_cast<std::size_t>(inserted_count) > kMaxTextBytes) return;
    cells.erase(cells.begin() + static_cast<std::ptrdiff_t>(first),
                cells.begin() + static_cast<std::ptrdiff_t>(limit));
    cells.insert(cells.begin() + static_cast<std::ptrdiff_t>(first),
                 static_cast<std::size_t>(inserted_count), UnicodeCell{});
} catch (...) {
}

extern "C" void OpusUnicodeOnReplaceCps(
    const int destination_doc, const long destination_first,
    const long destination_lim, const int source_doc,
    const long source_first, const long source_lim) try {
    if (destination_first < 0 || destination_lim < destination_first ||
        source_first < 0 || source_lim < source_first) return;
    const auto source = unicode_documents.find(source_doc);
    const auto destination = unicode_documents.find(destination_doc);
    if (source == unicode_documents.end() &&
        destination == unicode_documents.end()) return;
    const std::size_t requested = static_cast<std::size_t>(
        source_lim - source_first);
    if (requested > kMaxTextBytes) return;
    std::vector<UnicodeCell> copied(requested);
    if (source != unicode_documents.end()) {
        const std::vector<UnicodeCell>& source_cells = source->second.cells;
        const std::size_t first = (std::min)(
            static_cast<std::size_t>(source_first), source_cells.size());
        const std::size_t available = (std::min)(
            requested, source_cells.size() - first);
        std::copy_n(source_cells.begin() + static_cast<std::ptrdiff_t>(first),
                    available, copied.begin());
    }
    std::vector<UnicodeCell>& cells = unicode_documents[destination_doc].cells;
    if (static_cast<std::size_t>(destination_first) > cells.size()) {
        if (static_cast<std::size_t>(destination_first) > kMaxTextBytes) return;
        cells.resize(static_cast<std::size_t>(destination_first));
    }
    const std::size_t first = static_cast<std::size_t>(destination_first);
    const std::size_t limit = (std::min)(
        static_cast<std::size_t>(destination_lim), cells.size());
    if (cells.size() - (limit - first) + copied.size() > kMaxTextBytes) return;
    cells.erase(cells.begin() + static_cast<std::ptrdiff_t>(first),
                cells.begin() + static_cast<std::ptrdiff_t>(limit));
    cells.insert(cells.begin() + static_cast<std::ptrdiff_t>(first),
                 copied.begin(), copied.end());
} catch (...) {
}

const LanguageProfile& active_input_profile() {
    if (input_language != "auto") return profile_for_tag(input_language);
    const LANGID language = LOWORD(reinterpret_cast<ULONG_PTR>(
        GetKeyboardLayout(0)));
    return profile_for_language_id(language);
}

extern "C" int OpusUnicodeSetInputLanguage(const char* language) {
    if (language == nullptr || *language == '\0' ||
        _stricmp(language, "auto") == 0) {
        input_language = "auto";
        return true;
    }
    const LanguageProfile& profile = profile_for_tag(language);
    input_language = profile.tag;
    return true;
}

extern "C" int OpusUnicodeGetInputLanguage(char* language,
                                             const int capacity) {
    if (language == nullptr || capacity <= 0) return false;
    const std::string value = input_language == "auto" ? "auto" :
        active_input_profile().tag;
    lstrcpynA(language, value.c_str(), capacity);
    return true;
}

extern "C" int OpusUnicodeLegacyByteForScalar(const unsigned int scalar) {
    const LanguageProfile& selected = input_language == "auto" ?
        profile_for_scalar(scalar) : active_input_profile();
    return static_cast<unsigned char>(legacy_byte_for_scalar(scalar, selected));
}

extern "C" int OpusUnicodeSetScalar(const int doc, const long cp,
                                      const unsigned int scalar) try {
    if (doc < 0 || cp < 0 || static_cast<std::size_t>(cp) >= kMaxTextBytes ||
        scalar == 0 || scalar > 0x10ffff ||
        (scalar >= 0xd800 && scalar <= 0xdfff)) return false;
    std::vector<UnicodeCell>& cells = unicode_documents[doc].cells;
    if (cells.size() <= static_cast<std::size_t>(cp))
        cells.resize(static_cast<std::size_t>(cp) + 1);
    const LanguageProfile& profile = input_language == "auto" ?
        profile_for_scalar(scalar) : active_input_profile();
    cells[static_cast<std::size_t>(cp)] = {scalar, profile.language_id};
    return true;
} catch (...) {
    return false;
}

int code_page_for_charset(const int charset) {
    switch (charset) {
        case EASTEUROPE_CHARSET: return 1250;
        case RUSSIAN_CHARSET: return 1251;
        case GREEK_CHARSET: return 1253;
        case TURKISH_CHARSET: return 1254;
        case HEBREW_CHARSET: return 1255;
        case ARABIC_CHARSET: return 1256;
        case BALTIC_CHARSET: return 1257;
        case VIETNAMESE_CHARSET: return 1258;
        case THAI_CHARSET: return 874;
        case SHIFTJIS_CHARSET: return 932;
        case GB2312_CHARSET: return 936;
        case HANGEUL_CHARSET: return 949;
        case CHINESEBIG5_CHARSET: return 950;
        default: return 1252;
    }
}

std::uint32_t decode_legacy_byte(const unsigned char byte,
                                 const int code_page) {
    wchar_t decoded = 0;
    if (byte < 0x80) return byte;
    if (MultiByteToWideChar(code_page, 0,
                            reinterpret_cast<const char*>(&byte), 1,
                            &decoded, 1) == 1) return decoded;
    return 0xfffd;
}

extern "C" int OpusUnicodeTextToUtf8(
    const int doc, const long cp_first, const char* bytes, const int length,
    const int charset, char* output, const int capacity) try {
    if (cp_first < 0 || bytes == nullptr || length < 0 || capacity < 0 ||
        static_cast<std::size_t>(length) > kMaxTextBytes) return -1;
    std::wstring wide;
    wide.reserve(static_cast<std::size_t>(length));
    const int code_page = code_page_for_charset(charset);
    for (int index = 0; index < length; ++index) {
        std::uint32_t scalar = OpusUnicodeScalarAt(doc, cp_first + index);
        if (scalar == 0) scalar = decode_legacy_byte(
            static_cast<unsigned char>(bytes[index]), code_page);
        wide += scalar_to_wide(scalar);
    }
    const std::string utf8 = wide_to_utf8(wide);
    if (utf8.size() >= static_cast<std::size_t>(capacity)) return -1;
    if (output != nullptr && capacity > 0) {
        std::memcpy(output, utf8.data(), utf8.size());
        output[utf8.size()] = '\0';
    }
    return static_cast<int>(utf8.size());
} catch (...) {
    return -1;
}

extern "C" int OpusUnicodeLanguageTagAt(
    const int doc, const long cp, char* language, const int capacity) {
    if (language == nullptr || capacity <= 0) return false;
    const LANGID language_id = static_cast<LANGID>(
        OpusUnicodeLanguageAt(doc, cp));
    const LanguageProfile& profile = language_id != 0 ?
        profile_for_language_id(language_id) : default_language_profile();
    lstrcpynA(language, profile.tag, capacity);
    return true;
}

extern "C" int OpusUnicodeClipboardToLegacy(
    HANDLE unicode_handle, HANDLE* legacy_handle) try {
    if (legacy_handle == nullptr) return false;
    *legacy_handle = nullptr;
    pending_clipboard_cells.clear();
    if (unicode_handle == nullptr) return false;
    const SIZE_T bytes = GlobalSize(unicode_handle);
    if (bytes < sizeof(WCHAR) || bytes > kMaxTextBytes * sizeof(WCHAR) ||
        (bytes % sizeof(WCHAR)) != 0)
        return false;
    const auto* source = static_cast<const WCHAR*>(GlobalLock(unicode_handle));
    if (source == nullptr) return false;
    const std::size_t capacity = bytes / sizeof(WCHAR);
    std::size_t length = 0;
    while (length < capacity && source[length] != WCHAR{}) ++length;
    if (length == capacity) {
        GlobalUnlock(unicode_handle);
        return false;
    }
    std::string legacy;
    legacy.reserve(length + 1);
    pending_clipboard_cells.reserve(length);
    std::wstring text;
    text.reserve(length);
    for (std::size_t index = 0; index < length; ++index) {
        text.push_back(static_cast<wchar_t>(source[index]));
    }
    for (std::size_t index = 0; index < text.size();) {
        const std::uint32_t scalar = scalar_at(text, index);
        const LanguageProfile& profile = input_language == "auto" ?
            profile_for_scalar(scalar) : active_input_profile();
        append_legacy_scalar(legacy, &pending_clipboard_cells,
                             scalar, profile);
    }
    GlobalUnlock(unicode_handle);
    if (legacy.size() >= 0x10000 || legacy.size() > kMaxTextBytes) {
        pending_clipboard_cells.clear();
        return false;
    }
    HANDLE converted = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT,
                                   legacy.size() + 1);
    if (converted == nullptr) {
        pending_clipboard_cells.clear();
        return false;
    }
    void* destination = GlobalLock(converted);
    if (destination == nullptr) {
        GlobalFree(converted);
        pending_clipboard_cells.clear();
        return false;
    }
    if (!legacy.empty()) std::memcpy(destination, legacy.data(), legacy.size());
    GlobalUnlock(converted);
    *legacy_handle = converted;
    return true;
} catch (...) {
    pending_clipboard_cells.clear();
    return false;
}

extern "C" HANDLE OpusUnicodeCreateClipboardHandle(
    const int doc, const long cp_first, HANDLE legacy_handle) try {
    if (cp_first < 0 || legacy_handle == nullptr) return nullptr;
    const SIZE_T bytes = GlobalSize(legacy_handle);
    if (bytes == 0 || bytes > kMaxTextBytes) return nullptr;
    const auto* source = static_cast<const char*>(GlobalLock(legacy_handle));
    if (source == nullptr) return nullptr;
    std::size_t length = 0;
    while (length < bytes && source[length] != '\0') ++length;
    if (length == bytes) {
        GlobalUnlock(legacy_handle);
        return nullptr;
    }
    std::wstring wide;
    wide.reserve(length);
    for (std::size_t index = 0; index < length; ++index) {
        std::uint32_t scalar = OpusUnicodeScalarAt(
            doc, cp_first + static_cast<long>(index));
        if (scalar == 0) scalar = decode_legacy_byte(
            static_cast<unsigned char>(source[index]), 1252);
        wide += scalar_to_wide(scalar);
    }
    GlobalUnlock(legacy_handle);
    std::basic_string<WCHAR> clipboard;
    clipboard.reserve(wide.size());
    for (const wchar_t code_unit : wide) {
        clipboard.push_back(static_cast<WCHAR>(
            static_cast<std::uint16_t>(code_unit)));
    }
    if (clipboard.size() > (kMaxTextBytes / sizeof(WCHAR)) - 1)
        return nullptr;
    HANDLE result = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT,
                                (clipboard.size() + 1) * sizeof(WCHAR));
    if (result == nullptr) return nullptr;
    void* destination = GlobalLock(result);
    if (destination == nullptr) {
        GlobalFree(result);
        return nullptr;
    }
    if (!clipboard.empty())
        std::memcpy(destination, clipboard.data(),
                    clipboard.size() * sizeof(WCHAR));
    GlobalUnlock(result);
    return result;
} catch (...) {
    return nullptr;
}

extern "C" int OpusUnicodeBindPendingClipboard(const int doc) try {
    if (doc < 0 || pending_clipboard_cells.size() > kMaxTextBytes) return false;
    unicode_documents[doc].cells = pending_clipboard_cells;
    pending_clipboard_cells.clear();
    return true;
} catch (...) {
    pending_clipboard_cells.clear();
    return false;
}

extern "C" BOOL OpusUnicodeExtTextOut(
    HDC dc, const int x, const int y, const UINT options, const RECT* rectangle,
    const int doc, const long cp_first, const char* bytes, const UINT length,
    const int* advances) try {
    if (dc == nullptr || bytes == nullptr || length == 0)
        return ExtTextOutA(dc, x, y, options, rectangle, bytes, length, advances);
    const auto found = unicode_documents.find(doc);
    bool has_unicode = false;
    if (found != unicode_documents.end() && cp_first >= 0) {
        const std::vector<UnicodeCell>& cells = found->second.cells;
        const std::size_t first = static_cast<std::size_t>(cp_first);
        if (first < cells.size()) {
            const std::size_t count = (std::min)(
                static_cast<std::size_t>(length), cells.size() - first);
            has_unicode = std::any_of(
                cells.begin() + static_cast<std::ptrdiff_t>(first),
                cells.begin() + static_cast<std::ptrdiff_t>(first + count),
                [](const UnicodeCell& cell) { return cell.scalar != 0; });
        }
    }
    if (!has_unicode)
        return ExtTextOutA(dc, x, y, options, rectangle, bytes, length, advances);

    const int code_page = code_page_for_charset(GetTextCharset(dc));
    std::wstring wide;
    std::vector<int> wide_advances;
    wide.reserve(length);
    if (advances != nullptr) wide_advances.reserve(length);
    for (UINT index = 0; index < length; ++index) {
        std::uint32_t scalar = OpusUnicodeScalarAt(doc, cp_first + index);
        if (scalar == 0) scalar = decode_legacy_byte(
            static_cast<unsigned char>(bytes[index]), code_page);
        const std::wstring encoded = scalar_to_wide(scalar);
        wide += encoded;
        if (advances != nullptr) {
            wide_advances.push_back(advances[index]);
            if (encoded.size() == 2) wide_advances.push_back(0);
        }
    }
    std::basic_string<WCHAR> output;
    output.reserve(wide.size());
    for (const wchar_t code_unit : wide) {
        output.push_back(static_cast<WCHAR>(
            static_cast<std::uint16_t>(code_unit)));
    }
    return ExtTextOutW(dc, x, y, options, rectangle, output.data(),
                       static_cast<UINT>(output.size()),
                       advances != nullptr ? wide_advances.data() : nullptr);
} catch (...) {
    return ExtTextOutA(dc, x, y, options, rectangle, bytes, length, advances);
}

extern "C" int OpusPdfSnapshotBegin(
    const int page_width, const int page_height, const int margin_left,
    const int margin_right, const int margin_top, const int margin_bottom) {
    pending_pdf_export = {};
    DocumentSettings& settings = pending_pdf_export.settings;
    const bool dimensions_valid =
        page_width >= 720 && page_width <= 63360 &&
        page_height >= 720 && page_height <= 63360 &&
        margin_left >= 0 && margin_left <= 31680 &&
        margin_right >= 0 && margin_right <= 31680 &&
        margin_top >= 0 && margin_top <= 31680 &&
        margin_bottom >= 0 && margin_bottom <= 31680;
    settings.page_width = page_width;
    settings.page_height = page_height;
    settings.margin_left = margin_left;
    settings.margin_right = margin_right;
    settings.margin_top = margin_top;
    settings.margin_bottom = margin_bottom;
    settings.valid = dimensions_valid &&
        settings.margin_left + settings.margin_right < page_width &&
        settings.margin_top + settings.margin_bottom < page_height;
    return settings.valid;
}

extern "C" int OpusPdfSnapshotAddParagraph(
    const int alignment, const int left_indent, const int right_indent,
    const int first_line_indent, const int space_before,
    const int space_after, const int line_spacing,
    const int keep_together, const int keep_with_next,
    const int page_break_before, const int bottom_border) try {
    if (pending_pdf_export.paragraphs.size() >= kMaxParagraphs) return false;

    /* These values come from the original PAP: indentation and line spacing are
     * signed 16-bit fields, while before/after spacing are unsigned. Validate
     * their real storage domains instead of rejecting legitimate
     * exact-line-spacing and legacy sentinel values.
     */
    if (left_indent < INT16_MIN || left_indent > INT16_MAX ||
        right_indent < INT16_MIN || right_indent > INT16_MAX ||
        first_line_indent < INT16_MIN || first_line_indent > INT16_MAX ||
        space_before < 0 || space_before > UINT16_MAX ||
        space_after < 0 || space_after > UINT16_MAX ||
        line_spacing < INT16_MIN || line_spacing > INT16_MAX) return false;
    Paragraph paragraph;
    paragraph.alignment = alignment == 1 ? PFA_CENTER :
        alignment == 2 ? PFA_RIGHT :
        alignment == 3 ? PFA_JUSTIFY : PFA_LEFT;
    paragraph.left_indent = left_indent;
    paragraph.right_indent = right_indent;
    paragraph.first_line_indent = first_line_indent;
    paragraph.space_before = space_before;
    paragraph.space_after = space_after;
    paragraph.line_spacing = line_spacing;
    paragraph.keep_together = keep_together != 0;
    paragraph.keep_with_next = keep_with_next != 0;
    paragraph.page_break_before = page_break_before != 0;
    paragraph.bottom_border = bottom_border != 0;
    pending_pdf_export.paragraphs.push_back(std::move(paragraph));
    return true;
} catch (...) {
    return false;
}

extern "C" int OpusPdfSnapshotAddRun(
    const char* text, const int length, const char* font,
    const int half_points, const int bold, const int italic,
    const int underline, const int strike, const int small_caps,
    const int all_caps, const int hidden, const int color_index) try {
    if (text == nullptr || length < 0 ||
        static_cast<std::size_t>(length) > kMaxTextBytes ||
        pending_pdf_export.paragraphs.empty() ||
        pending_pdf_export.run_count >= kMaxRuns ||
        static_cast<std::size_t>(length) >
            kMaxTextBytes - pending_pdf_export.text_bytes) return false;
    RunStyle style;
    style.bold = bold != 0;
    style.italic = italic != 0;
    style.underline = underline != 0;
    style.strike = strike != 0;
    style.small_caps = small_caps != 0;
    style.all_caps = all_caps != 0;
    style.hidden = hidden != 0;
    style.half_points = half_points >= 8 && half_points <= 254 ?
        half_points : 20;
    if (font != nullptr && *font != '\0') {
        const std::size_t font_length = strnlen_s(font, 256);
        if (font_length == 256) return false;
        style.font = ansi_to_wide(std::string_view(font, font_length));
    }
    apply_legacy_color(color_index, style);
    std::wstring run_text = ansi_to_wide(
        std::string_view(text, static_cast<std::size_t>(length)));
    if (style.all_caps || style.small_caps) {
        for (wchar_t& character : run_text) {
            character = static_cast<wchar_t>(std::towupper(character));
        }
    }
    if (style.small_caps && !style.all_caps) {
        style.half_points = (std::max)(8, style.half_points * 4 / 5);
    }
    pending_pdf_export.paragraphs.back().runs.push_back(
        {style, std::move(run_text)});
    ++pending_pdf_export.run_count;
    pending_pdf_export.text_bytes += static_cast<std::size_t>(length);
    return true;
} catch (...) {
    return false;
}

extern "C" int OpusPdfSnapshotAddRunUtf8(
    const char* text, const int length, const char* font,
    const int half_points, const int bold, const int italic,
    const int underline, const int strike, const int small_caps,
    const int all_caps, const int hidden, const int color_index,
    const char* language, const int charset) try {
    if (text == nullptr || length < 0 ||
        static_cast<std::size_t>(length) > kMaxTextBytes ||
        pending_pdf_export.paragraphs.empty() ||
        pending_pdf_export.run_count >= kMaxRuns ||
        static_cast<std::size_t>(length) >
            kMaxTextBytes - pending_pdf_export.text_bytes) return false;
    RunStyle style;
    style.bold = bold != 0;
    style.italic = italic != 0;
    style.underline = underline != 0;
    style.strike = strike != 0;
    style.small_caps = small_caps != 0;
    style.all_caps = all_caps != 0;
    style.hidden = hidden != 0;
    style.half_points = half_points >= 8 && half_points <= 254 ?
        half_points : 20;
    if (font != nullptr && *font != '\0') {
        const std::size_t font_length = strnlen_s(font, 256);
        if (font_length == 256) return false;
        style.font = ansi_to_wide(std::string_view(font, font_length));
    }
    if (language != nullptr && *language != '\0') {
        const std::size_t language_length = strnlen_s(language, 32);
        if (language_length == 32) return false;
        const LanguageProfile& profile = profile_for_tag(
            std::string_view(language, language_length));
        style.language = profile.tag;
        style.code_page = profile.code_page;
        style.charset = charset >= 0 && charset <= 255 ? charset :
            profile.charset;
    }
    apply_legacy_color(color_index, style);
    std::wstring run_text = utf8_to_wide(
        std::string_view(text, static_cast<std::size_t>(length)));
    if (style.all_caps || style.small_caps) {
        for (wchar_t& character : run_text)
            character = static_cast<wchar_t>(std::towupper(character));
    }
    if (style.small_caps && !style.all_caps)
        style.half_points = (std::max)(8, style.half_points * 4 / 5);
    pending_pdf_export.paragraphs.back().runs.push_back(
        {style, std::move(run_text)});
    ++pending_pdf_export.run_count;
    pending_pdf_export.text_bytes += static_cast<std::size_t>(length);
    return true;
} catch (...) {
    return false;
}

extern "C" int OpusPdfSnapshotExportDialog(HWND owner) try {
    if (pending_pdf_export.paragraphs.empty()) return false;
    const int result = export_paragraphs_to_pdf_dialog(
        owner, pending_pdf_export.paragraphs, pending_pdf_export.settings);
    pending_pdf_export = {};
    return result;
} catch (...) {
    pending_pdf_export = {};
    return false;
}

extern "C" int OpusDocxSnapshotExportPath(const char* path) try {
    if (path == nullptr || *path == '\0' ||
        pending_pdf_export.paragraphs.empty()) return false;
    const bool written = has_extension(path, ".odt") ?
        write_odt(wide_path(path), pending_pdf_export.paragraphs,
                  pending_pdf_export.settings) :
        write_docx(wide_path(path), pending_pdf_export.paragraphs,
                   pending_pdf_export.settings);
    pending_pdf_export = {};
    return written;
} catch (...) {
    pending_pdf_export = {};
    return false;
}

extern "C" int OpusModernRtfFileToDocx(const char* rtf_path,
                                        const char* docx_path) try {
    return rtf_to_docx(wide_path(rtf_path), wide_path(docx_path));
} catch (...) {
    return false;
}

extern "C" int OpusModernRtfFileToOdt(const char* rtf_path,
                                       const char* odt_path) try {
    return rtf_to_odt(wide_path(rtf_path), wide_path(odt_path));
} catch (...) {
    return false;
}

extern "C" int OpusModernRtfFileToPdf(const char* rtf_path,
                                       const char* pdf_path) try {
    std::string rtf;
    return read_bytes(wide_path(rtf_path), rtf) &&
           rtf_to_pdf(rtf, wide_path(pdf_path));
} catch (...) {
    return false;
}

extern "C" int OpusExportRtfToPdfDialog(HWND owner, const char* rtf) try {
    if (rtf == nullptr) return false;
    const std::size_t length = strnlen_s(rtf, kMaxRtfBytes + 1);
    return length <= kMaxRtfBytes &&
        export_rtf_to_pdf_dialog(owner, std::string_view(rtf, length));
} catch (...) {
    return false;
}

extern "C" int OpusExportTextToPdfDialog(HWND owner, const char* text,
                                           const int length) try {
    if (text == nullptr || length < 0 ||
        static_cast<std::size_t>(length) > kMaxTextBytes) return false;
    return export_rtf_to_pdf_dialog(
        owner, ansi_text_to_rtf(std::string_view(
                   text, static_cast<std::size_t>(length))));
} catch (...) {
    return false;
}
