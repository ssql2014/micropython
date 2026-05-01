#include "hal_data.h"

void R_BSP_WarmStart(bsp_warm_start_event_t event);

// This function is called at various points during the startup process.
// event: where in the start up process the code is currently at.
void R_BSP_WarmStart(bsp_warm_start_event_t event) {
    if (BSP_WARM_START_RESET == event) {
        #if BSP_FEATURE_FLASH_LP_VERSION != 0

        // Enable reading from data flash.
        R_FACI_LP->DFLCTL = 1U;

        // Would normally have to wait tDSTOP(6us) for data flash recovery.  Placing the
        // enable here, before clock and C runtime initialization, should negate the need
        // for a delay since the initialization will typically take more than 6us.
        #endif
    }

    if (BSP_WARM_START_POST_C == event) {
        // At this point, C runtime environment and system clocks are set up.

        // Configure pins.
        R_IOPORT_Open(&g_ioport_ctrl, &g_bsp_pin_cfg);

        // Note: previously this hook contained an RA8P1-specific manual
        // MSTP-release for SCI8 (via PRCR unlock + MSTPCRB write).  Removed
        // because it was speculative — the actual root cause of the
        // UART_BYTES=0 issue turned out to be the J-Link OB CDC bridge on
        // the EK-RA8P1 board not delivering bytes to the host even from
        // working SCI8 transmits (proven via Renesas factory firmware also
        // emitting 0 bytes from VCOM despite SCI8 module-stop being clear).
        // The REPL is now routed through SEGGER RTT (over SWD) instead of
        // SCI8 VCOM — see MICROPY_HW_USE_RTT_REPL in mpconfigboard.h.
    }
}

#if BSP_TZ_SECURE_BUILD

BSP_CMSE_NONSECURE_ENTRY void template_nonsecure_callable();

// Trustzone Secure Projects require at least one nonsecure callable function in order to build.
BSP_CMSE_NONSECURE_ENTRY void template_nonsecure_callable() {
}

#endif
