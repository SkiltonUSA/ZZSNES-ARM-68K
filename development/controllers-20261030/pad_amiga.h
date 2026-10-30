/* GPL-3.0-or-later. Player-1 controller transport. No firmware/config writes.
   Poseidon ownership is temporary; no shared hid.class structures are read. */
#pragma once
#include <proto/poseidon.h>
#include <proto/potgo.h>
#include <clib/alib_protos.h>
#include <utility/hooks.h>
#include "pad_decode.h"
/* Poseidon V4 tag absent from older SDK headers. */
#ifndef ABA_ForceRelease
#define ABA_ForceRelease (ABA_Dummy+0x10)
#endif
struct Library *PsdBase,*PotgoBase;
static APTR volatile pad_binding;
static APTR pad_pipe;
static struct IORequest *pad_io;
static struct MsgPort *pad_port,*pad_owner;
static struct pad_hid pad_descriptor;
static UBYTE pad_packet[64];
static unsigned pad_pending,pad_bits,pad_mode,pad_pot,pad_failed;
static ULONG pad_retry;
static char pad_status[96]="Not active";
static int pad_uses_lowlevel(unsigned mode){return mode==3||mode==4;}
static int pad_read_config(unsigned*mode){
 FILE*f=fopen("PROGDIR:Controller.cfg","r");unsigned n;char extra;
 if(!f){*mode=3;return 1;}int ok=fscanf(f,"ZZPAD1 %u %c",&n,&extra)==1&&n<5;fclose(f);
 if(ok)*mode=n;return ok;
}
static int pad_write_config(unsigned mode){
 FILE*f=fopen("PROGDIR:Controller.tmp","w");if(!f)return 0;
 int ok=fprintf(f,"ZZPAD1 %u\n",mode)>0;if(fclose(f))ok=0;
 if(!ok){DeleteFile("PROGDIR:Controller.tmp");return 0;}
 DeleteFile("PROGDIR:Controller.bak");BPTR old=Lock("PROGDIR:Controller.cfg",ACCESS_READ);
 if(old){UnLock(old);if(!Rename("PROGDIR:Controller.cfg","PROGDIR:Controller.bak"))return 0;}
 if(Rename("PROGDIR:Controller.tmp","PROGDIR:Controller.cfg"))return 1;
 if(old)Rename("PROGDIR:Controller.bak","PROGDIR:Controller.cfg");return 0;
}
static void pad_pipe_close(void){
 if(pad_pipe){if(pad_pending){psdAbortPipe(pad_pipe);psdWaitPipe(pad_pipe);}psdFreePipe(pad_pipe);}
 pad_pipe=NULL;pad_io=NULL;pad_pending=pad_bits=0;
}
static ULONG pad_released(struct Hook*h,APTR object,APTR message){
 (void)h;(void)object;(void)message;
 /* The hook may run in a USB task. Only invalidate the binding here.
    Allocated pipes retain the device after unplug (Poseidon API contract).
    The task that submitted the IO owns abort/wait/free and the reply port. */
 pad_binding=NULL;return 0;
}
static struct Hook pad_hook={{NULL,NULL},(ULONG(*)())HookEntry,(ULONG(*)())pad_released,NULL};
static void pad_usb_release(void){
 if(!PsdBase)return;
 /* On the single 68k, don't let the release hook invalidate the argument
    between the null check and the library consuming it. Its synchronous hub
    request may Wait(), which permits the hub task and our hook to execute. */
 Forbid();int rebound=pad_binding!=NULL;if(pad_binding)psdReleaseAppBinding(pad_binding);Permit();
 pad_pipe_close();if(rebound)psdClassScan();
}
static void pad_gameport_close(void){
 if(pad_pot){WritePotgo(0,0xf000);FreePotBits(0xf000);pad_pot=0;}
}
static void pad_close(void){
 pad_usb_release();pad_gameport_close();
 if(PsdBase){CloseLibrary(PsdBase);PsdBase=NULL;}
 if(pad_port){DeleteMsgPort(pad_port);pad_port=NULL;}
 if(pad_owner){RemPort(pad_owner);DeleteMsgPort(pad_owner);pad_owner=NULL;}
 pad_bits=pad_failed=0;strcpy(pad_status,"Not active");
}
static int pad_open(unsigned mode){
 pad_close();pad_mode=mode;pad_retry=0;
 if(pad_uses_lowlevel(mode))return 1;
 pad_owner=CreateMsgPort();if(!pad_owner)return 0;
 Forbid();int busy=FindPort("ZZSNES.Controller")!=NULL;
 if(!busy){pad_owner->mp_Node.ln_Name="ZZSNES.Controller";AddPort(pad_owner);}Permit();
 if(busy){DeleteMsgPort(pad_owner);pad_owner=NULL;strcpy(pad_status,"Controller in use");return 0;}
 if(mode!=2)PsdBase=OpenLibrary("poseidon.library",4);
 if(PsdBase)pad_port=CreateMsgPort();
 PotgoBase=(struct Library*)OpenResource("potgo.resource");
 strcpy(pad_status,PsdBase?"Looking for SN30 Pro USB":"USB unavailable; gameport fallback");return 1;
}
static int pad_usb_attach(void){
 APTR dev,iface,ep,control=NULL;ULONG number=0,actual=0;UBYTE descriptor[1024];int ok=0,claimed=0;
 if(!PsdBase||!pad_port)return 0;
 psdLockReadPBase();
 dev=psdFindDevice(NULL,DA_VendorID,0x2dc8,DA_ProductID,0x6001,DA_IsConnected,TRUE,TAG_END);
 if(!dev){pad_failed=0;psdUnlockPBase();return 0;}
 if(pad_failed){psdUnlockPBase();return 0;}
 iface=psdFindInterface(dev,NULL,IFA_Class,3,TAG_END);
 ep=iface?psdFindEndpoint(iface,NULL,EA_IsIn,TRUE,EA_TransferType,3,TAG_END):NULL;
 if(!ep){strcpy(pad_status,"SN30 Pro: no HID input endpoint");goto end;}
 /* Do not steal another application's controller. hid.class can temporarily
    yield this specific SN30; releasing us restores its normal class binding. */
 ULONG app_bound=0;psdGetAttrs(PGA_DEVICE,dev,DA_HasAppBinding,&app_bound,TAG_END);
 if(app_bound){strcpy(pad_status,"SN30 Pro is in use by another app");goto end;}
 pad_binding=psdClaimAppBinding(ABA_Device,dev,ABA_ReleaseHook,&pad_hook,ABA_ForceRelease,TRUE,TAG_END);
 if(!pad_binding){strcpy(pad_status,"SN30 Pro could not be opened");goto end;}
 claimed=1;
 psdGetAttrs(PGA_INTERFACE,iface,IFA_InterfaceNum,&number,TAG_END);
 control=psdAllocPipe(dev,pad_port,NULL);if(!control)goto end;
 psdSetAttrs(PGA_PIPE,control,PPA_NakTimeout,TRUE,PPA_NakTimeoutTime,250,TAG_END);
 psdPipeSetup(control,0x81,6,0x2200,number);
 if(psdDoPipe(control,descriptor,sizeof(descriptor)))goto end;
 actual=psdGetPipeActual(control);
 if(actual>sizeof(descriptor)||!pad_hid_parse(&pad_descriptor,descriptor,actual)||!pad_sn30_profile(&pad_descriptor)){
  strcpy(pad_status,"SN30 Pro report format unsupported");goto end;
 }
 pad_pipe=psdAllocPipe(dev,pad_port,ep);if(!pad_pipe)goto end;
 psdGetAttrs(PGA_PIPE,pad_pipe,PPA_IORequest,&pad_io,TAG_END);if(!pad_io)goto end;
 psdSendPipe(pad_pipe,pad_packet,sizeof(pad_packet));pad_pending=1;ok=1;
 strcpy(pad_status,"USB: 8BitDo SN30 Pro (D-input)");
end:
 if(control)psdFreePipe(control);psdUnlockPBase();
 if(!ok&&claimed){pad_failed=1;pad_usb_release();}
 if(ok)pad_gameport_close();return ok;
}
static int pad_usb_poll(void){
 int alive=0;if(!PsdBase)return 0;psdLockReadPBase();
 if(pad_binding&&pad_pipe){alive=1;
  if(pad_pending&&CheckIO(pad_io)){
   LONG err=psdWaitPipe(pad_pipe);ULONG size=psdGetPipeActual(pad_pipe);pad_pending=0;
   if(err||size>sizeof(pad_packet)){pad_bits=0;alive=0;}
   else{unsigned bits;if(pad_hid_read(&pad_descriptor,pad_packet,size,&bits))pad_bits=bits;
    else pad_bits=0;
    psdSendPipe(pad_pipe,pad_packet,sizeof(pad_packet));pad_pending=1;}
  }
 }
 psdUnlockPBase();if(!pad_binding)alive=0;if(!alive&&pad_pipe)pad_usb_release();return alive;
}
/* Passive 5/7 pin-swap adapter only. Port 2 is reserved through potgo.resource;
   port 1/mouse registers and CIA output directions are never changed. */
#ifndef PAD_CIA_READ
#define PAD_CIA_READ() (*(volatile UBYTE*)0xbfe001)
#define PAD_JOY_READ() (*(volatile UWORD*)0xdff00c)
#define PAD_POT_READ() (*(volatile UWORD*)0xdff016)
#endif
static void pad_sega_select(unsigned high){
 /* HRM button input: OUTRY=DATRY=1 provides the sense pull-up.
    Keep pin 9 high; only the pin-5 TH data bit toggles. */
 WritePotgo(high?0xf000:0xe000,0xf000);
 /* CIA bus reads supply a CPU-speed-independent settling interval (~32us).
    Similar phase spacing to ADoom's Sega reader, without whole-POTGO writes. */
 for(unsigned i=0;i<23;i++)(void)PAD_CIA_READ();
}
static unsigned pad_sega_sample(void){
 unsigned j=PAD_JOY_READ(),n=0;
 if(!(PAD_CIA_READ()&128))n|=16;
 if(!(PAD_POT_READ()&0x4000))n|=32;
 if(j&2)n|=8;if(j&512)n|=4;j^=j<<1;if(j&2)n|=2;if(j&512)n|=1;return n;
}
static unsigned pad_gameport_poll(void){
 unsigned normal,low,id,extra,end;
 if(!PotgoBase)return 0;
 if(!pad_pot){unsigned got=AllocPotBits(0xf000);if(got!=0xf000){if(got)FreePotBits(got);strcpy(pad_status,"Gameport busy; close other controller apps");return 0;}pad_pot=1;}
 Disable();pad_sega_select(1);normal=pad_sega_sample();pad_sega_select(0);low=pad_sega_sample();
 pad_sega_select(1);pad_sega_select(0);
 pad_sega_select(1);pad_sega_select(0);id=pad_sega_sample();
 pad_sega_select(1);extra=pad_sega_sample();pad_sega_select(0);end=pad_sega_sample();pad_sega_select(1);Enable();
 strcpy(pad_status,(low&12)!=12?"Gameport: no Mega Drive pad detected":(id&15)==15&&(end&15)==0?"Gameport: Mega Drive 6-button":"Gameport: Mega Drive 3-button");
 return pad_sega(normal,low,id,extra,end);
}
static unsigned pad_poll(ULONG now){
 if(!pad_owner)return 0;
 if(pad_usb_poll())return pad_bits;
 if(pad_mode!=2&&(!pad_retry||(LONG)(now-pad_retry)>=0)){
  pad_retry=now+2000000u;if(pad_usb_attach())return 0;
 }
 if(pad_mode==1){if(!pad_failed)strcpy(pad_status,PsdBase?"Connect SN30 Pro USB in D-input mode":"Poseidon USB unavailable");return 0;}
 return pad_gameport_poll();
}
