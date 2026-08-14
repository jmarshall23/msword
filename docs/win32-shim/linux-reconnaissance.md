# Linux Reconnaissance Notes

This imports the still-useful findings from
`DONT-MERGE/jphonorato/docs/port-linux/00-reconocimiento.md` for this
Win32-on-SDL tree. The source document studied a Winelib route. Wine-specific build
mechanics are historical context here; the reusable findings are the compiler-surface
inventory, the LP64 risks, the build-generator assumptions, and the error-family
triage.

## Baseline Counts

Source §0 says that, after generated headers and compatibility flags, 156 of the 207
engine translation units compiled without editing `src/Opus/`, leaving 186 errors in
51 files. The earlier reconnaissance count was 154 of 207 with 202 errors.

Source §6.2 records the measured method: compile the 207 sources from
`opus_original_engine` with generated headers, case-compatible include paths, and
`-std=gnu89 -funsigned-char -fms-extensions -fpermissive`.

The portable lesson is that the porting surface is not primarily MSVC keywords. It is
mostly old C accepted by MSVC, generated-resource assumptions, and LP64/LLP64 width
differences.

## MSVC Surface Inventory

Source §1.1 found the segmented vocabulary already neutralized:

| Construct | Count | Current relevance |
|---|---:|---|
| `PASCAL` | 210 | Already mapped away by compatibility headers. |
| `FAR` | 557 | Already mapped away. |
| `HUGE` | 556 | Already mapped away. |
| `NEAR` | 45 | Already mapped away. |
| `__stdcall` / `_stdcall` | 2 | Only in `opus_asm_resn2_sttb.cpp`; not a practical x86-64 blocker. |

Source §1.2 found only three `__declspec` cases: one `selectany` table and two
`dllimport` declarations in `opus_asm_resn2_sttb.cpp`. Source §1.3 found
`#pragma pack(push/pop)` and `#pragma once` compatible with GCC. The problematic
pragma was the generated command export pragma:
`#pragma comment(linker, "/export:NAME")`.

Source §1.4 found no real inline assembly, no intrinsic dependency, and no
`__int8`-style integer types. The only `_asm` match was inside a comment. The original
assembly had already been translated into `src/port/original/opus-asm-*` sources.

Source §1.5 put the Microsoft secure-CRT gap inside modifiable port code:
`_snprintf_s`, `_vsnwprintf_s`, `_countof`, `_stricmp`, and `_wcsicmp`. The same
section also found `_stricmp` in the host `mkcmd.c` tool, where `strcasecmp` is the
direct host replacement.

Source §1.6 is the important compiler inventory:

| Error family | Source count | Why it matters here |
|---|---:|---|
| cast as lvalue | 21 plus assignment variants | Rewrite to standard assignments; do not preserve the MSVC extension. |
| non-load-time initializer | 90 | All came from `keys.h` function-pointer rows entering an integer union member. |
| K&R definition conflicts | 13 | Usually fixed by adding or aligning prototypes. |
| `static` after implicit non-static declaration | 10 | Mechanical forward declarations before first use. |
| calls through empty K&R function-pointer prototypes | 9 | Requires real target signatures, not a generic cast. |
| nameless nested struct | removed by `-fms-extensions` | `PAP`/`PAPS` in `props.h`; solved by the compatibility flag. |

Source §1.7 proved that Word resolves commands through
`GetProcAddress(GetModuleHandleW(NULL), name)`, with 427 generated exports. The Winelib
experiment proved `.spec` exports could satisfy that shape. For this SDL shim the
reusable requirement is narrower: command lookup still needs the generated command
names to be visible to the module resolver. The Wine `.spec` mechanism itself is not a
current instruction.

## LP64 And Width Risks

Source §4.1 measured the width split as `long=8 LONG=4 DWORD=4 ptr=8 int=4
wchar_t=2` under the Winelib toolchain. The key risk survives without Wine: LP64 hosts
make bare `long` 8 bytes, while Windows `LONG` and `DWORD` are 4 bytes.

The source counted 1254 bare `long` occurrences in 206 files. It named these
concentrators:

| File | Source count |
|---|---:|
| `port/original/opus_asm_native_adapters.cpp` | 54 |
| `Opus/CLIPBRD2.C` | 37 |
| `Opus/wordtech/plc.c` | 29 |
| `Opus/wordtech/savefast.c` | 25 |
| `Opus/wordtech/file.h` | 25 |

The actionable subset is not every `long`. Source §4.1 and the LP64 audit note point
to structures that reach `fread`, `fwrite`, PLC tables, or `sizeof` checks. Ordinary
runtime counters do not need serialization tests.

Current-tree correction to source §4.1: the source claimed `Opus/lib/qwindows.h:119`
was reachable through `Opus/interp/sym.c`. In this tree, `sym.c` and the other
interpreter includes select `opus_x64_compat.h` under `OPUS_X64` and include
`qwindows.h` only in the `#else` branch. `wordtech/word.h` follows the same pattern,
and `Opus/windows.h` is not the active route. Do not treat `qwindows.h` as the current
LP64 fix target.

The real current targets are the serialized or wire-format uses in `wordtech/plc.c`,
`savefast.c`, `file.h`, `CLIPBRD2.C`, and the shim's own Win32 type definitions. In
the shim, `LONG`, `DWORD`, `POINT`, `RECT`, and `MAKELONG` must keep Windows widths on
LP64 hosts.

Source §4.2 found a separate wide-character trap specific to Wine's
`-fshort-wchar`: libstdc++ was built with 4-byte `wchar_t`, while Wine forced 2-byte
`wchar_t`, so `std::wstring` could link and still misbehave at runtime. For this SDL
shim the useful rule is to keep host C++ string and filesystem code out of the
Windows-wide-character boundary unless a checked conversion is explicit.

## Build-Generator Assumptions

Source §5.1 says the Visual Studio presets are not portable to Ninja: the generator,
`architecture` field, and Debug/Release multi-config assumptions need separate Ninja
presets rather than edits that break Windows.

Source §5.2 identifies hard blockers that were build-system assumptions, not engine
code:

| Assumption | Reusable fix shape |
|---|---|
| top-level non-Windows fatal error | Gate by the chosen non-Windows port path instead of blocking configuration. |
| unconditional RC language | Keep RC behind Windows support or convert resource generation into explicit commands. |
| Windows Kits SDK discovery via Visual Studio cache variables | Route SDK or shim headers through an explicit configured header. |
| generated-header scripts referenced but absent | Replace missing generator scripts with tracked, host-portable generators. |

Source §5.3 found `src/cmake/` absent from every commit in that source baseline, so the
build was incomplete even before Linux. Source §5.4 classifies Visual Studio target
properties such as `FOLDER` and debugger working directories as harmless under Ninja;
keep them when preserving Windows behavior.

Source §5.5 found three command-line traps:

| Trap | Reusable result |
|---|---|
| uppercase `.C` sources treated as C++ | Materialize or list lowercase C paths for non-Windows toolchains. |
| MSVCRT mode conflicting with libstdc++ | Provide the few Microsoft CRT names locally instead of switching the whole port to MSVCRT headers. |
| `native`, `string`, and `sys` macros poisoning C++ headers | Keep those macros out of C++ standard-library includes. |

## LP64 Audit Targets

The LP64 audit note in source §8 says to inspect bare `long` uses that participate in
serialized structures, PLC tables, and `sizeof` calculations. It specifically names
`Opus/wordtech/plc.c`, `savefast.c`, `file.h`, `CLIPBRD2.C`, and
`port/original/opus_asm_native_adapters.cpp`.

The concrete precedent is source §8 plus the host-tool section: `bitapp.h` used
`typedef unsigned long DWORD`, which made `sizeof(BITMAP)` 18 bytes on LP64 instead of
the Win16 14-byte layout. That caused `fread` to consume four extra bytes per bitmap
resource and produced corrupt output before failing. This is the pattern to search for
in Word file formats: host `sizeof` must not define on-disk width.

For this tree, the audit output should separate:

| Class | Required evidence |
|---|---|
| on-disk or wire-format structure | fixed expected size, static assertion, and read/write test when practical |
| runtime-only structure | no serialization test unless it crosses a file, DDE, clipboard, or resource boundary |
| shim Win32 type | compile-time assertion that Windows widths hold on LP64 |

## Error-Family Triage

Source "Triage estructurado de los 182 errores" maps the 182-error baseline into five
families:

| Family | Count | Review rule |
|---|---:|---|
| `keys.h` function-pointer initializer | 90 | Root cause is the first union member width in `KME`, not 90 independent rows. |
| cast as lvalue | 36 | Rewrite mechanically only when the pointer walk is width-neutral; inspect `CHR`/`CHP`/`CHRT` walks separately. |
| `static` after implicit declaration | 10 | Add matching forward declarations before first use. |
| conflicting types | 13 | Split true LP64 width conflicts from K&R prototype conflicts. |
| `FARPROC` called with arguments | 7 remaining in that snapshot | Use a target-specific function-pointer typedef for each ordinal/API call. |

Source "Ronda de correcciones cast-as-lvalue y LP64" records two important review
lessons:

- Most cast-as-lvalue rewrites can be standard C assignments with no preprocessor
  split, because MSVC also accepts the standard form.
- `exp.c` packing is not a syntax cleanup. It writes into `int rgwArgs[]`, and the
  port-side caller passes one `int` slot per argument. Any 8-byte host `long` write
  changes the call ABI unless the value is intentionally kept at a 32-bit slot width
  or the caller learns argument widths.

## Current Use

Use this document to avoid reopening already answered questions:

- Do not spend time on segmented keywords, inline assembly, or `__intN` types.
- Treat LP64 serialization as a targeted file-format audit, not a global `long`
  replacement.
- Preserve Windows presets while adding non-Windows build paths.
- Keep Wine `.spec`, `winegcc`, and `wrc` details as historical evidence only; they are
  not SDL-shim implementation steps.
- For old C compiler errors, fix the root family once instead of patching each emitted
  line independently.
