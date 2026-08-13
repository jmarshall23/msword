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

Local Linux evidence:

The Lima Ubuntu aarch64 VM mounted `/Users` read-only, so validation copied the tree to
`/tmp/msword-linux-validate` first.

```sh
sudo apt-get update
sudo apt-get install -y cmake
cmake --version | head -n1
pkg-config --modversion sdl2
rm -rf /tmp/msword-linux-validate
mkdir -p /tmp/msword-linux-validate
tar -C /Users/jserv/playground/msword --exclude=.git --exclude=build --exclude=out --exclude=bin -cf - . | tar -C /tmp/msword-linux-validate -xf -
cd /tmp/msword-linux-validate
cmake -S src --preset linux-debug
cmake --build out/linux-debug --target opus-sdl-probe
SDL_VIDEODRIVER=dummy ./bin/opus-sdl-probe
```

Results:

- `cmake --version | head -n1` reported `cmake version 3.31.6`.
- `pkg-config --modversion sdl2` reported `2.32.4`.
- `cmake -S src --preset linux-debug` configured.
- `opus-sdl-probe` linked to `/tmp/msword-linux-validate/bin/opus-sdl-probe`.
- `SDL_VIDEODRIVER=dummy ./bin/opus-sdl-probe` exited 0.
