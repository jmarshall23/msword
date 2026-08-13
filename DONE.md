# DONE

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
