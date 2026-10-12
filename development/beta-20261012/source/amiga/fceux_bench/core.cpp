// GPL-3.0-or-later. Bounded NTSC mapper 0/90 harness for pinned FCEUX.
#include "platform.h"
#include "core.h"
readfunc ARead[65536];writefunc BWrite[65536];
static uint8 ram[2048],video[256*256],deemph[256*256],back[256*256];
uint8 *RAM=ram,*XBuf=video,*XDBuf=deemph,*XBackBuf=back;
uint8 PAL=0,mmc5ABMode=0;int dendy=0;
bool overclock_enabled=false,overclocking=false,skip_7bit_overclocking=false;
bool paldeemphswap=false,swapDuty=false;
int normalscanlines=240,totalscanlines=240,postrenderscanlines=0,vblankscanlines=0;
uint64 timestampbase=0;
FCEUS FSettings;
static FCEUGI info;FCEUGI *GameInfo=&info;
void (*GameStateRestore)(int)=nullptr;
int debug_loggingCD=0,FC_Error=0;unsigned char *cdloggerdata=nullptr;
volatile int datacount=0,undefinedcount=0;
static CartInfo cart;
static uint32 samples,nonzero;
static uint8 buttons,shift,strobe;
static int loaded;
void SetReadHandler(int32 first,int32 last,readfunc f){for(int i=first;i<=last;i++)ARead[i]=f;}
void SetWriteHandler(int32 first,int32 last,writefunc f){for(int i=first;i<=last;i++)BWrite[i]=f;}
readfunc GetReadHandler(int32 a){return ARead[a];}
writefunc GetWriteHandler(int32 a){return BWrite[a];}
static DECLFR(readnull){(void)A;return X.DB;}
static DECLFW(writenull){(void)A;(void)V;}
static DECLFR(readram){return RAM[A&2047];}
static DECLFW(writeram){RAM[A&2047]=V;}
static DECLFR(readpad){uint8 v=strobe?(buttons&1):(shift&1);if(!strobe)shift=(shift>>1)|128;return (X.DB&0xe0)|v;}
static DECLFR(readpad2){(void)A;return X.DB&0xe0;}
static DECLFW(writepad){(void)A;if((V&1)||strobe)shift=buttons;strobe=V&1;}
// Services excluded from this benchmark: debugger, cheats, save states,
// expansion controllers, NSF and movie/screenshot presentation.
#ifdef FC_STATES
#include "state_core.h"
#else
void AddExState(void *,uint32,int,const char *){}
void ResetExState(void (*)(void),void (*)(void)){}
#endif
void InputScanlineHook(uint8 *,uint8 *,uint32,int){}
void SetNESDeemph_OldHacky(uint8,int){} // Raw per-pixel emphasis remains XDBuf.
void FCEU_PutImage(){}
void FCEU_WriteWaveData(int32 *,int){}
void FCEU_printf(char *,...){}
void FCEU_PrintError(char *,...){FC_Error=1;}
void DoNSFFrame(){FC_Error=2;}
void MMC5_hb(int){FC_Error=3;}
int GetPRGAddress(int){FC_Error=4;return 0;}
int AllocGenieRW(){FC_Error=6;return 0;}
void FlushGenieRW(){FC_Error=7;}
void FCEU_dwmemset(void *p,uint32 v,int n){uint8 *d=(uint8 *)p;for(int i=0;i<n/4;i++){d[4*i]=v;d[4*i+1]=v>>8;d[4*i+2]=v>>16;d[4*i+3]=v>>24;}}
int fc_init(const uint8_t *rom,uint32_t size){
 if(loaded||size<16||memcmp(rom,"NES\032",4)||(rom[7]&15)||rom[8]||rom[9]||rom[10]||rom[11]||rom[12]||rom[13]||rom[14]||rom[15]||(rom[6]&14))return 1;
 unsigned id=(rom[6]>>4)|(rom[7]&240),prg=rom[4]*16384u,chr=rom[5]*8192u;
 if((id!=0&&id!=90)||!prg||!chr||size!=16+prg+chr||prg>524288||chr>262144||
    (prg&(prg-1))||(chr&(chr-1))||(id==0&&(prg>32768||chr!=8192)))return 1;
 loaded=1;info.type=GIT_CART;info.vidsys=GIV_NTSC;info.mappernum=id;
#ifdef FC_STATES
 state_rom_hash=state_hash(rom,size);state_rom_size=size;
#endif
 memset(&FSettings,0,sizeof(FSettings));FSettings.FirstSLine=0;FSettings.LastSLine=239;
 FSettings.SoundVolume=150;FSettings.TriangleVolume=FSettings.Square1Volume=FSettings.Square2Volume=FSettings.NoiseVolume=FSettings.PCMVolume=256;
 FSettings.SndRate=32000;FSettings.soundq=0;
 newppu=0;FCEUI_DisableSpriteLimitation(0); // Retain hardware's eight-sprite limit.
 X6502_Init();FCEUPPU_Init();FCEUPPU_SetVideoSystem(0);ResetCartMapping();
 SetupCartPRGMapping(0,(uint8 *)rom+16,prg,0);
 SetupCartCHRMapping(0,(uint8 *)rom+16+prg,chr,0);
 SetupCartMirroring(rom[6]&1,0,nullptr);
 if(id==90)Mapper90_Init(&cart);
 // Deterministic power-on RAM; not a claim of physical power-up randomness.
 for(unsigned i=0;i<2048;i++)RAM[i]=(i&1)?0xaa:0x55;
 SetReadHandler(0,65535,readnull);SetWriteHandler(0,65535,writenull);
 SetReadHandler(0,8191,readram);SetWriteHandler(0,8191,writeram);
 FCEUSND_Power();FCEUPPU_Power();
 SetReadHandler(0x4016,0x4016,readpad);SetReadHandler(0x4017,0x4017,readpad2);SetWriteHandler(0x4016,0x4016,writepad);
 if(id==90)cart.Power();else{setprg16(0x8000,0);setprg16(0xc000,prg/16384-1);setchr8(0);SetReadHandler(0x8000,0xffff,CartBR);}
 FCEUI_Sound(32000);timestampbase=0;X6502_Power();
 return FC_Error;
}
int fc_frame(uint8_t b){
 if(!loaded)return 1;buttons=b;
 FCEUPPU_Loop(0); // No presentation or sound skipping.
 int n=FlushEmulateSound();if(n<0||n>1024)return 8;samples=n;
 timestampbase+=timestamp;timestamp=soundtimestamp=0;
 nonzero=0;for(unsigned i=0;i<samples;i++)if(WaveFinal[i])nonzero++;
 return FC_Error;
}
uint32_t fc_video_hash(){uint32 h=2166136261u;for(unsigned i=0;i<61440;i++){h=(h^XBuf[i])*16777619u;h=(h^XDBuf[i])*16777619u;}return h;}
uint32_t fc_audio_hash(){uint32 h=2166136261u;for(unsigned i=0;i<samples;i++){uint16 v=(uint16)(int16)WaveFinal[i];h=(h^(v>>8))*16777619u;h=(h^(v&255))*16777619u;}return h;}
uint32_t fc_cycles(){return (uint32)timestampbase;}
uint32_t fc_samples(){return samples;}
uint32_t fc_nonzero(){return nonzero;}
const uint8_t *fc_pixels(){return XBuf;}
