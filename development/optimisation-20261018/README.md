# Optimisation candidate

Editable overlay for the current beta source tree. The default release rebuild
and archived release files are unchanged. This candidate is 0.6.2-dev.

Build: `python3 scripts/build_current.py --arm --optimisation --core-o2`.
Run `make test-optimisation-native`; see `docs/optimisation-20261018.md` for
relocated ARM checks, physical evidence and limits. Builds never deploy.

- Split presentation rows into left border / game / right border spans.
- Build with GCC `-O2` instead of `-Os` when `--core-o2` is selected. The
  soft-float ABI and numerical rules remain unchanged; no fast-math.
- Includes the existing Thin scanline overlay. Add `--superfx` to build the
  supplemental FX player, retaining its advisory FX3 warning and limitations.
- Add `VERIFY` to bounded session launches for active-frame readbacks.
- Cache display titles for sorting; fall back if temporary allocation fails.
- Batch hex serialization and parse bounded numeric/hex tokens without a
  formatted scan per record. No fixed library count limit is introduced.

The cores, XACP ownership and teardown contracts are retained. These changes do
not import DOSBox's x86 JIT, VFP scheduler or TinyGL matrix routines.
