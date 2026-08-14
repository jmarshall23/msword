#include <stdint.h>
#include <string.h>

/*
 * AMD64 entry adapters for original Microsoft C reference bodies that were
 * paired with 16-bit native assembly in the shipping build.  Structures stay
 * opaque except for the fixed four-int rectangle and one-int BRC value whose
 * by-value ABI is required by the original entry points.
 */

typedef struct OpusNativeRect {
    int left;
    int top;
    int right;
    int bottom;
} OpusNativeRect;

typedef struct OpusNativeBrc {
    int value;
} OpusNativeBrc;

typedef union OpusNativeFcid {
    uint32_t value;
} OpusNativeFcid;

int C_DisplayFli(int ww, void** hpldr, int idr, int dl, int yp_line);
int C_DisplayFliCore(int ww, OpusNativeRect clip, int dxp_to_xw, int yw_line);
int C_AddVisiSpaces(void* hdc, int dxp_to_xw, int yw_line,
                    OpusNativeRect* clip);
int C_DrawInsertLine(void* selection);

int C_FUpdateDr(int ww, void** hpldr, int idr, OpusNativeRect invalid,
                int abort_ok, int update_mode, long cp_update);
int C_ScrollDrUp(int ww, void** hpldr, int idr, void** hplcedl, int dl_from,
                 int dl_to, int yp_from, int yp_to, int yp_first_show,
                 int yp_lim_show);
int C_FUpdateTable(int ww, int doc, void** hpldr, int idr, long* cp,
                   int* yp_top, void** hplcedl, int dl_new, int dl_old,
                   int dl_mac, int yp_first_show, int yp_lim_ww, int yp_lim_dr,
                   OpusNativeRect invalid, int scroll_ok);
int C_FUpdTableDr(int ww, void** hpldr, int idr);
int C_WidthHeightFromBrc(OpusNativeBrc brc, int flags);
int C_FrameEasyTable(int ww, int doc, long cp, void** hpldr,
                     OpusNativeRect* drawn, void* parent_dr, int dyl,
                     int frame_lines, int dr_frame_lines, int first_row,
                     int last_row);

int C_FNewChpIns(int doc, long cp, void* chp, int stc);
int C_FAddRun(int fn, long fc_lim, char* properties, int property_count,
              void* fkpd, int flags);
int C_CbGenPrl(char* properties, char* base_properties, int sprm,
               char* result);
int C_CbGenChpxFromChp(void* result, void* chp, void* base_chp);
int C_ValFromPropSprm(char* properties, int sprm);
int C_AdjustCp(void* range, long inserted_count);
int C_InvalCaFierce();
int C_ItcGetTcxCache(int ww, int doc, long cp, void* tap, int itc, void* tcx);
int C_ItcGetTcx(int ww, void* tap, int itc, void* tcx);
int C_LbcFormatPara(void* lbs, int dyl_fill, int cl_lim);
int C_IbpLru();
int C_IbstFindSzFfn(void** string_table, void* font);
int C_LoadFont(void* chp, int widths_only);
int C_LoadFcidFull(OpusNativeFcid fcid);
int C_ResetFont(int printer_font);
int C_FGraphicsFcidToPlf(OpusNativeFcid fcid, void* logical_font,
                         int printer_font);
int C_FfcFormatFieldPdcp(long* dcp, int ww, int doc, long cp, int ch);
long C_DcpSkipFieldChPflcd(int ch, void* field, int show_result, int fetch);
int C_FShowResultPflcdFvc(void* field, int view);
int C_FillIfldFlcd(void** field_plc, int field_index, void* field);
int C_GetIfldFlcd(int doc, int field_index, void* field);
int C_SetFlcdCh(int doc, void* field, int ch);
int C_IfldInsertDocCp(int doc, long cp);
int C_FetchVisibleRgch(void* visible_fetch, int view, int properties,
                       int field_fetch);
int C_FetchCpPccpVisible(int doc, long cp, int* count, int view,
                         int field_fetch);
long C_CpVisibleBackCpField(int doc, long cp, int view);
long C_CpVisibleCpField(int doc, long cp, int view, int insertion);
int C_FVisibleCp(int ww, int doc, long cp);
int C_FCpVisiInOutline(int ww, int doc, long cp, int count,
                       long* first_invisible);
int C_FReplace(void* range, int fn, long fc, long count);
int C_XReplace(int plan, void* transaction, void* replace_state);
int C_FRepl1(void* range, int fn, long fc, long count);
int C_IpcdSplit(void** piece_plc, long cp);
int C_XRepl1(int plan, void* transaction, void* replace_state);
int C_FReplaceCps(void* delete_range, void* insert_range);
int C_XReplaceCps(int plan, void* transaction, void* replace_state);
char* C_PxsInit(char* replace_states, int state_count, void* transaction);
int C_PostTn(void* transaction, int intention, void** handle, int count);
int C_FDoTns(void* transaction);
int C_CloseTns(void* transaction);
int C_XDelFndSedPgdPad(int plan, void* transaction, void* replace_state);
int C_CbGenPapxFromPap(char* papx, void* pap, void* base_pap);
long C_CpFormatFrom(int ww, int doc, long cp);
void C_GetCpFirstCpLimDisplayPara(int ww, int doc, long cp, long* first,
                                  long* limit);
int C_FAssignLr(void* layout_state, int dyl_fill, int empty_ok);
int C_FAbortLayout(int outline, void* layout_state);
int C_FWidowControl(void* layout_state, void* new_layout_state, int dyl_fill,
                    int empty_ok);
int C_XpFromDcp(long dcp_first, long dcp_limit, unsigned* xp_first,
                int* character_index);
int C_FFillRgwWithSeqLevs(int doc, long cp, int outline_index,
                          int field_index, void** outline_plc,
                          void** field_plc, int* levels);
int C_FGetParaState(int all, int abort_ok);
int C_FMarkLine(void* selection, void** display_regions, int region_index,
                void* display_line, long selection_first,
                long selection_limit, long line_first);
int C_IfrdGatherFtnRef(void* layout_state, void* new_layout_state,
                       int reference_index, int normal, int reject_y,
                       int accept_y);
int C_FGetFtnBreak(void* layout_state, int line_index, void* footnote_line,
                   int placement);
char* C_PchSzRtfMove(int symbol_index, char* destination);
int C_FSearchRgrsym(char* symbol, int* symbol_index);
int C_RtfIn(void** input_block, char* text, int text_count);
char C_ChMapSpecChar(char character, int character_set);
int C_DxaFromDxp(int pixels);
int C_DxpFromDxa(int twips);
int C_MiscPlcLoops(void** plc, int first, int limit, char* result,
                   int operation);
int C_WCompSzSrt(char* first, char* second, int case_sensitive);
int C_WCompChCh(char first, char second);

void _BLTBH(void* source, void* destination, unsigned count) {
    memmove(destination, source, count);
}

long _UOP_LONG(int ignored, int high, int low) {
    (void)ignored;
    return (long)(((uint32_t)high << 16u) | ((uint32_t)low & 0xffffu));
}

int N_DisplayFli(int ww, void** hpldr, int idr, int dl, int yp_line) {
    C_DisplayFli(ww, hpldr, idr, dl, yp_line);
    return 0;
}

int N_DisplayFliCore(int ww, OpusNativeRect clip, int dxp_to_xw,
                     int yw_line) {
    C_DisplayFliCore(ww, clip, dxp_to_xw, yw_line);
    return 0;
}

int N_AddVisiSpaces(void* hdc, int dxp_to_xw, int yw_line,
                    OpusNativeRect* clip) {
    C_AddVisiSpaces(hdc, dxp_to_xw, yw_line, clip);
    return 0;
}

int N_DrawInsertLine(void* selection) {
    C_DrawInsertLine(selection);
    return 0;
}

int N_FUpdateDr(int ww, void** hpldr, int idr, OpusNativeRect invalid,
                int abort_ok, int update_mode, long cp_update) {
    return C_FUpdateDr(ww, hpldr, idr, invalid, abort_ok, update_mode,
                       cp_update);
}

int N_ScrollDrUp(int ww, void** hpldr, int idr, void** hplcedl, int dl_from,
                 int dl_to, int yp_from, int yp_to, int yp_first_show,
                 int yp_lim_show) {
    C_ScrollDrUp(ww, hpldr, idr, hplcedl, dl_from, dl_to, yp_from, yp_to,
                 yp_first_show, yp_lim_show);
    return 0;
}

int N_FUpdateTable(int ww, int doc, void** hpldr, int idr, long* cp,
                   int* yp_top, void** hplcedl, int dl_new, int dl_old,
                   int dl_mac, int yp_first_show, int yp_lim_ww, int yp_lim_dr,
                   OpusNativeRect invalid, int scroll_ok) {
    return C_FUpdateTable(ww, doc, hpldr, idr, cp, yp_top, hplcedl, dl_new,
                          dl_old, dl_mac, yp_first_show, yp_lim_ww, yp_lim_dr,
                          invalid, scroll_ok);
}

int N_FUpdTableDr(int ww, void** hpldr, int idr) {
    return C_FUpdTableDr(ww, hpldr, idr);
}

int N_WidthHeightFromBrc(OpusNativeBrc brc, int flags) {
    return C_WidthHeightFromBrc(brc, flags);
}

int N_FrameEasyTable(int ww, int doc, long cp, void** hpldr,
                     OpusNativeRect* drawn, void* parent_dr, int dyl,
                     int frame_lines, int dr_frame_lines, int first_row,
                     int last_row) {
    return C_FrameEasyTable(ww, doc, cp, hpldr, drawn, parent_dr, dyl,
                            frame_lines, dr_frame_lines, first_row, last_row);
}

int N_FNewChpIns(int doc, long cp, void* chp, int stc) {
    return C_FNewChpIns(doc, cp, chp, stc);
}

int N_FAddRun(int fn, long fc_lim, char* properties, int property_count,
              void* fkpd, int flags) {
    return C_FAddRun(fn, fc_lim, properties, property_count, fkpd, flags);
}

int N_CbGenPrl(char* properties, char* base_properties, int sprm,
               char* result) {
    return C_CbGenPrl(properties, base_properties, sprm, result);
}

int N_CbGenChpxFromChp(void* result, void* chp, void* base_chp) {
    return C_CbGenChpxFromChp(result, chp, base_chp);
}

int N_ValFromPropSprm(char* properties, int sprm) {
    return C_ValFromPropSprm(properties, sprm);
}

int N_AdjustCp(void* range, long inserted_count) {
    C_AdjustCp(range, inserted_count);
    return 0;
}

int N_InvalCaFierce() {
    C_InvalCaFierce();
    return 0;
}

int N_ItcGetTcxCache(int ww, int doc, long cp, void* tap, int itc, void* tcx) {
    return C_ItcGetTcxCache(ww, doc, cp, tap, itc, tcx);
}

int N_ItcGetTcx(int ww, void* tap, int itc, void* tcx) {
    return C_ItcGetTcx(ww, tap, itc, tcx);
}

int N_LbcFormatPara(void* lbs, int dyl_fill, int cl_lim) {
    return C_LbcFormatPara(lbs, dyl_fill, cl_lim);
}

int N_IbpLru() {
    return C_IbpLru();
}

int N_IbstFindSzFfn(void** string_table, void* font) {
    return C_IbstFindSzFfn(string_table, font);
}

int N_LoadFont(void* chp, int widths_only) {
    C_LoadFont(chp, widths_only);
    return 0;
}

int N_LoadFcidFull(OpusNativeFcid fcid) {
    C_LoadFcidFull(fcid);
    return 0;
}

int N_ResetFont(int printer_font) {
    C_ResetFont(printer_font);
    return 0;
}

int N_FGraphicsFcidToPlf(OpusNativeFcid fcid, void* logical_font,
                         int printer_font) {
    return C_FGraphicsFcidToPlf(fcid, logical_font, printer_font);
}

int N_FfcFormatFieldPdcp(long* dcp, int ww, int doc, long cp, int ch) {
    return C_FfcFormatFieldPdcp(dcp, ww, doc, cp, ch);
}

long N_DcpSkipFieldChPflcd(int ch, void* field, int show_result, int fetch) {
    return C_DcpSkipFieldChPflcd(ch, field, show_result, fetch);
}

int N_FShowResultPflcdFvc(void* field, int view) {
    return C_FShowResultPflcdFvc(field, view);
}

int N_FillIfldFlcd(void** field_plc, int field_index, void* field) {
    C_FillIfldFlcd(field_plc, field_index, field);
    return 0;
}

int N_GetIfldFlcd(int doc, int field_index, void* field) {
    C_GetIfldFlcd(doc, field_index, field);
    return 0;
}

int N_SetFlcdCh(int doc, void* field, int ch) {
    C_SetFlcdCh(doc, field, ch);
    return 0;
}

int N_IfldInsertDocCp(int doc, long cp) {
    return C_IfldInsertDocCp(doc, cp);
}

int N_FetchVisibleRgch(void* visible_fetch, int view, int properties,
                       int field_fetch) {
    C_FetchVisibleRgch(visible_fetch, view, properties, field_fetch);
    return 0;
}

int N_FetchCpPccpVisible(int doc, long cp, int* count, int view,
                         int field_fetch) {
    C_FetchCpPccpVisible(doc, cp, count, view, field_fetch);
    return 0;
}

long N_CpVisibleBackCpField(int doc, long cp, int view) {
    return C_CpVisibleBackCpField(doc, cp, view);
}

long N_CpVisibleCpField(int doc, long cp, int view, int insertion) {
    return C_CpVisibleCpField(doc, cp, view, insertion);
}

int N_FVisibleCp(int ww, int doc, long cp) {
    return C_FVisibleCp(ww, doc, cp);
}

int N_FCpVisiInOutline(int ww, int doc, long cp, int count,
                       long* first_invisible) {
    return C_FCpVisiInOutline(ww, doc, cp, count, first_invisible);
}

int N_FReplace(void* range, int fn, long fc, long count) {
    return C_FReplace(range, fn, fc, count);
}

int N_XReplace(int plan, void* transaction, void* replace_state) {
    C_XReplace(plan, transaction, replace_state);
    return 0;
}

int N_FRepl1(void* range, int fn, long fc, long count) {
    return C_FRepl1(range, fn, fc, count);
}

int N_IpcdSplit(void** piece_plc, long cp) {
    return C_IpcdSplit(piece_plc, cp);
}

int N_XRepl1(int plan, void* transaction, void* replace_state) {
    C_XRepl1(plan, transaction, replace_state);
    return 0;
}

int N_FReplaceCps(void* delete_range, void* insert_range) {
    return C_FReplaceCps(delete_range, insert_range);
}

int N_XReplaceCps(int plan, void* transaction, void* replace_state) {
    C_XReplaceCps(plan, transaction, replace_state);
    return 0;
}

void* N_PxsInit(void* replace_states, int state_count, void* transaction) {
    return C_PxsInit((char*)replace_states, state_count, transaction);
}

int N_PostTn(void* transaction, int intention, void** handle, int count) {
    C_PostTn(transaction, intention, handle, count);
    return 0;
}

int N_FDoTns(void* transaction) {
    return C_FDoTns(transaction);
}

int N_CloseTns(void* transaction) {
    C_CloseTns(transaction);
    return 0;
}

int N_XDelFndSedPgdPad(int plan, void* transaction, void* replace_state) {
    C_XDelFndSedPgdPad(plan, transaction, replace_state);
    return 0;
}

int N_CbGenPapxFromPap(char* papx, void* pap, void* base_pap) {
    return C_CbGenPapxFromPap(papx, pap, base_pap);
}

long N_CpFormatFrom(int ww, int doc, long cp) {
    return C_CpFormatFrom(ww, doc, cp);
}

void N_GetCpFirstCpLimDisplayPara(int ww, int doc, long cp, long* first,
                                  long* limit) {
    C_GetCpFirstCpLimDisplayPara(ww, doc, cp, first, limit);
}

int N_FAssignLr(void* layout_state, int dyl_fill, int empty_ok) {
    return C_FAssignLr(layout_state, dyl_fill, empty_ok);
}

int N_FAbortLayout(int outline, void* layout_state) {
    return C_FAbortLayout(outline, layout_state);
}

int N_FWidowControl(void* layout_state, void* new_layout_state, int dyl_fill,
                     int empty_ok) {
    return C_FWidowControl(layout_state, new_layout_state, dyl_fill, empty_ok);
}

int N_XpFromDcp(long dcp_first, long dcp_limit, unsigned* xp_first,
                int* character_index) {
    return C_XpFromDcp(dcp_first, dcp_limit, xp_first, character_index);
}

int N_FFillRgwWithSeqLevs(int doc, long cp, int outline_index,
                           int field_index, void** outline_plc,
                           void** field_plc, int* levels) {
    return C_FFillRgwWithSeqLevs(doc, cp, outline_index, field_index,
                                 outline_plc, field_plc, levels);
}

int N_FGetParaState(int all, int abort_ok) {
    return C_FGetParaState(all, abort_ok);
}

int N_FMarkLine(void* selection, void** display_regions, int region_index,
                void* display_line, long selection_first,
                long selection_limit, long line_first) {
    return C_FMarkLine(selection, display_regions, region_index, display_line,
                       selection_first, selection_limit, line_first);
}

int N_IfrdGatherFtnRef(void* layout_state, void* new_layout_state,
                       int reference_index, int normal, int reject_y,
                       int accept_y) {
    return C_IfrdGatherFtnRef(layout_state, new_layout_state, reference_index,
                              normal, reject_y, accept_y);
}

int N_FGetFtnBreak(void* layout_state, int line_index, void* footnote_line,
                   int placement) {
    return C_FGetFtnBreak(layout_state, line_index, footnote_line, placement);
}

char* N_PchSzRtfMove(int symbol_index, char* destination) {
    return C_PchSzRtfMove(symbol_index, destination);
}

int N_FSearchRgrsym(char* symbol, int* symbol_index) {
    return C_FSearchRgrsym(symbol, symbol_index);
}

void N_RtfIn(void** input_block, char* text, int text_count) {
    C_RtfIn(input_block, text, text_count);
}

char N_ChMapSpecChar(char character, int character_set) {
    return C_ChMapSpecChar(character, character_set);
}

int N_DxaFromDxp(int pixels) {
    return C_DxaFromDxp(pixels);
}

int N_DxpFromDxa(int twips) {
    return C_DxpFromDxa(twips);
}

void N_MiscPlcLoops(void** plc, int first, int limit, char* result,
                    int operation) {
    C_MiscPlcLoops(plc, first, limit, result, operation);
}

int N_WCompSzSrt(char* first, char* second, int case_sensitive) {
    return C_WCompSzSrt(first, second, case_sensitive);
}

int N_WCompChCh(char first, char second) {
    return C_WCompChCh(first, second);
}
