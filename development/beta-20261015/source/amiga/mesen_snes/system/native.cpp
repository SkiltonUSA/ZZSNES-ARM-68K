#include "SNES/SnesConsole.h"
#include "SNES/SnesPpu.h"
#include "SNES/SnesMemoryManager.h"
#include "SNES/SnesCpu.h"
#include "SNES/Spc.h"
#include "SNES/SnesControlManager.h"
#include <cstdio>
#include <cstdlib>
static uint32_t hash(const uint16_t*p,unsigned n){uint32_t h=2166136261u;while(n--)h=(h^*p++)*16777619u;return h;}
static unsigned polls;
static int cancel(){return ++polls==3;}
int main(int argc,char**argv){
 if(argc!=4)return 2;FILE*f=fopen(argv[1],"rb");if(!f)return 2;
 uint8_t rom[32769];unsigned size=fread(rom,1,sizeof(rom),f);fclose(f);
 SnesConsole console;if(!console.Load(rom,size))return 3;
 unsigned frames=atoi(argv[2]);if(!frames||frames>600)return 4;
 for(unsigned i=0;i<frames;i++){
  int rc=console.RunFrame();auto &e=*console.GetEmulator();auto &v=e.video;
  printf("FRAME n=%u rc=%d master=%llu cpu=%llu spc=%llu video=%08x size=%ux%u samples=%u pcm=%08x nonzero=%u\n",i+1,rc,
   (unsigned long long)console.GetMasterClock(),(unsigned long long)console.GetCpu()->GetCycleCount(),(unsigned long long)console.GetSpc()->GetState().Cycle,
   hash(v.pixels,v.width*v.height),v.width,v.height,e.audio.samples,e.audio.hash,e.audio.nonzero);
  if(rc)return rc;
 }
 auto &v=console.GetEmulator()->video;
 f=fopen(argv[3],"wb");if(!f)return 4;fprintf(f,"P6\n%u %u\n255\n",v.width,v.height);
 for(unsigned i=0;i<v.width*v.height;i++){unsigned p=v.pixels[i];unsigned char b[3]={(unsigned char)((p&31)*255/31),(unsigned char)(((p>>5)&31)*255/31),(unsigned char)(((p>>10)&31)*255/31)};fwrite(b,1,3,f);}fclose(f);
 if(frames>=2){
  for(unsigned y=0;y<239;y++)for(unsigned x=0;x<256;x++){
   unsigned want=(y<7||y>=231)?0:((x+y)&1)?0x03e0:0x001f;
   if(v.pixels[y*256+x]!=want)return 5;
  }
 }
 // DMA destination bytes independently equal the cartridge source bytes.
 if(memcmp(console.GetPpu()->GetVideoRam(),rom+0x200,16)||memcmp(console.GetPpu()->GetCgRam(),rom+0x210,8))return 6;
 if(console.GetSpc()->GetState().OutputReg[0]!=0xaa||console.GetSpc()->GetState().OutputReg[1]!=0xbb)return 7;
 auto*m=console.GetMemoryManager();m->Write(0x7e0011,0x53,MemoryOperationType::Write);
 if(m->Peek(0x11)!=0x53||m->Peek(0x800011)!=0x53)return 8;
 auto*pad=console.GetControlManager();pad->SetButtons(0,0xa500);pad->Write(0x4016,1);pad->Write(0x4016,0);
 unsigned buttons=0;for(unsigned i=0;i<16;i++)buttons=(buttons<<1)|(pad->Read(0x4016)&1);
 if(buttons!=0xa500||(pad->Read(0x4016)&1)!=1)return 9;
 SnesConsole cancelled;if(!cancelled.Load(rom,size)||cancelled.RunFrame(cancel)!=2||cancelled.GetEmulator()->video.frames)return 10;
 SnesConsole bad;rom[0x7fd6]=0x34;if(bad.Load(rom,size)||bad.Load(rom,size-1))return 11;
 puts("CHECK pixels=PASS dma=PASS spc_boot_ports=PASS wram_mirrors=PASS serial_pad=PASS cancellation=PASS reject_chip_and_truncation=PASS");
 return 0;
}
