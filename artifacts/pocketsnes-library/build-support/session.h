/* GPL-3.0-or-later. Parsed before the ARM launcher allocates anything. */
#include "ui_model.h"
static char fc_session_rom[UI_PATH],fc_slot_path[UI_PATH];
static unsigned fc_view=4,fc_fire,fc_resume;
static int fc_state_action;
static int fc_session(const char *path){
 FILE *f=fopen(path,"r");char line[1100];unsigned muted;int ok=0;
 if(!f)return 0;
 if(!fgets(line,sizeof(line),f)||strcmp(line,"ZZSNESSESSION2\n"))goto end;
 if(!fgets(line,sizeof(line),f)||sscanf(line,"%u %u %u %u",&fc_view,&muted,&fc_fire,&fc_resume)!=4||
 fc_view>=256||((fc_view>>1)&7)>4||((fc_view>>4)&3)>2||((fc_view>>6)&3)>2||muted>1||fc_fire>1||fc_resume>1)goto end;
 if(!fgets(line,sizeof(line),f)||!ui_unhex(fc_session_rom,line))goto end;
 if(!fgets(line,sizeof(line),f)||!ui_unhex(fc_slot_path,line))goto end;
 rom_path=fc_session_rom;player_muted=muted;ok=1;
end:fclose(f);return ok;
}
