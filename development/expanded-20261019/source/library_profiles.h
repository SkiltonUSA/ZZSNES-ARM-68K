/* Per-ROM sidecars leave the large library format and global defaults intact. */
static void profile_path(char*p,const char*rom){
 unsigned second=5381;for(const unsigned char*s=(const unsigned char*)rom;*s;s++)second=second*33u+*s;
 snprintf(p,UI_PATH,"PROGDIR:Profiles/%08x-%08x.cfg",ui_hash(rom),second);
}
static int settings_replace(const char*tmp,const char*path){char backup[UI_PATH+8];BPTR lock;int old=0;
 snprintf(backup,sizeof(backup),"%s.bak",path);lock=Lock(path,ACCESS_READ);if(lock){UnLock(lock);old=1;}else if(IoErr()!=ERROR_OBJECT_NOT_FOUND)return 0;
 if(old){if(delete_state_file(backup)<0||!Rename(path,backup))return 0;}
 if(!Rename(tmp,path)){if(old)Rename(backup,path);return 0;}return 1;
}
static int profile_write(void){
 FILE*f;int ok;struct ps_preferences p,check;char path[UI_PATH],tmp[UI_PATH+8],line[UI_PATH*2+4],rom[UI_PATH];
 settings_get(&p);if(!ps_preferences_valid(&p)||profile_bad)return 0;
 /* Cheats always belong to the selected game, even when editing defaults. */
 if(!game_profile&&p.cheats[0]){if(selected<0)return 0;game_profile=1;set(profile_label,MUIA_Text_Contents,(ULONG)"Settings: selected game");}
 if(game_profile){
  if(selected<0)return 0;BPTR dir=CreateDir("PROGDIR:Profiles");if(dir)UnLock(dir);
  profile_path(path,model.entry[selected].path);snprintf(tmp,sizeof(tmp),"%s.tmp",path);
  f=fopen(tmp,"w");if(!f)return 0;ui_hex(f,model.entry[selected].path);fputc('\n',f);ok=ps_preferences_write(f,&p);if(fclose(f))ok=0;if(!ok)return 0;
  f=fopen(tmp,"r");if(!f)return 0;ok=fgets(line,sizeof(line),f)&&ui_unhex(rom,line)&&!strcmp(rom,model.entry[selected].path)&&ps_preferences_read(f,&check)&&!memcmp(&p,&check,sizeof(p));fclose(f);
  return ok&&settings_replace(tmp,path);
 }
 f=fopen("PROGDIR:Controls.tmp","w");if(!f)return 0;ok=ps_controls_write(f,&default_controls);if(fclose(f))ok=0;if(!ok)return 0;
 f=fopen("PROGDIR:Controls.tmp","r");if(!f)return 0;struct ps_controls c;ok=ps_controls_read(f,&c)&&!memcmp(&c,&default_controls,sizeof(c));fclose(f);
 return ok&&settings_replace("PROGDIR:Controls.tmp","PROGDIR:Controls.cfg");
}
static void profile_select(void){
 const char*rom=selected>=0?model.entry[selected].path:"";struct ps_preferences p;
 if(!strcmp(rom,profile_rom))return;
 snprintf(profile_rom,sizeof(profile_rom),"%s",rom);game_profile=0;profile_bad=0;settings_defaults(&p);
 if(*rom){char path[UI_PATH],line[UI_PATH*2+4],stored[UI_PATH];FILE*f;profile_path(path,rom);f=fopen(path,"r");
  if(f){struct ps_preferences loaded;int ok=fgets(line,sizeof(line),f)&&ui_unhex(stored,line)&&!strcmp(stored,rom)&&ps_preferences_read(f,&loaded);fclose(f);
   if(ok){p=loaded;game_profile=1;}else profile_bad=1;
  }else if(errno!=ENOENT)profile_bad=1;
 }
 settings_set(&p);if(profile_bad){set(profile_label,MUIA_Text_Contents,(ULONG)"Game profile needs attention");message("Profile unreadable; use defaults or repair the profile.");}
}
static void profile_reset_game(void){
 if(selected>=0){char path[UI_PATH];profile_path(path,model.entry[selected].path);if(delete_state_file(path)<0){message("Could not remove game profile.");return;}}
 game_profile=profile_bad=0;struct ps_preferences p;settings_defaults(&p);settings_set(&p);message("Using global defaults.");
}
