# Build and verification

`make verify` checks every saved artifact SHA-256, core source manifests and
the arithmetic behind the published physical timing. Nothing here deploys or
contacts an Amiga.

## Native references

Install Python 3 and Clang with AddressSanitizer/UndefinedBehaviorSanitizer:

```sh
make test-native
```

PocketSNES uses the vendored, hash-checked snapshot without a network checkout.
Builders write adaptation patches under `.context/`. The only cartridge is our
original generated checkerboard. Mac execution speed is not ZZ9000 performance.

## Saved ARM payloads

Install `unicorn==2.1.4` in a Python environment. The checkers currently use
LLVM tools at `/opt/homebrew/opt/llvm/bin/`; adjust for another installation.

```sh
make test-model MODEL_PYTHON=/path/to/venv/bin/python
```

These test exact saved ELFs at synthetic addresses: relocation, output,
alignment, cancellation and corrupted fixtures. Physical cache-restoration
claims come from the hardware logs, not this instruction model.

## Cross-building

This is a backup of an existing development environment, not a turnkey SDK.
The tested ARM compiler was GCC arm-none-eabi 15.3 with picolibc 1.8.12. The
68k container image is pinned in `amiga/upstream.json`; Clang and LLVM lld
also build mapping probes and Mesen payloads.

Provide a persistent ARM toolchain container with **this checkout mounted at
/work**, GCC, G++, binutils and picolibc headers. For example:

```sh
export POCKET_TOOLCHAIN_COMMAND='["podman","exec","zzsnes-toolchain"]'
python3 scripts/build_pocketsnes_arm.py
python3 scripts/build_pocketsnes_host.py
```

The ARM builder seeds its cache from the exact archived PIC soft-float libc.
The 68k builder defaults to the development Mac's Podman connection
`nuflix-converter-root`; override with `AMIGA_CONTAINER_COMMAND` (JSON array).

To rebuild libc, unpack `sources/picolibc_1.8.12.orig.tar.gz` to
`.context/amiga/ffmpeg-bench/picolibc-1.8.12` (historical path retained for
this script), then run `scripts/build_pocketsnes_libc.py` with the same
container prefix. Its source hashes are in `libc-sources.json`.

Mesen examples:

```sh
python3 scripts/build_mesen_snes.py --arm --container-command '["podman"]'
python3 scripts/build_mesen_snes_system_arm.py --word-memory --container-command '["podman"]'
```

Generated tools/output remain ignored. Recorded manifests preserve original
workspace paths/hashes. Standalone changes select vendored PocketSNES sources
and configurable toolchain commands; they do not change emulator algorithms.
Saved executables are the exact hardware-tested versions, not a claim that
every new compiler generates identical bytes.

## MUI library and prerelease packaging

The exact tested binaries are in `artifacts/pocketsnes-library/` and the earlier
plain player in `artifacts/pocketsnes-player/`. To recreate the ROM-free ZIP:

```sh
make verify
python3 scripts/package_pocketsnes_library.py
```

Output: `.context/releases/ZZSnes-Library-0.2.0-dev.zip` and `SHA256SUMS.txt`.
The packager checks binary hashes, archive contents and executable flags; it
starts from an explicit file list, not a copy of the installed user drawer.

To rebuild the 68k player/launcher around the exact archived ARM payload:

```sh
python3 scripts/prepare_saved_library.py
export AMIGA_CONTAINER_COMMAND='["podman","--connection","nuflix-converter-root"]'
python3 scripts/build_pocketsnes_library.py --host-only
python3 tests/amiga/pocketsnes/check_library_host.py
```

Use your container command (e.g. `["docker"]`) instead of the example's Mac
connection. The backup validation rebuilt both binaries byte-for-byte.
A full build omits `--host-only` and requires the configured
`POCKET_TOOLCHAIN_COMMAND` container mounted on this checkout, as above. The
standalone builder reads the hash-checked vendored core and archived libc.

The save-state replay tests require the owner's SMW input at the ignored path
`.context/amiga/pocketsnes-game/input.sfc`. Supply your own authorised copy;
no build or test downloads it. With that file present:

```sh
python3 scripts/test_pocketsnes_library_native.py
/path/to/unicorn-venv/bin/python tests/amiga/pocketsnes/check_library_arm.py
```

The second cartridge fixture is generated from original source. The ARM model
uses the staged ELF and exact replay/presentation checks; native tests do not
claim Amiga execution. Original workbench validation and logs live under
`amiga/records/2026-10-10/pocketsnes-library/`. Source-level standalone changes
only replace workspace tool paths with configurable prefixes and saved inputs.
