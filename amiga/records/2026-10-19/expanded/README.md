# Expanded cartridge validation

See [the report](../../../../docs/expanded-cartridges.md) for scope and limits.
`build.json` records the final 0.6.4-dev build and resolved input hashes.
`physical.json` lists accepted final-build runs; `installed.json` records
verified deployment and previous executable hashes. Native/model results are
separate and do not prove physical cache behaviour or complete game compatibility.

`initial-*` logs are earlier development checks, not the final binary. The
initial DSP-1 attempt exited cleanly with the old host cartridge gate; build
ordering was corrected and the final DSP-1 lifecycle/resume passed. The earlier
normal/high-resolution checks are retained; both paths also passed after install.

No ROMs, save states, library settings or private screenshots are included.
