"""Opt-in native launcher/display/state candidate; accepted modes unchanged."""
from fceux_release import once
def frontend(name,s):
 if name=='host.c':
  s=once(s,'int nes_arguments(int argc,char **argv){','#include "session.h"\nint nes_arguments(int argc,char **argv){\n if(argc==3&&!strcmp(argv[1],"--session"))return fc_session(argv[2])?1:-1;')
  s=once(s,'#include "player_frontend.h"','#include "player_frontend.h"\n#include "state_host.h"\n#include "view.h"')
  s=once(s,'else if(key==0x37)', 'else if(key==0x54||key==0x57){if(!(code&128))fc_state_action=key==0x54?1:2;}\n   else if(key==0x37)')
  s=once(s,'full_slots.render*240*pitch+32*4','full_slots.render*240*pitch')
  s=once(s,'span=239*ri.BytesPerRow+256*bpp;','span=239*ri.BytesPerRow+320*bpp;')
  s=once(s,'ad_put(shared+MB_HOST,16,ri.BytesPerRow);','ad_put(shared+MB_HOST,32,fc_view);\n ad_put(shared+MB_HOST,16,ri.BytesPerRow);')
  s=once(s,'for(y=0;y<240;y++)for(x=0;x<256;x++){','for(y=0;y<240;y++)for(x=0;x<320;x++){')
  old='{unsigned index=((unsigned)indices[2*(y*256+x)]<<8)|indices[2*(y*256+x)+1];if(index>=512){bad++;continue;}c=nes_player_palette[index];}'
  s=once(s,old,'{int sx=fv_source(x,fc_view);if(sx<0||y<((fc_view>>6)&3)*8||y>=240-((fc_view>>6)&3)*8)c=fv_border(x,y,fc_view);else {unsigned index=((unsigned)indices[2*(y*256+sx)]<<8)|indices[2*(y*256+sx)+1];if(index>=512){bad++;continue;}c=nes_player_palette[index];}}')
  s=once(s,'if(!player_load_save())goto done;','if(!player_load_save())goto done;\n if(fc_resume&&fc_state_transaction(1)!=1)goto done;')
  # At top of loop after preceding command and flip, before another frame.
  s=once(s,'\n  if(player_paused){','\n  if(fc_state_action){int action=fc_state_action;fc_state_action=0;\n   if(!full_drain()||fc_state_transaction(action==2)<0)goto done;}\n  if(player_paused){')
  # Pause loop still services F5/F8 on next iteration.
  from game_snapshots import frontend as snapshots
  s=snapshots(s)
 elif name=='joystick.h':
  s=once(s,'fc_joy_buttons=b;return buttons|b;', 'if(fc_fire){UBYTE a=b&1;b=(b&~3u)|((b&2)>>1)|(a<<1);}\n fc_joy_buttons=b;return buttons|b;')
 elif name=='player_audio.h':
  s='static volatile int audio_state_drain,audio_state_drained;\n'+s
  s=once(s,'  Wait(mask|(1UL<<port->mp_SigBit));','''  if(audio_state_drain&&!available){
   int busy=0;
   for(i=0;i<2;i++)if(active[i]&&!CheckIO((struct IORequest *)req[i]))busy=1;
   if(!busy){
    for(i=0;i<2;i++)if(active[i]){
     profile_wait_io((struct IORequest *)req[i]);active[i]=0;
     if(req[i]->ahir_Std.io_Error)audio_error=1;
     else audio_completed+=submitted_count[i];
    }
    primed=0;audio_state_drained=1;
   }
  }
  Wait(mask|(1UL<<port->mp_SigBit));''')
 return s
def worker(s):
 s=s.replace('#include "nes_api.h"','#include "nes_api.h"\nextern unsigned fc_state_store(uint8_t *,unsigned);\nextern int fc_state_load(const uint8_t *,unsigned);')
 s=once(s,'  }else if(op==NES_FRAME && initialized){','''  }else if((op==10||op==11)&&initialized){
   uint32_t result=0;
   if(op==10&&arg>=32&&arg<=NES_SAVE_LIMIT){
    for(uint32_t a=(uint32_t)(base+NES_SAVE)&~31u;a<(uint32_t)(base+NES_SAVE)+arg;a+=32)__asm__ volatile("mcr p15,0,%0,c7,c6,1"::"r"(a):"memory");barrier();
    result=fc_state_load((const uint8_t *)base+NES_SAVE,arg);
   }else if(op==11){result=fc_state_store((uint8_t *)base+NES_SAVE,NES_SAVE_LIMIT);if(result)clean_range(base+NES_SAVE,result);}
   ad_put(reply,52,result);
  }else if(op==NES_FRAME && initialized){''')
 return s
def direct(s):
 s=once(s,'#include "fast_pixels.h"','#include "player_palette.h"\n#include "view.h"')
 s=once(s,'uint32_t error=0,bpp=nd_bpp(format),row_words[256];','uint32_t error=0,row_words[320],opt=ad_get(base+MB_HOST,32);')
 s=once(s,'if(ad_get(dst,0)!=nonce', 'if(!fv_valid(opt)||(format!=3&&format!=4))error=6;\n else if(ad_get(dst,0)!=nonce')
 s=once(s,'nd_pack_row(row_words,src+y*256,format);','fv_row(row_words,src+y*256,y,opt,format);')
 s=once(s,'x<256*bpp/4','x<320')
 return s
