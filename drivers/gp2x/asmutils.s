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


.global block_and @ void *src, size_t n, int andpat

block_and:
    stmfd   sp!, {r4-r5}
    orr     r2, r2, r2, lsl #8
    orr     r2, r2, r2, lsl #16
    mov     r1, r1, lsr #4
block_loop_and:
    ldmia   r0, {r3-r5,r12}
    subs    r1, r1, #1
    and     r3, r3, r2
    and     r4, r4, r2
    and     r5, r5, r2
    and     r12,r12,r2
    stmia   r0!, {r3-r5,r12}
    bne     block_loop_and
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


.global spend_cycles @ c

spend_cycles:
    mov     r0, r0, lsr #2  @ 4 cycles/iteration
    sub     r0, r0, #2      @ entry/exit/init
.sc_loop:
    subs    r0, r0, #1
    bpl     .sc_loop

    bx      lr


.global soft_scale @ void *dst, unsigned short *pal, int offs, int lines

soft_scale:
    stmfd   sp!,{r4-r11,lr}
    mov     lr, #0xff
    mov     lr, lr, lsl #1
    mov     r9, #0x3900        @ f800 07e0 001f | e000 0780 001c | 3800 01e0 0007
    orr     r9, r9, #0x00e7

    mov     r11,r3             @ r11= line counter
    mov     r3, r1             @ r3 = pal base

    mov     r12,#320
    mul     r2, r12,r2
    add     r4, r0, r2, lsl #1 @ r4 = dst_start
    add     r5, r0, r2         @ r5 = src_start
    mul     r12,r11,r12
    add     r0, r4, r12,lsl #1 @ r0 = dst_end
    add     r1, r5, r12        @ r1 = src_end

soft_scale_loop:
    sub     r1, r1, #64        @ skip borders
    mov     r2, #256/8

soft_scale_loop_line:
    ldr     r12, [r1, #-8]!
    ldr     r7,  [r1, #4]

    and     r4, lr, r12,lsl #1
    ldrh    r4, [r3, r4]
    and     r5, lr, r12,lsr #7
    ldrh    r5, [r3, r5]
    and     r4, r4, r9, lsl #2
    orr     r4, r4, r4, lsl #14       @ r4[31:16] = 1/4 pix_s 0
    and     r5, r5, r9, lsl #2
    sub     r6, r5, r5, lsr #2        @ r6 = 3/4 pix_s 1
    add     r4, r4, r6, lsl #16       @ pix_d 0, 1
    and     r6, lr, r12,lsr #15
    ldrh    r6, [r3, r6]
    and     r12,lr, r12,lsr #23
    ldrh    r12,[r3, r12]
    and     r6, r6, r9, lsl #2
    add     r5, r5, r6
    mov     r5, r5, lsr #1
    sub     r6, r6, r6, lsr #2        @ r6 = 3/4 pix_s 2
    orr     r5, r5, r6, lsl #16

    and     r6, lr, r7, lsl #1
    ldrh    r6, [r3, r6]
    and     r12,r12,r9, lsl #2
    add     r5, r5, r12,lsl #14       @ pix_d 2, 3
    and     r6, r6, r9, lsl #2
    orr     r6, r12,r6, lsl #16       @ pix_d 4, 5

    and     r12,lr, r7, lsr #7
    ldrh    r12,[r3, r12]
    and     r10,lr, r7, lsr #15
    ldrh    r10,[r3, r10]
    and     r12,r12,r9, lsl #2
    sub     r8, r12,r12,lsr #2        @ r8 = 3/4 pix_s 1
    add     r8, r8, r6, lsr #18
    and     r7, lr, r7, lsr #23
    ldrh    r7, [r3, r7]
    and     r10,r10,r9, lsl #2
    orr     r8, r8, r10,lsl #15
    add     r8, r8, r12,lsl #15       @ pix_d 6, 7
    sub     r10,r10,r10,lsr #2        @ r10= 3/4 pix_s 2
    and     r7, r7, r9, lsl #2
    add     r10,r10,r7, lsr #2        @ += 1/4 pix_s 3
    orr     r10,r10,r7, lsl #16       @ pix_d 8, 9

    subs    r2, r2, #1

    stmdb   r0!, {r4,r5,r6,r8,r10}
    bne     soft_scale_loop_line

    subs    r11,r11,#1
    bne     soft_scale_loop

    ldmfd   sp!,{r4-r11,lr}
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

