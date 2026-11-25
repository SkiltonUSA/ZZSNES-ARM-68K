# Fresh build records

The isolated rebuild completed and its `build.json` artifact hashes were verified.
These are build checks, not new hardware measurements. Previously tested candidate
binaries remain separate. Embedded build paths can change output hashes.

The builder records actual output hashes, staged source/library hashes, successful
main build commands and selected toolchain environment settings. This is not a
complete dependency or transitive helper-command manifest. Copied historical
`next-build.json` files are excluded. The manifest is written only after success.

Use `--output-dir .context/<new-name>` to preserve a recorded candidate. An existing
directory or a path outside the repository is rejected before changes are made.

Validation: `python3 tests/test_build_identity.py` checks hashes, missing-output
refusal and the non-destructive fresh-directory guard.
