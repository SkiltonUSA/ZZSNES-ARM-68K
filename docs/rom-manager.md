# ROM Manager

Open **ROM Manager** in the left sidebar.

- **Import ROM folder:** choose an SD card, mounted USB drive or directory, then
  import. Includes subdirectories and supported single-ROM ZIPs. Existing paths
  are skipped. It adds library entries; ROM files stay on the selected disk.
  Scanning runs once, with a Cancel button, and reports added/skipped entries.
- **Delete duplicates:** compares decoded ROM contents, including ZIPs and path
  aliases. Matching titles alone are not enough. Review the count and confirm.
  The first entry is retained; favourites and play history are merged. ROMs and
  saves stay on disk. Unreadable files are retained. For NES, separate path-based
  save slots remain at their original paths; this action does not merge states.
- **Back up save states:** choose a writable mounted disk or directory. Each run
  creates a new folder containing States, Snapshots, Library.cfg and the battery
  saves belonging to listed ROMs. Files are read back and compared before being
  committed to the backup. `Backup.txt` ending in COMPLETE identifies a finished
  backup; `Backup.part` indicates an interrupted/incomplete run. Existing backups
  are never overwritten. ROMs and emulator executables are not copied.

Battery files are numbered in the backup to prevent same-name collisions;
Backup.txt records the original and destination paths in hex. Library.cfg retains
ROM paths and settings. Restoring to a different ROM location may require updating
those paths, particularly for NES states keyed by ROM path. This is a backup
operation, not an automatic restore or save-format converter.

The footer shows **ZZ9000: n.n C** beside the development version. This is the
Zynq chip temperature, not the 68k CPU or case temperature. Read-only polling
runs approximately every two seconds while the launcher is visible and pauses
while a game runs. Unavailable or implausible sensor values show **n/a**.

## Validation

Native sanitizer tests cover 20,000-entry cleanup, favourite/history/selection
retention, verified copying, existing-file preservation, cancellation, empty
files and changes to a source during backup. Existing large-library and recursive
scan tests also pass. Hardware results are recorded in the development handoff.
No ARM player or emulation-core changes are required for these features.

Temperature register/units follow MNT Research's [ZZTop reference](https://mntre.com/media/ZZ9000_info_md/zz9000-firmware-archive/zztop-1.1b.lha), through the existing ZZTemperature implementation. It does not control cooling or change firmware.

Duplicate scanning shows a progress window: “Scanning for duplicates, zipped
files take longer”, the current filename, and its position in the library.
Cancel stops the scan without removing entries. The window closes before the
removal confirmation. It checks listed ROMs wherever they are stored; it does
not search the whole drive. ZIP files are decompressed to compare ROM contents.

Cancel is processed between ROM reads; an in-progress ZIP decode finishes first.
