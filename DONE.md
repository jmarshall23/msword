# DONE

## Replace WORD1 Save As UI harness

`opus_word1_save_as_test` now runs `WORD1 --scripted-save-as-test` in process
with `OPUS_HEADLESS=1 SDL_VIDEODRIVER=dummy`. The scripted mode exits the
normal startup loop, sends the File/Save As command to the live `OpusApp`
window, and fails unless the SDM file-dialog path reaches its test-hook
auto-cancel stage and returns with no dialog windows left open.

Validated with `cmake --build build-chrstride --target WORD1 -j2`,
`ctest --test-dir build-chrstride -R '^opus_word1_save_as_test$'
--output-on-failure`, the scripted group
`ctest --test-dir build-chrstride -R '^opus_word1_save_as_test$|^opus_word1_interaction_test$|^opus_word1_selection_test$|^opus_word1_about_test$|^opus_word1_unicode_test$|^opus_word1_clipboard_shortcut_test$|^opus_word1_typing_test$|^word1_scripted_key_test$'
--output-on-failure`, the broader
`ctest --test-dir build-chrstride -R 'strtbl|sttb|plc|sdm_cab|command|opus_x64_runtime_test|win32_memory|opus_win16_module_test|opus_win32_resource_test|opus_win32_gdi_(object|raster)_test|opus_win32_font_test|opus_win32_print_test|opus_win32_user32_test|win32_coverage|word1_port_smoke_test|word1_scripted_key_test|opus_word1_typing_test|opus_word1_clipboard_shortcut_test|opus_word1_unicode_test|opus_word1_about_test|opus_word1_selection_test|opus_word1_interaction_test|opus_word1_save_as_test'
--output-on-failure` sweep, `ctest --test-dir build-chrstride -N -L ui`
showing 3 remaining UI-labelled tests, and `git diff --check`.

Reviewed by agy and claude: agy could not start because its TTY UI failed to
open `/dev/tty`; claude stalled without output and was stopped.

## Replace WORD1 interaction UI harness

`opus_word1_interaction_test` now runs `WORD1 --scripted-interaction-test` in
process with `OPUS_HEADLESS=1 SDL_VIDEODRIVER=dummy`. The scripted mode exits
the normal startup loop, finds the live `OpusApp` window, verifies the
maximize/restore `WM_SYSCOMMAND` path, pre-queues a cancel command for the New
dialog loop, sends File/New, and fails unless the command returns with the app
alive and no dialog left open.

Validated with `cmake --build build-chrstride --target WORD1 -j2`,
`ctest --test-dir build-chrstride -R '^opus_word1_interaction_test$|^opus_word1_selection_test$|^opus_word1_about_test$|^opus_word1_unicode_test$|^opus_word1_clipboard_shortcut_test$|^opus_word1_typing_test$|^word1_scripted_key_test$'
--output-on-failure`, the broader
`ctest --test-dir build-chrstride -R 'strtbl|sttb|plc|sdm_cab|command|opus_x64_runtime_test|win32_memory|opus_win16_module_test|opus_win32_resource_test|opus_win32_gdi_(object|raster)_test|opus_win32_font_test|opus_win32_print_test|opus_win32_user32_test|win32_coverage|word1_port_smoke_test|word1_scripted_key_test|opus_word1_typing_test|opus_word1_clipboard_shortcut_test|opus_word1_unicode_test|opus_word1_about_test|opus_word1_selection_test|opus_word1_interaction_test'
--output-on-failure` sweep, `ctest --test-dir build-chrstride -N -L ui`
showing 4 remaining UI-labelled tests, and `git diff --check`.

Reviewed by agy and claude: agy could not start because its TTY UI failed to
open `/dev/tty`; claude timed out without output.

## Replace WORD1 selection UI harness

`opus_word1_selection_test` now runs `WORD1 --scripted-selection-test` in
process with `OPUS_HEADLESS=1 SDL_VIDEODRIVER=dummy`. The scripted mode exits
the normal startup loop, finds the live `OpusWwd` pane, inserts the selection
test sentence through Word's diagnostic insertion path, sends the pane mouse
down/up messages at the same sentence-end coordinate, and fails unless the
selection state matches the old insertion-point checks.

Validated with `cmake --build build-chrstride --target WORD1 -j2`,
`ctest --test-dir build-chrstride -R '^opus_word1_selection_test$|^opus_word1_about_test$|^opus_word1_unicode_test$|^opus_word1_clipboard_shortcut_test$|^opus_word1_typing_test$|^word1_scripted_key_test$'
--output-on-failure`, the broader
`ctest --test-dir build-chrstride -R 'strtbl|sttb|plc|sdm_cab|command|opus_x64_runtime_test|win32_memory|opus_win16_module_test|opus_win32_resource_test|opus_win32_gdi_(object|raster)_test|opus_win32_font_test|opus_win32_print_test|opus_win32_user32_test|win32_coverage|word1_port_smoke_test|word1_scripted_key_test|opus_word1_typing_test|opus_word1_clipboard_shortcut_test|opus_word1_unicode_test|opus_word1_about_test|opus_word1_selection_test'
--output-on-failure` sweep, `ctest --test-dir build-chrstride -N -L ui`
showing 5 remaining UI-labelled tests, and `git diff --check`.

Reviewed by agy and claude: agy could not start because its TTY UI failed to
open `/dev/tty`; claude timed out without output.

## Replace WORD1 About UI harness

`opus_word1_about_test` now runs `WORD1 --scripted-about-test` in process with
`OPUS_HEADLESS=1 SDL_VIDEODRIVER=dummy`. The scripted mode exits the normal
startup loop, finds the live `OpusApp` window, pre-queues an OK command for
the modal dialog loop, sends the Help/About command, and fails unless the
About dialog path runs and returns with the app window still alive.

Validated with `cmake --build build-chrstride --target WORD1 -j2`,
`ctest --test-dir build-chrstride -R '^opus_word1_about_test$|^opus_word1_unicode_test$|^opus_word1_clipboard_shortcut_test$|^opus_word1_typing_test$|^word1_scripted_key_test$'
--output-on-failure`, the broader
`ctest --test-dir build-chrstride -R 'strtbl|sttb|plc|sdm_cab|command|opus_x64_runtime_test|win32_memory|opus_win16_module_test|opus_win32_resource_test|opus_win32_gdi_(object|raster)_test|opus_win32_font_test|opus_win32_print_test|opus_win32_user32_test|win32_coverage|word1_port_smoke_test|word1_scripted_key_test|opus_word1_typing_test|opus_word1_clipboard_shortcut_test|opus_word1_unicode_test|opus_word1_about_test'
--output-on-failure` sweep, `ctest --test-dir build-chrstride -N -L ui`
showing 6 remaining UI-labelled tests, and `git diff --check`.

Reviewed by agy and claude: agy could not start because its TTY UI failed to
open `/dev/tty`; claude timed out without output.

## Replace WORD1 Unicode UI harness

`opus_word1_unicode_test` now runs `WORD1 --scripted-unicode-test` in process
with `OPUS_HEADLESS=1 SDL_VIDEODRIVER=dummy`. The scripted mode exits the
normal Word loop, finds the live `OpusWwd` pane, queues the same scalar set
through Word's existing Unicode diagnostic insertion path, and verifies every
stored scalar through the existing document query.

Validated with `cmake --build build-chrstride --target WORD1 -j2`,
`ctest --test-dir build-chrstride -R '^opus_word1_unicode_test$|^opus_word1_clipboard_shortcut_test$|^opus_word1_typing_test$|^word1_scripted_key_test$'
--output-on-failure`, the broader
`ctest --test-dir build-chrstride -R 'strtbl|sttb|plc|sdm_cab|command|opus_x64_runtime_test|win32_memory|opus_win16_module_test|opus_win32_resource_test|opus_win32_gdi_(object|raster)_test|opus_win32_font_test|opus_win32_print_test|opus_win32_user32_test|win32_coverage|word1_port_smoke_test|word1_scripted_key_test|opus_word1_typing_test|opus_word1_clipboard_shortcut_test|opus_word1_unicode_test'
--output-on-failure` sweep, `ctest --test-dir build-chrstride -N -L ui`
showing 7 remaining UI-labelled tests, and `git diff --check`.

Reviewed by agy and claude: agy could not start because its TTY UI failed to
open `/dev/tty`; claude timed out without output.

## Replace WORD1 clipboard shortcut UI harness

`opus_word1_clipboard_shortcut_test` now runs
`WORD1 --scripted-clipboard-test` in process with
`OPUS_HEADLESS=1 SDL_VIDEODRIVER=dummy`. The scripted mode types three
characters through the user32 queue, verifies the existing Ctrl+A/C/V/X/Z
command bindings through Word's diagnostic query, executes Ctrl+A, and fails
unless the document selection expands.

Validated with `cmake --build build-chrstride --target WORD1 -j2`,
`ctest --test-dir build-chrstride -R '^opus_word1_clipboard_shortcut_test$|^opus_word1_typing_test$|^word1_scripted_key_test$'
--output-on-failure`, the broader
`ctest --test-dir build-chrstride -R 'strtbl|sttb|plc|sdm_cab|command|opus_x64_runtime_test|win32_memory|opus_win16_module_test|opus_win32_resource_test|opus_win32_gdi_(object|raster)_test|opus_win32_font_test|opus_win32_print_test|opus_win32_user32_test|win32_coverage|word1_port_smoke_test|word1_scripted_key_test|opus_word1_typing_test|opus_word1_clipboard_shortcut_test'
--output-on-failure` sweep, `ctest --test-dir build-chrstride -N -L ui`
showing 8 remaining UI-labelled tests, and `git diff --check`.

Reviewed by agy and claude: agy could not start because its TTY UI failed to
open `/dev/tty`; claude timed out without output.

## Assert WORD1 scripted typing mutates the document

`WORD1 --scripted-typing-test` now finds the in-process `OpusWwd` document
pane after the scripted loop exits and reuses Word's existing diagnostic
selection query. The gate fails unless the three consumed `WM_CHAR` messages
also leave a canonical insertion selection and a document length at least as
large as the typed character count.

Validated with `cmake --build build-chrstride --target WORD1 -j2`,
`ctest --test-dir build-chrstride -R '^opus_word1_typing_test$|^word1_scripted_key_test$'
--output-on-failure`, the broader
`ctest --test-dir build-chrstride -R 'strtbl|sttb|plc|sdm_cab|command|opus_x64_runtime_test|win32_memory|opus_win16_module_test|opus_win32_resource_test|opus_win32_gdi_(object|raster)_test|opus_win32_font_test|opus_win32_print_test|opus_win32_user32_test|win32_coverage|word1_port_smoke_test|word1_scripted_key_test|opus_word1_typing_test'
--output-on-failure` sweep, and `git diff --check`.

Reviewed by agy and claude: agy could not start because its TTY UI failed to
open `/dev/tty`; claude timed out without output.

## Replace WORD1 typing UI harness

`opus_word1_typing_test` now runs `WORD1 --scripted-typing-test` in process
with `OPUS_HEADLESS=1 SDL_VIDEODRIVER=dummy` instead of launching the old
out-of-process `opus_word1_ui_test --typing` harness. The scripted mode reuses
the user32 queue and observer, queues three key cycles, and fails if Word's
message loop does not consume the expected three `WM_CHAR` messages.

Validated with `cmake --build build-chrstride --target WORD1 -j2`,
`ctest --test-dir build-chrstride -R '^opus_word1_typing_test$|^word1_scripted_key_test$'
--output-on-failure`, the broader
`ctest --test-dir build-chrstride -R 'strtbl|sttb|plc|sdm_cab|command|opus_x64_runtime_test|win32_memory|opus_win16_module_test|opus_win32_resource_test|opus_win32_gdi_(object|raster)_test|opus_win32_font_test|opus_win32_print_test|opus_win32_user32_test|win32_coverage|word1_port_smoke_test|word1_scripted_key_test|opus_word1_typing_test'
--output-on-failure` sweep, and `git diff --check`.

Reviewed by agy and claude: agy could not start because its TTY UI failed to
open `/dev/tty`; claude timed out without output.

## Advance WORD1 scripted startup paint

`WORD1 --scripted-key-test` now reaches startup paint and exits cleanly under
`OPUS_HEADLESS=1 SDL_VIDEODRIVER=dummy`. The shim fixes were the minimal missing
pieces on that path: separator menu ids survive `ModifyMenu`, plain
`GetModuleFileName` and `AdjustWindowRect` resolve to the existing shim behavior,
and the GDI path exposes background/text colors plus ANSI/plain `TextOut` and
`ExtTextOut`.

Regression coverage landed in the existing module, user32, and GDI raster tests.

## Register WORD1 scripted startup gate

`word1_scripted_key_test` now runs `WORD1 --scripted-key-test` through CTest with
the same headless environment used by the manual gate.

## Assert WORD1 scripted character delivery

`WORD1 --scripted-key-test` now arms a one-shot user32 observer for the expected
scripted character and returns nonzero if Word's message loop never consumes the
matching `WM_CHAR`. The user32 test covers the observer on a shifted key path.

## Assert repeated WORD1 scripted characters

The scripted character observer now waits for a count, and
`WORD1 --scripted-key-test` queues two key cycles before quitting. The CTest gate
therefore fails if repeated scripted keys do not become repeated consumed
`WM_CHAR` messages.

## Complete gdi32 shim tier

`src/port/win32/gdi32.c` now covers the checked Win32 GDI surface used by the
current port, and `docs/win32-shim/uncovered.txt` is empty.

`opus_win32_font_test` now reads `src/port/original/opus-win32-font-golden.tsv`
and checks the captured oracle advances for printable ASCII across `Tms Rmn`,
`Symbol`, `Helv`, and `Courier` at 8, 10, 12, 14, 18, 24, and 36 pt. The shim
uses the same captured rows for those startup face/size combinations and keeps
the old heuristic for uncaptured fonts.

Validated with `cmake --build build-chrstride --target opus_win32_font_test -j2`,
`ctest --test-dir build-chrstride -R '^opus_win32_font_test$' --output-on-failure`,
`cmake --build build-chrstride --target opus_x64_runtime_test -j2`, and
`ctest --test-dir build-chrstride -R '^opus_x64_runtime_test$' --output-on-failure`.
The broader regression sweep
`ctest --test-dir build-chrstride -R 'strtbl|sttb|plc|sdm_cab|command|opus_x64_runtime_test|win32_memory|opus_win32_resource_test|opus_win32_gdi_(object|raster)_test|opus_win32_font_test|opus_win32_print_test|opus_win32_user32_test|win32_coverage'
--output-on-failure` passed after building the missing test executables, and
`git diff --check` passed.

Reviewed by agy and claude: agy could not start because its TTY UI failed to
open `/dev/tty`; claude timed out without output.

## Add gdi32 excluded clip rectangles

`src/port/win32/gdi32.c` now implements `ExcludeClipRect` as exclusion state
on the DC instead of shrinking the base clip rectangle. Raster writes for text,
fills, shapes, and blits now skip excluded pixels, and `SaveDC`/`RestoreDC`
preserve the exclusion list with the rest of the DC state.

`opus-win32-gdi-raster-test` now covers a center clip hole, full exclusion,
and restoring the previous clip state. The final implemented symbol was removed
from `docs/win32-shim/uncovered.txt`.

Validated with `cmake --build build-chrstride --target opus_win32_gdi_raster_test
-j2`, `ctest --test-dir build-chrstride -R 'opus_win32_gdi_raster_test|win32_coverage'
--output-on-failure`, the broader
`ctest --test-dir build-chrstride -R 'opus_win32_(gdi_object|gdi_raster|font|print|user32|memory)_test|win32_coverage'
--output-on-failure` sweep, an empty `docs/win32-shim/uncovered.txt`, and
`git diff --check`.

Reviewed by agy and claude: agy could not start because its TTY UI failed to
open `/dev/tty`; claude timed out without output.

## Add gdi32 UTF-16 text output

`src/port/win32/gdi32.c` now implements `TextOutW` and `DrawTextW` as a
minimal visible text path using the selected font advances, text color, and
opaque background mode. `DrawTextW` handles null-terminated input and the
single-line center/vertical-center flags used by the shim chrome.

`opus-win32-gdi-raster-test` now covers visible UTF-16 text strokes, opaque
background fill, and `DrawTextW` count handling. The implemented symbols were
removed from `docs/win32-shim/uncovered.txt`.

Validated with `cmake --build build-chrstride --target opus_win32_gdi_raster_test
-j2`, `ctest --test-dir build-chrstride -R 'opus_win32_gdi_raster_test|win32_coverage'
--output-on-failure`, the broader
`ctest --test-dir build-chrstride -R 'opus_win32_(gdi_object|gdi_raster|font|print|user32|memory)_test|win32_coverage'
--output-on-failure` sweep, searches proving `TextOutW` and `DrawTextW` are gone
from `uncovered.txt`, and `git diff --check`.

Reviewed by agy and claude: agy could not start because its TTY UI failed to
open `/dev/tty`; claude timed out without output.

## Add gdi32 filled shapes

`src/port/win32/gdi32.c` now implements `Ellipse` and `Polygon` against the
selected bitmap, brush, pen, and current rectangular clip. `LineTo` and
`Polygon` share the same clipped line drawing helper.

`opus-win32-gdi-raster-test` now covers ellipse fill/outline and polygon
fill/outline. The implemented symbols were removed from
`docs/win32-shim/uncovered.txt`.

Validated with `cmake --build build-chrstride --target opus_win32_gdi_raster_test
-j2`, `ctest --test-dir build-chrstride -R 'opus_win32_gdi_raster_test|win32_coverage'
--output-on-failure`, the broader
`ctest --test-dir build-chrstride -R 'opus_win32_(gdi_object|gdi_raster|font|print|user32|memory)_test|win32_coverage'
--output-on-failure` sweep, searches proving `Ellipse` and `Polygon` are gone
from `uncovered.txt`, and `git diff --check`.

Reviewed by agy and claude: agy could not start because its TTY UI failed to
open `/dev/tty`; claude timed out without output.

## Add gdi32 metafile handles

`src/port/win32/gdi32.c` now implements `CreateMetaFile` and `CloseMetaFile`
as a minimal metafile DC lifecycle. Metafile-created DCs close to an opaque
metafile handle; ordinary DCs are rejected.

`opus-win32-gdi-object-test` now covers metafile DC creation, close rejection
for normal DCs, successful close, and handle deletion. The implemented symbols
were removed from `docs/win32-shim/uncovered.txt`.

Validated with `cmake --build build-chrstride --target opus_win32_gdi_object_test
-j2`, `ctest --test-dir build-chrstride -R 'opus_win32_gdi_object_test|win32_coverage'
--output-on-failure`, the broader
`ctest --test-dir build-chrstride -R 'opus_win32_(gdi_object|gdi_raster|font|print|user32|memory)_test|win32_coverage'
--output-on-failure` sweep, searches proving `CreateMetaFile` and
`CloseMetaFile` are gone from `uncovered.txt`, and `git diff --check`.

Reviewed by agy and claude: agy could not start because its TTY UI failed to
open `/dev/tty`; claude timed out without output.

## Add gdi32 device context creation

`src/port/win32/gdi32.c` now implements `CreateDCA` and `CreateICA` as
device-name-neutral DC creators backed by the existing compatible DC state.

`opus-win32-gdi-object-test` now covers independent device and information
contexts, default selected objects, and deletion. The implemented symbols were
removed from `docs/win32-shim/uncovered.txt`.

Validated with `cmake --build build-chrstride --target opus_win32_gdi_object_test
-j2`, `ctest --test-dir build-chrstride -R 'opus_win32_gdi_object_test|win32_coverage'
--output-on-failure`, the broader
`ctest --test-dir build-chrstride -R 'opus_win32_(gdi_object|gdi_raster|font|print|user32|memory)_test|win32_coverage'
--output-on-failure` sweep, searches proving `CreateDCA` and `CreateICA` are gone
from `uncovered.txt`, and `git diff --check`.

Reviewed by agy and claude: agy could not start because its TTY UI failed to
open `/dev/tty`; claude timed out without output.

## Add user32 hook pass-through

`src/port/win32/user32.c` now implements `CallNextHookEx` as a no-chain
pass-through returning zero. The port does not maintain native hook chains, and the
playback hook caller already uses this only for unhandled hook codes.

`opus-win32-user32-test` now covers null and non-null hook handles. The implemented
symbol was removed from `docs/win32-shim/uncovered.txt`.

Validated with `cmake --build build-chrstride --target opus_win32_user32_test -j2`,
`ctest --test-dir build-chrstride -R 'opus_win32_user32_test|win32_coverage'
--output-on-failure`, the broader
`ctest --test-dir build-chrstride -R 'opus_win32_(gdi_object|gdi_raster|font|print|user32|memory)_test|win32_coverage'
--output-on-failure` sweep, a search proving `CallNextHookEx` is gone from
`uncovered.txt`, and `git diff --check`.

Reviewed by agy and claude: agy could not start because its TTY UI failed to
open `/dev/tty`; claude timed out without output.

## Add user32 system menu state

`src/port/win32/user32.c` now creates a per-window system menu for
`GetSystemMenu`. Repeated calls return the same menu, `revert` clears the current
system menu, and window/menu destruction invalidates the stored handle.

`opus-win32-user32-test` now covers stable system-menu handles, the standard close
command, invalid-window behavior, revert, and recreation. The implemented symbol was
removed from `docs/win32-shim/uncovered.txt`.

Validated with `cmake --build build-chrstride --target opus_win32_user32_test -j2`,
`ctest --test-dir build-chrstride -R 'opus_win32_user32_test|win32_coverage'
--output-on-failure`, the broader
`ctest --test-dir build-chrstride -R 'opus_win32_(gdi_object|gdi_raster|font|print|user32|memory)_test|win32_coverage'
--output-on-failure` sweep, a search proving `GetSystemMenu` is gone from
`uncovered.txt`, and `git diff --check`.

Reviewed by agy and claude: agy could not start because its TTY UI failed to
open `/dev/tty`; claude timed out without output.

## Add user32 scroll state

`src/port/win32/user32.c` now stores horizontal and vertical scroll ranges and
positions per window. `SetScrollRange` records the range, and `SetScrollPos` returns
the previous position while clamping and storing the new one. New windows start with
the conventional `0..100` range.

`opus-win32-user32-test` now covers independent horizontal and vertical scroll
positions, default range behavior, previous-value returns, range clamping,
invalid-window behavior, and invalid bar handling. The implemented symbols were removed from
`docs/win32-shim/uncovered.txt`.

Validated with `cmake --build build-chrstride --target opus_win32_user32_test -j2`,
`ctest --test-dir build-chrstride -R 'opus_win32_user32_test|win32_coverage'
--output-on-failure`, the broader
`ctest --test-dir build-chrstride -R 'opus_win32_(gdi_object|gdi_raster|font|print|user32|memory)_test|win32_coverage'
--output-on-failure` sweep, searches proving the implemented scroll names are gone
from `uncovered.txt`, and `git diff --check`.

Reviewed by agy and claude: agy could not start because its TTY UI failed to
open `/dev/tty`; claude timed out without output.

## Add user32 paint invalidation

`src/port/win32/user32.c` now implements `BeginPaint`, `EndPaint`,
`InvalidateRect`, and `RedrawWindow`. `BeginPaint` returns a window DC and fills
`PAINTSTRUCT` from the client rectangle, `EndPaint` releases that DC, `InvalidateRect`
queues `WM_PAINT`, and `RedrawWindow` can dispatch immediate paint messages for the
target and direct children.

`opus-win32-user32-test` now covers paint struct fields, DC release, queued
invalidation, and immediate redraw dispatch. The implemented symbols were removed
from `docs/win32-shim/uncovered.txt`.

Validated with `cmake --build build-chrstride --target opus_win32_user32_test -j2`,
`ctest --test-dir build-chrstride -R 'opus_win32_user32_test|win32_coverage'
--output-on-failure`, the broader
`ctest --test-dir build-chrstride -R 'opus_win32_(gdi_object|gdi_raster|font|print|user32|memory)_test|win32_coverage'
--output-on-failure` sweep, searches proving the implemented paint names are gone
from `uncovered.txt`, and `git diff --check`.

Reviewed by agy and claude: agy could not start because its TTY UI failed to
open `/dev/tty`; claude timed out without output.

## Add user32 clipboard state

`src/port/win32/user32.c` now keeps a process-local clipboard registry and data
table for `RegisterClipboardFormatA`, open/close, empty, owner lookup, availability,
and handle get/set. The table preserves caller-provided handles and clears entries
without freeing them.

`opus-win32-user32-test` now covers registered format reuse, open-state failures,
owner tracking, `CF_TEXT` storage, custom format storage, availability, and emptying.
The implemented symbols were removed from `docs/win32-shim/uncovered.txt`.

Validated with `cmake --build build-chrstride --target opus_win32_user32_test -j2`,
`ctest --test-dir build-chrstride -R 'opus_win32_user32_test|win32_coverage'
--output-on-failure`, the broader
`ctest --test-dir build-chrstride -R 'opus_win32_(gdi_object|gdi_raster|font|print|user32|memory)_test|win32_coverage'
--output-on-failure` sweep, searches proving the implemented clipboard names are gone
from `uncovered.txt`, and `git diff --check`.

Reviewed by agy and claude: agy could not start because its TTY UI failed to
open `/dev/tty`; claude timed out without output.

## Add gdi32 edge drawing

`src/port/win32/gdi32.c` now implements `DrawEdge` for `EDGE_RAISED` and
`EDGE_SUNKEN` with the declared border side flags. It reuses `FillRect` for clipped
one-pixel spans instead of adding a separate raster path.

`opus-win32-gdi-raster-test` now covers raised rectangle edges and sunken partial
edges. The implemented symbol was removed from `docs/win32-shim/uncovered.txt`.

Validated with `cmake --build build-chrstride --target opus_win32_gdi_raster_test -j2`,
`ctest --test-dir build-chrstride -R 'opus_win32_gdi_raster_test|win32_coverage'
--output-on-failure`, the broader
`ctest --test-dir build-chrstride -R 'opus_win32_(gdi_object|gdi_raster|font|print|user32|memory)_test|win32_coverage'
--output-on-failure` sweep, a search proving `DrawEdge` is gone from `uncovered.txt`,
and `git diff --check`.

Reviewed by agy and claude: agy could not start because its TTY UI failed to
open `/dev/tty`; claude timed out without output.

## Add user32 system color brushes

`src/port/win32/user32.c` now implements `GetSysColorBrush` by caching brushes backed
by the existing `GetSysColor` palette. `opus-win32-user32-test` verifies the menu
system brush is stable and fills a bitmap with the matching system color.

The implemented symbol was removed from `docs/win32-shim/uncovered.txt`.

Validated with `cmake --build build-chrstride --target opus_win32_user32_test -j2`,
`ctest --test-dir build-chrstride -R 'opus_win32_user32_test|win32_coverage'
--output-on-failure`, the broader
`ctest --test-dir build-chrstride -R 'opus_win32_(gdi_object|gdi_raster|font|print|user32|memory)_test|win32_coverage'
--output-on-failure` sweep, a search proving `GetSysColorBrush` is gone from
`uncovered.txt`, and `git diff --check`.

Reviewed by agy and claude: agy could not start because its TTY UI failed to
open `/dev/tty`; claude timed out without output.

## Add gdi32 rectangular clip state

`src/port/win32/gdi32.c` now creates rectangular region handles with `CreateRectRgn`
and applies `IntersectClipRect` by narrowing the existing DC clip rectangle. Existing
raster paths already honor that clip, so no extra drawing code was needed.

`opus-win32-gdi-raster-test` now covers region handle deletion and clipped `PatBlt`
behavior for both simple and empty clip intersections. The implemented symbols were
removed from `docs/win32-shim/uncovered.txt`.

Validated with `cmake --build build-chrstride --target opus_win32_gdi_raster_test -j2`,
`ctest --test-dir build-chrstride -R 'opus_win32_gdi_raster_test|win32_coverage'
--output-on-failure`, the broader
`ctest --test-dir build-chrstride -R 'opus_win32_(gdi_object|gdi_raster|font|print|user32|memory)_test|win32_coverage'
--output-on-failure` sweep, searches proving the implemented names are gone from
`uncovered.txt`, and `git diff --check`.

Reviewed by agy and claude: agy could not start because its TTY UI failed to
open `/dev/tty`; claude timed out without output.

## Add gdi32 rectangle primitives

`src/port/win32/gdi32.c` now implements `FillRect`, `FrameRect`, and `Rectangle` on the
selected bitmap. `FillRect` uses the supplied brush, `FrameRect` draws a one-pixel
brush border, and `Rectangle` fills with the selected brush and draws the outline with
the selected pen, all through the existing clip and pixel helpers.

`opus-win32-gdi-raster-test` now covers filled rectangles, framed rectangles, and
selected-pen rectangle borders. The implemented symbols were removed from
`docs/win32-shim/uncovered.txt`.

Validated with `cmake --build build-chrstride --target opus_win32_gdi_raster_test -j2`,
`ctest --test-dir build-chrstride -R 'opus_win32_gdi_raster_test|win32_coverage'
--output-on-failure`, the broader
`ctest --test-dir build-chrstride -R 'opus_win32_(gdi_object|gdi_raster|font|print|user32|memory)_test|win32_coverage'
--output-on-failure` sweep, searches proving the implemented names are gone from
`uncovered.txt`, and `git diff --check`.

Reviewed by agy and claude: agy could not start because its TTY UI failed to
open `/dev/tty`; claude hung without findings and was interrupted.

## Add gdi32 DC state and line drawing

`src/port/win32/gdi32.c` now keeps DC background mode, text color, map mode,
viewport/window origin and extent, and current pen position. It implements the declared
gdi32 state APIs for those fields and draws simple clipped one-pixel `LineTo` strokes
with the selected pen color.

`opus-win32-gdi-raster-test` now covers previous-value returns for text/background/map
state, viewport/window state accessors, `MoveToEx`, horizontal and vertical `LineTo`,
and selected-pen color. The implemented symbols were removed from
`docs/win32-shim/uncovered.txt`.

Validated with `cmake --build build-chrstride --target opus_win32_gdi_raster_test -j2`,
`ctest --test-dir build-chrstride -R 'opus_win32_gdi_raster_test|win32_coverage'
--output-on-failure`, the broader
`ctest --test-dir build-chrstride -R 'opus_win32_(gdi_object|gdi_raster|font|print|user32|memory)_test|win32_coverage'
--output-on-failure` sweep after building the missing executables, searches proving the
implemented names are gone from `uncovered.txt`, and `git diff --check`.

Reviewed by agy and claude: agy could not start because its TTY UI failed to
open `/dev/tty`; claude hung without findings and was interrupted.

## Import Linux reconnaissance notes

`docs/win32-shim/linux-reconnaissance.md` now condenses
`DONT-MERGE/jphonorato/docs/port-linux/00-reconocimiento.md` into English for the
Win32-on-SDL shim. It keeps the source-backed findings from sections 1, 4, and 5, plus
the LP64 audit inventory and the 182-error triage where those make the findings
actionable.

The import marks Wine-specific build mechanics as historical evidence, not current
SDL-shim instructions. It also corrects a stale source claim: `qwindows.h` was claimed
reachable through `sym.c`, but the current `OPUS_X64` tree selects
`opus_x64_compat.h` instead, so the LP64 audit should target serialized `long` and
shim Win32 type widths rather than `qwindows.h`.

Validated with searches for the required source-section references, LP64 audit claims,
the 182-error triage, `BITAPP`, `KME`, `exp.c`, `qwindows`, Wine-only mechanics marked
historical, a filename check proving no new underscore filenames, a label-text search,
and `git diff --check`.

Reviewed by agy and claude: agy could not start because its TTY UI failed to
open `/dev/tty`; claude hung without findings and was interrupted.

## Import jphonorato's probes

`docs/win32-shim/font-probes/` now contains translated Win32 oracle probes for
integer glyph advances, synthetic bold and italic metrics, and startup font
substitution. The README documents the Wine/Windows commands for recapturing font
metrics while marking them as oracle-capture commands, not SDL-shim run instructions.

`docs/win32-shim/handle-check/` now contains a translated and shim-adapted
`GlobalHandle(ptr)` probe. It calls the current C shim's `GlobalAlloc`, `GlobalLock`,
`GlobalUnlock`, `GlobalHandle`, and `GlobalFree` path directly instead of importing the
discarded Qt shell's `OpusShellMemory` API.

The Qt comparison probes and table generator were left behind because they feed
`src/core` artifacts from the rejected Qt-shell architecture, not this Win32-on-SDL
shim.

Validated with C11 syntax checks for the font probes, a compiled and passing
`handle-check` run against `src/port/win32/kernel32.c`, filename checks proving no new
underscore filenames, a search proving no banned label text was added, and
`git diff --check`.

Reviewed by agy and claude: agy could not start because its TTY UI failed to
open `/dev/tty`; claude hung without findings and was interrupted.

## Preserve startup heap diagnosis

`docs/win32-shim/startup-debugging.md` now condenses the Spanish Fedora startup
heap-corruption notes into English. It keeps the actionable findings: the Fedora-only
reproduction boundary, ASan and Valgrind dead ends, limited value of glibc malloc
checking, `WINEDEBUG=+heap` as the useful historical Winelib trace path, offline
`addr2line` resolving `WORD1+0x1FD57C` to `N_FormatLineDxa`, and lowercase generated
paths for breakpoints.

The document marks Wine commands as historical because the SDL shim will not run under
Wine, and it links each retained claim back to the source Spanish note and section.

Validated with searches for `ASan`, `Valgrind`, `WINEDEBUG=+heap`, `addr2line`,
`N_FormatLineDxa`, `lowercase`, `historical`, `SDL shim`, and both source note
filenames in `docs/win32-shim/startup-debugging.md`, plus `git diff --check`.

Reviewed by agy and claude: agy could not start because its TTY UI failed to
open `/dev/tty`; claude hung without findings and was interrupted.

## Keep release packaging deferred

Release packaging remains deliberately deferred until macOS, Linux, and WebAssembly
are green. No packaging task or tracked CPack/WiX/MSI configuration was added before
those platform items are complete.

Validated with searches for `MSI`, `WiX`, `installer`, `package`, `packaging`,
`CPack`, and `cpack` across the tracked project areas. The only WiX packaging tree
found is the rejected reference under `DONT-MERGE/bahree/packaging/wix`; items 16, 17,
and 18 remain in `TODO.md`.

Reviewed by agy and claude: agy could not start because its TTY UI failed to
open `/dev/tty`; claude hung without findings and was interrupted.

## Refresh README build and test commands

`README.md` no longer describes the project as Windows-only, no longer lists
PowerShell as a general requirement, and uses shell-neutral commands for the current
CMake presets. It documents the Windows `x64-debug` and `x64-release` presets, the
non-Windows `macos-debug`, `linux-debug`, and `wasm-debug` porting presets, and the
`-LE ui` test form for headless runs because the UI-labelled tests need a display.

Validated with `rg -i powershell README.md`, searches for the current preset names
and `-LE ui`, `cmake --list-presets -S src`, and `git diff --check`.

Reviewed by agy and claude: agy could not start because its TTY UI failed to
open `/dev/tty`; claude hung without findings and was interrupted.

## State license status by source area

`README.md` now separates the license status for the original Microsoft source
trees, `src/port/**`, build-system files, and `docs/win32-shim/**`. It keeps the
top-level no-license warning, avoids copying jphonorato's MIT license, and points the
win32-shim attribution to `docs/win32-shim/README.md`.

Validated with source searches for `src/port/**`, build-system files,
`docs/win32-shim/**`, original Microsoft source trees, `MIT-licensed Win32Emu`,
and the absence of a top-level `LICENSE` file.

Reviewed by agy and claude: agy could not start because its TTY UI failed to
open `/dev/tty`; claude hung without findings and was interrupted.

## Fix CLIPBRD2 CHR stride

`src/Opus/CLIPBRD2.C` now advances the `CHR` scan with
`CbFromChrm(pchr->chrm)` instead of treating `chrm` as a byte count. This matches
the other live `CHR` walkers in `disp1.c`, `format.c`, and `select.c`, where
`chrm` is interpreted through the variant-size helper.

Validated with `cmake --build build-chrstride --target opus_original_engine opus_x64_runtime_test opus_original_plc_test opus_original_sttb_test opus_original_strtbl_test --parallel 8`,
`ctest --test-dir build-chrstride -R 'opus_x64_runtime_test|opus_original_plc_test|opus_original_sttb_test|opus_original_strtbl_test' --output-on-failure`,
a source search showing no remaining `+ pchr->chrm` stride and the corrected
`CbFromChrm(pchr->chrm)` site, and `git diff --check`.

Reviewed by agy and claude: agy could not start because its TTY UI failed to
open `/dev/tty`; claude hung without findings and was interrupted.

## Add build-output ignore rules

The repository now has a top-level `.gitignore` for build products and generated
resource outputs: `bin/`, `build/`, `build-*`, `out/`, CMake scratch files, and
`*.res`. It does not ignore ordinary source or documentation paths.

Validated with `git status --ignored -s` and `git check-ignore -v` on disposable
probe files under `bin/`, `build/`, `build-cmake-probe/`, `out/`, and a generated
`.res` file. `git check-ignore -v` returned no match for a disposable ordinary source
file, `docs/win32-shim/README.md`, or `src/port/win32/user32.c`.

Reviewed by agy and claude: agy could not start because its TTY UI failed to
open `/dev/tty`; claude hung without findings and was interrupted.

## Add trace fallback for runtime targets

`OpusX64TraceRibbon` now has one standalone fallback definition in
`src/port/original/opustracestub.c`, and `opus_x64_runtime` includes that
archive member directly. `opus_sdm_runtime.cpp` now only declares the function,
so the fallback member contains no unrelated SDM symbols.

Validated with `cmake --build build-trace-stub --target opus_x64_runtime_test WORD1 --parallel 8`,
`ctest --test-dir build-trace-stub -R opus_x64_runtime_test --output-on-failure`,
`nm` on `build/lib/Debug/libopus_x64_runtime.a` showing the single definition in
`opustracestub.c.o` and only an undefined reference from `opus_sdm_runtime.cpp.o`,
`nm` on `bin/WORD1` showing exactly one final `OpusX64TraceRibbon`,
`commentflow src/port/original/opustracestub.c src/port/original/opus_sdm_runtime.cpp`,
and `git diff --check`.

Reviewed by agy and claude: agy could not start because its TTY UI failed to
open `/dev/tty`; claude hung without findings and was interrupted.

## Add GDI DC and object state

`src/port/win32/gdi32.cpp` now owns the first GDI layer: `HDC`, `HBITMAP`, `HPEN`,
`HBRUSH`, and `HFONT` handles; stock objects; type-aware `SelectObject`;
`GetCurrentObject`; `SaveDC`/`RestoreDC`; object deletion rules; bitmap creation and
resource bitmap bridging. Resource-loaded bitmaps now enter the same GDI object table as
created bitmaps, so later raster work has one handle model.

`opus_win32_gdi_object_test` covers default DC selections, pen/brush/font/bitmap
selection return values, save/restore state, selected-object delete rejection, bitmap
metadata, stock-object delete rejection, and DC teardown. The small `CharUpperBuffA`,
`GetStringTypeA`, and `MulDiv` kernel helpers were added because `opus_x64_runtime_test`
reaches them before the later user32 work.

Validated with `ctest --test-dir build-item13 -R 'strtbl|sttb|plc|sdm_cab|command|win32_memory|opus_win32_gdi_object_test|win32_coverage' --output-on-failure`.
`opus_x64_runtime_test` now links on macOS, then stops at the next user32 scope:
`FInitSdm_sdm21` calls `GetSystemMetrics` and `GetSysColor`, which remain item 14 work.

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

## Add GDI raster operations

The non-Windows gdi32 shim now rasterizes `PatBlt`, `BitBlt`, `StretchBlt`,
`SetPixel`, `GetPixel`, `GetDIBits`, `SetStretchBltMode`, and `SetROP2` against
the selected bitmap in a memory DC. The implementation covers the raster ops
Word needs for toolbar, caret, and selection drawing, including `BLACKNESS`,
`WHITENESS`, `PATCOPY`, `PATINVERT`, `DSTINVERT`, `SRCCOPY`, `NOTSRCCOPY`,
`SRCAND`, `SRCINVERT`, and the documented pattern/source ROPs from the reference
notes. Resource 201's toolbar bitmap now returns one cached bitmap handle so
`LoadImageW(201)` and `LoadBitmapA(201)` agree.

`opus_win32_gdi_raster_test` exercises solid and patterned destination writes,
source blits, nearest-neighbor stretching, clipping from negative origins, DIB
row reads, and DC mode state returns. The Win32 coverage baseline no longer
lists the covered raster entry points.

Validated with `cmake -S src -B build-item13b -DCMAKE_C_FLAGS=-std=gnu89`,
`cmake --build build-item13b --target opus_original_engine opus_x64_runtime
opus_original_strtbl_test opus_original_sttb_test opus_original_plc_test
opus_sdm_cab_test opus_original_command_test opus_win32_memory_test
opus_win32_resource_test opus_win32_gdi_object_test opus_win32_gdi_raster_test
--parallel 8`, and `ctest --test-dir build-item13b -R
'strtbl|sttb|plc|sdm_cab|command|win32_memory|opus_win32_resource_test|opus_win32_gdi_(object|raster)_test|win32_coverage'
--output-on-failure`, which passed 10/10. `opus_x64_runtime_test` remains behind
item 14 because startup SDM still reaches missing user32 metrics/color APIs.

Reviewed by agy and claude: both kept this subtask to software raster operations
inside the gdi32 shim, with text metrics and user32 left for their own items.

## Add GDI text metrics

The non-Windows gdi32 shim now supplies deterministic font metrics for the
startup faces `Tms Rmn`, `Helv`, `Courier`, and `Symbol`. `CreateFontIndirectA/W`
normalizes those names through the substitution table, selected fonts feed
`GetTextMetricsA/W`, `GetTextExtentPoint32A`, and `GetCharWidthA`, and
`EnumFontsA`/`EnumFontFamiliesExA` enumerate the same four faces without holding
the GDI lock across callbacks. `GetDeviceCaps` now returns the live screen,
bitmap, raster, text, and 96-DPI caps Word reads while building font caches.

The metric data is a small built-in design-unit table, not captured Windows GDI
oracle data. It locks additivity and pitch behavior for this shim and leaves
exact Windows pagination fidelity as a data replacement task when oracle output
is available.

`opus_win32_font_test` covers whole-string versus per-character extent
additivity, `GetCharWidthA`, metric height/internal-leading coherence, zero
overhang, fixed versus variable pitch bits, font enumeration including early
callback stop, filtered `EnumFontFamiliesExA`, and the live device caps. The
Win32 coverage baseline no longer lists `EnumFontFamiliesExA`, `GetDeviceCaps`,
`GetTextExtentPoint32A`, or `GetTextMetricsA`.

Validated with `cmake -S src -B build-item13c -DCMAKE_C_FLAGS=-std=gnu89`,
`cmake --build build-item13c --target opus_original_strtbl_test
opus_original_sttb_test opus_original_plc_test opus_sdm_cab_test
opus_original_command_test opus_win32_memory_test opus_win32_resource_test
opus_win32_gdi_object_test opus_win32_gdi_raster_test opus_win32_font_test
--parallel 8`, and `ctest --test-dir build-item13c -R
'strtbl|sttb|plc|sdm_cab|command|win32_memory|opus_win32_resource_test|opus_win32_gdi_(object|raster)_test|opus_win32_font_test|win32_coverage'
--output-on-failure`, which passed 11/11. `opus_x64_runtime_test` still builds
but segfaults through a null call; unresolved imports are now user32 startup
surface only: `CreateWindowExA`, `RegisterClassExA`, `GetDC`, `ReleaseDC`,
`GetSystemMetrics`, and `GetSysColor`.

Reviewed by agy and claude: agy confirmed the minimal text-metrics surface and
font substitution table; claude caught the hidden `GetDeviceCaps` and
`GetCharWidthA` prerequisites and the reentrant `EnumFontsA` callback trap.

## Add GDI print escape failure

The non-Windows gdi32 shim now owns Word's live print surface: legacy
`Escape`. The shim reports unsupported escapes through `OutputDebugStringA`,
returns `SP_ERROR` for `STARTDOC`, returns zero for unsupported capability and
driver-control escapes, and clears the `NEXTBAND` output rectangle before
returning zero so a stale band cannot spin the print loop. The modern
`StartDoc`/`EndDoc`/page APIs were left out because this tree does not call
them.

`opus_win32_print_test` covers the `STARTDOC` failure, unsupported
`QUERYESCSUPPORT`, unchanged page-size output, zeroed `NEXTBAND`, and harmless
zero returns for the abort/end/draft controls. The Win32 coverage baseline no
longer lists `Escape`.

Validated with `cmake -S src -B build-item13d -DCMAKE_C_FLAGS=-std=gnu89`,
`cmake --build build-item13d --target opus_win32_print_test
opus_original_engine opus_x64_runtime --parallel 8`, and `ctest --test-dir
build-item13d -R
'strtbl|sttb|plc|sdm_cab|command|win32_memory|opus_win32_resource_test|opus_win32_gdi_(object|raster)_test|opus_win32_font_test|opus_win32_print_test|win32_coverage'
--output-on-failure`, which passed 12/12.

`GETPHYSPAGESIZE` returning zero still leaves a printer-path landmine in
`SCREEN2.C`: that path can read an uninitialized page point once printer DC
creation exists. It is currently unreachable because printer DC creation is not
implemented.

Reviewed by agy and claude: agy outlined the predictable failure contract;
claude narrowed the implementation to `Escape`, preserved its legacy prototype,
and caught the `NEXTBAND` stale-output trap.

## Add user32 startup seed

The non-Windows runtime now links a first `user32.cpp` shim. It implements the
startup surface reached by the current SDM/runtime path: `RegisterClass*`,
`CreateWindowEx*`, `DestroyWindow`, window validity, parent/owner traversal,
window long slots, basic rect/position/visibility/enabled/focus state, screen DC
acquisition through gdi32, deterministic system metrics/colors, and simple
message queue stubs. `kernel32.cpp` also owns `GetCurrentProcessId`,
`GetEnvironmentVariableA`, and `SetEnvironmentVariableA`.

`opus_x64_runtime_test` no longer links with macOS `dynamic_lookup`, so missing
Win32 imports surface as link errors instead of null function calls. The previous
segfaulting runtime test now passes.

`opus_win32_user32_test` covers duplicate class registration, synchronous
`WM_NCCREATE`, user data and extra bytes, parent versus owner, child invalidation
through `DestroyWindow`, focus/enabled state, work-area metrics, rect adjustment,
and screen/window DC release.

Validated with `cmake --build build-item14a --target
opus_original_strtbl_test opus_original_sttb_test opus_original_plc_test
opus_sdm_cab_test opus_original_command_test opus_win32_memory_test
opus_win32_resource_test opus_win32_gdi_object_test opus_win32_gdi_raster_test
opus_win32_font_test opus_win32_print_test opus_win32_user32_test
opus_x64_runtime_test opus_original_engine opus_x64_runtime --parallel 8`,
then `ctest --test-dir build-item14a -R
'strtbl|sttb|plc|sdm_cab|command|opus_x64_runtime_test|win32_memory|opus_win32_resource_test|opus_win32_gdi_(object|raster)_test|opus_win32_font_test|opus_win32_print_test|opus_win32_user32_test|win32_coverage'
--output-on-failure`, which passed 14/14.

Reviewed by agy and claude: agy identified the minimal startup API set, and
claude caught that `dynamic_lookup` was hiding the real link gate and narrowed the
message pump to no-work stubs for this runtime path.

## Add user32 message core

The non-Windows `user32.cpp` shim now owns the in-memory message core:
`SendMessageA/W` dispatch synchronously to the window procedure, `PostMessageA/W`
queue messages, `PeekMessageA` honors filter and remove flags, `GetMessageA`
returns queued messages and only quits on `WM_QUIT`, and `WaitMessage` fails fast
until an event backend exists. `DefWindowProcA/W` now defaults to zero, keeps
`WM_NCCREATE` successful, handles `WM_CLOSE`, and owns basic text
get/set/length messages.

`windows.h` now routes macro spellings for `SendMessage`, `GetMessage`,
`PostMessage`, and `DefWindowProc` through typed shim entry points with casts for
the historical C call sites. That makes macro-spelled engine calls visible to
`win32_coverage` without breaking old pointer/integer argument patterns.

`opus_win32_user32_test` now covers synchronous send dispatch, default return,
peek no-remove versus remove, hwnd/message filtering, `PostQuitMessage`,
`GetMessageA` quit return, and queued-message purge through `DestroyWindow`.
The Win32 coverage baseline no longer lists `PeekMessageA`, `SendMessageA`, or
`SendMessageW`.

Validated with `cmake --build build-item14b --target
opus_original_strtbl_test opus_original_sttb_test opus_original_plc_test
opus_sdm_cab_test opus_original_command_test opus_win32_memory_test
opus_win32_resource_test opus_win32_gdi_object_test opus_win32_gdi_raster_test
opus_win32_font_test opus_win32_print_test opus_win32_user32_test
opus_x64_runtime_test opus_original_engine opus_x64_runtime --parallel 8`,
then `ctest --test-dir build-item14b -R
'strtbl|sttb|plc|sdm_cab|command|opus_x64_runtime_test|win32_memory|opus_win32_resource_test|opus_win32_gdi_(object|raster)_test|opus_win32_font_test|opus_win32_print_test|opus_win32_user32_test|win32_coverage'
--output-on-failure`, which passed 14/14.

Reviewed by agy and claude: both prioritized the message core before SDL,
menus, caret, clipboard, or scrollbars. The `GetWindowWord`/`SetWindowWord`
surface remains out of this slice because its 16-bit extra-byte model needs a
separate pointer-width audit.

## Add user32 text and extra accessors

The non-Windows `user32.cpp` shim now handles byte-width extra-window access
instead of treating every positive offset as a pointer-sized slot.
`GetWindowLongA`/`SetWindowLongA` use four bytes, `GetWindowWord`/`SetWindowWord`
use two bytes, and pointer-sized `GetWindowLongPtrA`/`SetWindowLongPtrA` keep their
existing eight-byte behavior. `GetTopWindow` is declared and implemented because
the original engine calls it before passing the result into typed window helpers.

Window text is now available through `SetWindowTextW`, `GetWindowTextA/W`, and
`GetWindowTextLengthA/W`, using the same stored caption string already owned by
`SetWindowTextA` and `DefWindowProc`'s `WM_GETTEXT`/`WM_SETTEXT` handling. The
Win32 coverage baseline no longer lists `GetWindowTextA`,
`GetWindowTextLengthA`, `GetWindowTextLengthW`, `GetWindowTextW`, or
`SetWindowTextW`.

`opus_win32_user32_test` covers 32-bit and 16-bit extra-byte writes sharing the
same byte array without clobbering adjacent slots, narrow and wide window text
round trips, and `GetTopWindow` over a child window.

Validated with `cmake --build build-item14c --target
opus_win32_user32_test opus_x64_runtime_test opus_original_engine --parallel 8`
and `ctest --test-dir build-item14c -R
'opus_win32_user32_test|opus_x64_runtime_test|win32_coverage'
--output-on-failure`, which passed 3/3. Also validated with `cmake --build
build-item14c --target opus_original_strtbl_test opus_original_sttb_test
opus_original_plc_test opus_sdm_cab_test opus_original_command_test
opus_win32_memory_test opus_win32_resource_test opus_win32_gdi_object_test
opus_win32_gdi_raster_test opus_win32_font_test opus_win32_print_test
opus_win32_user32_test opus_x64_runtime_test opus_original_engine
opus_x64_runtime --parallel 8`, then `ctest --test-dir build-item14c -R
'strtbl|sttb|plc|sdm_cab|command|opus_x64_runtime_test|win32_memory|opus_win32_resource_test|opus_win32_gdi_(object|raster)_test|opus_win32_font_test|opus_win32_print_test|opus_win32_user32_test|win32_coverage'
--output-on-failure`, which passed 14/14.

Reviewed by agy for this slice; claude was asked and returned after implementation
was underway with guidance for the next input-message slice and a warning that the
final WORD1 link gate is still masked by macOS `dynamic_lookup`.

## Add user32 key translation core

The non-Windows `user32.cpp` shim now tracks key state for posted
`WM_KEYDOWN`/`WM_KEYUP` and `WM_SYSKEYDOWN`/`WM_SYSKEYUP` messages, exposes
`GetKeyState`, and accepts `SetKeyboardState`. `TranslateMessage` now synthesizes
printable `WM_CHAR` or `WM_SYSCHAR` messages for letters, digits, space, return,
and tab, inserting the generated character ahead of later queued input so the next
peek/get sees it.

`opus_win32_user32_test` covers shift key state, posted keydown state updates,
`TranslateMessage` producing a shifted `WM_CHAR`, dispatch through the test window
procedure, and keyup clearing the down bit. The Win32 coverage baseline no longer
lists `GetKeyState` or `SetKeyboardState`.

Validated with `cmake --build build-item14d --target
opus_win32_user32_test opus_x64_runtime_test opus_original_engine --parallel 8`
and `ctest --test-dir build-item14d -R
'opus_win32_user32_test|opus_x64_runtime_test|win32_coverage'
--output-on-failure`, which passed 3/3. Also validated with `cmake --build
build-item14d --target opus_original_strtbl_test opus_original_sttb_test
opus_original_plc_test opus_sdm_cab_test opus_original_command_test
opus_win32_memory_test opus_win32_resource_test opus_win32_gdi_object_test
opus_win32_gdi_raster_test opus_win32_font_test opus_win32_print_test
opus_win32_user32_test opus_x64_runtime_test opus_original_engine
opus_x64_runtime --parallel 8`, then `ctest --test-dir build-item14d -R
'strtbl|sttb|plc|sdm_cab|command|opus_x64_runtime_test|win32_memory|opus_win32_resource_test|opus_win32_gdi_(object|raster)_test|opus_win32_font_test|opus_win32_print_test|opus_win32_user32_test|win32_coverage'
--output-on-failure`, which passed 14/14.

Reviewed by agy and claude before implementation: agy recommended the key
translation slice; claude returned with client geometry as the next preferred slice
and confirmed the input core as the following bounded step.

## Add user32 client geometry

The non-Windows `user32.cpp` shim now owns the declared client-geometry surface:
`GetClientRect`, child-aware `GetWindowRect`, `ClientToScreen`,
`ScreenToClient`, `MoveWindow`, and ancestor-aware `IsWindowVisible`. Window
creation now seeds visibility from `WS_VISIBLE`, and `ShowWindow`/`SetWindowPos`
keep the visibility bit and stored state in sync.

The same slice implements the pure rect helpers already declared in the shim:
`IntersectRect`, `OffsetRect`, and `PtInRect`. Empty intersections zero the
destination and return `FALSE`, and point containment uses the Win32 exclusive
right/bottom edge convention.

`opus_win32_user32_test` covers child client size, child-to-screen conversion,
round-trip coordinate conversion, point inclusion edges, non-empty and empty
intersection, offsetting, parent-hidden visibility inheritance, `MoveWindow`, and
the updated child window screen rect. The Win32 coverage baseline no longer lists
`ClientToScreen`, `GetClientRect`, `IntersectRect`, `IsWindowVisible`,
`MoveWindow`, `OffsetRect`, `PtInRect`, or `ScreenToClient`.

Validated with `cmake --build build-item14e --target
opus_win32_user32_test opus_x64_runtime_test opus_original_engine --parallel 8`
and `ctest --test-dir build-item14e -R
'opus_win32_user32_test|opus_x64_runtime_test|win32_coverage'
--output-on-failure`, which passed 3/3. Also validated with `cmake --build
build-item14e --target opus_original_strtbl_test opus_original_sttb_test
opus_original_plc_test opus_sdm_cab_test opus_original_command_test
opus_win32_memory_test opus_win32_resource_test opus_win32_gdi_object_test
opus_win32_gdi_raster_test opus_win32_font_test opus_win32_print_test
opus_win32_user32_test opus_x64_runtime_test opus_original_engine
opus_x64_runtime --parallel 8`, then `ctest --test-dir build-item14e -R
'strtbl|sttb|plc|sdm_cab|command|opus_x64_runtime_test|win32_memory|opus_win32_resource_test|opus_win32_gdi_(object|raster)_test|opus_win32_font_test|opus_win32_print_test|opus_win32_user32_test|win32_coverage'
--output-on-failure`, which passed 14/14.

Reviewed by agy before implementation; claude was asked again but had not
returned before implementation, so this slice used claude's prior geometry review
for the child-coordinate conversion, visibility inheritance, and hidden
undeclared rect-helper traps.

## Add user32 pure rect helpers

The non-Windows user32 shim now declares and implements the pure rect helpers that
the engine calls but the coverage gate could not previously see: `SetRect`,
`InflateRect`, and `UnionRect`. `UnionRect` ignores empty inputs, zeroes the
destination and returns `FALSE` when both inputs are empty, and tolerates aliased
destination/source use through local emptiness checks.

`opus_win32_user32_test` extends the existing geometry block with set, inflate,
non-empty union, union with one empty input, and all-empty union assertions. No
coverage baseline edit was needed because the declarations and implementations
land in the same commit.

Validated with `cmake --build build-item14f --target
opus_win32_user32_test opus_x64_runtime_test opus_original_engine --parallel 8`
and `ctest --test-dir build-item14f -R
'opus_win32_user32_test|opus_x64_runtime_test|win32_coverage'
--output-on-failure`, which passed 3/3. Also validated with `cmake --build
build-item14f --target opus_original_strtbl_test opus_original_sttb_test
opus_original_plc_test opus_sdm_cab_test opus_original_command_test
opus_win32_memory_test opus_win32_resource_test opus_win32_gdi_object_test
opus_win32_gdi_raster_test opus_win32_font_test opus_win32_print_test
opus_win32_user32_test opus_x64_runtime_test opus_original_engine
opus_x64_runtime --parallel 8`, then `ctest --test-dir build-item14f -R
'strtbl|sttb|plc|sdm_cab|command|opus_x64_runtime_test|win32_memory|opus_win32_resource_test|opus_win32_gdi_(object|raster)_test|opus_win32_font_test|opus_win32_print_test|opus_win32_user32_test|win32_coverage'
--output-on-failure`, which passed 14/14.

Reviewed by agy before implementation; claude was asked for this slice but had not
returned before implementation, so the prechecks followed claude's prior warning
about declaring and defining hidden rect helpers in the same commit.

## Add user32 capture and cursor state

The non-Windows user32 shim now owns process-local mouse capture, current cursor,
and screen cursor position state. `SetCapture`, `GetCapture`, `ReleaseCapture`,
`SetCursor`, `SetCursorPos`, and `GetCursorPos` are implemented, and destroying a
captured window clears capture.

`opus_win32_user32_test` covers capture previous-window returns, capture handoff,
release, cursor-handle previous returns, cursor-position round trip, null
`GetCursorPos`, and capture clearing when a child window is destroyed. The Win32
coverage baseline no longer lists `GetCapture`, `GetCursorPos`, `ReleaseCapture`,
`SetCapture`, `SetCursor`, or `SetCursorPos`.

Validated with `cmake --build build-item14g --target
opus_win32_user32_test opus_x64_runtime_test opus_original_engine --parallel 8`
and `ctest --test-dir build-item14g -R
'opus_win32_user32_test|opus_x64_runtime_test|win32_coverage'
--output-on-failure`, which passed 3/3. Also validated with `cmake --build
build-item14g --target opus_original_strtbl_test opus_original_sttb_test
opus_original_plc_test opus_sdm_cab_test opus_original_command_test
opus_win32_memory_test opus_win32_resource_test opus_win32_gdi_object_test
opus_win32_gdi_raster_test opus_win32_font_test opus_win32_print_test
opus_win32_user32_test opus_x64_runtime_test opus_original_engine
opus_x64_runtime --parallel 8`, then `ctest --test-dir build-item14g -R
'strtbl|sttb|plc|sdm_cab|command|opus_x64_runtime_test|win32_memory|opus_win32_resource_test|opus_win32_gdi_(object|raster)_test|opus_win32_font_test|opus_win32_print_test|opus_win32_user32_test|win32_coverage'
--output-on-failure`, which passed 14/14.

Reviewed by agy before implementation; claude was asked for this capture/cursor
slice but had not returned before implementation.

## Add user32 hit testing

The non-Windows user32 shim now handles default hit testing. `DefWindowProcA/W`
return `HTCLIENT` for points inside the full window rectangle and `HTNOWHERE`
for misses, and `WindowFromPoint` walks the current window list from front to
back, asking candidates for `WM_NCHITTEST` while skipping hidden, disabled,
`HTNOWHERE`, and `HTTRANSPARENT` windows. `HTNOWHERE` is now declared in the shim
header, and the coverage baseline no longer lists `WindowFromPoint`.

`opus_win32_user32_test` covers default hit tests, child hit selection,
disabled-child fallback to the parent, top-level hits, misses, and the coverage
entry removal. This is deliberately not full non-client frame/caption hit
testing; the shim still returns client for any point inside the stored window
rect.

Validated with `cmake --build build-item14h --target
opus_win32_user32_test opus_x64_runtime_test opus_original_engine --parallel 8`
and `ctest --test-dir build-item14h -R
'opus_win32_user32_test|opus_x64_runtime_test|win32_coverage'
--output-on-failure`, which passed 3/3. Also validated with `cmake --build
build-item14h --target opus_original_strtbl_test opus_original_sttb_test
opus_original_plc_test opus_sdm_cab_test opus_original_command_test
opus_win32_memory_test opus_win32_resource_test opus_win32_gdi_object_test
opus_win32_gdi_raster_test opus_win32_font_test opus_win32_print_test
opus_win32_user32_test opus_x64_runtime_test opus_original_engine
opus_x64_runtime --parallel 8`, then `ctest --test-dir build-item14h -R
'strtbl|sttb|plc|sdm_cab|command|opus_x64_runtime_test|win32_memory|opus_win32_resource_test|opus_win32_gdi_(object|raster)_test|opus_win32_font_test|opus_win32_print_test|opus_win32_user32_test|win32_coverage'
--output-on-failure`, which passed 14/14.

Reviewed by agy before implementation; claude was asked for this exact slice but
had not returned before implementation.

## Add user32 scripted pump seam

The non-Windows user32 shim now has a minimal scripted input source for tests.
`OpusUser32PushScriptedInput` lives outside `windows.h` in `opusinputscript.h`,
so the coverage gate still tracks only declared Win32 surface. The pump moves at
most one scripted event at a time into the existing queue, updates key state when
that event becomes visible, lets `PeekMessageA` drain without sleeping, and lets
`GetMessageA`/`WaitMessage` keep the existing fail-fast abort when the script is
exhausted.

`PostQuitMessage` is now a quit flag plus exit code instead of a queued message.
`queue_take` synthesizes `WM_QUIT` after ordinary queued messages miss, leaves it
visible for `PM_NOREMOVE`, and consumes it only for `PM_REMOVE`.

`opus_win32_user32_test` covers the canonical
`GetMessageA`/`TranslateMessage`/`DispatchMessageA` loop over scripted
`WM_KEYDOWN` and `WM_KEYUP`, dispatch-posted quit, scripted `PeekMessageA`
drain, and repeated `PM_NOREMOVE` observation of `WM_QUIT` before removal.

Validated with `cmake -S src -B build-item14i -DCMAKE_BUILD_TYPE=Debug`,
`cmake --build build-item14i --target opus_win32_user32_test
opus_x64_runtime_test opus_original_engine --parallel 8`, and `ctest --test-dir
build-item14i -R
'opus_win32_user32_test|opus_x64_runtime_test|win32_coverage'
--output-on-failure`, which passed 3/3. Also validated with `cmake --build
build-item14i --target opus_original_strtbl_test opus_original_sttb_test
opus_original_plc_test opus_sdm_cab_test opus_original_command_test
opus_win32_memory_test opus_win32_resource_test opus_win32_gdi_object_test
opus_win32_gdi_raster_test opus_win32_font_test opus_win32_print_test
opus_win32_user32_test opus_x64_runtime_test opus_original_engine
opus_x64_runtime --parallel 8`, then `ctest --test-dir build-item14i -R
'strtbl|sttb|plc|sdm_cab|command|opus_x64_runtime_test|win32_memory|opus_win32_resource_test|opus_win32_gdi_(object|raster)_test|opus_win32_font_test|opus_win32_print_test|opus_win32_user32_test|win32_coverage'
--output-on-failure`, which passed 14/14.

The exact reviewer commands to agy and claude were attempted before
implementation, but both stalled without output and were stopped. This slice
follows agy's prior review that identified the blocking pump seam, scripted
input source, and quit-flag model as the next user32 step.

## Add user32 timer queue

The non-Windows user32 shim now implements `SetTimer` and `KillTimer` with a
process-local periodic timer list. The pump posts one due `WM_TIMER` at a time,
does not duplicate a timer message already queued for the same window/id, and
removes timers when their window is destroyed. `GetMessageA` and `WaitMessage`
can now sleep until the next timer deadline before preserving the existing
fail-fast abort for a missing backend.

`opus_win32_user32_test` covers timer creation, blocking `WaitMessage` delivery,
`PM_NOREMOVE` observation, `GetMessageA` removal, cancellation, and the
post-cancel no-delivery path. The Win32 coverage baseline no longer lists
`SetTimer` or `KillTimer`.

Validated with `cmake --build build-item14j --target
opus_win32_user32_test opus_x64_runtime_test opus_original_engine --parallel 8`
and `ctest --test-dir build-item14j -R
'opus_win32_user32_test|opus_x64_runtime_test|win32_coverage'
--output-on-failure`, which passed 3/3. Also validated with `cmake --build
build-item14j --target opus_original_strtbl_test opus_original_sttb_test
opus_original_plc_test opus_sdm_cab_test opus_original_command_test
opus_win32_memory_test opus_win32_resource_test opus_win32_gdi_object_test
opus_win32_gdi_raster_test opus_win32_font_test opus_win32_print_test
opus_win32_user32_test opus_x64_runtime_test opus_original_engine
opus_x64_runtime --parallel 8`, then `ctest --test-dir build-item14j -R
'strtbl|sttb|plc|sdm_cab|command|opus_x64_runtime_test|win32_memory|opus_win32_resource_test|opus_win32_gdi_(object|raster)_test|opus_win32_font_test|opus_win32_print_test|opus_win32_user32_test|win32_coverage'
--output-on-failure`, which passed 14/14.

The exact reviewer commands to agy and claude were attempted before
implementation, but both stalled without output and were stopped. This slice
follows agy's prior warning that missing `WM_TIMER` delivery belongs in the
user32 queue work because it can leave applications blocked in `GetMessage`.

## Add user32 async key and cursor visibility state

The non-Windows user32 shim now declares and implements `GetAsyncKeyState` and
`ShowCursor`, closing two previously hidden inventory rows. `GetAsyncKeyState`
returns the current shim key state used by `GetKeyState`, and mouse button
messages now update `VK_LBUTTON` and `VK_RBUTTON` in the same shared path.
`ShowCursor` maintains the Win32-style display counter, and `SM_CURSORLEVEL`
reports that counter.

`opus_win32_user32_test` covers cursor display count transitions, system metric
tracking, async key state for posted key down/up, invalid virtual-key handling,
and mouse button down/up state.

Validated with `cmake -S src -B build-item14k -DCMAKE_BUILD_TYPE=Debug`,
`cmake --build build-item14k --target opus_win32_user32_test
opus_x64_runtime_test opus_original_engine --parallel 8`, and `ctest --test-dir
build-item14k -R
'opus_win32_user32_test|opus_x64_runtime_test|win32_coverage'
--output-on-failure`, which passed 3/3. Also validated with `cmake --build
build-item14k --target opus_original_strtbl_test opus_original_sttb_test
opus_original_plc_test opus_sdm_cab_test opus_original_command_test
opus_win32_memory_test opus_win32_resource_test opus_win32_gdi_object_test
opus_win32_gdi_raster_test opus_win32_font_test opus_win32_print_test
opus_win32_user32_test opus_x64_runtime_test opus_original_engine
opus_x64_runtime --parallel 8`, then `ctest --test-dir build-item14k -R
'strtbl|sttb|plc|sdm_cab|command|opus_x64_runtime_test|win32_memory|opus_win32_resource_test|opus_win32_gdi_(object|raster)_test|opus_win32_font_test|opus_win32_print_test|opus_win32_user32_test|win32_coverage'
--output-on-failure`, which passed 14/14.

Reviewed by agy before implementation; claude was asked for this exact slice but
stalled without output and was stopped. The implementation includes agy's
mouse-button state trap.

## Add user32 window lookup helpers

The non-Windows user32 shim now implements the window-list helpers that can be
answered from existing in-memory state: `EnumWindows`, `EnumChildWindows`,
`EnumThreadWindows`, `FindWindowA`, `FindWindowExW`, `GetAncestor`,
`GetClassNameW`, `GetDlgCtrlID`, `BringWindowToTop`, `IsIconic`, `IsZoomed`,
`OpenIcon`, and `MessageBeep`. Enumeration snapshots handles before callbacks
so callback-side window mutation cannot invalidate the traversal, and
`BringWindowToTop` keeps the existing convention that the back of `g_windows` is
front-most.

`opus_win32_user32_test` covers class/text lookup, recursive child
enumeration, top-level enumeration stop behavior, thread-window enumeration,
ancestor walking, control IDs, class names, activation through
`BringWindowToTop`, simple minimized/maximized state, `OpenIcon`, and
`MessageBeep`. The Win32 coverage baseline no longer lists those names.

Validated with `cmake --build build-item14l --target
opus_win32_user32_test opus_x64_runtime_test opus_original_engine --parallel 8`
and `ctest --test-dir build-item14l -R
'opus_win32_user32_test|opus_x64_runtime_test|win32_coverage'
--output-on-failure`, which passed 3/3. Also validated with `cmake --build
build-item14l --target opus_original_strtbl_test opus_original_sttb_test
opus_original_plc_test opus_sdm_cab_test opus_original_command_test
opus_win32_memory_test opus_win32_resource_test opus_win32_gdi_object_test
opus_win32_gdi_raster_test opus_win32_font_test opus_win32_print_test
opus_win32_user32_test opus_x64_runtime_test opus_original_engine
opus_x64_runtime --parallel 8`, then `ctest --test-dir build-item14l -R
'strtbl|sttb|plc|sdm_cab|command|opus_x64_runtime_test|win32_memory|opus_win32_resource_test|opus_win32_gdi_(object|raster)_test|opus_win32_font_test|opus_win32_print_test|opus_win32_user32_test|win32_coverage'
--output-on-failure`, which passed 14/14.

Reviewed by agy before implementation; claude was asked for this exact slice but
stalled without output and was stopped. The implementation follows agy's
snapshot-enumeration warning.

## Add user32 menu core

The non-Windows user32 shim now owns a small in-memory menu tree. It implements
`CreateMenu`, `CreatePopupMenu`, `IsMenu`, `DestroyMenu`, `AppendMenuA/W`,
`InsertMenuA/W`, `ModifyMenuA/W`, `DeleteMenu`, `RemoveMenu`,
`GetMenuItemCount`, `GetMenuItemID`, `GetSubMenu`, `GetMenuState`,
`GetMenuStringW`, `CheckMenuItem`, `CheckMenuRadioItem`, `SetMenuInfo`,
`SetMenuItemBitmaps`, `GetMenu`, `SetMenu`, `DrawMenuBar`, and
`TrackPopupMenu`. It deliberately leaves popup UI/rendering out;
`TrackPopupMenu` reports no selection.

`OpusChangeMenu` already adapts legacy `ChangeMenu` to the primitive APIs, so
`ChangeMenu` itself was not reimplemented. Popup submenu handles are stored as
`UINT_PTR` to avoid pointer truncation, and `DestroyMenu` recursively
invalidates submenus still attached to the destroyed tree while `RemoveMenu` and
`DeleteMenu` only detach items.

`opus_win32_user32_test` covers popup/string/separator mutation,
by-position/by-command lookup, text retrieval, checked/radio state, window menu
attachment, no-op draw/track APIs, detached submenu lifetime, recursive
`DestroyMenu` invalidation, and invalid handle behavior. The Win32 coverage
baseline no longer lists those menu names.

Validated with `cmake -S src -B build-item14m -DCMAKE_BUILD_TYPE=Debug`,
`cmake --build build-item14m --target opus_win32_user32_test
opus_x64_runtime_test opus_original_engine --parallel 8`, and `ctest --test-dir
build-item14m -R
'opus_win32_user32_test|opus_x64_runtime_test|win32_coverage'
--output-on-failure`, which passed 3/3.
Also validated with `cmake --build build-item14m --target
opus_original_strtbl_test opus_original_sttb_test opus_original_plc_test
opus_sdm_cab_test opus_original_command_test opus_win32_memory_test
opus_win32_resource_test opus_win32_gdi_object_test
opus_win32_gdi_raster_test opus_win32_font_test opus_win32_print_test
opus_win32_user32_test opus_x64_runtime_test opus_original_engine
opus_x64_runtime --parallel 8`, then `ctest --test-dir build-item14m -R
'strtbl|sttb|plc|sdm_cab|command|opus_x64_runtime_test|win32_memory|opus_win32_resource_test|opus_win32_gdi_(object|raster)_test|opus_win32_font_test|opus_win32_print_test|opus_win32_user32_test|win32_coverage'
--output-on-failure`, which passed 14/14.

Reviewed by agy before implementation; claude was asked for this exact slice but
stalled without output and was stopped.

## Add user32 window properties

The non-Windows user32 shim now stores per-window properties for `SetPropW`,
`GetPropA`, `GetPropW`, and `RemovePropW`. String property keys are matched
case-insensitively, integer atom keys are accepted as raw 16-bit ids, `SetPropW`
replaces existing values, and `RemovePropW` returns the previous handle without
owning or freeing the payload.

`opus_win32_user32_test` covers string-key lookup through A and W APIs,
case-insensitive replacement, atom-key lookup and removal, missing-property
returns, and invalid window/name handling. The Win32 coverage baseline no longer
lists the implemented W property names.

Validated with `cmake -S src -B build-item14n -DCMAKE_BUILD_TYPE=Debug`,
`cmake --build build-item14n --target opus_win32_user32_test
opus_x64_runtime_test opus_original_engine --parallel 8`, and `ctest --test-dir
build-item14n -R
'opus_win32_user32_test|opus_x64_runtime_test|win32_coverage'
--output-on-failure`, which passed 3/3.
Also validated with `cmake --build build-item14n --target
opus_original_strtbl_test opus_original_sttb_test opus_original_plc_test
opus_sdm_cab_test opus_original_command_test opus_win32_memory_test
opus_win32_resource_test opus_win32_gdi_object_test
opus_win32_gdi_raster_test opus_win32_font_test opus_win32_print_test
opus_win32_user32_test opus_x64_runtime_test opus_original_engine
opus_x64_runtime --parallel 8`, then `ctest --test-dir build-item14n -R
'strtbl|sttb|plc|sdm_cab|command|opus_x64_runtime_test|win32_memory|opus_win32_resource_test|opus_win32_gdi_(object|raster)_test|opus_win32_font_test|opus_win32_print_test|opus_win32_user32_test|win32_coverage'
--output-on-failure`, which passed 14/14.

Reviewed by agy before implementation; claude was asked for this exact slice but
stalled without output and was stopped. agy's review called out atom keys,
case-insensitive matching, replacement semantics, caller-owned payloads, and
keeping property enumeration/global atom behavior out of scope.

## Add user32 callback and string helpers

The non-Windows user32 shim now implements `CallWindowProcW`, `lstrcmpW`, and
`lstrcmpiW`. `CallWindowProcW` invokes a non-null previous procedure directly
and returns zero for null. `lstrcmpW` compares 16-bit code units ordinally, and
`lstrcmpiW` folds only ASCII letters before comparing, leaving full NLS
collation out of scope.

`opus_win32_user32_test` covers direct callback invocation, null callback
handling, exact and ordered wide-string comparison, null-string handling,
ASCII-insensitive comparison, and non-ASCII code-unit behavior. The Win32
coverage baseline no longer lists those helper names.

Validated with `cmake -S src -B build-item14o -DCMAKE_BUILD_TYPE=Debug`,
`cmake --build build-item14o --target opus_win32_user32_test
opus_x64_runtime_test opus_original_engine --parallel 8`, and `ctest --test-dir
build-item14o -R
'opus_win32_user32_test|opus_x64_runtime_test|win32_coverage'
--output-on-failure`, which passed 3/3.
Also validated with `cmake --build build-item14o --target
opus_original_strtbl_test opus_original_sttb_test opus_original_plc_test
opus_sdm_cab_test opus_original_command_test opus_win32_memory_test
opus_win32_resource_test opus_win32_gdi_object_test
opus_win32_gdi_raster_test opus_win32_font_test opus_win32_print_test
opus_win32_user32_test opus_x64_runtime_test opus_original_engine
opus_x64_runtime --parallel 8`, then `ctest --test-dir build-item14o -R
'strtbl|sttb|plc|sdm_cab|command|opus_x64_runtime_test|win32_memory|opus_win32_resource_test|opus_win32_gdi_(object|raster)_test|opus_win32_font_test|opus_win32_print_test|opus_win32_user32_test|win32_coverage'
--output-on-failure`, which passed 14/14.

Reviewed by agy before implementation; claude was asked for this exact slice but
stalled without output and was stopped. agy's review called out null callback
handling, null string handling, ordinal comparison, ASCII-only case folding, and
avoiding Unicode/NLS dependencies.

## Add kernel string conversions

The non-Windows kernel32 shim now implements `MultiByteToWideChar` and
`WideCharToMultiByte` for the code pages used by current call sites: `CP_ACP`,
`CP_UTF8`, and 1252. The helpers support sizing calls, explicit byte/character
counts, `-1` counts including the null terminator, destination truncation
failure, UTF-8 encode/decode, and default-character reporting for wide
characters that cannot fit byte code pages.

`opus_win32_memory_test` now covers ACP sizing and truncation, explicit input
slicing, UTF-8 decode/encode, 1252 fallback with `WC_NO_BEST_FIT_CHARS`,
`used_default_char`, and unsupported code-page failure. The Win32 coverage
baseline no longer lists those conversion helpers.

Validated with `cmake -S src -B build-item12b -DCMAKE_BUILD_TYPE=Debug`,
`cmake --build build-item12b --target opus_win32_memory_test
opus_x64_runtime_test opus_original_engine --parallel 8`, and `ctest --test-dir
build-item12b -R
'opus_win32_memory_test|opus_x64_runtime_test|win32_coverage'
--output-on-failure`, which passed 3/3.
Also validated with `cmake --build build-item12b --target
opus_original_strtbl_test opus_original_sttb_test opus_original_plc_test
opus_sdm_cab_test opus_original_command_test opus_win32_memory_test
opus_win32_resource_test opus_win32_gdi_object_test
opus_win32_gdi_raster_test opus_win32_font_test opus_win32_print_test
opus_win32_user32_test opus_x64_runtime_test opus_original_engine
opus_x64_runtime --parallel 8`, then `ctest --test-dir build-item12b -R
'strtbl|sttb|plc|sdm_cab|command|opus_x64_runtime_test|win32_memory|opus_win32_resource_test|opus_win32_gdi_(object|raster)_test|opus_win32_font_test|opus_win32_print_test|opus_win32_user32_test|win32_coverage'
--output-on-failure`, which passed 14/14.

Reviewed by agy before implementation; claude was asked for this exact slice but
stalled without output and was stopped. agy's review called out supported code
pages, `-1` count semantics, sizing calls, truncation, UTF-8 handling,
default-character reporting, and accepting `WC_NO_BEST_FIT_CHARS` from current
call sites.

## Add kernel thread, memory, and temp-file helpers

The non-Windows kernel32 shim now implements `GetCurrentThreadId`,
`GlobalMemoryStatusEx`, and `GetTempFileNameA`. The thread id is stable and
nonzero for the current single-thread shim, memory status returns bounded
synthetic values when `dwLength` is valid, and temporary names use the existing
path normalization while creating the file when the unique value is generated.

`opus_win32_memory_test` covers stable thread id reporting, memory-status
success and invalid-argument failure, generated temporary file creation, and
explicit unique temporary-name composition. The Win32 coverage baseline no
longer lists those kernel helpers.

Validated with `cmake -S src -B build-item12c -DCMAKE_BUILD_TYPE=Debug`,
`cmake --build build-item12c --target opus_win32_memory_test
opus_x64_runtime_test opus_original_engine --parallel 8`, and `ctest --test-dir
build-item12c -R
'opus_win32_memory_test|opus_x64_runtime_test|win32_coverage'
--output-on-failure`, which passed 3/3. Also validated with `cmake --build
build-item12c --target opus_original_strtbl_test opus_original_sttb_test
opus_original_plc_test opus_sdm_cab_test opus_original_command_test
opus_win32_memory_test opus_win32_resource_test opus_win32_gdi_object_test
opus_win32_gdi_raster_test opus_win32_font_test opus_win32_print_test
opus_win32_user32_test opus_x64_runtime_test opus_original_engine
opus_x64_runtime --parallel 8`, then `ctest --test-dir build-item12c -R
'strtbl|sttb|plc|sdm_cab|command|opus_x64_runtime_test|win32_memory|opus_win32_resource_test|opus_win32_gdi_(object|raster)_test|opus_win32_font_test|opus_win32_print_test|opus_win32_user32_test|win32_coverage'
--output-on-failure`, which passed 14/14.

Reviewed by agy and claude before implementation, but both reviewer commands
stalled without final output and were stopped. The implementation keeps thread
modeling, real host memory accounting, and collision-resistant temp-file retry
logic out of scope until a real call site requires them.
