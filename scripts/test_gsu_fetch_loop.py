#!/usr/bin/env python3
"""Exercise actual old/new GSU dispatch at STOP, bank and PC boundaries."""
from pathlib import Path
import subprocess,tempfile
from prepare_gsu_fetch import prepare_loop
ROOT=Path(__file__).resolve().parents[1]

def body(text):
 a=text.index('static uint32 fx_run(')
 return text[a:text.index('\n#ifdef FX_FUNCTION_TABLE',a)]

def main():
 old=(ROOT/'development/expanded-20261019/core/src/fxinst.c').read_text()
 with tempfile.TemporaryDirectory() as name:
  p=Path(name);(p/'src').mkdir();(p/'src/fxinst.c').write_text(old);prepare_loop(p)
  new=(p/'src/fxinst.c').read_text()
  s=r'''
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
typedef uint32_t uint32;
static struct {uint32 vCounter,vInstCount,vStatusReg,vPipe,vPrgBankReg,vPipeAdr,pc;uint8_t *pvPrgBank;} GSU;
static uint8_t banks[2][65536];
static unsigned Fx3Active,steps,stop_after;
#define R15 GSU.pc
#define PIPE GSU.vPipe
#define USEX16(v) ((uint16_t)(v))
#define TF(flag) (GSU.vStatusReg&32)
#define READR14 ((void)0)
#define PRGBANK(i) GSU.pvPrgBank[USEX16(i)&(Fx3Active&&GSU.vPrgBankReg<0x40?0x7fff:0xffff)]
#ifdef FX_ADDRESS_CHECK
#define FETCHPIPE {PIPE=PRGBANK(R15);GSU.vPipeAdr=(GSU.vPrgBankReg<<16)+R15;}
#else
#define FETCHPIPE {PIPE=PRGBANK(R15);}
#endif
static void step(void){
 steps++;R15++;GSU.vStatusReg^=0x100;
 if(steps==5){GSU.vPrgBankReg=0x60;GSU.pvPrgBank=banks[1];R15=0xffff;}
 if(steps==stop_after){GSU.vStatusReg&=~32u;GSU.vCounter=0;GSU.vInstCount=0;}
}
static void (*fx_ppfOpcodeTable[1024])(void);
#define FX_STEP {uint32 vOpcode=(uint32)PIPE;FETCHPIPE;(*fx_ppfOpcodeTable[(GSU.vStatusReg&0x300)|vOpcode])();}
'''
  s+=body(old).replace('fx_run(','reference(')+body(new).replace('fx_run(','candidate(')
  s+=r'''
int main(void){
 for(unsigned i=0;i<1024;i++)fx_ppfOpcodeTable[i]=step;
 for(unsigned i=0;i<65536;i++){banks[0][i]=i; banks[1][i]=i>>8;}
 for(unsigned fx=0;fx<2;fx++)for(unsigned go=0;go<2;go++)
 for(unsigned budget=0;budget<128;budget++)for(unsigned stop=0;stop<130;stop++){
  memset(&GSU,0,sizeof(GSU));GSU.vStatusReg=go?32:0;GSU.pvPrgBank=banks[0];GSU.pc=0xfffe;GSU.vInstCount=7;
  Fx3Active=fx;steps=0;stop_after=stop;__typeof__(GSU) initial=GSU;
  uint32 ret=reference(budget);unsigned n=steps;__typeof__(GSU) expected=GSU;
  GSU=initial;steps=0;assert(candidate(budget)==ret);assert(steps==n);assert(!memcmp(&GSU,&expected,sizeof(GSU)));
 }
 puts("PASS 66560 actual-loop cases: normal/FX3, inactive/STOP, budget wrap, bank/PC transitions and pipeline");
}
'''
  f=p/'fixture.c';f.write_text(s)
  for flags in ([],['-DFX_ADDRESS_CHECK']):
   subprocess.run(['clang','-std=gnu99','-O2','-Wall','-Wextra','-Werror','-fsanitize=address,undefined',*flags,str(f),'-o',str(p/'test')],check=True)
   subprocess.run([str(p/'test')],check=True)
if __name__=='__main__':main()
