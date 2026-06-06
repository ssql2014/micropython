/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013-2020 Damien P. George
 * Copyright (c) 2021,2022 Renesas Electronics Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stdio.h>
#include <string.h>

#include "py/runtime.h"
#include "py/gc.h"
#include "py/mperrno.h"
#include "py/mphal.h"
#include "shared/readline/readline.h"
#include "shared/runtime/pyexec.h"
#include "shared/runtime/softtimer.h"
#include "shared/tinyusb/mp_usbd.h"
#include "lib/oofatfs/ff.h"
#include "lib/littlefs/lfs1.h"
#include "lib/littlefs/lfs1_util.h"
#include "lib/littlefs/lfs2.h"
#include "lib/littlefs/lfs2_util.h"
#include "extmod/modmachine.h"
#include "extmod/vfs.h"
#include "extmod/vfs_fat.h"
#include "extmod/vfs_lfs.h"

#include "boardctrl.h"
#include "systick.h"
#include "pendsv.h"
#include "powerctrl.h"
#include "pybthread.h"
#include "gccollect.h"
#include "factoryreset.h"
#include "modmachine.h"
#include "spi.h"
#include "uart.h"
#include "timer.h"
#include "led.h"
#include "pin.h"
#if !MICROPY_RA8P1_BRINGUP_NO_EXTINT
#include "extint.h"
#endif
#include "usrsw.h"
#include "rtc.h"
#if !MICROPY_RA8P1_BRINGUP_NO_STORAGE
#include "storage.h"
#endif
#if MICROPY_PY_LWIP
#include "lwip/init.h"
#include "lwip/apps/mdns.h"
#endif
#if MICROPY_PY_BLUETOOTH
#include "mpbthciport.h"
#include "extmod/modbluetooth.h"
#endif
#include "extmod/modnetwork.h"

#if defined(BSP_MCU_R7KA8P1KFLCAC) && MICROPY_RA8P1_BRINGUP_MIPI_CSI_TEST && MICROPY_RA8P1_BRINGUP_MIPI_CSI_BOOT_PROBE
extern void ra8p1_mipi_csi_boot_probe(void);
#endif

#if MICROPY_RA8P1_BRINGUP_DISPLAY_TEST || (defined(BSP_MCU_R7KA8P1KFLCAC) && MICROPY_RA8P1_SAFE_BOOT && MICROPY_RA8P1_SAFE_BOOT_SDRAM)
#include "common_data.h"
#endif

#define RA_EARLY_PRINT  1       /* for enabling mp_print in boardctrl. */

#if MICROPY_RA8P1_BRINGUP_DISPLAY_TEST

#ifndef MICROPY_RA8P1_BRINGUP_DISPLAY_DEMO
#define MICROPY_RA8P1_BRINGUP_DISPLAY_DEMO (0)
#endif

#define RA8P1_DEMO_W   1024u
#define RA8P1_DEMO_H   600u

static const uint32_t ra8p1_demo_bands[8] = {
    0x00FF0000u,  /* RED     */
    0x00FF8000u,  /* ORANGE  */
    0x00FFFF00u,  /* YELLOW  */
    0x0000FF00u,  /* GREEN   */
    0x0000FFFFu,  /* CYAN    */
    0x000000FFu,  /* BLUE    */
    0x008000FFu,  /* INDIGO  */
    0x00FF00FFu,  /* MAGENTA */
};

#if MICROPY_RA8P1_BRINGUP_DISPLAY_DEMO
static void ra8p1_demo_paint(uint32_t *fb, uint32_t phase) {
    /* 8 vertical color bands sliding by `phase` pixels, with diagonal
     * brightness sweep that wraps with phase.  About 600k pixel writes per
     * call; M85 @200 MHz handles this in well under a frame at 60Hz. */
    const uint32_t W = RA8P1_DEMO_W;
    const uint32_t H = RA8P1_DEMO_H;
    const uint32_t band_px = W / 8u;
    for (uint32_t y = 0; y < H; ++y) {
        uint32_t *row = fb + (uint32_t)y * W;
        for (uint32_t x = 0; x < W; ++x) {
            uint32_t band = (((x + phase) / band_px)) & 0x7u;
            uint32_t c = ra8p1_demo_bands[band];
            /* Brightness 0..255 from a slowly-rotating diagonal */
            uint32_t s = ((x + y + (phase << 1)) >> 2) & 0xFFu;
            /* Simple triangle so center of band is brightest */
            uint32_t shade = (s < 128u) ? (s * 2u) : ((255u - s) * 2u);
            uint32_t r = (((c >> 16) & 0xFFu) * shade) >> 8;
            uint32_t g = (((c >>  8) & 0xFFu) * shade) >> 8;
            uint32_t b = (( c        & 0xFFu) * shade) >> 8;
            row[x] = (r << 16) | (g << 8) | b;
        }
    }
}
#endif

static void ra8p1_display_smoke_test(void) {
    (void)R_IOPORT_Open(&g_ioport_ctrl, &g_bsp_pin_cfg);
    R_BSP_SdramInit(true);

    fsp_err_t err = g_display.p_api->open(g_display.p_ctrl, g_display.p_cfg);
    if (err != FSP_SUCCESS && err != FSP_ERR_ALREADY_OPEN) {
        return;
    }

#if MICROPY_RA8P1_BRINGUP_DISPLAY_DEMO
    /* Paint frame 0, then start scan-out, then animate by alternating buffers. */
    uint32_t *fb0 = (uint32_t *)&g_framebuffer[0][0];
    uint32_t *fb1 = (uint32_t *)&g_framebuffer[1][0];

    ra8p1_demo_paint(fb0, 0u);
    (void)g_display.p_api->start(g_display.p_ctrl);

    /* Bounded animation: ~600 frames so REPL gets a chance after the demo. */
    for (uint32_t frame = 1; frame < 600u; ++frame) {
        uint32_t *back = (frame & 1u) ? fb1 : fb0;
        ra8p1_demo_paint(back, frame * 6u);
        /* Hand the back-buffer to GLCDC for the next vsync. */
        (void)g_display.p_api->bufferChange(g_display.p_ctrl,
            (uint8_t *)back, DISPLAY_FRAME_LAYER_1);
        /* Crude pacing — bufferChange is honored at next vsync (~60Hz). */
        R_BSP_SoftwareDelay(15, BSP_DELAY_UNITS_MILLISECONDS);
    }
#else
    /* Plain solid-color smoke (red) - kept as fallback. */
    uint32_t *fb = (uint32_t *)&g_framebuffer[0][0];
    size_t pixels = sizeof(g_framebuffer[0]) / sizeof(uint32_t);
    for (size_t i = 0; i < pixels; ++i) {
        fb[i] = 0x00ff0000u;
    }
    (void)g_display.p_api->start(g_display.p_ctrl);
#endif
}
#endif

#if defined(BSP_MCU_R7KA8P1KFLCAC)
static volatile uint8_t ra8p1_loop_tx[] = {0x55, 0xaa, 0x33, 0xcc, 'L', 'O', 'O', 'P'};
static volatile uint8_t ra8p1_loop_rx[16];
static volatile uint32_t ra8p1_loop_rx_count;
static volatile uint32_t ra8p1_loop_elapsed_ms;
static volatile int ra8p1_loop_errcode;
#endif

#if MICROPY_PY_THREAD
static pyb_thread_t pyb_thread_main;
#endif

#if defined(MICROPY_HW_UART_REPL)
#ifndef MICROPY_HW_UART_REPL_RXBUF
#define MICROPY_HW_UART_REPL_RXBUF (260)
#endif
static machine_uart_obj_t machine_uart_repl_obj;
static uint8_t machine_uart_repl_rxbuf[MICROPY_HW_UART_REPL_RXBUF];
#endif

void MP_NORETURN __fatal_error(const char *msg) {
#if defined(BSP_MCU_R7KA8P1KFLCAC)
    (void)msg;
    for (;;) {
        __asm volatile ("nop");
    }
#else
    for (volatile uint delay = 0; delay < 1000000; delay++) {
    }
    led_state(1, 1);
    led_state(2, 1);
    led_state(3, 1);
    led_state(4, 1);
    mp_hal_stdout_tx_strn("\nFATAL ERROR:\n", 14);
    mp_hal_stdout_tx_strn(msg, strlen(msg));
    for (uint i = 0;;) {
        led_toggle(((i++) & 3) + 1);
        for (volatile uint delay = 0; delay < 1000000; delay++) {
        }
        if (i >= 16) {
            __WFI();
        }
    }
#endif
}

void nlr_jump_fail(void *val) {
    printf("FATAL: uncaught exception %p\n", val);
    mp_obj_print_exception(&mp_plat_print, MP_OBJ_FROM_PTR(val));
    __fatal_error("");
}

void abort(void) {
    __fatal_error("abort");
}

#ifndef NDEBUG
void MP_WEAK __assert_func(const char *file, int line, const char *func, const char *expr) {
    (void)func;
    printf("Assertion '%s' failed, at file %s:%d\n", expr, file, line);
    __fatal_error("");
}
#endif

static mp_obj_t pyb_main(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_opt, MP_ARG_INT, {.u_int = 0} }
    };

    if (mp_obj_is_str(pos_args[0])) {
        MP_STATE_PORT(pyb_config_main) = pos_args[0];

        // parse args
        mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
        mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);
        #if MICROPY_ENABLE_COMPILER
        MP_STATE_VM(mp_optimise_value) = args[0].u_int;
        #endif
    }
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_KW(pyb_main_obj, 1, pyb_main);

#if MICROPY_HW_FLASH_MOUNT_AT_BOOT
// avoid inlining to avoid stack usage within main()
MP_NOINLINE static bool init_flash_fs(uint reset_mode) {
    if (reset_mode == BOARDCTRL_RESET_MODE_FACTORY_FILESYSTEM) {
        // Asked by user to reset filesystem
        factory_reset_create_filesystem();
    }

    // Default block device to entire flash storage
    mp_obj_t bdev = MP_OBJ_FROM_PTR(&pyb_flash_obj);

    int ret;

    #if MICROPY_VFS_LFS1 || MICROPY_VFS_LFS2

    // Try to detect the block device used for the main filesystem based on the
    // contents of the superblock, which can be the first or second block.
    mp_int_t len = -1;
    uint8_t buf[64];
    for (size_t block_num = 0; block_num <= 1; ++block_num) {
        ret = storage_readblocks_ext(buf, block_num, 0, sizeof(buf));

        #if MICROPY_VFS_LFS1
        if (ret == 0 && memcmp(&buf[40], "littlefs", 8) == 0) {
            // LFS1
            lfs1_superblock_t *superblock = (void *)&buf[12];
            uint32_t block_size = lfs1_fromle32(superblock->d.block_size);
            uint32_t block_count = lfs1_fromle32(superblock->d.block_count);
            len = block_count * block_size;
            break;
        }
        #endif

        #if MICROPY_VFS_LFS2
        if (ret == 0 && memcmp(&buf[8], "littlefs", 8) == 0) {
            // LFS2
            lfs2_superblock_t *superblock = (void *)&buf[20];
            uint32_t block_size = lfs2_fromle32(superblock->block_size);
            uint32_t block_count = lfs2_fromle32(superblock->block_count);
            len = block_count * block_size;
            break;
        }
        #endif
    }

    if (len != -1) {
        // Detected a littlefs filesystem so create correct block device for it
        mp_obj_t args[] = { MP_OBJ_NEW_QSTR(MP_QSTR_len), MP_OBJ_NEW_SMALL_INT(len) };
        bdev = MP_OBJ_TYPE_GET_SLOT(&pyb_flash_type, make_new)(&pyb_flash_type, 0, 1, args);
    }

    #endif

    // Try to mount the flash on "/flash" and chdir to it for the boot-up directory.
    mp_obj_t mount_point = MP_OBJ_NEW_QSTR(MP_QSTR__slash_flash);
    ret = mp_vfs_mount_and_chdir_protected(bdev, mount_point);

    if (ret == -MP_ENODEV && bdev == MP_OBJ_FROM_PTR(&pyb_flash_obj)
        && reset_mode != BOARDCTRL_RESET_MODE_FACTORY_FILESYSTEM) {
        // No filesystem, bdev is still the default (so didn't detect a possibly corrupt littlefs),
        // and didn't already create a filesystem, so try to create a fresh one now.
        ret = factory_reset_create_filesystem();
        if (ret == 0) {
            ret = mp_vfs_mount_and_chdir_protected(bdev, mount_point);
        }
    }

    if (ret != 0) {
        printf("MPY: can't mount flash\n");
        return false;
    }

    return true;
}
#endif

int main(void) {
    // Hook for a board to run code at start up, for example check if a
    // bootloader should be entered instead of the main application.
    MICROPY_BOARD_STARTUP();

    // Initialize interrupt, systick and internal flash for RA.
    ra_init();

    MICROPY_BOARD_EARLY_INIT();

    // basic sub-system init
    #if MICROPY_PY_THREAD
    pyb_thread_init(&pyb_thread_main);
    #endif
    pendsv_init();
    #if MICROPY_HW_USE_RTT_REPL
    // RTT must be initialised before any mp_hal_stdout_tx_strn call so the REPL
    // banner reaches the host via SWD (the EK-RA8P1 J-Link OB CDC bridge does
    // not deliver bytes — see board README for details).
    extern void SEGGER_RTT_Init(void);
    SEGGER_RTT_Init();
    #endif
    led_init();
    #if MICROPY_HW_HAS_SWITCH
    switch_init0();
    #endif
    machine_init();

    #if defined(BSP_MCU_R7KA8P1KFLCAC) && MICROPY_RA8P1_BRINGUP_MIPI_CSI_TEST && MICROPY_RA8P1_BRINGUP_MIPI_CSI_BOOT_PROBE
    ra8p1_mipi_csi_boot_probe();
    #endif

    #if MICROPY_RA8P1_BRINGUP_DISPLAY_TEST
    ra8p1_display_smoke_test();
    #endif
    #if MICROPY_HW_ENABLE_RTC
    rtc_init_start(false);
    #endif
    uart_init0();
    spi_init0();
    #if MICROPY_HW_ENABLE_STORAGE && !MICROPY_RA8P1_BRINGUP_NO_STORAGE
    storage_init();
    #endif

    #if defined(MICROPY_HW_UART_REPL) && !MICROPY_HW_USE_RTT_REPL
    // Set up a UART REPL using a statically allocated object
    machine_uart_repl_obj.base.type = &machine_uart_type;
    machine_uart_repl_obj.uart_id = MICROPY_HW_UART_REPL;
    machine_uart_repl_obj.is_static = true;
    machine_uart_repl_obj.timeout = 0;
    machine_uart_repl_obj.timeout_char = 2;
    uart_init(&machine_uart_repl_obj, MICROPY_HW_UART_REPL_BAUD, UART_WORDLENGTH_8B, UART_PARITY_NONE, UART_STOPBITS_1, 0);
    uart_set_rxbuf(&machine_uart_repl_obj, sizeof(machine_uart_repl_rxbuf), machine_uart_repl_rxbuf);
    uart_attach_to_repl(&machine_uart_repl_obj, true);
    MP_STATE_PORT(machine_uart_obj_all)[MICROPY_HW_UART_REPL] = &machine_uart_repl_obj;
    #if RA_EARLY_PRINT
    MP_STATE_PORT(pyb_stdio_uart) = &machine_uart_repl_obj;
    #endif
    #if defined(BSP_MCU_R7KA8P1KFLCAC) && MICROPY_RA8P1_BRINGUP_LED_TEST
    uint32_t led_phase = 0;
    for (;;) {
        R_IOPORT_PinWrite(&g_ioport_ctrl, BSP_IO_PORT_06_PIN_00, led_phase ? BSP_IO_LEVEL_HIGH : BSP_IO_LEVEL_LOW);
        R_IOPORT_PinWrite(&g_ioport_ctrl, BSP_IO_PORT_03_PIN_03, led_phase ? BSP_IO_LEVEL_LOW : BSP_IO_LEVEL_HIGH);
        R_IOPORT_PinWrite(&g_ioport_ctrl, BSP_IO_PORT_10_PIN_07, led_phase ? BSP_IO_LEVEL_HIGH : BSP_IO_LEVEL_LOW);
        led_phase ^= 1;
        for (volatile uint32_t delay = 0; delay < 1200000; ++delay) {
            __asm__ __volatile__("nop");
        }
    }
    #endif
    #endif

    boardctrl_state_t state;
    state.reset_mode = 1;
    state.log_soft_reset = false;

    #if MICROPY_PY_BLUETOOTH
    mp_bluetooth_hci_init();
    #endif

    MICROPY_BOARD_BEFORE_SOFT_RESET_LOOP(&state);

    #if MICROPY_PY_LWIP
    // lwIP doesn't allow to reinitialise itself by subsequent calls to this function
    // because the system timeout list (next_timeout) is only ever reset by BSS clearing.
    // So for now we only init the lwIP stack once on power-up.
    lwip_init();
    #if LWIP_MDNS_RESPONDER
    mdns_resp_init();
    #endif
    #endif

soft_reset:

    MICROPY_BOARD_TOP_SOFT_RESET_LOOP(&state);

    // Python threading init
    #if MICROPY_PY_THREAD
    mp_thread_init();
    #endif

    // RA8P1 cstack-init bring-up note:
    // FSP startup leaves MSP at the BSP bootstrap stack (~0x22002000), far
    // below the linker's _estack (0x22100000).  Passing &_estack to
    // mp_cstack_init_with_top makes mp_cstack_check() see ~1 MB used and raise
    // RuntimeError("maximum recursion depth exceeded") on the first eval.
    // Using the runtime MSP with 64 KB headroom matches the actual stack.
    #if defined(BSP_MCU_R7KA8P1KFLCAC)
    {
        uintptr_t actual_sp = __get_MSP();
        mp_cstack_init_with_top((char *)actual_sp, 0x10000);
    }
    #else
    mp_cstack_init_with_top(&_estack, (char *)&_estack - (char *)&_sstack);
    #endif

    // GC init — heap in external SDRAM.  Internal 1 MB SRAM has a yet-unknown
    // failure mode for gc_init at heaps > ~128 KB even though individual
    // mem32[] writes to that region succeed at runtime.  Hypothesis: gc_init's
    // initial alloc-table sweep across the full heap range trips something
    // (cache coherency? unaligned long-stride access?).  SDRAM is uniform and
    // works — first 5 MB reserved for GLCDC framebuffer, next 2 MB for heap.
    #if defined(BSP_MCU_R7KA8P1KFLCAC) && MICROPY_RA8P1_SAFE_BOOT
    #if MICROPY_RA8P1_SAFE_BOOT_SDRAM
    (void)R_IOPORT_Open(&g_ioport_ctrl, &g_bsp_pin_cfg);
    R_BSP_SdramInit(true);
    #endif
    #if MICROPY_RA8P1_SAFE_BOOT_SDRAM_HEAP
    gc_init((char *)0x68500000UL, (char *)0x68700000UL);
    #else
    gc_init((char *)0x22020000UL, (char *)0x22030000UL);
    #endif
    #elif defined(BSP_MCU_R7KA8P1KFLCAC)
    R_BSP_SdramInit(true);
    gc_init((char *)0x68500000UL, (char *)0x68700000UL);
    #else
    gc_init(MICROPY_HEAP_START, MICROPY_HEAP_END);
    #endif

    #if MICROPY_ENABLE_PYSTACK
    static mp_obj_t pystack[384];
    mp_pystack_init(pystack, &pystack[384]);
    #endif

    // MicroPython init
    mp_init();

    // Initialise low-level sub-systems.  Here we need to very basic things like
    // zeroing out memory and resetting any of the sub-systems.  Following this
    // we can run Python scripts (eg boot.py), but anything that is configurable
    // by boot.py must be set after boot.py is run.

    #if defined(MICROPY_HW_UART_REPL) && !MICROPY_HW_USE_RTT_REPL
    MP_STATE_PORT(pyb_stdio_uart) = &machine_uart_repl_obj;
    #else
    MP_STATE_PORT(pyb_stdio_uart) = NULL;
    #endif
    readline_init0();
    machine_pin_init();
    #if !MICROPY_RA8P1_BRINGUP_NO_EXTINT
    extint_init0();
    #endif
    timer_init0();
#if MICROPY_HW_ENABLE_I2S
    machine_i2s_init0();
    #endif

    #if MICROPY_PY_NETWORK
    mod_network_init();
    #endif

    #if MICROPY_PY_LWIP
    mod_network_lwip_init();
    #endif

    // Initialise the local flash filesystem.
    // Create it if needed, mount in on /flash, and set it as current dir.
    bool mounted_flash = false;
    #if MICROPY_HW_FLASH_MOUNT_AT_BOOT
    mounted_flash = init_flash_fs(state.reset_mode);
    #endif

    // set sys.path based on mounted filesystems
    if (mounted_flash) {
        mp_obj_list_append(mp_sys_path, MP_OBJ_NEW_QSTR(MP_QSTR__slash_flash));
        mp_obj_list_append(mp_sys_path, MP_OBJ_NEW_QSTR(MP_QSTR__slash_flash_slash_lib));
    }

    // reset config variables; they should be set by boot.py
    MP_STATE_PORT(pyb_config_main) = MP_OBJ_NULL;

    // Run optional frozen boot code.
    #ifdef MICROPY_BOARD_FROZEN_BOOT_FILE
    pyexec_frozen_module(MICROPY_BOARD_FROZEN_BOOT_FILE, false);
    #endif

    // Run boot.py (or whatever else a board configures at this stage).
    int boot_res = MICROPY_BOARD_RUN_BOOT_PY(&state);
    // Now we initialise sub-systems that need configuration from boot.py,
    // or whose initialisation can be safely deferred until after running
    // boot.py.

    #if MICROPY_HW_ENABLE_USBDEV
    mp_usbd_init();
    #endif

    if (boot_res == BOARDCTRL_GOTO_SOFT_RESET_EXIT) {
        goto soft_reset_exit;
    }

    // At this point everything is fully configured and initialised.

    // Run main.py (or whatever else a board configures at this stage).
    if (MICROPY_BOARD_RUN_MAIN_PY(&state) == BOARDCTRL_GOTO_SOFT_RESET_EXIT) {
        goto soft_reset_exit;
    }

    #if MICROPY_ENABLE_COMPILER
    // Main script is finished, so now go into REPL mode.
    // The REPL mode can change, or it can request a soft reset.
    for (;;) {
        if (pyexec_mode_kind == PYEXEC_MODE_RAW_REPL) {
            if (pyexec_raw_repl() != 0) {
                break;
            }
        } else {
            if (pyexec_friendly_repl() != 0) {
                break;
            }
        }
    }
    #endif

soft_reset_exit:

    // soft reset

    MICROPY_BOARD_START_SOFT_RESET(&state);

    #if MICROPY_HW_ENABLE_STORAGE
    if (state.log_soft_reset) {
        mp_printf(&mp_plat_print, "MPY: sync filesystems\n");
    }
    storage_flush();
    #endif

    if (state.log_soft_reset) {
        mp_printf(&mp_plat_print, "MPY: soft reboot\n");
    }

    #if MICROPY_PY_BLUETOOTH
    mp_bluetooth_deinit();
    #endif
    #if MICROPY_PY_NETWORK
    mod_network_deinit();
    #endif
    soft_timer_deinit();
    timer_deinit();
    uart_deinit_all();
    #if MICROPY_HW_ENABLE_DAC
    dac_deinit_all();
    #endif
    machine_pin_deinit();
    machine_deinit();

    #if MICROPY_PY_THREAD
    pyb_thread_deinit();
    #endif

    MICROPY_BOARD_END_SOFT_RESET(&state);

    gc_sweep_all();
    mp_deinit();

    goto soft_reset;
}

MP_REGISTER_ROOT_POINTER(mp_obj_t pyb_config_main);
