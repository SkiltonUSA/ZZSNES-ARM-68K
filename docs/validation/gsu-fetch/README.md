# Super FX fetch experiments

Two isolated candidates were compared with the recorded 0.7.13 row-renderer
player. Neither changes the production build, guest timing, state format or
installed audio settings. These are investigations of Super FX audio starvation,
not fixes for synthesis corruption.

## Changes

- **Derived mask:** cache the FX3 program-bank mask when binding the program
  pointer, including register import, LJMP and state restoration. This removes
  repeated cartridge/bank classification from fetches but introduces a mask load.
- **Separate loop:** choose the ordinary Super FX or original FX3 pipeline loop
  once per invocation. Ordinary fetches retain the full 16-bit program counter;
  bank changes still reload the program pointer. STOP, instruction budgets and
  interrupt behavior are unchanged.

`scripts/prepare_gsu_fetch.py` contains both transformations. Build flags are
`--gsu-fetch-test` and `--gsu-fetch-loop-test`, respectively; both require
`--release-ax --output-dir .context/<fresh-directory>`. They are not launcher
options. Product strings remain 0.7.13-dev; [build hashes](builds.json) identify
these experiments separately from the published 0.7.13 package.

## Correctness checks

- Native 1,200-frame traces for Star Fox 2, Stunt Race FX and Doom: exact video,
  stereo PCM and serialized GSU agreement with the reference for each candidate.
- Cortex-A9 instruction-model replay: 30 frames from Star Fox 2 frame 600 and
  a Stunt Race FX race checkpoint, at two relocated candidate addresses; exact
  video/PCM/GSU agreement. These checks cover gameplay states, not just boot.
- Native and ARM FX3 fixtures cover low-ROM mirroring, extended-ROM/RAM execution,
  pixel operations, bounded execution, STOP and CPU mappings. The ARM fixture now
  accepts the current build's ELF packer and 96 MiB runtime layout explicitly;
  its older build mode remains available.
- Derived mask: 67,108,864 addresses across all 256 program banks and both FX
  modes, plus high-bit/wrap boundaries, under address/undefined-behavior sanitizers.
- Separate loop: 66,560 old/new loop cases in each of normal and address-debug
  builds, covering inactive G, STOP, exhausted/zero budgets, bank changes, PC wrap
  and pipeline state under sanitizers.
- `make test-native` passes, including expected reproduction of the archived
  sprite-restoration failure followed by passing corrected normal/Super FX replay.

Logs are kept beside this report. Commercial ROMs and checkpoints are not included.

## Instruction model versus hardware

For the same three Star Fox 2 frames (601–603), the model records 42,193,923
instruction slots for the reference, 41,738,357 for the mask candidate (1.08% fewer)
and 40,503,813 for the separate loop (4.01% fewer). Output traces agree.
Instruction slots are not Cortex-A9 cycles: memory behavior, instruction layout
and caches can outweigh fewer instructions.

Physical results and limits are recorded in [hardware measurements](hardware.md).
