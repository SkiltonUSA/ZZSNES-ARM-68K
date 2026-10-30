# Controller development log

1. Reviewed the existing lowlevel input path and MUI tester. Preserved their
   keyboard/player-2 settings and added a separate player-1 source preference.
2. Checked the SN30 Pro manual, SDL D-input mapping, Poseidon SDK/source and
   Mega Drive TH protocol. The Bluetooth-model SN30 is being used over USB;
   no controller was attached for this work.
3. Added failing native decoder tests, then descriptor-driven HID decoding and
   Sega six-button decoding. Added a shared 68k transport for tester and player.
4. Reviewed device lifetime: the USB release hook only invalidates the binding;
   the submitting task drains and frees its own requests. Added failure,
   disconnect/reconnect, configuration and resource-cleanup model tests.
5. Checked the gameport masks against the hardware manual. C/Start uses its
   documented button-sense pull-up; only TH toggles. Tests check phase order,
   partial allocation, port masks and interrupt/resource restoration.
6. A full ARM rebuild encountered a local toolchain/sysroot mismatch. Kept the
   recorded 0.7.8 ARM image instead. The host-only build accepts that exact
   release digest, extracts the image/probe/relocations and verifies their
   byte-for-byte presence in the new players. A modified reference is rejected.
7. Cross-built matching 0.7.9-dev launcher and both players. Decoder/transport
   sanitizer tests and 20,000-entry/legacy library regressions pass.
8. Ran physical USB-absence and 640×480 MUI checks. All seven pages, four input
   choices, scrolling and resizing pass. No ARM session or gameport polling;
   the existing ZZUAE session was left alone.
9. Added an error-path regression for returning USB to hid.class after an IO
   failure; observed the failure, corrected cleanup, then passed the tests.
10. Installed matching development binaries with verified checksums and
    rollback copies. Settings remained unchanged. The first bridge download
    timed out before installation; verified local originals and same-disk
    rollback copies were used instead.

Connected SN30 reports, audible/gameplay response, physical Sega timing and
hotplug remain **pending**. No controller compatibility or performance result
is inferred from the native model or USB-absence test. No new public release
was published; 0.7.8 remains the downloadable beta.
