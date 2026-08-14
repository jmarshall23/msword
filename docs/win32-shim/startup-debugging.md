# Startup Debugging Notes

This condenses the Fedora startup heap-corruption notes from
`DONT-MERGE/jphonorato/docs/port-linux/01-diagnostico-heap-corruption-arranque.md`
and `DONT-MERGE/jphonorato/docs/port-linux/02-pendientes-fedora.md`.

Those notes describe a historical Winelib build. The SDL shim will not run under
Wine, so every Wine command below is historical context, not a current build or test
instruction. The useful parts are the debugging dead ends, the symbolization path that
worked, and the source-path lesson for breakpoint setup.

## Reproduction Boundary

The crash reproduced consistently in the Fedora environment recorded in
`01-diagnostico-heap-corruption-arranque.md` sections 1 and 11:

- Fedora 44
- GCC 16.1.1
- wine-staging 11.0
- `gdb` 17.2
- `valgrind` 3.27.1

The later Debian environment in section 11.2 did not reproduce the crash:
Debian 13, GCC 14.2.0, Wine 10.0, and `gdb` 16.3. The Fedora follow-up note
therefore treats matching the full Fedora environment as the only confirmed way to
reproduce the original failure. Do not assume a Wine version alone is enough.

The crash had two allocator signatures in section 1:

- `malloc(): invalid size (unsorted)` under `gdb`
- `free(): invalid next size (normal)` in direct Wine runs

Both signatures were interpreted as heap metadata corruption detected at different
allocator operations, not as two unrelated failures.

## Dead Ends

`gdb` after process exit produced unusable stacks in section 2. The notes originally
attributed the empty shared-library list to `wine-preloader`; later section 12.2 adds
an important correction: reading `info sharedlibrary` after the inferior has already
exited is also enough to explain an empty result. Keep the warning, but do not treat
that specific symptom as proof of the root cause.

`winedbg --gdb` failed before useful debugging in section 3. Wine's `dbghelp` asserted
while parsing the GCC 16 DWARF for `WORD1.exe.so`, which also explains why the
project's crash log could show raw `WORD1+0x...` offsets without function names.
Offline symbolization worked; Wine's live DWARF path did not.

`valgrind` was not useful in section 4:

- Without `--trace-children`, it followed only the Wine supervisor process.
- With `--trace-children=yes`, it collided with Wine's low-address reservation before
  `WORD1.exe.so` loaded.

ASan was explicitly ruled out in section 8. A trivial Winelib binary linked with ASan
failed during ASan initialization, before reaching `main`, after several
`ASAN_OPTIONS` combinations. The failure was attributed to the same address-space
reservation class of problem as Valgrind.

The glibc malloc checker ran in section 12.4, but it had low diagnostic value:
Word's Win16/Win32 allocation path mostly goes through Wine's `Rtl*Heap`, not glibc
`malloc`. A clean glibc checker run would not prove the Wine heap was intact.

## What Worked

Offline `addr2line` against the unstripped `WORD1.exe.so` worked in section 5:

```sh
addr2line -e WORD1.exe.so -f -C -i 0x1FD57C
```

That resolved the verified `WORD1+0x1FD57C` frame to:

```text
N_FormatLineDxa
src/port/original/opus_asm_resn2_adapters.cpp:185
```

The notes are careful about the limit of that finding. `N_FormatLineDxa` was a
verified frame in the crashing path, but it did not identify the write that corrupted
the heap. Frame 0 was outside `WORD1.exe.so` and still needed module identification
from `/proc/PID/maps` or equivalent.

## Historical Next Step

For the old Winelib path, the useful next diagnostic was `WINEDEBUG=+heap`, recorded
in `01-diagnostico-heap-corruption-arranque.md` section 12.6 and promoted in
`02-pendientes-fedora.md` section 2:

```sh
WINEDEBUG=+heap wine WORD1.exe.so 2>heap.trace
```

`+heap` is an alias for `trace+heap`. It logs Wine `RtlAllocateHeap`,
`RtlFreeHeap`, `RtlReAllocateHeap`, and `RtlSizeHeap` calls with sizes and returned
pointers. In the historical Winelib build this was the path most likely to identify
which Wine heap block had been corrupted. It is not a command for the SDL shim.

## Source Paths And Breakpoints

Section 12.1 corrected an earlier breakpoint mistake. The generated C sources used by
that build lived under lowercase paths such as `generated/lowercase-c/loadfont.c`.
Breakpoints using uppercase original names stayed pending, while lowercase paths
resolved:

```sh
gdb -q --batch -ex "set breakpoint pending on" \
  -ex "break loadfont.c:349" -ex "break loadfont.c:709" \
  -ex "run" -ex "bt 6" --args wine WORD1.exe.so
```

The portable lesson is not Wine-specific: when debugging case-shimmed generated
sources, match the path recorded in DWARF, not the original mixed-case source name.

## Still Relevant To The SDL Shim

- Keep offline symbolization as a first-class fallback when live stack walkers cannot
  parse debug information.
- Treat allocator crash sites as detection points, not necessarily the corrupting
  writes.
- Prefer tracing the allocator actually used by the build. For Winelib that was
  Wine's `Rtl*Heap`; for the SDL shim it is the shim's own heap/global-memory
  implementation and host allocation layer.
- Preserve source-path case when setting breakpoints against generated files.
- Do not spend time retrying the same ASan or Valgrind-on-Winelib setup without new
  evidence that Wine's address-space behavior changed.
