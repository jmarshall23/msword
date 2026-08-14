#include "opus-native-compat.h"
#include "opus_elx_dispatch.h"
#include "opus-native-heap.h"
#include "inter.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

struct ITR vitr = {0};

HANDLE *lphevtHead = NULL;
HANDLE *lphrgbKeyState = NULL;
void **mpdochdod[8] = {0};
void **mpfnhfcb[8] = {0};
void **mpwwhwwd[8] = {0};
void **mpmwhmwd[8] = {0};

typedef char OpusAssertLongWidth[(sizeof(LONG) == 4) ? 1 : -1];
typedef char OpusAssertDwordWidth[(sizeof(DWORD) == 4) ? 1 : -1];
typedef char OpusAssertPointSize[(sizeof(POINT) == 8) ? 1 : -1];
typedef char OpusAssertRectSize[(sizeof(RECT) == 16) ? 1 : -1];
typedef char OpusAssertMakeLongWidth[
    (sizeof(MAKELONG(0x1122, 0x3344)) == sizeof(LONG)) ? 1 : -1];

void *N_PdodDoc(int doc);
void *N_PfcbFn(int fn);
void *N_PwwdWw(int ww);
void **N_HwwdWw(int ww);
void *N_PmwdMw(int mw);
void *N_PmwdWw(int ww);
int DocMother(int doc);
void *N_PdodMother(int doc);
int DocDotMother(int doc);

void *OpusTestDod(int slot, int dk, int docMother, int docDot);
void *OpusTestWwd(int slot, int mw);
void OpusTestSetDodCpMac(void *pdod, long cpMac);

typedef struct TestCa {
    long cpFirst;
    long cpLim;
    int doc;
} TestCa;

long CpMin(long first, long second);
long CpMax(long first, long second);
int NMultDiv(int value, int numerator, int denominator);
int FNeRgch(const void *first, const void *second, int count);
void *blt(const void *source, void *destination, int count);
long CpMacDoc(int doc);
long CpMacDocEdit(int doc);
TestCa *PcaSet(TestCa *range, int doc, long cpFirst, long cpLim);
TestCa *PcaSetDcp(TestCa *range, int doc, long cpFirst, long dcp);
long DcpCa(const TestCa *range);
int FInCa(int doc, long cp, const TestCa *range);
long LPushMacroArgsTyped(void *procedure, const int *arguments,
                         int argument_count, const int *types,
                         int type_count);

void AddDcbToLprgbst(int *offsets, int count, int delta, int threshold);

typedef uint16_t TestWord;

typedef struct TestSdmRec {
    int x;
    int y;
    int dx;
    int dy;
} TestSdmRec;

typedef struct TestElxNum {
    union {
        unsigned char rgb[8];
        double d;
    } num;
} TestElxNum;

typedef struct TestDltHeader {
    TestSdmRec rec;
    TestWord hid;
    TestWord tmc_sel_init;
    void *dialog_proc;
    TestWord base_item_count;
    TestWord border;
} TestDltHeader;

typedef struct TestDli {
    HWND hwnd;
    int dx;
    int dy;
    uint32_t flags;
    uintptr_t reference;
    unsigned char *runtime_items;
} TestDli;

int FInitSdm_sdm21(void *);
void EndSdm(void);
TestWord HdlgStartDlg(TestDltHeader **, void **, TestDli *);
TestWord HdlgSetCurDlg(TestWord);
HWND HwndFromDlg(TestWord);
HWND HwndOfTmc(TestWord);
void GetTmcRec(TestWord, TestSdmRec *);
int FModalDlg(TestWord);
int FFreeDlg(void);
TestWord TmcDoDlgDli(TestDltHeader **, void **, TestDli *);
void EndDlg(TestWord);
void SetTmcText_sdm21(TestWord, char *);
void GetTmcText_sdm21(TestWord, char *, TestWord);
void SetTmcVal_sdm21(TestWord, TestWord);
TestWord CEntryListBoxTmc(TestWord);
void GetListBoxEntry(TestWord, TestWord, char *, TestWord);
void **HcabAlloc_sdm21(TestWord);
void FreeCab(void **);
int FSetCabSz(void **, const char *, TestWord);
void GetCabSz(void **, char *, TestWord, TestWord);
int OpusWin95SaveAliasMatches(const unsigned char *);
int OpusWin95SaveAliasActiveMatches(const unsigned char *);
int OpusFinishWin95SaveAlias(const unsigned char *, int, int);

int OpusSaveDocumentAsDocx(int doc, const char *path) {
    (void)doc;
    (void)path;
    return 0;
}

int N_WCompSzSrt(char *first, char *second, int case_sensitive) {
    (void)case_sensitive;
    return strcmp(first, second);
}

typedef struct TestCabSaveNative {
    TestWord simple_words;
    TestWord handle_words;
    TestWord sab;
    TestWord alignment;
    char **file_name;
    int directory_list;
    int format;
    int quick_save;
    int backup;
    int lock_annotations;
    int options;
} TestCabSaveNative;

static int modal_init_count = 0;
static int modal_exit_count = 0;
static int new_modal_init_count = 0;
static int new_modal_exit_count = 0;
static int new_modal_controls_present = 0;

static int HasControlRecord(TestWord tmc) {
    TestSdmRec rectangle = {0};
    GetTmcRec(tmc, &rectangle);
    return HwndOfTmc(tmc) == NULL && rectangle.dx > 0 && rectangle.dy > 0;
}

static void CountedPath(const char *path, unsigned char *counted_path,
                        size_t capacity) {
    size_t length = strlen(path);
    counted_path[0] = (unsigned char)(length >= capacity ? capacity - 1
                                                         : length);
    memcpy(counted_path + 1, path, counted_path[0]);
}

static int FindListEntry(TestWord tmc, const char *text, TestWord *index) {
    TestWord count = CEntryListBoxTmc(tmc);
    TestWord item;
    for (item = 0; item < count; ++item) {
        char entry[260] = {0};
        GetListBoxEntry(tmc, item, entry, sizeof(entry));
        if (strcmp(entry, text) == 0) {
            *index = item;
            return 1;
        }
    }
    return 0;
}

static int ModalRuntimeProbe(TestWord message, TestWord a, TestWord b,
                             TestWord c, TestWord d) {
    (void)a;
    (void)b;
    (void)c;
    (void)d;
    if (message == 1) {
        ++modal_init_count;
        EndDlg(2);
    } else if (message == 4) {
        ++modal_exit_count;
    }
    return 1;
}

static int NewModalRuntimeProbe(TestWord message, TestWord a, TestWord b,
                                TestWord c, TestWord d) {
    (void)a;
    (void)b;
    (void)c;
    (void)d;
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

static void cleanup_browse_tree(void) {
    DeleteFileA("sdm.browse/sub.dir/open.doc");
    RemoveDirectoryA("sdm.browse/sub.dir");
    RemoveDirectoryA("sdm.browse");
}

static TestElxNum TestElxNumFromDouble(double value) {
    TestElxNum result;
    result.num.d = value;
    return result;
}

static void SetElxNumArg(OPUS_ELX_NATIVE_ARG *argument, double value) {
    TestElxNum num = TestElxNumFromDouble(value);
    argument->kind = opusElxArgNum;
    memcpy(argument->num, &num, sizeof(num));
}

static int TestElxMixed(int first, void *pointer, TestElxNum num,
                        int second, void *other, TestElxNum other_num) {
    return first + second + (pointer != NULL ? 10 : 0) +
           (other != NULL ? 20 : 0) + (int)num.num.d + (int)other_num.num.d;
}

static TestElxNum TestElxReturnNum(int addend, TestElxNum num) {
    return TestElxNumFromDouble(num.num.d + addend);
}

static uintptr_t TestElxReturnPointer(void *pointer) {
    return (uintptr_t)pointer;
}

static int elx_void_called = 0;
static void TestElxVoid(int value, void *pointer, TestElxNum num) {
    elx_void_called = value + (pointer != NULL ? 5 : 0) + (int)num.num.d;
}

static const char *macro_string_seen = NULL;
static long __cdecl TestMacroString(const char *text) {
    macro_string_seen = text;
    return text != NULL && strcmp(text, "macro string") == 0 ? 47 : 0;
}

static long __cdecl TestMacroLong(long value) {
    return value == 0x1234567L ? 43 : 0;
}

static long __cdecl TestMacroLongString(long value, const char *text) {
    return value == 0x2345678L && text == macro_string_seen ? 41 : 0;
}

static long __cdecl TestMacroDouble(int first, double value,
                                    const char *text) {
    return first == 5 && value == 12.5 && text == macro_string_seen ? 53 : 0;
}

static long __cdecl TestMacroTwoDoubles(int first, double left, double right,
                                        const char *text) {
    return first == 7 && left == 2.25 && right == 4.5 &&
                   text == macro_string_seen
               ? 61
               : 0;
}

static long __cdecl TestMacroWideDouble(int first, int second, int third,
                                        int fourth, double value,
                                        const char *text) {
    return first == 1 && second == 2 && third == 3 && fourth == 4 &&
                   value == 8.75 && text == macro_string_seen
               ? 67
               : 0;
}

static long __cdecl TestMacroSeventhDouble(int first, int second, int third,
                                           int fourth, int fifth, int sixth,
                                           double value) {
    return first == 1 && second == 2 && third == 3 && fourth == 4 &&
                   fifth == 5 && sixth == 6 && value == 14.5
               ? 71
               : 0;
}

static void SetMacroPointerArguments(int *arguments, const void *pointer_value) {
    uintptr_t pointer = (uintptr_t)pointer_value;
    arguments[0] = (int)(pointer & UINT32_MAX);
#if UINTPTR_MAX > UINT32_MAX
    arguments[1] = (int)(pointer >> 32);
#else
    arguments[1] = 0;
#endif
}

static void SetMacroLongArgument(int *arguments, long value) {
    memcpy(arguments, &value, sizeof(value));
}

static int RunMacroTypedBridgeTest(void) {
    enum {
        testDktInt = 0,
        testDktLong = 1,
        testDktDouble = 3,
        testDktString = 4
    };
    char text[] = "macro string";
    double value = 12.5;
    int string_arguments[2];
    int string_types[1] = {testDktString};
    int long_arguments[2] = {0};
    int long_types[1] = {testDktLong};
    int long_string_arguments[4] = {0};
    int long_string_types[2] = {testDktLong, testDktString};
    int mixed_arguments[5];
    int mixed_types[3] = {testDktInt, testDktDouble, testDktString};
    double left = 2.25;
    double right = 4.5;
    int double_arguments[7];
    int double_types[4] = {testDktInt, testDktDouble, testDktDouble,
                           testDktString};
    double wide_value = 8.75;
    int wide_arguments[8];
    int wide_types[6] = {testDktInt, testDktInt,    testDktInt,
                         testDktInt, testDktDouble, testDktString};
    double seventh_value = 14.5;
    int seventh_arguments[8];
    int seventh_types[7] = {testDktInt, testDktInt,    testDktInt,
                            testDktInt, testDktInt,    testDktInt,
                            testDktDouble};
    int i;

    SetMacroPointerArguments(string_arguments, text);
    if (LPushMacroArgsTyped((void *)TestMacroString, string_arguments, 2,
                            string_types, 1) != 47 ||
        macro_string_seen != text) {
        return 0;
    }

    SetMacroLongArgument(long_arguments, 0x1234567L);
    if (LPushMacroArgsTyped((void *)TestMacroLong, long_arguments,
                            sizeof(long) / sizeof(int), long_types, 1) !=
        43) {
        return 0;
    }

    SetMacroLongArgument(long_string_arguments, 0x2345678L);
    long_string_arguments[sizeof(long) / sizeof(int)] = string_arguments[0];
    long_string_arguments[sizeof(long) / sizeof(int) + 1] =
        string_arguments[1];
    if (LPushMacroArgsTyped((void *)TestMacroLongString,
                            long_string_arguments,
                            (int)(sizeof(long) / sizeof(int) + 2),
                            long_string_types, 2) != 41) {
        return 0;
    }

    mixed_arguments[0] = 5;
    memcpy(mixed_arguments + 1, &value, sizeof(value));
    mixed_arguments[3] = string_arguments[0];
    mixed_arguments[4] = string_arguments[1];
    if (LPushMacroArgsTyped((void *)TestMacroDouble, mixed_arguments, 5,
                            mixed_types, 3) != 53) {
        return 0;
    }

    double_arguments[0] = 7;
    memcpy(double_arguments + 1, &left, sizeof(left));
    memcpy(double_arguments + 3, &right, sizeof(right));
    double_arguments[5] = string_arguments[0];
    double_arguments[6] = string_arguments[1];
    if (LPushMacroArgsTyped((void *)TestMacroTwoDoubles, double_arguments, 7,
                            double_types, 4) != 61) {
        return 0;
    }

    wide_arguments[0] = 1;
    wide_arguments[1] = 2;
    wide_arguments[2] = 3;
    wide_arguments[3] = 4;
    memcpy(wide_arguments + 4, &wide_value, sizeof(wide_value));
    wide_arguments[6] = string_arguments[0];
    wide_arguments[7] = string_arguments[1];
    if (LPushMacroArgsTyped((void *)TestMacroWideDouble, wide_arguments, 8,
                            wide_types, 6) != 67) {
        return 0;
    }

    for (i = 0; i < 6; ++i) {
        seventh_arguments[i] = i + 1;
    }
    memcpy(seventh_arguments + 6, &seventh_value, sizeof(seventh_value));
    if (LPushMacroArgsTyped((void *)TestMacroSeventhDouble,
                            seventh_arguments, 8, seventh_types, 7) != 71) {
        return 0;
    }

    return 1;
}

static int RunElxDispatchTest(void) {
    OPUS_ELX_NATIVE_ARG args[6];
    OPUS_ELX_NATIVE_ARG num_args[2];
    int int_result = 0;
    uintptr_t pointer_result = 0;
    TestElxNum num_result;
    int marker = 0;

    args[0].kind = opusElxArgInt;
    args[0].integer = 3;
    args[1].kind = opusElxArgPointer;
    args[1].pointer = &marker;
    SetElxNumArg(&args[2], 7.0);
    args[3].kind = opusElxArgInt;
    args[3].integer = 4;
    args[4].kind = opusElxArgPointer;
    args[4].pointer = &int_result;
    SetElxNumArg(&args[5], 9.0);
    if (!OpusInvokeElx((void *)TestElxMixed, 2, 6, args, &int_result) ||
        int_result != 53) {
        return 0;
    }

    num_args[0].kind = opusElxArgInt;
    num_args[0].integer = 3;
    SetElxNumArg(&num_args[1], 7.0);
    if (!OpusInvokeElx((void *)TestElxReturnNum, 1, 2, num_args, &num_result) ||
        num_result.num.d != 10.0) {
        return 0;
    }

    if (!OpusInvokeElx((void *)TestElxReturnPointer, 3, 1, args + 1,
                       &pointer_result) ||
        pointer_result != (uintptr_t)&marker) {
        return 0;
    }

    if (!OpusInvokeElx((void *)TestElxVoid, 0, 3, args, NULL) ||
        elx_void_called != 15) {
        return 0;
    }

    args[0].kind = 99;
    if (OpusInvokeElx((void *)TestElxMixed, 2, 1, args, &int_result) ||
        OpusInvokeElx(NULL, 2, 0, NULL, &int_result) ||
        OpusInvokeElx((void *)TestElxMixed, 2, 7, args, &int_result)) {
        return 0;
    }
    return 1;
}

int main(void) {
    int offsets[] = {2, 8, 13, 3};
    const int expected_offsets[] = {2, 13, 18, 3};
    void **handle;
    int *values;
    int preserved;
    void **prc_handle;
    int prc_token;
    CHAR copied[16] = {0};
    const CHAR source[] = "Opus";
    CHAR counted[] = {3, 'A', 'b', 'C', 0};
    CHAR mixed[] = {'a', (CHAR)0xE0, 0};
    int dod = 101;
    int fcb = 202;
    int wwd = 303;
    int mwd = 404;
    void *pdod = &dod;
    void *pfcb = &fcb;
    void *pwwd = &wwd;
    void *pmwd = &mwd;
    void *motherDod;
    void *shortDod;
    void *templateDod;
    void *hMotherDod;
    void *hShortDod;
    void *hTemplateDod;
    void *realWwd;
    void *hRealWwd;
    int overlapping[] = {1, 2, 3, 4, 5};
    const int expected_overlapping[] = {1, 1, 2, 3, 4};
    TestCa range = {0};
    POINT point;
    HWND parent;
    TestDltHeader first_template = {{0, 0, 80, 20}, 101, 0, NULL, 1, 0};
    TestDltHeader second_template = {{2, 3, 90, 24}, 102, 0, NULL, 1, 0};
    TestDltHeader *first_template_pointer = &first_template;
    TestDltHeader *second_template_pointer = &second_template;
    TestDli first_initializer;
    TestDli second_initializer;
    TestWord first;
    char first_text[] = "first";
    HWND first_host;
    HWND first_control;
    TestWord second;
    char second_text[] = "second";
    HWND second_host;
    char text_buffer[16] = {0};
    TestDltHeader modal_template = {
        {8, 24, 206, 104}, 16, 0x8400,
        (void *)ModalRuntimeProbe, 11, 4};
    TestDltHeader *modal_template_pointer = &modal_template;
    TestDli modal_initializer;
    TestDltHeader new_modal_template = {
        {20, 24, 127, 114}, 2, 0x8404,
        (void *)NewModalRuntimeProbe, 9, 4};
    TestDltHeader *new_modal_template_pointer = &new_modal_template;
    enum {
        save_cab_words =
            (sizeof(TestCabSaveNative) + sizeof(TestWord) - 1) /
            sizeof(TestWord),
        save_cab_initializer = save_cab_words | (1u << 8u)
    };
    void **save_cab;
    char selected_path[] = "sdm-save.doc";
    TestDltHeader save_template = {{8, 24, 150, 102}, 4, 0x8400,
                                   NULL, 11, 4};
    TestDltHeader *save_template_pointer = &save_template;
    TestWord save_result;
    char staged_path[32768] = {0};
    unsigned char staged_counted[256] = {0};
    int save_alias_active;
    void **open_cab;
    char open_path[] = "sdm-open.doc";
    FILE *open_file;
    TestDltHeader open_template = {{8, 24, 206, 104}, 3, 0x8400,
                                   NULL, 11, 4};
    TestDltHeader *open_template_pointer = &open_template;
    TestWord open_result;
    char opened_staged_path[32768] = {0};
    unsigned char opened_staged_counted[256] = {0};
    int open_alias_active;
    FILE *browse_file;
    void **browse_open_cab;
    TestDltHeader *browse_open_template_pointer;
    TestWord browse_open;
    TestWord list_index = 0;
    char browsed_open_path[32768] = {0};
    int browsed_open_ok;
    void **browse_save_cab;
    TestDltHeader *browse_save_template_pointer;
    TestWord browse_save;
    char browsed_save_path[32768] = {0};
    int browsed_save_ok;

    if (!RunElxDispatchTest()) {
        return 31;
    }
    if (!RunMacroTypedBridgeTest()) {
        return 32;
    }

    AddDcbToLprgbst(offsets, (int)(sizeof(offsets) / sizeof(offsets[0])), 5,
                    8);
    if (memcmp(offsets, expected_offsets, sizeof(offsets)) != 0) {
        return 1;
    }

    handle = OpusHAllocateCb(4 * sizeof(int));
    if (handle == NULL || OpusCbOfH(handle) != 4 * sizeof(int)) {
        return 2;
    }
    values = (int *)OpusDerefH(handle);
    values[0] = 11;
    values[1] = 22;
    if (!OpusFChngSizeHCb(handle, 8 * sizeof(int), 1)) {
        OpusFreeH(handle);
        return 3;
    }
    values = (int *)OpusDerefH(handle);
    preserved = values[0] == 11 && values[1] == 22 &&
                OpusCbOfH(handle) == 8 * sizeof(int);
    OpusFreeH(handle);
    if (!preserved) {
        return 4;
    }

    prc_handle = OpusHAllocateCb(32);
    prc_token = OpusPrcTokenFromHandle(prc_handle);
    if (prc_handle == NULL || prc_token <= 0 || prc_token >= 0x3fff ||
        OpusPrcHandleFromToken(prc_token) != prc_handle ||
        OpusPrcTokenFromHandle(prc_handle) != prc_token) {
        OpusFreeH(prc_handle);
        return 17;
    }
    OpusFreeH(prc_handle);
    if (OpusPrcHandleFromToken(prc_token) != NULL) {
        return 18;
    }

    if (CchSz(source) != 5 || CchCopySz(source, copied) != 4 ||
        memcmp(source, copied, sizeof(source)) != 0) {
        return 5;
    }

    if (PchInSt(counted, 'b') != &counted[2] ||
        PchInSt(counted, 'z') != NULL) {
        return 6;
    }
    if (!FAlphaNum('Q') || !FAlphaNum('7') || FAlphaNum('!') ||
        !FUpper(0xD6) || !FLower(0xFF) || FDigit('A')) {
        return 7;
    }

    vitr.fFrench = 1;
    QszUpper(mixed);
    if (mixed[0] != 'A' || (unsigned char)mixed[1] != 65 ||
        FNeNcSz((const CHAR *)"author", (const CHAR *)"AUTHOR") ||
        !FNeNcSz((const CHAR *)"author", (const CHAR *)"authors")) {
        return 8;
    }

    mpdochdod[3] = &pdod;
    mpfnhfcb[4] = &pfcb;
    mpwwhwwd[5] = &pwwd;
    mpmwhmwd[6] = &pmwd;
    if (N_PdodDoc(3) != &dod || N_PfcbFn(4) != &fcb ||
        N_PwwdWw(5) != &wwd || N_HwwdWw(5) != &pwwd ||
        N_PmwdMw(6) != &mwd) {
        return 9;
    }

    motherDod = OpusTestDod(0, 0x0001, 0, 7);
    shortDod = OpusTestDod(1, 0x0800, 1, 0);
    templateDod = OpusTestDod(2, 0x0002, 0, 0);
    hMotherDod = motherDod;
    hShortDod = shortDod;
    hTemplateDod = templateDod;
    mpdochdod[1] = &hMotherDod;
    mpdochdod[2] = &hShortDod;
    mpdochdod[7] = &hTemplateDod;
    if (DocMother(1) != 1 || DocMother(2) != 1 || DocMother(0) != 0 ||
        N_PdodMother(1) != motherDod || N_PdodMother(2) != motherDod ||
        DocDotMother(1) != 7 || DocDotMother(2) != 7 ||
        DocDotMother(7) != 7 || DocDotMother(0) != 0) {
        return 10;
    }

    realWwd = OpusTestWwd(0, 6);
    hRealWwd = realWwd;
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

    blt(overlapping, overlapping + 1, 4);
    if (memcmp(overlapping, expected_overlapping, sizeof(overlapping)) != 0 ||
        !FNeRgch("Word", "word", 4) || FNeRgch("Word", "Word", 4)) {
        return 14;
    }

    OpusTestSetDodCpMac(motherDod, 103);
    if (CpMacDoc(1) != 101 || CpMacDocEdit(1) != 100 ||
        PcaSet(&range, 1, 10, 20) != &range || DcpCa(&range) != 10 ||
        !FInCa(1, 10, &range) || FInCa(1, 20, &range) ||
        PcaSetDcp(&range, 2, 30, 7)->cpLim != 37) {
        return 15;
    }

    point = MAKEPOINT(MAKELPARAM((WORD)-7, (WORD)9));
    if (point.x != -7 || point.y != 9) {
        return 16;
    }

    if (!FInitSdm_sdm21(NULL)) {
        return 19;
    }
    parent = CreateWindowExA(0, "STATIC", "SDM test parent",
                             WS_OVERLAPPED, 0, 0, 320, 200, NULL, NULL,
                             GetModuleHandleW(NULL), NULL);
    if (parent == NULL) {
        EndSdm();
        return 20;
    }
    first_initializer.hwnd = parent;
    first_initializer.dx = 4;
    first_initializer.dy = 5;
    first_initializer.flags = 2;
    first_initializer.reference = 11;
    first_initializer.runtime_items = NULL;
    second_initializer.hwnd = parent;
    second_initializer.dx = 7;
    second_initializer.dy = 8;
    second_initializer.flags = 3;
    second_initializer.reference = 22;
    second_initializer.runtime_items = NULL;
    first = HdlgStartDlg(&first_template_pointer, NULL, &first_initializer);
    SetTmcText_sdm21(0x400, first_text);
    first_host = HwndFromDlg(first);
    first_control = HwndOfTmc(0x400);

    second = HdlgStartDlg(&second_template_pointer, NULL, &second_initializer);
    SetTmcText_sdm21(0x400, second_text);
    second_host = HwndFromDlg(second);
    if (first == 0 || second == 0 || first == second || first_host == parent ||
        second_host == parent || first_host == second_host ||
        GetParent(first_host) != parent || GetParent(second_host) != parent ||
        first_control != NULL || FModalDlg(first) || !FModalDlg(second)) {
        DestroyWindow(parent);
        EndSdm();
        return 21;
    }

    HdlgSetCurDlg(first);
    GetTmcText_sdm21(0x400, text_buffer, sizeof(text_buffer));
    if (strcmp(text_buffer, "first") != 0 || !FFreeDlg() ||
        IsWindow(first_host) || !IsWindow(second_host)) {
        DestroyWindow(parent);
        EndSdm();
        return 22;
    }
    HdlgSetCurDlg(second);
    memset(text_buffer, 0, sizeof(text_buffer));
    GetTmcText_sdm21(0x400, text_buffer, sizeof(text_buffer));
    if (strcmp(text_buffer, "second") != 0) {
        DestroyWindow(parent);
        EndSdm();
        return 23;
    }

    modal_initializer.hwnd = parent;
    modal_initializer.dx = 0;
    modal_initializer.dy = 0;
    modal_initializer.flags = 1;
    modal_initializer.reference = 0;
    modal_initializer.runtime_items = NULL;
    if (TmcDoDlgDli(&modal_template_pointer, NULL, &modal_initializer) != 2 ||
        modal_init_count != 1 || modal_exit_count != 1 ||
        !IsWindow(second_host)) {
        DestroyWindow(parent);
        EndSdm();
        return 24;
    }
    if (TmcDoDlgDli(&new_modal_template_pointer, NULL,
                    &modal_initializer) != 2 ||
        new_modal_init_count != 1 || new_modal_exit_count != 1 ||
        !new_modal_controls_present || !IsWindow(second_host)) {
        DestroyWindow(parent);
        EndSdm();
        return 25;
    }

    save_cab = HcabAlloc_sdm21(save_cab_initializer);
    if (save_cab == NULL || !FSetCabSz(save_cab, selected_path, 1)) {
        if (save_cab != NULL) {
            FreeCab(save_cab);
        }
        DestroyWindow(parent);
        EndSdm();
        return 26;
    }
    SetEnvironmentVariableA("WORD1_TEST_FILE_DIALOG_PATH", NULL);
    save_result =
        TmcDoDlgDli(&save_template_pointer, save_cab, &modal_initializer);
    GetCabSz(save_cab, staged_path, sizeof(staged_path), 1);
    CountedPath(staged_path, staged_counted, sizeof(staged_counted));
    save_alias_active = OpusWin95SaveAliasMatches(staged_counted) != 0;
    if (OpusWin95SaveAliasActiveMatches(staged_counted) == 0) {
        OpusFinishWin95SaveAlias(staged_counted, 0, 0);
        DeleteFileA(selected_path);
        FreeCab(save_cab);
        DestroyWindow(parent);
        EndSdm();
        return 27;
    }
    OpusFinishWin95SaveAlias(staged_counted, 0, 0);
    DeleteFileA(selected_path);
    FreeCab(save_cab);
    if (save_result != 1 || staged_path[0] == '\0' || !save_alias_active) {
        DestroyWindow(parent);
        EndSdm();
        return 27;
    }

    open_cab = HcabAlloc_sdm21(save_cab_initializer);
    open_file = fopen(open_path, "wb");
    if (open_file != NULL) {
        fputs("open", open_file);
        fclose(open_file);
    }
    if (open_cab == NULL || open_file == NULL ||
        !FSetCabSz(open_cab, open_path, 1)) {
        if (open_cab != NULL) {
            FreeCab(open_cab);
        }
        DeleteFileA(open_path);
        DestroyWindow(parent);
        EndSdm();
        return 28;
    }
    SetEnvironmentVariableA("WORD1_TEST_FILE_DIALOG_PATH", NULL);
    open_result =
        TmcDoDlgDli(&open_template_pointer, open_cab, &modal_initializer);
    GetCabSz(open_cab, opened_staged_path, sizeof(opened_staged_path), 1);
    CountedPath(opened_staged_path, opened_staged_counted,
                sizeof(opened_staged_counted));
    open_alias_active = OpusWin95SaveAliasMatches(opened_staged_counted) != 0;
    if (OpusWin95SaveAliasActiveMatches(opened_staged_counted) != 0) {
        DeleteFileA(opened_staged_path);
        DeleteFileA(open_path);
        FreeCab(open_cab);
        DestroyWindow(parent);
        EndSdm();
        return 29;
    }
    DeleteFileA(opened_staged_path);
    DeleteFileA(open_path);
    FreeCab(open_cab);
    if (open_result != 1 || opened_staged_path[0] == '\0' ||
        !open_alias_active) {
        DestroyWindow(parent);
        EndSdm();
        return 29;
    }

    cleanup_browse_tree();
    if (!CreateDirectoryA("sdm.browse", NULL) ||
        !CreateDirectoryA("sdm.browse/sub.dir", NULL)) {
        DestroyWindow(parent);
        EndSdm();
        return 30;
    }
    browse_file = fopen("sdm.browse/sub.dir/open.doc", "wb");
    if (browse_file != NULL) {
        fputs("browse", browse_file);
        fclose(browse_file);
    }
    if (browse_file == NULL) {
        cleanup_browse_tree();
        DestroyWindow(parent);
        EndSdm();
        return 31;
    }

    browse_open_cab = HcabAlloc_sdm21(save_cab_initializer);
    if (browse_open_cab == NULL ||
        !FSetCabSz(browse_open_cab, "sdm.browse/*.doc", 1)) {
        if (browse_open_cab != NULL) {
            FreeCab(browse_open_cab);
        }
        cleanup_browse_tree();
        DestroyWindow(parent);
        EndSdm();
        return 32;
    }
    browse_open_template_pointer = &open_template;
    browse_open =
        HdlgStartDlg(&browse_open_template_pointer, browse_open_cab,
                     &modal_initializer);
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
    GetCabSz(browse_open_cab, browsed_open_path,
             sizeof(browsed_open_path), 1);
    browsed_open_ok =
        strstr(browsed_open_path, "sdm.browse") != NULL &&
        strstr(browsed_open_path, "sub.dir") != NULL &&
        strstr(browsed_open_path, "open.doc") != NULL;
    FFreeDlg();
    FreeCab(browse_open_cab);
    if (!browsed_open_ok) {
        cleanup_browse_tree();
        DestroyWindow(parent);
        EndSdm();
        return 35;
    }

    browse_save_cab = HcabAlloc_sdm21(save_cab_initializer);
    if (browse_save_cab == NULL ||
        !FSetCabSz(browse_save_cab, "save.doc", 1)) {
        if (browse_save_cab != NULL) {
            FreeCab(browse_save_cab);
        }
        cleanup_browse_tree();
        DestroyWindow(parent);
        EndSdm();
        return 36;
    }
    browse_save_template_pointer = &save_template;
    browse_save =
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
    GetCabSz(browse_save_cab, browsed_save_path,
             sizeof(browsed_save_path), 1);
    browsed_save_ok =
        strstr(browsed_save_path, "sdm.browse") != NULL &&
        strstr(browsed_save_path, "sub.dir") != NULL &&
        strstr(browsed_save_path, "save.doc") != NULL;
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
