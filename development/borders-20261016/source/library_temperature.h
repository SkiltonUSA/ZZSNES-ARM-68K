/* GPL-3.0-or-later. Read-only Zynq sensor, as used by ZZTemperature.
 * MNT ZZTop: manufacturer 0x6d6e, product 4/3, register +0xe0 in 0.1 C.
 * No XACP commands, ARM worker or register writes. */
#include <libraries/configvars.h>
#include <proto/expansion.h>
struct ExpansionBase *ExpansionBase;
static volatile UBYTE *temperature_registers;
static Object *temperature_text;
static unsigned temperature_ticks;
static char temperature_caption[40];
static void temperature_poll(void){
 if(temperature_ticks){temperature_ticks--;return;}temperature_ticks=39;
 unsigned raw=temperature_registers?*(volatile UWORD *)(temperature_registers+0xe0):0;
 if(raw>0&&raw<=1500)snprintf(temperature_caption,sizeof(temperature_caption),"ZZ9000: %u.%u C",raw/10,raw%10);
 else strcpy(temperature_caption,"ZZ9000: n/a");
 set(temperature_text,MUIA_Text_Contents,(ULONG)temperature_caption);
}
static void temperature_init(void){
 ExpansionBase=(struct ExpansionBase*)OpenLibrary("expansion.library",37);
 if(ExpansionBase){
  struct ConfigDev *board=FindConfigDev(NULL,0x6d6e,4);
  if(!board)board=FindConfigDev(NULL,0x6d6e,3);
  if(board&&board->cd_BoardAddr&&board->cd_BoardSize>0xe1)temperature_registers=(volatile UBYTE*)board->cd_BoardAddr;
  CloseLibrary((struct Library*)ExpansionBase);ExpansionBase=NULL;
 }
 temperature_poll();
}
