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

/* Four output pixels from a contiguous source row. Each 52-byte descriptor
 * contains source byte offset, 16 A indices, 16 B indices and 16 weights.
 * vtbl gathers within eight source pixels. Caller pads the source by 7 pixels
 * and handles its 0..3-pixel output tail. Only caller-saved SIMD regs used. */
.global em_blend_neon
.hidden em_blend_neon
em_blend_neon:
 movs r3,r3,lsr#2
 bxeq lr
3:
 ldr ip,[r2],#4
 add ip,r1,ip
 vld1.8 {d0-d3},[ip]
 vld1.8 {d4-d5},[r2]!
 vtbl.8 d16,{d0-d3},d4
 vtbl.8 d17,{d0-d3},d5
 vld1.8 {d4-d5},[r2]!
 vtbl.8 d18,{d0-d3},d4
 vtbl.8 d19,{d0-d3},d5
 vld1.8 {d4-d5},[r2]!
 vmull.u8 q10,d18,d4
 vmull.u8 q11,d19,d5
 vmlsl.u8 q10,d16,d4
 vmlsl.u8 q11,d17,d5
 vshll.u8 q12,d16,#8
 vshll.u8 q13,d17,#8
 vadd.i16 q10,q10,q12
 vadd.i16 q11,q11,q13
 vrshrn.u16 d0,q10,#8
 vrshrn.u16 d1,q11,#8
 vst1.8 {d0-d1},[r0]!
 subs r3,r3,#1
 bne 3b
 bx lr
