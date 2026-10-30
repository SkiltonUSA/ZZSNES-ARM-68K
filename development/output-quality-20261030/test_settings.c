#include <assert.h>
#include <stdlib.h>
#define EM_AUDIO_FILE "quality-test.cfg"
#include "audio_output.h"
int main(void){
 for(unsigned i=0;i<5;i++){assert(em_audio_save(i));assert(em_audio_load()==i);}assert(!em_audio_save(5));
 FILE*f=fopen(EM_AUDIO_FILE,"w");assert(f);fputs("ZZAudio1 999\n",f);fclose(f);assert(em_audio_load()==0);remove(EM_AUDIO_FILE);assert(em_audio_load()==0);
 unsigned char p[52]={0};memcpy(p,"FORM",4);p[7]=44;memcpy(p+8,"PREFAHIU",8);p[19]=32;p[25]=2;p[27]=12;uint32_t mode;
 for(unsigned n=0;n<52;n++)assert(!em_audio_unit0_mode(p,n,&mode));
 assert(em_audio_unit0_mode(p,52,&mode)&&mode==0x2000c);p[20]=255;assert(!em_audio_unit0_mode(p,52,&mode));p[20]=0;
 p[19]=33;assert(!em_audio_unit0_mode(p,52,&mode));p[19]=32;p[7]=255;assert(!em_audio_unit0_mode(p,52,&mode));
 assert(em_audio_mode(2,"Paula:14 bit stereo",1,14,1));assert(em_audio_mode(3,"Paula:14 bit stereo",1,14,1));assert(em_audio_mode(4,"ZZ9000AX:16 bit Stereo",1,16,1));
 assert(!em_audio_mode(2,"ZZ9000AX:16 bit Stereo",1,16,1));assert(!em_audio_mode(4,"Paula:14 bit stereo",1,14,1));assert(!em_audio_mode(2,"Paula:14 bit stereo",0,14,1));
 puts("PASS output selection persistence, malformed/truncated preferences, routing capabilities");
}
