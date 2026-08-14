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

## Shim dependency order

Execution order here is 13, 14, 15b. Item 15 still finishes after item 14 because
SDM's own control drawing needs both the device context and the input loop.
Numbering stays stable so citations do not rot.

### 13. gdi32 tier, 72 entries

The DC model is the design work: `CreateCompatibleDC`, `SelectObject` (167 sites in
`src/Opus`, the most-called Win32 function in the tree), `SaveDC`/`RestoreDC`, clipping,
and the raster ops (`PatBlt` at 96 sites, `BitBlt`, `StretchBlt`, `SetROP2`).
This is where the Word 95 toolbar sprite from resource 201 first has to blit through
the shim instead of merely loading as bytes.

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

Printing through Word's live `Escape` path now fails at `STARTDOC`; the modern
`StartDoc`/`EndDoc`/page APIs have no current call sites.

The DC state slice now covers `SetBkMode`, `SetTextColor`, `SetMapMode`, viewport and
window origin/extent accessors, `MoveToEx`, and simple clipped `LineTo` drawing through
the selected pen. The remaining work is still the higher-value paint, region, shape,
text, and font behavior needed by real document rendering.

The rectangle primitive slice now covers `FillRect`, `FrameRect`, and `Rectangle` on
the selected bitmap. Remaining shape work includes non-rectangular clipping, `Polygon`,
and `Ellipse`.

The rectangular clip slice now covers `CreateRectRgn` and `IntersectClipRect` for the
existing single-rectangle DC clip. `ExcludeClipRect` still needs real region handling
before it can model cut-outs without lying to callers.

The system color brush slice now covers `GetSysColorBrush` for chrome fills that need
an `HBRUSH` view of the existing `GetSysColor` palette.

The edge drawing slice now covers `DrawEdge` for raised and sunken one-pixel border
sides used by the chrome. Remaining shape work includes `Polygon`, `Ellipse`, and
UTF-16 text output.

The device context creation slice now covers `CreateDCA` and `CreateICA` by reusing
the existing memory DC state. It does not model physical devices or metafiles; those
stay separate from the selected-object DC behavior.

The metafile handle slice now covers `CreateMetaFile` and `CloseMetaFile` as an
opaque handle lifecycle. It does not record or replay drawing commands; add that only
when picture import/export reaches those paths.

The clipboard state slice now covers process-local custom formats, open/close, owner,
empty, availability, and handle get/set. It intentionally does not free handles when
the clipboard is emptied because ownership varies by caller path.

The paint invalidation slice now covers `BeginPaint`, `EndPaint`, `InvalidateRect`,
and `RedrawWindow` for the chrome repaint paths.

The scroll state slice now covers `SetScrollRange` and `SetScrollPos` for independent
horizontal and vertical window scroll state with default ranges and range clamping.

The system menu slice now covers `GetSystemMenu` with per-window system menus and
revert behavior.

The hook pass-through slice now covers `CallNextHookEx` for the playback hook paths
that delegate unhandled hook codes when no native hook chain exists.

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

A first non-Windows `user32.cpp` seed is in place. It covers the startup link
surface that blocked `opus_x64_runtime_test`: class registration, window creation
and destruction, parent/owner lookup, window extra bytes, DC acquisition, system
metrics/colors, simple visibility/enabled/focus state, environment/process helpers
used by the runtime harness, and honest no-work message stubs. The runtime test no
longer uses macOS `dynamic_lookup`, so missing imports fail at link time instead of
as null calls.

The message core now handles direct `SendMessageA/W`, queued `PostMessageA/W`,
`PeekMessageA` removal semantics, `GetMessageA` over queued messages, `WaitMessage`
as a backend-required fail-fast, `WM_CLOSE`, and default window text messages.
Macro spellings for `SendMessage`, `GetMessage`, `PostMessage`, and
`DefWindowProc` are routed through the shim with casts so old C call sites stay
buildable and visible to the coverage gate.

Window text and byte-width extra accessors are now in place. `GetWindowTextA/W`,
`GetWindowTextLengthA/W`, `SetWindowTextW`, `GetWindowWord`, `SetWindowWord`, and
`GetTopWindow` build through the original engine, and the coverage baseline no
longer lists the implemented text APIs. `GetWindowWord`/`SetWindowWord` currently
cover positive per-window extra-byte offsets; negative `GWW_*` slots stay out until
the call-site audit proves this tree needs them. The final WORD1 link gate is still
not flipped because the app target keeps macOS `dynamic_lookup` while many user32
entries remain uncovered.

The in-memory input core now tracks key down/up state from posted key messages and
`SetKeyboardState`, exposes `GetKeyState`, and has `TranslateMessage` synthesize
printable `WM_CHAR`/`WM_SYSCHAR` messages. This proves dispatch through the existing
queue and window procedure, but it is not yet the headless event source: blocking
`GetMessage`/`WaitMessage` still fail fast until the SDL or scripted pump feeds the
queue. The next reviewed small slice is client geometry and rect algebra, because
mouse packing and paint/update work need coherent client coordinates.

Client geometry and the common rect helpers now exist for the declared user32
surface: `GetClientRect`, `GetWindowRect` with child-to-screen conversion,
`ClientToScreen`, `ScreenToClient`, `IntersectRect`, `OffsetRect`, `PtInRect`,
`MoveWindow`, and ancestor-aware `IsWindowVisible`. The coverage baseline no longer
lists those names. Undeclared rect helpers such as `SetRect`, `InflateRect`, and
`UnionRect` remain hidden from `win32_coverage` until they are declared and
implemented together.

The pure rect-helper blind spot has been closed for `SetRect`, `InflateRect`, and
`UnionRect`: all three are now declared and implemented together, so the coverage
gate can see them without adding stale uncovered entries. Paint/update-region rect
helpers remain out of scope until the paint slice.

Capture and cursor-position state are now in the shim: `SetCapture`,
`GetCapture`, `ReleaseCapture`, `SetCursor`, `SetCursorPos`, and `GetCursorPos`
all have process-local state and coverage entries removed. This is still not the
headless event source; it gives later mouse/scripted input somewhere to store
capture and screen cursor coordinates.

Default hit testing is now in place: `DefWindowProcA/W` handle `WM_NCHITTEST`
with screen coordinates, and `WindowFromPoint` walks visible/enabled windows in
reverse creation order while skipping `HTNOWHERE` and `HTTRANSPARENT`. This is
still not a mouse pump; it supplies hit-test routing for future mouse/scripted
input.

A minimal scripted-input pump is now in place for tests. `PeekMessageA`,
`GetMessageA`, and `WaitMessage` can pull one scripted event at a time into the
existing queue, and `PostQuitMessage` is modeled as a quit flag so `PM_NOREMOVE`
can observe it without consuming it. This is still not SDL input, timer
delivery, or full mouse packing; those are the remaining event-source slices.

Timer delivery is now in the same queue path: `SetTimer` and `KillTimer` keep
process-local periodic timers, `GetMessageA` and `WaitMessage` can wait until
the next timer deadline, and due timers enter the queue as `WM_TIMER` without
duplicating an already queued timer message. SDL input and full mouse packing
remain the event-source work before the headless keystroke gate.

The hidden input/cursor state entries are now declared and implemented:
`GetAsyncKeyState` reads the same key state as the queue path, mouse button
messages update `VK_LBUTTON`/`VK_RBUTTON`, and `ShowCursor` maintains the
cursor display counter reported by `SM_CURSORLEVEL`. SDL event translation is
still needed to feed those states from real backend events.

The in-memory window lookup helpers are now backed by the existing window list:
enumeration, class/text lookup, ancestor lookup, control IDs, simple iconic and
zoomed state, top-window activation, and `MessageBeep` no-op success. These
close more declared surface for Word and the chrome bridge without adding
backend behavior.

A first in-memory menu core is now in place: menu handles,
append/insert/modify/remove/delete, item count/id/state/text/submenu queries,
check/radio state, window menu attachment, and no-op draw/track calls. This is
still not a popup UI or menu rendering path; it gives Word's startup/menu
mutation code a real menu tree to maintain.

Window properties are now backed by per-window state for `SetPropW`,
`GetPropA/W`, and `RemovePropW`. String keys are matched case-insensitively and
integer atom keys are accepted as raw 16-bit ids; property enumeration and a
global atom table remain out of scope until real call sites require them.

The small callback/string helpers `CallWindowProcW`, `lstrcmpW`, and
`lstrcmpiW` are now implemented. String comparison is ordinal over 16-bit code
units, with ASCII-only folding for the insensitive form; full NLS collation is
not part of the shim until a non-ASCII call site proves it is needed.

The kernel string conversion helpers `MultiByteToWideChar` and
`WideCharToMultiByte` now cover the code pages this tree calls today:
`CP_ACP`, `CP_UTF8`, and 1252. They handle sizing calls, `-1` terminator
counts, truncation failure, UTF-8 round trips, and default-character fallback
for wide characters that do not fit a byte code page.

The small kernel environment helpers `GetCurrentThreadId`,
`GlobalMemoryStatusEx`, and `GetTempFileNameA` are now implemented for the
single-thread shim and test harness. They provide a stable thread id, bounded
memory-status numbers, and Win32-style temporary file names with file creation
when the caller asks the shim to generate the unique value.

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

What remains is SDM's own control drawing, over item 13's device context, after item 14.

Exclude `opus_win95_chrome.cpp` from non-Windows targets until the SDM path can redraw
that chrome. It is 2882 lines of Win32 chrome with `<windowsx.h>` and `uxtheme.dll`, and
has no cross-platform meaning as written.

The two comdlg32 entries become a small SDM-drawn file browser over item 12's file APIs.

Done when: a new `opus_sdm_render_test` renders the About and Save As dialogs to a
pixel buffer and diffs against a checked-in reference image, and
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
