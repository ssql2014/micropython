/*
 * Python binding for the EK-RA8P1 GLCDC framebuffer.
 *
 * Exposes a `ra8p1_display` module to MicroPython with:
 *   ra8p1_display.init()              -- open GLCDC and start scan-out
 *   ra8p1_display.deinit()            -- stop and close GLCDC
 *   ra8p1_display.WIDTH, HEIGHT       -- resolution constants
 *   ra8p1_display.STRIDE, BPP, FORMAT -- buffer geometry
 *   ra8p1_display.framebuffer(idx)    -- memoryview('I') over g_framebuffer[idx]
 *   ra8p1_display.flip(idx)           -- bufferChange to the named back buffer
 *   ra8p1_display.fill(idx, color)    -- fast 32-bit fill
 *   ra8p1_display.pixel(idx,x,y,col)  -- single-pixel write
 *
 * Pixel format on this board (per FSP-generated common_data.c) is
 * DISPLAY_IN_FORMAT_32BITS_RGB888 — 32 bits per pixel, top byte unused,
 * payload as 0x00RRGGBB.
 */

#include "py/runtime.h"
#include "py/objarray.h"

#if defined(BSP_MCU_R7KA8P1KFLCAC) && MICROPY_HW_ENABLE_DISPLAY

#include "common_data.h"

#define DISPLAY_PIXEL_BYTES (4U)

static bool ra8p1_display_open = false;
static int ra8p1_display_last_open_err = FSP_SUCCESS;
static int ra8p1_display_last_start_err = FSP_SUCCESS;

#if defined(MICROPY_RA8P1_BRINGUP_MIPI_DSI_TEST) && MICROPY_RA8P1_BRINGUP_MIPI_DSI_TEST
extern volatile uint32_t g_ra8p1_dsi_diag_stage;
extern volatile uint32_t g_ra8p1_dsi_diag_panel_clk;
extern volatile uint32_t g_ra8p1_dsi_diag_rstsr;
extern volatile uint32_t g_ra8p1_dsi_diag_plsr;
extern volatile uint32_t g_ra8p1_dsi_diag_linksr;
extern volatile uint32_t g_ra8p1_dsi_diag_vmsr;
extern volatile int32_t  g_ra8p1_dsi_diag_err;
extern volatile uint32_t g_ra8p1_glcdc_diag_stage;
extern volatile uint32_t g_ra8p1_glcdc_diag_panel_clk_programmed;
extern volatile uint32_t g_ra8p1_glcdc_diag_panel_clk_after_write;
extern volatile uint32_t g_ra8p1_glcdc_diag_panel_clk_before_dsi;
extern volatile int32_t  g_ra8p1_glcdc_diag_dsi_err;
#if defined(MICROPY_RA8P1_SAFE_BOOT) && MICROPY_RA8P1_SAFE_BOOT
extern volatile uint32_t g_ra8p1_safe_boot_diag_stage;
extern volatile uint32_t g_ra8p1_safe_boot_diag_pdctrgd_before;
extern volatile uint32_t g_ra8p1_safe_boot_diag_pdctrgd_after;
extern volatile uint32_t g_ra8p1_safe_boot_diag_pgcsar;
extern volatile uint32_t g_ra8p1_safe_boot_diag_psarc;
extern volatile int32_t  g_ra8p1_safe_boot_diag_power_err;
#endif
#endif

#define RA8P1_DISPLAY_DICT_STORE_INT(dict, key, value) \
    mp_obj_dict_store((dict), MP_OBJ_NEW_QSTR(MP_QSTR_##key), mp_obj_new_int(value))

#define RA8P1_DISPLAY_DICT_STORE_UINT(dict, key, value) \
    mp_obj_dict_store((dict), MP_OBJ_NEW_QSTR(MP_QSTR_##key), mp_obj_new_int_from_uint(value))

static inline uint32_t *ra8p1_display_buf(mp_int_t idx) {
    if (idx < 0 || idx >= DISPLAY_FRAMEBUFFER_COUNT_INPUT0) {
        mp_raise_ValueError(MP_ERROR_TEXT("buffer index out of range"));
    }
    return (uint32_t *)&g_framebuffer[idx][0];
}

static inline size_t ra8p1_display_stride_pixels(void) {
    return (size_t)DISPLAY_BUFFER_STRIDE_BYTES_INPUT0 / DISPLAY_PIXEL_BYTES;
}

static inline size_t ra8p1_display_pixel_count(void) {
    return ra8p1_display_stride_pixels() * (size_t)DISPLAY_VSIZE_INPUT0;
}

static mp_obj_t ra8p1_display_init(void) {
    R_BSP_SdramInit(true);
    fsp_err_t err = g_display.p_api->open(g_display.p_ctrl, g_display.p_cfg);
    ra8p1_display_last_open_err = err;
    if (err != FSP_SUCCESS && err != FSP_ERR_ALREADY_OPEN) {
        mp_raise_OSError(err);
    }
    err = g_display.p_api->start(g_display.p_ctrl);
    ra8p1_display_last_start_err = err;
    if (err != FSP_SUCCESS) {
        mp_raise_OSError(err);
    }
    ra8p1_display_open = true;
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_0(ra8p1_display_init_obj, ra8p1_display_init);

static mp_obj_t ra8p1_display_deinit(void) {
    if (ra8p1_display_open) {
        (void)g_display.p_api->stop(g_display.p_ctrl);
        (void)g_display.p_api->close(g_display.p_ctrl);
        ra8p1_display_open = false;
    }
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_0(ra8p1_display_deinit_obj, ra8p1_display_deinit);

static mp_obj_t ra8p1_display_status(void) {
    mp_obj_t status = mp_obj_new_dict(24);
    RA8P1_DISPLAY_DICT_STORE_INT(status, open, ra8p1_display_open);
    RA8P1_DISPLAY_DICT_STORE_INT(status, last_open_err, ra8p1_display_last_open_err);
    RA8P1_DISPLAY_DICT_STORE_INT(status, last_start_err, ra8p1_display_last_start_err);
    RA8P1_DISPLAY_DICT_STORE_UINT(status, glcdc_state, g_display_ctrl.state);
    RA8P1_DISPLAY_DICT_STORE_UINT(status, glcdc_panel_clk, R_GLCDC->SYSCNT.PANEL_CLK);
    RA8P1_DISPLAY_DICT_STORE_UINT(status, glcdc_bg_en, R_GLCDC->BG.EN);
    RA8P1_DISPLAY_DICT_STORE_UINT(status, glcdc_bg_mon, R_GLCDC->BG.MON);
#if defined(MICROPY_RA8P1_BRINGUP_MIPI_DSI_TEST) && MICROPY_RA8P1_BRINGUP_MIPI_DSI_TEST
    RA8P1_DISPLAY_DICT_STORE_UINT(status, dsi_open, g_mipi_dsi0_ctrl.open);
    RA8P1_DISPLAY_DICT_STORE_UINT(status, dsi_stage, g_ra8p1_dsi_diag_stage);
    RA8P1_DISPLAY_DICT_STORE_INT(status, dsi_err, g_ra8p1_dsi_diag_err);
    RA8P1_DISPLAY_DICT_STORE_UINT(status, dsi_panel_clk_at_check, g_ra8p1_dsi_diag_panel_clk);
    RA8P1_DISPLAY_DICT_STORE_UINT(status, dsi_rstsr, g_ra8p1_dsi_diag_rstsr);
    RA8P1_DISPLAY_DICT_STORE_UINT(status, dsi_plsr, g_ra8p1_dsi_diag_plsr);
    RA8P1_DISPLAY_DICT_STORE_UINT(status, dsi_linksr, g_ra8p1_dsi_diag_linksr);
    RA8P1_DISPLAY_DICT_STORE_UINT(status, dsi_vmsr, g_ra8p1_dsi_diag_vmsr);
    RA8P1_DISPLAY_DICT_STORE_UINT(status, glcdc_diag_stage, g_ra8p1_glcdc_diag_stage);
    RA8P1_DISPLAY_DICT_STORE_UINT(status, glcdc_panel_clk_programmed, g_ra8p1_glcdc_diag_panel_clk_programmed);
    RA8P1_DISPLAY_DICT_STORE_UINT(status, glcdc_panel_clk_after_write, g_ra8p1_glcdc_diag_panel_clk_after_write);
    RA8P1_DISPLAY_DICT_STORE_UINT(status, glcdc_panel_clk_before_dsi, g_ra8p1_glcdc_diag_panel_clk_before_dsi);
    RA8P1_DISPLAY_DICT_STORE_INT(status, glcdc_dsi_err, g_ra8p1_glcdc_diag_dsi_err);
#if defined(MICROPY_RA8P1_SAFE_BOOT) && MICROPY_RA8P1_SAFE_BOOT
    RA8P1_DISPLAY_DICT_STORE_UINT(status, safe_boot_stage, g_ra8p1_safe_boot_diag_stage);
    RA8P1_DISPLAY_DICT_STORE_INT(status, safe_boot_power_err, g_ra8p1_safe_boot_diag_power_err);
    RA8P1_DISPLAY_DICT_STORE_UINT(status, safe_boot_pdctrgd_before, g_ra8p1_safe_boot_diag_pdctrgd_before);
    RA8P1_DISPLAY_DICT_STORE_UINT(status, safe_boot_pdctrgd_after, g_ra8p1_safe_boot_diag_pdctrgd_after);
    RA8P1_DISPLAY_DICT_STORE_UINT(status, safe_boot_pgcsar, g_ra8p1_safe_boot_diag_pgcsar);
    RA8P1_DISPLAY_DICT_STORE_UINT(status, safe_boot_psarc, g_ra8p1_safe_boot_diag_psarc);
#endif
#endif
    return status;
}
static MP_DEFINE_CONST_FUN_OBJ_0(ra8p1_display_status_obj, ra8p1_display_status);

static mp_obj_t ra8p1_display_framebuffer(mp_obj_t idx_in) {
    mp_int_t idx = mp_obj_get_int(idx_in);
    uint32_t *buf = ra8p1_display_buf(idx);
    /* High bit on typecode marks the memoryview as writable. */
    return mp_obj_new_memoryview(0x80 | 'I',
        ra8p1_display_pixel_count(), buf);
}
static MP_DEFINE_CONST_FUN_OBJ_1(ra8p1_display_framebuffer_obj, ra8p1_display_framebuffer);

static mp_obj_t ra8p1_display_flip(mp_obj_t idx_in) {
    mp_int_t idx = mp_obj_get_int(idx_in);
    uint32_t *buf = ra8p1_display_buf(idx);
    fsp_err_t err = g_display.p_api->bufferChange(g_display.p_ctrl,
        (uint8_t *)buf, DISPLAY_FRAME_LAYER_1);
    if (err != FSP_SUCCESS) {
        mp_raise_OSError(err);
    }
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(ra8p1_display_flip_obj, ra8p1_display_flip);

static mp_obj_t ra8p1_display_fill(mp_obj_t idx_in, mp_obj_t color_in) {
    mp_int_t idx = mp_obj_get_int(idx_in);
    uint32_t color = (uint32_t)mp_obj_get_int_truncated(color_in);
    uint32_t *buf = ra8p1_display_buf(idx);
    size_t n = ra8p1_display_pixel_count();
    for (size_t i = 0; i < n; ++i) {
        buf[i] = color;
    }
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(ra8p1_display_fill_obj, ra8p1_display_fill);

static mp_obj_t ra8p1_display_pixel(size_t n_args, const mp_obj_t *args) {
    mp_int_t idx = mp_obj_get_int(args[0]);
    mp_int_t x = mp_obj_get_int(args[1]);
    mp_int_t y = mp_obj_get_int(args[2]);
    mp_int_t color = mp_obj_get_int_truncated(args[3]);
    if (x < 0 || x >= DISPLAY_HSIZE_INPUT0 || y < 0 || y >= DISPLAY_VSIZE_INPUT0) {
        mp_raise_ValueError(MP_ERROR_TEXT("pixel out of range"));
    }
    uint32_t *buf = ra8p1_display_buf(idx);
    buf[(size_t)y * ra8p1_display_stride_pixels() + (size_t)x] = (uint32_t)color;
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(ra8p1_display_pixel_obj, 4, 4, ra8p1_display_pixel);

static const mp_rom_map_elem_t ra8p1_display_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),    MP_ROM_QSTR(MP_QSTR_ra8p1_display) },
    { MP_ROM_QSTR(MP_QSTR_init),        MP_ROM_PTR(&ra8p1_display_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_deinit),      MP_ROM_PTR(&ra8p1_display_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR_status),      MP_ROM_PTR(&ra8p1_display_status_obj) },
    { MP_ROM_QSTR(MP_QSTR_framebuffer), MP_ROM_PTR(&ra8p1_display_framebuffer_obj) },
    { MP_ROM_QSTR(MP_QSTR_flip),        MP_ROM_PTR(&ra8p1_display_flip_obj) },
    { MP_ROM_QSTR(MP_QSTR_fill),        MP_ROM_PTR(&ra8p1_display_fill_obj) },
    { MP_ROM_QSTR(MP_QSTR_pixel),       MP_ROM_PTR(&ra8p1_display_pixel_obj) },
    { MP_ROM_QSTR(MP_QSTR_WIDTH),       MP_ROM_INT(DISPLAY_HSIZE_INPUT0) },
    { MP_ROM_QSTR(MP_QSTR_HEIGHT),      MP_ROM_INT(DISPLAY_VSIZE_INPUT0) },
    { MP_ROM_QSTR(MP_QSTR_STRIDE),      MP_ROM_INT(DISPLAY_BUFFER_STRIDE_BYTES_INPUT0) },
    { MP_ROM_QSTR(MP_QSTR_BPP),         MP_ROM_INT(32) },
    { MP_ROM_QSTR(MP_QSTR_FORMAT),      MP_ROM_QSTR(MP_QSTR_XRGB8888) },
};
static MP_DEFINE_CONST_DICT(ra8p1_display_module_globals, ra8p1_display_module_globals_table);

const mp_obj_module_t mp_module_ra8p1_display = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&ra8p1_display_module_globals,
};

MP_REGISTER_MODULE(MP_QSTR_ra8p1_display, mp_module_ra8p1_display);

#endif // BSP_MCU_R7KA8P1KFLCAC && MICROPY_HW_ENABLE_DISPLAY
