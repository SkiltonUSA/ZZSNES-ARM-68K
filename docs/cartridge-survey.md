# Supplied SNES cartridge survey

All eleven archives contain one cartridge. None exceeds the current 4 MiB ROM
limit. Super Street Fighter II is exactly 4 MiB and needs no enhancement chip.
The selection exercises five Super FX cartridges, one SA-1 cartridge and five
standard cartridges. No larger shared ROM/SRAM allocation was needed for this set.

| Cartridge | ROM size | Hardware | Native 30-frame state replay | A4000TX check |
| --- | ---: | --- | --- | --- |
| Super R-Type (USA) | 1 MiB | Standard LoROM | pass | 120 frames; clean exit |
| Super Star Fox Weekend (USA) | 1 MiB | Super FX | video mismatch | Not run |
| Super Street Fighter II (USA) | 4 MiB | Standard Fast HiROM | pass | 120 frames; clean exit |
| Stunt Race FX (USA) (Rev 1) | 1 MiB | Super FX | video mismatch | Not run |
| Star Fox 2 (USA) | 1 MiB | Super FX | pass | Not run |
| Star Fox (USA) (Rev 2) | 1 MiB | Super FX | pass | Not run |
| Kirby Super Star (USA) | 4 MiB | SA-1 | pass | Not run |
| Imperium (USA) | 1 MiB | Standard LoROM | video mismatch | 120 frames; clean exit |
| GunForce (USA) | 0.5 MiB | Standard LoROM | pass | 120 frames; clean exit |
| Final Fantasy III (USA) (Rev 1) | 3 MiB | Standard Fast HiROM + 8 KiB SRAM | pass | 120 frames; clean exit |
| Doom (USA) | 2 MiB | Super FX | pass | Not run |

The physical standard-game checks ran 120 frames from the original ZIPs and
confirmed register/cache witness restoration and owned-memory release. These
are bounded startup checks, not full playthroughs or measured gameplay FPS.
The files were added to the local library; ROM data is not included in this repo.

## Enhancement-chip status

The ordinary player still rejects Super FX and SA-1 cartridges. A separate
native-only test copy relaxed its cartridge gate for these specific chip types.
All eleven reached 600 frames without sanitizer errors. Most passed a further
30-frame video/PCM replay after save/load, but Super Star Fox Weekend, Stunt Race
FX and Imperium produced different video at the first replayed frame. PCM matched
at the reported mismatch. Imperium uses standard hardware, so that finding needs
a separate PPU/save-state investigation rather than a coprocessor assumption.
Passing a title-screen replay is not proof of complete gameplay or save support.

Our old Snes9x2002 snapshot code saves normal RAM and cartridge SRAM plus
conditional SA-1 structures. Unlike modern Snes9x's explicit `SnapFX` fields,
it does not explicitly save the complete Super FX internal state. The missing
coverage is a concrete reason not to enable the chips by merely removing the
gate. It is not yet proven to be the sole cause of these observed mismatches.

Next: audit and add Super FX serialization, reproduce the replay mismatches at
multiple gameplay checkpoints, check fresh-process relocation, then run bounded
A9 tests and measure chip-heavy gameplay. SA-1 also needs physical testing and
save/persistence checks. Competition-cartridge behavior needs its own review.

[Memory/code comparison and upstream permalinks](snes-memory-compatibility.md).
[Recorded results](../amiga/records/2026-10-16/cartridges/).

## Reproduce the native survey

`python3 scripts/probe_cartridges.py --allow-experimental-chips /path/to/game.zip`

Requires Clang with ASan/UBSan and Python. Runs in an ignored private directory;
it neither deploys nor changes the normal player. The optional flag changes
only that test copy's gate. Save replay mismatches are recorded in native-results.json; they do not become compatibility
claims. Provide your own cartridges.
