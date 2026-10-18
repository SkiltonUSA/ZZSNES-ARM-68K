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
- [ROM Manager](docs/rom-manager.md): bulk folder import, duplicate-entry cleanup and verified save-state backups to disk or USB.
- Live ZZ9000 temperature beside the launcher version.
- ROM header region beside the preview; automatic PAL **50 fps** / NTSC **59.92 fps** pacing in the development build ([details](docs/region-pacing.md)).
- Red Launch button available on every launcher page.
- No fixed ROM-count limit; tested with 20,000 entries per library.
- Fullscreen or Workbench window; **Alt+Return** switches during play.
- Optional FPS overlay; fullscreen FPGA scanlines and windowed Soft scanlines in the development package.
- Monitor aspect correction, borders including EmuELEC, overscan masks and joystick testing.
- Five save slots per game; save, load, resume and delete.
- Guarded NEON stores and scanline dimming, with an integer fallback.

Save-state previews include miniature screenshots.

## Scanlines

[Download the development package](artifacts/development/region-20261018/ZZSNES-0.6.2-dev-region.zip).

The current development package uses **Xanxi’s FPGA scanlines in fullscreen**
and **Soft software scanlines in a Workbench window**. The two effects do not
run together. Choose **Display → Scanlines**; **F7** cycles
Off → Soft → Gradient → Classic in fullscreen and toggles Off/Soft in a window.
**Alt+Return** switches display modes. Hardware scanlines are disabled on normal
exit, rather than restoring a previous global setting.

Double-click the bundled **Install-Scanlines** to add missing `ZZScanlines` and
`ZZScanlinesGUI` controls. It preserves existing tools in `SYS:Utilities/ZZScanlines`
or `C:`. Both packages include the controls, installer, source and GPL license.
**Compatible FPGA scanline firmware is required for the fullscreen effect**;
the installer does not update firmware or startup files. The emulator uses the
register interface directly; the separate utility need not be running.

Thanks to [Xanxi’s zz9000-scanlines](https://github.com/Xanxi-Amiga/zz9000-scanlines)
for the hardware interface and bundled tools. Windowed Soft filtering remains
our ARM software routine shared with ZZDosBox. Previous tagged beta downloads
use software scanlines in both modes; those archives have not been replaced.
[Automatic package-manager installation (experimental)](docs/package-manager.md) · [Installation and controls](docs/scanline-install.txt) · [Integration report](docs/fpga-scanlines-20261018.md).

Hardware filtering occurs after framebuffer readback, so F9 screenshots do not
capture the FPGA effect. The following images show the earlier software effect.

### Games with Soft scanlines

Captured on real Amiga/ZZ9000 hardware using **0.6.2-dev**, fullscreen and Sharp pixels.

| Super Mario World | F-Zero |
| --- | --- |
| ![Super Mario World with Soft software scanlines](docs/screenshots/scanlines/snes-smw.png) | ![F-Zero with Soft software scanlines](docs/screenshots/scanlines/snes-fzero.png) |

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
[Measurements and validation](docs/optimisation-20261018.md) · [Follow-up testing](docs/stability-20261018.md) · [Save-state restore fix](docs/state-restore-20261018.md).
New libraries default to fullscreen with Soft scanlines; the loading panel lists **F9: Screenshot**. Existing release downloads are unchanged.

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
Software filtering and NEON presentation work are shared with ZZDosBox.
The current fullscreen scanline backend uses Xanxi’s FPGA interface as described above.

[Historical Thin scanlines candidate](docs/thin-scanlines.md): 640×480 output preserves every
image row and inserts narrow black separators. Separate from the published beta.

ZIP decompression uses Mark Adler’s **puff** from zlib; its notice is included.
Component licences and notices are included. The Snes9x core is non-commercial.
