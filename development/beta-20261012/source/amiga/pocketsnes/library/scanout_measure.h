/* Read-only startup sampling of firmware's FPGA vblank status register.
 * No bitmap lock, DMA programming or OS/ARM clock changes. These are host
 * observations, not timestamps of the DMA address latch or monitor pixels. */
static int full_measure_refresh(void){
 ULONG start=us(),last_event=start,last_rise=0,sum=0,min=0xffffffffu,max=0;
 unsigned n=0;int previous=full_blank(),have_rise=0;
 while(n<16){
  int blank=full_blank();ULONG now=us();
  if((ULONG)(now-start)>800000u){puts("SCANOUT_REFRESH unavailable: insufficient observed edges");return 0;}
  if(!previous&&blank){
   if(have_rise){ULONG dt=now-last_rise;sum+=dt;if(dt<min)min=dt;if(dt>max)max=dt;n++;}
   last_rise=now;have_rise=1;
  }
  previous=blank;
  if((ULONG)(now-last_event)>=2000u){events();last_event=now;if(quit)return 0;}
 }
 if(!sum)return 0;
 printf("SCANOUT_REFRESH intervals=%u mean_us=%u min_us=%u max_us=%u observed_millihz=%u\n",n,sum/n,min,max,(ULONG)(16000000000ULL/sum));
 return 1;
}
