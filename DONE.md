# DONE

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
