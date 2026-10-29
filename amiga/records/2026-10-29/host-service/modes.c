#include <stdio.h>
#include <stddef.h>
#include <exec/types.h>
#include <devices/ahi.h>
#include <proto/exec.h>
#include <proto/ahi.h>
struct Library *AHIBase;
int main(void){
 struct MsgPort *port=CreateMsgPort();struct AHIRequest *r=NULL;ULONG id=AHI_INVALID_ID;int opened=0,rc=20;
 if(!port)return 20;r=(struct AHIRequest *)CreateIORequest(port,sizeof(*r));if(!r)goto done;
 r->ahir_Version=4;if(OpenDevice(AHINAME,AHI_NO_UNIT,(struct IORequest *)r,0))goto done;
 opened=1;AHIBase=(struct Library *)r->ahir_Std.io_Device;
 printf("AHI version=%u.%u prefs_size=%u unit_offset=%u mode_offset=%u frequency_offset=%u\n",AHIBase->lib_Version,AHIBase->lib_Revision,(unsigned)sizeof(struct AHIUnitPrefs),(unsigned)offsetof(struct AHIUnitPrefs,ahiup_Unit),(unsigned)offsetof(struct AHIUnitPrefs,ahiup_AudioMode),(unsigned)offsetof(struct AHIUnitPrefs,ahiup_Frequency));
 while((id=AHI_NextAudioID(id))!=AHI_INVALID_ID){char name[128]={0};
  if(AHI_GetAudioAttrs(id,NULL,AHIDB_BufferLen,sizeof(name),AHIDB_Name,(ULONG)name,TAG_DONE))printf("MODE id=%08x name=%s\n",id,name);
 }
 rc=0;
done:if(opened)CloseDevice((struct IORequest *)r);if(r)DeleteIORequest((struct IORequest *)r);DeleteMsgPort(port);return rc;
}
