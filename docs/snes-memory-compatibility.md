# SNES cartridge memory: comparison and implementation plan

ZIP loading does not expand supported cartridge hardware. ZZSNES currently
accepts ordinary LoROM/HiROM up to 4 MiB, including PAL/NTSC and a 512-byte
copier header. Its ARM core is PocketSNES/libretro Snes9x2002, not current
Snes9x or Mesen. No larger-ROM or enhancement-chip support is claimed here.

## Three different kinds of expansion

| Change | Current ZZSNES | Upstream reference | What our port needs |
| --- | --- | --- | --- |
| Larger ROM / bank mapping | 4 MiB loader and shared ROM region | Current Snes9x has a 16 MiB allocation ceiling and explicit extended HiROM mappings; Mesen detects extended layouts and maps ROM banks separately | Relocate the save-transfer region, audit the ARM heap/layout, implement/test the matching map, then test real cartridges |
| More cartridge SRAM | Header permits up to 128 KiB; core allocates and serializes 128 KiB | Mesen sizes save RAM from the header, capped at 256 KiB in the reviewed standard path, and separately handles coprocessor RAM | Expand allocation, masks, mappings and persistence together; preserve/reject old states explicitly |
| SA-1, Super FX, DSP and other hardware | Explicitly rejected by the cartridge gate | Separate coprocessor implementations and cartridge-specific maps | Audit each chip's compiled code, callbacks and complete save state; measure ARM workload per game |

The 16 MiB value is Snes9x's code allocation ceiling, not a promise that every
16 MiB ROM layout works. Likewise, having coprocessor source files in our tree
does not establish working support.

## Why simply raising the ROM limit is wrong

The current shared layout places ROM data at offset 0x200000 with a
0x400000-byte capacity. The save-transfer buffer starts at 0x600000 and occupies
0x100000 bytes. Increasing the ROM capacity in place would overlap that buffer.
The worker heap is a separate 18 MiB BSS arena within the owned allocation.
All relocated code/BSS, display and transfer bounds must remain valid together.

The older Snes9x2002 core allocates up to 6 MiB for ROMs and contains a
TalesROMMap path. That is a useful starting point, but not equivalent to the
modern Snes9x/Mesen extended-mapping implementations. Its snapshot code writes
a fixed 0x20000-byte SRAM block and has conditional SA-1 sections. Other chip
state cannot be assumed complete simply because a game boots.

## Comparison of the approaches

Snes9x uses direct mapping tables, mirrors, offset-aware HiROM helpers and
write protection. Its header scoring and extended-ROM ordering logic must
agree with the map selected. This is closer to our existing core structure.

Mesen uses cartridge flags, header scoring, ROM/RAM handlers, bank registration
and separately owned coprocessors. It offers a useful independent reference
for address decoding and RAM ownership. Copying its cartridge class into
PocketSNES would not be a drop-in port.

## Proposed first milestone

1. Identify the supplied cartridge: size, copier header, mapper, ROM type,
   SRAM size and region. Establish whether it needs extra ROM, RAM or a chip.
2. For a standard extended ROM, add a synthetic bank-signature fixture that
   checks low/high banks, mirrors, reset vectors and SRAM exclusion.
3. Move transfer buffers and update host/ARM bounds as one change.
4. Check existing standard cartridges and save-state round trips.
5. Run the selected extended cartridge on the A9 with bounded frames and
   teardown checks before enabling it in the normal launcher.
6. Treat each enhancement chip as a separate compatibility/performance task.

No new test cartridge was attached to this request at the time of the review.
Cartridge-specific implementation and physical validation remain pending.

## Reviewed sources

- [Snes9x memory allocation and maps](https://github.com/snes9xgit/snes9x/blob/1bcc369e89f08243e0a462882fb1f3e42e51de3a/memmap.cpp)
- [Snes9x ROM-size ceiling and memory structure](https://github.com/snes9xgit/snes9x/blob/1bcc369e89f08243e0a462882fb1f3e42e51de3a/memmap.h)
- [Snes9x snapshots](https://github.com/snes9xgit/snes9x/blob/1bcc369e89f08243e0a462882fb1f3e42e51de3a/snapshot.cpp)
- [Mesen cartridge detection, RAM and bank mappings](https://github.com/SourMesen/Mesen2/blob/b9fa69ddc6d0a331fb103fdb5eef6904305703c2/Core/SNES/BaseCartridge.cpp)
- [Mesen memory manager](https://github.com/SourMesen/Mesen2/blob/b9fa69ddc6d0a331fb103fdb5eef6904305703c2/Core/SNES/SnesMemoryManager.cpp)
- [Current port cartridge gate](../development/beta-20261015/build-inputs/player/cartridge.h)
- [Current shared layout](../development/beta-20261015/build-inputs/player/wire.h)
- [Current Snes9x2002 allocation/mapping](../development/beta-20261015/source/core/src/memmap.c)
- [Current Snes9x2002 snapshots](../development/beta-20261015/source/core/src/snapshot.c)

Upstream sources were inspected at the commit hashes in these permalinks.
This is a code comparison, not a benchmark or proof of compatibility.
