# Complete-frame stage profiling

The profiling diagnostic is a separate binary, `ZZSnes-Mesen-Profile`. It uses
the same six-frame original cartridge and correctness/cancellation/restoration
checks as the system test. It does not replace the installed MUI System test.

```sh
python3 scripts/build_mesen_snes_system_arm.py --profile \
  --container-command '["podman","--connection","nuflix-converter-root"]'
.context/amiga/mesen-bench/venv/bin/python tests/amiga/mesen_snes/check_system_arm.py \
  .context/amiga/mesen-snes-profile/system.elf --profile-clock
clang++ -std=c++17 -DSS_PROFILE -fsanitize=address,undefined \
  -Iamiga/mesen_snes/system amiga/mesen_snes/system/profile.cpp \
  tests/amiga/mesen_snes/profile.cpp -o .context/amiga/mesen-snes-profile/profile-test
.context/amiga/mesen-snes-profile/profile-test
```

Instrumented scopes are PPU scanline completion/rendering/register access and
SPC Run/ProcessEndFrame. Nested scopes of the same category do not read the
timer again. Cross-category nesting is accounted exclusively, so frame-end
SPC work is excluded from the PPU total. The unit test covers nested same- and
cross-category scopes, disabled timing, absent timer and 32-bit timer wrap.
The A9 model uses a synthetic clock callback to exercise instrumentation, not
to make physical timing claims. Physical timing uses the existing calibrated
ARM global timer; emulated clocks are unchanged.

Timing is active only inside RunFrame. Initialization, per-frame output checks,
final hashing and destruction are excluded. PPU and SPC totals include some
instrumentation cost; the residual includes CPU/bus/IRQ scheduling, loop work
and remaining profiling overhead. It is not pure 65C816 instruction time.
Read totals alongside an uninstrumented baseline to assess perturbation.
All DSP work called by the SPC is included in SPC; this pass does not isolate
each DSP voice or instruction. The fixture generates silence, not active music.

Profiling uses an opt-in generated copy of the worker. Its four counter words
and version marker occupy offsets 96..112 of the existing reply area. They are
published before the existing token/sequence acknowledgment; the 68k reader
pulls 128 bytes instead of 96. Allocation, MMU/cache settings, stop word and
common worker sources are unchanged. Model addresses are synthetic only.

After a fresh live ownership check and verified transfer, run from Shell with
`Stack 65536`, redirect to a log, then fetch the log through the existing bridge.
`python3 scripts/analyze_mesen_snes_profile.py <logs...>` validates signatures,
all restored registers, cancellation and stage accounting before summarizing.
