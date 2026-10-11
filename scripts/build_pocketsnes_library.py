#!/usr/bin/env python3
"""Opt-in SNES MUI, ROM library and thumbnail/state build; never deploys."""
from pathlib import Path
import sys
from pocketsnes_library import ROOT,LIB
code=(ROOT/'scripts/build_pocketsnes_player.py').read_text()
code=code.replace("ROOT/'.context/amiga/pocketsnes-player'","ROOT/'.context/amiga/pocketsnes-library'")
code=code.replace("original=b.prepare", "from pocketsnes_library import frontend,core_patch\nfrontend(OUT)\noriginal=b.prepare")
code=code.replace("return [s if s.name!='bench.c' else b.SRC/'player.c'", "core_patch(OUT)\n return [s if s.name!='bench.c' else OUT/'player.c'")
code=code.replace("exec(compile(code,'build_pocketsnes_arm.py','exec'))", "code=code.replace('0x1800000','0x2000000').replace(\"0x110000u\",\"0x11c000u\").replace(\"inside(SRC/'payload.ld')\",\"inside(OUT/'payload.ld')\")\n exec(compile(code,'build_pocketsnes_arm.py','exec'))")
code=code.replace("b.SRC/'elf.py'","OUT/'elf.py'").replace("b.SRC/'player.c'","OUT/'player.c'").replace('0.1-preview','0.2.0-dev')
code=code.replace('-DZZ_BLOCK_SIZE=0x1800000','-DZZ_BLOCK_SIZE=0x2000000').replace('-DMB_ALLOCATION=0x1800000u','-DMB_ALLOCATION=0x2000000u')
code=code.replace("exec(compile(code,'build_pocketsnes_arm.py','exec'))", "code=code.replace('header_digest=hashlib.sha256', \"header_inputs += [OUT/n for n in ['state_format.h','cartridge.h','view.h','pixels.h','nes_api.h','wire.h','direct_plan.h']]\\nheader_digest=hashlib.sha256\")\n exec(compile(code,'build_pocketsnes_arm.py','exec'))")
# Append MUI compilation using the same known 68k toolchain.
code+='''
subprocess.run([*container_command(),'run','--rm','--platform','linux/amd64','-v',f'{ROOT}:/work','-w','/work',PIN['image'],'m68k-amigaos-gcc','-std=c99','-noixemul','-m68020','-O2','-g','-Wall','-Wextra','-Werror','-Wno-unused-function','-Wno-misleading-indentation','-I'+str(relative),str(relative/'launcher.c'),'-lamiga','-o',str(relative/'ZZSnesLibrary')],cwd=ROOT,check=True)
'''
code+='\nfrom pocketsnes_library import manifest\nmanifest(OUT)\n'
exec(compile(code,str(ROOT/'scripts/build_pocketsnes_player.py'),'exec'))
