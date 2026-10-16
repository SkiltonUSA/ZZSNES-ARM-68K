/* GPL-3.0-or-later. Host-only timer latency; no ARM/shared layout changes. */
#include "pace.h"
struct fc_wait_profile { ULONG calls,requested,actual,maximum; };
static struct fc_wait_profile fc_waits[6];
static int profile_pause(unsigned kind,ULONG delay){
 ULONG start=us(),elapsed;int ok=pause_us(delay);
 elapsed=us()-start;fc_waits[kind].calls++;fc_waits[kind].requested+=delay;
 fc_waits[kind].actual+=elapsed;
 if(elapsed>fc_waits[kind].maximum)fc_waits[kind].maximum=elapsed;
 return ok;
}
static ULONG fc_send_us,fc_waitio_us;
static void profile_send_io(struct IORequest *r){
 ULONG start=us();SendIO(r);fc_send_us+=us()-start;
}
static LONG profile_wait_io(struct IORequest *r){
 ULONG start=us();LONG result=WaitIO(r);fc_waitio_us+=us()-start;return result;
}
static void print_wait_profile(void){
 unsigned i;
 printf("AUDIO_IO_PROFILE send_us=%u waitio_us=%u\n",fc_send_us,fc_waitio_us);
 for(i=0;i<6;i++)printf("WAIT_PROFILE kind=%u calls=%u requested_us=%u actual_us=%u max_us=%u\n",
  i,fc_waits[i].calls,fc_waits[i].requested,fc_waits[i].actual,fc_waits[i].maximum);
}
