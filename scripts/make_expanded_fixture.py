#!/usr/bin/env python3
"""Original Mode 5 fixture for native, model and optional hardware testing."""
from pathlib import Path
import build_mesen_snes_system as base
ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'.context/expanded-fixtures';OUT.mkdir(parents=True,exist_ok=True)
base.OUT=OUT
rom=bytearray(base.fixture());needle=bytes([0xa9,0,0x8d,5,0x21]);at=rom.index(needle);rom[at+1]=5
rom[0x7fc0:0x7fd5]=b'ZZSNES HIRES TEST'.ljust(21,b' ')
rom[0x7fdc:0x7fe0]=bytes(4);checksum=(sum(rom)+510)&65535
rom[0x7fdc:0x7fe0]=(checksum^65535).to_bytes(2,'little')+checksum.to_bytes(2,'little')
(OUT/'HighRes-Test.sfc').write_bytes(rom)
print(OUT/'HighRes-Test.sfc')
# Family/large-ROM boot fixtures. Chip operations are exercised separately by
# tests/features.c; these files check host transfer, cartridge gating and boot.
standard=(OUT/'System-Test.sfc').read_bytes()
for name,n,h,map_mode,chip in [('C4-Test.sfc',32768,0x7fc0,0x20,0xf3),('SDD1-6M-Test.sfc',0x600000,0x7fc0,0x32,0x43),('ExHiROM-6M-Test.sfc',0x600000,0x40ffc0,0x25,0)]:
 data=bytearray(n);offset=0x408000 if h==0x40ffc0 else 0;data[offset:offset+len(standard)]=standard
 data[h:h+21]=name.encode().ljust(21,b' ');data[h+21]=map_mode;data[h+22]=chip;data[h+23]=13 if n>0x400000 else 5
 data[h+28:h+32]=b'\xff\xff\x00\x00'
 (OUT/name).write_bytes(data)
 print(OUT/name)
