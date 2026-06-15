# Changelog: fix/final-refactor

Issue #60 (Namngivning och single-responsibility-genomgång) execution sequence.

## Step 1 — move `ring_buffer` to `components/`

- Moved `scout_screen/main/adapters/ring_buffer.{c,h}` →
  `scout_screen/components/ring_buffer/ring_buffer.{c,h}` (contents unchanged). It is a
  generic int32 stats ring with no project knowledge, so `components/` is the correct home
  per `docs/CODING_STYLE.md`.
- Added `scout_screen/components/ring_buffer/CMakeLists.txt` (`SRCS ring_buffer.c`,
  `INCLUDE_DIRS "."`, no REQUIRES — only uses `<stdint.h>`).
- `scout_screen/main/CMakeLists.txt`: removed `adapters/ring_buffer.c` from SRCS, added
  `ring_buffer` to REQUIRES. Only consumer `screen_state.{c,h}` is unchanged.
- Verified: scout_screen builds.
