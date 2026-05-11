/*
 * CANFD driver for RA8P1 – implements the ra8p1_canfd_* interface used by
 * machine_can.c.
 *
 * Configuration:
 *   Nominal bit rate : 1 Mbit/s (adjustable via ra8p1_canfd_init baudrate arg)
 *   Data bit rate    : same as nominal (FD, no BRS for simplicity)
 *   Frame filter     : accept all (mask_id = 0, mask_id_mode = 0)
 *   RX               : FIFO 0 (polling, no interrupts)
 *   TX               : TX message buffer 0 (blocking, timeout-based)
 *
 * Clock assumption: CANFD peripheral clock = 40 MHz (from PLL2 ÷ 6).
 * Recalculate bit timing if the CANFDCLK is different on your board.
 *
 * Pin connections (verified against ra_cfg.txt):
 *   CANFD0 CTX0 → P401 (N17)  — enable in FSP pin config when transceiver fitted
 *   CANFD0 CRX0 → P402 (L14)  — enable in FSP pin config when transceiver fitted
 * Both pins are disabled by default (no on-board transceiver on EK-RA8P1).
 */

#if defined(RA8P1)

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "hal_data.h"
#include "ra8p1_canfd.h"
#include "r_canfd.h"

/* ---- bit timing for 1 Mbit/s @ 40 MHz CANFDCLK ----------------------
 *   Prescaler=4, TSEG1=7, TSEG2=2, SJW=1
 *   Tq = 1/(40M/4) = 100 ns ; 1 bit = (1+7+2) Tq = 1 μs = 1 Mbit/s
 * -------------------------------------------------------------------- */
static can_bit_timing_cfg_t s_timing_nominal = {
    .baud_rate_prescaler = 4,
    .time_segment_1      = 7,
    .time_segment_2      = 2,
    .synchronization_jump_width = 1,
};

/* FD data timing = same as nominal (no bit-rate switching) */
static can_bit_timing_cfg_t s_timing_data = {
    .baud_rate_prescaler = 4,
    .time_segment_1      = 7,
    .time_segment_2      = 2,
    .synchronization_jump_width = 1,
};

/* ---- Accept-all AFL entry ------------------------------------------ */
/* mask_id = 0 → accept any ID; mask_frame_type = mask_id_mode = 0 → match all */
static const canfd_afl_entry_t s_afl[] = {
    {
        .id       = { .id = 0, .frame_type = CAN_FRAME_TYPE_DATA,
                      .id_mode = CAN_ID_MODE_STANDARD },
        .mask     = { .mask_id = 0, .mask_frame_type = 0, .mask_id_mode = 0 },
        .destination = {
            .minimum_dlc   = 0,
            .rx_buffer     = CANFD_RX_MB_NONE,        /* to FIFO, not mailbox */
            .fifo_select_flags = CANFD_RX_FIFO_0,
        },
    },
};

/* ---- Global CANFD configuration ------------------------------------ */
static canfd_global_cfg_t s_global_cfg = {
    .global_interrupts  = 0,        /* no global IRQ */
    .global_config      = 0,        /* default: ISO CAN FD */
    /* rx_fifo_config[0]: enable FIFO 0, depth=4, no interrupt */
    .rx_fifo_config     = { [0] = 0x00000041U }, /* RFDC=4, RFIE=0, RFE=1 */
    .rx_mb_config       = 0,        /* no RX mailboxes */
    .global_err_ipl     = 0,
    .rx_fifo_ipl        = 0,
    .common_fifo_config = { 0 },
};

/* ---- Extended & base config ---------------------------------------- */
static canfd_extended_cfg_t s_ext_cfg = {
    .p_afl            = s_afl,
    .txmb_txi_enable  = 0,          /* TX IRQs disabled */
    .error_interrupts = 0,
    .p_data_timing    = &s_timing_data,
    .delay_compensation = 0,
    .p_global_cfg     = &s_global_cfg,
};

static can_cfg_t s_cfg = {
    .channel      = 0,
    .p_bit_timing = &s_timing_nominal,
    .p_callback   = NULL,
    .p_context    = NULL,
    .p_extend     = &s_ext_cfg,
    .ipl          = 0,
    .error_irq    = FSP_INVALID_VECTOR,
    .rx_irq       = FSP_INVALID_VECTOR,
    .tx_irq       = FSP_INVALID_VECTOR,
};

static canfd_instance_ctrl_t s_ctrl;
static bool s_open = false;

/* ---- public API ----------------------------------------------------- */

/* Recalculate bit timing prescaler for a given nominal baudrate (bps).
 * Assumes CANFDCLK = 40 MHz and fixed TSEG1=7, TSEG2=2. */
static void update_timing(uint32_t baudrate) {
    /* baud = CANFDCLK / (pre * (1 + TSEG1 + TSEG2)) */
    uint32_t tq_total = 1U + 7U + 2U; /* = 10 */
    uint32_t clk_hz   = 40000000U;    /* 40 MHz – update if CANFDCLK differs */
    uint32_t pre      = clk_hz / (baudrate * tq_total);
    if (pre < 1) { pre = 1; }
    if (pre > 1024) { pre = 1024; }
    s_timing_nominal.baud_rate_prescaler = (uint16_t)pre;
    s_timing_data.baud_rate_prescaler    = (uint16_t)pre;
}

int ra8p1_canfd_init(uint32_t baudrate) {
    if (s_open) { R_CANFD_Close(&s_ctrl); s_open = false; }
    update_timing(baudrate);
    fsp_err_t err = R_CANFD_Open(&s_ctrl, &s_cfg);
    if (err != FSP_SUCCESS) { return -1; }
    s_open = true;
    return 0;
}

void ra8p1_canfd_deinit(void) {
    if (s_open) { R_CANFD_Close(&s_ctrl); s_open = false; }
}

/* Send a CAN/CANFD frame.  Blocks until the TX buffer is free or timeout. */
int ra8p1_canfd_send(uint32_t id, bool ext, bool fd,
                     const uint8_t *data, uint8_t dlc, uint32_t timeout_ms) {
    if (!s_open) { return -1; }

    can_frame_t frame = {
        .id         = id,
        .id_mode    = ext ? CAN_ID_MODE_EXTENDED : CAN_ID_MODE_STANDARD,
        .type       = CAN_FRAME_TYPE_DATA,
        .data_length_code = dlc,
        .options    = fd ? CANFD_FRAME_OPTION_FD : 0,
    };
    if (dlc > 64) { dlc = 64; }
    memcpy(frame.data, data, dlc);

    fsp_err_t err = R_CANFD_Write(&s_ctrl, CANFD_TX_MB_0, &frame);
    if (err != FSP_SUCCESS) { return -1; }

    /* Poll TX-complete flag with timeout (rough 1 kHz loop iteration). */
    uint32_t retries = timeout_ms;
    while (retries--) {
        /* TX buffer 0 status: bit 0 of CFDTMSTS0 */
        if (!(R_CANFD->CFDTMSTS[0] & 0x01U)) { return 0; } /* sent */
        R_BSP_SoftwareDelay(1, BSP_DELAY_UNITS_MILLISECONDS);
    }
    return -1; /* timeout */
}

/* Receive from FIFO 0. Returns 0 on success, -1 if FIFO empty. */
int ra8p1_canfd_recv(uint32_t *id, bool *ext, bool *fd,
                     uint8_t *data, uint8_t *dlc, uint32_t timeout_ms) {
    if (!s_open) { return -1; }

    can_frame_t frame;
    uint32_t retries = timeout_ms + 1;
    while (retries--) {
        fsp_err_t err = R_CANFD_Read(&s_ctrl, CANFD_RX_FIFO_0, &frame);
        if (err == FSP_SUCCESS) {
            *id  = frame.id;
            *ext = (frame.id_mode == CAN_ID_MODE_EXTENDED);
            *fd  = (frame.options & CANFD_FRAME_OPTION_FD) != 0;
            *dlc = frame.data_length_code;
            if (*dlc > 64) { *dlc = 64; }
            memcpy(data, frame.data, *dlc);
            return 0;
        }
        if (timeout_ms == 0) { return -1; }
        R_BSP_SoftwareDelay(1, BSP_DELAY_UNITS_MILLISECONDS);
    }
    return -1;
}

bool ra8p1_canfd_is_open(void) { return s_open; }

#endif /* RA8P1 */
