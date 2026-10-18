# ZZSNES 0.6.2-dev: standard-player state fix

This is the standard player component for the existing 0.6.2-dev development
installation, not a complete installer or a new tagged Beta. Back up the
previous `ZZSnes-ui` before replacing it; keep the matching launcher and other
files. Existing ROMs, states and preferences are retained.

The supplemental `ZZSnes-FX` player is unchanged. Rebuild with
`scripts/build_current.py --arm --optimisation --core-o2` using the documented
ARM/68k toolchains. See [the restore report](../../../docs/state-restore-20261018.md)
and [test records](../../../amiga/records/2026-10-18/state-restore/).

No ROMs or save files are included. Existing Snes9x/PocketSNES license terms apply.
