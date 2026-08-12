#include "opus_x64_compat.h"
#include "opus_x64_heap.h"
#include "inter.h"

#include <array>
#include <cstddef>
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
extern "C" void OpusX64TraceRibbon(const char* stage, int message, int tmc,
                                    int first_value, int second_value,
                                    long cp_first, long cp_limit,
                                    int insertion) {
    static_cast<void>(stage);
    static_cast<void>(message);
    static_cast<void>(tmc);
    static_cast<void>(first_value);
    static_cast<void>(second_value);
    static_cast<void>(cp_first);
    static_cast<void>(cp_limit);
    static_cast<void>(insertion);
}

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
extern "C" int FModalDlg(TestWord);
extern "C" int FFreeDlg();
extern "C" TestWord TmcDoDlgDli(TestDltHeader**, void**, TestDli*);
extern "C" void EndDlg(TestWord);
extern "C" void SetTmcText_sdm21(TestWord, char*);
extern "C" void GetTmcText_sdm21(TestWord, char*, TestWord);

int modal_init_count = 0;
int modal_exit_count = 0;
int new_modal_init_count = 0;
int new_modal_exit_count = 0;
bool new_modal_controls_present = false;

bool WindowHasClass(const HWND window, const char* expected) {
    char class_name[32] = {};
    return window != nullptr &&
           GetClassNameA(window, class_name, sizeof(class_name)) != 0 &&
           std::strcmp(class_name, expected) == 0;
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
            WindowHasClass(HwndOfTmc(0x0402), "Button") &&
            WindowHasClass(HwndOfTmc(0x0403), "Button") &&
            WindowHasClass(HwndOfTmc(0x0404), "Edit") &&
            WindowHasClass(HwndOfTmc(0x0405), "ListBox");
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
        first_control == nullptr || GetParent(first_control) != first_host ||
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
    EndSdm();
    DestroyWindow(parent);

    return 0;
}
