# DONE

## Add GDI DC and object state

`src/port/win32/gdi32.cpp` now owns the first GDI layer: `HDC`, `HBITMAP`, `HPEN`,
`HBRUSH`, and `HFONT` handles; stock objects; type-aware `SelectObject`;
`GetCurrentObject`; `SaveDC`/`RestoreDC`; object deletion rules; bitmap creation and
resource bitmap bridging. Resource-loaded bitmaps now enter the same GDI object table as
created bitmaps, so later raster work has one handle model.

`opus_win32_gdi_object_test` covers default DC selections, pen/brush/font/bitmap
selection return values, save/restore state, selected-object delete rejection, bitmap
metadata, stock-object delete rejection, and DC teardown. The small `CharUpperBuffA`,
`GetStringTypeA`, and `MulDiv` kernel helpers were added because `opus_x64_runtime_test`
reaches them before the later user32 work.

Validated with `ctest --test-dir build-item13 -R 'strtbl|sttb|plc|sdm_cab|command|win32_memory|opus_win32_gdi_object_test|win32_coverage' --output-on-failure`.
`opus_x64_runtime_test` now links on macOS, then stops at the next user32 scope:
`FInitSdm_sdm21` calls `GetSystemMetrics` and `GetSysColor`, which remain item 14 work.

## Implement kernel32 shim tier

`src/port/win32/kernel32.cpp` now provides the non-Windows kernel32 surface reached by
the current non-UI tests: heap and global memory, local aliases, file/path operations,
directory scanning, time, process/error stubs, SRW locks, command-line setup, and the
small string helpers. `C:\...` paths map through `OPUS_C_DRIVE`, defaulting to the repo
root, with backslash conversion and case-insensitive component retry.

The `Global*` model keeps a registry from locked pointers back to their owning handles,
returns the `GlobalHandle(ptr)` token in the high word for the live `HIWORD` caller,
saturates `GlobalUnlock` at zero, and leaves freed metadata long enough to reject a
double free. A focused `opus_win32_memory_test` covers zero-init, lock/write,
`GlobalHandle(ptr)`, realloc zero extension, and double-free failure.

The item-12 C tests now build on non-Windows without linking `user32`, with Apple using
the existing dynamic-lookup pattern for unused static-runtime references. The coverage
baseline was refreshed after the implemented kernel32 entries became reachable.

Validated with `ctest --test-dir build-item12 -R 'strtbl|sttb|plc|sdm_cab|command|win32_memory|win32_coverage' --output-on-failure`.

## Make SDM controls state-owned again

`opus_sdm_runtime.cpp` no longer creates Win32 child controls for SDM-owned buttons,
edits, list boxes, combo boxes, or statics. Those controls now live as `ControlState`
records, with untracked labels and frames kept alongside the dialog state. The Open and
Save As path no longer calls the native common file dialog; the existing test hook still
injects a selected path, and otherwise the SDM path cancels until the later file browser
is drawn over the shim.

`HwndOfTmc` keeps its ABI but now returns `nullptr` for SDM controls, and
`opus_x64_runtime_test.cpp` checks the control records through `GetTmcRec` instead of
expecting child HWND classes.

Validated with the done-condition grep for `CreateWindowExA` in
`opus_sdm_runtime.cpp`, which now finds only the two `OpusSdmDialog` host-window calls.
`opus_sdm_runtime.cpp` and `opus_x64_runtime_test.cpp` compile in the focused runtime
target. `win32_coverage` passes after removing the now-stale uncovered entries
`GetClassNameA`, `GetComboBoxInfo`, `GetFileAttributesExA`, and
`SetEnvironmentVariableA`.

The full default macOS build still hits the existing legacy C `implicit-int` failures,
and the focused runtime test links far enough to hit the existing macOS `-luser32`
linker gap.

## Move `RC` out of the top-level `LANGUAGES`

`src/CMakeLists.txt` now declares only `C` and `CXX` in `project()`, enables `RC` only
on Windows, and adds `port/word1.rc` to `WORD1` only on Windows.

Validated on macOS with `cmake -S src -B out/item1-verify -G Ninja`: configure now gets
past `project()` and stops at the next known non-Windows gate, with no RC compiler
error.

Reviewed by agy and claude: no findings.

## Commit `src/cmake/`

`src/cmake/GenerateElxInfoHeader.cmake` and
`src/cmake/GenerateMenuHelpHeader.cmake` are committed from the jphonorato reference
tree. The ELX generation command now uses `cmake -P` instead of PowerShell.

Validated with `rg -n "powershell" src/CMakeLists.txt`, direct `cmake -P` runs for both
generators, and non-Windows configure reaching the next known gate.

Reviewed by agy and claude: no blocking findings.

## Port native non-MSVC host-tool builds

The legacy C host tools now get native clang/gcc flags and host CRT shims without
touching `opus_cabi_tool`. `BITAPP` pins its serialized `BITMAP` layout to the Win16
14-byte form on LP64, `mkcmd` avoids nonstandard lvalue-cast increments and packed host
pointers, and `mkdlg` has a normal host `main` under `OPUS_X64_TOOL`.

Validated by linking `mkcmd`, `mkdlg`, `mergeelx`, `bitapp`, and `opus_dibapp_tool`
directly with AppleClang, then running `bitapp` on `src/Opus/resource/8hdr.bmp`.

Reviewed by agy and claude: cross-build host-tool imports and the CTest reference remain
open in `TODO.md`.

## Add macOS and WebAssembly configure presets with SDL probe

`macos-debug` and `wasm-debug` presets now configure with Ninja. SDL2 is the selected
backend dependency: macOS uses CMake's SDL2 package, and WebAssembly links the probe with
Emscripten's `-sUSE_SDL=2`.

Validated by configuring `macos-debug` and `wasm-debug`, building `opus-sdl-probe` for
both, and running the macOS probe with `SDL_VIDEODRIVER=dummy`.

## Add the BITAPP regression and finish the macOS tool gate

The macOS native tool gate now builds `mkcmd`, `mkdlg`, `mergeelx`, `bitapp`, `dibapp`
and `opus_cabi_tool` from the `macos-debug` preset. The non-Windows shim header has the
minimal Win32 base types needed by `opus_cabi_tool`, and `opus_x64_compat.h` only includes
`malloc.h` under MSVC.

`opus_bitapp_8hdr_test` runs `opus_bitapp_tool` on `Opus/resource/8hdr.bmp` and compares
the output to the checked-in `port/tools/references/8hdr.hb` oracle.

Validated from `src` with the macOS tool build command, then from the repository root
with `ctest --test-dir out/macos-debug -R opus_bitapp_8hdr_test --output-on-failure`.

## Import native host tools for WebAssembly cross builds

`wasm-debug` now imports the five build-machine tools from the native tools directory
instead of compiling them with Emscripten. `opus_cabi_tool` remains target-built, and the
Emscripten build runs its JavaScript output through Node so it still measures the target
layout.

BITAPP custom commands use relative input and output paths from `src`, matching the
legacy tool's switch parser. MKCMD now computes its dialog source path relative to the
generated command directory instead of relying on a hardcoded depth.

Validated by first building the native tools with `macos-debug`, then building
`opus_generated_commands`, `opus_generated_embedded_resources` and
`opus_generated_dib_resources` from `wasm-debug`.

## Complete Linux configure and SDL probe validation

`linux-debug` now configures on a case-sensitive Linux filesystem. The original engine
source list uses `Opus/CMD3.C`, matching the checked-in filename instead of relying on
macOS case folding.

Validated `cmake -S src --preset macos-debug` on macOS. In a Lima Ubuntu aarch64 VM,
installed CMake 3.31.6 with apt, copied the repository to `/tmp/msword-linux-validate`
because `/Users` was read-only, then ran `cmake -S src --preset linux-debug`,
`cmake --build out/linux-debug --target opus-sdl-probe`, and
`SDL_VIDEODRIVER=dummy ./bin/opus-sdl-probe`.

## Swap `opus_windows_sdk.h`, and close the seven leaks

`src/port/original/windows.h` now routes through the generated
`opus_windows_sdk.h`: MSVC still uses the real Windows SDK, while non-Windows
uses `src/port/win32/windows.h`. The non-Windows include path exposes
`src/port/win32` and the `src/port/win32/case` forwarders for `Windows.h`,
`DbgHelp.h`, and the original Opus mixed-case quoted headers.

Shim-only SDK declarations moved out of `src/port/original` into
`src/port/win32`. The live SDK header leaks were closed with local include
routes for DbgHelp, shell, common-dialog, object-base, IME, and windowsx
surfaces. `rtcapi` and `malloc.h` are MSVC-only, and `opus_asm_file2.cpp`
uses direct MSVC `_chdir`/`_getdcwd` declarations rather than adding an
unused non-Windows `direct.h` shim.

On macOS, `WORD1` and `opus_word1_ui_test` compile and link with the deliberate
Darwin dynamic-loader handoff. The smoke test reaches runtime loading and
aborts on missing `_SymFunctionTableAccess64`, a DbgHelp shim body, not on a
missing type, macro, prototype, header, or C++ linkage error.

Validated with `commentflow` on the changed C/C++ headers and sources,
`git diff --check`, the item 6 include grep returning only the intended
`windows.h` routes, `cmake -S src --preset macos-debug`,
`cmake --build out/macos-debug --target opus_word1_ui_test`, and fork CI run
`31735543515` green on Windows, macOS and Linux.

Reviewed by agy and claude: no blocking findings after correcting the stale
`direct.h` wording.

## Decide `WCHAR` width

Non-Windows `WCHAR` is fixed at 16 bits in `src/port/win32/windows.h`, with
both C++ `static_assert(sizeof(WCHAR) == 2)` and a C typedef assertion. The
port uses `OPUSW("")` and explicit `WCHAR` buffers at live non-Windows `W`
API boundaries instead of relying on host `wchar_t` width.

The live entry point is `opus_original_startup_probe.cpp`; `opus_product_entry.cpp`
is still dead because no CMake target references it. The non-Windows `main`
bridge widens `argv` bytes into `WCHAR` storage and enters `wWinMain` without
calling unresolved Win32 shim bodies first. Windows keeps the DbgHelp stack
walk; non-Windows crash logging avoids taking DbgHelp callback addresses so
the dynamic loader no longer blocks the self-test before `wWinMain`.

Validated with `commentflow src/port/original/opus_original_startup_probe.cpp`,
`git diff --check`, `cmake --build out/macos-debug --target WORD1`,
`cmake --build out/macos-debug --target opus_word1_ui_test`,
`bin/WORD1 --self-test`, the `word1_port_smoke_test` CTest, `nm -u bin/WORD1`
showing no unresolved
`SymFunctionTableAccess64`, `SymGetModuleBase64` or `StackWalk64`, and raw
wide-literal/`wchar_t` greps over the live item files.

Reviewed by agy and claude: they identified the DbgHelp load-time binding as
the only blocking proof gap; the follow-up patch removed that blocker.

## Replace self-module command lookup

`ResolveCommandAddress` no longer asks `GetProcAddress` to find Word commands
through the executable export table. A CMake generation step now reads
`opuscmd_native.inc`, writes `opuscmddata.inc` without the forced `/export:`
pragmas, and writes `opuscmdtable.h` with 427 direct command declarations and a
sorted address table. `opus_asm_movecmds.c` includes the filtered command data
and uses `bsearch` over the generated table.

`mkcmd.c` remains untouched. The raw `opuscmd_native.inc` is still the generated
intermediate, but it is no longer compiled by `opus_asm_movecmds.c`, so the
product no longer needs platform-specific self-export linker tricks for command
resolution.

Validated with `cmake -S src --preset macos-debug`,
`cmake --build out/macos-debug --target opus_generated_commands`,
`cmake --build out/macos-debug --target opus_original_command_test`,
`cmake --build out/macos-debug --target WORD1`,
`ctest --test-dir out/macos-debug -R 'opus_original_command_test|word1_port_smoke_test' --output-on-failure`,
`bin/WORD1 --self-test`, `commentflow src/port/original/opus_asm_movecmds.c`,
`git diff --check`, generated table count of 427 entries, no `/export:` or
`#pragma comment` lines in `opuscmddata.inc`, no command-table names unresolved
in `bin/WORD1`, and no `/export:` strings in `libopus_original_engine.a`.

Reviewed by agy and claude: agy caught that compiling raw `opuscmd_native.inc`
would leave forced exports in place; claude confirmed the generated filtered
include and table approach and narrowed the proof away from an invalid
zero-global-symbol `nm` check.

## Resolve Win16 module ordinals

Non-Windows builds now compile `port/win32/module.cpp` into the x64 runtime. It
returns stable sentinel handles for the Win16 system module names `KERNEL`,
`USER`, `GDI`, and `KEYBOARD`, keeps `GetModuleHandleA/W(NULL)` as the self
module handle, returns `0` for external `LoadLibraryA` and `LoadLibraryExW`
probes, and resolves only integer-resource ordinals on exact sentinel handles.

The implemented live ordinal surface is documented in
`docs/win32-shim/module-ordinal-map.tsv`. Live mappings cover `KERNEL`
`GetFreeSpace` plus the flat selector no-ops, `USER` `ExitWindows` as a no-op,
`KEYBOARD` `SetSpeed` as a no-op, and the GDI bitmap ordinals used by the
clipboard/startup paths. Dead rows such as `GDI` ordinal `440` remain
documented but unmapped.

Validated with `cmake -S src --preset macos-debug`,
`cmake --build out/macos-debug --target opus_win16_module_test`,
`ctest --test-dir out/macos-debug -R opus_win16_module_test --output-on-failure`,
`cmake --build out/macos-debug --target WORD1`,
`ctest --test-dir out/macos-debug -R 'opus_win16_module_test|word1_port_smoke_test' --output-on-failure`,
`bin/WORD1 --self-test`, `OPUS_TRACE=1 bin/WORD1 --self-test`,
`cmake --build out/macos-debug --target opus_word1_ui_test`,
`commentflow src/port/win32/module.cpp src/port/original/opus-win16-module-test.cpp`,
`git diff --check`, and `nm -u bin/WORD1` showing no unresolved
`GetModuleHandleA/W`, `GetProcAddress`, `LoadLibraryA`, `LoadLibraryExW`, or
`FreeLibrary`.

Reviewed by agy and claude: both required the shim to sit beneath the existing
`OpusGetProcAddress` wrapper, to use exact sentinel handles, to omit the dead
`GDI` 440 mapping, and to land the ordinal TSV with the code.

## Replace PE resources

Non-Windows builds now generate raw byte arrays for
`src/port/assets/word95-toolbar.bmp`, `src/port/icons/ICON8_1.ico`, and
`src/port/icons/ICON2_1.ico`. The shim implements resource ID lookup for
`LoadImageA/W`, `LoadBitmapA/W`, and `LoadIconA/W`, plus `DestroyIcon` and
`DrawIconEx` for the generated icon handles.

The toolbar bitmap handle is no longer only a non-null token: `GetObjectA`,
`GetBitmapBits`, and `GetBitmapDimensionEx` expose the generated 340x20x24 BMP
metadata and pixel bytes. Stock `OBM_*` bitmap IDs used by the port return
stable shim handles.

`WORD1` compiles `port/word1.rc` and `port/winword.manifest` only on Windows.
The toolbar render proof belongs to the later device-context and windowing
work: this item supplies the resource bytes, not the screen path.

Validated with `cmake -S src --preset macos-debug`,
`cmake --build out/macos-debug --target opus_win32_resource_test WORD1`,
`ctest --test-dir out/macos-debug -R 'opus_win32_resource_test|opus_win16_module_test|word1_port_smoke_test|opus_original_command_test' --output-on-failure`,
`rg -n "word1\\.rc|winword\\.manifest" out/macos-debug/build.ninja`
returning no matches, `nm -u build/tests/Debug/opus_win32_resource_test`
showing no unresolved resource shim APIs, `commentflow
src/port/win32/resource.cpp src/port/original/opus-win32-resource-test.cpp`,
and `git diff --check`.

Reviewed by agy and claude: both rejected adding narrow GDI/User32 shims here
and agreed that the original visual-render criterion depended on later
device-context and windowing work. Claude also caught that the resource test
should link without Darwin dynamic lookup, which now passes.

## Add the Win32 coverage gate

Non-Windows native CTest now has `win32_coverage`, a CMake script test that runs
`nm` over `libopus_original_engine.a` and `libopus_x64_runtime.a`. It uses
`src/port/win32/windows.h` as the Win32 name filter, so macro spellings such as
`SendMessage` are checked after compilation as their real undefined symbols
rather than by source grep.

The current uncovered set is checked in as
`docs/win32-shim/uncovered.txt`. The gate fails if a new declared Win32 symbol is
undefined and absent from that baseline, and it also fails when a baseline entry
has become covered but was not removed. That keeps the list monotonic as shim
work lands.

Validated with `cmake -S src --preset macos-debug`,
`cmake --build out/macos-debug --target WORD1 opus_win32_resource_test`,
`ctest --test-dir out/macos-debug -R win32_coverage --output-on-failure`, and
a temporary-baseline failure proof where removing `AcquireSRWLockExclusive` from
the baseline made `CheckWin32Coverage.cmake` fail with that exact name under
`New uncovered entry points`.

Reviewed by agy and claude: both recommended the `nm`-over-archives path, a pure
CMake script, and a burn-down baseline instead of a source grep or compiled
scanner. Claude also called out the Mach-O leading-underscore trap, which the
script strips before matching.

## Add GDI raster operations

The non-Windows gdi32 shim now rasterizes `PatBlt`, `BitBlt`, `StretchBlt`,
`SetPixel`, `GetPixel`, `GetDIBits`, `SetStretchBltMode`, and `SetROP2` against
the selected bitmap in a memory DC. The implementation covers the raster ops
Word needs for toolbar, caret, and selection drawing, including `BLACKNESS`,
`WHITENESS`, `PATCOPY`, `PATINVERT`, `DSTINVERT`, `SRCCOPY`, `NOTSRCCOPY`,
`SRCAND`, `SRCINVERT`, and the documented pattern/source ROPs from the reference
notes. Resource 201's toolbar bitmap now returns one cached bitmap handle so
`LoadImageW(201)` and `LoadBitmapA(201)` agree.

`opus_win32_gdi_raster_test` exercises solid and patterned destination writes,
source blits, nearest-neighbor stretching, clipping from negative origins, DIB
row reads, and DC mode state returns. The Win32 coverage baseline no longer
lists the covered raster entry points.

Validated with `cmake -S src -B build-item13b -DCMAKE_C_FLAGS=-std=gnu89`,
`cmake --build build-item13b --target opus_original_engine opus_x64_runtime
opus_original_strtbl_test opus_original_sttb_test opus_original_plc_test
opus_sdm_cab_test opus_original_command_test opus_win32_memory_test
opus_win32_resource_test opus_win32_gdi_object_test opus_win32_gdi_raster_test
--parallel 8`, and `ctest --test-dir build-item13b -R
'strtbl|sttb|plc|sdm_cab|command|win32_memory|opus_win32_resource_test|opus_win32_gdi_(object|raster)_test|win32_coverage'
--output-on-failure`, which passed 10/10. `opus_x64_runtime_test` remains behind
item 14 because startup SDM still reaches missing user32 metrics/color APIs.

Reviewed by agy and claude: both kept this subtask to software raster operations
inside the gdi32 shim, with text metrics and user32 left for their own items.

## Add GDI text metrics

The non-Windows gdi32 shim now supplies deterministic font metrics for the
startup faces `Tms Rmn`, `Helv`, `Courier`, and `Symbol`. `CreateFontIndirectA/W`
normalizes those names through the substitution table, selected fonts feed
`GetTextMetricsA/W`, `GetTextExtentPoint32A`, and `GetCharWidthA`, and
`EnumFontsA`/`EnumFontFamiliesExA` enumerate the same four faces without holding
the GDI lock across callbacks. `GetDeviceCaps` now returns the live screen,
bitmap, raster, text, and 96-DPI caps Word reads while building font caches.

The metric data is a small built-in design-unit table, not captured Windows GDI
oracle data. It locks additivity and pitch behavior for this shim and leaves
exact Windows pagination fidelity as a data replacement task when oracle output
is available.

`opus_win32_font_test` covers whole-string versus per-character extent
additivity, `GetCharWidthA`, metric height/internal-leading coherence, zero
overhang, fixed versus variable pitch bits, font enumeration including early
callback stop, filtered `EnumFontFamiliesExA`, and the live device caps. The
Win32 coverage baseline no longer lists `EnumFontFamiliesExA`, `GetDeviceCaps`,
`GetTextExtentPoint32A`, or `GetTextMetricsA`.

Validated with `cmake -S src -B build-item13c -DCMAKE_C_FLAGS=-std=gnu89`,
`cmake --build build-item13c --target opus_original_strtbl_test
opus_original_sttb_test opus_original_plc_test opus_sdm_cab_test
opus_original_command_test opus_win32_memory_test opus_win32_resource_test
opus_win32_gdi_object_test opus_win32_gdi_raster_test opus_win32_font_test
--parallel 8`, and `ctest --test-dir build-item13c -R
'strtbl|sttb|plc|sdm_cab|command|win32_memory|opus_win32_resource_test|opus_win32_gdi_(object|raster)_test|opus_win32_font_test|win32_coverage'
--output-on-failure`, which passed 11/11. `opus_x64_runtime_test` still builds
but segfaults through a null call; unresolved imports are now user32 startup
surface only: `CreateWindowExA`, `RegisterClassExA`, `GetDC`, `ReleaseDC`,
`GetSystemMetrics`, and `GetSysColor`.

Reviewed by agy and claude: agy confirmed the minimal text-metrics surface and
font substitution table; claude caught the hidden `GetDeviceCaps` and
`GetCharWidthA` prerequisites and the reentrant `EnumFontsA` callback trap.

## Add GDI print escape failure

The non-Windows gdi32 shim now owns Word's live print surface: legacy
`Escape`. The shim reports unsupported escapes through `OutputDebugStringA`,
returns `SP_ERROR` for `STARTDOC`, returns zero for unsupported capability and
driver-control escapes, and clears the `NEXTBAND` output rectangle before
returning zero so a stale band cannot spin the print loop. The modern
`StartDoc`/`EndDoc`/page APIs were left out because this tree does not call
them.

`opus_win32_print_test` covers the `STARTDOC` failure, unsupported
`QUERYESCSUPPORT`, unchanged page-size output, zeroed `NEXTBAND`, and harmless
zero returns for the abort/end/draft controls. The Win32 coverage baseline no
longer lists `Escape`.

Validated with `cmake -S src -B build-item13d -DCMAKE_C_FLAGS=-std=gnu89`,
`cmake --build build-item13d --target opus_win32_print_test
opus_original_engine opus_x64_runtime --parallel 8`, and `ctest --test-dir
build-item13d -R
'strtbl|sttb|plc|sdm_cab|command|win32_memory|opus_win32_resource_test|opus_win32_gdi_(object|raster)_test|opus_win32_font_test|opus_win32_print_test|win32_coverage'
--output-on-failure`, which passed 12/12.

`GETPHYSPAGESIZE` returning zero still leaves a printer-path landmine in
`SCREEN2.C`: that path can read an uninitialized page point once printer DC
creation exists. It is currently unreachable because printer DC creation is not
implemented.

Reviewed by agy and claude: agy outlined the predictable failure contract;
claude narrowed the implementation to `Escape`, preserved its legacy prototype,
and caught the `NEXTBAND` stale-output trap.

## Add user32 startup seed

The non-Windows runtime now links a first `user32.cpp` shim. It implements the
startup surface reached by the current SDM/runtime path: `RegisterClass*`,
`CreateWindowEx*`, `DestroyWindow`, window validity, parent/owner traversal,
window long slots, basic rect/position/visibility/enabled/focus state, screen DC
acquisition through gdi32, deterministic system metrics/colors, and simple
message queue stubs. `kernel32.cpp` also owns `GetCurrentProcessId`,
`GetEnvironmentVariableA`, and `SetEnvironmentVariableA`.

`opus_x64_runtime_test` no longer links with macOS `dynamic_lookup`, so missing
Win32 imports surface as link errors instead of null function calls. The previous
segfaulting runtime test now passes.

`opus_win32_user32_test` covers duplicate class registration, synchronous
`WM_NCCREATE`, user data and extra bytes, parent versus owner, child invalidation
through `DestroyWindow`, focus/enabled state, work-area metrics, rect adjustment,
and screen/window DC release.

Validated with `cmake --build build-item14a --target
opus_original_strtbl_test opus_original_sttb_test opus_original_plc_test
opus_sdm_cab_test opus_original_command_test opus_win32_memory_test
opus_win32_resource_test opus_win32_gdi_object_test opus_win32_gdi_raster_test
opus_win32_font_test opus_win32_print_test opus_win32_user32_test
opus_x64_runtime_test opus_original_engine opus_x64_runtime --parallel 8`,
then `ctest --test-dir build-item14a -R
'strtbl|sttb|plc|sdm_cab|command|opus_x64_runtime_test|win32_memory|opus_win32_resource_test|opus_win32_gdi_(object|raster)_test|opus_win32_font_test|opus_win32_print_test|opus_win32_user32_test|win32_coverage'
--output-on-failure`, which passed 14/14.

Reviewed by agy and claude: agy identified the minimal startup API set, and
claude caught that `dynamic_lookup` was hiding the real link gate and narrowed the
message pump to no-work stubs for this runtime path.

## Add user32 message core

The non-Windows `user32.cpp` shim now owns the in-memory message core:
`SendMessageA/W` dispatch synchronously to the window procedure, `PostMessageA/W`
queue messages, `PeekMessageA` honors filter and remove flags, `GetMessageA`
returns queued messages and only quits on `WM_QUIT`, and `WaitMessage` fails fast
until an event backend exists. `DefWindowProcA/W` now defaults to zero, keeps
`WM_NCCREATE` successful, handles `WM_CLOSE`, and owns basic text
get/set/length messages.

`windows.h` now routes macro spellings for `SendMessage`, `GetMessage`,
`PostMessage`, and `DefWindowProc` through typed shim entry points with casts for
the historical C call sites. That makes macro-spelled engine calls visible to
`win32_coverage` without breaking old pointer/integer argument patterns.

`opus_win32_user32_test` now covers synchronous send dispatch, default return,
peek no-remove versus remove, hwnd/message filtering, `PostQuitMessage`,
`GetMessageA` quit return, and queued-message purge through `DestroyWindow`.
The Win32 coverage baseline no longer lists `PeekMessageA`, `SendMessageA`, or
`SendMessageW`.

Validated with `cmake --build build-item14b --target
opus_original_strtbl_test opus_original_sttb_test opus_original_plc_test
opus_sdm_cab_test opus_original_command_test opus_win32_memory_test
opus_win32_resource_test opus_win32_gdi_object_test opus_win32_gdi_raster_test
opus_win32_font_test opus_win32_print_test opus_win32_user32_test
opus_x64_runtime_test opus_original_engine opus_x64_runtime --parallel 8`,
then `ctest --test-dir build-item14b -R
'strtbl|sttb|plc|sdm_cab|command|opus_x64_runtime_test|win32_memory|opus_win32_resource_test|opus_win32_gdi_(object|raster)_test|opus_win32_font_test|opus_win32_print_test|opus_win32_user32_test|win32_coverage'
--output-on-failure`, which passed 14/14.

Reviewed by agy and claude: both prioritized the message core before SDL,
menus, caret, clipboard, or scrollbars. The `GetWindowWord`/`SetWindowWord`
surface remains out of this slice because its 16-bit extra-byte model needs a
separate pointer-width audit.

## Add user32 text and extra accessors

The non-Windows `user32.cpp` shim now handles byte-width extra-window access
instead of treating every positive offset as a pointer-sized slot.
`GetWindowLongA`/`SetWindowLongA` use four bytes, `GetWindowWord`/`SetWindowWord`
use two bytes, and pointer-sized `GetWindowLongPtrA`/`SetWindowLongPtrA` keep their
existing eight-byte behavior. `GetTopWindow` is declared and implemented because
the original engine calls it before passing the result into typed window helpers.

Window text is now available through `SetWindowTextW`, `GetWindowTextA/W`, and
`GetWindowTextLengthA/W`, using the same stored caption string already owned by
`SetWindowTextA` and `DefWindowProc`'s `WM_GETTEXT`/`WM_SETTEXT` handling. The
Win32 coverage baseline no longer lists `GetWindowTextA`,
`GetWindowTextLengthA`, `GetWindowTextLengthW`, `GetWindowTextW`, or
`SetWindowTextW`.

`opus_win32_user32_test` covers 32-bit and 16-bit extra-byte writes sharing the
same byte array without clobbering adjacent slots, narrow and wide window text
round trips, and `GetTopWindow` over a child window.

Validated with `cmake --build build-item14c --target
opus_win32_user32_test opus_x64_runtime_test opus_original_engine --parallel 8`
and `ctest --test-dir build-item14c -R
'opus_win32_user32_test|opus_x64_runtime_test|win32_coverage'
--output-on-failure`, which passed 3/3. Also validated with `cmake --build
build-item14c --target opus_original_strtbl_test opus_original_sttb_test
opus_original_plc_test opus_sdm_cab_test opus_original_command_test
opus_win32_memory_test opus_win32_resource_test opus_win32_gdi_object_test
opus_win32_gdi_raster_test opus_win32_font_test opus_win32_print_test
opus_win32_user32_test opus_x64_runtime_test opus_original_engine
opus_x64_runtime --parallel 8`, then `ctest --test-dir build-item14c -R
'strtbl|sttb|plc|sdm_cab|command|opus_x64_runtime_test|win32_memory|opus_win32_resource_test|opus_win32_gdi_(object|raster)_test|opus_win32_font_test|opus_win32_print_test|opus_win32_user32_test|win32_coverage'
--output-on-failure`, which passed 14/14.

Reviewed by agy for this slice; claude was asked and returned after implementation
was underway with guidance for the next input-message slice and a warning that the
final WORD1 link gate is still masked by macOS `dynamic_lookup`.

## Add user32 key translation core

The non-Windows `user32.cpp` shim now tracks key state for posted
`WM_KEYDOWN`/`WM_KEYUP` and `WM_SYSKEYDOWN`/`WM_SYSKEYUP` messages, exposes
`GetKeyState`, and accepts `SetKeyboardState`. `TranslateMessage` now synthesizes
printable `WM_CHAR` or `WM_SYSCHAR` messages for letters, digits, space, return,
and tab, inserting the generated character ahead of later queued input so the next
peek/get sees it.

`opus_win32_user32_test` covers shift key state, posted keydown state updates,
`TranslateMessage` producing a shifted `WM_CHAR`, dispatch through the test window
procedure, and keyup clearing the down bit. The Win32 coverage baseline no longer
lists `GetKeyState` or `SetKeyboardState`.

Validated with `cmake --build build-item14d --target
opus_win32_user32_test opus_x64_runtime_test opus_original_engine --parallel 8`
and `ctest --test-dir build-item14d -R
'opus_win32_user32_test|opus_x64_runtime_test|win32_coverage'
--output-on-failure`, which passed 3/3. Also validated with `cmake --build
build-item14d --target opus_original_strtbl_test opus_original_sttb_test
opus_original_plc_test opus_sdm_cab_test opus_original_command_test
opus_win32_memory_test opus_win32_resource_test opus_win32_gdi_object_test
opus_win32_gdi_raster_test opus_win32_font_test opus_win32_print_test
opus_win32_user32_test opus_x64_runtime_test opus_original_engine
opus_x64_runtime --parallel 8`, then `ctest --test-dir build-item14d -R
'strtbl|sttb|plc|sdm_cab|command|opus_x64_runtime_test|win32_memory|opus_win32_resource_test|opus_win32_gdi_(object|raster)_test|opus_win32_font_test|opus_win32_print_test|opus_win32_user32_test|win32_coverage'
--output-on-failure`, which passed 14/14.

Reviewed by agy and claude before implementation: agy recommended the key
translation slice; claude returned with client geometry as the next preferred slice
and confirmed the input core as the following bounded step.

## Add user32 client geometry

The non-Windows `user32.cpp` shim now owns the declared client-geometry surface:
`GetClientRect`, child-aware `GetWindowRect`, `ClientToScreen`,
`ScreenToClient`, `MoveWindow`, and ancestor-aware `IsWindowVisible`. Window
creation now seeds visibility from `WS_VISIBLE`, and `ShowWindow`/`SetWindowPos`
keep the visibility bit and stored state in sync.

The same slice implements the pure rect helpers already declared in the shim:
`IntersectRect`, `OffsetRect`, and `PtInRect`. Empty intersections zero the
destination and return `FALSE`, and point containment uses the Win32 exclusive
right/bottom edge convention.

`opus_win32_user32_test` covers child client size, child-to-screen conversion,
round-trip coordinate conversion, point inclusion edges, non-empty and empty
intersection, offsetting, parent-hidden visibility inheritance, `MoveWindow`, and
the updated child window screen rect. The Win32 coverage baseline no longer lists
`ClientToScreen`, `GetClientRect`, `IntersectRect`, `IsWindowVisible`,
`MoveWindow`, `OffsetRect`, `PtInRect`, or `ScreenToClient`.

Validated with `cmake --build build-item14e --target
opus_win32_user32_test opus_x64_runtime_test opus_original_engine --parallel 8`
and `ctest --test-dir build-item14e -R
'opus_win32_user32_test|opus_x64_runtime_test|win32_coverage'
--output-on-failure`, which passed 3/3. Also validated with `cmake --build
build-item14e --target opus_original_strtbl_test opus_original_sttb_test
opus_original_plc_test opus_sdm_cab_test opus_original_command_test
opus_win32_memory_test opus_win32_resource_test opus_win32_gdi_object_test
opus_win32_gdi_raster_test opus_win32_font_test opus_win32_print_test
opus_win32_user32_test opus_x64_runtime_test opus_original_engine
opus_x64_runtime --parallel 8`, then `ctest --test-dir build-item14e -R
'strtbl|sttb|plc|sdm_cab|command|opus_x64_runtime_test|win32_memory|opus_win32_resource_test|opus_win32_gdi_(object|raster)_test|opus_win32_font_test|opus_win32_print_test|opus_win32_user32_test|win32_coverage'
--output-on-failure`, which passed 14/14.

Reviewed by agy before implementation; claude was asked again but had not
returned before implementation, so this slice used claude's prior geometry review
for the child-coordinate conversion, visibility inheritance, and hidden
undeclared rect-helper traps.

## Add user32 pure rect helpers

The non-Windows user32 shim now declares and implements the pure rect helpers that
the engine calls but the coverage gate could not previously see: `SetRect`,
`InflateRect`, and `UnionRect`. `UnionRect` ignores empty inputs, zeroes the
destination and returns `FALSE` when both inputs are empty, and tolerates aliased
destination/source use through local emptiness checks.

`opus_win32_user32_test` extends the existing geometry block with set, inflate,
non-empty union, union with one empty input, and all-empty union assertions. No
coverage baseline edit was needed because the declarations and implementations
land in the same commit.

Validated with `cmake --build build-item14f --target
opus_win32_user32_test opus_x64_runtime_test opus_original_engine --parallel 8`
and `ctest --test-dir build-item14f -R
'opus_win32_user32_test|opus_x64_runtime_test|win32_coverage'
--output-on-failure`, which passed 3/3. Also validated with `cmake --build
build-item14f --target opus_original_strtbl_test opus_original_sttb_test
opus_original_plc_test opus_sdm_cab_test opus_original_command_test
opus_win32_memory_test opus_win32_resource_test opus_win32_gdi_object_test
opus_win32_gdi_raster_test opus_win32_font_test opus_win32_print_test
opus_win32_user32_test opus_x64_runtime_test opus_original_engine
opus_x64_runtime --parallel 8`, then `ctest --test-dir build-item14f -R
'strtbl|sttb|plc|sdm_cab|command|opus_x64_runtime_test|win32_memory|opus_win32_resource_test|opus_win32_gdi_(object|raster)_test|opus_win32_font_test|opus_win32_print_test|opus_win32_user32_test|win32_coverage'
--output-on-failure`, which passed 14/14.

Reviewed by agy before implementation; claude was asked for this slice but had not
returned before implementation, so the prechecks followed claude's prior warning
about declaring and defining hidden rect helpers in the same commit.
