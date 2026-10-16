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

The current launcher source is `development/library-20261016/source/`; use
`make test-library-large` after library changes. Player snapshot stays unchanged.
