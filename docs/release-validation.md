# ZZSNES 0.6.5 Beta packaging validation

The release retains the physically tested **0.6.5-dev** executables byte-for-byte.
Packaging does not change emulation code or establish a new performance result.
Expected executable hashes and build selection: [release-build.json](release-build.json).

| Asset | Bytes | SHA256 |
| --- | ---: | --- |
| `ZZSNES-0.6.5-dev-beta.zip` | 1,101,873 | `9626cea70d8ab40ec59c05f5b4d3d463b084afab8560d61fa7830da4440b1c43` |
| `ZZSNES-0.6.5-dev-beta-amipkg.lha` | 2,995,064 | `a884535f402315ea01d01b298757d3f53a3562fa89e284281d5620706dfefa06` |
| `ZZSNES-release.json` | 1,116 | `ade04f96d80d28684144704b5f6323473782e432d7eef78be41ba749441e1278` |

## Checks performed

- Cross-compiled the new installer helper and patched amipkg client for 68020.
- Ran the patched amipkg portable-core test suite successfully.
- Extracted both LHA layers with 7-Zip; every payload file matched the manual ZIP.
- Verified every inner SHA256 and each executable against its physical-test record.
- Verified the signed catalogue with the Python signer and the same C verifier used by the Amiga helper.
- Confirmed changed catalogues and empty keys fail authentication.
- Checked catalogue readability, prerequisite-before-copy order, destination assigns, scanline post-install action and Workbench icon types.
- Checked all files for excluded ROM/save/config/signing-key payloads.
- Passed current source/artifact verification and packaging/documentation whitespace checks.
- Preserved imported upstream/core bytes and unified-patch context, including inherited whitespace.

The original upstream C verification fixture expects its official catalogue and
was skipped when that catalogue was absent; the new installer test explicitly
checks our actual catalogue using the C verifier, so that path was not left untested.

These are release and host checks. The new complete offline wrapper has **not**
been installed on the physical Amiga in this packaging session. Earlier isolated
amipkg/scanline installations passed; current game execution evidence is in
`amiga/records/2026-10-19/options/`.
The installer dependency checks are compiled for AmigaOS; host validation does not
prove that a different machine has those dependencies or compatible firmware.

Re-run: `python3 scripts/test_installer.py .context/release` (requires 7zz and a host C compiler).
