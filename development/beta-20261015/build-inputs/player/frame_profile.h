/* GPL-3.0-or-later. Host durations; no I/O in frame/flip sampling. */
struct fc_stage { uint64_t sum; ULONG n,max,bins[6]; };
static struct fc_stage fc_stages[6];
static ULONG fc_profile_frame,fc_last_flip,fc_input_changes;
static void fc_sample(unsigned stage,ULONG dt){
 struct fc_stage *s=&fc_stages[stage];unsigned b;
 if(fc_profile_frame<60)return; /* Exclude initial readback/startup. */
 s->sum+=dt;s->n++;if(dt>s->max)s->max=dt;
 b=dt<=1000?0:dt<=8000?1:dt<=17000?2:dt<=20000?3:dt<=34000?4:5;
 s->bins[b]++;
}
static void fc_flip_sample(void){ULONG now=us();if(fc_last_flip)fc_sample(5,now-fc_last_flip);fc_last_flip=now;}
static void fc_profile_print(void){
 unsigned i;for(i=0;i<6;i++){struct fc_stage *s=&fc_stages[i];
 printf("FRAME_TIMING stage=%u count=%u mean_us=%u max_us=%u bins=%u,%u,%u,%u,%u,%u\n",i,s->n,s->n?(ULONG)(s->sum/s->n):0,s->max,s->bins[0],s->bins[1],s->bins[2],s->bins[3],s->bins[4],s->bins[5]);}
 printf("INPUT_PROFILE changes=%u\n",fc_input_changes);
}
