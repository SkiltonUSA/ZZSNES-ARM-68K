# USB/CD32 controller selection — 0.7.10-dev

- Generated-player ASan/UBSan tests pass for USB/CD32 and native CD32: seven button flags, directions, custom mappings, two-button pads, keyboard fallback and missing lowlevel.library. USB bindings and gameport resources are not claimed by these sources.
- Existing SN30 decoder/transport and Sega transport tests pass. Dynamic 20,000-entry library and legacy migration tests pass.
- Matching launcher and players cross-built with warnings treated as errors. The 0.7.8 ARM image, mapping probe and relocations remain byte-identical in both players.
- Installed executable hashes match the build receipt. Library/settings checksums unchanged; prior installed executables retained as `.pre-usb-cd32`.
- Physical 640x480 MUI probe passes all seven pages, all five controller choices and resizing without input polling. ZZUAE was left running; no Core1 game tests performed.
- ZZNES USB CD32 and Sega operation is user-reported. ZZSNES connected-controller acceptance is pending. No FPS/audio/core claim from these checks.
