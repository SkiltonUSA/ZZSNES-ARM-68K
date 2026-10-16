/* Native MUI Area; palette pens work on both 8-bit and RTG Workbench screens. */
#include <proto/graphics.h>
#include <graphics/gfxbase.h>
#include "state_format.h"
struct GfxBase *GfxBase;
static struct MUI_CustomClass *thumbclass;
static Object *thumbnail;
static uint8_t thumbdata[PS_THUMB_BYTES];static int thumbvalid;
struct thumb_instance{LONG pens[64];};
static ULONG thumb_dispatch(struct IClass*cl __asm("a0"),Object*obj __asm("a2"),Msg msg __asm("a1")){
 struct thumb_instance*d=INST_DATA(cl,obj);ULONG result;unsigned i;
 switch(msg->MethodID){
 case MUIM_Setup:
  if(!DoSuperMethodA(cl,obj,msg))return FALSE;
  for(i=0;i<64;i++)d->pens[i]=ObtainBestPen(_screen(obj)->ViewPort.ColorMap,((i>>4)&3)*0x55555555u,((i>>2)&3)*0x55555555u,(i&3)*0x55555555u,TAG_DONE);return TRUE;
 case MUIM_Cleanup:
  for(i=0;i<64;i++)if(d->pens[i]>=0)ReleasePen(_screen(obj)->ViewPort.ColorMap,d->pens[i]);break;
 case MUIM_AskMinMax:{struct MUIP_AskMinMax*m=(struct MUIP_AskMinMax*)msg;result=DoSuperMethodA(cl,obj,msg);m->MinMaxInfo->MinWidth+=128;m->MinMaxInfo->DefWidth+=128;m->MinMaxInfo->MaxWidth+=128;m->MinMaxInfo->MinHeight+=112;m->MinMaxInfo->DefHeight+=112;m->MinMaxInfo->MaxHeight+=112;return result;}
 case MUIM_Draw:
  result=DoSuperMethodA(cl,obj,msg);if(!(((struct MUIP_Draw*)msg)->flags&(MADF_DRAWOBJECT|MADF_DRAWUPDATE)))return result;
  for(unsigned y=0;y<112;y++)for(unsigned x=0;x<128;x++){
   unsigned at=(y*128+x)*2,p=thumbvalid?((unsigned)thumbdata[at]<<8)|thumbdata[at+1]:0;
   unsigned index=((p>>14)&3)*16+((p>>9)&3)*4+((p>>3)&3);LONG pen=d->pens[index];if(pen<0)pen=0;
   SetAPen(_rp(obj),pen);WritePixel(_rp(obj),_mleft(obj)+x,_mtop(obj)+y);
  }return result;
 }return DoSuperMethodA(cl,obj,msg);
}
static void thumb_load(const char *path){FILE*f;uint8_t*p;unsigned n;thumbvalid=0;
 p=AllocVec(PS_STATE_MAX+1,MEMF_PUBLIC);if(!p)goto done;
 f=fopen(path,"rb");if(f){n=fread(p,1,PS_STATE_MAX+1,f);if(!ferror(f)&&ps_valid(p,n)){memcpy(thumbdata,p+PS_THUMB_OFFSET,PS_THUMB_BYTES);thumbvalid=1;}fclose(f);}FreeVec(p);
 done:if(thumbnail)MUI_Redraw(thumbnail,MADF_DRAWOBJECT);
}
