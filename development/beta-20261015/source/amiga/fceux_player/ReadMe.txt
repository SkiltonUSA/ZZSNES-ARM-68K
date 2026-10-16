ZZFceux 0.2.0-rc1 — ZZ9000 NES player
====================================

Experimental release for the tested A4000TX, AmigaOS 3.2.3,
ZZ9000 XX19c / XACP 1.7, Zorro III and enabled ZZ9000 Fast RAM.
Needs Picasso96API.library and a 320x240 32-bit RTG screen mode.
Audio uses AHI unit 0. lowlevel.library V40 enables optional joystick input.
No SDL dependency. ARM CPU/PPU/APU and direct framebuffer drawing;
68k OS/input/audio coordination. Close other Core1 applications first.

INSTALL / START
Extract the complete drawer. Double-click ZZFceux, acknowledge Start, then
choose a cartridge from ROMs. The icon supplies the required 65536-byte stack.
If extracting ZIP loses executable protection, in Shell use:
  Protect ZZFceux +e
  Stack 65536
  ZZFceux
Cancel the file requester using its Cancel button.

Shell alternatives:
  Stack 65536
  ZZFceux "ROMs/your-game.nes"
For a captured log:
  ZZFceux "ROMs/your-game.nes" >ZZFceux.log

CONTROLS
Arrows: directions; Z/X: A/B; Return: Start; left Shift: Select.
P: pause/resume; M: mute/unmute; Q or Escape: return to Workbench.
Physical joystick port 2 (lowlevel port 1): directions, red/fire=A,
blue/second button=B; CD32 green=Select, play=Start. Keyboard remains usable.
Unknown/mouse devices are ignored; no forced port type or hardware pokes.
Joystick state is sampled after each flip drains, used on the following frame.
Physical controller mapping still needs owner acceptance on this machine.

SUPPORT / LIMITATIONS
NTSC iNES mapper 0 or 90, CHR ROM; no trainers, batteries or four-screen mode.
Not arbitrary NES compatibility; no SNES, save states or battery saves.
Old FCEUX PPU, eight-sprite limit and quality-0 32kHz mono audio.
The included Controller-Test.nes is an original GPL test fixture: patterned
background, movable sprite and a pulse tone. It is not a commercial game.
Add your own supported ROMs to ROMs; no commercial games are supplied.

RESULTS ON A4000TX
Mario Bros: 1800 frames, scripted Start at frames 90–99, 55.47 FPS overall;
31 sampled native frame/cycle matches, all 958349 audio samples completed.
Super Mario World (mapper 90): 300 startup frames, 54.26 FPS.
Timing includes first-frame readback and final audio drain. These are bounded
workloads, not whole-game compatibility or steady 60 FPS claims.
ARM presentation remains about 1.15 ms. This release adds input, packaging
and timing instrumentation; it does not establish a further speed improvement.
Completed AHI requests do not establish audible quality or exact A/V sync.
Prior accepted ZZFceux-fast remains the fallback; this binary is a prerelease.

AUDIO RESEARCH
Playback still uses AHI unit 0 (Paula on the tested machine). ZZ9000AX modes
cafe0000/cafe0001 were enumerated and separately allocated/released at 32kHz
without changing global preferences. No AX PCM playback was tested or added.
A future per-application AHI backend can select AX; direct Core1-to-AX needs
a documented ownership/lifetime interface compatible with the private MMU.
Do not treat XACP's decoder output ring as an application-owned AX DMA ring.

DEVELOPER DIAGNOSTICS
  ZZFceux <fresh-nonzero-hex-nonce> RUN "ROM.nes" 300 VERIFY
VERIFY checks complete framebuffer on frame 1 and each 60th; it is slower.
  ZZFceux <fresh-nonzero-hex-nonce> RUN "ROM.nes" 1800 INPUT
INPUT overrides live controls with deterministic Start at frames 90–99;
intended only for comparison with the supplied native reference.
FRAME_TIMING reports stages 0=request, 1=flip drain, 2=audio preparation,
3=presentation, 4=whole loop, 5=observed flip-ACK interval. First 60 frames
are excluded. Bin upper bounds: 1/8/17/20/34 ms, then above 34 ms.
Stages overlap ARM/audio work; do not sum host durations as ARM utilisation.
ACK observation times include host scheduling and are not exact scanout times.

CREDITS / SOURCE
XANI / Xanxi-Amiga: XACP platform and ZZPicoDrive deferred-PAN guidance.
https://github.com/Xanxi-Amiga/XACP-ZZ9000
FCEUX contributors / Rezzy-dev: CPU/PPU/APU core (GPL-2.0-or-later).
Our integration, launcher, original test ROM and artwork: GPL-3.0-or-later.
No firmware or PicoDrive source is included. See COPYING3, FCEUX-COPYING,
ThirdParty.txt and COPYING.RUNTIME. Corresponding source and evidence:
https://github.com/SkiltonUSA/ZZNes-ARM-68K
