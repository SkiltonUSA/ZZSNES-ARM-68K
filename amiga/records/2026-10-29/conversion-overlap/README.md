# Conversion-overlap evidence

See ../../../../docs/conversion-overlap.md for the result and build commands.
The private hardware harness records both consoles and uses workspace paths;
adapt test ROM and transport paths locally. No ROMs or executables are included.

- results-0: 8 timing logs and 2 separate pixel-verification logs.
- lifecycle: 2 completed readiness-gated UI checks, plus 2 earlier NES attempts
  that injected input too soon. All 4 ended cleanly; do not count the early
  attempts as proof of a complete window/fullscreen round trip.
- Per-build folders: hashes and generated source patches.
- red.log: expected assertion failure when old unlock behavior is used during
  a proposed overlapping render; not a defect report for the original schedule.
- current-tests logs: native checks passed, model invocation lacked the local
  tools. model-tests logs record the successful corrected model invocations.
- Hardware timing uses one initial readback; periodic readbacks are separate.
- build.py contains the exact private relink parameters; prepare.py records
  the transformation used. The supported repository build flag reproduces it.
