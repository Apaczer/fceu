@ vim:filetype=armasm

@ test
.global flushcache @ beginning_addr, end_addr, flags

flushcache:
    swi #0x9f0002
    mov pc, lr


.global block_or @ void *src, size_t n, int pat

block_or:
    stmfd   sp!, {r4-r5}
    orr     r2, r2, r2, lsl #8
    orr     r2, r2, r2, lsl #16
    mov     r1, r1, lsr #4
block_loop_or:
    ldmia   r0, {r3-r5,r12}
    subs    r1, r1, #1
    orr     r3, r3, r2
    orr     r4, r4, r2
    orr     r5, r5, r2
    orr     r12,r12,r2
    stmia   r0!, {r3-r5,r12}
    bne     block_loop_or
    ldmfd   sp!, {r4-r5}
    bx      lr


.global block_andor @ void *src, size_t n, int andpat, int orpat

block_andor:
    stmfd   sp!, {r4-r6}
    orr     r2, r2, r2, lsl #8
    orr     r2, r2, r2, lsl #16
    orr     r3, r3, r3, lsl #8
    orr     r3, r3, r3, lsl #16
    mov     r1, r1, lsr #4
block_loop_andor:
    ldmia   r0, {r4-r6,r12}
    subs    r1, r1, #1
    and     r4, r4, r2
    orr     r4, r4, r3
    and     r5, r5, r2
    orr     r5, r5, r3
    and     r6, r6, r2
    orr     r6, r6, r3
    and     r12,r12,r2
    orr     r12,r12,r3
    stmia   r0!, {r4-r6,r12}
    bne     block_loop_andor
    ldmfd   sp!, {r4-r6}
    bx      lr



/* buggy and slow, probably because function call overhead
@ renderer helper, based on bitbank's method
.global draw8pix @ uint8 *P, uint8 *C, uint8 *PALRAM @ dest, src, pal

draw8pix:
    stmfd sp!, {r4,r5}

    ldrb  r3, [r1]            @ get bit 0 pixels
    mov   r12,#1
    orr   r12,r12,r12,lsl #8
    orr   r12,r12,r12,lsl #16
    ldrb  r1, [r1, #8]        @ get bit 1 pixels
    orr   r3, r3, r3, lsl #9  @ shift them over 1 byte + 1 bit
    orr   r3, r3, r3, lsl #18 @ now 4 pixels take up 4 bytes
    and   r4, r12,r3, lsr #7  @ mask off the upper nibble pixels we want
    and   r5, r12,r3, lsr #3  @ mask off the lower nibble pixels we want
    ldr   r2, [r2]

    orr   r1, r1, r1, lsl #9  @ process the bit 1 pixels
    orr   r1, r1, r1, lsl #18
    and   r3, r12,r1, lsr #7  @ mask off the upper nibble pixels we want
    and   r1, r12,r1, lsr #3  @ mask off the lower nibble
    orr   r4, r4, r3, lsl #1
    orr   r5, r5, r1, lsl #5

    @ can this be avoided?
    mov   r4, r4, lsl #3      @ *8
    mov   r3, r2, ror r4
    strb  r3, [r0], #1
    mov   r4, r4, lsr #8
    mov   r3, r2, ror r4
    strb  r3, [r0], #1
    mov   r4, r4, lsr #8
    mov   r3, r2, ror r4
    strb  r3, [r0], #1
    mov   r4, r4, lsr #8
    mov   r3, r2, ror r4
    strb  r3, [r0], #1

    mov   r5, r5, lsl #3      @ *8
    mov   r3, r2, ror r5
    strb  r3, [r0], #1
    mov   r5, r5, lsr #8
    mov   r3, r2, ror r5
    strb  r3, [r0], #1
    mov   r5, r5, lsr #8
    mov   r3, r2, ror r5
    strb  r3, [r0], #1
    mov   r5, r5, lsr #8
    mov   r3, r2, ror r5
    strb  r3, [r0], #1

    ldmfd sp!, {r4,r5}
    bx    lr
*/

