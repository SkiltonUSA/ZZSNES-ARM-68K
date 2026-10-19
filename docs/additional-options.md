# Additional ZZSNES options — 0.6.5-dev

This development update adds the remaining controls from the options review.
Use the matching launcher and both players; the 0.6.5 Beta packages this matched build.

| Feature | Location / behaviour |
| --- | --- |
| Automatic battery saves | Cartridge SRAM loads at startup and saves on a clean exit, beside the ROM or ZIP as `.sav`. Save states continue to include SRAM. |
| Per-game profiles | Games: **Save for game**, **Use defaults**, **Edit defaults**. Profiles retain display, emulation, controls and cheats for that ROM path. |
| Automatic frameskip | Display: **Automatic** responds to low audio supply; **Auto threshold** uses the selected buffer percentage. Game timing and PCM generation continue while pictures are skipped. |
| Filter strength | Display: low-pass on/off and **5–95%** strength, default 60%. |
| Transparency | Display: enabled by default. Disabling it changes graphics; no general performance gain is claimed. |
| Player 2 and mapping | Controllers: keyboard mapping for all 12 SNES inputs and seven gamepad buttons per player. Standard joystick directions remain directional. |
| Cheats | Save states: enable and enter Game Genie or Pro Action Replay codes separated by semicolons. Use **Save for game** to retain them. Maximum 32 codes. |

Changes take effect on the next launch. **Save settings** saves the current
scope shown on the Games page. **Save for game** creates or updates a profile;
**Use defaults** removes its active profile; **Edit defaults** edits global
settings without removing the selected game's profile. Selecting another ROM
loads its saved profile or the global defaults. Save edits before switching ROMs.
Cheats are never stored as global defaults.

Player 1 uses physical joystick port 2 and its configured keys. Player 2 is off
by default; assign its keys in the mapping table. Enable keyboard-only or physical port 1 plus keyboard; port 1 is
normally the Amiga mouse connection. Both inputs require suitable hardware for
extra buttons. Emulator hotkeys remain reserved and cannot be remapped.

Battery files are separate from the five save-state slots. A missing file leaves
the core's normal initial SRAM intact. Wrong-size or unreadable files stop the
launch rather than being overwritten. Changed data is written to `.sav.tmp`,
read back, then replaces `.sav` while retaining `.sav.bak`. This is exit-time
persistence, not periodic autosaving or protection against a reset during play.
For read-only ROM media, import the ROM into a writable directory first.

Profiles are small sidecar files; they do not add per-ROM structures to the
large library index. ROM Manager backups include profiles, controller settings,
battery files and existing saves/screenshots. Deleting a game also removes its
profile and profile backups. Renaming its display title preserves its profile.

## Implementation and verification

Automatic frameskip receives real producer-queue and outstanding AHI-request
occupancy from the audio task. It uses whole-block completion, not an exact DMA
sample cursor. Feedback is inactive before prebuffering, without AHI, and during
a state drain. The upstream automatic-skip limit still forces periodic pictures.
Fixed frameskip remains available. This does not increase emulated game speed.
The FPS overlay counts emulated frames, not distinct rendered pictures.

The expanded option word preserves old zero/default fields. Existing version-2
standard and version-3 expanded save states remain readable; new states store
the emulation options. Display resolution remains a presentation preference.
Controller mappings and cheat lists belong to the launch profile, not a state.
Older players cannot read states containing the new option bits.

The bounded cheat parser validates the whole list before changing active cheats.
It replaces an inherited parser that wrote its terminator beyond a variable-length
buffer. It also corrects reversed byte/address arguments in the inherited
mapped-memory cheat write/restore path, and rejects duplicate target addresses
before changing active cheats. Cheat addresses refer to emulated SNES memory, not Amiga or ARM memory.

Validation records are in `amiga/records/2026-10-19/options/`. Native/model tests
and physical results are recorded separately. Second-controller gameplay needs
a connected second controller; software input tests do not establish that result.

## Installed development build

The matched 0.6.5-dev launcher and players passed checksum verification on the
A4000TX. Library settings were unchanged during replacement and startup; the
previous executables were retained for rollback.

Native sanitizers and relocated ARM checks pass. Physical checks cover SRAM
persistence across processes, mapped-SRAM cheats, SMW save/load/capture/display
switches/exit/resume, and compact MUI profile/control operation. Audio gaps were
recorded; these checks do not establish a speed gain or uninterrupted audio.
This matched build is distributed as 0.6.5 Beta.
