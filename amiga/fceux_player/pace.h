/* GPL-3.0-or-later. Preserve cadence across small OS scheduling delays.
 * Limit catch-up to two frames; reset after long stalls and when paused.
 * Unsigned wrap is intentional. Caller supplies zero on pause/resume. */
static inline ULONG fc_next_deadline(ULONG now,ULONG previous){
 if(!previous || (LONG)(now-previous)>33278)previous=now;
 return previous+16639u;
}
