/* GPL-3.0-or-later. Bounded host observations, not scanout timestamps.
 * Main task only; cumulative audio counters are sampled without blocking the
 * worker. Their individual reads are not an atomic multi-counter snapshot.
 * Print after audio close, never inside the emulation/display loop. */
#define AT_FRAMES 1200u
#define AT_ACKS 1202u
struct at_row {ULONG stage[5],copy,wait,samples,submitted,completed,gaps;};
struct at_ack_row {ULONG frame,time;};
static struct at_row at_rows[AT_FRAMES];
static struct at_ack_row at_acks[AT_ACKS];
static ULONG at_count,at_ack_count,at_overflow;
static void at_stage(unsigned stage,ULONG frame,ULONG dt){
 if(stage>=5)return;
 if(frame>=AT_FRAMES){at_overflow=1;return;}
 at_rows[frame].stage[stage]=dt;
}
static void at_audio(ULONG frame,ULONG copy,ULONG wait,ULONG samples,
 ULONG submitted,ULONG completed,ULONG gaps){
 struct at_row *r;
 if(frame>=AT_FRAMES){at_overflow=1;return;}
 r=&at_rows[frame];r->copy=copy;r->wait=wait;r->samples=samples;
 r->submitted=submitted;r->completed=completed;r->gaps=gaps;
 at_count=frame+1;
}
static void at_ack(ULONG frame,ULONG now){
 if(at_ack_count>=AT_ACKS){at_overflow=1;return;}
 at_acks[at_ack_count].frame=frame;at_acks[at_ack_count++].time=now;
}
static void at_print(void){
 ULONG i;printf("AUDIO_TRACE frames=%u acks=%u overflow=%u\n",at_count,at_ack_count,at_overflow);
 for(i=0;i<at_count;i++){
  struct at_row *r=&at_rows[i];
  printf("AT_FRAME %u %u %u %u %u %u %u %u %u %u %u %u\n",i,
   r->stage[0],r->stage[1],r->stage[2],r->stage[3],r->stage[4],
   r->copy,r->wait,r->samples,r->submitted,r->completed,r->gaps);
 }
 for(i=0;i<at_ack_count;i++)printf("AT_ACK %u %u\n",at_acks[i].frame,at_acks[i].time);
}
