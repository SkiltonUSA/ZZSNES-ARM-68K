# ZZSNES 0.7.3 Beta validation

This release installs matching **0.7.3-dev** launcher/player executables. Normal
regional cadence, input, state saves and screenshots are enabled. The AX backend
is automatic, with configured AHI unit 0 as fallback. No global AHI edits.

[Build command and tested executable hashes](release-build.json).

| Asset | Bytes | SHA256 |
| --- | ---: | --- |
| `ZZSNES-0.7.3-dev-beta.zip` | 1117730 | `653770c299e23509a34574558a6992820271ab4eee6aeb4c95c45b65d9336555` |
| `ZZSNES-0.7.3-dev-beta-amipkg.lha` | 3024206 | `5d0acb14d2f77c06ece037bdebc59199d110dfe252289ab6702748b55a538222` |
| `ZZSNES-release.json` | 1116 | `498eaecedd3f7d8a79c0996afe018d460c5f145377b5ac4e35e933df3dedfa82` |

## Checks

- Current-source hash verification passed after refreshing four stale manifest
  entries for reviewed documentation/audio changes and the build selector.

- Both full release builds pass, with ARM image/MMU headers identical to the
  previously tested register-dispatch builds. Version strings match across each
  launcher/player and the additional SNES FX player.
- Native AX ownership, worker and API-failure sanitizer checks pass. Release
  selection rejects benchmark-only scheduling/polling/batching flags.
- Two 180-frame physical VERIFY runs pass four exact framebuffer checks each,
  complete PCM accounting, register/MMU/cache witnesses and owned-memory release.
- Both normal players pass F9 capture, paused save/load, window/fullscreen
  switching, resume and quit. User input and snapshots are active in this build.
- ZIP and nested LHA payloads match byte-for-byte; executable/member hashes,
  signed catalogue verification, corrupted-signature rejection, prerequisites,
  icons and ROM/save/config/private-key exclusions pass.

Both exact offline installer packages were installed successfully on the A4000TX.
Dependency checks and catalogue signatures passed; installed executable hashes
matched the tested release builds. Existing library/session/controller settings
were byte-identical before installation and after opening/closing both launchers.
The launchers displayed the new versions and existing libraries on Workbench.
Both icons specify a 64 KiB stack. A first CLI attempt without that stack was
safely rejected; retrying with `Stack 65536` passed.

The private test drawer was removed. Core1 is idle, the bridge responds, and
global AHI / saved screenmode preference checksums are unchanged. Existing
scanline tools were preserved. Original application files and settings were
backed up locally before installation. ROMs and existing saves were not changed.


The initial NES test harness looked for a version string in the runtime log,
which the host does not print. The executable version and transfer checksum were
already verified; the log assertion was removed, and its successful run retained.

The earlier +3.2% NES / +4.6% SNES FPS figures describe controlled short audio
comparisons, not a new release-wide performance guarantee. These functional runs
include deliberate readbacks, file writes and pauses and are not FPS benchmarks.
Listening/long-session validation remains pending. The earlier unexplained NES
freeze remains unresolved. This is a beta release.

[Hardware logs and checks](../amiga/records/2026-10-29/ax-release/).
