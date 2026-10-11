# SNES word-memory optimisation — 10 October 2026

[Implementation/results](../../../mesen_snes/README.md#nes-style-word-memory-optimisation-10-october-2026)
and [ARM core reference review](../../../mesen_snes/ARM-CORE-REFERENCES.md).

An independently written aligned scalar memcpy/memset carries over the NES
principle of wider memory/pixel operations. Byte edges and mismatched alignment
retain byte handling. No emulation algorithm, frameskip, audio shortcut,
VFP/NEON or firmware/cache/layout change was made. Native ASan/UBSan memory
checks and 3,096 bounded A9 memory calls pass; disassembly has no VFP/NEON.

| Physical run | Mean ms per frame |
| --- | ---: |
| Fresh profile baseline | 40.7997 |
| Word-memory profile | 39.7370 |
| Word-memory profile repeat | 39.7636 |
| Fresh uninstrumented baseline | 40.7010 |
| Word-memory uninstrumented | 39.9452 |
| Installed MUI System button | 39.9473 |

Each mean uses ten six-frame batches after a warmup. First partial startup
frame included; setup, final validation and destruction excluded. The final
non-profiled improvement is about **1.9% less time**. Do not use the larger
profiled delta as the installed performance claim. This is a simple silent,
headless fixture; game FPS and full-speed SNES remain unproven.

Both new builds passed two relocated A9 model tests, pixel corruption rejection
and cancellation. Physical logs validate original pixels, master/CPU/SPC cycles,
silent PCM, active cancellation, RET1, all 14 registers, MMU/cache witness and
allocation release. Common ARM regression/MCP smoke pass. No crash/reboot.

Installed under SD032G:Dev/ZZSnes-Mesen:
- Normal ZZSnes-Mesen-System now contains System-Words: 303,456 bytes,
  MD5 f1a45b2fe4c9e5f73b1e33cf7951dfd8.
- ZZSnes-Mesen-System.before-words retains the 301,660-byte original,
  MD5 0c3b1311be30a1b368b36c2323d7a2bd.
- Explicit ZZSnes-Mesen-System-Words copy also retained.
- ZZSnes-Mesen-Profile-Words: 304,776 bytes,
  MD5 0399ed78c76708ee333ea852dfb3628d.
- Original system manifest/readme backed up with before-words names.
  Current system-build.json corresponds to the System-Words output name.

MUI/CPU/NES executables are unchanged. The existing System button passed and
its screenshot was visually checked. Detailed manifests retain source/output
hashes; no GitHub publication was requested.

The three supplied ARM core references were reviewed separately. The ARMSNES
assembler probe JSON records a narrow syntax/target check, not a port or runtime
benchmark. Its generated adapted source stays in the research directory; no
Snes9x core code was copied into this Mesen integration. ARMSNES/PocketSNES is a
candidate for a separate headless comparison; DrPocketSNES ASMCPU is another
lead, with deferred-raster compatibility trade-offs requiring explicit testing.
