#!/usr/bin/env python3
"""Build the Mesen SNES system diagnostic for A9/68k; no deployment."""
import sys,subprocess,importlib.util,argparse,json,hashlib
from pathlib import Path
root=Path(__file__).resolve().parents[1];sys.path.insert(0,str(root/'scripts'))
import build_mesen_snes_system as b
from build_zz9000_debug import unpack_elf
from amiga import PIN, container_command
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--container-command');p.add_argument('--profile',action='store_true');p.add_argument('--word-memory',action='store_true');p.add_argument('--ppu-opt',choices=['Os','O2'],default='Os');a=p.parse_args()
if a.profile:b.OUT=root/'.context/amiga/mesen-snes-profile'
if a.ppu_opt!='Os':b.OUT=b.OUT.with_name(b.OUT.name+'-'+a.ppu_opt.lower())
if a.word_memory:b.OUT=b.OUT.with_name(b.OUT.name+'-words')
sources=b.prepare();rom=b.fixture();out=b.OUT
chunks=[];calls=[]
for off in range(0,len(rom),64):
 chunk=rom[off:off+64]
 if any(chunk):
  chunks.append(f'static const uint8_t data_{off}[]={{'+','.join(map(str,chunk))+'};')
  calls.append(f'memcpy(system_rom+{off},data_{off},{len(chunk)});')
(out/'fixture.h').write_text('static uint8_t system_rom[32768];\n'+'\n'.join(chunks)+'\nstatic void init_rom(){memset(system_rom,0,sizeof(system_rom));'+''.join(calls)+'}\n')
common=root/'amiga/mesen_bench'
flags=['--target=arm-none-eabi','-mcpu=cortex-a9','-marm','-mfloat-abi=soft','-mno-unaligned-access','-ffreestanding','-fno-builtin','-fPIC','-fvisibility=hidden','-fno-stack-protector','-O2','-g','-DSS_ARM','-DSS_INTEGER_CLOCK','-DZZ_BLOCK_SIZE=0x400000','-DZZ_CONTROL=0x80000','-DMB_MMU','-DMB_DCACHE','-DMB_ALLOCATION=0x400000u',*['-I'+str(p) for p in [out,b.SRC,common,root/'amiga/arm_debug',root/'amiga/arm_debug/zz9000']]]
sources += [b.SRC/'profile.cpp',b.SRC/'console.cpp',b.SRC/'runtime.cpp',b.SRC/'panic.cpp',b.SRC/'bench.cpp',common/'worker.cpp',common/'mmu.c',common/'mmu_asm.S',common/'runtime.c',root/'amiga/fceux_bench/runtime.c']
if a.word_memory:sources=[b.SRC/'memory.c' if s==common/'runtime.c' else s for s in sources]
if a.profile:
 flags.append('-DSS_PROFILE')
 worker=(common/'worker.cpp').read_text().replace('#include "wire.h"','#include "wire.h"\n#include "profile.h"')
 worker=worker.replace('        ad_put(reply,68,token^ZZ_XOR);','''        ad_put(reply,96,ss_profile.ticks[0]);ad_put(reply,100,ss_profile.ticks[1]);
        ad_put(reply,104,ss_profile.calls[0]);ad_put(reply,108,ss_profile.calls[1]);
        ad_put(reply,112,0x53535031u);
        ad_put(reply,68,token^ZZ_XOR);''')
 (out/'worker-profile.cpp').write_text(worker)
 sources=[out/'worker-profile.cpp' if s==common/'worker.cpp' else s for s in sources]
objects=[]
for i,src in enumerate(sources):
 obj=out/(str(i)+'-'+src.stem+'.arm.o');objects.append(obj)
 extra=['-std=c++17','-fno-exceptions','-fno-rtti','-nostdinc++'] if src.suffix=='.cpp' else []
 subprocess.run(['clang',*flags,*extra,*(['-'+a.ppu_opt] if src.name=='SnesPpu.cpp' else []),'-c',src,'-o',obj],check=True)
ld='/opt/homebrew/opt/lld/bin/ld.lld';elf=out/'system.elf'
subprocess.run([ld,'-shared','-Bsymbolic','--no-undefined','-T',root/'amiga/fceux_bench/payload.ld',*objects,'-o',elf],check=True)
spec=importlib.util.spec_from_file_location('pack',root/'amiga/fceux_bench/elf.py');m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
image,rel,entry=m.unpack(elf.read_bytes());print('ARM image',len(image),'relocations',len(rel))

subprocess.run(['clang',*flags,'-c',root/'amiga/arm_debug/zz9000/mapping_probe.S','-o',out/'mapping.o'],check=True)
subprocess.run([ld,'-shared','-Bsymbolic','--no-undefined','-T',root/'amiga/arm_debug/zz9000/payload.ld',out/'mapping.o','-o',out/'mapping.elf'],check=True)
mapping,mrel,point=unpack_elf((out/'mapping.elf').read_bytes());assert not mrel and not point and len(mapping)<=1024
def array(n,data):return 'static const unsigned char '+n+'[]={'+','.join(map(str,data))+'};\n'
(out/'zz_payload.h').write_text(f'#define ZZ_ENTRY {entry}u\n'+array('zz_image',image)+array('zz_mapping_probe',mapping)+'static const unsigned long zz_relocations[]={'+','.join(map(str,rel))+'};\n')
(out/'expected.h').write_text('#define MESEN_EXPECT_CYCLES 277071u\n#define MESEN_EXPECT_HASH 0x1e1e29c5u\n')
host=(common/'host.c').read_text().replace('8991','6').replace('instructions=6','frames=6').replace('cpu_us=','core_us=')
host=host.replace('MESEN CPU-ONLY','MESEN2 SNES SYSTEM; CPU/PPU/DMA/SPC/DSP; headless fixture')
host=host.replace('MESEN_CPU PASS; CPU-only throughput, NOT emulated video FPS','MESEN_SNES_SYSTEM PASS; headless startup fixture, NOT game/display FPS')
needle='if(value(0)!=6 || value(1)'
assert needle in host
pos=host.index('    for(i=0;i<11;i++)')
host=host[:pos]+'    printf("SYSTEM master=%u spc=%u pcm=%08x samples=%u\\n",value(7),value(8),value(9),value(10));\n    if(value(7)!=2093734 || value(8)!=199650 || value(9)!=0x2cfa6c7d || value(10)!=3119)goto done;\n'+host[pos:]
if a.profile:
 host=host.replace('syncio->pull(reply,96,NULL);','syncio->pull(reply,128,NULL);')
 needle='    }\n#ifdef MB_MMU\n    if(!request(MB_CANCEL))'
 assert needle in host
 addition='''        { ULONG ppu=ad_get(shared+MB_REPLY,96),spc=ad_get(shared+MB_REPLY,100);
          if(ad_get(shared+MB_REPLY,112)!=0x53535031u || (uint64_t)ppu+spc>value(6))goto done;
          printf("PROFILE n=%u ppu_ticks=%u spc_ticks=%u residual_ticks=%u ppu_calls=%u spc_calls=%u\\n",i,ppu,spc,value(6)-ppu-spc,ad_get(shared+MB_REPLY,104),ad_get(shared+MB_REPLY,108));
        }
'''
 host=host.replace(needle,addition+needle)
(out/'host.c').write_text(host)
appname='ZZSnes-Mesen-Profile' if a.profile else 'ZZSnes-Mesen-System'
if a.ppu_opt!='Os':appname+='-'+a.ppu_opt
if a.word_memory:appname+='-Words'
argv=json.loads(a.container_command) if a.container_command else container_command()
assert isinstance(argv,list) and argv and all(isinstance(x,str) and x for x in argv)
relative=out.relative_to(root)
subprocess.run([*argv,'run','--rm','--platform','linux/amd64','-v',f'{root}:/work','-w','/work',PIN['image'],'m68k-amigaos-gcc',
 '-std=c99','-noixemul','-m68020','-O2','-g','-Wall','-Wextra','-Werror',
 '-DZZ_HEADLESS_BENCH','-DZZ_RELEASE','-DZZ_BLOCK_SIZE=0x400000','-DZZ_CONTROL=0x80000','-DMB_ALLOCATION=0x400000u',
 '-DZZ_APP_NAME="'+appname+'"','-DZZ_APP_VERSION="0.2.0-dev"','-DZZ_MIN_STACK=65536',
 '-DMB_MMU','-DMB_DCACHE','-DZZ_HEADLESS_MMU','-DZZ_HEADLESS_ICACHE',
 '-Iamiga/arm_debug','-Iamiga/arm_debug/zz9000','-Iamiga/mesen_bench','-I'+str(relative),
 'amiga/arm_debug/zz9000/launcher.c',str(relative/'host.c'),'-lamiga','-o',str(relative/appname)],cwd=root,check=True)
ui=out/'ZZSnes-Mesen'
subprocess.run([*argv,'run','--rm','--platform','linux/amd64','-v',f'{root}:/work','-w','/work',PIN['image'],'m68k-amigaos-gcc',
 '-std=c99','-noixemul','-m68020','-O2','-g','-Wall','-Wextra','-Werror','-Wno-misleading-indentation',
 'amiga/mesen_snes/launcher.c','-lamiga','-o',str(relative/'ZZSnes-Mesen')],cwd=root,check=True)
assert ui.read_bytes()[:4]==b'\0\0\x03\xf3'
binary=out/appname;assert binary.read_bytes()[:4]==b'\0\0\x03\xf3'
record={'word_memory':a.word_memory,'profile':a.profile,'scope':'Six headless SNES fixture frames, actual CPU/PPU/DMA/SPC/DSP. Silent PCM, no game or display FPS.',
 'upstream':json.loads((b.SRC/'vendor/upstream.json').read_text()),'image_bytes':len(image),'relocations':len(rel),
 'allocation':4194304,'bss_offset':0x220000,'heap_bytes':0x160000,'cache_policy':'Unchanged owned-memory MMU / private inner WBWA / outer and mailbox NC',
 'flags':[x for x in flags if not x.startswith('-I')],'ppu_flags':['-'+a.ppu_opt],
 'compiler':subprocess.check_output(['clang','--version'],text=True),
 'source_hashes':{str(p.relative_to(root)):hashlib.sha256(p.read_bytes()).hexdigest() for p in [root/'amiga/mesen_snes/launcher.c',*b.SRC.glob('*.*'),root/'scripts/build_mesen_snes_system.py',Path(__file__),*common.glob('*.[ch]'),common/'mmu_asm.S',root/'amiga/fceux_bench/runtime.c',root/'amiga/fceux_bench/payload.ld',root/'amiga/fceux_bench/elf.py',root/'amiga/arm_debug/zz9000/launcher.c',root/'amiga/arm_debug/zz9000/layout.h',root/'amiga/arm_debug/zz9000/mapping_probe.S',root/'amiga/arm_debug/protocol.h'] if p.is_file()},
 'outputs':{p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in [elf,binary,ui,out/'integration.patch',out/'System-Test.sfc']},
 'executable_md5':hashlib.md5(binary.read_bytes()).hexdigest()}
(out/'build.json').write_text(json.dumps(record,indent=2)+'\n')
print('Built',binary.name,binary.stat().st_size,'bytes',record['executable_md5'])
