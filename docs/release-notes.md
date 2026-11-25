# ZZSNES 0.7.13 Beta

Application/launcher version: **0.7.13-dev**. AmiPkg version: **0.7.13-dev**.

## Changes

- Corrected flipped-tile clipping and batched complete-row rendering, with unchanged guest timing and sound synthesis.
- Matching rebuilt launcher and player versions, with isolated build records.
- AmiPkg includes the current README, credits and licence documents.

Super FX games can still exceed the audio-production budget; Star Fox 2, Stunt Race FX and Doom audio gaps remain unresolved. No pitch, frameskip or emulated clock changes are introduced.

## Installation

Extract the LHA and open Install. Choose a writable destination. Close the emulator
before updating; existing settings, ROMs and saves are preserved. MUI and AHI
requirements are checked. Clock controls require the optional clock service;
normal emulation does not. No firmware or clock change happens automatically.

## Validation

[Build identities, model tests and physical checks](validation/release-0.7.13/README.md).
Listening acceptance is separate from automated delivery and clean-exit checks.
