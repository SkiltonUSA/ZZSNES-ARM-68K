/* GPL-3.0-or-later. Frame-boundary states for this pinned old-PPU/LQ core.
 * Included in core.cpp: no pointers, mapping tables, OS or ARM state saved.
 * Host and ARM are little endian. Format is private, not desktop .fcs/.mst.
 * Validate the complete envelope before changing any emulated machine state. */
static uint32 state_rom_hash,state_rom_size;
static SFORMAT state_extra[64];
static unsigned state_extra_count;
static void (*state_pre)(),(*state_post)();
static uint32 state_hash(const uint8 *p,unsigned n){uint32 h=2166136261u;while(n--)h=(h^*p++)*16777619u;return h;}
static void state_put(uint8 *p,uint32 v){for(unsigned i=0;i<4;i++)p[i]=v>>(8*i);}
static uint32 state_get(const uint8 *p){return p[0]|uint32(p[1])<<8|uint32(p[2])<<16|uint32(p[3])<<24;}
void ResetExState(void (*a)(),void (*b)()){state_extra_count=0;memset(state_extra,0,sizeof(state_extra));state_pre=a;state_post=b;}
void AddExState(void *v,uint32 n,int type,const char *desc){
 if(state_extra_count>=63){FC_Error=10;return;}
 state_extra[state_extra_count++]={v,n|(type?FCEUSTATE_RLSB:0),desc};
}
extern SFORMAT FCEUPPU_STATEINFO[],FCEUSND_STATEINFO[],FC_PPU_EXTRA[],FC_SOUND_EXTRA[],FC_FILTER_EXTRA[];
static SFORMAT state_cpu[]={
 {&X.PC,2,"PC"},{&X.A,1,"A"},{&X.X,1,"X"},{&X.Y,1,"Y"},{&X.S,1,"S"},
 {&X.P,1,"P"},{&X.DB,1,"DB"},{&X.jammed,1,"JAM"},{&X.IRQlow,4,"IRQ"},
 {&X.tcount,4,"TC"},{&X.count,4,"CNT"},{&X.mooPI,1,"MPI"},
 {ram,sizeof(ram),"RAM"},{&timestampbase,8,"TSB"},{&timestamp,4,"TS"},
 {&soundtimestamp,4,"STS"},{&buttons,1,"PAD"},{&shift,1,"SH"},{&strobe,1,"STR"},{nullptr,0,nullptr}};
static SFORMAT *state_groups[]={state_cpu,FCEUPPU_STATEINFO,FCEUSND_STATEINFO,FC_PPU_EXTRA,FC_SOUND_EXTRA,FC_FILTER_EXTRA,state_extra};
static unsigned state_table(SFORMAT *s,uint8 *p,int mode,unsigned depth=0){
 unsigned total=0;if(depth>2)return 32769;
 for(;s->v;s++){
  if(s->s==~0u){unsigned n=state_table((SFORMAT *)s->v,p?p+total:nullptr,mode,depth+1);total+=n;}
  else {unsigned n=s->s&0x3fffffffu;void *v=(s->s&FCEUSTATE_INDIRECT)?*(void **)s->v:s->v;
   if(n>32768-total||!v)return 32769;
   if(p){if(mode)memcpy(v,p+total,n);else memcpy(p+total,v,n);}total+=n;}
  if(total>32768)return 32769;
 }return total;
}
static unsigned state_payload(uint8 *p,int mode){unsigned n=0;
 for(unsigned i=0;i<sizeof(state_groups)/sizeof(*state_groups);i++){
  unsigned k=state_table(state_groups[i],p?p+n:nullptr,mode);if(k>32768-n)return 32769;n+=k;
 }return n;
}
unsigned fc_state_store(uint8 *p,unsigned capacity){
 if(!loaded||!p)return 0;unsigned n=state_payload(nullptr,0);
 if(n>32736||capacity<n+32)return 0;
 FCEUPPU_SaveState();FCEUSND_SaveState();if(state_pre)state_pre();
 state_payload(p+32,0);if(state_post)state_post();
 memcpy(p,"ZZS1",4);state_put(p+4,1);state_put(p+8,state_rom_hash);state_put(p+12,state_rom_size);
 state_put(p+16,n);state_put(p+20,state_hash(p+32,n));state_put(p+24,0x20261011u);state_put(p+28,0);
 return n+32;
}
int fc_state_load(const uint8 *p,unsigned n){
 if(!loaded||!p||n<32||n>32768||memcmp(p,"ZZS1",4)||state_get(p+4)!=1||
 state_get(p+8)!=state_rom_hash||state_get(p+12)!=state_rom_size||state_get(p+16)!=n-32||
 state_get(p+20)!=state_hash(p+32,n-32)||state_get(p+24)!=0x20261011u||state_get(p+28)||
 state_payload(nullptr,0)!=n-32)return 0;
 state_payload((uint8 *)p+32,1);
 if(GameStateRestore)GameStateRestore(22020);
 FCEUPPU_LoadState(22020);FCEUSND_LoadState(22020);
 samples=nonzero=0;return 1;
}
