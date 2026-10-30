/* GPL-3.0-or-later. Frame-boundary pause before slow state/snapshot I/O.
 * Core1 is idle and the bitmap is unlocked. Retire real PCM before silence,
 * then let the worker prime a fresh batch when emulation resumes. */
static int fc_audio_drain(void){
 audio_state_drained=0;audio_state_drain=1;player_audio_flush();
 if(audio_opened){ULONG start=us();
  while(!audio_state_drained){
   if(audio_error||audio_done||(ULONG)(us()-start)>1000000u){
    puts("AUDIO drain failed");audio_state_drain=0;return 0;
   }
   if(!pause_us(1000)){audio_state_drain=0;return 0;}
  }
 }
 audio_state_drain=0;return 1;
}
