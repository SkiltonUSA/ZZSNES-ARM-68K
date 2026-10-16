# Direct ZIP ROM loading

`source/` contains editable MUI/library sources and the bounded ZIP reader.
`host/` contains the 68k player overlays and its copy of the same ZIP reader.
`build-inputs/` retains the prepared launcher inputs. `make current` rebuilds
both 68k executables using these overlays and the unchanged ARM payload from
`development/beta-20261015/`.

Stored and raw-deflate ZIP32 archives with one matching cartridge are loaded
into caller-owned memory. No member is written to disk. Directory scanning
only probes archive metadata and yields between archive candidates. Actual
loading verifies local/central metadata, byte counts and CRC32.

Mark Adler's puff 2.3 sources are unmodified from zlib v1.3.1; see
`source/zip/upstream.json` for pinned URLs/hashes and `puff.h` for the licence.
Compile puff separately from the host/UI translation unit.

Run `make test-zip`, `make test-library-large`, `make test-current` and
`make verify-current`. Native tests do not establish physical execution.
See `docs/zip-validation.md` for the separate hardware checks.

The supplied colour artwork is unchanged. `regenerate.py` recreates the embedded
MUI images and Workbench icon using Python and ImageMagick.
