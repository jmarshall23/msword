#pragma once

#include <stddef.h>

/*
 * Native-layout accessors shared by the AMD64 assembly translations.
 * The implementations are compiled as C against the original Opus headers,
 * so Microsoft C owns the historical bitfield layout instead of duplicating
 * offsets in the translated C++ routines.
 */

#ifdef __cplusplus
extern "C" {
#endif

int OpusDodIsMother(const void* pdod);
int OpusDodIsDocument(const void* pdod);
int OpusDodIsTemplate(const void* pdod);
int OpusDodMotherDoc(const void* pdod);
int OpusDodTemplateDoc(const void* pdod);
long OpusDodCpMac(const void* pdod);
int OpusWwdMw(const void* pwwd);
void OpusWwdCoordinateOffsets(const void* pwwd, int* page_x, int* page_y,
                              int* window_x, int* window_y);
void* OpusSttbData(void* psttb);
int OpusSttbCount(const void* psttb);
int OpusSttbUsesStyleRules(const void* psttb);
size_t OpusSttbDataSize(void** hsttb);
void* OpusPlcCpData(void* pplc);
int OpusPlcIMac(const void* pplc);
int OpusPlcIMax(const void* pplc);
int OpusPlcEntrySize(const void* pplc);
int OpusPlcAdjustIndex(const void* pplc);
long OpusPlcAdjustment(const void* pplc);
int OpusPlcHasMultipleCps(const void* pplc);
void OpusPlcSetHint(void* pplc, int index);
void OpusPlcSetAdjustIndex(void* pplc, int index);
void OpusPlcClearAdjustment(void* pplc);
void* OpusPlData(void* ppl);
int OpusPlCount(const void* ppl);
int OpusPlEntrySize(const void* ppl);
int OpusDiskPlcSize(int entry_count, int runtime_entry_size);
int OpusPackPlcDisk(const void* pplc, int entry_count, long cp_last_override,
                    void* bytes, int byte_count);
int OpusUnpackPlcDisk(void* pplc, int entry_count, const void* bytes,
                      int byte_count);
int OpusPackPcdDisk(const void* pcd, void* bytes);
int OpusUnpackPcdDisk(void* pcd, const void* bytes);
int OpusPackSedDisk(const void* sed, void* bytes);
int OpusUnpackSedDisk(void* sed, const void* bytes);
int OpusDiskFibSize(void);
int OpusDiskFileHeaderSize(void);
long OpusDiskFibFcMin(const void* bytes, int byte_count);
int OpusPackFibDisk(const void* fib, void* bytes, int byte_count);
int OpusUnpackFibDisk(void* fib, const void* bytes, int byte_count);
int OpusDiskKmeSize(void);
int OpusPackKmeDisk(const void* kme, int value, void* bytes);
int OpusUnpackKmeDisk(void* kme, const void* bytes);

#ifdef __cplusplus
}
#endif
