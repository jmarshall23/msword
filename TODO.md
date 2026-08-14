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
The remaining MSVC-only C++ constructs are confined to `opus_asm_resn2_sttb.cpp`
(`__declspec(dllimport)`) and `opus_original_startup_probe.cpp` (`<rtcapi.h>`,
`wWinMain`). The Win95 chrome `uxtheme.dll` path now builds from C11. The dead
`opus_product_entry.cpp` stub is gone; every file named here is compiled by a live target.

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

The non-Windows configure, SDL probe, and three-platform CI gates are complete.

---

## Cut the seam

Execution order in this phase is 7, then 6, then 8, 9, 10, 11. The numbering is
kept stable so citations elsewhere do not rot.

## Targets, in order

## Pointer-width and LP64 correctness

Real defects in the current x64 build. All line numbers below were re-derived against
this tree; an earlier draft carried jphonorato's post-patch numbers, which are offset by
roughly 18 lines in `exp.c` and 29 in `CLIPBRD2.C`.

### 23. `CP` is `long` and PLC tables are serialized

8 bytes under LP64, 4 under LLP64 and wasm32. Keep the on-disk format at a 32-bit
ceiling and enforce it with fixed-width fields rather than `sizeof(CP)`. Apply the same
on-disk-width rule to `KME` (`wordwin.h:452`): do not widen `int w`, the first member of
the union at `:459`, because that changes `sizeof(KME)`, and `openrare.c:782` reads
keymaps as `iMac * cwKME * 2`.

Complete the `CP`, `FC`, PLC row and `KME` entries in `docs/lp64-audit.tsv` before
editing code, then make the item 22 layout test fail on the current serialized layout.
The generic PLC CP array now packs to 4-byte little-endian values on `OPUS_X64`, and
the PCD/SED row writers/readers use their historical 8-byte and 6-byte disk shapes.
The FIB/file-header path now packs the 420-byte disk FIB into a fixed 512-byte header
while keeping the LP64 runtime FIB layout separate.
Persisted keymap entries now pack to their fixed 4-byte disk shape instead of following
the host pointer-sized runtime `KME`.
The scripted Save As path can now accept an explicit output path, and SDL headless
startup now creates the `OpusWwd` document pane after the default system menu gained
the Win3 `SC_TASKLIST` command that `FCreateMw` rewrites. The explicit SDL headless
Save As harness now exits 0 on macOS, writes the requested native `.doc` artifact,
and produces byte-identical output across repeated local runs. Next run the document
save/open roundtrip with PLC, FIB and keymap data present, capture the saved bytes
from Windows, macOS and Linux, and compare them.

Done when: a document saved on Windows opens byte-identically on macOS and Linux, and
back.

### 25. Write the toolchain-agnostic fixes once

jphonorato guards most of his `src/Opus/` changes with
`#if defined(__GNUC__) && !defined(_MSC_VER)` and keeps the old text in the `#else`. For
the genuinely divergent cases, FARPROC prototypes in `CLIPBRD2.C`, `GRSPEC.C` and
`eldde.c`, and the packing in `exp.c`, that is right. For the rest it is twice the code
for nothing.

Cite these as classes, not single lines, since they recur:

- Cast-as-lvalue pointer assignments. The stale 19-site list from the reference tree
  no longer applies to this tree; the current live cleanup removed the two remaining
  pointer increments in `debug/debugstr.c` and `debug/debuginf.c`. Keep the check
  actionable with `rg -n "^[[:space:]]*\([^)]*\*\)[[:space:]]*[A-Za-z_][A-Za-z0-9_]*[[:space:]]*(\+=|-=|=[^=])" src/Opus`,
  which should return no matches.
- Pointer subtraction between incompatible types: current cleanup made the cited
  `elcore.c`, `inssubs.c:652`, and `pagevw.c:1380` offsets use explicit,
  same-typed subtraction; `elxprocs.c:89` is already same-typed in this tree.
- K&R definitions that conflict with a prototype: current cleanup aligned the
  active `help.h` `FDlgAbout` fallback, `src/Opus/mathapi.c` `LWholeFromNum`,
  and the SDM parser callback declarations with their existing call signatures.
  Locate any remaining instances by compiling, not by line number; the four
  cited in an earlier draft were jphonorato's post-patch positions.
- Flexible array spellings inside `PLDR`, `RSBI` and `GRPZPP` now use the
  existing zero-length member form unconditionally, removing the last local
  `__GNUC__` source guards from `src/Opus/`.

While doing this, fix the formatting the guards introduced in jphonorato's tree before
adopting: a typedef block landed inside a function body at `eldde.c:1279`, and
`GRSPEC.C:1432` lost the indentation of an `if`.

Done when: the Windows build is byte-identical.

---

## Hygiene
