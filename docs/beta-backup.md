# ZZSNES 0.4.0 Beta backup

12 October 2026. This is a source/executable development backup, not an Amiga
SD-card image. No commercial ROMs, user saves or personal Library.cfg are included.

- `artifacts/beta-0.4.0/`: exact verified installed application/player,
  Workbench icon, credits and licences. No new GitHub Release was published.
- `development/beta-20261012/source/`: current editable adapters, shared MUI,
  cadence/compatibility/presentation code, relevant scripts and native tests,
  retaining workspace-relative paths. This snapshot supersedes earlier UI and
  scheduler source for the current Beta; legacy root build targets are preserved.
- `development/beta-20261012/build-inputs/`: exact staged C/headers and ARM ELF
  consumed by the tested Beta build. This makes the 68k build independent of
  the original workstation’s ignored source caches.
- `docs/beta-sha256.json`: hashes of every archived source/build input and
  installed-package file. `docs/beta-build.json`: pinned compiler image.
- `amiga/records/2026-10-12/`: cadence A/B timing, raw logs, migration hashes and
  production lifecycle evidence. Historical paths/versions there are evidence.

## Verify and rebuild

```sh
make verify-beta
make beta
```

The default container command is Podman with the original Mac connection name.
To use Docker instead:

```sh
BETA_CONTAINER_COMMAND='["docker","run","--rm","--platform","linux/amd64"]' make beta
```

Outputs go to `.context/beta-rebuild/`. Rebuilds embed different debug paths;
use the archived executables for exact installed bytes. This rebuild retains
the archived hardware-tested ARM payload. It does not regenerate the emulator
core or promise a complete clean-room ARM rebuild from this convenience target.
The original core vendors/pins and older rebuild tools remain in the repository;
workspace scripts in the snapshot document how the later stages were generated
and may still require the documented intermediate caches and toolchain.

The owner’s installed location is `SD032G:Emulator/ZZSNES`. The previous
`SD032G:Dev/` application drawers remain recovery copies. ROMs were renamed
for short-name filesystem safety and personal configurations were migrated;
those personal data files are excluded from Git. Three NES names were then
corrected to preserve `(E)` PAL detection tags; see the PAL path-fix record.

No new hardware benchmark was run for this backup. The README’s timing figures
are recomputed from the already recorded A4000TX tests. Near-60 emulation FPS
is distinct from approximately 58 submitted pictures/s in the NTSC tests.
