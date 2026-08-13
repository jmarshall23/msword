# TODO

Goal: make this tree build and run on macOS, then GNU/Linux, then WebAssembly, by
implementing the Win32 subset Word actually calls on top of SDL, without rewriting
Word.

Four rules that decide every item below:

1. `src/Opus/` and `src/OpusEtAl/` stay as they are. Microsoft's source is the
   product; if a change is unavoidable it goes behind a guard and gets justified.
2. `src/port/original/` keeps calling Win32. We do not port it to SDL. We implement
   Win32 beneath it.
3. New platform code lives in `src/port/win32/`. Nothing else grows a platform `#ifdef`.
4. If a shim entry point is not reached by this codebase, it does not get written.

Every item states what to do and a check that fails until it is done. Two independent
agents reviewed it twice. Their corrections are folded in, and every place a review was
wrong, or an earlier draft of this file was wrong, is marked in the item so nobody
re-opens it.

## Scope

`docs/win32-shim/api-inventory.tsv` lists 333 Win32 entry points over 4521 call sites
(3120 in `src/Opus`, 1401 in `src/port`), sorted by call count.

| DLL | Entry points |
|---|---:|
| user32 | 173 |
| kernel32 | 84 |
| gdi32 | 72 |
| comdlg32 | 3 |
| shell32 | 1 |

It is a lower bound, not an authority, and the second review found out why. The first
version was built by intersecting call sites with an export dump of real Windows ME
DLLs, which lists only true exports. Word calls the TCHAR macro spellings, so
`SendMessage` (343 sites) was filed under `SendMessageA` with a count of zero, and the
same for `PostMessage`, `TextOut`, `DefWindowProc`, `GetMessage`, `GetModuleHandle`,
`LoadLibrary`, `DispatchMessage` and `CreateFontIndirect`. Folding `NAME`, `NAMEA` and
`NAMEW` into one row recovered 455 call sites.

Nine more had to be added by hand because only the macro form is ever called, so no
export name matched: `PeekMessage`, `GetWindowLong`, `SetWindowLong`, `CreateWindow`,
`RegisterClass`, `LoadBitmap`, `GetTextExtent`, `GetTextMetrics`, `CreateIC`. Note
`CreateWindow` and `RegisterClass` among them, without which nothing runs at all. Any
API that Win16 had and Win32 dropped is invisible to this method by construction, so
assume more are missing and treat item 11's link-time gate, not this file, as the
authority.

Read `docs/win32-shim/README.md` before using it. In particular, `Beep`, `DeleteAtom`
and `IsZoomed` are Word's own routines, not imports, so their high call counts are
noise; and the shim must be a static library so Word's definitions win at link.

Two things not in the way: zero SEH and zero inline assembly across the whole tree.
The MSVC-only constructs are confined to four files, `opus_asm_resn2_sttb.cpp`
(`__declspec(dllimport)`), `opus_original_startup_probe.cpp` (`<rtcapi.h>`, `wWinMain`),
`opus_product_entry.cpp` (`wWinMain`), and `opus_win95_chrome.cpp` (`<windowsx.h>`,
`uxtheme.dll`). Only three of those are live: `opus_product_entry.cpp` is in no CMake
target and is never compiled, which item 7 picks up.

## What the five reference trees were worth

| Tree | Taken | Left |
|---|---|---|
| `DONT-MERGE/Win32Emu` | Extracted to `docs/win32-shim/` before deletion: the API inventory, the GDI state models, the ROP set, the WASM findings | The emulator itself. C# over an x86 interpreter and a guest-memory model; none of it transliterates |
| `DONT-MERGE/winmine-port` | The shim shape: one `.c` per DLL, `TRACE()` on every entry, `PANIC()` on unimplemented | Its method. It `dd`s sections out of a PE and links them at fixed x86-32 addresses, so it cannot reach arm64 or wasm32 |
| `DONT-MERGE/jphonorato` | The LP64 audit, the missing build generators, the host-tool separation, the case-sensitive include census, the font-metric oracle probes, and the Winelib heap-diagnosis playbook | Winelib and Qt as the strategy. Winelib reaches Linux only; Qt rewrites the shell instead of implementing Win32 beneath Word |
| `DONT-MERGE/free-api` | The header seam mechanism, the `WinMain` bridge, and the scope rule. MIT, C++20, SDL3, 163 commits. Proof the layering works | Its GDI, and the idea of vendoring it wholesale. See item 13 |
| `DONT-MERGE/bahree` | The Windows CI job | MSI packaging, for now |
| `DONT-MERGE/WineGlass-Native` | Recorded in `docs/win32-shim/reference-notes.md`: the ten stubs-that-succeed failures behind item 11's `PANIC()` rule, the tagged handle spaces, the clock and `WM_TIMER` findings for items 12 and 14, and a third GDI confirming item 13 is written from scratch | All of it as code. Its README says Apache 2.0 and it ships no LICENSE file, but the question does not arise: it is an x86 emulator, so PE loader, `HLT` thunks, guest memory and JIT have no counterpart here. Its one clean file, `wg_sync.c`, implements sync objects Word never calls |

free-api deserves detail rather than a summary, because it is the closest thing to what
the shim plan describes that already exists. It carries roughly 80 Win32 entry points plus 8
MSVC-CRT shims. Do not bother computing its overlap with our list as a percentage; the
shape of the gap is what decides, and its declared scope is two games
(`docs/scope.md`). It has no `LoadLibrary`, `GetProcAddress` or `FreeLibrary` at all, so
items 8 and 9 get nothing from it, and no `Global*` allocator, so neither does item 12.
It has no fonts, no text, no brushes, no pens, no regions, no `SaveDC`, no `PatBlt`, no `BitBlt`,
and `StretchBlt` accepts only `SRCCOPY` (`src/wingdi_blit.cpp:25`). Its own `windows.h`
lists "real GDI drawing, palettes, DIB sections, brushes, icons, cursors, fonts" as
unsupported. That is precisely the half of gdi32 a word processor is made of.

---

## Configure off Windows

The non-Windows configure and SDL probe gates are complete. Item 5 is next.

### 5. CI on three platforms

`.github/workflows/ci.yml` exists and omits the dead release metadata, signing, WiX, MSI
and artifact upload steps from `DONT-MERGE/bahree/.github/workflows/windows-release.yml`.
The Windows job keeps the useful Checkout, Configure, Build and Test shape.

macOS and Linux jobs install SDL2, configure their Ninja presets, and build the current
non-Windows gates: `opus-sdl-probe`, the native generator tools, and the generated
command/resource targets. The 10 tests that run `opus_word1_ui_test` are labelled `ui`;
CI excludes that label until item 14 gives them a headless path.

Do: open a pull request and confirm the Windows, macOS and Linux jobs are green.

Status: PR #12 is open at `https://github.com/jmarshall23/msword/pull/12` from
`jserv:dev`. Fork CI is green for Windows, macOS and Linux, but the upstream PR check
rollup is empty because the upstream default branch has no Actions workflow yet.

Done when: the three jobs are green on a pull request.

---

## Cut the seam

Execution order in this phase is 7, then 6, then 8, 9, 10, 11. The numbering is
kept stable so citations elsewhere do not rot.

### 6. Swap `opus_windows_sdk.h`, and close the seven leaks

Prerequisite: item 7's `WCHAR` rule. It is cheap and this header cannot be written
without it, so do item 7 first even though it is numbered second.

`src/port/original/opus_windows_sdk.h.in` is two lines, `#pragma once` and
`#include "@OPUS_WINDOWS_SDK_HEADER@"`, and `src/port/original/windows.h` includes it.
On MSVC, generate the SDK include as today; otherwise generate an include of
`src/port/win32/windows.h`. One generated header then feeds all 207 engine translation
units plus the port.

The route is worth stating correctly, because an earlier draft of this item described a
mechanism that does not exist. Nothing in `src/Opus` includes `"windows.h"`:
`grep -rl '#include *"windows.h"' src/Opus` returns zero files. The SDK arrives instead
through `opus_x64_compat.h:15`, which is `#include <windows.h>` with angle brackets,
reached from `src/Opus/wordtech/word.h:39` (the `#ifdef OPUS_X64` branch that replaces
`qwindows.h` and `sbmgr.h`) and directly from the seven `src/Opus/interp/*.c` files. It
still lands on `src/port/original/windows.h` because `OPUS_ORIGINAL_INCLUDE_DIRS`
(`src/CMakeLists.txt:671-679`) puts `port/original` at `:673` ahead of `Opus` at `:678`
and `-I` applies to angle includes on every compiler in play. The conclusion holds; the
route to it is `opus_x64_compat.h`, not a quoted include.

That covers `windows.h` and nothing else. Seven SDK headers bypass it:

| Header | Where |
|---|---|
| `<malloc.h>` | `opus_x64_compat.h:19`, which nearly everything in the port includes |
| `<direct.h>` | `opus_asm_file2.cpp:3` |
| `<DbgHelp.h>`, `<rtcapi.h>` | `opus_original_startup_probe.cpp:2,3` |
| `<shellapi.h>` | `opus_win16_platform.cpp:6` |
| `<commdlg.h>` | `opus_sdm_runtime.cpp:3` |
| `<windowsx.h>` | `opus_win95_chrome.cpp:3` |

`src/port/win32/` must also provide `windowsx.h`, `shellapi.h`, `commdlg.h` and
`direct.h`; `<rtcapi.h>` and `<malloc.h>` get `#if defined(_MSC_VER)` guards in the two
files that use them. Of those, free-api supplies one usable reference, not three:
`direct.h` is 28 lines marked PARTIAL with three real declarations backed by
`src/crt_direct.cpp`, and it matches what `opus_asm_file2.cpp:3` needs. Its `commdlg.h`
(20 lines) and `windowsx.h` (29 lines) are both marked STUB and declare essentially
nothing, while we need `OPENFILENAME` plus the two file dialogs for
`opus_sdm_runtime.cpp:3` and the `HANDLE_MSG`/`GET_X_LPARAM` crackers for
`opus_win95_chrome.cpp:3`. Write those two from the call sites.

Case-sensitive filesystems are a second problem, and bigger than the SDK headers.
free-api's mechanism is a directory of two-line forwarders
(`include_non_windows/Windows.h` is `#pragma once` plus `#include <windows.h>`), added
to the include path only on non-Windows targets. Copy it, and size it correctly: the SDK
spellings are just `Windows.h` and `DbgHelp.h` from
`opus_original_startup_probe.cpp:1-2`, but `src/Opus` itself has nine quoted includes
whose target differs only by case, which break on Linux and Emscripten while macOS
silently hides them:

```
debug.h -> DEBUG.H      pic.h -> PIC.H          rareflag.h -> RAREFLAG.H
resource.h -> RESOURCE.H   rtf.h -> RTF.H       rtftbl.h -> RTFTBL.H
saveFast.h -> savefast.h   screen.h -> SCREEN.H spell.h -> SPELL.H
```

`RESOURCE.H` is the one item 9 needs for the ordinal table. Eleven forwarders total, no
source file touched.

Layout:

```
src/port/win32/
  windows.h     types, handles, constants, prototypes for the 333 entries
  windowsx.h shellapi.h commdlg.h direct.h    thin, only what is used
  case/        two-line forwarders for Windows.h, DbgHelp.h and the rest
  kernel32.c gdi32.c user32.c comdlg32.c
  backend_sdl.c   the only file that includes SDL
  trace.c         TRACE/PANIC and unimplemented-call accounting
```

Build the first pass mechanically:

- Generate the first declaration list from `docs/win32-shim/api-inventory.tsv`.
- Add only typedefs, structs, constants and macros needed by the current compile error.
- Rebuild `WORD1` after each pass.
- Stop this item when the next failure is a missing shim function body, not a missing
  type, macro or prototype.
- Do not add declarations without a cited call site.

Done when: `grep -rn '#include *<' src/port/original/ | grep -vE '<(c?std|string|vector|array|algorithm|limits|utility|mutex|atomic|type_traits|unordered_map|map|memory|iostream|cmath|cctype|cwchar|cstdarg|cstdio|cstdlib|cstddef|cstdint|cstring|optional|functional|new)'`
returns only `windows.h` lines.

### 7. Decide `WCHAR` width before item 6 lands

WORD1, the startup probe, the modern formats test and the UI test compile with
`UNICODE;_UNICODE` (`src/CMakeLists.txt:1001`, `:1023`, `:1088`, `:1110`).
`src/port/original/` still has live `L"..."` literals and `wchar_t` declarations, with
the densest surface in `opus_win95_chrome.cpp`, `opus_modern_formats.cpp`,
`opus_word1_ui_test.cpp` and `opus_original_startup_probe.cpp`. It also calls
`SendMessageW`, `PostMessageW` and `GetModuleHandleW`, plus `wWinMain` entry points at
`opus_product_entry.cpp:44` and `opus_original_startup_probe.cpp:399`. Windows `WCHAR`
is 16-bit; clang, gcc and Emscripten default `wchar_t` to 32-bit.
The startup self-test path sits in `opus_original_startup_probe.cpp`.

This is jphonorato's fifth obstacle, the one he scoped to "~5 files in `src/port/`".

Status: `src/port/win32/windows.h` defines `WCHAR`, `LPWSTR`, `LPCWSTR`, `PWSTR` and
`PCWSTR` as 16-bit shim types, and asserts `sizeof(WCHAR) == 2`. `opus_x64_compat.h`
defines `OPUSW("")` for C++ port code so non-Windows literals can use UTF-16 storage
instead of host `wchar_t`. `opus_original_startup_probe.cpp` uses that boundary for
`--self-test` detection before narrowing the command line with `WideCharToMultiByte`.
`opus_asm_resn2_sttb.cpp` also declares `GetModuleHandleW` with `LPCWSTR` for its trace
base-address lookup. `opus_original_startup_probe.cpp` also uses the same boundary for
the RTC failure callback's `W` parameters and diagnostic message buffer, with lossy
ASCII-only diagnostic narrowing before writing crash text. `opus_win95_chrome.cpp` uses
`OPUSW`/`WCHAR` for its toolbar class
names, pane-proc property name, and the `user32.dll`/`uxtheme.dll` dynamic-library
lookups.
Its repeated fixed class names, font faces, default combo text, empty window captions,
format-button glyph labels, text-color palette labels, language menu labels, toolbar
popup and submenu labels, table-menu labels, fixed submenu attachment labels, fixed
Window-menu labels, root/file/view menu rewrite labels, document context-menu labels,
the `dwmapi.dll` dynamic-library lookup and the zoom combo's fixed choice labels also
use the same boundary. Its dynamic zoom-percent text now uses `WCHAR` storage too.
Its menu lookup helpers now use `WCHAR` buffers, `std::basic_string<WCHAR>` and
`LPCWSTR` lookup keys instead of host `wchar_t` storage.
Its toolbar numbered-button glyph buffer and pending `WM_CHAR` surrogate storage now
also use `WCHAR`/`OPUSW` instead of host `wchar_t`/raw `L` literals.
Its ANSI combo mirror conversion helpers and mirrored combo text now use `WCHAR`
storage for values passed through `W` APIs.
Its ruler tick labels now use `WCHAR` storage before passing text to `TextOutW`.
Its IME result text buffer now uses `WCHAR` storage before parsing UTF-16 code units.
`opus_modern_formats.cpp` uses the same boundary for its `Msftedit.dll` RichEdit load
and no longer passes a host `wchar_t` empty title to the hidden RichEdit window.
Its PDF export dialog strings and writable default path buffer now also use
`OPUSW`/`WCHAR`, with a boundary copy back to the file's current `std::wstring`
path model.
Its hidden RichEdit text extraction now reads `GetWindowTextW` into `WCHAR`
storage before copying back to the file's current `std::wstring` text model.
Its `CF_UNICODETEXT` clipboard boundary now uses `WCHAR` code units instead of
host `wchar_t` sizing and storage.
Its `ExtTextOutW` bridge now copies generated UTF-16 code units into `WCHAR`
storage before drawing.
Its RichEdit face-name extraction now reads `CHARFORMAT2W::szFaceName` as
bounded `WCHAR` storage before updating the file's style model.
Its PDF font face-name probe now reads the `GetTextFaceW` output into bounded
`WCHAR` storage before copying back to the PDF font model.
Its PDF font creation now copies the requested face name into `WCHAR` storage
before calling `CreateFontW`.
Its PDF glyph lookup now copies the candidate UTF-16 code units into `WCHAR`
storage before calling `GetGlyphIndicesW`.
Its regular-file size check now copies the probed path into `WCHAR` storage
before calling `GetFileAttributesExW`.
Its atomic file read/write helpers now share that path copy for their
`CreateFileW`, `GetFileAttributesW`, `ReplaceFileW`, `MoveFileExW`, and
`DeleteFileW` boundaries.
Its OPC package read path now copies the source document path into `WCHAR`
storage before calling `IOpcFactory::CreateStreamOnFile`.
Its DOCX package write helpers now pass `WCHAR` part names, content types,
relationship targets, and relationship types into the OPC COM write APIs.
Its DOCX package read helper now passes `WCHAR` part names into the OPC COM
part lookup API.
`opus_sdm_runtime.cpp` now uses a `WCHAR` buffer for the refresh-font text read through
`GetWindowTextW` before converting it back to the dialog's ANSI state.

Do: do not use `-fshort-wchar`. Convert owned live port literals through `OPUSW("")`
or explicit UTF-16 buffers; do not touch `src/Opus` for this. The live files to audit
before claiming this done are `opus_original_startup_probe.cpp`, `opus_win95_chrome.cpp`,
`opus_sdm_runtime.cpp`, `opus_modern_formats.cpp`, `opus_asm_resn2_sttb.cpp` and
`opus_asm_movecmds.c`. `opus_word1_ui_test.cpp` is test-only but must follow the same
rule before non-Windows UI tests run.

The entry point is the other half of this, and there is only one live site, not two.
`opus_product_entry.cpp` appears nowhere in `src/CMakeLists.txt`; it is in no target and
is never compiled, so treat it as dead until a CMake target references it. Its
`wWinMain` at `:44` and its share of the wide literals do not count. The live bridge is
`opus_original_startup_probe.cpp:399`, and free-api has a working pattern to copy:
`FREE_API_IMPLEMENT_WINMAIN` plus
`FreeApiRunWinMain(entryPoint, argc, argv)` at `free-api/src/winmain_bridge.cpp:50`, with
`main` at `:504` converting `argv` into a command line. Ours additionally has to build a
wide command line, which is item 7's decision again.

Done when: a non-Windows compile check proves `sizeof(WCHAR) == 2`, no live
non-Windows port code passes raw `L""` or `wchar_t*` values to `W` APIs, `WORD1
--self-test` reaches `wWinMain` on macOS, and any remaining failure is a missing shim
header, declaration, or function body from item 6 rather than a wide-character boundary.

### 8. Replace `GetProcAddress` on our own module with a generated table

`src/port/original/opus_asm_movecmds.c:172` resolves 427 Word commands by name against
the executable's own export table. Every platform needs a different trick: jphonorato
spent a phase generating a winebuild `.spec`, ELF needs `-rdynamic`, Mach-O needs
`-Wl,-export_dynamic`, Emscripten needs `EXPORTED_FUNCTIONS`.

The catch, and the reason this item previously was not executable: `mkcmd.c:1415` emits
only `#pragma comment(linker, "/export:%s")` lines plus a name-string table into
`opuscmd_native.inc`. There are no C declarations for the 427 symbols, so the table
cannot simply be written by hand, and rule 1 puts `mkcmd.c` off limits.

Do: add a CMake script step that reads the generated `opuscmd_native.inc`, emits
`extern int NAME();` and `{ "NAME", (OPUS_PFN)NAME }` into a second generated header,
and make `ResolveCommandAddress` bsearch it. `OpusEtAl` is not touched.

Done when: `nm` on the WORD1 binary shows zero dynamic exports and
`opus_original_command_test` still passes.

### 9. Module resolution for everything item 8 does not cover

Item 8 handles our own module. Word does dynamic loading in two other shapes:

Win16 ordinal lookup into system DLLs, 11 sites, all
`GetProcAddress(module, MAKEINTRESOURCE(ordinal))`: `eldde.c:1286`, `CLIPBRD2.C:607,
1200`, `initwin.c:1158, 1704, 1724, 2231`, `quit.c:353`, `help.c:991`, `elfile.c:331`,
`dlgmisc.c:2334`.

Do not enumerate the ordinals from the call sites; the table is already in the tree at
`src/Opus/RESOURCE.H:195-218`, under the comment "GetProcAddress ordinal values (taken
from appropriate .DEF files)". It gives `idoVkKeyScan` 129, `idoSetSpeed` 7,
`idoAllocSelector` 175, `idoFreeSelector` 176, `idoPrestoChangoSelector` 177,
`idoGetFreeSpace` 169, `idoAllocDsToCsAlias` 171, `idoExtTextOut` 14, `idoDeviceMode` 13,
`idoExitWindowsV3` 7, `idoSetDIBits` 440, `idoGetDIBits` 441, `idoCreateDIBitmap` 442.

Three of the eleven sites are dead under `OPUS_X64` and should not drive the design:
`initwin.c:1704` and `:1724` sit in the `#else`/`#ifndef` of an `OPUS_X64` guard, as does
`help.c:991`. `KEYBOARD` still needs a sentinel, but on the strength of `dlgmisc.c:2334`
alone. One site also evades an `ido` grep: `initwin.c:2231` writes the literal
`MAKEINTRESOURCE(442)` with a comment noting 442 is `CreateDIBitmap` under Windows 3.

`GetModuleHandleA(NULL)` must return our own image handle, not a system sentinel; items
8 and 10 both depend on that. Note also that `GetProcAddress` is `#define`d to
`OpusGetProcAddress` at `opus_x64_compat.h:30`, so the shim must sit behind that wrapper
rather than replacing it.

Real external DLLs: graphics filters through `HOurLoadLibrary` (`GRSPEC.C:1725`, called
at `:1417` and `:1539`), the ET and spell libraries by ordinal (`etcmd.c:349` then
`:362-374`, `SPELL.C:1144` then `:1156-1188`), file converters by name string
(`filecvt.c:459-498`), printer drivers (`print2.c:1440-1483`).

Do: (i) `GetModuleHandleA` returns sentinel handles for `KERNEL`, `USER`, `GDI`,
`KEYBOARD`, and `GetProcAddress` with a `MAKEINTRESOURCE` low word maps ordinals to shim
functions through a static table.
(ii) `LoadLibrary` of any other name returns 0, which is below 32 and is the failure
path the external call sites already test for. Read `GRSPEC.C:1417`, `etcmd.c:349` and
`print2.c:1440` and confirm each degrades rather than crashes.

Produce `docs/win32-shim/module-ordinal-map.tsv` with columns `dll`, `ordinal`,
`function`, `source_file`, `source_line`, `live_under_OPUS_X64`. Seed it from
`src/Opus/RESOURCE.H:195-218`, confirm each `MAKEINTRESOURCE` call site against the
table, and implement only rows marked `live_under_OPUS_X64=yes`.

Done when: a TRACE-only run of WORD1 shows every `LoadLibrary` returning 0 with no
PANIC, and the spell and graphics-filter paths report unavailable rather than faulting.

### 10. Replace PE resources

Item 1 stops compiling `word1.rc`, but its contents are not cosmetic.
`opus_win95_chrome.cpp:43` defines `kToolbarBitmap = 201` and `:2565` calls `LoadImageW`
to fetch it at runtime. `word1.rc` also carries icons 301 and 302 and the manifest.

Do: emit `src/port/assets/word95-toolbar.bmp` and the two `.ico` files as generated C
byte arrays, reusing `opus_dibapp_tool` (`src/CMakeLists.txt:265`), and implement
`LoadImageW`, `LoadBitmap` and `LoadIcon` in the shim over that array. Version info and
the manifest are Windows-only and get dropped off Windows.

Done when: the toolbar renders on macOS with no `.rc` in the build.

### 11. Coverage gate over the inventory

Make `src/port/win32/windows.h` the single source of truth: the check scans for every
identifier declared there that appears followed by `(` anywhere in `src/Opus` or
`src/port`, and diffs against what the shim defines. That makes the check
self-consistent, unlike a scan against an external name list.

The scanner must expand the header's own `#define NAME NAMEA` aliases before scanning,
or it reproduces the exact bug that made the Scope section a lower bound: searching for
`SendMessageA(` finds zero hits in `src/Opus` and the gate reports full coverage of a
function with 343 live callers. Collect both the declared names and the alias mappings,
and grep for both spellings. Better still, once the shim links, make the real check
`nm` on the archive against the undefined symbols in the engine, which is the only
authority that cannot be fooled by a macro.

Cross-reference `docs/win32-shim/api-inventory.tsv` for priority order and for
Win32Emu's implemented/stub/none column as a second opinion on how much behavior each
entry point needs.

Adopt free-api's scope rule verbatim (`free-api/docs/scope.md`): every public entry
point must cite a real usage site as `file:line`, and anything without one does not
belong, "no matter how common or obviously useful it seems". That is rule 4 of this
document with an enforcement mechanism, and `api-inventory.tsv` is already the citation
table it requires. free-api's exceptions are worth copying too: compile-only stubs that
fail safely, and test-only symbols, both documented as such rather than silently
present.

Pair with winmine-port's discipline: every shim entry logs under `TRACE()`, anything
unimplemented calls `PANIC()` with its own name. Running the binary then gives the real
call order, which beats guessing from static analysis.

Done when: `ctest -R win32_coverage` fails with a named list when an entry point is
called but not defined.

---

## Shim dependency order

Execution order here is 15a, 12, 13, 14, 15b. Item 15 splits: the decision and the
deletion of the native control classes come first because they change how much of items
13 and 14 has to exist, and SDM's own control drawing comes last because it needs both.
Numbering stays stable so citations do not rot.

### 12. kernel32 tier, 84 entries, no SDL

Memory (`GlobalAlloc`/`Lock`/`Unlock`/`ReAlloc`/`Free`/`Handle`/`Size`, the `Heap*`
family, `LocalReAlloc`), files (`CreateFileA`, `OpenFile`, `WriteFile`,
`FindFirstFileA`/`FindNextFileA`/`FindClose`, `GetFileAttributesA`, `GetFullPathNameA`,
`GetTempPathA`, `MoveFileA`, `CopyFileA`, `DeleteFileA`, `CreateDirectoryA`,
`RemoveDirectoryA`), time, and the error stubs.

Path rule: map `C:\foo\bar` to `${OPUS_C_DRIVE}/foo/bar`, defaulting `OPUS_C_DRIVE` to
the repo root. Convert `\` to `/`. On `ENOENT`, retry each path component with a
case-insensitive directory scan. Reject other drive letters with `ERROR_PATH_NOT_FOUND`.
macOS hides case mistakes; Linux and CI will not.

The `Global*` family is the real work, not the file APIs: `GlobalUnlock` 99 sites,
`GlobalFree` 70, `GlobalLock` 51 in `src/Opus` alone, and it must be written from
scratch. An earlier draft claimed `opus_x64_heap.cpp` "already implements the
double-indirect handle model, so it moves into the shim largely unchanged". It does not.
That file implements Word's own `H*` family (`OpusHAllocateCb`, `OpusDerefH`,
`OpusFChngSizeHCb`, declared `opus_x64_heap.h:9-22`) on top of `HeapAlloc`. It is a
client of the shim's `Heap*`, not a provider of `Global*`, it contains no `Global*` code,
and moving it inverts the dependency. Leave it where it is.

Two Win16 behaviors the naive port gets wrong, both load-bearing here:

- `GlobalLock` and `GlobalUnlock` are not balanced in this codebase, 51 against 99.
  Win16 `GlobalUnlock` on a fixed or already-unlocked block is a no-op returning FALSE,
  not an error. The lock count saturates at zero, never goes negative, and never frees.
- `GlobalHandle` returned a packed DWORD with the handle in the high word, and
  `ripaux.c:52` still does `HIWORD(GlobalHandle(ps))`. Return a bare pointer and that
  site silently reads zero.

Four entry points with live call sites that the earlier draft omitted: `GlobalCompact`
(`elfile.c:196, 201`, `help.c:987, 996`, `wproc.c:2716`, where the result is divided, so
returning 0 changes behavior), `GlobalWire` (`initwin.c:760`), `GlobalHandle`
(`ripaux.c:52`), `GlobalSize`. Also state the clipboard rule: a handle passed to
`SetClipboardData`, 14 sites in `src/Opus`, transfers ownership, and freeing it as well
double-frees on `EmptyClipboard`.

jphonorato's Qt memory contract is useful even though the Qt boundary is not. Keep a
registry from locked pointer back to owning handle, because `GlobalHandle(ptr)` is live,
and leave a freed marker long enough for tests to catch lock-after-free and double-free.
Do not rewrite Word's own `HQ`/`HqAllocLcb` family into this layer; it already routes
through `opus_x64_heap.h` and is a client of `Heap*`, not a Win16 `Global*` call.

Done when: `ctest -R 'strtbl|sttb|plc|sdm_cab|command' --output-on-failure` is green on
macOS. Note those are the only five of the seven non-UI tests reachable here:
`opus_x64_runtime_test` links `gdi32` explicitly at `src/CMakeLists.txt:933` and belongs
to item 13, and `word1_port_smoke_test` runs `WORD1 --self-test` (`:975`), which links
the whole engine and belongs at the end of item 14. Add one assert-style memory test for
`GlobalAlloc` -> `GlobalLock` -> write -> `GlobalHandle(ptr)` -> `GlobalUnlock` ->
`GlobalFree`, plus a death/check-failure case for double-free.

### 13. gdi32 tier, 72 entries

The DC model is the design work: `CreateCompatibleDC`, `SelectObject` (167 sites in
`src/Opus`, the most-called Win32 function in the tree), `SaveDC`/`RestoreDC`, clipping,
and the raster ops (`PatBlt` at 96 sites, `BitBlt`, `StretchBlt`, `SetROP2`).

Start from the state models and the four-step raster shape in
`docs/win32-shim/reference-notes.md`. They came from an implementation that ran real
programs, and two fields there, `owns_selected_bitmap` and `is_info_context`, encode
`BeginPaint`/`EndPaint` ownership and the `CreateIC` restriction that otherwise cost a
day each.

Do not expect to borrow a GDI from a game-oriented shim. free-api is the cautionary
case and the comparison is in `reference-notes.md`: its `CompatDC`
(`src/internal/FreeApiGdi.hpp:27`) holds a selected bitmap and surface geometry and
nothing else, no pen, brush, font, clip, background mode, text color or ROP. Games blit
sprites; Word draws text into a stateful DC. The two need different objects, and the
missing half is the half we are paying for.

Back a DC with a plain 32-bit pixel buffer and do the raster work in C. Do not map
`SetROP2` and `PatBlt` onto SDL's renderer; the ROP semantics do not survive it, and
Word uses `DSTINVERT` and `PATINVERT` for caret and selection, where being wrong is
immediately visible. SDL only puts the finished buffer on screen. That makes a
software-only build free, which CI and WASM both need.

Fonts are the second-largest cost after item 15. The API surface is Win16, not the
modern one. Call sites in `src/Opus`, per the inventory: `GetTextExtent` 26,
`GetTextMetrics` 11, `CreateFontIndirect` 7, `EnumFonts` 7. `GetTextExtentPoint32A`
appears once and `EnumFontFamiliesExA` not at all; both come from port code.
Implementing the modern pair alone would miss the entire engine-side surface. Layout is exact, so
wrong metrics means wrong line breaks means a document that does not match.

Use jphonorato's font probes as tests, not his Qt implementation. The important result
is that font widths have to be captured as integer character advances from the same
kind of rasterizer path the oracle uses; arithmetic from design units is not enough.
The OPUS_X64 build's live `FTI` is `src/Opus/fontwin.h`, not the `#ifdef MAC` structure
in `wordtech/format.h`, so do not design the cache fill path from the dead Mac layout.
The four startup font names also need an explicit substitution table before pagination
can be trusted: `Tms Rmn`, `Symbol`, `Helv`, `Courier`.

Printing (`Escape` at 26 sites, `StartDoc`/`EndDoc`/`EndPage`) is stubbed to failure.

Split item 13 into executable subtasks:

- 13a DC/object model: `HDC`, `HBITMAP`, `HPEN`, `HBRUSH`, `HFONT`, `SelectObject`,
  `SaveDC`, `RestoreDC`.
- 13b Raster ops: `PatBlt`, `BitBlt`, `StretchBlt`, `SetROP2` over 32-bit buffers.
- 13c Text metrics: `CreateFontIndirect`, `EnumFonts`, `GetTextExtent`,
  `GetTextMetrics`, plus the font-substitution table.
- 13d Printing stubs: `Escape`, `StartDoc`, `EndDoc`, `EndPage` fail predictably with
  `TRACE`.

Each subtask gets one assert or golden test before moving to the next.

Done when: `opus_x64_runtime_test` passes on macOS, and a golden-file test runs
`GetTextExtent` over printable ASCII at 8, 10, 12, 14, 18, 24 and 36 pt for the startup
font names and diffs against output captured from the Windows build.

### 14. user32 tier, 173 entries

Windowing and the message pump, input translation from SDL events to
`WM_KEYDOWN`/`WM_CHAR`/`WM_?BUTTON*` with correct `wParam`/`lParam` packing, menus,
caret, clipboard, scrollbars.

The message loop is not ours to invert, and an earlier draft of this item decided
otherwise on a false premise. It said "invert the loop into a step function, one path,
no Asyncify". Grepping `src/Opus` for `GetMessage`, `PeekMessage` and `WaitMessage`
gives 39 calls across 17 files, 17 of them blocking (`GetMessage` or `WaitMessage`),
nested arbitrarily deep in the call stack:
`wproc.c:2489, 2497, 2583, 2591, 2608, 2644` inside `FIsKeyMessage`, `idle.c:1072, 1086,
1261`, `prompt.c:1748`, `help.c:1342`, `raremsg.c:639`, `iconbar3.c:1092`,
`eldde.c:781`, plus the main pump in `OpusOriginalWinMain` at `wproc.c:527-563`. Rule 1
makes all of that read-only. You cannot return to `emscripten_set_main_loop` from inside
`FIsKeyMessage` without unwinding the C stack, so the inversion is not merely expensive,
it is unavailable.

For WebAssembly, choose Asyncify. Add `docs/win32-shim/wasm-pump.md` listing every shim
function allowed to yield and the matching `ASYNCIFY_IMPORTS` entry. Do not implement a
pthread pump unless Asyncify fails a measured size or speed gate. JSPI and dropping WASM
are not item 14 work.

What the shim owns is its own queue, not Word's loop, and it must preserve free-api's
invariant: `PeekMessage` never sleeps, only `GetMessage` and `WaitMessage` may
(`free-api/src/winuser_message.cpp:48-53` and `:122-125`, with an Android branch added
alongside the `SDL_Delay(1)` at `:168-170`).

```c
/* src/port/win32/user32.c */
BOOL WINAPI PeekMessageA(LPMSG m, HWND h, UINT lo, UINT hi, UINT flags) {
    Opus_PumpOnce();                  /* drain SDL into the queue; never sleeps */
    return Opus_QueueTake(m, h, lo, hi, flags);
}
BOOL WINAPI GetMessageA(LPMSG m, HWND h, UINT lo, UINT hi) {
    for (;;) {
        if (Opus_QueueTake(m, h, lo, hi, PM_REMOVE))
            return m->message != WM_QUIT;
        Opus_PumpBlockUntilMessage(); /* native: SDL_WaitEvent.
                                         wasm+Asyncify: emscripten_sleep(0).
                                         wasm+pthread: cond_wait on the queue. */
    }
}
BOOL WINAPI WaitMessage(void) { Opus_PumpBlockUntilMessage(); return TRUE; }
```

The one pump we do own is `opus_sdm_runtime.cpp:2567-2578`, and item 15 deletes it.

`GetWindowWord`/`SetWindowWord` are 90 sites combined in `src/Opus` (68 and 22). They
are the Win16 per-window extra-bytes mechanism and a 16-bit slot cannot hold what Word
stores there on a 64-bit host. Audit the call sites before assuming a straight port;
this is the same class of defect as item 19, a pointer squeezed through a slot too
narrow to hold it.

Headless mode belongs here, not deferred forever: `backend_sdl.c` honours
`SDL_VIDEODRIVER=dummy` under `OPUS_HEADLESS=1`, and the input translator replays a
scripted event list instead of SDL events. The 8 UI tests currently drive WORD1
out of process with `SendInput` and `GetGUIThreadInfo`
(`opus_word1_ui_test.cpp:391, 297`), which has no meaning under a self-contained shim;
they become in-process scripted-event tests against the same harness.

Message ordering is where ports like this actually fail. Word assumes Windows 2/3
delivery order around focus, capture and paint. Expect more time there than on any
individual entry point.

Done when: `word1_port_smoke_test` passes on macOS and a headless run opens a window and
dispatches a keystroke. `ctest -L ui` cannot be this item's check: those tests drive the
About and Save As dialogs (`src/CMakeLists.txt:1000-1007`), which are SDM dialogs, so
they belong to item 15b.

### 15. The controls decision, which is the biggest single item

`opus_sdm_runtime.cpp` builds dialogs from real Win32 control classes through
`create_native_control()` and `create_untracked_control()` helpers starting at `:814`:
44 `"BUTTON"`, 17 `"COMBOBOX"`, 14 `"STATIC"`, 6 `"LISTBOX"`, 5 `"EDIT"`, plus
`GetOpenFileNameA`/`GetSaveFileNameA` at `:2184`. Shimming those means five control
classes with their full `BM_*`, `EM_*`, `LB_*` and `CB_*` protocols, focus and keyboard
navigation, and a common file dialog: more work than gdi32 and user32 combined.

It is also self-inflicted. Word 1.1a shipped SDM, which drew its own controls on Win16
primitives. Native controls are a choice this port made, in code we own.

Split this in two, because as one item it deadlocks against item 14: 14's checks need
dialogs, and 15's check needs 13's device context and 14's input routing.

15a, before item 12: make SDM draw its own controls again in `src/port/`. Remove
`create_native_control`, `create_untracked_control`, built-in class-name
`CreateWindowExA` calls, and `GetOpenFileNameA`/`GetSaveFileNameA` usage from
`opus_sdm_runtime.cpp`. Replace each with an SDM-owned object record; no rendering yet.
This deletes the largest chunk of the shim before it is written and the dialogs end up
looking like Word 1.1a rather than like host widgets.

15b, after item 14: SDM's own control drawing, over item 13's device context.

Exclude `opus_win95_chrome.cpp` from non-Windows targets until the SDM path can redraw
that chrome. It is 2882 lines of Win32 chrome with `<windowsx.h>` and `uxtheme.dll`, and
has no cross-platform meaning as written.

The two comdlg32 entries become a small SDM-drawn file browser over item 12's file APIs.

Done when: 15a, zero `CreateWindowExA` calls with a built-in class name remain in
`opus_sdm_runtime.cpp`. 15b, a new `opus_sdm_render_test` renders the About and Save As
dialogs to a pixel buffer and diffs against a checked-in reference image, and
`ctest -L ui` is green headless on the Linux runner.

---

## Targets, in order

### 16. macOS

arm64 and x86_64. `char` is signed by default on clang, and the engine builds with `/J`
under MSVC precisely to keep the original compiler's unsigned default
(`src/CMakeLists.txt:690-692`, whose comment says not to change it). The clang
equivalent is `-funsigned-char` and it is not optional, for the engine and for the five
host tools in item 3.

Done when: WORD1 launches, shows a window, and accepts typing.

### 17. Linux

jphonorato's ground truth: `ninja -k 0` reaches 0 errors across all 207 engine
translation units under winegcc (`00-reconocimiento.md:2181, 2271, 2416`), after
triaging 182 errors into 5 families at `:1451-1620`. One review reported this as
"154/207, still failing"; that is the opening reconnaissance figure at `:19`, not the
closing one. Reuse the triage, and expect the remaining families to be pointer-width
and LP64 work.

What Linux adds over macOS is a case-sensitive filesystem. jphonorato counted 12 headers
across 202 files in his tree; measured in this one it is 11 forwarders, the nine
`src/Opus` quoted includes plus `Windows.h` and `DbgHelp.h`, all of which item 6 already
covers. LP64 is not on this list; macOS is LP64 too and item 3 already forced that.

Do not adopt winegcc. Under the shim there is no Wine, just an ordinary clang or gcc
build of the same sources macOS builds.

Done when: the macOS test set passes on Linux, including `ctest -L ui` headless.

### 18. WebAssembly

wasm32 has 4-byte pointers. Item 4 removes the configure assertion; the work is finding
every 8-byte assumption: `opus_x64_heap.cpp`'s handle encoding, `opus_x64_layout.c`, and
any `#pragma pack` struct with a pointer member (`Opus/cmdtbl.h:65`,
`opus_asm_movecmds.c:12`, with the size assertions at `opus_asm_movecmds.c:164-170` that
will fire). Configure a wasm32 build early, even if it does not link, to collect the
list.

The blocking-loop problem is item 14's, and Asyncify is the chosen route for this item.
Files come from MEMFS or IDBFS behind item 12's file APIs. SDL2 is the default from item
5a because `-sUSE_SDL=2` is a first-class Emscripten port; SDL3 is only revisited if the
probe in `docs/win32-shim/sdl.md` proves it across macOS, Linux and Emscripten before
item 13 writes `backend_sdl.c`.

Done when: WORD1 loads in a browser and renders its main window.

---

## Pointer-width and LP64 correctness

Real defects in the current x64 build. All line numbers below were re-derived against
this tree; an earlier draft carried jphonorato's post-patch numbers, which are offset by
roughly 18 lines in `exp.c` and 29 in `CLIPBRD2.C`.

### 19. WordBasic `Declare ... As String` truncates the pointer to 32 bits

`src/Opus/interp/exp.c:1530-1531`:

```c
*pwArgs++ = HIWORD(lpstr);
*pwArgs++ = LOWORD(lpstr);
```

Both macros take 16-bit halves, so a heap pointer loses its top 32 bits before
`LPushMacroArgs` sees it. Note where they come from, because an earlier draft said
`qwindows.h:143-144` and item 22 has since established that file is not compiled:
`exp.c:3-7` takes `opus_x64_compat.h` under `OPUS_X64` and `<qwindows.h>` only in the
`#else`, and `opus_x64_compat.h` defines `LOWORDX`/`HIWORDX` at `:239, 242` but not
`LOWORD`/`HIWORD`. So these resolve to the Windows SDK's `windef.h` definitions, which
are 16-bit halves of a `DWORD_PTR`. The defect is identical either way; only the file to
open changes.

jphonorato's fix: pack `uintptr_t` as lo32/hi32 into two `int` slots and add
`LPushMacroArgsTyped(proc, args, cwArgs, rgdkt, idktMacParam)` in
`opus_asm_misc.cpp` to reassemble it. Adopt unguarded. On wasm32 the hi32 slot is always
zero, which is correct and costs nothing.

Done when: a test declares a macro function taking a string, calls it, and asserts the
callee sees the full pointer.

### 20. `dktDouble` arguments are passed in integer registers

Same path. `exp.c` writes an 8-byte `NUM` into `int rgwArgs[]` and `invoke_macro` passes
one `int` per slot, so a `double` parameter arrives as two integer arguments where the
ABI wants one XMM register. Fix once item 19's per-argument DKT table exists.

Done when: the same test covers a `Declare ... As Double`.

### 21. `CLIPBRD2.C:783` walks the CHR array by the wrong stride

```c
(char *)pchr += pchr->chrm;
```

Every other walk uses `CbFromChrm(chrm)`: `disp1.c:1722`, `format.c:3013`,
`select.c:960`. `chrm` is a discriminator tag on this port, not a byte count. They
cannot all be right.

Done when: the site matches the other three, or a comment explains why it must not.

### 22. Bare `long` in serialized structures

An earlier draft headlined this as "`qwindows.h:119` is the construct that already
corrupted our tooling". That is false and the item should not send anyone to that file:
`qwindows.h` is not compiled in this build. Every include of it sits in the `#else` of
an `#ifdef OPUS_X64` (`wordtech/word.h:39-42` and the seven `interp/*.c`), and nothing
includes `Opus/windows.h`, its only other route. Its `typedef unsigned long DWORD` is
dead text; `DWORD` in this build comes from the SDK and is correctly 32-bit. Rule 1 puts
the file off limits anyway.

The real exposure is bare `long` and `LONG` in structures that reach `fread`/`fwrite`.
Audit `wordtech/plc.c`, `savefast.c`, `file.h`. Add the one new instance this plan
creates: the shim's own `LONG` must be `int32_t`, not `long`, or every `RECT`, `POINT`
and `MAKELONG` changes width on LP64.

Produce `docs/lp64-audit.tsv` with columns `file`, `line`, `type`,
`serialized_or_runtime`, `windows_size`, `host_size`, `fix`. Mark only on-disk or
wire-format rows as `serialized`; ordinary runtime-only uses do not need serialization
tests.

Done when: a static-assertion test checks the `sizeof` of every serialized structure
listed in `docs/lp64-audit.tsv` against the Win16 values and passes on all four targets.

### 23. `CP` is `long` and PLC tables are serialized

8 bytes under LP64, 4 under LLP64 and wasm32. Keep the on-disk format at a 32-bit
ceiling and enforce it with fixed-width fields rather than `sizeof(CP)`. Apply the same
on-disk-width rule to `KME` (`wordwin.h:452`): do not widen `int w`, the first member of
the union at `:459`, because that changes `sizeof(KME)`, and `openrare.c:782` reads
keymaps as `iMac * cwKME * 2`.

Add `CP`, PLC rows and `KME` to `docs/lp64-audit.tsv` before editing code, then make the
test from item 22 fail on the current layout.

Done when: a document saved on Windows opens byte-identically on macOS and Linux, and
back.

### 24. `OpusX64TraceRibbon` has no default definition

`Opus/rulrib.c:1657` declares `extern void OpusX64TraceRibbon();` with no prototype and
calls it with 8 arguments at `:1693` and `:1713`. The only definition is
`opus_original_startup_probe.cpp:377`, which is WORD1-only, so any other target linking
the engine fails. bahree pasted a no-op stub into `opus_x64_runtime_test.cpp`; the next
test target needs the same paste.

An earlier draft of this item got the library wrong and would not have fixed the link.
The callers that break the test are in `opus_sdm_runtime.cpp:1779, 1794, 1955, 1981,
1998`, which is compiled into `opus_x64_runtime` (`src/CMakeLists.txt:723`), and
`opus_x64_runtime_test` links `opus_x64_runtime user32 gdi32` (`:933`), not
`opus_original_engine`. A stub in the engine resolves nothing.

No header is needed either. `rulrib.c:1657` already declares the function at block
scope, and every parameter type survives default argument promotion, so that
non-prototype declaration is compatible with the real one and the calls at `:1693,
1713` already pass the right eight arguments. `opus_sdm_runtime.cpp:24` declares it
inside an `extern "C"` block. Adding an include to `rulrib.c` would be a `src/Opus` edit
for no gain.

Do: add one file to `opus_x64_runtime`, containing nothing else.

```c
/* src/port/original/opus_trace_stub.c
 * Fallback definition of OpusX64TraceRibbon. This file must contain NOTHING
 * else: a static-archive member is pulled in only to resolve an undefined
 * symbol, so WORD1, which defines the real one in
 * opus_original_startup_probe.cpp:377, never pulls this member and never sees
 * a duplicate. Any other symbol here would force it in and break that link.
 * Signature matches opus_original_startup_probe.cpp:377-380 exactly. */
void OpusX64TraceRibbon(const char *stage, int message, int tmc,
                        int first_value, int second_value,
                        long cp_first, long cp_limit, int insertion)
	{
	(void)stage; (void)message; (void)tmc; (void)first_value;
	(void)second_value; (void)cp_first; (void)cp_limit; (void)insertion;
	}
```

and one line in the `add_library(opus_x64_runtime STATIC ...)` list near
`src/CMakeLists.txt:705-724`:

```cmake
    port/original/opus_trace_stub.c
```

Done when: `opus_x64_runtime_test` links with no stub pasted into its own source, and
`nm` on WORD1 shows exactly one `OpusX64TraceRibbon`, the one from
`opus_original_startup_probe.o`.

### 25. Write the toolchain-agnostic fixes once

jphonorato guards most of his `src/Opus/` changes with
`#if defined(__GNUC__) && !defined(_MSC_VER)` and keeps the old text in the `#else`. For
the genuinely divergent cases, FARPROC prototypes in `CLIPBRD2.C`, `GRSPEC.C` and
`eldde.c`, and the packing in `exp.c`, that is right. For the rest it is twice the code
for nothing.

Cite these as classes, not single lines, since they recur:

- Cast-as-lvalue, 20 sites. `grep -rnE '^[[:space:]]*\([a-zA-Z_][^)]*\*\)[[:space:]]*[a-zA-Z_]' src/Opus | grep -E '(\+=|-=|=[^=])'`
  returns 24 lines; four of them (`disp1.c:1676`, `elfile.c:1469`, `formula.c:503`,
  `elsubs.c:354`) are ordinary casts inside expressions, not assignments. The 20 real
  ones: `disp1.c:798, 799, 852, 904, 959, 974, 1045, 1046, 1072, 1073, 1249, 1250,
  1722`, `CLIPBRD2.C:783`, `format.c:3013`, `elsubs2.c:290`, `spelcore.c:157`,
  `inssubs.c:1684, 1687`, and `tabs.c:213`, which is a cast-as-lvalue through
  `#define vptdsd ((TDSD *) pcmb->pv)` at `tabs.c:131`. `(char *)p += n` becoming
  `p = (T *)((char *)p + n)` changes nothing for MSVC and sheds a deprecated extension no
  other compiler accepts.
- Pointer subtraction between incompatible types: `elcore.c`, `elxprocs.c:89`,
  `inssubs.c:652`, `pagevw.c:1380`.
- K&R definitions that conflict with a prototype: `help.h` `FDlgAbout`, `mathapi.c`
  `LWholeFromNum`, and the SDM parser declarations. Locate by compiling, not by line
  number; the four cited in an earlier draft were jphonorato's post-patch positions.

While doing this, fix the formatting the guards introduced in jphonorato's tree before
adopting: a typedef block landed inside a function body at `eldde.c:1279`, and
`GRSPEC.C:1432` lost the indentation of an `if`.

Done when: the non-MSVC build has no `__GNUC__` guard in `src/Opus/` outside the four
genuinely divergent files, and the Windows build is byte-identical.

---

## Hygiene

### 26. No `.gitignore`

`bin/`, `out/`, `build/`, `*.res`, generated headers. jphonorato has a usable one; drop
his personal entries (`CLAUDE.md`, `GROK.md`, editor directories), which belong in
`.git/info/exclude`.

Done when: `git status --ignored -s` shows build outputs ignored and source files
visible.

### 27. Do not copy jphonorato's LICENSE

That fork put a top-level MIT license over a tree carrying Microsoft's copyright notices
in nearly every file. The README is currently correct that no license file exists and
rights need review before redistribution. A license can cover `src/port/**` and the
build system with the scope stated. This gets more pressing once there are macOS and
Linux binaries to hand out. Separately, `docs/win32-shim/` is derived from MIT-licensed
Win32Emu and its README carries that attribution.

Done when: `README.md` states license status separately for `src/port/**`, build files,
`docs/win32-shim/**`, and Microsoft's original sources.

### 28. Import `00-reconocimiento.md` in English

2431 lines, every claim backed by a command that was run: per-phase error counts, 182
GCC errors triaged into 5 families with per-file counts, and the BITAPP `sizeof` failure
reproduced end to end. Entirely in Spanish. Sections 1, 4, 5 and the phase-5 LP64
inventory need an English version at minimum.

Done when: the English doc links each retained claim to the original Spanish section and
omits Winelib-only implementation steps that do not apply to the SDL shim.

### 28a. Import jphonorato's probes, not the Qt core

Copy the small diagnostic programs under `docs/port-qt/scripts/fidelity/` and
`docs/port-qt/scripts/handle-check/` into `docs/win32-shim/` after translating the
README text. Do not copy `src/core/`: its contracts are for a Qt shell extraction, while
this repo's contract is still Win32-on-SDL. The probes are valuable because they pin
font substitution, integer glyph advances, and handle round-trips independently of that
architecture.

Done when: `docs/win32-shim/` has a documented command for recapturing font metrics and
a documented command for validating `GlobalHandle(ptr)` round-trips against the shim.

### 28b. Preserve the startup heap diagnosis

Translate and condense `docs/port-linux/01-diagnostico-heap-corruption-arranque.md` and
`02-pendientes-fedora.md` into `docs/win32-shim/startup-debugging.md`. Keep the actionable
bits: ASan and Valgrind fight Wine's preloader, `WINEDEBUG=+heap` was the useful next
step for Winelib, `addr2line` against `WORD1.exe.so` resolved the verified frame to
`N_FormatLineDxa`, and breakpoints against case-shimmed sources need lowercase paths.
Mark every Wine-only command as historical, because the SDL shim will not run under
Wine.

Done when: nobody has to re-read the Spanish Fedora notes to know which debugging paths
were already dead ends and which observations may still inform the native shim.

### 29. README is wrong in four places

It claims Windows-only, which item 16 changes; it says "run the complete Debug test
suite" without noting 8 of 15 tests need a display (`README.md:75`); it documents
`src/cmake/` as an existing directory, which is the one item 2 says was never committed;
and `README.md:18` lists PowerShell as a build requirement, which item 2 removes.

Also `src/port/tools/make_win95_toolbar_sprite.ps1` is a second PowerShell script. It is
not wired into CMake, so it is a manual asset step: leave it or fold it into
`opus_dibapp_tool` alongside item 10.

Done when: README commands match the current presets and `rg powershell README.md` is
empty unless documenting Windows-only or manual asset steps.

### 30. Release packaging is premature

bahree's MSI installs one file with no dependency handling. Revisit when there are three
platforms to package behind a green suite.

Done when: this remains a note only; no packaging task exists before items 16, 17 and 18
are green.
