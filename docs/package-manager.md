# Experimental amipkg packaging

`scripts/package_amipkg.py` converts the complete development ZIP into a stored
LHA archive and an amipkg recipe. It bundles **Xanxi's ZZScanlines and
ZZScanlinesGUI**, their source/license and our Install-Scanlines helper. The
recipe runs that helper automatically after copying the emulator. Existing
tools in SYS:Utilities/ZZScanlines or C: are preserved; missing tools are added.
It also creates a Workbench drawer icon.

Compatible FPGA firmware is still required. Installing the tools does not
install firmware or alter startup files. The emulator uses the register
interface directly; the separate tool need not be running.

## Build a package

```sh
python3 scripts/package_amipkg.py path/to/development.zip \
  --app APP --version VERSION \
  --url https://your-server.example/APP-VERSION.lha \
  --output .context/amipkg
```

Replace APP with ZZNES or ZZSNES, and VERSION with the packaged application
version. Default destination is SYS:Programs/APP. `--destination` chooses another
absolute Amiga drawer and is encoded in the recipe, not inferred at install time.
For isolated tests only, `--scanline-destination` redirects the shared tools.
Use a fresh managed drawer; do not adopt an existing ROM/save collection.

The input ZIP must already include the complete scanline bundle. The packager
rejects ROMs, save folders containing files, and Library.cfg. It preserves all
included bytes; no private game data is distributed. Install MUI, Picasso96,
AHI and the compatible XACP environment separately before running the emulator.
This recipe does not auto-install or update those system components.

Publish the LHA and combine the generated entry JSON into a catalogue using
[amipkg's signed repository workflow](https://github.com/thomas-luebker/amipkg/blob/main/HOSTING.md).
Keep the private signing key off the server. No public feed has been created by
this test.

## Tested status and limitations

Stock amipkg 0.7.11 incorrectly loads custom-repository recipes from the official
catalogue. [Local patch](package-manager/amipkg-recipe-origin.patch), based on
66cfb9888f7d6b28d1411ff254817ee14ed4e043, fixes that lookup and identifies the test
CLI as 0.7.11-zztest1. Host tests pass. On a physical A4000TX, installing ZZNES
added both missing tools; installing ZZSNES preserved both. The real SYS: tools
were also preserved. These were isolated installs, not production upgrades.
LHA extraction on the Mac matches every source ZIP file byte-for-byte; official
recipe validation reports no errors and the expected script-review warning.

amipkg 0.7.11 upgrades are not fully transactional; post-install script failure
only warns. This is development packaging, not an unattended updater. The shared
tools installed by the helper remain after an emulator is removed. Their presence
does not establish firmware compatibility. Existing tagged releases are unchanged.
