# ZZSNES development guide

Read README.md, docs/session-memory.md and the relevant core README first.
Keep Mesen and PocketSNES diagnostic paths separate. Preserve upstream notices,
pins and generated-patch provenance. Never claim instruction-model results as
physical cache validation or headless fixture timing as game/display FPS.

Run `make test-native` for core changes and the relevant relocated ARM model
checks after an ARM build. Verify saved-artifact checksums with `make verify`.
Use only original or user-authorised ROMs; do not commit commercial game data.

Physical launch requires a free Core1, runtime-probed Exec-owned ZZ9000 RAM,
uncached mailbox, bounded commands, cancellation and reset before release.
Preserve all register/MMU/cache witness checks. Do not reuse recorded physical
addresses as allocations. Only the recorded XX19c/XACP 1.7 contract is verified.

Builds/tests do not deploy or connect automatically. Read current hardware
ownership before any explicitly authorised physical test. Keep .context/ and
.tools/ ignored. Update the handoff when installed state changes.

Current beta source/build inputs are in `development/beta-20261015/`; use
`make current-arm`, `make test-current` and `make verify-current` for that path.

The current launcher source is `development/borders-20261016/source/`; use
`make test-library-large` after library changes. Current 68k host overlays are in `development/borders-20261016/host/`; the ARM
payload remains from the original snapshot. Run `make test-zip` after archive changes.

Current ARM presentation overlays and packed image are in the snapshot player/
directory. Run make test-border and make test-current after border changes.

Compatibility candidate overlays live in development/compatibility-20261016/.
Cartridge/timing changes require an explicit build_current.py --arm --compatibility
or --experimental-chips flag. All fresh ARM builds include the snapshot sprite-list
restore fix from that overlay; archived release payloads remain unchanged. Read
docs/compatibility-development.md before enabling a chip or claiming game support.

Run `make test-superfx-timing` after mapper changes. The isolated Super FX
candidate fixes ROM-bank timings; special-chip states remain incomplete.

Super FX launcher integration uses `make current-superfx` (`--arm --superfx`).
The supplemental ZZSnes-FX player uses the compatibility overlay and the
shared host/ARM gate in development/fx3-20261016/cartridge.h. The normal
player remains for standard ROMs. FX3 notices are advisory, not a launch veto.
Experimental FX3 behaviour is documented in docs/fx3-development.md. Run
test-fx3, test-fx3-warning and test-library-large, plus native and relocated ARM
tests after player changes. Commercial FX3 compatibility remains unverified.

Thin scanlines are a separate 640x480 presentation candidate; see
docs/thin-scanlines.md and development/thin-scanlines-20261016. Use
current-thin/current-thin-superfx, test-thin-geometry and the opt-in relocated
pixel tests. Preserve the original modes and every logical image row.

Latest development launcher overlay: `development/scalable-launcher-20261019/source`.
Use `make launcher` and `make test-launcher-layout`; `--probe` builds an isolated
read-only MUI check without deployment. Keep historical beta snapshots unchanged.
