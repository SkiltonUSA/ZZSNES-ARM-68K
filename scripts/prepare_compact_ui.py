"""Compact MUI library navigation and activity history; no player/core changes."""
from pathlib import Path
import shutil

ROOT = Path(__file__).resolve().parents[1]

def once(text, before, after):
    if text.count(before) != 1:
        raise ValueError('Unexpected launcher source: ' + before[:100])
    return text.replace(before, after, 1)

def prepare(lib):
    lib = Path(lib)
    source = ROOT / 'development/compact-ui-20261030'
    for name in ('library_activity.h', 'library_style.h'):
        shutil.copy2(source / name, lib / name)
    p = lib / 'library_mui.h'
    s = p.read_text()
    snes = 'static Object *core_skip' in s
    s = once(s, '"Save states","Disks","ROM Manager"', '"Save states","Sound","ROM Manager"')
    s = once(s, 'static void message(const char *s){snprintf(status,sizeof(status),"%s",s);set(status_text,MUIA_Text_Contents,(ULONG)status);}', '#include "library_activity.h"')
    s = s.replace('static char status[240],joy[400];', 'static char joy[400];')
    s = s.replace('*latest,*fav,*remove_button', '*latest,*remove_button')
    s = once(s, 'static const char *filters[]={"All ROMs","Recently played","Favourites",NULL};\n', '')
    s = once(s, 'selected=n>=0&&n<visible_count?visible[n]:-1;', 'selected=n>=0&&n<visible_count?visible[n]:-1;activity_selection();')
    s = s.replace('*listview,*all,*recent,*favourites,*saves;', '*listview,*saves;')
    s = s.replace('*add,*scan_browse,*save,', '*add,*save,')
    s = once(s, '    Child,HGroup,MUIA_ShowMe,!ui_compact,Child,HSpace(0),Child,library_logo(80),Child,HSpace(0),End,\n', '')
    s = once(s, '    Child,HGroup,Child,all=ui_button("All games"),Child,recent=ui_button("Recently played"),Child,favourites=ui_button("Favourites"),End,\n', '')
    search_line = next(line for line in s.splitlines(True) if 'Child,search=StringObject' in line)
    s = s.replace(search_line, '')
    count_line = '    Child,count_text=textline("Your game library"),\n'
    s = once(s, count_line, '')
    s = once(s, '     Child,VGroup,MUIA_Weight,45,\n', '     Child,VGroup,GroupFrameT("Games"),MUIA_Weight,50,\n' + search_line + count_line)
    s = once(s, '      Child,HGroup,Child,add=ui_button("Add..."),End,\n      Child,HGroup,Child,fav=ui_button("Favourite on/off"),Child,remove_button=ui_button("Delete..."),End,', '      Child,HGroup,Child,add=ui_button("Add..."),Child,remove_button=ui_button("Delete..."),Child,HSpace(0),End,')
    s = s.replace('set(fav,MUIA_Disabled,selected<0);', '')
    s = s.replace('notify(fav,MUIA_Pressed,FALSE,ID_FAV);', '')
    s = s.replace('notify(scan_browse,MUIA_Pressed,FALSE,ID_SCAN);', '')
    s = once(s, ' notify(all,MUIA_Pressed,FALSE,ID_ALL);notify(recent,MUIA_Pressed,FALSE,ID_RECENT);notify(favourites,MUIA_Pressed,FALSE,ID_FAVOURITES);\n', '')
    s = once(s, ' notify(scan_drives,MUIA_Pressed,FALSE,ID_SCAN_DRIVES);\n', '')
    # The manager owns the existing scan engine and cancellation path.
    begin = s.index('   Child,VGroup,UI_PAGE,\n    Child,VGroup,UI_PANEL("Local ROM scan")')
    end = s.index('   Child,VGroup,UI_PAGE,\n    Child,manager_controls', begin)
    audio_line = next(line for line in s.splitlines(True) if 'Child,audio_output=cycle' in line)
    mute_line = next(line for line in s.splitlines(True) if 'Child,mute=CheckMark' in line)
    s = s[:begin] + '   Child,VGroup,UI_PAGE,\n    Child,VGroup,UI_PANEL("Audio output"),\n' + audio_line + mute_line.replace(',Child,VSpace(0),End,', ',End,') + '\n    Child,VSpace(0),End,\n' + s[end:]
    s = once(s, audio_line + mute_line, '    Child,VSpace(0),End,\n')
    if snes:
        low = next(line for line in s.splitlines(True) if 'Child,core_filter=CheckMark' in line)
        strength = next(line for line in s.splitlines(True) if 'Child,core_strength=SliderObject' in line)
        s = s.replace(low, '').replace(strength, '')
        audio_end = mute_line.replace(',Child,VSpace(0),End,', ',End,')
        s = once(s, audio_end, audio_end + '\n    Child,ColGroup(2),UI_PANEL("Filter"),\n' + low + strength + '    End,')
        profile = '    Child,profile_label=textline("Settings: global defaults"),\n    Child,HGroup,Child,profile_save=ui_button("Save for game"),Child,profile_reset=ui_button("Use defaults"),Child,profile_global=ui_button("Edit defaults"),End,\n'
        s = once(s, profile, '')
        s = once(s, '    Child,textline("Save for game keeps these settings with the selected ROM."),\n', profile)
        s = s.replace('"USB: SN30 Pro","Mega Drive 6","Amiga / CD32","USB / CD32 (Poseidon)"', '"SN30 Pro USB","Sega Mega Drive 6","Amiga / CD32 gameport","CD32 USB (Poseidon)"')
    else:
        # Existing NES player consumes lowlevel.library's CD32-compatible mapping.
        # These describe the supported OS/adaptor routes, not new direct USB drivers.
        s = once(s, 'static Object *audio_output;', 'static Object *audio_output;\n#include "library_input_route.h"')
        s = once(s, '    Child,controller=controller_new(),', '    Child,HGroup,Child,textline("Controller"),Child,input_route=cycle(input_routes),End,\n    Child,controller=controller_new(),')
        s = once(s, 'if(!em_audio_save(value(audio_output,MUIA_Cycle_Active)))return 0;', 'if(!input_route_save(value(input_route,MUIA_Cycle_Active)))return 0;if(!em_audio_save(value(audio_output,MUIA_Cycle_Active)))return 0;')
        s = once(s, ' set(audio_output,MUIA_Cycle_Active,em_audio_load());', ' set(input_route,MUIA_Cycle_Active,input_route_load());\n set(input_route,MUIA_ShortHelp,(ULONG)"Uses AmigaOS port 2 mappings for all routes.\\nCD32 USB: Poseidon HID joystick mapping.\\nSega: compatible gameport/CD32 adapter; no direct six-button protocol.\\nButton mapping below applies to the selected controller.");\n set(audio_output,MUIA_Cycle_Active,em_audio_load());')
        shutil.copy2(source/'library_input_route.h', lib/'library_input_route.h')
    s = once(s, ' if(!app||!ui_scroll_pages(pages))return 0;', ' if(!app)return 0;scan_drives=manager_import;scan_path=manager_import_path;\n if(!ui_scroll_pages(pages))return 0;')
    s = s.replace('(active?"Stop scan":"Scan once")', '(active?"Importing...":"Import ROMs")')
    s = s.replace('ui_button("Import ROM folder")', 'ui_button("Import ROMs")')
    s = s.replace('*resume,*latest,*remove_button', '*resume,*remove_button')
    s = s.replace('*listview,*saves;', '*listview;')
    s = s.replace('set(latest,MUIA_Disabled,latest_slot<0);', '').replace('set(latest,MUIA_Disabled,TRUE);', '')
    s = once(s, ' notify(latest,MUIA_Pressed,FALSE,ID_LATEST);notify(saves,MUIA_Pressed,FALSE,ID_SAVES);', '')
    s = once(s, '      Child,latest=ui_button("Continue latest save"),Child,saves=ui_button("Choose save state..."),',
     '      Child,ListviewObject,GroupFrameT("Current settings"),MUIA_Listview_Input,FALSE,MUIA_FixHeight,ui_compact?72:120,\n       MUIA_ShortHelp,(ULONG)"Selected launch settings; Auto uses the configured output. This is not hardware detection.",\n       MUIA_Listview_List,settings_list=ListObject,ReadListFrame,MUIA_Font,ui_font(),MUIA_List_ConstructHook,MUIV_List_ConstructHook_String,MUIA_List_DestructHook,MUIV_List_DestructHook_String,End,End,')
    s = s.replace('\\nContinue uses the newest readable save by the Amiga file date.', '')

    s = s.replace('ui_compact?108:148', '108').replace('ui_compact?21:30', '18')
    s = s.replace('MUIA_Group_Horiz,!ui_compact', 'MUIA_Group_Horiz,TRUE')
    # New layout identity starts at its wider default; subsequent resizing is remembered.
    s = s.replace('ui_compact?0x534e554a:0x534e5549', 'ui_compact?0x534e5531:0x534e5530')
    s = s.replace('ui_compact?0x4e455356:0x4e455355', 'ui_compact?0x4e455531:0x4e455530')
    s = once(s, 'MUIA_FixWidth,108,MUIA_Listview_List,navigation=', 'MUIA_FixWidth,108,MUIA_Listview_ScrollerPos,MUIV_Listview_ScrollerPos_None,MUIA_Listview_List,navigation=')
    s = s.replace('F9 in game: capture library picture.', 'F9: capture picture')
    s = s.replace('incomplete?"ROM deleted; some save files could not be removed.":""', 'incomplete?"ROM deleted; some save files could not be removed.":"Game files deleted."')
    s = s.replace('"Continue latest save"', '"Continue"').replace('"Choose save state..."', '"Save states..."')
    s = s.replace('"Add games, or clear the search/filter."', '"Select a game."')
    s = once(s, 'visible_count=ui_visible(&model,model.filter,q,visible);', 'visible_count=ui_visible(&model,0,q,visible);')
    s = once(s, '"%s - %d / %u games",filters[model.filter],visible_count,model.count', '"%d / %u games",visible_count,model.count')
    s = once(s, '  Child,status_text=TextObject,MUIA_Font,ui_font(),TextFrame,MUIA_Text_SetMin,FALSE,MUIA_FixHeightTxt,(ULONG)"Ag",MUIA_Text_Contents,(ULONG)"",End,', '  Child,ListviewObject,GroupFrameT("Activity"),MUIA_Listview_Input,FALSE,MUIA_FixHeight,ui_compact?52:70,\n   MUIA_Listview_List,status_text=ListObject,ReadListFrame,MUIA_Font,MUIV_Font_Fixed,\n   MUIA_List_ConstructHook,MUIV_List_ConstructHook_String,MUIA_List_DestructHook,MUIV_List_DestructHook_String,End,End,')
    s = once(s, 'if(!build())goto done;temperature_init();refresh();', 'if(!build())goto done;ui_log_ready=1;settings_refresh();message(LIB_TITLE " " LIB_VERSION " ready");temperature_init();refresh();')
    s = once(s, '  switch(id){case', '  activity_action(id);\n  switch(id){case')
    s = once(s, 'set(heading,MUIA_Text_Contents,(ULONG)tabs[n]);}}break;', 'set(heading,MUIA_Text_Contents,(ULONG)tabs[n]);message(tabs[n]);}}break;')
    s = once(s, ' snprintf(search_seen,sizeof(search_seen),"%s",q);search_ticks=0;', ' activity_search(q);\n snprintf(search_seen,sizeof(search_seen),"%s",q);search_ticks=0;')
    s = once(s, 'else if(id>=ID_SLOT_DELETE&&id<ID_SLOT_DELETE+5)delete_slot(id-ID_SLOT_DELETE);break;', 'else if(id>=ID_SLOT_DELETE&&id<ID_SLOT_DELETE+5)delete_slot(id-ID_SLOT_DELETE);\n    else activity_setting(id);break;')
    # Register after settings initialization, so opening the app is not logged as edits.
    watches = [('display_mode','Display','display_choices'),('scaling_mode','Scaling','scaling_choices'),('scanline_mode','Scanlines','scanline_choices'),('monitor','Monitor','monitors'),('theme','Border','themes'),('mask','Side mask','masks'),('vmask','Top/bottom mask','masks'),('audio_output','Audio output','audio_choices'),('slot','Save slot','slots')]
    watches += [('pad_source','Controller','pad_choices')] if snes else [('input_route','Controller','input_routes'),('fire','Fire button','fires')]
    if snes:watches += [('core_skip','Frameskip','skip_choices'),('core_clock','CPU','clock_choices'),('core_detail','Resolution','detail_choices'),('second_player','Player 2','second_choices')]
    priority={'display_mode':0,'pad_source':1,'input_route':1,'audio_output':2,'scanline_mode':3}
    watches.sort(key=lambda item:priority.get(item[0],4))
    lines = ''.join(f' activity_watch({obj},"{label}",MUIA_Cycle_Active,{choices});\n' for obj,label,choices in watches)
    for obj,label in [('mute','Start muted'),('fps_option','FPS overlay'),('optimise_option','Optimised display')]+([('core_filter','Audio filter'),('core_transparency','Transparency'),('cheat_enabled','Cheats')] if snes else []):
        lines += f' activity_watch({obj},"{label}",MUIA_Selected,NULL);\n'
    if snes:
        lines += ' activity_watch(core_strength,"Filter strength",MUIA_Numeric_Value,NULL);\n activity_watch(core_threshold,"Frameskip threshold",MUIA_Numeric_Value,NULL);\n'
        lines += ' for(unsigned j=0;j<2;j++){for(unsigned k=0;k<12;k++)activity_watch(keymap[j][k],j?"P2 keyboard mapping":"P1 keyboard mapping",MUIA_Cycle_Active,ps_key_names);for(unsigned k=0;k<7;k++)activity_watch(joymap[j][k],j?"P2 button mapping":"P1 button mapping",MUIA_Cycle_Active,ps_button_names);}\n'
    anchor=' return 1;\n}\n\n#ifdef ZZ_UI_LAYOUT_PROBE'
    s = once(s, anchor, lines + anchor)
    p.write_text(s)
    p=lib/'library_launch.h';s=p.read_text()
    s=once(s, 'message(rc==0?"":"Player stopped with an error. See Last-session.log.");', 'message(rc==0?"Game closed; returned to library.":"Player stopped with an error. See Last-session.log.");')
    s=once(s, 'child_running=1;child_poll_ticks=0;return 1;', 'child_running=1;child_poll_ticks=0;message("Player started; waiting for first frame.");return 1;')
    s=once(s, 'if(lock){UnLock(lock);splash_close();', 'if(lock){UnLock(lock);message("Game ready.");splash_close();')
    p.write_text(s)

    from prepare_console_style import prepare as console_style
    console_style(lib)
