#pragma once

#include <Windows.h>

#ifndef MAX_SYM_NAME
#define MAX_SYM_NAME 2000
#endif
#ifndef SYMOPT_LOAD_LINES
#define SYMOPT_LOAD_LINES 0x00000010
#endif
#ifndef SYMOPT_UNDNAME
#define SYMOPT_UNDNAME 0x00000002
#endif
#ifndef SYMOPT_DEFERRED_LOADS
#define SYMOPT_DEFERRED_LOADS 0x00000004
#endif

typedef struct _SYMBOL_INFO {
    ULONG SizeOfStruct;
    ULONG TypeIndex;
    ULONG64 Reserved[2];
    ULONG Index;
    ULONG Size;
    ULONG64 ModBase;
    ULONG Flags;
    ULONG64 Value;
    ULONG64 Address;
    ULONG Register;
    ULONG Scope;
    ULONG Tag;
    ULONG NameLen;
    ULONG MaxNameLen;
    CHAR Name[1];
} SYMBOL_INFO, *PSYMBOL_INFO;

typedef struct _IMAGEHLP_LINE64 {
    DWORD SizeOfStruct;
    PVOID Key;
    DWORD LineNumber;
    PCHAR FileName;
    DWORD64 Address;
} IMAGEHLP_LINE64, *PIMAGEHLP_LINE64;

typedef enum _ADDRESS_MODE {
    AddrMode1616,
    AddrMode1632,
    AddrModeReal,
    AddrModeFlat
} ADDRESS_MODE;

typedef struct _ADDRESS64 {
    DWORD64 Offset;
    WORD Segment;
    ADDRESS_MODE Mode;
} ADDRESS64, *LPADDRESS64;

typedef struct _KDHELP64 {
    DWORD64 Thread;
    DWORD ThCallbackStack;
    DWORD ThCallbackBStore;
    DWORD NextCallback;
    DWORD FramePointer;
    DWORD64 KiCallUserMode;
    DWORD64 KeUserCallbackDispatcher;
    DWORD64 SystemRangeStart;
    DWORD64 KiUserExceptionDispatcher;
    DWORD64 StackBase;
    DWORD64 StackLimit;
    DWORD64 Reserved[5];
} KDHELP64, *PKDHELP64;

typedef struct _STACKFRAME64 {
    ADDRESS64 AddrPC;
    ADDRESS64 AddrReturn;
    ADDRESS64 AddrFrame;
    ADDRESS64 AddrStack;
    ADDRESS64 AddrBStore;
    PVOID FuncTableEntry;
    DWORD64 Params[4];
    BOOL Far;
    BOOL Virtual;
    DWORD64 Reserved[3];
    KDHELP64 KdHelp;
} STACKFRAME64, *LPSTACKFRAME64;

#ifdef __cplusplus
extern "C" {
#endif

DWORD SymSetOptions(DWORD options);
BOOL SymInitialize(HANDLE process, LPCSTR user_search_path,
                   BOOL invade_process);
BOOL SymFromAddr(HANDLE process, DWORD64 address,
                 DWORD64* displacement, PSYMBOL_INFO symbol);
BOOL SymGetLineFromAddr64(HANDLE process, DWORD64 address,
                          DWORD* displacement, PIMAGEHLP_LINE64 line);
BOOL SymCleanup(HANDLE process);
PVOID SymFunctionTableAccess64(HANDLE process, DWORD64 address_base);
DWORD64 SymGetModuleBase64(HANDLE process, DWORD64 address);
BOOL StackWalk64(DWORD machine_type, HANDLE process, HANDLE thread,
                 LPSTACKFRAME64 stack_frame, PVOID context_record,
                 PVOID read_memory_routine,
                 PVOID (*function_table_access_routine)(HANDLE, DWORD64),
                 DWORD64 (*get_module_base_routine)(HANDLE, DWORD64),
                 PVOID translate_address);

#ifdef __cplusplus
}
#endif
