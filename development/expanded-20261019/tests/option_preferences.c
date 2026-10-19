#include <assert.h>
#include "preferences.h"
int main(void){
 struct ps_preferences a={0},b={0};ps_controls_default(&a.controls);a.core=128|(96u<<9)|(51u<<16);a.view=512|4;a.cheats_enabled=1;strcpy(a.cheats,"7E12345A;DF47-0915");a.controls.second=2;
 a.controls.keys[1][0]=15;a.controls.joy[1][3]=11;
 FILE*f=tmpfile();assert(f&&ps_preferences_write(f,&a));rewind(f);assert(ps_preferences_read(f,&b));fclose(f);assert(!memcmp(&a,&b,sizeof(a)));
 assert(ps_key_mask(&b.controls,0,0x31)==1);assert(ps_key_mask(&b.controls,1,0x23)==1);assert(!ps_key_mask(&b.controls,0,0x59));
 assert(ps_options_valid(a.core)&&ps_filter_strength(a.core)==95&&ps_skip_threshold(a.core)==50&&ps_skip_mode(a.core)==5);
 assert(!ps_cheats_valid("7E12345AZ"));assert(!ps_cheats_valid("XXXX-0000"));
 char too_many[PS_CHEAT_BYTES];memset(too_many,0,sizeof(too_many));for(unsigned i=0;i<33;i++)strcat(too_many,"7E12345A;");assert(!ps_cheats_valid(too_many));
 f=tmpfile();assert(f);fputs("INPUT 9999999999999999999999999999999\n",f);rewind(f);assert(!ps_controls_read(f,&b.controls));fclose(f);
 puts("PASS profile/options/control roundtrip, key mapping, bounded numbers and cheat syntax");return 0;
}
