# Mesen2 SNES CPU bring-up

Owner requested SNES review and setup as a separate Mesen application.
Sources/review: `amiga/mesen_snes/README.md`. No complete SNES emulator or
frame rate is established by this milestone.

Installed `SD032G:Dev/ZZSnes-Mesen/`, with native MUI launcher and icon,
CPU diagnostic, manifest/licenses/readme, and empty ROMs/States drawers.
Final binary checksums were compared with native Amiga MD5Sum output:

- ZZSnes-Mesen: 42,536 bytes, `daebe82ae5e5d182d54be1d4e393c89d`.
- ZZSnes-Mesen-CPU: 194,776 bytes, `cd3fb1fe04dc9aba419c4e29bbde6f56`.

`native.log`: ASan/UBSan smoke program, checked and timed paths agree,
cancellation after 128 instructions. `arm-model.log`: two relocated A9
executions agree, corrupted fixture rejected, cancellation works. This does
not model physical caches/XACP. `regression.log`: common ARM/MCP tests passed.

`hardware.log`: real Core1, 5,144 instructions / 15,436 emulated cycles,
signature e4b09881. Ten warm samples average 1,287.2 us (range 1,286–1,288).
No PPU/SPC/DMA/system bus; not a game FPS benchmark. Timer is ~333.33 MHz,
distinct from CPU clock. Setup and final RAM hashing are outside the timer.

`ui-hardware.log`: test launched by clicking the new Workbench window's
button; result displayed. Initial two-line status area was too short for its
four-line result. Fixed the initial MUI layout to reserve four lines.
`ui-hardware-final.log` and `workbench.png`: same test from the final UI binary,
readable result. Close gadget and Workbench icon reopen verified.

All three physical runs pass runtime mapping, hash/cycles, active cancellation,
RET1, MMU/cache restoration, 14 register comparisons and witness checks,
then release the owned allocation. No crash/reboot or firmware changes.
No commercial ROM or state data used. Existing NES library left open.

Final state: new SNES development window open; no Core1 owner; Roadie/bridge
alive. Temporary transfer server stopped and RAM-only Workbench helper removed.
No public repository/release created or modified in this task.
