/* Isolated diagnostic drawer only. Exercises real MUI controls and sidecars. */
static int expanded_options_ui_probe(void){
 struct ps_preferences defaults,game,actual;char a[UI_PATH],b[UI_PATH];int errors=0;
 settings_defaults(&defaults);settings_set(&defaults);settings_get(&actual);if(actual.core!=defaults.core)errors++;game_profile=0;collect();if(!profile_write())return 1;
 snprintf(a,sizeof(a),"%s/Profile-A.sfc",appdir);snprintf(b,sizeof(b),"%s/Profile-B.sfc",appdir);
 int ia=ui_add(&model,a),ib=ui_add(&model,b);if(ia<0||ib<0)return 1;
 selected=ia;profile_select();game=defaults;game.core=128|(81u<<9)|(51u<<16)|256;game.controls.second=1;game.controls.keys[1][0]=15;game.controls.joy[0][0]=8;
#ifdef PS_TIMED_FX
 if(defaults.core&PS_TIMED_FX)errors++;
 game.core|=PS_TIMED_FX;
#endif
 game.cheats_enabled=1;strcpy(game.cheats,"7E12345A");game_profile=1;settings_set(&game);if(controller_mapped_bits(JP_TYPE_JOYSTK|JPF_BUTTON_RED)!=256)errors++;if(!profile_write())errors++;
 selected=ib;profile_select();settings_get(&actual);
 if(actual.controls.second||actual.cheats[0]||game_profile||actual.windowed!=defaults.windowed||ps_skip_mode(actual.core)!=ps_skip_mode(defaults.core))errors++;
 selected=ia;profile_select();settings_get(&actual);
 if(!game_profile||profile_bad||actual.controls.second!=1||actual.controls.keys[1][0]!=15||strcmp(actual.cheats,game.cheats)||actual.core!=game.core)errors++;
 profile_reset_game();settings_get(&actual);if(game_profile||actual.cheats[0]||actual.controls.second||actual.core!=defaults.core)errors++;
 printf("OPTIONS_UI %s errors=%d profile isolation/reload/reset, cheats, mapping and sliders\n",errors?"FAIL":"PASS",errors);fflush(stdout);return errors;
}
