/* GPL-3.0-or-later. Included after request(); no graphics calls under lock. */
#include <devices/ahi.h>
#include <libraries/asl.h>
#include <proto/asl.h>
#include "cartridge.h"
struct Library *AslBase;
static char chosen_rom[1024],save_path[1040];
static UBYTE *save_data;
static ULONG save_size,save_hash,audio_samples,audio_gaps;
static ULONG audio_wait_us,audio_copy_us,audio_send_us;
static ULONG bytes_hash(const UBYTE *p,ULONG n){ULONG h=2166136261u;while(n--)h=(h^*p++)*16777619u;return h;}
static int player_choose(void){
 struct FileRequester *r;int ok=0;
 if(rom_path||player_demo)return 1;
 AslBase=OpenLibrary("asl.library",38);if(!AslBase)return 0;
 r=AllocAslRequestTags(ASL_FileRequest,ASLFR_TitleText,(ULONG)"ZZNes - select an NTSC .nes cartridge",ASLFR_InitialPattern,(ULONG)"#?.nes",ASLFR_DoPatterns,TRUE,TAG_DONE);
 if(r){if(AslRequestTags(r,TAG_DONE)){
   if(strlen((const char *)r->fr_Drawer)<sizeof(chosen_rom)){strcpy(chosen_rom,(const char *)r->fr_Drawer);if(AddPart(chosen_rom,r->fr_File,sizeof(chosen_rom))){rom_path=chosen_rom;ok=1;}}
  }else quit=1;FreeAslRequest(r);}
 CloseLibrary(AslBase);AslBase=NULL;return ok;
}
#include "player_audio.h"
static int player_load_save(void){
 FILE *f;ULONG n;save_size=ad_get(shared+MB_REPLY,44);if(!save_size||!rom_path)return 1;
 if(save_size>NES_SAVE_LIMIT||strlen(rom_path)>1023)return 0;
 strcpy(save_path,rom_path);strcat(save_path,".sav");save_data=AllocVec(save_size+1,MEMF_PUBLIC|MEMF_CLEAR);if(!save_data)return 0;
 f=fopen(save_path,"rb");if(f){n=fread(save_data,1,save_size+1,f);if(ferror(f)||n!=save_size){fclose(f);puts("Save file has wrong size/read error; refusing to overwrite it");return 0;}fclose(f);}
 else if(errno!=ENOENT){puts("Cannot read saved RAM; refusing to overwrite it");return 0;}
 save_hash=bytes_hash(save_data,save_size);memcpy((void *)(shared+NES_SAVE),save_data,save_size);syncio->push(shared+NES_SAVE,save_size,NULL);
 return request(NES_SAVE_LOAD,save_size);
}
static int player_store_save(void){
 FILE *f;ULONG hash;char temp[1050],backup[1050];int had_old=0,ok;
 if(!save_data||!save_size)return 1;
 /* Caller drains flip, waits for acknowledged frame and clears quit only for this bounded transaction. */
 if(!request(NES_SAVE_STORE,save_size))return 0;
 syncio->pull(shared+NES_SAVE,save_size,NULL);CopyMem((APTR)(shared+NES_SAVE),save_data,save_size);
 hash=bytes_hash(save_data,save_size);if(hash==save_hash)return 1;
 strcpy(temp,save_path);strcat(temp,".tmp");strcpy(backup,save_path);strcat(backup,".bak");
 f=fopen(temp,"wb");if(!f)return 0;ok=fwrite(save_data,1,save_size,f)==save_size;if(fclose(f))ok=0;if(!ok)return 0;
 /* Verify before replacing the last save; retain a recoverable previous file. */
 f=fopen(temp,"rb");if(!f)return 0;{ULONG n=fread(save_data,1,save_size+1,f);ok=n==save_size&&!ferror(f)&&bytes_hash(save_data,n)==hash;}fclose(f);if(!ok)return 0;
 {BPTR lock=Lock(save_path,ACCESS_READ);if(lock){UnLock(lock);had_old=1;}}
 if(had_old){DeleteFile(backup);if(!Rename(save_path,backup))return 0;}
 if(!Rename(temp,save_path)){if(had_old)Rename(backup,save_path);return 0;}
 printf("SAVE wrote %u bytes to %s; hash=%08x\n",save_size,save_path,hash);save_hash=hash;return 1;
}
