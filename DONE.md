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
