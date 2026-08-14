# Font Probe Notes

These probes preserve the useful parts of jphonorato's font-fidelity scripts from
`DONT-MERGE/jphonorato/docs/port-qt/scripts/fidelity/`.

They are diagnostic programs, not build inputs. Run them against a Windows or Winelib
oracle when font metrics need to be recaptured for the shim. The SDL shim cannot use
the Wine commands below until its gdi32 font path is complete.

## Files

| File | Purpose |
|---|---|
| `gdi-metrics.c` | Captures printable ASCII advances with the Word-style formula: one-character `GetTextExtentPoint32A` minus `tmOverhang`. |
| `gdi-synth.c` | Checks overhang and face resolution for bold, italic, and synthetic era-name cases. |
| `font-substitution.c` | Resolves Word's startup font names, `Tms Rmn`, `Symbol`, `Helv`, and `Courier`, to the actual face selected by GDI. |

The Qt comparison probes and table generator from the source directory were deliberately
left behind. They belong to the discarded Qt-shell experiment and generate
`src/core` artifacts that this SDL-shim tree does not use.

## Recapturing Font Metrics

Build the Win32 oracle probes with Wine or with a Windows compiler:

```sh
winegcc -o gdi-metrics.exe gdi-metrics.c -lgdi32 -luser32
winegcc -o gdi-synth.exe gdi-synth.c -lgdi32 -luser32
winegcc -o font-substitution.exe font-substitution.c -lgdi32 -luser32
```

Capture integer advances for a face and point size:

```sh
WINEDEBUG=-all ./gdi-metrics.exe "Liberation Serif" 14 > metrics.txt
```

Capture startup font substitution:

```sh
WINEDEBUG=-all ./font-substitution.exe > font-substitution.txt
```

Capture synthetic bold and italic behavior:

```sh
WINEDEBUG=-all ./gdi-synth.exe > gdi-synth.txt
```

For the future golden-file test, repeat `gdi-metrics.exe` for the startup font names
at 8, 10, 12, 14, 18, 24, and 36 pt, then diff the resulting advances against the
shim's gdi32 output.
