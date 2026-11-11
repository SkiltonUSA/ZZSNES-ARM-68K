# Renderer and GSU investigation

This record separates physical timings from native/model correctness tests.
Installed release players and user preferences were not replaced.

## Renderer: repeated physical result

Star Fox 2, NTSC, same private frame-3000 checkpoint, 660 emulated frames,
Paula AHI mode `0002000e`, window scale 2, software scanlines mode 2,
default core options. Run order: reference, rows, rows, reference.

| Build | Run | ARM core time (s) | Audio starvation events |
|---|---:|---:|---:|
| Reference | 1 | 9.759496 | 130 |
| Rows | 2 | 9.202681 | 108 |
| Rows | 3 | 9.202347 | 106 |
| Reference | 4 | 9.759891 | 129 |

Mean core time falls **5.709%**, from 9.7596935 to 9.202514 seconds.
This is a reduction in core execution time, not a 5.709% game-FPS claim.
Presentation work is separate. All four runs delivered 352,913 PCM frames,
completed without display faults, restored registers/MMU/cache witnesses and
released allocated memory. Audio starvation persists; `AUDIO_END gaps=0` does
not override the host's nonzero starvation counters.

Chrono Trigger's 1,200-frame opening sequence shows no material change:
9.207992 seconds reference versus 9.207110 seconds rows. Both delivered
641,658 PCM frames with zero starvation and clean teardown. This opening
sequence is not representative of all gameplay scenes.

## GSU experiment

The opt-in counter change and its invariants are described in
[the development note](../../../../docs/renderer-gsu-review.md).
Do not combine model instruction counts with hardware timing percentages.

For the same 30-frame Star Fox 2 checkpoint sequence:

| Model build | ARM instruction slots | Slots in `fx_run` |
|---|---:|---:|
| Reference renderer | 196,763,388 | 8,173,125 |
| Rows renderer | 186,859,829 | 8,173,125 |
| Rows + local GSU counter | 186,205,991 | 7,519,287 |

The counter experiment removes 653,838 modeled slots, about 0.35% of the
rows build's total. All three output trace SHA-256 values match. The disassembly
shows the counter load/store removed from the opcode loop; additional register
saves occur at function entry/exit. These are not measured A9 cycles.

### Physical counter comparison

Same checkpoint, 660 frames and settings as above; a fresh rows/counter/counter/
rows sequence isolates the additional counter change:

| Build | ARM core time (s) | Audio starvation events |
|---|---:|---:|
| Rows | 9.202610 | 106 |
| Rows + counter | 9.184374 | 105 |
| Rows + counter | 9.183732 | 104 |
| Rows | 9.203969 | 106 |

Mean core time falls **0.209%** relative to rows. This small repeatable difference
does not establish a noticeable gameplay improvement. Audio starvation remains.
All four runs deliver 352,913 PCM frames, finish with exit 0, restore the
register/MMU/cache witnesses and release memory. No display faults are reported.
Keep the counter option experimental pending broader hardware workloads; do not
add the separately measured percentages to claim a universal combined gain.

## Correctness coverage

- Six additional ROMs, 1,200 frames each: renderer video/PCM hashes match.
  Synthetic raster coverage includes flips, clipping, transparency and depth.
- 33,280 GSU counter boundary cases pass with ASan/UBSan.
- Star Fox 2, Stunt Race FX and Doom: 3,600 frames each with identical per-frame
  video/PCM/serialized-GSU-state hashes in the native harness.
- Star Fox 2 and Stunt Race FX: 30-frame ARM video/PCM/GSU-state comparisons at
  two relocated addresses pass.
- Native core suite and archived-artifact verification pass.

Build hashes, modeled profiles, disassembly and compact check summaries are
alongside this record. No ROMs, checkpoints or captured game audio are included.

## Test interruptions

Two setup attempts were excluded: a raw ROM incorrectly supplied to the ZIP
harness, and an overlong test tag. A reused battery-save path then caused the
player to refuse a different game's save; the Chrono tests were moved into a
separate directory. No game save was overwritten by that refusal.

The bridge intermittently timed out while monitoring Chrono Trigger. Fresh
read-only checks later established Core1 was free; recovered logs confirmed
exit 0 and clean teardown for both reference and rows. No reboot, forced worker
stop or firmware change was used. These monitoring failures remain recorded;
they do not establish a player crash or a fix for bridge responsiveness.
