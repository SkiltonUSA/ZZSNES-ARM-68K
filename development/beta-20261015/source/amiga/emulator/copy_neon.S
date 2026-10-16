/* GPL-3.0-or-later. Exactly 320 words; caller saves/restores SIMD state.
 * Same contiguous NEON load/store strategy as ZZDosBox's pixel routines. */
.syntax unified
.arm
.fpu neon
.text
.global em_copy_neon
.hidden em_copy_neon
em_copy_neon:
 mov r2,#40
1: vld1.32 {d0-d3},[r1]!
 vst1.32 {d0-d3},[r0]!
 subs r2,r2,#1
 bne 1b
 bx lr

/* Fuse scanline byte dimming with the store, as in ZZDosBox scale().
 * r2 = brightness numerator (0..8); each output byte = byte*r2/8. */
.global em_dim_neon
.hidden em_dim_neon
em_dim_neon:
 vdup.8 d6,r2
 mov r3,#80
2: vld1.8 {d0-d1},[r1]!
 vmull.u8 q1,d0,d6
 vmull.u8 q2,d1,d6
 vshrn.u16 d0,q1,#3
 vshrn.u16 d1,q2,#3
 vst1.8 {d0-d1},[r0]!
 subs r3,r3,#1
 bne 2b
 bx lr
