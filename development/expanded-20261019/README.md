# Expanded cartridge development build

Opt-in overlay for ZZSNES 0.6.5-dev. Build with `make current-expanded` and
run `make test-expanded`. ARM model tests: `make test-expanded-arm` with
`MODEL_PYTHON` pointing to a Unicorn environment and `LLVM_NM` available.
No build/test target deploys automatically.

The base is the preserved beta snapshot plus compatibility, FX3, thin-display,
optimisation and scalable-launcher overlays. `common/` is copied to the host,
ARM core and launcher so cartridge bounds, state format and options agree.
The existing source/licence notices apply. No commercial ROM data is included.

Changes include cartridge gating, C4 storage separation, S-DD1 decompression,
6 MiB/ExHiROM loading, GSU/DSP/render/timing state, 512-column rendering,
640x480 presentation, and persistent emulation settings. Interlace currently
uses bob/line doubling; field-accurate interlace is not claimed.

Memory layout within a runtime-probed, Exec-owned 48 MiB allocation:

| Region | Offset | Capacity |
|---|---:|---:|
| Image | 0 | <1 MiB |
| Tables, witness, existing control/PCM/preview | unchanged | bounded by existing contracts |
| ROM transfer | `0x200000` | 6 MiB |
| Full-detail picture transfer | `0x800000` | 512 × 480 × 2 bytes |
| Save transfer | `0x880000` | 2 MiB |
| Private BSS including 24 MiB heap | `0xb00000` | linker-checked below stack |
| Stack reserve | `0x2fd0000` | 192 KiB |

These are relative offsets, never static physical DDR allocations. The MMU,
cache witnesses, single-Core1 ownership and reset-before-free contract remain.

States use version 3. They include emulation options, chip continuation data,
audio/scheduler timing and render state. Runtime pointers are reconstructed.
Version 2 standard-cartridge states remain readable; old incomplete special-chip
states are rejected. Resolution is a presentation preference and is not forced
by loading a state. Current save states restore their emulation options.

Additional options, SRAM persistence and per-game profiles are documented in
`docs/additional-options.md`. The 0.6.4 physical records describe the preceding
build; new results are under `records/2026-10-19/options`.
