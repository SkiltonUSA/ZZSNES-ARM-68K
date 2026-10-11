# Standalone backup validation — 10 October 2026

- Saved executables, ELF payloads, runtime archive and picolibc source archive
  pass their SHA-256 inventories. Pinned emulator files match upstream manifests.
- Native ASan/UBSan builds pass for PocketSNES (three identical six-frame runs),
  the Mesen CPU fixture and the Mesen integer-clock system fixture.
- All three saved ARM payloads pass relocated instruction-model checks at two
  bases, corrupt-fixture rejection and cancellation. PocketSNES synthetic-clock
  checks also pass. These use the existing Unicorn 2.1.4 environment.
- The standalone 68k PocketSNES launcher was rebuilt with the saved ARM ELF,
  matching MMU headers and pinned cross-toolchain. It is byte-identical to the
  hardware-tested 542,084-byte executable, MD5
  `78a67d1434e85d6b304916333514e262`.
- No new ARM-core cross-compilation, hardware run, deployment or firmware change
  was performed during this backup. Original physical evidence is preserved.

Logs: [native](../amiga/records/2026-10-10/standalone-backup/native.log),
[ARM models](../amiga/records/2026-10-10/standalone-backup/model.log),
[68k rebuild](../amiga/records/2026-10-10/standalone-backup/host.log).
Upstream whitespace is preserved to retain source hashes; authored backup
README, guides and verifier pass whitespace checks.
