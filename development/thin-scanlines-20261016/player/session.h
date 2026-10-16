/* GPL-3.0-or-later. Parsed before the ARM launcher allocates anything. */
#include "ui_model.h"
static char fc_session_rom[UI_PATH],fc_slot_path[UI_PATH],fc_snapshot_path[UI_PATH];
static unsigned fc_view=0,fc_fire,fc_resume;
static int fc_state_action,fc_snapshot_action;
static int fc_session(const char *path){
 FILE *f=fopen(path,"r");char line[1100];unsigned muted;int ok=0;
 if(!f)return 0;
 if(!fgets(line,sizeof(line),f)||strcmp(line,"ZZSNESSESSION2\n"))goto end;
 if(!fgets(line,sizeof(line),f)||sscanf(line,"%u %u %u %u",&fc_view,&muted,&fc_fire,&fc_resume)!=4||
 fc_view>=256||((fc_view>>4)&3)>2||((fc_view>>6)&3)>2||muted>1||fc_fire>1||fc_resume>1)goto end;
 if(!fgets(line,sizeof(line),f)||!ui_unhex(fc_session_rom,line))goto end;
 if(!fgets(line,sizeof(line),f)||!ui_unhex(fc_slot_path,line))goto end;
 /* Optional extension: older libraries and players retain their session ABI. */
 fc_snapshot_path[0]=0;
 if(fgets(line,sizeof(line),f)&&!ui_unhex(fc_snapshot_path,line))goto end;
 if(fgets(line,sizeof(line),f)){
 if(sscanf(line,"DISPLAY %u %u %u %u",&fc_windowed,&fc_scanlines,&fc_fps,&fc_optimise)!=4||fc_windowed>1||fc_scanlines>4||fc_fps>1||fc_optimise>1)goto end;}
 if(ferror(f))goto end;
 rom_path=fc_session_rom;player_muted=muted;ok=1;
end:fclose(f);return ok;
}
