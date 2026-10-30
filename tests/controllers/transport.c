/* Execute the real transport against a small Poseidon/Exec model.
   This checks ownership and error paths, not USB or gameport electrical timing. */
#include <assert.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hid_fixture.h"
typedef void *APTR;
typedef unsigned char UBYTE;
typedef unsigned short UWORD;
typedef unsigned long ULONG;
typedef long LONG;
typedef long BPTR;
struct Library {int dummy;};
struct IORequest {int dummy;};
struct Node {char *ln_Name;};
struct MsgPort {struct Node mp_Node;};
struct Hook {void *links[2];ULONG (*entry)();ULONG (*subentry)();void *data;};
enum {TAG_END,TRUE,ACCESS_READ,DA_VendorID,DA_ProductID,DA_IsConnected,
 IFA_Class,EA_IsIn,EA_TransferType,DA_HasAppBinding,PGA_DEVICE,ABA_Device,
 ABA_ReleaseHook,ABA_Dummy=100,IFA_InterfaceNum,PGA_INTERFACE,PGA_PIPE,
 PPA_NakTimeout,PPA_NakTimeoutTime,PPA_IORequest};
static struct Library library;
static struct IORequest io;
static struct MsgPort *registered;
static struct Hook *release_hook;
static int stack=1,connected=1,busy,completed,io_error,descriptor_error,allocation_error;
static int allocations,frees,sends,waits,aborts,releases,rescans,claims,forbids;
static unsigned pot_available,pot_reserved,pot_writes,pot_values[16],interrupts_off;
static const unsigned phases[]={0,12,0,12,0,15,4,0,0};
static ULONG actual;
static UBYTE *destination;
static char device,interface,endpoint,binding;
static struct Library *OpenLibrary(const char*n,unsigned v){(void)n;(void)v;return stack?&library:NULL;}
static void CloseLibrary(struct Library*p){assert(p==&library);}
static void *OpenResource(const char*n){(void)n;return NULL;}
static struct MsgPort *CreateMsgPort(void){return calloc(1,sizeof(struct MsgPort));}
static void DeleteMsgPort(struct MsgPort*p){free(p);}
static void Forbid(void){forbids++;}
static void Permit(void){assert(forbids);forbids--;}
static struct MsgPort *FindPort(const char*n){(void)n;return registered;}
static void AddPort(struct MsgPort*p){assert(!registered);registered=p;}
static void RemPort(struct MsgPort*p){assert(registered==p);registered=NULL;}
static void Disable(void){assert(!interrupts_off);interrupts_off=1;}
static void Enable(void){assert(interrupts_off);interrupts_off=0;}
static unsigned AllocPotBits(unsigned mask){assert(mask==0xf000);return pot_reserved=pot_available;}
static void FreePotBits(unsigned mask){assert(mask==pot_reserved);pot_reserved=0;}
static void WritePotgo(unsigned data,unsigned mask){
 assert(pot_available&&pot_reserved==0xf000&&mask==0xf000);
 assert(data==0||(data&0xc000)==0xc000); /* C/Start sense pull-up, never low */
 assert(pot_writes<16);pot_values[pot_writes++]=data;
}
static unsigned sample(void){assert(pot_writes&&pot_writes<=9);return phases[pot_writes-1];}
static unsigned cia_read(void){return sample()&16?0x7f:0xff;}
static unsigned pot_read(void){return sample()&32?0:0x4000;}
static unsigned joy_read(void){
 unsigned n=sample(),r=(n>>3)&1,l=(n>>2)&1,d=(n>>1)&1,u=n&1;
 return (r<<1)|(r^d)|(l<<9)|((l^u)<<8);
}
#define PAD_CIA_READ() cia_read()
#define PAD_JOY_READ() joy_read()
#define PAD_POT_READ() pot_read()
static void DeleteFile(const char*p){remove(p);}
static BPTR Lock(const char*p,int mode){(void)mode;FILE*f=fopen(p,"r");if(!f)return 0;fclose(f);return 1;}
static void UnLock(BPTR l){(void)l;}
static int Rename(const char*a,const char*b){return rename(a,b)==0;}
static ULONG HookEntry(void){return 0;}
static void psdLockReadPBase(void){}
static void psdUnlockPBase(void){}
static APTR psdFindDevice(APTR p,...){(void)p;return connected?&device:NULL;}
static APTR psdFindInterface(APTR p,...){assert(p==&device);return &interface;}
static APTR psdFindEndpoint(APTR p,...){assert(p==&interface);return &endpoint;}
static void psdGetAttrs(int type,APTR object,int tag,...){
 (void)type;(void)object;va_list a;va_start(a,tag);
 if(tag==DA_HasAppBinding)*va_arg(a,ULONG*)=busy;
 else if(tag==IFA_InterfaceNum)*va_arg(a,ULONG*)=0;
 else if(tag==PPA_IORequest)*va_arg(a,struct IORequest**)=&io;
 else assert(0);va_end(a);
}
static APTR psdClaimAppBinding(int tag,...){
 assert(tag==ABA_Device);va_list a;va_start(a,tag);assert(va_arg(a,APTR)==&device);
 assert(va_arg(a,int)==ABA_ReleaseHook);release_hook=va_arg(a,struct Hook*);va_end(a);
 claims++;return &binding;
}
static void detached(void){assert(release_hook);((ULONG(*)(struct Hook*,APTR,APTR))release_hook->subentry)(release_hook,&binding,NULL);}
static void psdReleaseAppBinding(APTR p){assert(p==&binding);assert(forbids);releases++;detached();}
static APTR psdAllocPipe(APTR d,struct MsgPort*p,APTR ep){
 assert(d==&device&&p);(void)ep;if(allocation_error)return NULL;
 allocations++;return malloc(1);
}
static void psdFreePipe(APTR p){assert(p);frees++;free(p);}
static void psdAbortPipe(APTR p){assert(p);aborts++;}
static LONG psdWaitPipe(APTR p){assert(p);waits++;completed=0;return io_error;}
static void psdSetAttrs(int type,APTR p,...){(void)type;assert(p);}
static void psdPipeSetup(APTR p,int type,int request,int value,ULONG index){
 assert(p&&type==0x81&&request==6&&value==0x2200&&index==0);
}
static int psdDoPipe(APTR p,UBYTE *buf,unsigned size){
 assert(p&&size>=sizeof(descriptor));memcpy(buf,descriptor,sizeof(descriptor));
 actual=sizeof(descriptor);return descriptor_error;
}
static ULONG psdGetPipeActual(APTR p){assert(p);return actual;}
static void psdSendPipe(APTR p,UBYTE *buf,unsigned size){assert(p&&size==64);destination=buf;sends++;}
static int CheckIO(struct IORequest*p){assert(p==&io);return completed;}
static void psdClassScan(void){rescans++;}
#include "../../development/controllers-20261030/pad_amiga.h"
static void packet(unsigned char buttons,unsigned length){
 const UBYTE report[]={1,buttons,0,8,128,128};memcpy(destination,report,sizeof(report));
 actual=length;completed=1;
}
int main(void){
 unsigned mode=99;assert(pad_read_config(&mode)&&mode==3);
 for(unsigned i=0;i<5;i++){assert(pad_write_config(i));assert(pad_read_config(&mode)&&mode==i);}
 FILE*f=fopen("PROGDIR:Controller.cfg","w");assert(f);fputs("ZZPAD1 99\n",f);fclose(f);
 mode=2;assert(!pad_read_config(&mode)&&mode==2);DeleteFile("PROGDIR:Controller.cfg");
 /* Both lowlevel sources leave USB class bindings and gameport pins alone. */
 for(unsigned m=3;m<=4;m++){
  assert(pad_uses_lowlevel(m));assert(pad_open(m));
  assert(!PsdBase&&!pad_owner&&!pad_binding&&!pad_pot&&claims==0);
  pad_close();
 }
 assert(!pad_uses_lowlevel(0)&&!pad_uses_lowlevel(1)&&!pad_uses_lowlevel(2));
 assert(pad_open(1));assert(pad_usb_attach());assert(pad_binding&&pad_pending);
 packet(2,6);assert(pad_usb_poll()&&pad_bits==1); /* B */
 packet(0,2);assert(pad_usb_poll()&&pad_bits==0); /* truncated */
 packet(1,6);assert(pad_usb_poll()&&pad_bits==256); /* A */
 int before=frees;connected=0;detached();
 assert(frees==before); /* USB task must not free the submitting task's pipe */
 assert(!pad_usb_poll()&&pad_bits==0&&!pad_pipe&&!pad_pending);
 assert(aborts==1);pad_close();assert(allocations==frees&&!registered&&!forbids);
 connected=1;assert(pad_open(1));assert(pad_usb_attach());pad_close();
 assert(releases==1&&rescans==1&&allocations==frees);
 busy=1;assert(pad_open(1));int old=claims;
 assert(!pad_usb_attach()&&claims==old);pad_close();busy=0;
 descriptor_error=1;assert(pad_open(1));assert(!pad_usb_attach());pad_close();
 assert(allocations==frees&&!pad_binding);descriptor_error=0;
 allocation_error=1;assert(pad_open(1));assert(!pad_usb_attach());pad_close();
 assert(allocations==frees&&!pad_binding);allocation_error=0;
 assert(pad_open(1));assert(pad_usb_attach());packet(2,6);io_error=1;old=rescans;
 assert(!pad_usb_poll()&&!pad_pipe&&pad_bits==0);pad_close();io_error=0;
 assert(rescans==old+1); /* IO failure must also restore hid.class */
 connected=0;assert(pad_open(1));old=claims;
 assert(pad_poll(1)==0&&pad_poll(100)==0&&claims==old);pad_close();
 stack=0;assert(pad_open(1));assert(!PsdBase&&pad_poll(1)==0);pad_close();
 assert(allocations==frees&&!registered&&!forbids);
 /* Test the actual clocking/port decoder, not only pre-decoded button masks. */
 assert(pad_open(2));PotgoBase=&library;pot_available=0x3000;
 assert(pad_poll(1)==0&&pot_writes==0&&!pot_reserved);
 pot_available=0xf000;assert(pad_poll(2)==512); /* fourth high: Sega X */
 assert(pot_writes==9&&!interrupts_off);
 for(unsigned i=0;i<9;i++)assert(pot_values[i]==(i&1?0xe000:0xf000));
 pad_close();assert(pot_values[9]==0&&!pot_reserved);
 puts("PASS USB transport: reports, missing stack/device, busy binding, failures, disconnect/reconnect, balanced cleanup");
 puts("PASS Sega transport: resource contention, TH phases, pin 9 sense pull-up, port 1 untouched, interrupt/resource release");
 return 0;
}
