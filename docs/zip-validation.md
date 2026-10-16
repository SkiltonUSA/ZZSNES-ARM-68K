# Direct ZIP loading — ZZSNES 0.6.0 Beta

The 68k launcher and player load ZIP cartridges directly into bounded RAM.
The ARM payload matches the previous beta byte-for-byte. Cartridge gates,
shared-memory layout, scheduling and save serialization are unchanged.
No gameplay speed improvement is claimed.

## Native checks

- 40 archive/plain-file cases under ASan/UBSan: exact bytes, output guards,
  stored/deflate, nested names, data descriptors, metadata and maximum comments.
- Malformed/truncated archives, unsupported features, multiple ROMs, oversized
  output and CRC failures rejected.
- 20,000-entry library persistence, sorting, filtering and allocation-failure tests.
- Mounted-volume scanning, ZIP candidate filtering, cancellation and cleanup.
- Existing relocated ARM presenter tests: baseline pixels, scanlines,
  NEON/fallback, SIMD restoration and cancellation.

The first model-test command used a Python environment without Unicorn; rerunning
with the documented model environment passed. This was a test setup issue.

## Physical A4000TX checks

| Check | Result |
| --- | --- |
| Plain cartridge, 120 frames | Passed |
| Same cartridge in deflated ZIP, 120 frames | Passed |
| Deliberately invalid ZIP CRC | Rejected before ROM publication; expected error exit |
| ZIP session: pause, save, load, artwork capture, quit | Passed |
| MUI library: select ZIP, Play, save/load/capture, return | Passed |
| Register/cache witness restoration and owned-memory release | Passed |

The NES GUI harness initially expected a window although fullscreen was selected.
The player was running normally; the corrected check completed the lifecycle.
ZIP tests used the owner's Mario Bros. (NES) and Super Mario World (SNES) files.
These were private test inputs and are not distributed. One tested cartridge
per console does not establish compatibility for every supported mapper.

NES also passed a ZIP-member filename heuristic fixture: a neutral archive name
containing `Test (Europe).nes` selected PAL. This verifies filename propagation,
not the compatibility of an additional PAL game.

[Machine-readable results and native output](../amiga/records/2026-10-16/zip/).
No display/frame-rate benchmark was performed for this loading feature.

The tested launcher/player pair was installed with file checksum verification.
Existing library settings and icon metadata were retained; previous executables
were kept for rollback.
