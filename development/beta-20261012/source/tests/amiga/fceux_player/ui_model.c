#include "ui_model.h"
#include <assert.h>
static struct ui_model a,b;
static void roundtrip(void){FILE*f=tmpfile();assert(f);assert(ui_write(f,&a));rewind(f);assert(ui_read(f,&b));assert(!memcmp(&a,&b,sizeof(a)));fclose(f);}
int main(void){FILE *f;char quoted[1100],title[UI_TITLE];unsigned i;int visible[UI_MAX];ui_defaults(&a);
 assert(ui_add(&a,"SD032G:ROMs/A *\"quoted\" game.nes")==0);a.entry[0].favorite=1;a.entry[0].played=17;a.clock=18;a.theme=4;a.vmask=2;a.slot=4;
 assert(ui_add(&a,a.entry[0].path)==0);assert(a.count==1);assert(ui_add(&a,"bad\nfile")==-1);
 for(i=1;i<UI_MAX;i++){char p[60];snprintf(p,sizeof(p),"ROMs/game-%u.nes",i);assert(ui_add(&a,p)==(int)i);}assert(ui_add(&a,"too-many.nes")==-1);
 for(i=5;i<=6;i++){a.theme=i;roundtrip();}a.theme=4;
 strcpy(a.entry[0].title,"A friendly title");strcpy(a.entry[0].art,"SD032G:Artwork/A game.iff");a.last=1;a.filter=2;roundtrip();
 assert(ui_quote(quoted,sizeof(quoted),"A *\"B\".nes"));assert(!strcmp(quoted,"\"A ***\"B*\".nes\""));assert(!ui_quote(quoted,3,"toolong"));
 f=tmpfile();fputs(UI_CONFIG_PREFIX "1\n0 9 0 0 0 0 0 0\n",f);rewind(f);assert(!ui_read(f,&b));fclose(f);
 /* Old libraries migrate without changing the path used to find NES saves. */
 f=tmpfile();fputs(UI_CONFIG_PREFIX "1\n7 1 2 1 0 4 1 1\n1 7 524f4d733a67616d652e6e6573\n",f);rewind(f);assert(ui_read(f,&b));fclose(f);
 assert(b.count==1&&!strcmp(b.entry[0].path,"ROMs:game.nes")&&b.entry[0].favorite&&b.entry[0].played==7&&b.slot==4&&!b.entry[0].title[0]&&!b.entry[0].art[0]);
 a=b;roundtrip();assert(ui_hash(a.entry[0].path)==ui_hash(b.entry[0].path));
 memset(a.entry[0].path,'p',UI_PATH-1);a.entry[0].path[UI_PATH-1]=0;
 memset(a.entry[0].art,'a',UI_PATH-1);a.entry[0].art[UI_PATH-1]=0;
 memset(a.entry[0].title,'t',UI_TITLE-1);a.entry[0].title[UI_TITLE-1]=0;roundtrip();
 ui_defaults(&a);ui_add(&a,"ROMs:Zelda_game.nes");ui_add(&a,"ROMs:Mario.sfc");ui_add(&a,"ROMs:Another.smc");
 a.entry[0].favorite=1;ui_played(&a,0);ui_played(&a,1);
 assert(ui_visible(&a,0,"",visible)==3&&visible[0]==2&&visible[1]==1&&visible[2]==0);
 assert(ui_visible(&a,1,"",visible)==2&&visible[0]==1&&visible[1]==0);
 assert(ui_visible(&a,2,"",visible)==1&&visible[0]==0);
 assert(ui_visible(&a,0,"MaRiO",visible)==1&&visible[0]==1);
 assert(ui_visible(&a,2,"Mario",visible)==0);
 ui_title(&a.entry[0],title);assert(!strcmp(title,"Zelda game"));
 strcpy(a.entry[0].title,"\033bMy\tTitle");ui_title(&a.entry[0],title);assert(!strcmp(title," bMy Title"));
 assert(ui_visible(&a,0,"my",visible)==1&&visible[0]==0);
 assert(ui_visible(&a,0,"Zelda_game.nes",visible)==1&&visible[0]==0);
 a.clock=UINT_MAX;a.entry[1].played=UINT_MAX;ui_played(&a,0);assert(a.entry[0].played>a.entry[1].played);roundtrip();
 /* Reusing the tail after remove must not inherit its old title/art/favourite. */
 a.count=0;assert(ui_add(&a,"new.nes")==0);assert(!a.entry[0].title[0]&&!a.entry[0].art[0]&&!a.entry[0].favorite&&!a.entry[0].played);
 f=tmpfile();fputs(UI_CONFIG_PREFIX "2\n0 0 0 0 0 0 0 0 0 0\n0 0 6100 - -\n",f);rewind(f);assert(!ui_read(f,&b));fclose(f);
 f=tmpfile();fputs(UI_CONFIG_PREFIX "2\n0 0 0 0 0 0 0 0 0 0\n0 0 61 - - trailing\n",f);rewind(f);assert(!ui_read(f,&b));fclose(f);
 puts("PASS v1 migration/v2 round-trip, library limits, titles, artwork links, search/filter/order, history overflow, hostile paths, malformed preferences");
}
