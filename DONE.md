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
