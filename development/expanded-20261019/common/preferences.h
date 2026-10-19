#pragma once
#include "input_config.h"
#include "cheat_codes.h"
#include "core_options.h"
struct ps_preferences {
 unsigned view,windowed,scanlines,fps,optimise,mute,fire,core,cheats_enabled;
 struct ps_controls controls;
 char cheats[PS_CHEAT_BYTES];
};
static int ps_preferences_valid(const struct ps_preferences*p){
 return !(p->view&~767u)&&((p->view>>4)&3)<3&&((p->view>>6)&3)<3&&p->windowed<2&&p->scanlines<4&&p->fps<2&&p->optimise<2&&p->mute<2&&p->fire<2&&p->cheats_enabled<2&&ps_options_valid(p->core)&&ps_controls_valid(&p->controls)&&ps_cheats_valid(p->cheats);
}
static int ps_preferences_write(FILE*f,const struct ps_preferences*p){
 fprintf(f,"ZZSNESOPTIONS1\n%u %u %u %u %u %u %u %u %u\n",p->view,p->windowed,p->scanlines,p->fps,p->optimise,p->mute,p->fire,p->core,p->cheats_enabled);
 ps_controls_write(f,&p->controls);fprintf(f,"%s\n",p->cheats);return !ferror(f);
}
static int ps_preferences_read(FILE*f,struct ps_preferences*p){
 char line[512];struct ps_preferences t={0};
 if(!fgets(line,sizeof(line),f)||strcmp(line,"ZZSNESOPTIONS1\n"))return 0;
 if(!fgets(line,sizeof(line),f)||sscanf(line,"%u %u %u %u %u %u %u %u %u",&t.view,&t.windowed,&t.scanlines,&t.fps,&t.optimise,&t.mute,&t.fire,&t.core,&t.cheats_enabled)!=9||!ps_controls_read(f,&t.controls))return 0;
 if(!fgets(t.cheats,sizeof(t.cheats),f)||!strchr(t.cheats,'\n'))return 0;t.cheats[strcspn(t.cheats,"\r\n")]=0;
 if(!ps_preferences_valid(&t))return 0;*p=t;return 1;
}
