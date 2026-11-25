# Offset-background renderer experiment

An isolated candidate specializes SNES background modes 2/4 and 8/16-pixel tile
geometry once per drawing call. The compiler can fold constant branches and
address calculations inside each tile loop. Both modes preserve their different
horizontal/vertical offset behavior. No scanlines, pixels or guest cycles are
omitted; colour math, clipping, tile caching and saved-state formats are unchanged.

The transformation is `offset_specialization` in
`scripts/prepare_ppu_experiment.py`. Build with:

```sh
python3 scripts/build_current.py --release-ax --ppu-test offset \
  --output-dir .context/offset-candidate
python3 scripts/test_offset_renderer.py --rom /path/to/game.sfc --frames 1200
```

Use the project's ARM/68k toolchains. Output must be a fresh isolated directory.
This experiment is separate from both GSU-fetch candidates; no combined gain is
claimed. [Build identities](builds.json) show only `core/src/gfx.c` changed among
core inputs, plus the generated player payload. Production remains 0.7.13.

## Validation

- 2,048 randomized native cases match screen and depth buffers exactly: modes
  2/4, 8/16-pixel tiles, scrolling/page wrap, clipped spans, priority, flips and
  colour-math combinations. Address/undefined-behavior sanitizers pass.
- 1,200 frames each of Star Fox 2, Stunt Race FX and Doom match native video,
  stereo PCM and serialized GSU state exactly.
- Thirty ARM-model frames from Star Fox 2 frame 600 and a Stunt Race FX race
  checkpoint match video/PCM/GSU state at two candidate relocation addresses.
- Three modeled Star Fox 2 frames use 41,036,751 instruction slots versus
  42,193,923 for the reference (**2.74% fewer**). The offset renderer itself
  decreases from 8,459,202 to 7,302,030 slots. Output traces agree. This metric is
  not A9 cycles, FPS or proof of audio continuity.

Commercial ROMs/checkpoints are not included. Native and model logs are beside
this report. [Nine hardware runs](hardware.md) show 1.69% lower Star Fox 2 core
time but 0.53% higher Stunt Race FX core time. Audio starvation remains. The
candidate stays experimental; the production player is unchanged.
