#!/usr/bin/env python3
"""Pinned PocketSNES (Snes9x2002) original-fixture benchmark, no deployment."""
import json,hashlib,shutil,subprocess,re,sys,difflib
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];OUT=ROOT/'.context/amiga/pocketsnes';SRC=ROOT/'amiga/pocketsnes'
PIN='6ffbf9ef4f0063e1f1b78a40d10c50fc52f2524c'
def decode(raw):
    try:return raw.decode('utf-8-sig')
    except UnicodeDecodeError:return raw.decode('latin1')
def prepare():
    checkout=ROOT/'.context/research/snes9x2002'
    if not checkout.exists():
        subprocess.run(['git','clone','https://github.com/libretro/snes9x2002',checkout],check=True)
        subprocess.run(['git','-C',checkout,'checkout',PIN],check=True)
    assert subprocess.check_output(['git','-C',checkout,'rev-parse','HEAD'],text=True).strip()==PIN
    OUT.mkdir(parents=True,exist_ok=True)
    patch='';hashes={}
    for folder in ['src','libretro']:
        for p in (checkout/folder).rglob('*'):
            if not p.is_file():continue
            rel=p.relative_to(checkout);raw=p.read_bytes();hashes[str(rel)]=hashlib.sha256(raw).hexdigest()
            dest=OUT/rel;dest.parent.mkdir(parents=True,exist_ok=True)
            if p.suffix in ['.c','.h']:
                text=decode(raw)
                if str(rel)=='src/port.h':text=text.replace('#define FAST_LSB_WORD_ACCESS','// Disabled: A9 strict alignment. #define FAST_LSB_WORD_ACCESS')
                # No game patches or idle-loop skipping in the comparison.
                if str(rel)=='src/memmap.c':
                    text=text.replace('(1 << (Memory.ROM [o + 0xd7] - 7)) > 48', 'Memory.ROM [o + 0xd7] >= 13')
                    text=text.replace('1 << (Memory.ROMSize - 7)', '(Memory.ROMSize >= 7 && Memory.ROMSize < 38 ? 1u << (Memory.ROMSize - 7) : 0u)')
                if str(rel)=='libretro/libretro.c':
                    text=text.replace('Settings.SpeedHacks = TRUE','Settings.SpeedHacks = FALSE').replace('Settings.Shutdown = Settings.ShutdownMaster = TRUE','Settings.Shutdown = Settings.ShutdownMaster = FALSE')
                dest.write_text(text)
                patch+=''.join(difflib.unified_diff(decode(raw).splitlines(True),text.splitlines(True),'upstream/'+str(rel),'generated/'+str(rel)))
            else:dest.write_bytes(raw)
    (OUT/'integration.patch').write_text(patch)
    (OUT/'upstream.json').write_text(json.dumps({'url':'https://github.com/libretro/snes9x2002','commit':PIN,'files':hashes},indent=2)+'\n')
    import build_mesen_snes_system as fixture
    fixture.OUT=OUT;rom=fixture.fixture()
    (OUT/'fixture.h').write_text('static const unsigned char pocket_rom[32768]={'+','.join(map(str,rom))+'};\n')
    names='apu apuaux c4 c4emu cheats cheats2 clip data dsp1 fxemu fxinst globals dma memmap cpu cpuexec cpuops sa1 sa1cpu sdd1 sdd1emu snapshot soundux spc700 srtc ppu_ gfx tile'.split()
    return [OUT/'src'/f'{n}.c' for n in names]+[OUT/'libretro/libretro.c',OUT/'libretro/libretro-common/streams/memory_stream.c',SRC/'bench.c']
def flags():return ['-O2','-g','-fno-strict-aliasing','-DHAVE_STRINGS_H','-DHAVE_STDINT_H','-DHAVE_INTTYPES_H','-D__LIBRETRO__','-DUSE_SA1','-D__OLD_RASTER_FX__',*['-I'+str(p) for p in [OUT,OUT/'src',OUT/'libretro',OUT/'libretro/libretro-common/include',ROOT/'amiga/mesen_bench']]]
if __name__=='__main__':
    sources=prepare()
    header_digest=hashlib.sha256(b''.join(p.read_bytes() for p in sorted(OUT.rglob('*.h')) if p.name!='zz_payload.h')).digest()
    objects=[]
    common=['clang','-std=gnu99',*flags(),'-Wno-incompatible-pointer-types','-Wno-pointer-sign','-DPOCKET_NATIVE','-fsanitize=address,undefined','-fno-sanitize-recover=all']
    for i,source in enumerate(sources):
        obj=OUT/(str(i)+'-native.o');objects.append(obj)
        digest=hashlib.sha256(source.read_bytes()+header_digest+str(common).encode()).hexdigest();stamp=obj.with_suffix('.sha256')
        if not obj.exists() or not stamp.exists() or stamp.read_text()!=digest:
            subprocess.run([*common,'-c',source,'-o',obj],check=True);stamp.write_text(digest)
    subprocess.run(['clang','-fsanitize=address,undefined',*objects,'-lm','-o',OUT/'native'],check=True)
    r=subprocess.run([OUT/'native'],text=True,capture_output=True,cwd=OUT);(OUT/'native.log').write_text(r.stdout+r.stderr);print(r.stdout+r.stderr);r.check_returncode()
