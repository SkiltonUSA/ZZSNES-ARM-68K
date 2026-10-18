# Standard-player save-state restoration

## Failure and cause

A Super Mario World state saved during the Smooth presentation check loaded
successfully in the same process, but a fresh player timed out before its first
frame. The previous optimisation player also failed with Smooth disabled.
The problem was therefore independent of the presentation filter.

Replaying the exact failing state in the Cortex-A9 instruction model exposed an
invalid read in `DrawOBJS`, while indexing sprite dimensions from `GFX.OBJList`.
These derived sprite lists and dimensions are not serialized. The upstream
snapshot loader invalidates them, but the supplemental IPPU state can restore
`OBJChanged = FALSE`. A fresh process then renders with an unbuilt list;
a same-process load can instead retain a list from a different scene.

## Fix

The compatibility overlay already calls `S9xSetupOBJ()` after restoring the
complete PPU/RAM snapshot. Fresh ARM builds now apply that snapshot fix to the
standard player as well. Builds stage a private core copy; archived source and
release payloads remain intact. This neither enables extra cartridge types in
the standard player nor changes the save format. The Super FX player already
contained the same restore call.

`make current` without `--arm` still uses the archived payload. To include the
fix, rebuild with `scripts/build_current.py --arm --optimisation --core-o2`;
add `--superfx` for the supplemental player.

## Regression checks

`make test-state-sprites` generates an original 32 KiB test ROM and saves a
scene with one visible sprite. It clears or replaces the derived sprite data
before loading, then checks the restored list, terminator, dimensions and
positions. The archived core fails this exact check (expected exit 42).
The standard snapshot-only fix and supplemental core pass, with exact video
and PCM replay for the next 30 frames, under ASan/UBSan. This test also runs
through `make test-native`.

`tests/amiga/pocketsnes/check_current_state.py` accepts `--state` for a private
existing checkpoint. It checks fresh-process and repeated restoration at two
ARM relocation bases, exact 30-frame video/stereo PCM replay, and rejection of
corrupt or wrong-ROM states. Commercial ROMs and states are excluded from Git.

## A4000TX results

The exact previously failing state resumed for **300 frames** with zero reported
audio gaps. An earlier good state also completed 300 frames with zero gaps.
Both used software scanlines and the FPS overlay; the failing checkpoint used
Smooth scaling. Each completed processor/MMU/cache witness restoration,
returned exit zero and released its owned memory.

An interactive run then completed three paused save/load cycles, F9 snapshot,
four window/fullscreen switches and F10 exit. It recorded 950 emulated frames
and three audio gap events; this is a functional lifecycle check, not a claim
of uninterrupted audio. The new save then resumed in a fresh process for
300 frames with zero gaps and normal restoration/release.

The fixed standard player is installed as **0.6.2-dev**. Its SHA256 is
`9dd6e23ae4d42f84e128c314fde085e0e91ae03e62c491fb67b34211042504c7`.
The previous player was preserved for rollback. A final 120-frame resume from
the installed drawer passed with zero gaps, normal restoration and exit zero. The rebuilt supplemental
player is byte-identical to its previous installed build and was not replaced.
Launcher, library preferences, ROMs and existing user states are unchanged.

[Raw logs and build hashes](../amiga/records/2026-10-18/state-restore/).
The original failing logs remain in the earlier follow-up evidence. No firmware,
global AHI or saved Workbench screen preferences were changed. Tagged Beta
release downloads are unchanged. Complete special-chip state support remains
unfinished; these results do not establish every game's compatibility.
