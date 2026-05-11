/*
 * The MIT License (MIT)
 *
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

#include "hal_data.h"
#include "ra_config.h"
#include "ra_gpio.h"
#include "ra_int.h"
#include "ra_utils.h"
#include "ra_sci.h"
#include "instances/r_sci_b_uart.h"
#include "r_sci_uart.h"

#if !defined(RA_PRI_UART)
#define RA_PRI_UART (1)
#endif

#if defined(BSP_MCU_R7KA8P1KFLCAC)
#define RA8P1_SCIB_UART_CH (8U)
#define RA8P1_SCIB_UART_CCR1_SPB2_OFFSET (4U)
#define RA8P1_SCIB_UART_CCR1_SPB2_MASK   (0x00000030U)
#define RA8P1_SCIB_UART_CCR1_PARITY_OFFSET (8U)
#define RA8P1_SCIB_UART_CCR1_PARITY_MASK   (0x00000300U)
#define RA8P1_SCIB_UART_CCR3_CHAR_OFFSET   (8U)
#define RA8P1_SCIB_UART_CCR3_CHAR_MASK     (0x00000300U)
#define RA8P1_SCIB_UART_CCR3_CKE_OFFSET    (24U)
#define RA8P1_SCIB_UART_CCR3_CKE_MASK      (0x03000000U)
#define RA8P1_SCIB_UART_CCR2_DEFAULT_VALUE (0xFF00FF04U)
#define RA8P1_SCIB_UART_FCR_DEFAULT_VALUE  (0x1F1F0000U)
#define RA8P1_SCIB_UART_FCR_RESET_TX_RX    (0x00808000U)
#define RA8P1_SCIB_UART_CFCLR_CLEAR_ALL    (0x9D070010U)

typedef struct st_ra8p1_sci_b_baud_setting_t {
    union {
        uint32_t baudrate_bits;
        struct {
            uint32_t       : 3;
            uint32_t       : 1;
            uint32_t bgdm  : 1;
            uint32_t abcs  : 1;
            uint32_t abcse : 1;
            uint32_t       : 1;
            uint32_t brr   : 8;
            uint32_t brme  : 1;
            uint32_t       : 3;
            uint32_t cks   : 2;
            uint32_t       : 2;
            uint32_t mddr  : 8;
        } baudrate_bits_b;
    };
} ra_scib_baud_setting_t;

static inline bool ra_sci_use_scib(uint32_t ch) {
    return ch == RA8P1_SCIB_UART_CH;
}

static inline R_SCI_B0_Type *ra_scib_reg_ptr(uint32_t ch) {
    return ch == RA8P1_SCIB_UART_CH ? R_SCI_B8 : NULL;
}

static sci_b_uart_instance_ctrl_t ra8p1_scib8_ctrl;
static bool ra8p1_scib8_open;

typedef struct {
    uint8_t bgdm;
    uint8_t abcs;
    uint8_t abcse;
    uint8_t cks;
} ra8p1_scib_baud_const_t;

static bool ra_scib_calc_baud(uint32_t baudrate, ra_scib_baud_setting_t *p_baud_setting) {
    static const ra8p1_scib_baud_const_t g_async_baud[] = {
        {0U, 0U, 1U, 0U}, {1U, 1U, 0U, 0U}, {1U, 0U, 0U, 0U}, {0U, 0U, 1U, 1U},
        {0U, 0U, 0U, 0U}, {1U, 0U, 0U, 1U}, {0U, 0U, 1U, 2U}, {0U, 0U, 0U, 1U},
        {1U, 0U, 0U, 2U}, {0U, 0U, 1U, 3U}, {0U, 0U, 0U, 2U}, {1U, 0U, 0U, 3U},
        {0U, 0U, 0U, 3U},
    };
    static const uint16_t g_div_coefficient[] = {6U, 8U, 16U, 24U, 32U, 64U, 96U, 128U, 256U, 384U, 512U, 1024U, 2048U};
    const uint32_t freq_hz = 20000000U;
    int32_t hit_bit_err = 100000;
    uint8_t hit_mddr = 0U;
    p_baud_setting->baudrate_bits = 0U;
    p_baud_setting->baudrate_bits_b.brr = 255U;
    p_baud_setting->baudrate_bits_b.mddr = 128U;
    for (uint32_t select_16 = 0; select_16 <= 1U && hit_bit_err > 5000; ++select_16) {
        for (uint32_t i = 0; i < 13U; ++i) {
            if (((uint8_t)select_16) ^ (g_async_baud[i].abcs | g_async_baud[i].abcse)) {
                continue;
            }
            uint32_t divisor = (uint32_t)g_div_coefficient[i] * baudrate;
            uint32_t temp_brr = freq_hz / divisor;
            if (temp_brr <= 256U) {
                while (temp_brr > 0U) {
                    temp_brr -= 1U;
                    int32_t err_divisor = (int32_t)(divisor * (temp_brr + 1U));
                    int32_t bit_err = (int32_t)((((int64_t)freq_hz) * 100000) / err_divisor - 100000);
                    uint8_t mddr = (uint8_t)((uint32_t)err_divisor / (freq_hz / 256U));
                    if (mddr < 128U) {
                        break;
                    }
                    bit_err = (((bit_err + 100000) * (int32_t)mddr) / 256) - 100000;
                    if (bit_err < 0) {
                        bit_err = -bit_err;
                    }
                    if (bit_err < hit_bit_err) {
                        p_baud_setting->baudrate_bits_b.bgdm = g_async_baud[i].bgdm;
                        p_baud_setting->baudrate_bits_b.abcs = g_async_baud[i].abcs;
                        p_baud_setting->baudrate_bits_b.abcse = g_async_baud[i].abcse;
                        p_baud_setting->baudrate_bits_b.cks = g_async_baud[i].cks;
                        p_baud_setting->baudrate_bits_b.brr = (uint8_t)temp_brr;
                        hit_bit_err = bit_err;
                        hit_mddr = mddr;
                        p_baud_setting->baudrate_bits_b.brme = 1U;
                        p_baud_setting->baudrate_bits_b.mddr = hit_mddr;
                    }
                }
            }
        }
    }
    return hit_bit_err <= 5000;
}

static void ra_scib_init_uart(uint32_t ch, uint32_t baud, uint32_t bits, uint32_t parity, uint32_t stop, uint32_t flow) {
    (void)flow;
#if defined(BSP_MCU_R7KA8P1KFLCAC)
  #if BSP_FEATURE_TZ_VERSION == 2 && BSP_TZ_NONSECURE_BUILD == 1
    R_SYSTEM->PRCR_NS = (uint16_t) 0xA503U;
  #else
    R_SYSTEM->PRCR = (uint16_t) 0xA503U;
  #endif
    R_SYSTEM->SCICKCR |= (uint8_t) 0x40U;
    FSP_HARDWARE_REGISTER_WAIT((uint8_t) ((R_SYSTEM->SCICKCR & 0x80U) >>
                                          7U),
                               1U);
    R_SYSTEM->SCICKDIVCR = BSP_CFG_SCICLK_DIV;
    R_SYSTEM->SCICKCR = (uint8_t) (BSP_CFG_SCICLK_SOURCE | 0x40U |
                                   0x80U);
    R_SYSTEM->SCICKCR &= (uint8_t) ~0x40U;
    FSP_HARDWARE_REGISTER_WAIT((uint8_t) ((R_SYSTEM->SCICKCR & 0x80U) >>
                                          7U),
                               0U);
  #if BSP_FEATURE_TZ_VERSION == 2 && BSP_TZ_NONSECURE_BUILD == 1
    R_SYSTEM->PRCR_NS = (uint16_t) 0xA500U;
  #else
    R_SYSTEM->PRCR = (uint16_t) 0xA500U;
  #endif
#endif
    R_SCI_B0_Type *reg = ra_scib_reg_ptr(ch);
    ra_scib_baud_setting_t baud_setting = {0};
    if (reg == NULL) {
        return;
    }
    (void)ra_scib_calc_baud(baud, &baud_setting);
    if (ch == 8) {
        sci_b_uart_instance_ctrl_t *ctrl = &ra8p1_scib8_ctrl;
        static sci_b_uart_extended_cfg_t ext;
        static sci_b_baud_setting_t fsp_baud_setting;
        static uart_cfg_t cfg;
        memset(ctrl, 0, sizeof(*ctrl));
        memset(&ext, 0, sizeof(ext));
        memset(&cfg, 0, sizeof(cfg));
        ext.clock = SCI_B_UART_CLOCK_INT;
        ext.rx_edge_start = SCI_B_UART_START_BIT_FALLING_EDGE;
        ext.noise_cancel = SCI_B_UART_NOISE_CANCELLATION_DISABLE;
        memset(&fsp_baud_setting, 0, sizeof(fsp_baud_setting));
        if (R_SCI_B_UART_BaudCalculate(baud, false, 5000U, &fsp_baud_setting) != FSP_SUCCESS) {
            fsp_baud_setting.baudrate_bits = baud_setting.baudrate_bits;
        }
        ext.p_baud_setting = &fsp_baud_setting;
        ext.rx_fifo_trigger = SCI_B_UART_RX_FIFO_TRIGGER_MAX;
        ext.flow_control_pin = (bsp_io_port_pin_t) 0xFFFFu;
        ext.flow_control = SCI_B_UART_FLOW_CONTROL_RTS;
        ext.rs485_setting.enable = SCI_B_UART_RS485_DISABLE;
        cfg.channel = 8;
        cfg.data_bits = bits == 7 ? UART_DATA_BITS_7 : (bits == 9 ? UART_DATA_BITS_9 : UART_DATA_BITS_8);
        cfg.parity = parity == 2 ? UART_PARITY_EVEN : (parity ? UART_PARITY_ODD : UART_PARITY_OFF);
        cfg.stop_bits = stop == 2 ? UART_STOP_BITS_2 : UART_STOP_BITS_1;
#if defined(VECTOR_NUMBER_SCI8_RXI)
        cfg.rxi_irq = VECTOR_NUMBER_SCI8_RXI;
        cfg.rxi_ipl = 12;
#endif
#if defined(VECTOR_NUMBER_SCI8_TXI)
        cfg.txi_irq = VECTOR_NUMBER_SCI8_TXI;
        cfg.txi_ipl = 12;
#endif
#if defined(VECTOR_NUMBER_SCI8_TEI)
        cfg.tei_irq = VECTOR_NUMBER_SCI8_TEI;
        cfg.tei_ipl = 12;
#endif
#if defined(VECTOR_NUMBER_SCI8_ERI)
        cfg.eri_irq = VECTOR_NUMBER_SCI8_ERI;
        cfg.eri_ipl = 12;
#endif
        cfg.p_extend = &ext;
        ra8p1_scib8_open = (FSP_SUCCESS == R_SCI_B_UART_Open((uart_ctrl_t *) ctrl, &cfg));
        if (ra8p1_scib8_open) {
            ra8p1_scib8_open = (FSP_SUCCESS == R_SCI_B_UART_BaudSet((uart_ctrl_t *) ctrl, &fsp_baud_setting));
        }
        return;
    }
    uint32_t ccr0 = R_SCI_B0_CCR0_IDSEL_Msk;
    reg->CCR0 = ccr0;
    uint32_t ccr3 = (uint32_t)R_SCI_B0_CCR3_LSBF_Msk;
    if (bits == 7) {
        ccr3 |= (1U << RA8P1_SCIB_UART_CCR3_CHAR_OFFSET) & RA8P1_SCIB_UART_CCR3_CHAR_MASK;
    } else if (bits == 9) {
        ccr3 |= (2U << RA8P1_SCIB_UART_CCR3_CHAR_OFFSET) & RA8P1_SCIB_UART_CCR3_CHAR_MASK;
    }
    if (stop == 2) {
        ccr3 |= R_SCI_B0_CCR3_STP_Msk;
    }
    ccr3 |= (0U << RA8P1_SCIB_UART_CCR3_CKE_OFFSET) & RA8P1_SCIB_UART_CCR3_CKE_MASK;
    reg->CCR3 = ccr3;
    reg->CCR2 = baud_setting.baudrate_bits ? baud_setting.baudrate_bits : RA8P1_SCIB_UART_CCR2_DEFAULT_VALUE;
    uint32_t ccr1 = (3U << RA8P1_SCIB_UART_CCR1_SPB2_OFFSET) & RA8P1_SCIB_UART_CCR1_SPB2_MASK;
    if (parity != 0) {
        ccr1 |= (((parity == 2) ? 1U : 3U) << RA8P1_SCIB_UART_CCR1_PARITY_OFFSET) & RA8P1_SCIB_UART_CCR1_PARITY_MASK;
    }
    reg->CCR1 = ccr1;
    reg->CCR4 = 0U;
    reg->FCR = RA8P1_SCIB_UART_FCR_RESET_TX_RX;
    reg->FCR = RA8P1_SCIB_UART_FCR_DEFAULT_VALUE;
    reg->DCR = 0U;
    reg->CFCLR = RA8P1_SCIB_UART_CFCLR_CLEAR_ALL;
    reg->FFCLR = 1U;
#if defined(VECTOR_NUMBER_SCI8_RXI)
    if (ch == 8) {
        R_BSP_IrqEnable(VECTOR_NUMBER_SCI8_RXI);
    }
#endif
#if defined(VECTOR_NUMBER_SCI8_ERI)
    if (ch == 8) {
        R_BSP_IrqEnable(VECTOR_NUMBER_SCI8_ERI);
    }
#endif
#if defined(VECTOR_NUMBER_SCI8_TXI)
    if (ch == 8) {
        R_BSP_IrqEnable(VECTOR_NUMBER_SCI8_TXI);
    }
#endif
#if defined(VECTOR_NUMBER_SCI8_TEI)
    if (ch == 8) {
        R_BSP_IrqEnable(VECTOR_NUMBER_SCI8_TEI);
    }
#endif
    ccr0 |= R_SCI_B0_CCR0_RE_Msk | R_SCI_B0_CCR0_RIE_Msk | R_SCI_B0_CCR0_TE_Msk;
    reg->CCR0 = ccr0;
    while (reg->CESR_b.RIST != 1U) {
    }
}
#endif

#if defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wunused-parameter"
// #pragma GCC diagnostic ignored "-Wconversion"
// #pragma GCC diagnostic ignored "-Wshift-negative-value"
// #pragma GCC diagnostic ignored "-Wunused-but-set-variable"
// #pragma GCC diagnostic ignored "-Wsequence-point"
// #pragma GCC diagnostic ignored "-Wunused-function"
#endif

enum
{
    #if defined(VECTOR_NUMBER_SCI0_RXI)
    SCI0_IDX,
    #endif
    #if defined(VECTOR_NUMBER_SCI1_RXI)
    SCI1_IDX,
    #endif
    #if defined(VECTOR_NUMBER_SCI2_RXI)
    SCI2_IDX,
    #endif
    #if defined(VECTOR_NUMBER_SCI3_RXI)
    SCI3_IDX,
    #endif
    #if defined(VECTOR_NUMBER_SCI4_RXI)
    SCI4_IDX,
    #endif
    #if defined(VECTOR_NUMBER_SCI5_RXI)
    SCI5_IDX,
    #endif
    #if defined(VECTOR_NUMBER_SCI6_RXI)
    SCI6_IDX,
    #endif
    #if defined(VECTOR_NUMBER_SCI7_RXI)
    SCI7_IDX,
    #endif
    #if defined(VECTOR_NUMBER_SCI8_RXI)
    SCI8_IDX,
    #endif
    #if defined(VECTOR_NUMBER_SCI9_RXI)
    SCI9_IDX,
    #endif
    SCI_IDX_MAX,
};

#if defined(BSP_MCU_R7KA8P1KFLCAC) && !defined(VECTOR_NUMBER_SCI6_RXI)
#define RA8P1_SCI6_FALLBACK_SLOT (SCI_IDX_MAX)
#define RA_SCI_SLOT_MAX (SCI_IDX_MAX + 1)
#else
#define RA_SCI_SLOT_MAX (SCI_IDX_MAX)
#endif

static const IRQn_Type idx_to_rxirq[] = {
    #if defined(VECTOR_NUMBER_SCI0_RXI)
    VECTOR_NUMBER_SCI0_RXI,
    #endif
    #if defined(VECTOR_NUMBER_SCI1_RXI)
    VECTOR_NUMBER_SCI1_RXI,
    #endif
    #if defined(VECTOR_NUMBER_SCI2_RXI)
    VECTOR_NUMBER_SCI2_RXI,
    #endif
    #if defined(VECTOR_NUMBER_SCI3_RXI)
    VECTOR_NUMBER_SCI3_RXI,
    #endif
    #if defined(VECTOR_NUMBER_SCI4_RXI)
    VECTOR_NUMBER_SCI4_RXI,
    #endif
    #if defined(VECTOR_NUMBER_SCI5_RXI)
    VECTOR_NUMBER_SCI5_RXI,
    #endif
    #if defined(VECTOR_NUMBER_SCI6_RXI)
    VECTOR_NUMBER_SCI6_RXI,
    #endif
    #if defined(VECTOR_NUMBER_SCI7_RXI)
    VECTOR_NUMBER_SCI7_RXI,
    #endif
    #if defined(VECTOR_NUMBER_SCI8_RXI)
    VECTOR_NUMBER_SCI8_RXI,
    #endif
    #if defined(VECTOR_NUMBER_SCI9_RXI)
    VECTOR_NUMBER_SCI9_RXI,
    #endif
    #if defined(BSP_MCU_R7KA8P1KFLCAC) && !defined(VECTOR_NUMBER_SCI6_RXI)
    FSP_INVALID_VECTOR,
    #endif
};

static const IRQn_Type idx_to_txirq[] = {
    #if defined(VECTOR_NUMBER_SCI0_TXI)
    VECTOR_NUMBER_SCI0_TXI,
    #endif
    #if defined(VECTOR_NUMBER_SCI1_TXI)
    VECTOR_NUMBER_SCI1_TXI,
    #endif
    #if defined(VECTOR_NUMBER_SCI2_TXI)
    VECTOR_NUMBER_SCI2_TXI,
    #endif
    #if defined(VECTOR_NUMBER_SCI3_TXI)
    VECTOR_NUMBER_SCI3_TXI,
    #endif
    #if defined(VECTOR_NUMBER_SCI4_TXI)
    VECTOR_NUMBER_SCI4_TXI,
    #endif
    #if defined(VECTOR_NUMBER_SCI5_TXI)
    VECTOR_NUMBER_SCI5_TXI,
    #endif
    #if defined(VECTOR_NUMBER_SCI6_TXI)
    VECTOR_NUMBER_SCI6_TXI,
    #endif
    #if defined(VECTOR_NUMBER_SCI7_TXI)
    VECTOR_NUMBER_SCI7_TXI,
    #endif
    #if defined(VECTOR_NUMBER_SCI8_TXI)
    VECTOR_NUMBER_SCI8_TXI,
    #endif
    #if defined(VECTOR_NUMBER_SCI9_TXI)
    VECTOR_NUMBER_SCI9_TXI,
    #endif
    #if defined(BSP_MCU_R7KA8P1KFLCAC) && !defined(VECTOR_NUMBER_SCI6_RXI)
    FSP_INVALID_VECTOR,
    #endif
};

static const IRQn_Type idx_to_teirq[] = {
    #if defined(VECTOR_NUMBER_SCI0_TEI)
    VECTOR_NUMBER_SCI0_TEI,
    #endif
    #if defined(VECTOR_NUMBER_SCI1_TEI)
    VECTOR_NUMBER_SCI1_TEI,
    #endif
    #if defined(VECTOR_NUMBER_SCI2_TEI)
    VECTOR_NUMBER_SCI2_TEI,
    #endif
    #if defined(VECTOR_NUMBER_SCI3_TEI)
    VECTOR_NUMBER_SCI3_TEI,
    #endif
    #if defined(VECTOR_NUMBER_SCI4_TEI)
    VECTOR_NUMBER_SCI4_TEI,
    #endif
    #if defined(VECTOR_NUMBER_SCI5_TEI)
    VECTOR_NUMBER_SCI5_TEI,
    #endif
    #if defined(VECTOR_NUMBER_SCI6_TEI)
    VECTOR_NUMBER_SCI6_TEI,
    #endif
    #if defined(VECTOR_NUMBER_SCI7_TEI)
    VECTOR_NUMBER_SCI7_TEI,
    #endif
    #if defined(VECTOR_NUMBER_SCI8_TEI)
    VECTOR_NUMBER_SCI8_TEI,
    #endif
    #if defined(VECTOR_NUMBER_SCI9_TEI)
    VECTOR_NUMBER_SCI9_TEI,
    #endif
    #if defined(BSP_MCU_R7KA8P1KFLCAC) && !defined(VECTOR_NUMBER_SCI6_RXI)
    FSP_INVALID_VECTOR,
    #endif
};

static const IRQn_Type idx_to_erirq[] = {
    #if defined(VECTOR_NUMBER_SCI0_ERI)
    VECTOR_NUMBER_SCI0_ERI,
    #endif
    #if defined(VECTOR_NUMBER_SCI1_ERI)
    VECTOR_NUMBER_SCI1_ERI,
    #endif
    #if defined(VECTOR_NUMBER_SCI2_ERI)
    VECTOR_NUMBER_SCI2_ERI,
    #endif
    #if defined(VECTOR_NUMBER_SCI3_ERI)
    VECTOR_NUMBER_SCI3_ERI,
    #endif
    #if defined(VECTOR_NUMBER_SCI4_ERI)
    VECTOR_NUMBER_SCI4_ERI,
    #endif
    #if defined(VECTOR_NUMBER_SCI5_ERI)
    VECTOR_NUMBER_SCI5_ERI,
    #endif
    #if defined(VECTOR_NUMBER_SCI6_ERI)
    VECTOR_NUMBER_SCI6_ERI,
    #endif
    #if defined(VECTOR_NUMBER_SCI7_ERI)
    VECTOR_NUMBER_SCI7_ERI,
    #endif
    #if defined(VECTOR_NUMBER_SCI8_ERI)
    VECTOR_NUMBER_SCI8_ERI,
    #endif
    #if defined(VECTOR_NUMBER_SCI9_ERI)
    VECTOR_NUMBER_SCI9_ERI,
    #endif
    #if defined(BSP_MCU_R7KA8P1KFLCAC) && !defined(VECTOR_NUMBER_SCI6_RXI)
    FSP_INVALID_VECTOR,
    #endif
};

static uint32_t ch_to_idx[SCI_CH_MAX] = {
    #if defined(VECTOR_NUMBER_SCI0_RXI)
    SCI0_IDX,
    #else
    0,
    #endif
    #if defined(VECTOR_NUMBER_SCI1_RXI)
    SCI1_IDX,
    #else
    0,
    #endif
    #if defined(VECTOR_NUMBER_SCI2_RXI)
    SCI2_IDX,
    #else
    0,
    #endif
    #if defined(VECTOR_NUMBER_SCI3_RXI)
    SCI3_IDX,
    #else
    0,
    #endif
    #if defined(VECTOR_NUMBER_SCI4_RXI)
    SCI4_IDX,
    #else
    0,
    #endif
    #if defined(VECTOR_NUMBER_SCI5_RXI)
    SCI5_IDX,
    #else
    0,
    #endif
    #if defined(VECTOR_NUMBER_SCI6_RXI)
    SCI6_IDX,
    #elif defined(BSP_MCU_R7KA8P1KFLCAC)
    RA8P1_SCI6_FALLBACK_SLOT,
    #else
    0,
    #endif
    #if defined(VECTOR_NUMBER_SCI7_RXI)
    SCI7_IDX,
    #else
    0,
    #endif
    #if defined(VECTOR_NUMBER_SCI8_RXI)
    SCI8_IDX,
    #else
    0,
    #endif
    #if defined(VECTOR_NUMBER_SCI9_RXI)
    SCI9_IDX,
    #else
    0,
    #endif
};

static R_SCI0_Type *sci_regs[] = {
    #if defined(VECTOR_NUMBER_SCI0_RXI)
    R_SCI0,
    #endif
    #if defined(VECTOR_NUMBER_SCI1_RXI)
    R_SCI1,
    #endif
    #if defined(VECTOR_NUMBER_SCI2_RXI)
    R_SCI2,
    #endif
    #if defined(VECTOR_NUMBER_SCI3_RXI)
    R_SCI3,
    #endif
    #if defined(VECTOR_NUMBER_SCI4_RXI)
    R_SCI4,
    #endif
    #if defined(VECTOR_NUMBER_SCI5_RXI)
    R_SCI5,
    #endif
    #if defined(VECTOR_NUMBER_SCI6_RXI)
    R_SCI6,
    #endif
    #if defined(VECTOR_NUMBER_SCI7_RXI)
    R_SCI7,
    #endif
    #if defined(VECTOR_NUMBER_SCI8_RXI)
    R_SCI8,
    #endif
    #if defined(VECTOR_NUMBER_SCI9_RXI)
    R_SCI9,
    #endif
    #if defined(BSP_MCU_R7KA8P1KFLCAC) && !defined(VECTOR_NUMBER_SCI6_RXI)
    R_SCI6,
    #endif
};

static uint32_t sci_module_mask[] = {
    #if defined(VECTOR_NUMBER_SCI0_RXI)
    R_MSTP_MSTPCRB_MSTPB31_Msk,
    #endif
    #if defined(VECTOR_NUMBER_SCI1_RXI)
    R_MSTP_MSTPCRB_MSTPB30_Msk,
    #endif
    #if defined(VECTOR_NUMBER_SCI2_RXI)
    R_MSTP_MSTPCRB_MSTPB29_Msk,
    #endif
    #if defined(VECTOR_NUMBER_SCI3_RXI)
    R_MSTP_MSTPCRB_MSTPB28_Msk,
    #endif
    #if defined(VECTOR_NUMBER_SCI4_RXI)
    R_MSTP_MSTPCRB_MSTPB27_Msk,
    #endif
    #if defined(VECTOR_NUMBER_SCI5_RXI)
    R_MSTP_MSTPCRB_MSTPB26_Msk,
    #endif
    #if defined(VECTOR_NUMBER_SCI6_RXI)
    R_MSTP_MSTPCRB_MSTPB25_Msk,
    #endif
    #if defined(VECTOR_NUMBER_SCI7_RXI)
    R_MSTP_MSTPCRB_MSTPB24_Msk,
    #endif
    #if defined(VECTOR_NUMBER_SCI8_RXI)
    R_MSTP_MSTPCRB_MSTPB23_Msk,
    #endif
    #if defined(VECTOR_NUMBER_SCI9_RXI)
    R_MSTP_MSTPCRB_MSTPB22_Msk,
    #endif
    #if defined(BSP_MCU_R7KA8P1KFLCAC) && !defined(VECTOR_NUMBER_SCI6_RXI)
    R_MSTP_MSTPCRB_MSTPB25_Msk,
    #endif
};

static const ra_af_pin_t ra_sci_tx_pins[] = {
    #if defined(RA4M1)

    { AF_SCI1, 0, P101 },
    { AF_SCI1, 0, P205 },
    { AF_SCI1, 0, P411 },

    { AF_SCI2, 1, P213 },
    { AF_SCI2, 1, P401 },
    { AF_SCI2, 1, P501 },

    { AF_SCI2, 2, P102 },
    { AF_SCI1, 2, P112 },
    { AF_SCI1, 2, P302 },

    { AF_SCI2, 9, P109 },
    { AF_SCI2, 9, P203 },
    { AF_SCI2, 9, P409 },
    { AF_SCI2, 9, P602 },

    #elif defined(RA4W1)
    { AF_SCI1, 0, P101 },

    { AF_SCI2, 1, P213 },

    { AF_SCI1, 4, P205 },

    { AF_SCI2, 9, P109 },

    #elif defined(RA6M1)

    { AF_SCI1, 0, P101 },
    { AF_SCI1, 0, P411 },

    { AF_SCI2, 1, P213 },

    { AF_SCI1, 2, P112 },
    { AF_SCI1, 2, P302 },

    { AF_SCI2, 3, P409 },

    { AF_SCI1, 4, P205 },

    { AF_SCI1, 8, P105 },

    { AF_SCI2, 9, P109 },
    { AF_SCI2, 9, P602 },

    #elif defined(RA6M2)

    { AF_SCI1, 0, P101 },
    { AF_SCI1, 0, P411 },

    { AF_SCI1, 1, P213 },
    { AF_SCI2, 1, P709 },

    { AF_SCI1, 2, P112 },
    { AF_SCI1, 2, P302 },

    { AF_SCI2, 3, P310 },
    { AF_SCI2, 3, P409 },

    { AF_SCI1, 4, P205 },
    { AF_SCI1, 4, P512 },

    { AF_SCI2, 5, P501 },

    { AF_SCI1, 6, P305 },
    { AF_SCI1, 6, P506 },

    { AF_SCI2, 7, P401 },
    { AF_SCI2, 7, P613 },

    { AF_SCI1, 8, P105 },

    { AF_SCI2, 9, P109 },
    { AF_SCI2, 9, P203 },
    { AF_SCI2, 9, P602 },

    #elif defined(RA6M3)

    { AF_SCI1, 0, P101 },
    { AF_SCI1, 0, P411 },

    { AF_SCI2, 1, P213 },
    { AF_SCI2, 1, P709 },

    { AF_SCI1, 2, P112 },
    { AF_SCI1, 2, P302 },

    { AF_SCI2, 3, P310 },
    { AF_SCI2, 3, P409 },
    { AF_SCI2, 3, P707 },

    { AF_SCI1, 4, P205 },
    { AF_SCI1, 4, P512 },
    { AF_SCI1, 4, P900 },

    { AF_SCI2, 5, P501 },
    { AF_SCI2, 5, P805 },

    { AF_SCI1, 6, P305 },
    { AF_SCI1, 6, P506 },

    { AF_SCI2, 7, P401 },
    { AF_SCI2, 7, P613 },

    { AF_SCI1, 8, P105 },
    { AF_SCI1, 8, PA00 },

    { AF_SCI2, 9, P109 },
    { AF_SCI2, 9, P203 },
    { AF_SCI2, 9, P602 },

    #elif defined(RA6M5)

    { AF_SCI1, 0, P101 },
    { AF_SCI1, 0, P411 },

    { AF_SCI2, 1, P213 },
    { AF_SCI2, 1, P709 },

    { AF_SCI1, 2, P112 },
    { AF_SCI1, 2, P302 },

    { AF_SCI2, 3, P310 },
    { AF_SCI2, 3, P409 },
    { AF_SCI2, 3, P707 },

    { AF_SCI1, 4, P205 },
    { AF_SCI1, 4, P207 },
    { AF_SCI1, 4, P512 },
    { AF_SCI1, 4, P900 },

    { AF_SCI2, 5, P501 },
    { AF_SCI2, 5, P805 },

    { AF_SCI1, 6, P305 },
    { AF_SCI1, 6, P506 },

    { AF_SCI2, 7, P401 },
    { AF_SCI2, 7, P613 },

    { AF_SCI1, 8, P105 },
    { AF_SCI1, 8, PA00 },

    { AF_SCI2, 9, P109 },
    { AF_SCI2, 9, P203 },
    { AF_SCI2, 9, P602 },

    #elif defined(RA8P1)

    { AF_SCI1, 0, P101 },
    { AF_SCI1, 0, P411 },

    { AF_SCI2, 1, P213 },
    { AF_SCI2, 1, P709 },

    { AF_SCI1, 2, P112 },

    { AF_SCI2, 3, P310 },
    { AF_SCI2, 3, P409 },
    { AF_SCI2, 3, P707 },

    { AF_SCI1, 4, P205 },
    { AF_SCI1, 4, P207 },
    { AF_SCI1, 4, P512 },
    { AF_SCI1, 4, P900 },

    { AF_SCI2, 5, P501 },
    { AF_SCI2, 5, P805 },

    { AF_SCI1, 6, P302 },
    { AF_SCI1, 6, P305 },
    { AF_SCI1, 6, P506 },

    { AF_SCI2, 7, P401 },
    { AF_SCI2, 7, P613 },

    { AF_SCI1, 8, P105 },
    { AF_SCI1, 8, PA00 },
    { AF_SCI1, 8, PD02 },

    { AF_SCI2, 9, P109 },
    { AF_SCI2, 9, P203 },
    { AF_SCI2, 9, P602 },

    #else
    #error "CMSIS MCU Series is not specified."
    #endif
};
#define SCI_TX_PINS_SIZE sizeof(ra_sci_tx_pins) / sizeof(ra_af_pin_t)

static const ra_af_pin_t ra_sci_rx_pins[] = {
    #if defined(RA4M1)

    { AF_SCI1, 0, P100 },
    { AF_SCI1, 0, P104 },
    { AF_SCI1, 0, P206 },
    { AF_SCI1, 0, P410 },

    { AF_SCI2, 1, P212 },
    { AF_SCI2, 1, P402 },
    { AF_SCI2, 1, P502 },
    { AF_SCI2, 1, P708 },

    { AF_SCI1, 2, P301 },

    { AF_SCI2, 9, P110 },
    { AF_SCI2, 9, P202 },
    { AF_SCI2, 9, P408 },
    { AF_SCI2, 9, P601 },

    #elif defined(RA4W1)
    { AF_SCI1, 0, P100 },
    { AF_SCI1, 0, P104 },

    { AF_SCI2, 1, P212 },
    { AF_SCI2, 1, P402 },

    { AF_SCI1, 4, P206 },

    { AF_SCI2, 9, P110 },

    #elif defined(RA6M1)

    { AF_SCI1, 0, P100 },
    { AF_SCI1, 0, P410 },

    { AF_SCI2, 1, P212 },
    { AF_SCI2, 1, P708 },

    { AF_SCI1, 2, P113 },
    { AF_SCI1, 2, P301 },

    { AF_SCI2, 3, P408 },

    { AF_SCI1, 4, P206 },

    { AF_SCI1, 8, P104 },

    { AF_SCI2, 9, P110 },
    { AF_SCI2, 9, P601 },

    #elif defined(RA6M2)

    { AF_SCI1, 0, P100 },
    { AF_SCI1, 0, P410 },

    { AF_SCI1, 1, P212 },
    { AF_SCI2, 1, P708 },

    { AF_SCI1, 2, P113 },
    { AF_SCI1, 2, P301 },

    { AF_SCI2, 3, P309 },
    { AF_SCI2, 3, P408 },

    { AF_SCI1, 4, P206 },
    { AF_SCI1, 4, P511 },

    { AF_SCI2, 5, P502 },

    { AF_SCI1, 6, P304 },
    { AF_SCI1, 6, P505 },

    { AF_SCI2, 7, P402 },
    { AF_SCI2, 7, P614 },

    { AF_SCI1, 8, P104 },

    { AF_SCI2, 9, P110 },
    { AF_SCI2, 9, P202 },
    { AF_SCI2, 9, P601 },

    #elif defined(RA6M3) || defined(RA6M5)

    { AF_SCI1, 0, P100 },
    { AF_SCI1, 0, P410 },

    { AF_SCI2, 1, P212 },
    { AF_SCI2, 1, P708 },

    { AF_SCI1, 2, P113 },
    { AF_SCI1, 2, P301 },

    { AF_SCI2, 3, P309 },
    { AF_SCI2, 3, P408 },
    { AF_SCI2, 3, P706 },

    { AF_SCI1, 4, P206 },
    { AF_SCI1, 4, P315 },
    { AF_SCI1, 4, P511 },

    { AF_SCI2, 5, P502 },
    { AF_SCI2, 5, P513 },

    { AF_SCI1, 6, P304 },
    { AF_SCI1, 6, P505 },

    { AF_SCI2, 7, P402 },
    { AF_SCI2, 7, P614 },

    { AF_SCI1, 8, P104 },
    { AF_SCI1, 8, P607 },
    { AF_SCI1, 8, PD03 },

    { AF_SCI2, 9, P110 },
    { AF_SCI2, 9, P202 },
    { AF_SCI2, 9, P601 },

    #elif defined(RA8P1)

    { AF_SCI1, 0, P100 },
    { AF_SCI1, 0, P410 },

    { AF_SCI2, 1, P212 },
    { AF_SCI2, 1, P708 },

    { AF_SCI1, 2, P113 },

    { AF_SCI2, 3, P309 },
    { AF_SCI2, 3, P408 },
    { AF_SCI2, 3, P706 },

    { AF_SCI1, 4, P206 },
    { AF_SCI1, 4, P315 },
    { AF_SCI1, 4, P511 },

    { AF_SCI2, 5, P502 },
    { AF_SCI2, 5, P513 },

    { AF_SCI1, 6, P301 },
    { AF_SCI1, 6, P304 },
    { AF_SCI1, 6, P505 },

    { AF_SCI2, 7, P402 },
    { AF_SCI2, 7, P614 },

    { AF_SCI1, 8, P104 },
    { AF_SCI1, 8, P607 },

    { AF_SCI2, 9, P110 },
    { AF_SCI2, 9, P202 },
    { AF_SCI2, 9, P601 },

    #else
    #error "CMSIS MCU Series is not specified."
    #endif
};
#define SCI_RX_PINS_SIZE sizeof(ra_sci_rx_pins) / sizeof(ra_af_pin_t)

static const ra_af_pin_t ra_sci_cts_pins[] = {
    #if defined(RA4M1)

    { AF_SCI1, 0, P103 },
    { AF_SCI1, 0, P401 },
    { AF_SCI1, 0, P407 },
    { AF_SCI1, 0, P413 },

    { AF_SCI2, 1, P101 },
    { AF_SCI2, 1, P403 },
    { AF_SCI1, 1, P408 },
    { AF_SCI2, 1, P504 },

    { AF_SCI1, 2, P110 },
    { AF_SCI1, 2, P203 },

    { AF_SCI2, 9, P108 },
    { AF_SCI2, 9, P205 },
    { AF_SCI2, 9, P301 },
    { AF_SCI2, 9, P603 },

    #elif defined(RA4W1)
    { AF_SCI1, 0, P103 },

    { AF_SCI2, 1, P101 },

    { AF_SCI1, 4, P407 },

    { AF_SCI2, 9, P108 },
    { AF_SCI2, 9, P205 },

    #elif defined(RA6M1)

    { AF_SCI1, 0, P103 },
    { AF_SCI1, 0, P413 },

    { AF_SCI2, 1, P101 },

    { AF_SCI1, 2, P110 },

    { AF_SCI2, 3, P411 },

    { AF_SCI1, 4, P401 },
    { AF_SCI1, 4, P407 },

    { AF_SCI1, 8, P107 },

    { AF_SCI2, 9, P108 },
    { AF_SCI2, 9, P301 },

    #elif defined(RA6M2)

    { AF_SCI1, 0, P103 },
    { AF_SCI1, 0, P413 },

    { AF_SCI2, 1, P101 },
    { AF_SCI2, 1, P711 },

    { AF_SCI1, 2, P110 },
    { AF_SCI1, 2, P203 },

    { AF_SCI2, 3, P312 },
    { AF_SCI2, 3, P411 },

    { AF_SCI1, 4, P401 },
    { AF_SCI1, 4, P407 },

    { AF_SCI2, 5, P504 },

    { AF_SCI1, 6, P307 },
    { AF_SCI1, 6, P503 },

    { AF_SCI2, 7, P403 },
    { AF_SCI2, 7, P611 },

    { AF_SCI1, 8, P107 },

    { AF_SCI2, 9, P108 },
    { AF_SCI2, 9, P205 },
    { AF_SCI2, 9, P301 },
    { AF_SCI2, 9, P603 },

    #elif defined(RA6M3)

    { AF_SCI1, 0, P103 },
    { AF_SCI1, 0, P413 },

    { AF_SCI2, 1, P101 },
    { AF_SCI2, 1, P711 },

    { AF_SCI1, 2, P110 },
    { AF_SCI1, 2, P203 },

    { AF_SCI2, 3, P312 },
    { AF_SCI2, 3, P411 },
    { AF_SCI2, 3, PB01 },

    { AF_SCI1, 4, P401 },
    { AF_SCI1, 4, P407 },

    { AF_SCI2, 5, P504 },
    { AF_SCI2, 5, P507 },

    { AF_SCI1, 6, P307 },
    { AF_SCI1, 6, P503 },

    { AF_SCI2, 7, P403 },
    { AF_SCI2, 7, P611 },

    { AF_SCI1, 8, P107 },
    { AF_SCI1, 8, P606 },

    { AF_SCI2, 9, P108 },
    { AF_SCI2, 9, P205 },
    { AF_SCI2, 9, P301 },
    { AF_SCI2, 9, P603 },

    #elif defined(RA6M5) || defined(RA8P1)

    { AF_SCI1, 0, P103 },
    { AF_SCI1, 0, P413 },
    { AF_SCI1, 0, P414 }, /* CTS only */
    { AF_SCI1, 0, P800 }, /* CTS only */

    { AF_SCI2, 1, P101 },
    { AF_SCI2, 1, P711 },

    { AF_SCI1, 2, P110 },
    { AF_SCI1, 2, P203 },

    { AF_SCI2, 3, P308 }, /* CTS only */
    { AF_SCI2, 3, P312 },
    { AF_SCI2, 3, P411 },
    { AF_SCI2, 3, P412 }, /* CTS only */
    { AF_SCI2, 3, P705 }, /* CTS only */

    { AF_SCI1, 4, P401 },
    { AF_SCI1, 4, P402 }, /* CTS only */
    { AF_SCI1, 4, P407 },
    { AF_SCI1, 4, P408 }, /* CTS only */

    { AF_SCI2, 5, P500 }, /* CTS only */
    { AF_SCI2, 5, P504 },
    { AF_SCI2, 5, P508 },

    { AF_SCI1, 6, P307 },
    { AF_SCI1, 6, P308 }, /* CTS only */
    { AF_SCI1, 6, P502 }, /* CTS only */
    { AF_SCI1, 6, P503 },

    { AF_SCI2, 7, P403 },
    { AF_SCI2, 7, P404 }, /* CTS only */
    { AF_SCI2, 7, P610 }, /* CTS only */
    { AF_SCI2, 7, P611 },

    { AF_SCI1, 8, P107 },
    { AF_SCI1, 8, P605 }, /* CTS only */
    { AF_SCI1, 8, P606 },
    { AF_SCI1, 8, P801 }, /* CTS only */

    { AF_SCI2, 9, P108 },
    { AF_SCI2, 9, P114 }, /* CTS only */
    { AF_SCI2, 9, P205 },
    { AF_SCI2, 9, P206 }, /* CTS only */
    { AF_SCI2, 9, P301 },
    { AF_SCI2, 9, P303 }, /* CTS only */
    { AF_SCI2, 9, P603 },
    { AF_SCI2, 9, P604 }, /* CTS only */

    #else
    #error "CMSIS MCU Series is not specified."
    #endif
};
#define SCI_CTS_PINS_SIZE sizeof(ra_sci_cts_pins) / sizeof(ra_af_pin_t)

typedef struct _sci_fifo {
    volatile uint32_t tail, head, len, busy;
    uint8_t *bufp;
    uint32_t size;
} sci_fifo;

static uint32_t ra_sci_init_flag[RA_SCI_SLOT_MAX] = {
    #if defined(VECTOR_NUMBER_SCI0_RXI)
    0,
    #endif
    #if defined(VECTOR_NUMBER_SCI1_RXI)
    0,
    #endif
    #if defined(VECTOR_NUMBER_SCI2_RXI)
    0,
    #endif
    #if defined(VECTOR_NUMBER_SCI3_RXI)
    0,
    #endif
    #if defined(VECTOR_NUMBER_SCI4_RXI)
    0,
    #endif
    #if defined(VECTOR_NUMBER_SCI5_RXI)
    0,
    #endif
    #if defined(VECTOR_NUMBER_SCI6_RXI)
    0,
    #endif
    #if defined(VECTOR_NUMBER_SCI7_RXI)
    0,
    #endif
    #if defined(VECTOR_NUMBER_SCI8_RXI)
    0,
    #endif
    #if defined(VECTOR_NUMBER_SCI9_RXI)
    0,
    #endif
    #if defined(BSP_MCU_R7KA8P1KFLCAC) && !defined(VECTOR_NUMBER_SCI6_RXI)
    0,
    #endif
};
static SCI_CB sci_cb[RA_SCI_SLOT_MAX] = {
    #if defined(VECTOR_NUMBER_SCI0_RXI)
    (SCI_CB)0,
    #endif
    #if defined(VECTOR_NUMBER_SCI1_RXI)
    (SCI_CB)0,
    #endif
    #if defined(VECTOR_NUMBER_SCI2_RXI)
    (SCI_CB)0,
    #endif
    #if defined(VECTOR_NUMBER_SCI3_RXI)
    (SCI_CB)0,
    #endif
    #if defined(VECTOR_NUMBER_SCI4_RXI)
    (SCI_CB)0,
    #endif
    #if defined(VECTOR_NUMBER_SCI5_RXI)
    (SCI_CB)0,
    #endif
    #if defined(VECTOR_NUMBER_SCI6_RXI)
    (SCI_CB)0,
    #endif
    #if defined(VECTOR_NUMBER_SCI7_RXI)
    (SCI_CB)0,
    #endif
    #if defined(VECTOR_NUMBER_SCI8_RXI)
    (SCI_CB)0,
    #endif
    #if defined(VECTOR_NUMBER_SCI9_RXI)
    (SCI_CB)0,
    #endif
    #if defined(BSP_MCU_R7KA8P1KFLCAC) && !defined(VECTOR_NUMBER_SCI6_RXI)
    (SCI_CB)0,
    #endif
};
static uint8_t ch_9bit[RA_SCI_SLOT_MAX] = {
    #if defined(VECTOR_NUMBER_SCI0_RXI)
    0,
    #endif
    #if defined(VECTOR_NUMBER_SCI1_RXI)
    0,
    #endif
    #if defined(VECTOR_NUMBER_SCI2_RXI)
    0,
    #endif
    #if defined(VECTOR_NUMBER_SCI3_RXI)
    0,
    #endif
    #if defined(VECTOR_NUMBER_SCI4_RXI)
    0,
    #endif
    #if defined(VECTOR_NUMBER_SCI5_RXI)
    0,
    #endif
    #if defined(VECTOR_NUMBER_SCI6_RXI)
    0,
    #endif
    #if defined(VECTOR_NUMBER_SCI7_RXI)
    0,
    #endif
    #if defined(VECTOR_NUMBER_SCI8_RXI)
    0,
    #endif
    #if defined(VECTOR_NUMBER_SCI9_RXI)
    0,
    #endif
    #if defined(BSP_MCU_R7KA8P1KFLCAC) && !defined(VECTOR_NUMBER_SCI6_RXI)
    0,
    #endif
};
static uint8_t tx_buf[RA_SCI_SLOT_MAX][SCI_TX_BUF_SIZE];
static uint8_t rx_buf[RA_SCI_SLOT_MAX][SCI_RX_BUF_SIZE];
static volatile sci_fifo tx_fifo[RA_SCI_SLOT_MAX];
static volatile sci_fifo rx_fifo[RA_SCI_SLOT_MAX];
static uint32_t m_cts_pin[RA_SCI_SLOT_MAX] = {
    #if defined(VECTOR_NUMBER_SCI0_RXI)
    PIN_END,
    #endif
    #if defined(VECTOR_NUMBER_SCI1_RXI)
    PIN_END,
    #endif
    #if defined(VECTOR_NUMBER_SCI2_RXI)
    PIN_END,
    #endif
    #if defined(VECTOR_NUMBER_SCI3_RXI)
    PIN_END,
    #endif
    #if defined(VECTOR_NUMBER_SCI4_RXI)
    PIN_END,
    #endif
    #if defined(VECTOR_NUMBER_SCI5_RXI)
    PIN_END,
    #endif
    #if defined(VECTOR_NUMBER_SCI6_RXI)
    PIN_END,
    #endif
    #if defined(VECTOR_NUMBER_SCI7_RXI)
    PIN_END,
    #endif
    #if defined(VECTOR_NUMBER_SCI8_RXI)
    PIN_END,
    #endif
    #if defined(VECTOR_NUMBER_SCI9_RXI)
    PIN_END,
    #endif
    #if defined(BSP_MCU_R7KA8P1KFLCAC) && !defined(VECTOR_NUMBER_SCI6_RXI)
    PIN_END,
    #endif
};
static uint32_t m_rts_pin[RA_SCI_SLOT_MAX] = {
    #if defined(VECTOR_NUMBER_SCI0_RXI)
    PIN_END,
    #endif
    #if defined(VECTOR_NUMBER_SCI1_RXI)
    PIN_END,
    #endif
    #if defined(VECTOR_NUMBER_SCI2_RXI)
    PIN_END,
    #endif
    #if defined(VECTOR_NUMBER_SCI3_RXI)
    PIN_END,
    #endif
    #if defined(VECTOR_NUMBER_SCI4_RXI)
    PIN_END,
    #endif
    #if defined(VECTOR_NUMBER_SCI5_RXI)
    PIN_END,
    #endif
    #if defined(VECTOR_NUMBER_SCI6_RXI)
    PIN_END,
    #endif
    #if defined(VECTOR_NUMBER_SCI7_RXI)
    PIN_END,
    #endif
    #if defined(VECTOR_NUMBER_SCI8_RXI)
    PIN_END,
    #endif
    #if defined(VECTOR_NUMBER_SCI9_RXI)
    PIN_END,
    #endif
    #if defined(BSP_MCU_R7KA8P1KFLCAC) && !defined(VECTOR_NUMBER_SCI6_RXI)
    PIN_END,
    #endif
};

static inline uint32_t ra_sci_slot(uint32_t ch) {
#if defined(BSP_MCU_R7KA8P1KFLCAC) && !defined(VECTOR_NUMBER_SCI6_RXI)
    if (ch == 6) { return RA8P1_SCI6_FALLBACK_SLOT; }
#endif
    return ch_to_idx[ch];
}

static inline R_SCI0_Type *ra_sci_reg_ptr(uint32_t ch) {
#if defined(BSP_MCU_R7KA8P1KFLCAC) && !defined(VECTOR_NUMBER_SCI6_RXI)
    if (ch == 6) { return R_SCI6; }
#endif
    return sci_regs[ra_sci_slot(ch)];
}

static inline uint32_t ra_sci_mask_for_ch(uint32_t ch) {
#if defined(BSP_MCU_R7KA8P1KFLCAC) && !defined(VECTOR_NUMBER_SCI6_RXI)
    if (ch == 6) { return R_MSTP_MSTPCRB_MSTPB25_Msk; }
#endif
    return sci_module_mask[ra_sci_slot(ch)];
}

static inline IRQn_Type ra_sci_irq_or_invalid(const IRQn_Type *table, uint32_t ch) {
#if defined(BSP_MCU_R7KA8P1KFLCAC) && !defined(VECTOR_NUMBER_SCI6_RXI)
    if (ch == 6) { return FSP_INVALID_VECTOR; }
#endif
    return table[ra_sci_slot(ch)];
}

static void delay_us(volatile unsigned int us) {
    us *= 48;
    while (us-- > 0) {
        ;
    }
}

bool ra_af_find_ch_af(ra_af_pin_t *af_pin, uint32_t size, uint32_t pin, uint32_t *ch, uint32_t *af) {
    bool find = false;
    uint32_t i;
    for (i = 0; i < size; i++) {
        if (af_pin->pin == pin) {
            find = true;
            *ch = af_pin->ch;
            *af = af_pin->af;
            break;
        }
        af_pin++;
    }
    return find;
}

static void ra_sci_tx_set_pin(uint32_t pin) {
    bool find = false;
    uint32_t ch;
    uint32_t af;
    find = ra_af_find_ch_af((ra_af_pin_t *)&ra_sci_tx_pins, SCI_TX_PINS_SIZE, pin, &ch, &af);
    if (find) {
        ra_gpio_config(pin, GPIO_MODE_AF_PP, GPIO_NOPULL, GPIO_LOW_POWER, af);
    }
}

static void ra_sci_rx_set_pin(uint32_t pin) {
    bool find = false;
    uint32_t ch;
    uint32_t af;
    find = ra_af_find_ch_af((ra_af_pin_t *)&ra_sci_rx_pins, SCI_RX_PINS_SIZE, pin, &ch, &af);
    if (find) {
        ra_gpio_config(pin, GPIO_MODE_INPUT, GPIO_PULLUP, GPIO_LOW_POWER, af);
    }
}

static void ra_sci_cts_set_pin(uint32_t pin) {
    bool find = false;
    uint32_t ch;
    uint32_t af;
    find = ra_af_find_ch_af((ra_af_pin_t *)&ra_sci_cts_pins, SCI_CTS_PINS_SIZE, pin, &ch, &af);
    if (find) {
        ra_gpio_config(pin, GPIO_MODE_INPUT, GPIO_PULLUP, GPIO_LOW_POWER, af);
    }
}

static void ra_sci_module_start(uint32_t ch) {
#if defined(BSP_MCU_R7KA8P1KFLCAC)
    if (ch == 6) {
        volatile uint16_t *prcr = (volatile uint16_t *)0x5001E3FE;
        volatile uint32_t *mstpcrb = (volatile uint32_t *)0x50203004;
        uint32_t mask = ra_sci_mask_for_ch(ch);
        *prcr = 0xA502;
        *mstpcrb &= ~mask;
        __asm__ volatile ("dsb 0xf" ::: "memory");
        (void)*mstpcrb;
        *prcr = 0xA500;
        __asm__ volatile ("dsb 0xf; isb 0xf" ::: "memory");
        return;
    }
#endif
    ra_mstpcrb_start(ra_sci_mask_for_ch(ch));
}

static void ra_sci_module_stop(uint32_t ch) {
    ra_mstpcrb_stop(ra_sci_mask_for_ch(ch));
}

void ra_sci_rx_set_callback(int ch, SCI_CB cb) {
    sci_cb[ra_sci_slot(ch)] = cb;
}

static void ra_sci_irq_disable(uint32_t ch) {
#if defined(BSP_MCU_R7KA8P1KFLCAC)
    if (ra_sci_use_scib(ch)) { return; }
#endif
    uint32_t idx = ra_sci_slot(ch);
    IRQn_Type rxirq = ra_sci_irq_or_invalid(idx_to_rxirq, ch);
    IRQn_Type txirq = ra_sci_irq_or_invalid(idx_to_txirq, ch);
    IRQn_Type teirq = ra_sci_irq_or_invalid(idx_to_teirq, ch);
    IRQn_Type erirq = ra_sci_irq_or_invalid(idx_to_erirq, ch);
    if (rxirq != FSP_INVALID_VECTOR) { R_BSP_IrqDisable(rxirq); R_BSP_IrqStatusClear(rxirq); }
    if (txirq != FSP_INVALID_VECTOR) { R_BSP_IrqDisable(txirq); R_BSP_IrqStatusClear(txirq); }
    if (teirq != FSP_INVALID_VECTOR) { R_BSP_IrqDisable(teirq); R_BSP_IrqStatusClear(teirq); }
    if (erirq != FSP_INVALID_VECTOR) { R_BSP_IrqDisable(erirq); R_BSP_IrqStatusClear(erirq); }
}

static void ra_sci_irq_enable(uint32_t ch) {
#if defined(BSP_MCU_R7KA8P1KFLCAC)
    if (ra_sci_use_scib(ch)) { return; }
#endif
    uint32_t idx = ra_sci_slot(ch);
    IRQn_Type rxirq = ra_sci_irq_or_invalid(idx_to_rxirq, ch);
    IRQn_Type txirq = ra_sci_irq_or_invalid(idx_to_txirq, ch);
    IRQn_Type teirq = ra_sci_irq_or_invalid(idx_to_teirq, ch);
    IRQn_Type erirq = ra_sci_irq_or_invalid(idx_to_erirq, ch);
    if (rxirq != FSP_INVALID_VECTOR) { R_BSP_IrqEnable(rxirq); }
    if (txirq != FSP_INVALID_VECTOR) { R_BSP_IrqEnable(txirq); }
    if (teirq != FSP_INVALID_VECTOR) { R_BSP_IrqEnable(teirq); }
    if (erirq != FSP_INVALID_VECTOR) { R_BSP_IrqEnable(erirq); }
}

void ra_sci_rxirq_disable(uint32_t ch) {
#if defined(BSP_MCU_R7KA8P1KFLCAC)
    if (ra_sci_use_scib(ch)) { return; }
#endif
    uint32_t idx = ra_sci_slot(ch);
    IRQn_Type rxirq = ra_sci_irq_or_invalid(idx_to_rxirq, ch);
    if (rxirq != FSP_INVALID_VECTOR) { R_BSP_IrqDisable(rxirq); }
}

void ra_sci_rxirq_enable(uint32_t ch) {
#if defined(BSP_MCU_R7KA8P1KFLCAC)
    if (ra_sci_use_scib(ch)) { return; }
#endif
    uint32_t idx = ra_sci_slot(ch);
    IRQn_Type rxirq = ra_sci_irq_or_invalid(idx_to_rxirq, ch);
    if (rxirq != FSP_INVALID_VECTOR) { R_BSP_IrqEnable(rxirq); }
}

bool ra_sci_is_rxirq_enable(uint32_t ch) {
#if defined(BSP_MCU_R7KA8P1KFLCAC)
    if (ra_sci_use_scib(ch)) { return false; }
#endif
    uint32_t idx = ra_sci_slot(ch);
    IRQn_Type rxirq = ra_sci_irq_or_invalid(idx_to_rxirq, ch);
    if (rxirq == FSP_INVALID_VECTOR) { return true; }
    uint32_t _irq = (uint32_t)rxirq;
    return NVIC->ISER[(_irq >> 5UL)] == (uint32_t)(1UL << (_irq & 0x1FUL));
}

static void ra_sci_irq_priority(uint32_t ch, uint32_t ipl) {
#if defined(BSP_MCU_R7KA8P1KFLCAC)
    if (ra_sci_use_scib(ch)) { return; }
#endif
    uint32_t idx = ra_sci_slot(ch);
    IRQn_Type rxirq = ra_sci_irq_or_invalid(idx_to_rxirq, ch);
    IRQn_Type txirq = ra_sci_irq_or_invalid(idx_to_txirq, ch);
    IRQn_Type teirq = ra_sci_irq_or_invalid(idx_to_teirq, ch);
    IRQn_Type erirq = ra_sci_irq_or_invalid(idx_to_erirq, ch);
    if (rxirq != FSP_INVALID_VECTOR) { R_BSP_IrqCfg(rxirq, ipl, (void *)NULL); }
    if (txirq != FSP_INVALID_VECTOR) { R_BSP_IrqCfg(txirq, ipl, (void *)NULL); }
    if (teirq != FSP_INVALID_VECTOR) { R_BSP_IrqCfg(teirq, ipl, (void *)NULL); }
    if (erirq != FSP_INVALID_VECTOR) { R_BSP_IrqCfg(erirq, ipl, (void *)NULL); }
}

static void ra_sci_isr_rx(uint32_t ch) {
    IRQn_Type irq = R_FSP_CurrentIrqGet();
    uint32_t idx = ra_sci_slot(ch);
    uint16_t d;
    if (ch_9bit[idx]) {
        d = (uint16_t)ra_sci_reg_ptr(ch)->RDRHL;
    } else {
        d = (uint16_t)ra_sci_reg_ptr(ch)->RDR;
    }
    if (sci_cb[idx]) {
        if ((*sci_cb[idx])(ch, (int)d)) {
            // goto ra_sci_isr_rx_exit;
        }
    }
    uint32_t size = rx_fifo[idx].size;
    sci_fifo *rxfifo = (sci_fifo *)&rx_fifo[idx];
    if (rxfifo->len < size) {
        uint32_t i = rxfifo->head;
        if (ch_9bit[idx]) {
            *(uint16_t *)(rxfifo->bufp + i) = (uint16_t)d;
            i += 2;
            rxfifo->len += 2;
        } else {
            *(rxfifo->bufp + i) = (uint8_t)d;
            i++;
            rxfifo->len++;
        }
        rxfifo->head = i % size;
        if (m_rts_pin[idx] != PIN_END) {
            if (rxfifo->len > (size - RA_SCI_FLOW_START_NUM)) {
                ra_gpio_write(m_rts_pin[idx], 1);
            }
        }
    }
// ra_sci_isr_rx_exit:
    R_BSP_IrqStatusClear(irq);
}

static void ra_sci_isr_er(uint32_t ch) {
    IRQn_Type irq = R_FSP_CurrentIrqGet();
    R_BSP_IrqStatusClear(irq);
    uint32_t idx = ra_sci_slot(ch);
    R_SCI0_Type *sci_reg = ra_sci_reg_ptr(ch);
    sci_reg->RDR;
    while (0 != (sci_reg->SSR & 0x38)) {
        sci_reg->RDR;
        sci_reg->SSR = (uint8_t)((sci_reg->SSR & ~0x38) | 0xc0);
        if (0 != (sci_reg->SSR & 0x38)) {
            __asm__ __volatile__ ("nop");
        }
    }
}

static void ra_sci_isr_tx(uint32_t ch) {
    IRQn_Type irq = R_FSP_CurrentIrqGet();
    uint32_t idx = ra_sci_slot(ch);
    uint32_t size = tx_fifo[idx].size;
    sci_fifo *txfifo = (sci_fifo *)&tx_fifo[idx];
    if (txfifo->len != 0) {
        uint32_t i = txfifo->tail;
        if (ch_9bit[idx]) {
            ra_sci_reg_ptr(ch)->TDRHL = *(uint16_t *)(txfifo->bufp + i);
            i += 2;
            txfifo->len -= 2;
        } else {
            ra_sci_reg_ptr(ch)->TDR = (uint8_t)*(txfifo->bufp + i);
            i++;
            txfifo->len--;
        }
        txfifo->tail = i % size;
    } else {
        /* tx_fifo[idx].len == 0 */
        /* after transfer completed */
        uint8_t scr = ra_sci_reg_ptr(ch)->SCR;
        scr &= (uint8_t) ~0x80; /* TIE disable */
        scr |= (uint8_t)0x04;  /* TEIE enable */
        ra_sci_reg_ptr(ch)->SCR = scr;
    }
    R_BSP_IrqStatusClear(irq);
}

void ra_sci_isr_te(uint32_t ch) {
    IRQn_Type irq = R_FSP_CurrentIrqGet();
    uint32_t idx = ra_sci_slot(ch);
    tx_fifo[idx].busy = 0;
    ra_sci_reg_ptr(ch)->SCR &= (uint8_t) ~0x84; /* TIE and TEIE disable */
    R_BSP_IrqStatusClear(irq);
}

int ra_sci_rx_ch(uint32_t ch) {
    uint16_t c;
    uint32_t idx = ra_sci_slot(ch);
    uint32_t size = rx_fifo[idx].size;
    sci_fifo *rxfifo = (sci_fifo *)&rx_fifo[idx];
    if (rxfifo->len) {
        uint32_t state = ra_disable_irq();
        uint32_t i = rxfifo->tail;
        if (ch_9bit[idx]) {
            c = *(uint16_t *)(rxfifo->bufp + i);
            i += 2;
            rxfifo->len -= 2;
        } else {
            c = (uint16_t)*(rxfifo->bufp + i);
            i++;
            rxfifo->len--;
        }
        rxfifo->tail = i % size;
        if (m_rts_pin[idx] != PIN_END) {
            if (rxfifo->len <= (size - RA_SCI_FLOW_START_NUM)) {
                ra_gpio_write(m_rts_pin[idx], 0);
            }
        }
        ra_enable_irq(state);
    } else {
        c = 0;
    }
    return (int)c;
}

int ra_sci_rx_any(uint32_t ch) {
    uint32_t idx = ra_sci_slot(ch);
    return (int)(rx_fifo[idx].len != 0);
}

void ra_sci_tx_ch(uint32_t ch, int c) {
    uint32_t idx = ra_sci_slot(ch);
#if defined(BSP_MCU_R7KA8P1KFLCAC)
    if (ra_sci_use_scib(ch)) {
        uint8_t data = (uint8_t)c;
        // The FSP-generated vector_data.c on this board has no SCI8 TXI/RXI
        // vectors (only [0]=glcdc_line_detect_isr is registered), so the FSP
        // R_SCI_B_UART_Write IRQ-driven path queues a byte then spins forever
        // on tx_src_bytes / CSR.TEND because the TX-complete ISR is never
        // serviced.  Bypass the FSP-IRQ branch and use direct register polling
        // until vector_data.c is regenerated with SCI_B UART vectors.
        // Re-enable by deleting the `(0) && ` guard below.
        if ((0) && ra8p1_scib8_open) {
            if (FSP_SUCCESS == R_SCI_B_UART_Write((uart_ctrl_t *)&ra8p1_scib8_ctrl, &data, 1U)) {
                while ((ra8p1_scib8_ctrl.tx_src_bytes != 0U) || (ra8p1_scib8_ctrl.p_reg->CSR_b.TEND == 0U)) {
                }
                return;
            }
        }
        R_SCI_B0_Type *reg = ra_scib_reg_ptr(ch);
        while ((reg->CSR & R_SCI_B0_CSR_TDRE_Msk) == 0U) {
        }
        reg->TDR_BY = data;
        reg->CFCLR |= R_SCI_B0_CFCLR_TDREC_Msk;
        while ((reg->CSR & R_SCI_B0_CSR_TEND_Msk) == 0U) {
        }
        return;
    }
#endif
#if defined(MICROPY_RA8P1_BRINGUP_POLLED_TX) && (MICROPY_RA8P1_BRINGUP_POLLED_TX == 1)
    if (ch_9bit[idx]) {
        while ((ra_sci_reg_ptr(ch)->SSR & 0x80U) == 0) {
        }
        ra_sci_reg_ptr(ch)->TDRHL = (uint16_t)c;
        while ((ra_sci_reg_ptr(ch)->SSR & 0x04U) == 0) {
        }
    } else {
        while ((ra_sci_reg_ptr(ch)->SSR & 0x80U) == 0) {
        }
        ra_sci_reg_ptr(ch)->TDR = (uint8_t)c;
        while ((ra_sci_reg_ptr(ch)->SSR & 0x04U) == 0) {
        }
    }
    return;
#endif
    uint32_t size = tx_fifo[idx].size;
    sci_fifo *txfifo = (sci_fifo *)&tx_fifo[idx];
    while (tx_fifo[idx].len == size) {
    }
    uint32_t state = ra_disable_irq();
    uint32_t i = tx_fifo[idx].head;
    if (ch_9bit[idx]) {
        *(uint16_t *)(txfifo->bufp + i) = (uint16_t)c;
        i += 2;
        txfifo->len += 2;
    } else {
        *(txfifo->bufp + i) = (uint8_t)c;
        i++;
        txfifo->len++;
    }
    txfifo->head = i % size;
    if (!txfifo->busy) {
        txfifo->busy = 1;
        uint8_t scr = ra_sci_reg_ptr(ch)->SCR;
        if ((scr & 0xa0) != 0) {
            ra_sci_reg_ptr(ch)->SCR &= ~0xa0;
        }
        ra_sci_reg_ptr(ch)->SCR |= 0xa0; /* TIE- and TE-interrupt enable */
    }
    ra_enable_irq(state);
}

int ra_sci_tx_wait(uint32_t ch) {
    uint32_t idx = ra_sci_slot(ch);
    return (int)(tx_fifo[idx].len != (tx_fifo[idx].size - 1));
}

int ra_sci_tx_busy(uint32_t ch) {
    uint32_t idx = ra_sci_slot(ch);
    return (int)(tx_fifo[idx].busy);
}

int ra_sci_tx_bufsize(uint32_t ch) {
    uint32_t idx = ra_sci_slot(ch);
    return (int)(tx_fifo[idx].size - 1);
}

void ra_sci_tx_break(uint32_t ch) {
    uint32_t idx = ra_sci_slot(ch);
    R_SCI0_Type *sci_reg = ra_sci_reg_ptr(ch);
    uint8_t scr = sci_reg->SCR;
    uint8_t smr = sci_reg->SMR;
    sci_reg->SCR = 0;
    while (sci_reg->SCR != 0) {
        ;
    }
    sci_reg->SMR_b.STOP = 1;
    sci_reg->SCR = scr;
    sci_reg->TDR = 0;
    while (sci_reg->SSR_b.TDRE == 0) {
        ;
    }
    sci_reg->SMR = smr;
    return;
}

void ra_sci_tx_str(uint32_t ch, uint8_t *p) {
    int c;
    uint32_t idx = ra_sci_slot(ch);
    if (ch_9bit[idx]) {
        uint16_t *q = (uint16_t *)p;
        while ((c = *q++) != 0) {
            ra_sci_tx_ch(ch, (int)c);
        }
    } else {
        while ((c = (int)*p++) != 0) {
            ra_sci_tx_ch(ch, (int)c);
        }
    }
}

static void ra_sci_fifo_set(sci_fifo *fifo, uint8_t *bufp, uint32_t size) {
    fifo->head = 0;
    fifo->tail = 0;
    fifo->len = 0;
    fifo->busy = 0;
    fifo->bufp = bufp;
    fifo->size = size;
}

void ra_sci_txfifo_set(uint32_t ch, uint8_t *bufp, uint32_t size) {
    uint32_t idx = ra_sci_slot(ch);
    sci_fifo *fifo = (sci_fifo *)&tx_fifo[idx];
    ra_sci_fifo_set(fifo, bufp, size);
}

void ra_sci_rxfifo_set(uint32_t ch, uint8_t *bufp, uint32_t size) {
    uint32_t idx = ra_sci_slot(ch);
    sci_fifo *fifo = (sci_fifo *)&rx_fifo[idx];
    ra_sci_fifo_set(fifo, bufp, size);
}

static void ra_sci_fifo_init(uint32_t ch) {
    uint32_t idx = ra_sci_slot(ch);
    ra_sci_txfifo_set(ch, (uint8_t *)&tx_buf[idx][0], SCI_TX_BUF_SIZE);
    ra_sci_rxfifo_set(ch, (uint8_t *)&rx_buf[idx][0], SCI_RX_BUF_SIZE);
}

void ra_sci_set_baud(uint32_t ch, uint32_t baud) {
    uint32_t idx = ra_sci_slot(ch);
    R_SCI0_Type *sci_reg = ra_sci_reg_ptr(ch);
    baud_setting_t baud_setting;

    // baudrate_modulation=1(enabled), baudrate_max_err=5%(general value for FSP example).
    if (R_SCI_UART_BaudCalculate(baud, 1, 5000, &baud_setting) == FSP_SUCCESS) {
        // Disable transmitter and receiver.
        uint8_t scr = sci_reg->SCR;
        sci_reg->SCR = scr & ~(0xF0);

        // Change baudrate
        sci_reg->BRR = baud_setting.brr;
        // Update CKS(b1:b0)
        sci_reg->SMR_b.CKS = (uint8_t)(0x03U & baud_setting.cks);
        sci_reg->MDDR = baud_setting.mddr;
        // Update BGDM(b6)/ABCS(b4)/ABCSE(b3)/BRME(b2) bits
        sci_reg->SEMR = (uint8_t)((sci_reg->SEMR & ~(0x5cU)) | (baud_setting.semr_baudrate_bits & 0x5cU));

        // Restore SCR
        sci_reg->SCR = scr;
    }
}

/*
 * bits: 7, 8, 9
 * parity: none:0, odd:1, even:2
 */
void ra_sci_init_with_flow(uint32_t ch, uint32_t tx_pin, uint32_t rx_pin, uint32_t baud, uint32_t bits, uint32_t parity, uint32_t stop, uint32_t flow, uint32_t cts_pin, uint32_t rts_pin) {
    uint8_t smr = 0;
    uint8_t scmr = (uint8_t)0xf2;
    uint32_t idx = ra_sci_slot(ch);
    R_SCI0_Type *sci_reg = ra_sci_reg_ptr(ch);
    if (ra_sci_init_flag[idx] == 0) {
        ra_sci_fifo_init(ch);
        sci_cb[idx] = 0;
        ra_sci_init_flag[idx]++;
    } else {
        ra_sci_irq_disable(ch);
        ra_sci_module_stop(ch);
    }
    ra_sci_module_start(ch);
    ra_sci_tx_set_pin(tx_pin);
    ra_sci_rx_set_pin(rx_pin);
    if (flow) {
        if (cts_pin != (uint32_t)PIN_END) {
            m_cts_pin[idx] = cts_pin;
            ra_sci_cts_set_pin(cts_pin);
        }
        if (rts_pin != (uint32_t)PIN_END) {
            m_rts_pin[idx] = rts_pin;
            ra_gpio_config(rts_pin, GPIO_MODE_OUTPUT_PP, GPIO_NOPULL, GPIO_LOW_POWER, 0);
            ra_gpio_write(rts_pin, 0);
        }
    }
    ra_sci_irq_disable(ch);
    ra_sci_irq_priority(ch, RA_PRI_UART);
    uint32_t state = ra_disable_irq();
#if defined(BSP_MCU_R7KA8P1KFLCAC)
    if (ra_sci_use_scib(ch)) {
        ch_9bit[idx] = (bits == 9) ? 1 : 0;
        ra_scib_init_uart(ch, baud, bits, parity, stop, flow);
        ra_enable_irq(state);
        if (!ra_sci_init_flag[idx]) {
            ra_sci_init_flag[idx] = true;
        }
        return;
    }
#endif
    sci_reg->SCR = 0;
    while (sci_reg->SCR != 0) {
        ;
    }
    if (bits == 7) {
        smr |= (uint8_t)0x40;
    } else {
        smr &= (uint8_t) ~0x40;
    }
    if (parity != 0) {
        smr |= (uint8_t)0x20;
    } else {
        smr &= (uint8_t) ~0x20;
    }
    if (parity == 1) {
        smr |= (uint8_t)0x10;
    } else {
        smr &= (uint8_t) ~0x10;
    }
    if (stop == 2) {
        smr |= (uint8_t)0x80;
    } else {
        smr &= (uint8_t) ~0x80;
    }
    sci_reg->SMR = smr;
    if (bits == 9) {
        scmr &= (uint8_t) ~0x10;
        ch_9bit[idx] = 1;
    } else {
        scmr |= (uint8_t)0x10;
        ch_9bit[idx] = 0;
    }
    if (flow) {
        sci_reg->SPMR |= 0x01;
    }
    sci_reg->SCMR = scmr;
    sci_reg->SEMR = (uint8_t)0xc0;
    ra_sci_set_baud(ch, baud);
    delay_us(10);
    sci_reg->SCR = (uint8_t)0x50;
    ra_sci_irq_enable(ch);
    ra_enable_irq(state);
    if (!ra_sci_init_flag[idx]) {
        ra_sci_init_flag[idx] = true;
    }
}

void ra_sci_init(uint32_t ch, uint32_t tx_pin, uint32_t rx_pin, uint32_t baud, uint32_t bits, uint32_t parity, uint32_t stop, uint32_t flow) {
    ra_sci_init_with_flow(ch, tx_pin, rx_pin, baud, bits, parity, stop, flow, PIN_END, PIN_END);
}

void ra_sci_deinit(uint32_t ch) {
    uint32_t idx = ra_sci_slot(ch);
    if (ra_sci_init_flag[idx] != 0) {
        ra_sci_init_flag[idx]--;
        if (ra_sci_init_flag[idx] == 0) {
            ra_sci_irq_disable(ch);
            ra_sci_module_stop(ch);
            sci_cb[idx] = 0;
        }
    }
}

/* rx-interrupt */
void sci_uart_rxi_isr(void) {
    IRQn_Type irq = R_FSP_CurrentIrqGet();
    uint32_t ch = irq_to_ch[(uint32_t)irq];
    ra_sci_isr_rx(ch);
}

/* tx-interrupt */
void sci_uart_txi_isr(void) {
    IRQn_Type irq = R_FSP_CurrentIrqGet();
    uint32_t ch = irq_to_ch[(uint32_t)irq];
    ra_sci_isr_tx(ch);
}

/* er-interrupt */
void sci_uart_eri_isr(void) {
    IRQn_Type irq = R_FSP_CurrentIrqGet();
    uint32_t ch = irq_to_ch[(uint32_t)irq];
    ra_sci_isr_er(ch);
}

/* te-interrupt */
void sci_uart_tei_isr(void) {
    IRQn_Type irq = R_FSP_CurrentIrqGet();
    uint32_t ch = irq_to_ch[(uint32_t)irq];
    ra_sci_isr_te(ch);
}
