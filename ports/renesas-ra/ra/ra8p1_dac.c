/*
 * DAC_B driver for RA8P1 – implements the ra_dac_* interface used by
 * machine_dac.c.
 *
 * RA8P1 has two DAC_B channels:
 *   DA0 → P014 (verify against EK-RA8P1 schematic)
 *   DA1 → P015 (verify against EK-RA8P1 schematic)
 *
 * The r_dac_b FSP driver is kept open between writes so the output
 * voltage is held.  ra_dac_deinit() calls R_DAC_B_Close.
 */

#if defined(RA8P1)

#include <stdint.h>
#include <stdbool.h>

#include "hal_data.h"
#include "ra_config.h"
#include "ra_gpio.h"
#include "ra_dac.h"
#include "r_dac_b.h"

/* ---- pin map -------------------------------------------------------- */

/* DAC output pins on EK-RA8P1 (needs board schematic verification). */
#define RA8P1_DAC0_PIN  P014
#define RA8P1_DAC1_PIN  P015
#define RA8P1_DAC_COUNT 2

typedef struct {
    uint32_t           pin;
    uint8_t            ch;
    bool               open;
    uint16_t           last_val;
    dac_b_instance_ctrl_t ctrl;
} ra8p1_dac_ch_t;

static ra8p1_dac_ch_t s_dac[RA8P1_DAC_COUNT] = {
    { .pin = RA8P1_DAC0_PIN, .ch = 0, .open = false, .last_val = 0 },
    { .pin = RA8P1_DAC1_PIN, .ch = 1, .open = false, .last_val = 0 },
};

/* ---- extended & base config ----------------------------------------- */

static const dac_b_extended_cfg_t s_ext_cfg = {
    .internal_output_enabled = false,
    .data_format             = DAC_DATA_FORMAT_FLUSH_RIGHT,
    .vrefh                   = DAC_B_VREFH_NORMAL,   /* VREFH >= 2.7V (AVCC0) */
};

/* ---- helpers -------------------------------------------------------- */

static ra8p1_dac_ch_t *find_by_pin(uint32_t pin) {
    for (int i = 0; i < RA8P1_DAC_COUNT; i++) {
        if (s_dac[i].pin == pin) { return &s_dac[i]; }
    }
    return NULL;
}

static ra8p1_dac_ch_t *find_by_ch(uint8_t ch) {
    for (int i = 0; i < RA8P1_DAC_COUNT; i++) {
        if (s_dac[i].ch == ch) { return &s_dac[i]; }
    }
    return NULL;
}

static bool open_ch(ra8p1_dac_ch_t *d) {
    if (d->open) { return true; }
    dac_cfg_t cfg = {
        .channel  = d->ch,
        .p_extend = &s_ext_cfg,
    };
    if (R_DAC_B_Open(&d->ctrl, &cfg) != FSP_SUCCESS) { return false; }
    d->open = true;
    return true;
}

/* ---- public interface ----------------------------------------------- */

bool ra_dac_is_dac_pin(uint32_t pin) {
    return find_by_pin(pin) != NULL;
}

uint8_t ra_dac_is_running(uint8_t ch) {
    ra8p1_dac_ch_t *d = find_by_ch(ch);
    return (d && d->open) ? 1 : 0;
}

void ra_dac_start(uint8_t ch) {
    ra8p1_dac_ch_t *d = find_by_ch(ch);
    if (!d) { return; }
    if (!open_ch(d)) { return; }
    R_DAC_B_Start(&d->ctrl);
}

void ra_dac_stop(uint8_t ch) {
    ra8p1_dac_ch_t *d = find_by_ch(ch);
    if (d && d->open) { R_DAC_B_Stop(&d->ctrl); }
}

void ra_dac_write(uint8_t ch, uint16_t val) {
    ra8p1_dac_ch_t *d = find_by_ch(ch);
    if (!d) { return; }
    if (!open_ch(d)) { return; }
    /* Scale 16-bit MicroPython value to 12-bit DAC_B range */
    uint16_t dac_val = (uint16_t)(val >> 4);
    R_DAC_B_Write(&d->ctrl, dac_val);
    d->last_val = val;
}

uint16_t ra_dac_read(uint8_t ch) {
    ra8p1_dac_ch_t *d = find_by_ch(ch);
    return d ? d->last_val : 0;
}

void ra_dac_init(uint32_t dac_pin, uint8_t ch) {
    ra8p1_dac_ch_t *d = find_by_pin(dac_pin);
    if (!d || d->ch != ch) { return; }
    /* Configure the pin as analog output (no digital function). */
    R_IOPORT_PinCfg(&g_ioport_ctrl, (bsp_io_port_pin_t)dac_pin,
                    IOPORT_CFG_ANALOG_ENABLE);
    open_ch(d);
}

void ra_dac_deinit(uint32_t dac_pin, uint8_t ch) {
    ra8p1_dac_ch_t *d = find_by_pin(dac_pin);
    if (!d || d->ch != ch) { return; }
    if (d->open) {
        R_DAC_B_Stop(&d->ctrl);
        R_DAC_B_Close(&d->ctrl);
        d->open = false;
    }
    /* Restore pin to high-Z digital input. */
    R_IOPORT_PinCfg(&g_ioport_ctrl, (bsp_io_port_pin_t)dac_pin,
                    IOPORT_CFG_PORT_DIRECTION_INPUT);
}

#endif /* RA8P1 */
