#include "opus_x64_compat.h"
#include "opus_x64_heap.h"
#include "inter.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>

extern "C" struct ITR vitr = {};

extern "C" void** mpdochdod[8] = {};
extern "C" void** mpfnhfcb[8] = {};
extern "C" void** mpwwhwwd[8] = {};
extern "C" void** mpmwhmwd[8] = {};

extern "C" void* N_PdodDoc(int doc);
extern "C" void* N_PfcbFn(int fn);
extern "C" void* N_PwwdWw(int ww);
extern "C" void** N_HwwdWw(int ww);
extern "C" void* N_PmwdMw(int mw);
extern "C" void* N_PmwdWw(int ww);
extern "C" int DocMother(int doc);
extern "C" void* N_PdodMother(int doc);
extern "C" int DocDotMother(int doc);

extern "C" void* OpusTestDod(int slot, int dk, int docMother, int docDot);
extern "C" void* OpusTestWwd(int slot, int mw);
extern "C" void OpusTestSetDodCpMac(void* pdod, long cpMac);

struct TestCa {
    long cpFirst;
    long cpLim;
    int doc;
};

extern "C" long CpMin(long first, long second);
extern "C" long CpMax(long first, long second);
extern "C" int NMultDiv(int value, int numerator, int denominator);
extern "C" int FNeRgch(const void* first, const void* second, int count);
extern "C" void* blt(const void* source, void* destination, int count);
extern "C" long CpMacDoc(int doc);
extern "C" long CpMacDocEdit(int doc);
extern "C" TestCa* PcaSet(TestCa* range, int doc, long cpFirst, long cpLim);
extern "C" TestCa* PcaSetDcp(TestCa* range, int doc, long cpFirst, long dcp);
extern "C" long DcpCa(const TestCa* range);
extern "C" int FInCa(int doc, long cp, const TestCa* range);

extern "C" void AddDcbToLprgbst(int* offsets, int count, int delta,
                                  int threshold);

using TestWord = std::uint16_t;

struct TestSdmRec {
    int x;
    int y;
    int dx;
    int dy;
};

struct TestDltHeader {
    TestSdmRec rec;
    TestWord hid;
    TestWord tmc_sel_init;
    void* dialog_proc;
    TestWord base_item_count;
    TestWord border;
};

struct TestDli {
    HWND hwnd;
    int dx;
    int dy;
    std::uint32_t flags;
    std::uintptr_t reference;
    unsigned char* runtime_items;
};

extern "C" int FInitSdm_sdm21(void*);
extern "C" void EndSdm();
extern "C" TestWord HdlgStartDlg(TestDltHeader**, void**, TestDli*);
extern "C" TestWord HdlgSetCurDlg(TestWord);
extern "C" HWND HwndFromDlg(TestWord);
extern "C" HWND HwndOfTmc(TestWord);
extern "C" void GetTmcRec(TestWord, TestSdmRec*);
extern "C" int FModalDlg(TestWord);
extern "C" int FFreeDlg();
extern "C" TestWord TmcDoDlgDli(TestDltHeader**, void**, TestDli*);
extern "C" void EndDlg(TestWord);
extern "C" void SetTmcText_sdm21(TestWord, char*);
extern "C" void GetTmcText_sdm21(TestWord, char*, TestWord);
extern "C" void SetTmcVal_sdm21(TestWord, TestWord);
extern "C" TestWord CEntryListBoxTmc(TestWord);
extern "C" void GetListBoxEntry(TestWord, TestWord, char*, TestWord);
extern "C" void** HcabAlloc_sdm21(TestWord);
extern "C" void FreeCab(void**);
extern "C" int FSetCabSz(void**, const char*, TestWord);
extern "C" void GetCabSz(void**, char*, TestWord, TestWord);
extern "C" int OpusWin95SaveAliasMatches(const unsigned char*);
extern "C" int OpusFinishWin95SaveAlias(const unsigned char*, int, int);

extern "C" int OpusSaveDocumentAsDocx(int, const char*) {
    return 0;
}

struct TestCabSaveNative {
    TestWord simple_words;
    TestWord handle_words;
    TestWord sab;
    TestWord alignment;
    char** file_name;
    int directory_list;
    int format;
    int quick_save;
    int backup;
    int lock_annotations;
    int options;
};

int modal_init_count = 0;
int modal_exit_count = 0;
int new_modal_init_count = 0;
int new_modal_exit_count = 0;
bool new_modal_controls_present = false;

bool HasControlRecord(const TestWord tmc) {
    TestSdmRec rectangle{};
    GetTmcRec(tmc, &rectangle);
    return HwndOfTmc(tmc) == nullptr && rectangle.dx > 0 &&
           rectangle.dy > 0;
}

void CountedPath(const char* path, unsigned char* counted_path,
                 const std::size_t capacity) {
    const std::size_t length = std::strlen(path);
    counted_path[0] = static_cast<unsigned char>(
        length >= capacity ? capacity - 1 : length);
    std::memcpy(counted_path + 1, path, counted_path[0]);
}

bool FindListEntry(const TestWord tmc, const char* text, TestWord* index) {
    const TestWord count = CEntryListBoxTmc(tmc);
    for (TestWord item = 0; item < count; ++item) {
        char entry[260] = {};
        GetListBoxEntry(tmc, item, entry, sizeof(entry));
        if (std::strcmp(entry, text) == 0) {
            *index = item;
            return true;
        }
    }
    return false;
}

int ModalRuntimeProbe(TestWord message, TestWord, TestWord, TestWord,
                      TestWord) {
    if (message == 1) {
        ++modal_init_count;
        EndDlg(2);
    } else if (message == 4) {
        ++modal_exit_count;
    }
    return 1;
}

int NewModalRuntimeProbe(TestWord message, TestWord, TestWord, TestWord,
                         TestWord) {
    if (message == 1) {
        ++new_modal_init_count;
        new_modal_controls_present =
            HasControlRecord(0x0402) &&
            HasControlRecord(0x0403) &&
            HasControlRecord(0x0404) &&
            HasControlRecord(0x0405);
        EndDlg(2);
    } else if (message == 4) {
        ++new_modal_exit_count;
    }
    return 1;
}

int main() {
    std::array offsets{2, 8, 13, 3};
    AddDcbToLprgbst(offsets.data(), static_cast<int>(offsets.size()), 5, 8);
    if (offsets != std::array{2, 13, 18, 3}) {
        return 1;
    }

    void** handle = OpusHAllocateCb(4 * sizeof(int));
    if (handle == nullptr || OpusCbOfH(handle) != 4 * sizeof(int)) {
        return 2;
    }
    auto* values = static_cast<int*>(OpusDerefH(handle));
    values[0] = 11;
    values[1] = 22;
    if (!OpusFChngSizeHCb(handle, 8 * sizeof(int), 1)) {
        OpusFreeH(handle);
        return 3;
    }
    values = static_cast<int*>(OpusDerefH(handle));
    const bool preserved = values[0] == 11 && values[1] == 22 &&
                           OpusCbOfH(handle) == 8 * sizeof(int);
    OpusFreeH(handle);
    if (!preserved) {
        return 4;
    }

    void** prc_handle = OpusHAllocateCb(32);
    const int prc_token = OpusPrcTokenFromHandle(prc_handle);
    if (prc_handle == nullptr || prc_token <= 0 || prc_token >= 0x3fff ||
        OpusPrcHandleFromToken(prc_token) != prc_handle ||
        OpusPrcTokenFromHandle(prc_handle) != prc_token) {
        OpusFreeH(prc_handle);
        return 17;
    }
    OpusFreeH(prc_handle);
    if (OpusPrcHandleFromToken(prc_token) != nullptr) {
        return 18;
    }

    CHAR copied[16] = {};
    const CHAR source[] = "Opus";
    if (CchSz(source) != 5 || CchCopySz(source, copied) != 4 ||
        std::memcmp(source, copied, sizeof(source)) != 0) {
        return 5;
    }

    CHAR counted[] = {3, 'A', 'b', 'C', 0};
    if (PchInSt(counted, 'b') != &counted[2] ||
        PchInSt(counted, 'z') != nullptr) {
        return 6;
    }
    if (!FAlphaNum('Q') || !FAlphaNum('7') || FAlphaNum('!') ||
        !FUpper(0xD6) || !FLower(0xFF) || FDigit('A')) {
        return 7;
    }

    CHAR mixed[] = {'a', static_cast<CHAR>(0xE0), 0};
    vitr.fFrench = 1;
    QszUpper(mixed);
    if (mixed[0] != 'A' || static_cast<unsigned char>(mixed[1]) != 65 ||
        FNeNcSz(reinterpret_cast<const CHAR*>("author"),
                reinterpret_cast<const CHAR*>("AUTHOR")) ||
        !FNeNcSz(reinterpret_cast<const CHAR*>("author"),
                 reinterpret_cast<const CHAR*>("authors"))) {
        return 8;
    }

    int dod = 101;
    int fcb = 202;
    int wwd = 303;
    int mwd = 404;
    void* pdod = &dod;
    void* pfcb = &fcb;
    void* pwwd = &wwd;
    void* pmwd = &mwd;
    mpdochdod[3] = &pdod;
    mpfnhfcb[4] = &pfcb;
    mpwwhwwd[5] = &pwwd;
    mpmwhmwd[6] = &pmwd;
    if (N_PdodDoc(3) != &dod || N_PfcbFn(4) != &fcb ||
        N_PwwdWw(5) != &wwd || N_HwwdWw(5) != &pwwd ||
        N_PmwdMw(6) != &mwd) {
        return 9;
    }

    // dkDoc=0x0001, dkDot=0x0002, dkFtn=0x0800 in original doc.h.
    void* motherDod = OpusTestDod(0, 0x0001, 0, 7);
    void* shortDod = OpusTestDod(1, 0x0800, 1, 0);
    void* templateDod = OpusTestDod(2, 0x0002, 0, 0);
    void* hMotherDod = motherDod;
    void* hShortDod = shortDod;
    void* hTemplateDod = templateDod;
    mpdochdod[1] = &hMotherDod;
    mpdochdod[2] = &hShortDod;
    mpdochdod[7] = &hTemplateDod;
    if (DocMother(1) != 1 || DocMother(2) != 1 || DocMother(0) != 0 ||
        N_PdodMother(1) != motherDod || N_PdodMother(2) != motherDod ||
        DocDotMother(1) != 7 || DocDotMother(2) != 7 ||
        DocDotMother(7) != 7 || DocDotMother(0) != 0) {
        return 10;
    }

    void* realWwd = OpusTestWwd(0, 6);
    void* hRealWwd = realWwd;
    mpwwhwwd[2] = &hRealWwd;
    if (N_PmwdWw(2) != &mwd) {
        return 11;
    }

    if (!FInitSegTable(123) || sbMac != 123) {
        return 12;
    }

    if (CpMin(7, -2) != -2 || CpMax(7, -2) != 7 ||
        NMultDiv(5, 3, 2) != 8 || NMultDiv(-5, 3, 2) != -8 ||
        NMultDiv(40000, 2, 1) != 32767 || NMultDiv(1, 1, 0) != 32767) {
        return 13;
    }

    std::array<int, 5> overlapping{1, 2, 3, 4, 5};
    blt(overlapping.data(), overlapping.data() + 1, 4);
    if (overlapping != std::array<int, 5>{1, 1, 2, 3, 4} ||
        !FNeRgch("Word", "word", 4) || FNeRgch("Word", "Word", 4)) {
        return 14;
    }

    OpusTestSetDodCpMac(motherDod, 103);
    TestCa range{};
    if (CpMacDoc(1) != 101 || CpMacDocEdit(1) != 100 ||
        PcaSet(&range, 1, 10, 20) != &range || DcpCa(&range) != 10 ||
        !FInCa(1, 10, &range) || FInCa(1, 20, &range) ||
        PcaSetDcp(&range, 2, 30, 7)->cpLim != 37) {
        return 15;
    }

    const POINT point = MAKEPOINT(MAKELPARAM(
        static_cast<WORD>(-7), static_cast<WORD>(9)));
    if (point.x != -7 || point.y != 9) {
        return 16;
    }

    if (!FInitSdm_sdm21(nullptr)) {
        return 19;
    }
    const HWND parent = CreateWindowExA(0, "STATIC", "SDM test parent",
                                        WS_OVERLAPPED, 0, 0, 320, 200,
                                        nullptr, nullptr,
                                        GetModuleHandleW(nullptr), nullptr);
    if (parent == nullptr) {
        EndSdm();
        return 20;
    }
    TestDltHeader first_template{{0, 0, 80, 20}, 101, 0, nullptr, 1, 0};
    TestDltHeader second_template{{2, 3, 90, 24}, 102, 0, nullptr, 1, 0};
    auto* first_template_pointer = &first_template;
    auto* second_template_pointer = &second_template;
    TestDli first_initializer{parent, 4, 5, 2, 11, nullptr};
    TestDli second_initializer{parent, 7, 8, 3, 22, nullptr};
    const TestWord first = HdlgStartDlg(&first_template_pointer, nullptr,
                                        &first_initializer);
    char first_text[] = "first";
    SetTmcText_sdm21(0x400, first_text);
    const HWND first_host = HwndFromDlg(first);
    const HWND first_control = HwndOfTmc(0x400);

    const TestWord second = HdlgStartDlg(&second_template_pointer, nullptr,
                                         &second_initializer);
    char second_text[] = "second";
    SetTmcText_sdm21(0x400, second_text);
    const HWND second_host = HwndFromDlg(second);
    if (first == 0 || second == 0 || first == second || first_host == parent ||
        second_host == parent || first_host == second_host ||
        GetParent(first_host) != parent || GetParent(second_host) != parent ||
        first_control != nullptr ||
        FModalDlg(first) || !FModalDlg(second)) {
        DestroyWindow(parent);
        EndSdm();
        return 21;
    }

    HdlgSetCurDlg(first);
    char text_buffer[16] = {};
    GetTmcText_sdm21(0x400, text_buffer, sizeof(text_buffer));
    if (std::strcmp(text_buffer, "first") != 0 || !FFreeDlg() ||
        IsWindow(first_host) || !IsWindow(second_host)) {
        DestroyWindow(parent);
        EndSdm();
        return 22;
    }
    HdlgSetCurDlg(second);
    std::memset(text_buffer, 0, sizeof(text_buffer));
    GetTmcText_sdm21(0x400, text_buffer, sizeof(text_buffer));
    if (std::strcmp(text_buffer, "second") != 0) {
        DestroyWindow(parent);
        EndSdm();
        return 23;
    }
    /* Character is native-modal but is not routed to a common file dialog. */
    TestDltHeader modal_template{{8, 24, 206, 104}, 16, 0x8400,
                                  reinterpret_cast<void*>(ModalRuntimeProbe),
                                  11, 4};
    auto* modal_template_pointer = &modal_template;
    TestDli modal_initializer{parent, 0, 0, 1, 0, nullptr};
    if (TmcDoDlgDli(&modal_template_pointer, nullptr, &modal_initializer) != 2 ||
        modal_init_count != 1 || modal_exit_count != 1 ||
        !IsWindow(second_host)) {
        DestroyWindow(parent);
        EndSdm();
        return 24;
    }
    TestDltHeader new_modal_template{
        {20, 24, 127, 114}, 2, 0x8404,
        reinterpret_cast<void*>(NewModalRuntimeProbe), 9, 4};
    auto* new_modal_template_pointer = &new_modal_template;
    if (TmcDoDlgDli(&new_modal_template_pointer, nullptr,
                    &modal_initializer) != 2 ||
        new_modal_init_count != 1 || new_modal_exit_count != 1 ||
        !new_modal_controls_present || !IsWindow(second_host)) {
        DestroyWindow(parent);
        EndSdm();
        return 25;
    }

    constexpr TestWord save_cab_words = static_cast<TestWord>(
        (sizeof(TestCabSaveNative) + sizeof(TestWord) - 1) /
        sizeof(TestWord));
    constexpr TestWord save_cab_initializer =
        static_cast<TestWord>(save_cab_words | (1u << 8u));
    void** save_cab = HcabAlloc_sdm21(save_cab_initializer);
    char selected_path[] = "sdm-save.doc";
    if (save_cab == nullptr ||
        !FSetCabSz(save_cab, selected_path, 1)) {
        if (save_cab != nullptr) {
            FreeCab(save_cab);
        }
        DestroyWindow(parent);
        EndSdm();
        return 26;
    }
    SetEnvironmentVariableA("WORD1_TEST_FILE_DIALOG_PATH", nullptr);
    SetEnvironmentVariableA("TMPDIR", "/tmp/");
    TestDltHeader save_template{{8, 24, 150, 102}, 4, 0x8400,
                                 nullptr, 11, 4};
    auto* save_template_pointer = &save_template;
    const TestWord save_result =
        TmcDoDlgDli(&save_template_pointer, save_cab, &modal_initializer);
    char staged_path[32768] = {};
    GetCabSz(save_cab, staged_path, sizeof(staged_path), 1);
    unsigned char staged_counted[256] = {};
    CountedPath(staged_path, staged_counted, sizeof(staged_counted));
    const bool save_alias_active =
        OpusWin95SaveAliasMatches(staged_counted) != 0;
    OpusFinishWin95SaveAlias(staged_counted, 0, 0);
    DeleteFileA(selected_path);
    FreeCab(save_cab);
    if (save_result != 1 || staged_path[0] == '\0' || !save_alias_active) {
        DestroyWindow(parent);
        EndSdm();
        return 27;
    }

    void** open_cab = HcabAlloc_sdm21(save_cab_initializer);
    char open_path[] = "sdm-open.doc";
    FILE* open_file = std::fopen(open_path, "wb");
    if (open_file != nullptr) {
        std::fputs("open", open_file);
        std::fclose(open_file);
    }
    if (open_cab == nullptr || open_file == nullptr ||
        !FSetCabSz(open_cab, open_path, 1)) {
        if (open_cab != nullptr) {
            FreeCab(open_cab);
        }
        DeleteFileA(open_path);
        DestroyWindow(parent);
        EndSdm();
        return 28;
    }
    SetEnvironmentVariableA("WORD1_TEST_FILE_DIALOG_PATH", nullptr);
    TestDltHeader open_template{{8, 24, 206, 104}, 3, 0x8400,
                                 nullptr, 11, 4};
    auto* open_template_pointer = &open_template;
    const TestWord open_result =
        TmcDoDlgDli(&open_template_pointer, open_cab, &modal_initializer);
    char opened_staged_path[32768] = {};
    GetCabSz(open_cab, opened_staged_path, sizeof(opened_staged_path), 1);
    unsigned char opened_staged_counted[256] = {};
    CountedPath(opened_staged_path, opened_staged_counted,
                sizeof(opened_staged_counted));
    const bool open_alias_active =
        OpusWin95SaveAliasMatches(opened_staged_counted) != 0;
    DeleteFileA(opened_staged_path);
    DeleteFileA(open_path);
    FreeCab(open_cab);
    if (open_result != 1 || opened_staged_path[0] == '\0' ||
        !open_alias_active) {
        DestroyWindow(parent);
        EndSdm();
        return 29;
    }

    const auto cleanup_browse_tree = [] {
        DeleteFileA("sdm.browse/sub.dir/open.doc");
        RemoveDirectoryA("sdm.browse/sub.dir");
        RemoveDirectoryA("sdm.browse");
    };

    cleanup_browse_tree();
    if (!CreateDirectoryA("sdm.browse", nullptr) ||
        !CreateDirectoryA("sdm.browse/sub.dir", nullptr)) {
        DestroyWindow(parent);
        EndSdm();
        return 30;
    }
    FILE* browse_file = std::fopen("sdm.browse/sub.dir/open.doc", "wb");
    if (browse_file != nullptr) {
        std::fputs("browse", browse_file);
        std::fclose(browse_file);
    }
    if (browse_file == nullptr) {
        cleanup_browse_tree();
        DestroyWindow(parent);
        EndSdm();
        return 31;
    }

    void** browse_open_cab = HcabAlloc_sdm21(save_cab_initializer);
    if (browse_open_cab == nullptr ||
        !FSetCabSz(browse_open_cab, "sdm.browse/*.doc", 1)) {
        if (browse_open_cab != nullptr) {
            FreeCab(browse_open_cab);
        }
        cleanup_browse_tree();
        DestroyWindow(parent);
        EndSdm();
        return 32;
    }
    auto* browse_open_template_pointer = &open_template;
    const TestWord browse_open =
        HdlgStartDlg(&browse_open_template_pointer, browse_open_cab,
                     &modal_initializer);
    TestWord list_index = 0;
    if (browse_open == 0 || !FindListEntry(0x0402, "[sub.dir]", &list_index)) {
        FreeCab(browse_open_cab);
        cleanup_browse_tree();
        DestroyWindow(parent);
        EndSdm();
        return 33;
    }
    SetTmcVal_sdm21(0x0402, list_index);
    SendMessageA(HwndFromDlg(browse_open), WM_COMMAND,
                 MAKEWPARAM(0x0402, LBN_DBLCLK), 0);
    if (!FindListEntry(0x0401, "open.doc", &list_index)) {
        FFreeDlg();
        FreeCab(browse_open_cab);
        cleanup_browse_tree();
        DestroyWindow(parent);
        EndSdm();
        return 34;
    }
    SetTmcVal_sdm21(0x0401, list_index);
    SendMessageA(HwndFromDlg(browse_open), WM_COMMAND,
                 MAKEWPARAM(0x0401, LBN_DBLCLK), 0);
    char browsed_open_path[32768] = {};
    GetCabSz(browse_open_cab, browsed_open_path, sizeof(browsed_open_path), 1);
    const bool browsed_open_ok =
        std::strstr(browsed_open_path, "sdm.browse") != nullptr &&
        std::strstr(browsed_open_path, "sub.dir") != nullptr &&
        std::strstr(browsed_open_path, "open.doc") != nullptr;
    FFreeDlg();
    FreeCab(browse_open_cab);
    if (!browsed_open_ok) {
        cleanup_browse_tree();
        DestroyWindow(parent);
        EndSdm();
        return 35;
    }

    void** browse_save_cab = HcabAlloc_sdm21(save_cab_initializer);
    if (browse_save_cab == nullptr ||
        !FSetCabSz(browse_save_cab, "save.doc", 1)) {
        if (browse_save_cab != nullptr) {
            FreeCab(browse_save_cab);
        }
        cleanup_browse_tree();
        DestroyWindow(parent);
        EndSdm();
        return 36;
    }
    auto* browse_save_template_pointer = &save_template;
    const TestWord browse_save =
        HdlgStartDlg(&browse_save_template_pointer, browse_save_cab,
                     &modal_initializer);
    if (browse_save == 0 ||
        !FindListEntry(0x0403, "[sdm.browse]", &list_index)) {
        FreeCab(browse_save_cab);
        cleanup_browse_tree();
        DestroyWindow(parent);
        EndSdm();
        return 37;
    }
    SetTmcVal_sdm21(0x0403, list_index);
    SendMessageA(HwndFromDlg(browse_save), WM_COMMAND,
                 MAKEWPARAM(0x0403, LBN_DBLCLK), 0);
    if (!FindListEntry(0x0403, "[sub.dir]", &list_index)) {
        FFreeDlg();
        FreeCab(browse_save_cab);
        cleanup_browse_tree();
        DestroyWindow(parent);
        EndSdm();
        return 38;
    }
    SetTmcVal_sdm21(0x0403, list_index);
    SendMessageA(HwndFromDlg(browse_save), WM_COMMAND,
                 MAKEWPARAM(0x0403, LBN_DBLCLK), 0);
    SendMessageA(HwndFromDlg(browse_save), WM_COMMAND,
                 MAKEWPARAM(1, BN_CLICKED), 0);
    char browsed_save_path[32768] = {};
    GetCabSz(browse_save_cab, browsed_save_path, sizeof(browsed_save_path), 1);
    const bool browsed_save_ok =
        std::strstr(browsed_save_path, "sdm.browse") != nullptr &&
        std::strstr(browsed_save_path, "sub.dir") != nullptr &&
        std::strstr(browsed_save_path, "save.doc") != nullptr;
    FFreeDlg();
    FreeCab(browse_save_cab);
    cleanup_browse_tree();
    if (!browsed_save_ok) {
        DestroyWindow(parent);
        EndSdm();
        return 39;
    }
    EndSdm();
    DestroyWindow(parent);

    return 0;
}
