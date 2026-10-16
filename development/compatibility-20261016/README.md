# Compatibility candidate source

Overlay on development/beta-20261015/source/core. Individual patches show:

- `snapshot.patch`: rebuild derived sprite lists after state restoration.
- `memmap.patch`: initialize the Super FX ROM bank timings as well as the lower
  memory blocks. Zero-cycle ROM fetches otherwise break sound startup timing.

Save format and allocation layout are unchanged. Upstream notices remain in
both source files. Only explicit diagnostic builds use this overlay.

`--experimental-chips` also alters the staged host/core cartridge gate for CLI
tests; it is not a supported-chip release. The normal MUI gate remains unchanged.
See docs/compatibility-development.md for test results and remaining failures.
