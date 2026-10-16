/* GPL-3.0-or-later. Bounded native MUI previews, no ARM/display-card ownership.
 * Original palette renderer adapted from our SNES thumbnail area. */
#include <stdint.h>
#include <proto/graphics.h>
#include <graphics/gfxbase.h>
#include <proto/datatypes.h>
#include <datatypes/pictureclass.h>
#include "snapshot_format.h"
struct GfxBase *GfxBase;
struct Library *DataTypesBase;
#define PREVIEW_W 128
#define PREVIEW_H 112
#define PREVIEW_BYTES (PREVIEW_W*PREVIEW_H*2)
struct preview_instance {LONG pens[64];UBYTE pixels[PREVIEW_BYTES];int valid;const char *label;};
static struct MUI_CustomClass *preview_class;
static ULONG preview_dispatch(struct IClass*cl __asm("a0"),Object*obj __asm("a2"),Msg msg __asm("a1")){
 struct preview_instance*d=INST_DATA(cl,obj);ULONG result;unsigned i;
 switch(msg->MethodID){
 case MUIM_Setup:
  if(!DoSuperMethodA(cl,obj,msg))return FALSE;
  for(i=0;i<64;i++)d->pens[i]=ObtainBestPen(_screen(obj)->ViewPort.ColorMap,((i>>4)&3)*0x55555555u,((i>>2)&3)*0x55555555u,(i&3)*0x55555555u,TAG_DONE);return TRUE;
 case MUIM_Cleanup:
  for(i=0;i<64;i++)if(d->pens[i]>=0)ReleasePen(_screen(obj)->ViewPort.ColorMap,d->pens[i]);break;
 case MUIM_AskMinMax:{struct MUIP_AskMinMax*m=(struct MUIP_AskMinMax*)msg;result=DoSuperMethodA(cl,obj,msg);
  m->MinMaxInfo->MinWidth+=112;m->MinMaxInfo->DefWidth+=112;m->MinMaxInfo->MaxWidth+=112;
  m->MinMaxInfo->MinHeight+=98;m->MinMaxInfo->DefHeight+=98;m->MinMaxInfo->MaxHeight+=98;return result;}
 case MUIM_Draw:
  result=DoSuperMethodA(cl,obj,msg);if(!(((struct MUIP_Draw*)msg)->flags&(MADF_DRAWOBJECT|MADF_DRAWUPDATE)))return result;
  for(unsigned y=0;y<98;y++)for(unsigned x=0;x<112;x++){
   unsigned at=((y*PREVIEW_H/98)*PREVIEW_W+x*PREVIEW_W/112)*2,p=d->valid?((unsigned)d->pixels[at]<<8)|d->pixels[at+1]:0;
   LONG pen=d->pens[((p>>14)&3)*16+((p>>9)&3)*4+((p>>3)&3)];if(pen<0)pen=0;
   SetAPen(_rp(obj),pen);WritePixel(_rp(obj),_mleft(obj)+x,_mtop(obj)+y);
  }
  if(!d->valid){const char*s=d->label?d->label:"No artwork";SetAPen(_rp(obj),d->pens[63]>=0?d->pens[63]:1);Move(_rp(obj),_mleft(obj)+4,_mtop(obj)+PREVIEW_H/2);unsigned n=strlen(s);while(n&&TextLength(_rp(obj),s,n)>_mwidth(obj)-8)n--;Text(_rp(obj),s,n);}return result;
 }return DoSuperMethodA(cl,obj,msg);
}
static Object *preview_new(void){return NewObject(preview_class->mcc_Class,NULL,MUIA_Frame,MUIV_Frame_Text,TAG_DONE);}
static void preview_set(Object*obj,const UBYTE*data,const char*label){struct preview_instance*d=INST_DATA(preview_class->mcc_Class,obj);
 d->valid=data!=NULL;d->label=label;if(data)memcpy(d->pixels,data,PREVIEW_BYTES);MUI_Redraw(obj,MADF_DRAWOBJECT);}
static void preview_copy(Object*dst,Object*src){struct preview_instance*d=INST_DATA(preview_class->mcc_Class,src);preview_set(dst,d->valid?d->pixels:NULL,d->label);}
static int preview_snapshot(Object*obj,const char*path,uint32_t id,uint32_t id2,unsigned bytes){FILE*f;UBYTE*p;unsigned n;int ok=0;
 p=AllocVec(SS_BYTES+1,MEMF_PUBLIC);if(!p)return 0;f=fopen(path,"rb");if(f){n=fread(p,1,SS_BYTES+1,f);ok=!ferror(f)&&ss_valid(p,n,id,id2,bytes);fclose(f);if(ok)preview_set(obj,p+SS_HEADER,NULL);}FreeVec(p);return ok;
}
/* Optional OS picture datatypes, V43 pixel-array interface. Decoder failures
 * leave a placeholder. Read only one source row at a time, letterbox to fit.
 * Imported artwork should be <=512x512 / 1 MiB; no downloads while browsing. */
static int preview_art(Object*obj,const char*path){Object*dt;struct BitMapHeader*bm=NULL;UBYTE row[512*3],pixels[PREVIEW_BYTES];
 struct pdtBlitPixelArray read;unsigned w,h,x,y,dx,dy;int ok=0;BPTR lock;struct FileInfoBlock*fib;
 if(!DataTypesBase||!*path)return 0;fib=AllocDosObject(DOS_FIB,NULL);lock=Lock(path,ACCESS_READ);
 if(!fib||!lock){if(fib)FreeDosObject(DOS_FIB,fib);if(lock)UnLock(lock);return 0;}
 ok=Examine(lock,fib)&&fib->fib_DirEntryType<0&&fib->fib_Size>0&&fib->fib_Size<=1048576;UnLock(lock);FreeDosObject(DOS_FIB,fib);if(!ok)return 0;ok=0;
 dt=NewDTObject((CONST_STRPTR)path,DTA_SourceType,DTST_FILE,DTA_GroupID,GID_PICTURE,PDTA_Remap,FALSE,PDTA_DestMode,PMODE_V43,TAG_DONE);if(!dt)return 0;
 GetDTAttrs(dt,PDTA_BitMapHeader,(ULONG)&bm,TAG_DONE);
 if(!bm||!bm->bmh_Width||!bm->bmh_Height||bm->bmh_Width>512||bm->bmh_Height>512)goto done;
 w=PREVIEW_W;h=bm->bmh_Height*w/bm->bmh_Width;if(h>PREVIEW_H){h=PREVIEW_H;w=bm->bmh_Width*h/bm->bmh_Height;}if(!w||!h)goto done;
 dx=(PREVIEW_W-w)/2;dy=(PREVIEW_H-h)/2;memset(pixels,0,sizeof(pixels));memset(&read,0,sizeof(read));
 read.MethodID=PDTM_READPIXELARRAY;read.pbpa_PixelData=row;read.pbpa_PixelFormat=PBPAFMT_RGB;read.pbpa_PixelArrayMod=bm->bmh_Width*3;read.pbpa_Width=bm->bmh_Width;read.pbpa_Height=1;
 for(y=0;y<h;y++){read.pbpa_Top=y*bm->bmh_Height/h;if(!DoDTMethodA(dt,NULL,NULL,(Msg)&read))goto done;
  for(x=0;x<w;x++){unsigned sx=x*bm->bmh_Width/w,at=((dy+y)*PREVIEW_W+dx+x)*2;
   unsigned rgb=((row[sx*3]&248)<<8)|((row[sx*3+1]&252)<<3)|(row[sx*3+2]>>3);pixels[at]=rgb>>8;pixels[at+1]=rgb;}}
 preview_set(obj,pixels,NULL);ok=1;
done:DisposeDTObject(dt);return ok;
}
