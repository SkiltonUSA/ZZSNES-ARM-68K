/* GPL-3.0-or-later. Per-app output selection; never writes system preferences.
 * 0 Auto, 1 configured AHI, 2 Paula 22.05 kHz, 3 Paula 28 kHz, 4 ZZ9000AX. */
#pragma once
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#ifndef EM_AUDIO_FILE
#define EM_AUDIO_FILE "PROGDIR:Audio.cfg"
#endif
static unsigned em_audio_load(void){
 unsigned choice=0;char extra;FILE*f=fopen(EM_AUDIO_FILE,"r");
 if(f){if(fscanf(f,"ZZAudio1 %u %c",&choice,&extra)!=1||choice>4)choice=0;fclose(f);}return choice;
}
static int em_audio_save(unsigned choice){
 FILE*f;if(choice>4)return 0;f=fopen(EM_AUDIO_FILE,"w");if(!f)return 0;
 int ok=fprintf(f,"ZZAudio1 %u\n",choice)>0;if(fclose(f))ok=0;return ok;
}
static uint32_t em_audio_be32(const unsigned char*p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
/* Bounds-checked IFF preference reader. No dependency on native struct packing. */
static int em_audio_unit0_mode(const unsigned char*p,size_t n,uint32_t*mode){
 if(n<12||memcmp(p,"FORM",4)||memcmp(p+8,"PREF",4))return 0;
 uint32_t size=em_audio_be32(p+4);if(size<4||size>n-8)return 0;
 size_t end=8+size,off=12;
 while(off+8<=end){uint32_t len=em_audio_be32(p+off+4);off+=8;if(len>end-off)return 0;
  if(!memcmp(p+off-8,"AHIU",4)&&len>=12&&p[off]==0){*mode=em_audio_be32(p+off+4);return 1;}
  off+=len;if(len&1){if(off==end)return 0;off++;}
 }return 0;
}
#ifdef EM_AUDIO_AHI
#ifndef EM_AUDIO_PREFS_FILE
#define EM_AUDIO_PREFS_FILE "ENV:Sys/ahi.prefs"
#endif
static int em_audio_unit0_paula(void){
 unsigned char p[4096];uint32_t id;size_t n;FILE*f=fopen(EM_AUDIO_PREFS_FILE,"rb");if(!f)return 0;
 n=fread(p,1,sizeof(p),f);int ok=!ferror(f);fclose(f);
 if(!ok||!em_audio_unit0_mode(p,n,&id))return 0;
 /* Driver IDs are not allocated here: confirm the registered mode's name. */
 char name[128]={0};return AHI_GetAudioAttrs(id,NULL,AHIDB_BufferLen,sizeof(name),AHIDB_Name,(ULONG)name,TAG_DONE)&&!strncmp(name,"Paula:",6);
}
#endif
static int em_audio_mode(unsigned choice,const char*name,unsigned stereo,unsigned bits,unsigned realtime){
 return stereo&&realtime&&((choice==4&&bits==16&&!strcmp(name,"ZZ9000AX:16 bit Stereo"))||
  ((choice==2||choice==3)&&bits>=14&&!strcmp(name,"Paula:14 bit stereo")));
}
