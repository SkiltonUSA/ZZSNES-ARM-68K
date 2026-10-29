# ZZSNES offline installer

[Download 0.7.3 Beta](https://github.com/SkiltonUSA/ZZSNES-ARM-68K/releases/tag/v0.7.3-beta). Choose `ZZSNES-0.7.3-dev-beta-amipkg.lha`
for the installer or `ZZSNES-0.7.3-dev-beta.zip` for manual copying. They contain
the same tested executable bytes. No ROMs, firmware or system libraries are included.

## Install

Extract the LHA, open the installer drawer and double-click **Install**. Choose
a parent drawer; the installer creates its `ZZSNES` drawer there. From a Shell
in the extracted installer drawer, an exact destination can be specified:

```text
Execute Install DEST "Work:Emulators/ZZSNES"
```

Requires `C:lha`, and `C:IconX` for double-click launch. No Internet connection,
Mac, or existing amipkg installation is required. Close this emulator launcher
and other Core1 applications before starting.

## Dependency checks

| Component | Installer behaviour |
| --- | --- |
| MUI 3.8 (`muimaster.library` v19+) | Required |
| Graphics v39+, ASL v38+, keymap v37+ | Required |
| Picasso96API v2+ | Required |
| ZZ9000 graphics and Fast RAM boards | Required |
| AHI v4+, registered audio modes and accessible unit 0 | Required; detection plays no sound |
| Datatypes v43+ / lowlevel v40+ | Reported; artwork / physical controller support |
| Launcher and Core1 ownership | Stop if this launcher or a Core1 application is active |
| XACP/FPGA firmware, RAM allocation, display modes | Runtime checks; not inferred from installed tools |

Missing requirements stop installation with an explanation. System libraries
and drivers are not downloaded or replaced; use their official installers.
The host targets the 68020 instruction set; a TF4060 is not required.

## Scanline tools and existing data

The payload includes **Xanxi's ZZScanlines and ZZScanlinesGUI**, source/licences
and **Install-Scanlines**. Installation automatically runs the helper, preserving
controls already in `SYS:Utilities/ZZScanlines` or `C:` and adding missing ones.
Compatible FPGA firmware is required for the fullscreen effect. Windowed Soft
scanlines are software. No startup or firmware changes are made.

Only distributed program files enter package receipts. Library.cfg, Controls.cfg,
profiles, ROMs, saves and snapshots are absent from the payload and preserved.
The destination receives a Workbench drawer icon if missing. Shared scanline
tools remain after package removal. The launcher remains the place to delete games.

## Offline packaging and limitations

The private **amipkg 0.7.11-zzinstall1** client is based on upstream
`66cfb9888f7d6b28d1411ff254817ee14ed4e043`. Its local fixes select the correct
custom-repository recipe, run prerequisites before upgrade removal, handle
assign-root copy paths, and retain resolved receipt paths. It is not an upstream
amipkg release. [Patch](../scripts/amipkg-zzinstall.patch).

The wrapper verifies an Ed25519 catalogue signature before staging it; amipkg
checks payload SHA256 before extraction. The bundled key verifies that catalogue,
not independent publisher identity; download the installer from the linked release.
No online update feed exists. Re-run **Install** for an upgrade; the `.invalid`
repository URL is an offline placeholder. The system amipkg installation is untouched.

`PackageManager` contains the private client, receipts and cache. Temporary
`ZZPKG:` and `ZZEMUDEST:` assigns last until reboot. The package engine does not
provide complete rollback: retain backups when upgrading. The wrapper reruns
the scanline installer because amipkg post-script failures are otherwise warnings.

Earlier isolated A4000TX package tests established missing-tool installation and
existing-tool preservation. Current release archive, signature, executable hashes
and validation results are recorded in `docs/release-validation.md`.

## Rebuild

```sh
python3 scripts/build_installer.py
python3 scripts/package_current.py --build .context/ax-release-build \
  --fx-build .context/ax-release-build --version 0.7.3-dev --output .context/release
python3 .context/amipkg-installer/client-src/tools/amipkg-repo-sign keygen /private/path/zzemu
python3 scripts/package_installer.py .context/release/ZZSNES-0.7.3-dev-beta.zip \
  --key /private/path/zzemu.secret --output .context/release
```

The private signing key must stay outside Git and release archives. Player build
commands and expected executable hashes are in [release-build.json](release-build.json).
The older `package_amipkg.py` recipe-only workflow remains available for repository
maintainers; publishing a feed requires amipkg's signed repository workflow.
