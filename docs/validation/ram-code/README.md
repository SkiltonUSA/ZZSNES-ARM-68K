# SNES RAM-code and register-dispatch checks

Development validation of the isolated clipped-flip-build payload. No core or production
binary changed in this pass. The console uses generated register handlers,
not a guest-address block JIT; it has no guest FPU.

An original cartridge builds a subroutine in RAM, repeatedly switches its opcode
between INX and DEX, and checks wrap/sign-boundary results. It completes 256 pairs
of calls (512 operations) while recording real entries into generated ARM handlers.
Tests pass at two relocation addresses. A save state then restores deliberately
changed RAM code, followed by identical next-frame video and PCM output.

The common register emitter also passes all 8-bit and 16-bit values for decrement,
copy and increment, with four input flag patterns and three flag representations:
**2,368,512 arithmetic/flags/untouched-memory cases**. That exhaustive run uses the
NES harness; both repositories have the same emitter bytes, recorded by hash.
Each repository's regular 9,576-case test passes separately.

```sh
python3 scripts/test_ram_code.py .context/clipped-flip-build --snes
python3 tests/amiga/test_console_jit.py --exhaustive
```

Requires Unicorn, Clang and LLVM nm (`LLVM_NM` for the SNES helper). The generated
ROM is stored only under `.context/ram-code-test`. This checks instruction-model
behaviour, not real cache coherency, mapper completeness, game speed or audible
sound. Physical validation of these new fixtures remains pending: the bridge
reported its Amiga connection disconnected before a hardware test started.

[Results](results.txt) · [Exact input hashes](hashes.json).
