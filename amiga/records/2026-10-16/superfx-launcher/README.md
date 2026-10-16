# Super FX launcher integration

The MUI library enables Play for valid Super FX and FX3 headers. It launches
ZZSnes-FX, built with the previously tested compatibility timing/state overlay.
Standard cartridges still launch ZZSnes-ui. FX3 text is advisory. No game ROM
is modified; FX3-specific instructions are not implemented by this change.

Validation: all 256 cartridge-type IDs, header normalization and malformed
headers under ASan/UBSan; 20,000-entry library regressions; native core suite;
two-base ARM presentation checks and original fixture save/replay checks.
The original checkerboard fixture with FX3 types 17 and 18 also passed native
initialization and video/audio replay. This tests admission, not FX3 instructions.

Physical tests use an isolated library whose entries point to the existing
Star Fox 2 and Doom ZIPs. The first Star Fox 2 click preceded completion of
ROM identification and therefore did not start a player. The corrected UI
test waits for identification before clicking Play.

No performance or full special-chip save-state compatibility claim is made.
The published beta archive is unchanged.

All three physical UI launches (Doom, Star Fox 2, original FX3-header fixture)
opened the game window, exited with F10 and passed register/cache witness
restoration and owned-memory release. The fixture exercises admission only;
it contains no FX3 opcodes. The real game ROMs were untouched. The test library
closed at the end; normal library settings were not used by the tests.
