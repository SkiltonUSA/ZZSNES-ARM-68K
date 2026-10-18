# ZZSNES — 0.6.1 Beta

<img src="docs/screenshots/logo.png" width="160" alt="ZZSNES logo">

A native **MUI** game library for **AmigaOS and ZZ9000**, using **PocketSNES / Snes9x2002**
and **[Xanxi’s XACP platform](https://github.com/Xanxi-Amiga/XACP-ZZ9000)**.
The Cortex-A9 runs emulation and fullscreen rendering. The 68k handles AmigaOS,
files, controls, display coordination and batched AHI audio.

**[Download 0.6.1 Beta](https://github.com/SkiltonUSA/ZZSNES-ARM-68K/releases/tag/v0.6.1-beta)** ·
[User guide](docs/current-guide.md) · [Changes](CHANGELOG.md) ·
[Build and measurements](docs/current-development.md) · [Interface gallery](docs/current-interface.md)

![ZZSNES interface](docs/screenshots/interface.png)

## Features

- ZZDosBox-style sidebar: Games, Display, Controllers, Save states and About.
- Colour logo, Workbench icon, startup panel and loading panel with controls.
- Single-ROM ZIP loading (stored/deflate); no external unpacker required.
- Add ROMs or folders; search, favourites, recent games and captured artwork.
- Rename titles and delete games with confirmation.
- Disks: scan a chosen directory or mounted drive once, including USB storage.
- Red Launch button available on every launcher page.
- No fixed ROM-count limit; tested with 20,000 entries per library.
- Fullscreen or Workbench window; **Alt+Return** switches during play.
- Optional FPS overlay and **Classic, Soft or Gradient** software scanlines.
- Monitor aspect correction, borders including EmuELEC, overscan masks and joystick testing.
- Five save slots per game; save, load, resume and delete.
- Guarded NEON stores and scanline dimming, with an integer fallback.

Save-state previews include miniature screenshots.

## Software scanlines

The scanline effect is built into the emulator and rendered by the ZZ9000
Cortex-A9. **Xanxi’s separate ZZScanlines utility is not required**, and the
effect does not require additional FPGA scanline support. XACP remains required
for the emulator itself.

- **Off:** unfiltered output.
- **Classic:** alternating black rows.
- **Soft:** alternating dimmed rows, retaining more fine detail.
- **Gradient:** a repeating four-row brightness pattern.

Select **Display → Software scanlines** (labelled **Scanlines** in older builds).
The routine is shared with ZZDosBox and works in **fullscreen and Workbench
windows**, including with the **FPS overlay** enabled.

The separate development build also offers **Thin (640×480)**, preserving each
logical image row with a one-pixel black separator. [Thin mode details](docs/thin-scanlines.md).

If Workbench’s global scanline effect is enabled, turn it off to avoid applying
both effects together. Software scanlines affect this emulator’s image only.

## Requirements

- **ZZ9000** with compatible **XACP firmware**; tested on **XX19c / XACP 1.7**.
- **Picasso96**, **AHI**, and **MUI 3.8 or compatible**, installed separately.
- A writable application drawer and your own ROMs.
- AmigaOS and a Zorro III Amiga; host executables target the **68020 instruction set**.

**A TF4060 or 68060 accelerator is not required.** The ZZ9000 Cortex-A9 runs
the emulation. A faster 68k helps file access, input, window updates and AHI
audio servicing; it does not increase the ARM clock speed.

**Tested configuration:** A4000TX, TF4060/68060, AmigaOS 3.2.3.
The 68020 build target is not a measured minimum-speed guarantee. These builds
have not yet been validated on a 68030, and the published FPS results apply
to the tested configuration.

The SNES core reserves 32 MiB of ZZ9000 RAM, plus display and OS allocations.
Only one Core1 application can run at a time.

## Performance

An opt-in **0.6.2-dev optimisation candidate** adds faster ARM presentation, an O2 core build, faster large libraries and optional NEON smooth scrolling.
[Measurements and validation](docs/optimisation-20261018.md) · [Follow-up testing](docs/stability-20261018.md).
New libraries default to fullscreen with Soft software scanlines; the loading panel lists **F9: Screenshot**. Existing release downloads are unchanged.

The established steady emulation measurements remain:

| Game / emulator | Steady emulation FPS |
| --- | ---: |
| Contra — ZZNES | **59.893** |
| Super Mario World — ZZSNES | **59.920** |

Repeated 600-frame comparisons measured **16.9% less ARM presentation time with Soft scanlines**.
Plain output retains the faster existing STMIA copy path.
These are presentation-stage savings, not equivalent increases in game FPS.
[Method, raw logs and limitations](docs/current-development.md).

![ZZSNES running in a Workbench window](docs/screenshots/window.png)

## Compatibility and credits

Automatic PAL/NTSC timing for supported standard SNES cartridges.
[Cartridge restrictions and tested games](docs/amiga-zzsnes-pal-compatibility.md).
[Expanded-memory comparison](docs/snes-memory-compatibility.md) · [Cartridge test findings](docs/cartridge-survey.md).
The published beta supports standard cartridges. The development launcher also
starts Super FX games through the experimental player. FX3 shows an advisory
warning and permits a launch attempt. The supplemental development core now
implements [experimental FX3 behaviour](docs/fx3-development.md), informed by
MesenCE; commercial FX3 gameplay has not been validated.
SA-1 remains disabled.
[Compatibility development and current test findings](docs/compatibility-development.md).
Audio: 32,040 Hz stereo through AHI.

Development: **Codex assisted, Amiga-Bridge MCP**. Interface: **MUI**.
Thanks to **Xanxi** for XACP, and the **PocketSNES / Snes9x2002** contributors for the core.
The software scanline effects and NEON presentation work are shared with ZZDosBox;
they do not require the separate ZZ9000 scanline application or an FPGA change.

[Thin scanlines candidate](docs/thin-scanlines.md): 640×480 output preserves every
image row and inserts narrow black separators. Separate from the published beta.

ZIP decompression uses Mark Adler’s **puff** from zlib; its notice is included.
Component licences and notices are included. The Snes9x core is non-commercial.
