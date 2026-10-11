# Workbench prerelease backup validation — 11 October 2026

The standalone checkout rebuilds both 68k programs byte-for-byte using the
archived hardware-tested ARM ELF and pinned cross-toolchain:

- ZZSnesLibrary: MD5 `89358045e6941538fbdc601e5546291e` (91,040 bytes).
- ZZSnes-ui: MD5 `4fc02f99b9fcc8808c3696090f39fbca` (642,752 bytes).

Actual generated stereo AHI worker/state-drain and all 16,384 allocation
alignment checks pass in the standalone checkout. `make verify` checks saved
artifacts, vendor source pins, original timing, displayed FPS and teardown.

The ZIP packager verifies explicit membership, binary hashes, extraction round
trip, executable permissions, empty settings and ROM/state exclusions. Installer
and drawer icons accompany the exact binaries. Commercial ROMs, user states,
OS files and firmware are absent. The archive checksum is in `release.json`.
No new target launch/deployment or firmware change was performed for packaging.
Physical results are retained under the 10 October records.
