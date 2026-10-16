/* GPL-3.0-or-later. Refuse unavailable SIMD; never change CPACR/FPEXC.
 * Library math and optional PIC VFP share this saved state. */
.syntax unified
.arm
.fpu neon
.text
.global db_neon_available
.hidden db_neon_available
db_neon_available:
 mrc p15,0,r1,c1,c0,2
 str r1,[r0]
 tst r1,#0xc0000000
 bne 1f
 and r1,r1,#0x00f00000
 cmp r1,#0x00f00000
 bne 1f
 isb
 vmrs r1,fpexc
 str r1,[r0,#4]
 tst r1,#0x40000000
 beq 1f
 vmrs r1,mvfr0
 str r1,[r0,#8]
 and r1,r1,#15
 cmp r1,#2
 bne 1f
 vmrs r1,mvfr1
 str r1,[r0,#12]
 ubfx r1,r1,#8,#12
 movw r2,#0x111
 cmp r1,r2
 bne 1f
 mov r0,#1
 bx lr
1: mov r0,#0
 bx lr
.global db_neon_snapshot
.hidden db_neon_snapshot
db_neon_snapshot:
 vstmia r0!,{d0-d15}
 vstmia r0!,{d16-d31}
 vmrs r1,fpscr
 str r1,[r0]
 bx lr
.global db_neon_restore
.hidden db_neon_restore
db_neon_restore:
 vldmia r0!,{d0-d15}
 vldmia r0!,{d16-d31}
 ldr r1,[r0]
 vmsr fpscr,r1
 bx lr
.global db_neon_witness
.hidden db_neon_witness
db_neon_witness:
 vmov.i32 q0,#21
 vadd.i32 q0,q0,q0
 vmov r0,s0
 bx lr
.global db_neon_ieee_begin
.hidden db_neon_ieee_begin
db_neon_ieee_begin:
 mov r0,#0
 vmsr fpscr,r0
 bx lr
