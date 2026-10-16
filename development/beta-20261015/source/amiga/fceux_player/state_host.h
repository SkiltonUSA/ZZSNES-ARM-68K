/* GPL-3.0-or-later. Only called at acknowledged frame boundaries, unlocked.
 * A bad file does not terminate playback. Mailbox failure does: reset-before-free.
 * Transfers reuse the bounded 32 KiB battery area (supported ROMs have none). */
static int fc_state_transaction(int load){
 UBYTE *p;FILE *f;ULONG n,ok=0;char temp[UI_PATH+8],backup[UI_PATH+8];int had=0;
 if(!fc_slot_path[0]){puts("STATE choose a slot in ZZNes Library first");return 1;}
 if(direct_locked||!full_drain())return -1;
 audio_state_drained=0;audio_state_drain=1;player_audio_flush();
 if(audio_opened){ULONG start=us();
  while(!audio_state_drained){if(audio_error||audio_done||(ULONG)(us()-start)>1000000u){puts("STATE audio drain failed");return -1;}if(!pause_us(1000))return -1;}
 }
 audio_state_drain=0;
 p=AllocVec(NES_SAVE_LIMIT+1,MEMF_PUBLIC);if(!p)return 0;
 if(load){
  f=fopen(fc_slot_path,"rb");if(!f){puts("STATE slot empty or unreadable");goto end;}
  n=fread(p,1,NES_SAVE_LIMIT+1,f);ok=!ferror(f)&&n>=32&&n<=NES_SAVE_LIMIT;fclose(f);
  if(!ok){puts("STATE invalid file size");goto end;}
  memcpy((void *)(shared+NES_SAVE),p,n);syncio->push(shared+NES_SAVE,n,NULL);
  if(!request(10,n)){FreeVec(p);return -1;}
  ok=ad_get(shared+MB_REPLY,52);printf("STATE load %s path=%s\n",ok?"OK":"rejected (ROM/version/checksum)",fc_slot_path);
 }else{
  if(!request(11,0)){FreeVec(p);return -1;}n=ad_get(shared+MB_REPLY,52);
  if(n<32||n>NES_SAVE_LIMIT)goto end;
  syncio->pull(shared+NES_SAVE,n,NULL);CopyMem((APTR)(shared+NES_SAVE),p,n);
  snprintf(temp,sizeof(temp),"%s.tmp",fc_slot_path);snprintf(backup,sizeof(backup),"%s.bak",fc_slot_path);
  f=fopen(temp,"wb");if(!f)goto end;ok=fwrite(p,1,n,f)==n;if(fclose(f))ok=0;if(!ok)goto end;
  {ULONG h=bytes_hash(p,n),count;f=fopen(temp,"rb");if(!f){ok=0;goto end;}count=fread(p,1,NES_SAVE_LIMIT+1,f);ok=count==n&&!ferror(f)&&bytes_hash(p,n)==h;fclose(f);if(!ok)goto end;}
  {BPTR lock=Lock(fc_slot_path,ACCESS_READ);if(lock){had=1;UnLock(lock);}}
  if(had){DeleteFile(backup);if(!Rename(fc_slot_path,backup)){ok=0;goto end;}}
  if(!Rename(temp,fc_slot_path)){if(had)Rename(backup,fc_slot_path);ok=0;goto end;}
  printf("STATE save OK bytes=%u path=%s\n",n,fc_slot_path);
 }
end:
 if(!ok)puts("STATE operation not completed; previous file/state retained");
 fflush(stdout);FreeVec(p);next_frame_us=0;buttons=0;fc_joy_buttons=0;return ok?1:0;
}
