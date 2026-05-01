/* Minimal setjmp/longjmp for ARM Cortex-M (Thumb-2) — used by MICROPY_NLR_SETJMP.
 * Saves r4-r11, sp, lr.  Returns 0 from setjmp; longjmp returns 'val' (or 1 if 0).
 */
    .syntax unified
    .thumb

    .global setjmp
    .type   setjmp, %function
    .thumb_func
setjmp:
    /* r0 = jmp_buf* env */
    stmia   r0!, {r4-r11}
    str     sp, [r0], #4
    str     lr, [r0]
    movs    r0, #0
    bx      lr
    .size   setjmp, .-setjmp

    .global longjmp
    .type   longjmp, %function
    .thumb_func
longjmp:
    /* r0 = jmp_buf* env, r1 = val */
    ldmia   r0!, {r4-r11}
    ldr     sp, [r0], #4
    ldr     lr, [r0]
    cmp     r1, #0
    it      eq
    moveq   r1, #1
    mov     r0, r1
    bx      lr
    .size   longjmp, .-longjmp
