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
The MSVC-only constructs are confined to three files, `opus_asm_resn2_sttb.cpp`
(`__declspec(dllimport)`), `opus_original_startup_probe.cpp` (`<rtcapi.h>`, `wWinMain`),
and `opus_win95_chrome.cpp` (`<windowsx.h>`, `uxtheme.dll`). The dead
`opus_product_entry.cpp` stub is gone; every file named here is compiled by a
live target.

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
command/resource targets. The former WORD1 UI-labelled tests now run through
headless scripted `WORD1` modes, while `ctest -L ui` now runs the headless
SDM render reference-image test.

Do: check PR #12's status rollup with `gh pr view 12 --repo
jmarshall23/msword --json statusCheckRollup`, then confirm that the Windows,
macOS, and Linux jobs are attached to the PR and green.

Status: PR #12 is open at `https://github.com/jmarshall23/msword/pull/12` from
`jserv:dev` and is not a draft. As of 2026-08-14 08:07 UTC, fork CI run
`31782393836` is green for Windows, macOS, and Linux at PR head
`16ec02585a507893172b8fe7d1796a5390840812`, but `statusCheckRollup` on the
upstream PR is still empty. The remaining check is to make the upstream PR show
the three green jobs, not just the fork-side run for the same head SHA.

Done when: the three jobs are green on a pull request.

---

## Cut the seam

Execution order in this phase is 7, then 6, then 8, 9, 10, 11. The numbering is
kept stable so citations elsewhere do not rot.

## Shim dependency order

The remaining shim dependency work is SDM control drawing. SDM now has the
device-context and input-loop pieces it needs; the next local task is replacing
the port-owned native-control choice with SDM-drawn controls.

### 15. The controls decision, which is the biggest single item

`opus_sdm_runtime.cpp` builds dialogs from real Win32 control classes through
`create_native_control()` and `create_untracked_control()` helpers starting at `:814`:
44 `"BUTTON"`, 17 `"COMBOBOX"`, 14 `"STATIC"`, 6 `"LISTBOX"`, 5 `"EDIT"`, plus
`GetOpenFileNameA`/`GetSaveFileNameA` at `:2184`. Shimming those means five control
classes with their full `BM_*`, `EM_*`, `LB_*` and `CB_*` protocols, focus and keyboard
navigation, and a common file dialog: more work than gdi32 and user32 combined.

It is also self-inflicted. Word 1.1a shipped SDM, which drew its own controls on Win16
primitives. Native controls are a choice this port made, in code we own.

What remains is SDM's own control drawing over the existing device context and
input loop.

Exclude `opus_win95_chrome.cpp` from non-Windows targets until the SDM path can redraw
that chrome. It is 2882 lines of Win32 chrome with `<windowsx.h>` and `uxtheme.dll`, and
has no cross-platform meaning as written.

The two comdlg32 entries become a small SDM-drawn file browser over item 12's file APIs.

Current finding: `opus_sdm_render_test` now materializes About and Save As through
`OpusSdmRenderDialogPreview` and diffs the rendered buffers against checked-in
PPM reference images under the headless `ui` CTest label. The live SDM path no
longer creates native child controls for SDM-owned controls, and
`opus_sdm_runtime.cpp` no longer calls `GetOpenFileNameA` or `GetSaveFileNameA`.
`opus_x64_runtime_test` now proves the Open and Save As modal paths can complete
from the SDM edit-control path without `WORD1_TEST_FILE_DIALOG_PATH`. The
same test also drives the state-owned file and directory lists through the SDM
host command path: Open navigates a directory list and selects a file-list
entry, while Save As navigates the directory list before accepting the dialog.
The remaining check is runner evidence for the full item 15 gate on Linux.

Done when: a new `opus_sdm_render_test` renders the About and Save As dialogs to a
pixel buffer and diffs against a checked-in reference image, and
`ctest -L ui` is green headless on the Linux runner, and the Linux runner also
passes the headless SDM file-browser test that completes Open and Save As
through list/directory selection without native common dialogs or test-only
environment injection.

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

The blocking-loop problem is handled by the user32 queue model, and Asyncify is
the chosen route for this item.
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

### 25. Write the toolchain-agnostic fixes once

jphonorato guards most of his `src/Opus/` changes with
`#if defined(__GNUC__) && !defined(_MSC_VER)` and keeps the old text in the `#else`. For
the genuinely divergent cases, FARPROC prototypes in `CLIPBRD2.C`, `GRSPEC.C` and
`eldde.c`, and the packing in `exp.c`, that is right. For the rest it is twice the code
for nothing.

Cite these as classes, not single lines, since they recur:

- Cast-as-lvalue, 19 sites. `grep -rnE '^[[:space:]]*\([a-zA-Z_][^)]*\*\)[[:space:]]*[a-zA-Z_]' src/Opus | grep -E '(\+=|-=|=[^=])'`
  returns 24 lines; four of them (`disp1.c:1676`, `elfile.c:1469`, `formula.c:503`,
  `elsubs.c:354`) are ordinary casts inside expressions, not assignments. The 19 real
  ones: `disp1.c:798, 799, 852, 904, 959, 974, 1045, 1046, 1072, 1073, 1249, 1250,
  1722`, `format.c:3013`, `elsubs2.c:290`, `spelcore.c:157`,
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
