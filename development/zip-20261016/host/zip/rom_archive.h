/* GPL-3.0-or-later. Read one cartridge from a plain file or ZIP into caller RAM.
 * No archive member is ever written to a filesystem or interpreted as a path.
 * ZIP32, single disk, stored/deflate. ROM limits apply before allocation/inflate.
 * puff retains Mark Adler's licence in puff.h. */
#ifndef EM_ROM_ARCHIVE_H
#define EM_ROM_ARCHIVE_H
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <limits.h>
#include "puff.h"

enum em_rom_error { EM_ROM_OK, EM_ROM_IO, EM_ROM_MEMORY, EM_ROM_LARGE,
 EM_ROM_BAD_ZIP, EM_ROM_ZIP_FEATURE, EM_ROM_NONE, EM_ROM_MULTIPLE,
 EM_ROM_DEFLATE, EM_ROM_CRC };
struct em_zip_member { uint32_t offset, packed, size, crc, directory;
 unsigned method, flags; char name[512]; };
static const char *em_rom_message(int e) {
 static const char *const text[]={"", "Cannot read cartridge file.",
  "Not enough memory to load cartridge.", "Cartridge exceeds this player's size limit.",
  "Damaged ZIP archive.", "Unsupported ZIP format, encryption, compression or long filename.",
  "ZIP contains no cartridge for this emulator.",
  "ZIP contains several cartridges. Extract the game you want first.",
  "Cannot decompress ZIP cartridge.", "ZIP cartridge checksum failed."};
 return e>=0&&e<10?text[e]:"Cannot load cartridge.";
}
static unsigned em_zip_u16(const unsigned char *p){return p[0]|(unsigned)p[1]<<8;}
static uint32_t em_zip_u32(const unsigned char *p){return em_zip_u16(p)|(uint32_t)em_zip_u16(p+2)<<16;}
static int em_rom_zip(const char *p){size_t n=strlen(p);return n>4&&!strcasecmp(p+n-4,".zip");}
static int em_rom_extension(const char *p,int snes){size_t n=strlen(p);
 return n>4&&(snes?(!strcasecmp(p+n-4,".sfc")||!strcasecmp(p+n-4,".smc")):!strcasecmp(p+n-4,".nes"));}
static int em_zip_read(FILE *f,uint32_t at,void *p,size_t n){return !fseek(f,(long)at,SEEK_SET)&&fread(p,1,n,f)==n;}
static uint32_t em_zip_crc(const unsigned char *p,unsigned n){
 static const uint32_t table[16]={0,0x1db71064,0x3b6e20c8,0x26d930ac,0x76dc4190,0x6b6b51f4,0x4db26158,0x5005713c,
  0xedb88320,0xf00f9344,0xd6d6a3e8,0xcb61b38c,0x9b64c2b0,0x86d3d2d4,0xa00ae278,0xbdbdf21c};
 uint32_t c=~0u;while(n--){c^=*p++;c=(c>>4)^table[c&15];c=(c>>4)^table[c&15];}return ~c;
}
static int em_zip_find(FILE *f,unsigned limit,int snes,struct em_zip_member *member){
 long length;unsigned char *tail,h[46];unsigned count,found=0;uint32_t bytes,start,end,at;int result=EM_ROM_BAD_ZIP;
 if(fseek(f,0,SEEK_END)||(length=ftell(f))<22||length>INT32_MAX)return EM_ROM_BAD_ZIP;
 bytes=(uint32_t)length<65557?(uint32_t)length:65557;start=(uint32_t)length-bytes;
 tail=malloc(bytes);if(!tail)return EM_ROM_MEMORY;
 if(!em_zip_read(f,start,tail,bytes)){free(tail);return EM_ROM_IO;}
 at=bytes-22;
 for(;;){if(em_zip_u32(tail+at)==0x06054b50u&&em_zip_u16(tail+at+20)==bytes-at-22)break;
  if(!at){free(tail);return EM_ROM_BAD_ZIP;}--at;}
 const unsigned char *e=tail+at;end=start+at;count=em_zip_u16(e+10);
 uint32_t size=em_zip_u32(e+12),offset=em_zip_u32(e+16);
 if(em_zip_u16(e+4)||em_zip_u16(e+6)||count!=em_zip_u16(e+8)||count==65535||size==UINT32_MAX||offset==UINT32_MAX){free(tail);return EM_ROM_ZIP_FEATURE;}
 free(tail);
 if(offset>end||size!=end-offset)return EM_ROM_BAD_ZIP;
 at=offset;
 for(unsigned i=0;i<count;i++){
  if(at>end||end-at<46||!em_zip_read(f,at,h,46)||em_zip_u32(h)!=0x02014b50u)return EM_ROM_BAD_ZIP;
  unsigned names=em_zip_u16(h+28),extra=em_zip_u16(h+30),comment=em_zip_u16(h+32);
  uint32_t step=46u+names+extra+comment;if(step>end-at)return EM_ROM_BAD_ZIP;
  if(names>=sizeof(member->name))return EM_ROM_ZIP_FEATURE;
  if(names&&names<sizeof(member->name)){
   char name[512];if(fread(name,1,names,f)!=names)return EM_ROM_IO;name[names]=0;
   if(memchr(name,0,names))return EM_ROM_BAD_ZIP;
   const char *base=strrchr(name,'/');base=base?base+1:name;
   if(strncmp(name,"__MACOSX/",9)&&strncmp(base,"._",2)&&em_rom_extension(name,snes)){
    if(++found>1)return EM_ROM_MULTIPLE;
    memcpy(member->name,name,names+1);member->flags=em_zip_u16(h+8);member->method=em_zip_u16(h+10);
    member->crc=em_zip_u32(h+16);member->packed=em_zip_u32(h+20);member->size=em_zip_u32(h+24);
    member->offset=em_zip_u32(h+42);member->directory=offset;
    result=EM_ROM_OK;
    if(em_zip_u16(h+34)||member->packed==UINT32_MAX||member->size==UINT32_MAX||member->offset==UINT32_MAX||
       (member->flags&~(2u|4u|8u|2048u))||(member->method!=0&&member->method!=8))result=EM_ROM_ZIP_FEATURE;
    else if(!member->size||member->size>limit||member->packed>limit+(uint64_t)limit/8+65536)result=EM_ROM_LARGE;
   }
  }
  at+=step;
 }
 if(at!=end)return EM_ROM_BAD_ZIP;
 return found?result:EM_ROM_NONE;
}
/* Directory-only probe for drive scanning; no decompression or output writes. */
static int em_rom_probe(const char *path,unsigned limit,int snes){
 struct em_zip_member member;FILE*f=fopen(path,"rb");int e;if(!f)return EM_ROM_IO;
 e=em_zip_find(f,limit,snes,&member);fclose(f);return e;
}
static int em_rom_load(const char *path,unsigned char *out,unsigned limit,unsigned *used,int snes,char *name,unsigned name_cap){
 FILE *f=fopen(path,"rb");int e=EM_ROM_IO;unsigned char *packed=NULL;struct em_zip_member m;
 *used=0;if(name&&name_cap)snprintf(name,name_cap,"%s",path);
 if(!f)return e;
 if(!em_rom_zip(path)){
  size_t n=fread(out,1,limit,f);int next=fgetc(f);
  e=ferror(f)?EM_ROM_IO:next!=EOF?EM_ROM_LARGE:EM_ROM_OK;
  if(!e)*used=(unsigned)n;
 }else{
  unsigned char h[30];uint32_t data;unsigned long in,outsize;
  e=em_zip_find(f,limit,snes,&m);if(e)goto done;
  if(m.offset>m.directory||m.directory-m.offset<30||!em_zip_read(f,m.offset,h,30)||em_zip_u32(h)!=0x04034b50u||
     em_zip_u16(h+6)!=m.flags||em_zip_u16(h+8)!=m.method){e=EM_ROM_BAD_ZIP;goto done;}
  uint32_t skip=30u+em_zip_u16(h+26)+em_zip_u16(h+28);
  if(skip>m.directory-m.offset){e=EM_ROM_BAD_ZIP;goto done;}data=m.offset+skip;
  if(m.packed>m.directory-data||em_zip_u16(h+26)!=strlen(m.name)){e=EM_ROM_BAD_ZIP;goto done;}
  char local[512];if(fread(local,1,strlen(m.name),f)!=strlen(m.name)||memcmp(local,m.name,strlen(m.name))){e=EM_ROM_BAD_ZIP;goto done;}
  if(!(m.flags&8)&&(em_zip_u32(h+14)!=m.crc||em_zip_u32(h+18)!=m.packed||em_zip_u32(h+22)!=m.size)){e=EM_ROM_BAD_ZIP;goto done;}
  if(m.method==0){
   if(m.packed!=m.size){e=EM_ROM_BAD_ZIP;goto done;}
   if(!em_zip_read(f,data,out,m.size)){e=EM_ROM_IO;goto done;}
  }else{
   packed=malloc(m.packed?m.packed:1);if(!packed){e=EM_ROM_MEMORY;goto done;}
   if(!em_zip_read(f,data,packed,m.packed)){e=EM_ROM_IO;goto done;}
   in=m.packed;outsize=m.size;
   if(puff(out,&outsize,packed,&in)||in!=m.packed||outsize!=m.size){e=EM_ROM_DEFLATE;goto done;}
  }
  if(em_zip_crc(out,m.size)!=m.crc){e=EM_ROM_CRC;goto done;}
  if(name&&name_cap)snprintf(name,name_cap,"%s",m.name);*used=m.size;e=EM_ROM_OK;
 }
done:free(packed);fclose(f);return e;
}
#endif
