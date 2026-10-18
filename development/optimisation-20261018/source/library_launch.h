/* GPL-3.0-or-later. Asynchronous child lifetime with explicit ready/exit files.
 * Ready is published after a complete frame, not merely after opening a window. */
static Object*library_logo(unsigned);
static Object*loading_window;
static int child_running;
static unsigned startup_ticks,child_poll_ticks;
static Object*splash_create(int loading){Object*w;
 w=WindowObject,MUIA_Window_Title,(ULONG)LIB_TITLE,MUIA_Window_Borderless,TRUE,
 MUIA_Window_CloseGadget,FALSE,MUIA_Window_SizeGadget,FALSE,MUIA_Window_DepthGadget,FALSE,MUIA_Window_DragBar,FALSE,
 MUIA_Window_LeftEdge,MUIV_Window_LeftEdge_Centered,MUIA_Window_TopEdge,MUIV_Window_TopEdge_Centered,
 WindowContents,HGroup,GroupFrame,InnerSpacing(24,18),MUIA_Group_Spacing,20,
  Child,library_logo(128),Child,VGroup,MUIA_Group_Spacing,10,
   Child,TextObject,MUIA_Font,MUIV_Font_Title,MUIA_Text_Contents,(ULONG)(loading?"LOADING":LIB_TITLE),End,
   Child,textline(LIB_VERSION " / AmigaOS / ZZ9000"),
   Child,textline(loading?"Alt + Return   Fullscreen / window\nF10 or Esc      Return to library\nP                   Pause\nF5 / F8          Save / load state\nF9                 Screenshot":"Cortex-A9 emulation\nNative Amiga interface"),
  End,End,End;
 if(w){DoMethod(app,OM_ADDMEMBER,(ULONG)w);set(w,MUIA_Window_Open,TRUE);}return w;
}
static void splash_close(void){if(loading_window){set(loading_window,MUIA_Window_Open,FALSE);DoMethod(app,OM_REMMEMBER,(ULONG)loading_window);MUI_DisposeObject(loading_window);loading_window=NULL;}}
static void launch_poll(void){BPTR lock;FILE*f;int rc;
 if(startup_ticks){if(!--startup_ticks){splash_close();set(win,MUIA_Window_Open,TRUE);}return;}
 if(!child_running)return;
 /* Once visible, poll completion at 2 Hz rather than doing DOS lookups per tick. */
 if(!loading_window&&++child_poll_ticks<10)return;child_poll_ticks=0;
 lock=Lock("PROGDIR:Player.ready",ACCESS_READ);if(lock){UnLock(lock);splash_close();DeleteFile("PROGDIR:Player.ready");}
 f=fopen("PROGDIR:Player.exit","r");if(!f)return;rc=20;if(fscanf(f,"%d",&rc)!=1){fclose(f);return;}fclose(f);
 child_running=0;splash_close();DeleteFile("PROGDIR:Player.exit");set(win,MUIA_Window_Sleep,FALSE);set(win,MUIA_Window_Open,TRUE);
 previous_joy=~0u;refresh();message(rc==0?"":"Player stopped with an error. See Last-session.log.");
}
static int launch_child(const char*command){char script[UI_PATH],exitfile[UI_PATH],quoted[UI_PATH*2+4],qexit[UI_PATH*2+4],cmd[UI_PATH*2+32];FILE*f;BPTR out;
 strcpy(script,appdir);strcpy(exitfile,appdir);
 if(!AddPart(script,"Run-player",sizeof(script))||!AddPart(exitfile,"Player.exit",sizeof(exitfile))||!ui_quote(quoted,sizeof(quoted),script)||!ui_quote(qexit,sizeof(qexit),exitfile))return 0;
 f=fopen("PROGDIR:Run-player","w");if(!f)return 0;
 fprintf(f,"FailAt 21\n%s\nEcho $RC >%s\n",command,qexit);
 if(fclose(f))return 0;DeleteFile("PROGDIR:Player.ready");DeleteFile("PROGDIR:Player.exit");
 snprintf(cmd,sizeof(cmd),"Execute %s",quoted);out=Open("PROGDIR:Last-session.log",MODE_NEWFILE);if(!out)return 0;
 loading_window=splash_create(1);set(win,MUIA_Window_Open,FALSE);
 if(SystemTags(cmd,SYS_Input,0,SYS_Output,out,SYS_Asynch,TRUE,NP_StackSize,65536,TAG_DONE)==-1){Close(out);splash_close();set(win,MUIA_Window_Open,TRUE);return 0;}
 /* Async SystemTags owns output; do not close a handle still in use by DOS. */
 child_running=1;child_poll_ticks=0;return 1;
}
