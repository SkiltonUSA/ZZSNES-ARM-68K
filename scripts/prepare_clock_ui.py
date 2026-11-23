"""Add the optional board-wide clock Settings page to the generated console UI."""
from pathlib import Path
import shutil

def prepare(lib):
    root=Path(__file__).resolve().parents[1]
    p=lib/'library_mui.h';s=p.read_text()
    if '#include "zzclock/mui.h"' in s:return
    shutil.copytree(root/'amiga/common/zzclock',lib/'zzclock',dirs_exist_ok=True)
    def change(a,b):
        nonlocal s
        assert s.count(a)==1,(a[:80],s.count(a))
        s=s.replace(a,b,1)
    change('#define LIB_PAGE_COUNT 7','#define LIB_PAGE_COUNT 8')
    change('static Object *root_group,','#include "zzclock/mui.h"\nstatic Object *root_group,')
    change('tab_buttons[7]','tab_buttons[8]')
    change('"ROM Manager","About",NULL','"ROM Manager","About","Settings",NULL')
    change('Child,tab_buttons[6]=ui_about(),','Child,tab_buttons[7]=ui_tab("Settings"),Child,tab_buttons[6]=ui_about(),')
    change('for(unsigned i=0;i<7;i++)notify(tab_buttons[i]', 'zzclock_wire(app);\n for(unsigned i=0;i<8;i++)notify(tab_buttons[i]')
    change('id<707','id<708')
    change('  activity_action(id);','  {const char *msg=zzclock_event(id,child_running||manager_mode||scan_active);if(msg){message(msg);id=0;}}\n  activity_action(id);')
    change('  End,\n  Child,details_panel=VGroup','   Child,VGroup,Child,zzclock_panel(),Child,VSpace(0),End,\n  End,\n  Child,details_panel=VGroup')
    change(' message(tabs[page]);',' set(tab_buttons[7],MUIA_Background,page==7?(ULONG)"2:r77777777,99999999,bbbbbbbb":(ULONG)MUII_ButtonBack);\n message(tabs[page]);')
    p.write_text(s)
    p=lib/'library_manager.h';s=p.read_text().replace('i<7;i++)set(tab_buttons','i<8;i++)set(tab_buttons');p.write_text(s)
