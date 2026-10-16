# Super FX launcher player

`cartridge.h` extends the standard gate for LoROM Super FX cartridge types
13/14/15/1A and FX3 types 17/18 (hex). Build with `make current-superfx`.
Host and ARM use this same header. The library detects the same types and
launches ZZSnes-FX automatically; warnings are advisory.

The player uses the compatibility overlay's Super FX timing correction and
sprite-cache reconstruction, followed by the experimental FX3 C overlay in
`core/`. See [implementation and validation](../../docs/fx3-development.md).
Standard games retain the normal player. No ROM bytes are patched.
