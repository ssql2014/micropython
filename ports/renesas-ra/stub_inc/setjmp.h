/* Minimal setjmp.h for ARM Cortex-M baremetal — used by MICROPY_NLR_SETJMP.
 * Homebrew's arm-none-eabi-gcc 15.2.0 ships without newlib, so we provide our
 * own setjmp/longjmp via setjmp_arm.S in the port root.
 *
 * jmp_buf layout matches ARM EABI: r4-r11, sp, lr (10 words = 40 bytes).
 */
#ifndef _SETJMP_H_LOCAL_STUB
#define _SETJMP_H_LOCAL_STUB

#ifdef __cplusplus
extern "C" {
#endif

/* 10 words = r4-r11 (8) + sp + lr = 10. Round up to 16 for alignment safety. */
typedef int jmp_buf[16];

extern int  setjmp(jmp_buf env) __attribute__((__returns_twice__));
extern void longjmp(jmp_buf env, int val) __attribute__((__noreturn__));

#ifdef __cplusplus
}
#endif

#endif
