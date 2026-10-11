/* Supplemental state omitted by upstream snapshot.c. No persisted pointers.
 * Private ABI: sizeof/endianness match is checked in our outer state header. */
#include "gfx.h"
extern int DummyEchoBuffer[SOUND_BUFFER_SIZE];
struct ps_mixer {
 InternalPPU ippu;uint8 regs[11];SSoundData sound;SoundStatus status;int echo[24000],loop[16],filter[8],prev[2];unsigned long z;
};
unsigned ps_mixer_size(void){return sizeof(struct ps_mixer);}
void ps_mixer_state(int load,void *data){
 static struct ps_mixer storage;struct ps_mixer*m=&storage;
 if(load)memcpy(m,data,sizeof(*m));
 if(!load){
  memset(m,0,sizeof(*m));m->sound=SoundData;m->status=so;m->ippu=IPPU;
  for(unsigned i=0;i<3;i++){m->ippu.TileCache[i]=0;m->ippu.TileCached[i]=0;}m->ippu.XB=0;memcpy(m->regs,&GFX.r212c,11);
  for(unsigned i=0;i<8;i++){m->sound.channels[i].block=0;m->sound.channels[i].echo_buf_ptr=0;}
  memcpy(m->echo,Echo,sizeof(Echo));memcpy(m->loop,Loop,sizeof(m->loop));memcpy(m->filter,FilterTaps,sizeof(m->filter));memcpy(m->prev,MixOutputPrev,sizeof(m->prev));m->z=Z;
 }else{
  SoundData=m->sound;so=m->status;
  for(unsigned i=0;i<3;i++){m->ippu.TileCache[i]=IPPU.TileCache[i];m->ippu.TileCached[i]=IPPU.TileCached[i];}m->ippu.XB=IPPU.XB;IPPU=m->ippu;memcpy(&GFX.r212c,m->regs,11);
  for(unsigned i=0;i<8;i++){SoundData.channels[i].block=SoundData.channels[i].decoded;SoundData.channels[i].echo_buf_ptr=(SoundData.echo_enable&(1<<i))?EchoBuffer:DummyEchoBuffer;}
  memcpy(Echo,m->echo,sizeof(Echo));memcpy(Loop,m->loop,sizeof(m->loop));memcpy(FilterTaps,m->filter,sizeof(m->filter));memcpy(MixOutputPrev,m->prev,sizeof(m->prev));Z=m->z;
 }
 if(!load)memcpy(data,m,sizeof(*m));
}
