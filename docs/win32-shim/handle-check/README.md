# Handle Check Probe

This probe preserves the useful part of jphonorato's handle round-trip check from
`DONT-MERGE/jphonorato/docs/port-qt/scripts/handle-check/`, adapted to this tree's
Win32 shim API.

The original probe called the discarded Qt shell's `OpusShellMemory` interface. This
version calls the shim directly:

1. `GlobalAlloc`
2. `GlobalLock`
3. write a known byte pattern
4. `GlobalUnlock`
5. `GlobalLock` again and verify the pattern
6. `GlobalHandle(ptr)` and verify the shim finds the allocation token
7. `GlobalFree`
8. verify `GlobalLock` after free fails with `ERROR_INVALID_HANDLE`
9. verify a second `GlobalFree` reports `ERROR_INVALID_HANDLE`

## Validating GlobalHandle

From this directory, compile the probe against the C shim:

```sh
cc -std=c11 -I../../../src/port/win32 \
  handle-check.c ../../../src/port/win32/kernel32.c \
  -pthread -o handle-check
```

Run it:

```sh
./handle-check
```

Expected output ends with:

```text
handle-check: OK
```

This is a host-side shim probe, not a Wine command.
