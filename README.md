# Microsoft Word for Windows 1.1a - Native x64 Port

This project is a native x64 port of Microsoft Word for Windows 1.1a, whose
historical codename was **Opus**. It builds the original Word source and
resources together with modern replacements for the 16-bit assembly,
segmented-memory, and Win16 platform boundaries.

The result is the original Word application and user experience running as a
64-bit Windows executable, with macOS, Linux, and WebAssembly work proceeding
behind the Win32 shim in `src/port/win32/`. This is not an emulator or a
reimplementation using a modern editor control.

## Requirements

- CMake 3.25 or newer
- For Windows: 64-bit Windows, Visual Studio 2022 with **Desktop development
  with C++**, and a Windows 10 or Windows 11 SDK installed through Visual
  Studio
- For macOS and Linux configure/build probes: Ninja, a C/C++ compiler, and SDL2
- For WebAssembly configure/build probes: Emscripten

## Build and run

Clone the repository, then configure and build from `src` with the preset for
your host:

```sh
git clone https://github.com/jmarshall23/msword.git
cd msword/src

cmake --preset x64-debug
cmake --build --preset x64-debug
```

On Windows, run the built application from `src`:

```sh
../bin/WORD1.exe
```

For an optimized Windows build, use the release preset instead:

```sh
cmake --preset x64-release
cmake --build --preset x64-release
```

The Windows presets use the Visual Studio 2022 x64 generator. After
configuration, the generated solution can also be opened directly from
`out/MicrosoftWordX64Port.sln`; use `WORD1` as the startup project.

The non-Windows presets currently build configure probes, native tools, and
runtime tests used by the porting work:

```sh
cmake --preset macos-debug
cmake --build --preset macos-debug

cmake --preset linux-debug
cmake --build --preset linux-debug

cmake --preset wasm-debug
cmake --build --preset wasm-debug
```

## Test

Run the configured Debug tests from the repository root:

```sh
ctest --test-dir ./out -C Debug --output-on-failure
```

Or, when your current directory is `src`:

```sh
ctest --test-dir ../out -C Debug --output-on-failure
```

For a release Windows build, replace `Debug` with `Release`. The configured
suite covers the ported x64 runtime, original Word data structures and command
tables, process startup, and automated UI workflows including typing,
selection, formatting, dialogs, and saving.

The `ui` labelled tests drive the desktop application and need a display. For
headless runs, exclude them:

```sh
ctest --test-dir ./out -C Debug -LE ui --output-on-failure
```

Ninja presets use preset-specific build directories under `out/`, for example:

```sh
ctest --test-dir ./out/macos-debug -LE ui --output-on-failure
ctest --test-dir ./out/linux-debug -LE ui --output-on-failure
```

## Project layout

| Path | Purpose |
| --- | --- |
| `src/Opus/` | Original Microsoft Word/Opus application source and resources |
| `src/OpusEtAl/` | Original supporting tools, libraries, and build inputs |
| `src/OpusProg/` | Historical program documentation |
| `src/port/original/` | x64 compatibility layer, translated routines, and tests |
| `src/port/tools/` | Native replacements for historical build-time tools |
| `src/cmake/` | Resource and source-generation helpers |
| `out/` | CMake cache and generated Visual Studio solution |
| `build/` | Intermediate tools, tests, probes, PDBs, and diagnostics |
| `bin/` | Final executable and runtime files |

`out`, `build`, and `bin` are generated locally during configuration and
compilation.

## How the port works

The original C and resource files remain the authoritative implementation.
The port adds only the platform work needed to build and run that code safely
on 64-bit hosts:

- 16-bit x86 assembly entry points are translated to fixed-width C or C++.
- Segmented and double-indirect memory handles are mapped to an x64-safe native
  runtime.
- Win16-specific startup, messaging, graphics, file, and resource behavior is
  adapted to current Win32 APIs.
- Original command, dialog, cursor, bitmap, and other generated assets are
  rebuilt by native host tools as part of the CMake graph.
- Unit, runtime, smoke, and UI tests guard compatibility with the original
  algorithms and application behavior.

CMake inventories the legacy assembly tree but does not compile those modules
into native targets. This keeps the historical implementation available as a
reference while ensuring all shipped code is valid for AMD64.

## Useful targets

| Target | Description |
| --- | --- |
| `WORD1` | The native x64 Microsoft Word executable |
| `opus_original_engine` | Original Word application engine compiled for x64 |
| `opus_x64_runtime` | Native runtime and translated assembly behavior |
| `opus_word1_ui_test` | Automated end-to-end UI test driver |
| `legacy_sources` | IDE-visible reference collection of the original assembly |

Build a specific target with:

```sh
cmake --build --preset x64-debug --target WORD1
```

## Contributing

Changes should preserve the original Word behavior while keeping all native
interfaces pointer-width safe. Prefer source-equivalent translations of
historical routines, isolate unavoidable Windows API adaptation at the port
boundary, and add focused tests for newly translated behavior.

## License Status

This repository does not currently include a top-level license file. Review the
applicable rights before redistributing source or binaries.

- `src/Opus/`, `src/OpusEtAl/`, and the other original Microsoft Word source
  trees retain their historical Microsoft and third-party copyright notices.
  They are not covered by a new project license in this repository.
- `src/port/**` contains porting code added around the original sources. Its
  intended license still needs to be stated explicitly before redistribution.
- Build-system files, including CMake scripts and workflow files, also need an
  explicit project license before redistribution.
- `docs/win32-shim/**` is derived from MIT-licensed Win32Emu reference
  material, with attribution and scope details in `docs/win32-shim/README.md`.
