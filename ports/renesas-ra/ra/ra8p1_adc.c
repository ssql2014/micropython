/*
 * ADC_B driver for RA8P1 – implements the ra_adc_* interface used by
 * machine_adc.c (included via MICROPY_PY_MACHINE_ADC_INCLUDEFILE).
 *
 * RA8P1 uses r_adc_b (dual-converter ADC_B) instead of r_adc.
 * Converter 0 handles physical channels 0-10; converter 1 handles 11-22.
 * Internal channels (temperature, VREF, VBATT) are on converter 0.
 *
 * Implementation strategy: each ra_adc_read_ch() call performs a complete
 * Open → ScanCfg → ScanStart → poll-ADSR → Read → Close cycle.  This is
 * slower than keeping the peripheral open but avoids state-machine complexity
 * while the driver is being validated on hardware.  Promote to a keep-open
 * design once the full peripheral test suite passes.
 */

#if defined(RA8P1)

#include <stdint.h>
#include <stdbool.h>

#include "hal_data.h"
#include "ra_config.h"
#include "ra_gpio.h"
#include "ra_adc.h"
#include "r_adc_b.h"

/* ---- pin → channel map (EK-RA8P1, verify against board schematic) ---- */

typedef struct { uint32_t pin; uint8_t ch; } adc8p1_pin_ch_t;

static const adc8p1_pin_ch_t s_pin_ch[] = {
    /* Converter 0: physical channels 0-10, 12-13 */
    { P000, AN000 }, { P001, AN001 }, { P002, AN002 }, { P003, AN003 },
    { P004, AN004 }, { P005, AN005 }, { P006, AN006 }, { P007, AN007 },
    { P008, AN008 }, { P009, AN009 }, { P010, AN010 },
    { P014, AN012 }, { P015, AN013 },
    /* Converter 1: physical channels 16-28 (AN100-AN128 encode as 32+n) */
    { P500, AN116 }, { P501, AN117 }, { P502, AN118 }, { P503, AN119 },
    { P504, AN120 }, { P505, AN121 }, { P506, AN122 }, { P507, AN123 },
    { P508, AN124 }, { P800, AN125 }, { P801, AN126 }, { P802, AN127 },
    { P803, AN128 },
};
#define PIN_CH_COUNT ((uint32_t)(sizeof(s_pin_ch) / sizeof(s_pin_ch[0])))

/* ---- resolution (ADC_B produces 12-bit results) ---------------------- */

static uint8_t s_resolution = 12;

/* ---- helpers --------------------------------------------------------- */

/* Physical channel 0-22 → converter unit (0 or 1).
 * Internal channels (ADC_TEMP=29, ADC_REF=30) are on unit 0. */
static uint8_t channel_to_unit(uint8_t ch) {
    /* AN1xx encode as 32+n → unit 1 */
    if (ch >= 32) {
        return 1;
    }
    /* Channels 0-10, 12-13 → unit 0; 11, 14-22 → unit 1. */
    return (ch <= 13) ? 0 : 1;
}

/* Translate ra_adc.h channel enum (AN000=0..AN128=56, ADC_TEMP=29,
 * ADC_REF=30) to the FSP adc_channel_t used by R_ADC_B_Read. */
static adc_channel_t ra_ch_to_fsp(uint8_t ra_ch) {
    if (ra_ch == ADC_TEMP) { return ADC_CHANNEL_TEMPERATURE; }
    if (ra_ch == ADC_REF)  { return ADC_CHANNEL_VOLT; }
    if (ra_ch >= 32) {
        /* AN1xx: strip the 32 offset to get the physical channel number
         * then add back as the converter-1 channel (same numeric value). */
        return (adc_channel_t)(ra_ch - 32);
    }
    return (adc_channel_t)ra_ch;
}

/* ---- ADC_B static config objects ------------------------------------- */

/* ISR config: all IPLs = 0 means interrupts disabled (polling mode). */
static const adc_b_isr_cfg_t s_isr_cfg = { 0 };

/* Extended config with safe defaults for single-scan software-triggered use. */
static const adc_b_extended_cfg_t s_ext_cfg = {
    .pga_gain            = { 0, 0, 0, 0 },
    .clock_control_bits  = {
        .source_selection = 2,  /* PCLKA */
        .divider          = 3,  /* ÷4: 240MHz PCLKA → 60MHz ADC_B clock */
    },
    /* converter 0 & 1: single scan, sequential method */
    .adc_b_converter_mode[0] = { .mode = ADC_B_CONVERTER_MODE_SINGLE_SCAN,
                                  .method = 0 },
    .adc_b_converter_mode[1] = { .mode = ADC_B_CONVERTER_MODE_SINGLE_SCAN,
                                  .method = 0 },
    .scan_group_enable    = 0x0001U,  /* enable scan group 0 only */
    /* Group 0 → converter 0 (updated dynamically for unit-1 channels) */
    .converter_selection_0 = 0x00U,
    .fifo_enable_mask      = 0x0000U, /* no FIFO */
    .p_isr_cfg             = &s_isr_cfg,
    /* sampling_state_table[0] = 10 ADC clocks (safe minimum) */
    .sampling_state_table  = { 10, 0 },
    /* conversion_state: CST0=7, CST1=7 in bits [3:0] and [19:16] */
    .conversion_state      = 0x00070007U,
};

/* ---- per-read dynamic objects (not const, reconfigured each call) ----- */

static adc_b_instance_ctrl_t    s_ctrl;
static adc_b_virtual_channel_cfg_t s_vchan;
static adc_b_group_cfg_t        s_group;
static adc_b_virtual_channel_cfg_t *s_vchan_ptrs[1];
static adc_b_group_cfg_t        *s_group_ptrs[1];
static adc_b_scan_cfg_t         s_scan;
static adc_b_extended_cfg_t     s_ext_dyn; /* mutable copy of s_ext_cfg */

static adc_cfg_t s_cfg = {
    .unit       = 0,
    .mode       = ADC_MODE_SINGLE_SCAN,
    .resolution = ADC_RESOLUTION_12_BIT,
    .alignment  = ADC_ALIGNMENT_RIGHT,
    .trigger    = ADC_TRIGGER_SOFTWARE,
    .p_callback = NULL,
    .p_context  = NULL,
    .p_extend   = &s_ext_dyn,
    /* IRQ fields unused (polling mode) */
    .scan_end_ipl        = (uint8_t)0,
    .scan_end_b_ipl      = (uint8_t)0,
    .scan_end_irq        = FSP_INVALID_VECTOR,
    .scan_end_b_irq      = FSP_INVALID_VECTOR,
};

/* ---- public interface ------------------------------------------------ */

bool ra_adc_pin_to_ch(uint32_t pin, uint8_t *ch) {
    for (uint32_t i = 0; i < PIN_CH_COUNT; i++) {
        if (s_pin_ch[i].pin == pin) {
            *ch = s_pin_ch[i].ch;
            return true;
        }
    }
    *ch = (uint8_t)ADC_NON;
    return false;
}

bool ra_adc_ch_to_pin(uint8_t ch, uint32_t *pin) {
    for (uint32_t i = 0; i < PIN_CH_COUNT; i++) {
        if (s_pin_ch[i].ch == ch) {
            *pin = s_pin_ch[i].pin;
            return true;
        }
    }
    *pin = (uint32_t)PIN_END;
    return false;
}

uint8_t ra_adc_get_channel(uint32_t pin) {
    uint8_t ch;
    return ra_adc_pin_to_ch(pin, &ch) ? ch : (uint8_t)ADC_NON;
}

void ra_adc_set_pin(uint32_t pin, bool adc_enable) {
    uint32_t cfg = adc_enable ? IOPORT_CFG_ANALOG_ENABLE : IOPORT_CFG_PORT_DIRECTION_INPUT;
    R_IOPORT_PinCfg(&g_ioport_ctrl, (bsp_io_port_pin_t)pin, cfg);
}

void ra_adc_enable(uint32_t pin)  { ra_adc_set_pin(pin, true); }
void ra_adc_disable(uint32_t pin) { ra_adc_set_pin(pin, false); }

void ra_adc_set_resolution_set(uint8_t res) {
    /* ADC_B always produces 12-bit results; ignore other values. */
    (void)res;
    s_resolution = 12;
}

uint8_t ra_adc_get_resolution(void) { return s_resolution; }

uint16_t ra_adc_read_ch(uint8_t ra_ch) {
    uint8_t unit = channel_to_unit(ra_ch);
    adc_channel_t fsp_ch = ra_ch_to_fsp(ra_ch);

    /* Build mutable extended config, adjusting converter selection for unit. */
    s_ext_dyn = s_ext_cfg;
    /* converter_selection byte 0 = group 0 unit (0=unit0, 1=unit1) */
    s_ext_dyn.converter_selection[0] = unit;

    /* Virtual channel: map virtual 0 → physical channel ra_ch. */
    s_vchan = (adc_b_virtual_channel_cfg_t){
        .channel_id             = ADC_B_VIRTUAL_CHANNEL_0,
        .channel_cfg_bits       = {
            .group           = 0,
            .channel         = (uint32_t)fsp_ch,
            .differential    = 0,
            .sample_table_id = 0,
        },
        .channel_control_a      = 0,
        .channel_control_b      = 0,
        .channel_control_c_bits = { .data_is_unsigned = 1 },
    };
    s_vchan_ptrs[0] = &s_vchan;

    /* Scan group 0: one virtual channel, software start, no interrupts. */
    s_group = (adc_b_group_cfg_t){
        .scan_group_id            = 0,
        .converter_selection      = (adc_b_unit_id_t)unit,
        .scan_group_enable        = true,
        .virtual_channel_count    = 1,
        .scan_end_interrupt_enable = false,
        .p_virtual_channels       = s_vchan_ptrs,
    };
    s_group_ptrs[0] = &s_group;

    s_scan = (adc_b_scan_cfg_t){ .group_count = 1, .p_adc_groups = s_group_ptrs };

    s_cfg.unit = unit;

    /* --- open → config → start → poll → read → close --- */
    if (R_ADC_B_Open(&s_ctrl, &s_cfg) != FSP_SUCCESS) { return 0; }
    if (R_ADC_B_ScanCfg(&s_ctrl, &s_scan) != FSP_SUCCESS) {
        R_ADC_B_Close(&s_ctrl); return 0;
    }
    R_ADC_B_ScanStart(&s_ctrl);

    /* Poll ADSR: bit 0 = any scan active.  Timeout after ~1ms. */
    uint32_t timeout = 60000U; /* ~1ms at 60MHz ADC_B clock */
    while ((R_ADC_B->ADSR & 0x1U) && --timeout) { ; }

    uint16_t result = 0;
    R_ADC_B_Read(&s_ctrl, fsp_ch, &result);
    R_ADC_B_Close(&s_ctrl);
    return result;
}

uint16_t ra_adc_read(uint32_t pin) {
    uint8_t ch;
    if (!ra_adc_pin_to_ch(pin, &ch)) { return 0; }
    return ra_adc_read_ch(ch);
}

int16_t ra_adc_read_itemp(void) {
    return (int16_t)ra_adc_read_ch(ADC_TEMP);
}

float ra_adc_read_ftemp(void) {
    /* TODO: apply RA8P1 temperature calibration coefficients. */
    return (float)ra_adc_read_itemp();
}

float ra_adc_read_fref(void) {
    return (float)ra_adc_read_ch(ADC_REF);
}

void ra_adc_all(uint32_t resolution, uint32_t mask) {
    (void)resolution; (void)mask;
    /* Bulk scan not implemented; called only for compatibility. */
}

uint16_t ra_adc_all_read_ch(uint32_t ch) {
    return ra_adc_read_ch((uint8_t)ch);
}

bool ra_adc_init(void)   { return true; }
bool ra_adc_deinit(void) { return true; }

__attribute__((weak)) void adc_scan_end_isr(void) { }

#endif /* RA8P1 */
