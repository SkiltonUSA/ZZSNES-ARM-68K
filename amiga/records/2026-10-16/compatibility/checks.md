# Verification

- `make test-native`: passed original PocketSNES/Mesen fixture checks.
- `make verify-current`: passed archived source/artifact hashes; release unchanged.
- Candidate ARM build: passed; source byte comparison confirms the tested
  snapshot.c matches development/compatibility-20261016/core/src/snapshot.c.
- Relocated current presenter: passed (arm-present.log).
- Relocated original-fixture state replay: passed (arm-state.log).
- Baseline frame-600 controls: Imperium one video mismatch, Stunt Race FX 18;
  same-process and fresh-process failures. Candidate: zero for these checkpoints.
- Native extended survey: 44 restore checks; individual failures recorded,
  not converted into a suite-wide compatibility pass.
- Seven bounded physical runs and Imperium interactive state lifecycle: passed
  ownership/cache/register teardown; normal library installation retained.
