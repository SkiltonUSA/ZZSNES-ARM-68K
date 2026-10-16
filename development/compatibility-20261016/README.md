# Compatibility candidate source

Overlay on development/beta-20261015/source/core. The accompanying snapshot.patch
shows the sole core change: rebuild derived sprite lists after state restoration.
The save format and allocation layout are unchanged. Upstream source notices
remain in snapshot.c. This overlay is used only by explicit diagnostic builds.

`--experimental-chips` also alters the staged host/core cartridge gate for CLI
tests; it is not a supported-chip release. The normal MUI gate remains unchanged.
See docs/compatibility-development.md from the repository root for tests,
remaining failures and the separate output directory.
