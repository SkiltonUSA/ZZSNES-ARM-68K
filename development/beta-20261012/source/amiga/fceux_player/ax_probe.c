/* GPL-3.0-or-later. Enumerate AHI modes; optional allocation only, no sound.
 * Run while other audio applications and Core1 applications are idle. */
#include <exec/types.h>
#include <devices/ahi.h>
#include <proto/exec.h>
#include <proto/ahi.h>
#include <stdio.h>
#include <string.h>
struct Library *AHIBase;
int main(int argc,char **argv){
 struct MsgPort *p=CreateMsgPort();struct AHIRequest *r=NULL;int opened=0,rc=20,found=0;
 ULONG id=AHI_INVALID_ID;
 if(!p)return 20;
 r=(struct AHIRequest *)CreateIORequest(p,sizeof(*r));if(!r)goto done;
 r->ahir_Version=4;if(OpenDevice(AHINAME,AHI_NO_UNIT,(struct IORequest *)r,0))goto done;
 opened=1;AHIBase=(struct Library *)r->ahir_Std.io_Device;
 while((id=AHI_NextAudioID(id))!=AHI_INVALID_ID){
  char name[128]={0};ULONG bits=0,channels=0;
  if(!AHI_GetAudioAttrs(id,NULL,AHIDB_BufferLen,sizeof(name),AHIDB_Name,(ULONG)name,AHIDB_Bits,(ULONG)&bits,AHIDB_MaxChannels,(ULONG)&channels,TAG_DONE))continue;
  printf("AHI_MODE id=%08x bits=%u channels=%u name=%s\n",id,bits,channels,name);
  if(strstr(name,"ZZ9000AX")&&bits==16){
   found=1;
   if(argc==2&&!strcmp(argv[1],"ALLOC")){
    struct AHIAudioCtrl *ctrl=AHI_AllocAudio(AHIA_AudioID,id,AHIA_MixFreq,32000,AHIA_Channels,1,AHIA_Sounds,2,TAG_DONE);
    printf("AX_ALLOC mode=%08x success=%u (no playback)\n",id,!!ctrl);
    if(!ctrl)goto done;
    AHI_FreeAudio(ctrl);
   }
  }
 }
 printf("AX_PROBE found=%u; global preferences unchanged; no PCM or Core1 access\n",found);rc=found?0:5;
done:
 if(opened)CloseDevice((struct IORequest *)r);
 if(r)DeleteIORequest((struct IORequest *)r);
 DeleteMsgPort(p);return rc;
}
