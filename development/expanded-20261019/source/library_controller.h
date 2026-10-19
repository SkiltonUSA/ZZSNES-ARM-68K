/* GPL-3.0-or-later. Native controller diagram; no image assets or Core1 use.
 * Lowlevel mapping mirrors the player's joystick.h, including its type gate.
 * The diagram is an indicator, not a button-binding editor. */
static unsigned controller_mapped_bits(ULONG);
static const char *controller_binding(unsigned);
static unsigned controller_binding_hash(void);
static unsigned controller_bits(ULONG s,unsigned swap){(void)swap;return controller_mapped_bits(s);}
#ifndef CONTROLLER_MODEL_ONLY
struct controller_instance {unsigned bits,swap,pad,bindings;LONG active_pen;};
static struct MUI_CustomClass *controller_class;
static Object *controller;
static void controller_text(Object*obj,LONG x,LONG y,LONG width,const char*s){
 struct RastPort*rp=_rp(obj);LONG len=strlen(s),w;
 while(len&&(w=TextLength(rp,s,len))>width-4)len--;
 w=TextLength(rp,s,len);Move(rp,x+(width-w)/2,y);Text(rp,s,len);
}
static void controller_button(Object*obj,struct controller_instance*d,LONG x,LONG y,LONG w,const char*title,const char*binding,unsigned bit){
 if(ui_compact){if(x>=470)x-=258;else if(x>=224){x-=212;y+=96;}}
 struct RastPort*rp=_rp(obj);LONG top=_mtop(obj)+y,left=_mleft(obj)+(_mwidth(obj)-(ui_compact?424:680))/2+x;
 LONG on=bit&&(d->bits&bit),fg=_pens(obj)[MPEN_TEXT];
 SetAPen(rp,on&&d->active_pen>=0?d->active_pen:_pens(obj)[MPEN_BACKGROUND]);RectFill(rp,left,top,left+w-1,top+33);
 SetAPen(rp,_pens(obj)[on?MPEN_SHADOW:MPEN_SHINE]);Move(rp,left,top+33);Draw(rp,left,top);Draw(rp,left+w-1,top);
 SetAPen(rp,_pens(obj)[on?MPEN_SHINE:MPEN_SHADOW]);Draw(rp,left+w-1,top+33);Draw(rp,left,top+33);
 SetAPen(rp,fg);controller_text(obj,left,top+11,w,title);controller_text(obj,left,top+26,w,binding);
}
static ULONG controller_dispatch(struct IClass*cl __asm("a0"),Object*obj __asm("a2"),Msg msg __asm("a1")){
 struct controller_instance*d=INST_DATA(cl,obj);ULONG result;
 switch(msg->MethodID){
 case MUIM_Setup:
  if(!DoSuperMethodA(cl,obj,msg))return FALSE;
  d->active_pen=ObtainBestPen(_screen(obj)->ViewPort.ColorMap,0x55555555u,0xccccccccu,0x77777777u,TAG_DONE);return TRUE;
 case MUIM_Cleanup:
  if(d->active_pen>=0)ReleasePen(_screen(obj)->ViewPort.ColorMap,d->active_pen);d->active_pen=-1;break;
 case MUIM_AskMinMax:{struct MUIP_AskMinMax*m=(struct MUIP_AskMinMax*)msg;result=DoSuperMethodA(cl,obj,msg);
  m->MinMaxInfo->MinWidth+=ui_compact?424:680;m->MinMaxInfo->DefWidth+=ui_compact?424:680;m->MinMaxInfo->MaxWidth=MUI_MAXMAX;
  m->MinMaxInfo->MinHeight+=ui_compact?260:190;m->MinMaxInfo->DefHeight+=ui_compact?260:190;m->MinMaxInfo->MaxHeight+=ui_compact?260:190;return result;}
 case MUIM_Draw:{
  result=DoSuperMethodA(cl,obj,msg);if(!(((struct MUIP_Draw*)msg)->flags&(MADF_DRAWOBJECT|MADF_DRAWUPDATE)))return result;
  struct RastPort*rp=_rp(obj);LONG left=_mleft(obj)+(_mwidth(obj)-(ui_compact?424:680))/2,top=_mtop(obj);
  SetAPen(rp,_pens(obj)[MPEN_BACKGROUND]);RectFill(rp,_mleft(obj),top,_mright(obj),_mbottom(obj));
  SetAPen(rp,_pens(obj)[MPEN_SHADOW]);DrawEllipse(rp,left+104,top+89,97,82);
  controller_button(obj,d,67,18,74,"Up","Stick",16);
  controller_button(obj,d,12,71,74,"Left","Stick",64);
  controller_button(obj,d,122,71,74,"Right","Stick",128);
  controller_button(obj,d,67,124,74,"Down","Stick",32);
  controller_button(obj,d,224,108,94,"Select",controller_binding(2),4);
  controller_button(obj,d,326,108,94,"Start",controller_binding(3),8);
  controller_button(obj,d,470,4,88,"L",controller_binding(10),1024);
  controller_button(obj,d,582,4,88,"R",controller_binding(11),2048);
  controller_button(obj,d,526,48,88,"X",controller_binding(9),512);
  controller_button(obj,d,470,90,88,"Y",controller_binding(1),2);
  controller_button(obj,d,582,90,88,"A",controller_binding(8),256);
  controller_button(obj,d,526,132,88,"B",controller_binding(0),1);
  SetAPen(rp,_pens(obj)[MPEN_TEXT]);controller_text(obj,left+(ui_compact?80:210),top+(ui_compact?250:180),250,"Green = joystick input held");return result;
 }
 }return DoSuperMethodA(cl,obj,msg);
}
static Object*controller_new(void){return NewObject(controller_class->mcc_Class,NULL,MUIA_Frame,MUIV_Frame_Group,TAG_DONE);}
static void controller_update(ULONG s,unsigned swap,int visible){
 struct controller_instance*d=INST_DATA(controller_class->mcc_Class,controller);
 unsigned bits=controller_bits(s,swap),pad=(s&JP_TYPE_MASK)==JP_TYPE_GAMECTLR;
 unsigned bindings=controller_binding_hash();
 if(d->bits!=bits||d->swap!=swap||d->pad!=pad||d->bindings!=bindings){d->bits=bits;d->swap=swap;d->pad=pad;d->bindings=bindings;
  if(visible)MUI_Redraw(controller,MADF_DRAWOBJECT);}
}
#endif
