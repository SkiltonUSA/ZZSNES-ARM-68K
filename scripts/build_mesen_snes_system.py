#!/usr/bin/env python3
"""Build the opt-in Mesen2 complete-system fixture; does not deploy."""
import argparse,difflib,hashlib,json,re,shutil,subprocess,os
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
SRC=ROOT/'amiga/mesen_snes/system'
OUT=ROOT/'.context/amiga/mesen-snes-system'

def body_replace(text,pattern,body=None):
    matches=list(re.finditer(pattern,text))
    for m in reversed(matches):
        start=text.index('{',m.start());depth=1;end=start+1
        while depth:
            depth+=(text[end]=='{')-(text[end]=='}');end+=1
        text=text[:m.start()]+('' if body is None else text[m.start():start+1]+body+'}')+text[end:]
    return text

def prepare():
    OUT.mkdir(parents=True,exist_ok=True)
    manifest=json.loads((SRC/'vendor/upstream.json').read_text());patch=''
    extra={'SNES/'+n for n in ['SnesConsole.h','BaseCartridge.h','SnesControlManager.h']}
    allowed=(set(manifest['files'])|extra|{'pch.h'})-{'SNES/SpcFileData.h'}
    for name,digest in manifest['files'].items():
        p=SRC/'vendor'/name;raw=p.read_bytes();assert hashlib.sha256(raw).hexdigest()==digest,name
        text=raw.decode('utf-8-sig')
        text=re.sub(r'^#include "([^"]+)"\s*$',lambda m:m[0] if m[1] in allowed else '',text,flags=re.M)
        text=text.replace(' : public ISerializable','').replace(', public ISerializable','')
        text=text.replace('throw std::runtime_error("invalid start/end address");', 'assert(false); return;')
        text=text.replace('throw std::runtime_error("unsupported bpp");', 'assert(false); return 0;')
        text=re.sub(r'throw std::runtime_error\("[^"]*"\)', 'ss_fail()', text)
        text=body_replace(text,r'void [A-Za-z_0-9]+::Serialize\(Serializer\s*&\s*s\)\s*\{')
        text=body_replace(text,r'\tvoid Serialize\(Serializer\s*&\s*s\)\s*\{')
        text=re.sub(r'\s*void Serialize\(Serializer\s*&\s*s\) override;', '',text)
        if name=='SNES/SnesPpu.cpp':
            for fun in ['DebugProcessMode7Overlay','DebugProcessMainSubScreenViews']:
                text=body_replace(text,r'void SnesPpu::'+fun+r'\(\)\s*\{','')
            text=text.replace(', _console->GetControlManager()->GetPortStates()', '')
        if name=='SNES/Spc.cpp':
            text=body_replace(text,r'void Spc::LoadSpcFile\(SpcFileData\* data\)\s*\{')
            text=text.replace('_memoryManager->GetMasterClock() * _clockRatio - _state.Cycle <= 1', 'SS_FIRST_HALF(_memoryManager->GetMasterClock(), _clockRatio, _spcSampleRate, _state.Cycle)')
            text=text.replace('_clockRatio = (double)(_spcSampleRate * 64) / _console->GetMasterClockRate();', '#ifndef SS_INTEGER_CLOCK\n\t_clockRatio = (double)(_spcSampleRate * 64) / _console->GetMasterClockRate();\n#endif')
        if name in ('SNES/Spc.cpp','SNES/Spc.Instructions.cpp'):
            text=text.replace('_memoryManager->GetMasterClock() * _clockRatio', 'SS_CLOCK(_memoryManager->GetMasterClock(), _clockRatio, _spcSampleRate)')
        if name=='SNES/DSP/DspVoice.cpp':
            for kind in ['Cubic','Sinc']:
                text=text.replace(f'output = DspInterpolation::{kind}(_interpolationPos, _sampleBuffer, _bufferPos);', 'assert(false); output = 0;')
        # Coarse instrumentation only in opt-in SS_PROFILE builds. Account
        # nested SPC work separately when the PPU completes a frame.
        scopes={
            'SNES/SnesPpu.cpp': ['bool SnesPpu::ProcessEndOfScanline(uint16_t& hClock)',
                'void SnesPpu::RenderScanline()', 'uint8_t SnesPpu::Read(uint16_t addr)',
                'void SnesPpu::Write(uint32_t addr, uint8_t value)'],
            'SNES/Spc.cpp': ['void Spc::ProcessEndFrame()'],
            'SNES/Spc.Instructions.cpp': ['void Spc::Run()'],
        }
        for signature in scopes.get(name,[]):
            needle=signature+'\n{';assert text.count(needle)==1,signature
            text=text.replace(needle,needle+'\n\tSS_PROFILE_SCOPE('+('0' if 'SnesPpu' in name else '1')+');')
        dest=OUT/name;dest.parent.mkdir(parents=True,exist_ok=True);dest.write_text(text)
        patch+=''.join(difflib.unified_diff(raw.decode('utf-8-sig').splitlines(True),text.splitlines(True),'upstream/'+name,'generated/'+name))
    for n in extra:shutil.copyfile(SRC/Path(n).name,OUT/n)
    shutil.copyfile(SRC/'pch.h',OUT/'pch.h')
    shutil.copyfile(SRC/'tiny.h',OUT/'tiny.h')
    shutil.copyfile(SRC/'profile.h',OUT/'profile.h')
    (OUT/'integration.patch').write_text(patch)
    return sorted(p for p in (OUT/'SNES').rglob('*.cpp'))

def fixture():
    rom=bytearray(32768);code=bytearray([0x78,0x18,0xfb,0xc2,0x10])
    def write(a,v):code.extend([0xa9,v,0x8d,a&255,a>>8])
    write(0x2100,0x80);write(0x4200,0);write(0x420c,0)
    write(0x2105,0);write(0x2107,4);write(0x210b,0);write(0x212c,1)
    write(0x2115,0x80);write(0x2116,0);write(0x2117,0)
    def dma(src,n,mode,dest):
        for a,v in [(0x4300,mode),(0x4301,dest),(0x4302,src&255),(0x4303,src>>8),(0x4304,0),(0x4305,n&255),(0x4306,n>>8),(0x420b,1)]:write(a,v)
    dma(0x8200,16,1,0x18)
    write(0x2121,0);dma(0x8210,8,0,0x22)
    write(0x2100,15)
    # Keep executing while the real scanline scheduler runs.
    code.extend([0x80,0xfe])
    assert len(code)<512
    rom[:len(code)]=code
    rom[0x200:0x210]=bytes([0xaa,0x55,0x55,0xaa]*4)
    rom[0x210:0x218]=bytes([0,0x7c,0x1f,0,0xe0,3,0xff,0x7f])
    rom[0x7fc0:0x7fd5]=b'ZZSNES ORIGINAL TEST'.ljust(21,b' ')
    rom[0x7fd5:0x7fdc]=bytes([0x20,0,5,0,1,0,0])
    rom[0x7ffc:0x7ffe]=bytes([0,0x80]);rom[0x7fea:0x7fec]=bytes([0,0x80])
    # SNES checksum/complement bytes contribute a fixed $1fe.
    checksum=(sum(rom)+0x1fe)&65535
    rom[0x7fdc:0x7fe0]=(checksum^65535).to_bytes(2,'little')+checksum.to_bytes(2,'little')
    assert len(rom)==32768 and sum(rom)&65535==checksum
    (OUT/'System-Test.sfc').write_bytes(rom)
    return rom

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--frames',type=int,default=6);p.add_argument('--integer-clock',action='store_true');a=p.parse_args()
    sources=prepare();fixture()
    exe=OUT/('snes-system-native-integer' if a.integer_clock else 'snes-system-native')
    subprocess.run(['clang++','-std=c++17','-O2','-g','-fsanitize=address,undefined','-fno-sanitize-recover=all',
        *(['-DSS_INTEGER_CLOCK'] if a.integer_clock else []),'-I'+str(OUT),*sources,SRC/'console.cpp',SRC/'native.cpp','-o',exe],check=True)
    result=subprocess.run([exe,OUT/'System-Test.sfc',str(a.frames),OUT/'frame.ppm'],text=True,capture_output=True)
    (OUT/('native-integer.log' if a.integer_clock else 'native.log')).write_text(result.stdout+result.stderr);print(result.stdout+result.stderr,end='');result.check_returncode()
if __name__=='__main__':main()
