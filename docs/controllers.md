# Controllers — 0.7.9 development build

**Physical SN30 Pro and Mega Drive button tests are pending.** Decoder and
transport-model tests pass; the A4000TX passes USB-absence cleanup and the
640×480 MUI layout check. These options are not in the published 0.7.8 package.

In **Controllers**, select the player-1 source and use **Save settings**:

| Source | Behaviour |
|---|---|
| Auto: USB / Mega Drive | Prefer a supported SN30 Pro over USB; otherwise use the Mega Drive pad. |
| USB: SN30 Pro | Wired D-input, through Poseidon 4 or later. |
| Mega Drive 6 | Gameport 2 through a passive adapter swapping pins 5 and 7. |
| Amiga / CD32 | Existing lowlevel.library mappings, including configured Poseidon mappings. |

Existing installations retain **Amiga / CD32** until a source is selected.
Auto falls back to the Mega Drive input if Poseidon or the SN30 is absent.
The controller diagram uses the same decoder as the player. Testing is suspended
while another emulator holds the hardware. Existing keyboard mappings and
player-2 options remain available.

## SN30 Pro

For the Bluetooth-model SN30 Pro used **with a USB data cable**, start it in
D-input with **B + Start**, then connect the cable. Bluetooth dongles and X-input
are not supported by this path. The current USB profile recognises
`2dc8:6001`; other identifiers require a profile check.

A/B/X/Y/L/R/Start/Select follow the printed Nintendo labels. The D-pad and left
stick control directions. L2/R2, stick clicks and Home are not assigned.

Poseidon's standard joystick interface exposes seven button flags. The new
path temporarily claims this specific HID device to retain all eight SNES
buttons. It reads the device's report descriptor, checks the supported fields,
and uses asynchronous interrupt input. It does not install a USB class or alter
saved Poseidon settings. Closing the tester/player releases its binding and
asks Poseidon to restore normal class handling.

## Mega Drive six-button pad

This option requires the **passive 5/7 adapter**, not an unmodified direct
connection or a Mega Drive-to-CD32 converter. A CD32 converter uses the
Amiga/CD32 option instead.

| Sega | SNES |
|---|---|
| B / C / A / X | B / A / Y / X |
| Y / Z | L / R |
| Start / Mode | Start / Select |

The TH sequence identifies the extended buttons. A three-button pad exposes
only its available controls. `potgo.resource` reserves port-2 pins; TH is driven
while C/Start uses the hardware manual’s button-sense pull-up (OUT and DAT high). Port 1 and the potentiometer START bit
are untouched. Physical timing and adapter compatibility still need testing.

## Development and validation

- `make test-controllers`: ASan/UBSan decoder, configuration and transport tests.
- USB model: every mapped button, hat/stick, truncated reports, missing driver,
  absent/busy device, descriptor/allocation/IO failure, disconnect/reconnect,
  request draining and resource release.
- Sega model: signature and all buttons, actual polling phases, partial resource
  allocation, register masks, interrupt restoration and cleanup.
- Physical A4000TX: Poseidon present, SN30 absent, zero input and clean exit.
- Physical MUI: four source choices, seven pages, scrolling and resizing pass
  within a 640×480 budget. No gameport or ARM execution during these checks.

Controller changes are confined to the 68k launcher/player. The candidate uses
the **unchanged 0.7.8 ARM payload**; no emulation, audio or performance claim is
made from these controller tests. `build_current.py --release-ax
--reuse-arm-from /path/to/0.7.8/ZZSnes-ui` accepts only the recorded release
digest and preserves its image, mapping probe and relocation table. The normal
ARM source-build path remains available. Keep launcher and both players together.

References: [8BitDo manual](https://download.8bitdo.com/Manual/Controller/SN30pro%2BSF30pro/SN30-Pro-8.pdf),
[SDL controller profiles](https://github.com/libsdl-org/SDL/blob/SDL2/src/joystick/SDL_gamecontrollerdb.h),
[Poseidon HID source](https://github.com/aros-development-team/AROS/blob/master/rom/usb/classes/hid/hid.class.c),
[Amiga hardware manual](https://amigadev.elowar.com/read/ADCD_2.1/Hardware_Manual_guide/node018B.html),
[ADoom Sega protocol reference](https://github.com/mheyer32/ADoom/blob/master/adoom_src/amiga_sega.s).
