# SDL probe

Decision: use SDL2 for the first shim backend.

Local macOS evidence:

```sh
command -v sdl2-config
pkg-config --modversion sdl2
cmake -S src --preset macos-debug
cmake --build out/macos-debug --target opus-sdl-probe
SDL_VIDEODRIVER=dummy ./bin/opus-sdl-probe
```

Results:

- `sdl2-config` resolved to `/opt/homebrew/bin/sdl2-config`.
- `pkg-config --modversion sdl2` reported `2.32.70`.
- `cmake -S src --preset macos-debug` configured.
- `opus-sdl-probe` built and ran with `SDL_VIDEODRIVER=dummy`.

Local WebAssembly evidence:

```sh
command -v emcc
command -v em++
cmake -S src --preset wasm-debug
cmake --build out/wasm-debug --target opus-sdl-probe
```

Results:

- `emcc` and `em++` resolved from Homebrew.
- `cmake -S src --preset wasm-debug` configured with Emscripten.
- `opus-sdl-probe.js` linked with `-sUSE_SDL=2`.

Linux remains to be run on a Linux host:

```sh
cmake -S src --preset linux-debug
cmake --build out/linux-debug --target opus-sdl-probe
SDL_VIDEODRIVER=dummy ./bin/opus-sdl-probe
```
