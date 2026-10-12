/* SNES-only deadlines matching the core-reported region. */
static inline ULONG fc_next_deadline(ULONG now,ULONG previous,ULONG period){
 if(!previous || (LONG)(now-previous)>(LONG)(period*2u))previous=now;
 return previous+period;
}
