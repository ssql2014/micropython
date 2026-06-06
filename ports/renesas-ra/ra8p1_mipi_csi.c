/*
 * Minimal EK-RA8P1 MIPI-CSI/VIN bring-up module.
 *
 * This exposes the FSP VIN/MIPI-CSI path verified with the official Renesas
 * example, plus a private GPIO bit-bang I2C path for the on-board switch and
 * OV5640 sensor used during early RA8P1 bring-up.
 */

#include "py/runtime.h"
#include "py/objarray.h"

#if defined(BSP_MCU_R7KA8P1KFLCAC) && defined(MICROPY_RA8P1_BRINGUP_MIPI_CSI_TEST) && MICROPY_RA8P1_BRINGUP_MIPI_CSI_TEST

#include <string.h>

#include "common_data.h"
#include "ra_gpio.h"
#include "ra_i2c.h"
#if defined(RA8P1_CSI_USE_FSP_IIC) && RA8P1_CSI_USE_FSP_IIC
#include "r_iic_master.h"
#endif

static bool ra8p1_mipi_csi_open;
static volatile uint32_t ra8p1_mipi_csi_vin_callbacks;
static volatile uint32_t ra8p1_mipi_csi_csi_callbacks;
static volatile uint32_t ra8p1_mipi_csi_last_vin_event;
static volatile uint32_t ra8p1_mipi_csi_last_vin_event_status;
static volatile uint32_t ra8p1_mipi_csi_last_vin_interrupt_status;
static volatile uint32_t ra8p1_mipi_csi_last_csi_event;
static volatile uint32_t ra8p1_mipi_csi_last_csi_event_idx;
static volatile uint32_t ra8p1_mipi_csi_last_csi_event_data;
static volatile uint32_t ra8p1_mipi_csi_phase;
static volatile uint32_t ra8p1_mipi_csi_pdctrgd_before;
static volatile uint32_t ra8p1_mipi_csi_pdctrgd_after;
static int ra8p1_mipi_csi_last_power_err = FSP_SUCCESS;
static int ra8p1_mipi_csi_last_gpt_open_err = FSP_SUCCESS;
static int ra8p1_mipi_csi_last_gpt_start_err = FSP_SUCCESS;
static int ra8p1_mipi_csi_last_vin_open_err = FSP_SUCCESS;
static int ra8p1_mipi_csi_last_capture_start_err = FSP_SUCCESS;
static int ra8p1_mipi_csi_last_i2c_err = FSP_SUCCESS;
static int ra8p1_mipi_csi_last_switch_err = FSP_SUCCESS;
static int ra8p1_mipi_csi_last_camera_err = FSP_SUCCESS;
static int ra8p1_mipi_csi_last_stream_err = FSP_SUCCESS;
static int ra8p1_mipi_csi_last_test_pattern_err = FSP_SUCCESS;
static bool ra8p1_mipi_csi_ra_i2c_ready;
static uint32_t ra8p1_mipi_csi_last_ra_i2c_status;
static uint32_t ra8p1_mipi_csi_last_ra_i2c_error;
static uint32_t ra8p1_mipi_csi_last_ra_i2c_bytes_transferred;
static uint32_t ra8p1_mipi_csi_last_ra_i2c_bytes_remaining;
static uint32_t ra8p1_mipi_csi_last_ra_i2c_iccr1;
static uint32_t ra8p1_mipi_csi_last_ra_i2c_iccr2;
static uint32_t ra8p1_mipi_csi_last_ra_i2c_icsr2;
static uint32_t ra8p1_mipi_csi_last_ra_i2c_icier;
static uint32_t ra8p1_mipi_csi_last_ra_i2c_icser;
#if defined(RA8P1_CSI_USE_FSP_IIC) && RA8P1_CSI_USE_FSP_IIC
static bool ra8p1_mipi_csi_fsp_i2c_ready;
static volatile i2c_master_event_t ra8p1_mipi_csi_fsp_i2c_event;
#endif
static uint32_t ra8p1_mipi_csi_last_i2c_backend;
static uint32_t ra8p1_mipi_csi_last_i2c_addr;
static uint32_t ra8p1_mipi_csi_last_i2c_reg;
static uint32_t ra8p1_mipi_csi_camera_writes;
static uint32_t ra8p1_mipi_csi_camera_readbacks;
static uint32_t ra8p1_mipi_csi_camera_mismatch_reg;
static uint32_t ra8p1_mipi_csi_camera_mismatch_expected;
static uint32_t ra8p1_mipi_csi_camera_mismatch_actual;
static vin_extended_cfg_t ra8p1_mipi_csi_vin_runtime_extend;
static capture_cfg_t ra8p1_mipi_csi_vin_runtime_cfg;
static bool ra8p1_mipi_csi_vin_runtime_ready;
static uint32_t ra8p1_mipi_csi_output_width;
static uint32_t ra8p1_mipi_csi_output_height;

typedef struct _ra8p1_mipi_csi_probe_result_t {
    int err;
    uint32_t samples;
    uint32_t active_samples;
    uint32_t first_activity_ms;
    uint32_t first_vin_ms;
    uint32_t first_vin_lc;
    uint32_t first_csi_rxst;
    uint32_t first_csi_dlst0;
    uint32_t first_csi_dlst1;
    uint32_t first_csi_vcst0;
    uint32_t vin_ms_or;
    uint32_t vin_ms_last;
    uint32_t vin_lc_max;
    uint32_t vin_ints_or;
    uint32_t csi_rxst_or;
    uint32_t csi_rxst_last;
    uint32_t csi_dlst0_or;
    uint32_t csi_dlst1_or;
    uint32_t csi_vcst0_or;
    uint32_t csi_gsst_or;
    uint32_t vin_callbacks;
    uint32_t csi_callbacks;
    uint32_t phase;
    uint32_t source;
    uint32_t runs;
} ra8p1_mipi_csi_probe_result_t;

static ra8p1_mipi_csi_probe_result_t ra8p1_mipi_csi_last_probe;
static uint32_t ra8p1_mipi_csi_probe_runs;

#define RA8P1_CSI_PRCR_KEY (0xA500U)
#define RA8P1_CSI_PRCR_UNLOCK_LPM (RA8P1_CSI_PRCR_KEY | R_SYSTEM_PRCR_PRC1_Msk)
#define RA8P1_CSI_POWER_WAIT_TIMEOUT (10000000U)
#define RA8P1_CSI_PIN_INDEX_MASK (0xFFU)
#define RA8P1_CSI_I2C_TIMEOUT_MS (1000U)
#define RA8P1_CSI_I2C_DELAY_US (5U)
#define RA8P1_CSI_I2C_SWITCH_ADDR (0x43U)
#define RA8P1_CSI_CAMERA_ADDR (0x3CU)
#define RA8P1_CSI_SWITCH_MIPI_SEL_PIN (6U)
#define RA8P1_CSI_SWITCH_PIN_DIR_REG (0x03U)
#define RA8P1_CSI_SWITCH_OUTPUT_STATE_REG (0x05U)
#define RA8P1_CSI_SWITCH_OUTPUT_ENABLE_REG (0x07U)
#define RA8P1_CSI_OV5640_END (0xFFFFU)
#define RA8P1_CSI_OV5640_WAIT (0xAAAAU)
#define RA8P1_CSI_OV5640_SYS_CTRL0_REG (0x3008U)
#define RA8P1_CSI_OV5640_SYS_CTRL0_SW_PWDN (0x42U)
#define RA8P1_CSI_OV5640_SYS_CTRL0_SW_PWUP (0x02U)
#define RA8P1_CSI_CAMERA_IMAGE_WIDTH (1024U)
#define RA8P1_CSI_CAMERA_IMAGE_HEIGHT (600U)
#define RA8P1_CSI_OV5640_FPS_HTS (1461U)
#define RA8P1_CSI_OV5640_FPS_VTS (1095U)
#define RA8P1_CSI_OUTPUT_IMAGE_WIDTH (RA8P1_CSI_CAMERA_IMAGE_WIDTH)
#define RA8P1_CSI_OUTPUT_IMAGE_HEIGHT (RA8P1_CSI_CAMERA_IMAGE_HEIGHT)
#define RA8P1_CSI_I2C_BACKEND_BITBANG (0U)
#define RA8P1_CSI_I2C_BACKEND_RA_I2C (1U)
#define RA8P1_CSI_I2C_BACKEND_RA_I2C_FALLBACK (2U)
#define RA8P1_CSI_I2C_BACKEND_FSP_IIC (3U)
#ifndef RA8P1_CSI_TRY_RA_I2C
#define RA8P1_CSI_TRY_RA_I2C (0)
#endif
#ifndef RA8P1_CSI_USE_FSP_IIC
#define RA8P1_CSI_USE_FSP_IIC (0)
#endif
#ifndef RA8P1_CSI_XCLK_SETTLE_MS
#define RA8P1_CSI_XCLK_SETTLE_MS (10U)
#endif
#ifndef RA8P1_CSI_RESET_LOW_MS
#define RA8P1_CSI_RESET_LOW_MS (20U)
#endif
#ifndef RA8P1_CSI_RESET_HIGH_MS
#define RA8P1_CSI_RESET_HIGH_MS (20U)
#endif
#ifndef RA8P1_CSI_START_PROBE_WINDOW_MS
#define RA8P1_CSI_START_PROBE_WINDOW_MS (1000U)
#endif
#ifndef RA8P1_CSI_MIPI_SEL_STATE
#define RA8P1_CSI_MIPI_SEL_STATE (1)
#endif
#ifndef RA8P1_CSI_VERIFY_CAMERA_WRITES
#define RA8P1_CSI_VERIFY_CAMERA_WRITES (1)
#endif

#define RA8P1_CSI_PROBE_SOURCE_NONE (0U)
#define RA8P1_CSI_PROBE_SOURCE_REPL (1U)
#define RA8P1_CSI_PROBE_SOURCE_BOOT (2U)

#define RA8P1_CSI_DICT_STORE_INT(dict, key, value) \
    mp_obj_dict_store((dict), MP_OBJ_NEW_QSTR(MP_QSTR_##key), mp_obj_new_int(value))

#define RA8P1_CSI_DICT_STORE_UINT(dict, key, value) \
    mp_obj_dict_store((dict), MP_OBJ_NEW_QSTR(MP_QSTR_##key), mp_obj_new_int_from_uint(value))

static inline uint8_t *ra8p1_mipi_csi_buffer_ptr(mp_int_t idx) {
    if (idx == 0) {
        return vin_image_buffer_1;
    }
    if (idx == 1) {
        return vin_image_buffer_2;
    }
    mp_raise_ValueError(MP_ERROR_TEXT("buffer index must be 0 or 1"));
}

static uint32_t ra8p1_mipi_csi_gptclk_hz(void) {
#if defined(BSP_CFG_GPT_COUNT_CLOCK_SOURCE) && (BSP_CFG_GPT_COUNT_CLOCK_SOURCE == 1) && \
    defined(BSP_CFG_CLOCK_SOURCE) && (BSP_CFG_CLOCK_SOURCE == BSP_CLOCKS_SOURCE_CLOCK_PLL1P) && \
    defined(BSP_CFG_PLL1P_FREQUENCY_HZ) && defined(BSP_CFG_PCLKD_DIV)
    return BSP_CFG_PLL1P_FREQUENCY_HZ / R_FSP_ClockDividerGet(BSP_CFG_PCLKD_DIV);
#else
    uint32_t gptckdiv = R_SYSTEM->GPTCKDIVCR & R_SYSTEM_GPTCKDIVCR_GPTCKDIV_Msk;
    uint32_t divisor = R_FSP_ClockDividerGet(gptckdiv);

#if defined(BSP_CFG_GPTCLK_SOURCE) && defined(BSP_CFG_PLL2R_FREQUENCY_HZ) && \
    (BSP_CFG_GPTCLK_SOURCE == BSP_CLOCKS_SOURCE_CLOCK_PLL2R)
    return BSP_CFG_PLL2R_FREQUENCY_HZ / divisor;
#elif defined(BSP_CFG_GPTCLK_SOURCE) && defined(BSP_CFG_PLL2P_FREQUENCY_HZ) && \
    (BSP_CFG_GPTCLK_SOURCE == BSP_CLOCKS_SOURCE_CLOCK_PLL2P)
    return BSP_CFG_PLL2P_FREQUENCY_HZ / divisor;
#else
    (void)divisor;
    return 0;
#endif
#endif
}

static fsp_err_t ra8p1_mipi_csi_step_do(mp_int_t step);

typedef struct {
    uint16_t reg;
    uint8_t val;
} ra8p1_mipi_csi_sensor_reg_t;

static const ra8p1_mipi_csi_sensor_reg_t ra8p1_mipi_csi_ov5640_mipi[] = {
    {0x3008, 0x42},
    {0x3103, 0x13}, {0x3103, 0x03}, {0x3000, 0x00},
    {0x3004, 0xff}, {0x3002, 0x1c}, {0x3006, 0xc3},
    {0x300e, 0x45}, {0x302e, 0x08}, {0x3034, 0x18},
    {0x501f, 0x00}, {0x4300, 0x32},
    {0x3618, 0x00}, {0x3612, 0x29}, {0x3709, 0x52}, {0x370c, 0x03},
    {0x3820, 0x40}, {0x3821, 0x01},
    {0x3630, 0x36},
    {0x3631, 0x0e}, {0x3632, 0xe2}, {0x3633, 0x12}, {0x3621, 0xe0},
    {0x3704, 0xa0},
    {0x3703, 0x5a}, {0x3715, 0x78}, {0x3717, 0x01}, {0x370b, 0x60}, {0x3705, 0x1a},
    {0x3905, 0x02},
    {0x3906, 0x10}, {0x3901, 0x0a}, {0x3731, 0x12}, {0x3600, 0x08}, {0x3601, 0x33},
    {0x302d, 0x60}, {0x3620, 0x52}, {0x371b, 0x20}, {0x471c, 0x50},
    {0x3a13, 0x43}, {0x3a18, 0x00}, {0x3a19, 0xf8}, {0x3635, 0x13},
    {0x3636, 0x03}, {0x3634, 0x40}, {0x3622, 0x01},
    {0x4001, 0x02}, {0x4004, 0x02}, {0x4005, 0x1a}, {0x5001, 0xa3},
    {0x3503, 0x07}, {0x3500, 0x00}, {0x3501, 0xff}, {0x3502, 0x00},
    {0x350a, 0x00}, {0x350b, 0x20},
    {0x3a0f, 0x30}, {0x3a10, 0x28}, {0x3a1b, 0x30}, {0x3a1e, 0x26},
    {0x3a11, 0x60}, {0x3a1f, 0x14},
    {0x3a18, 0x00}, {0x3a19, 0x7c},
    {0x3406, 0x00}, {0x3400, 0x07}, {0x3401, 0x70},
    {0x3402, 0x04}, {0x3403, 0x00}, {0x3404, 0x05}, {0x3405, 0xc0},
    {0x5480, 0x01}, {0x5481, 0x08}, {0x5482, 0x14}, {0x5483, 0x28},
    {0x5484, 0x51}, {0x5485, 0x65}, {0x5486, 0x71}, {0x5487, 0x7d},
    {0x5488, 0x87}, {0x5489, 0x91}, {0x548a, 0x9a}, {0x548b, 0xaa},
    {0x548c, 0xb8}, {0x548d, 0xcd}, {0x548e, 0xdd}, {0x548f, 0xea},
    {0x5490, 0x1d},
    {0x5580, 0x06}, {0x5583, 0x40}, {0x5584, 0x10}, {0x5589, 0x10},
    {0x558a, 0x00}, {0x558b, 0xf8},
    {0x5800, 0x23}, {0x5801, 0x14}, {0x5802, 0x0f}, {0x5803, 0x0f},
    {0x5804, 0x12}, {0x5805, 0x26}, {0x5806, 0x0c}, {0x5807, 0x08},
    {0x5808, 0x05}, {0x5809, 0x05}, {0x580a, 0x08}, {0x580b, 0x0d},
    {0x580c, 0x08}, {0x580d, 0x03}, {0x580e, 0x00}, {0x580f, 0x00},
    {0x5810, 0x03}, {0x5811, 0x09}, {0x5812, 0x07}, {0x5813, 0x03},
    {0x5814, 0x00}, {0x5815, 0x01}, {0x5816, 0x03}, {0x5817, 0x08},
    {0x5818, 0x0d}, {0x5819, 0x08}, {0x581a, 0x05}, {0x581b, 0x06},
    {0x581c, 0x08}, {0x581d, 0x0e}, {0x581e, 0x29}, {0x581f, 0x17},
    {0x5820, 0x11}, {0x5821, 0x11}, {0x5822, 0x15}, {0x5823, 0x28},
    {0x5824, 0x46}, {0x5825, 0x26}, {0x5826, 0x08}, {0x5827, 0x26},
    {0x5828, 0x64}, {0x5829, 0x26}, {0x582a, 0x24}, {0x582b, 0x22},
    {0x582c, 0x24}, {0x582d, 0x24}, {0x582e, 0x06}, {0x582f, 0x22},
    {0x5830, 0x40}, {0x5831, 0x42}, {0x5832, 0x24}, {0x5833, 0x26},
    {0x5834, 0x24}, {0x5835, 0x22}, {0x5836, 0x22}, {0x5837, 0x26},
    {0x5838, 0x44}, {0x5839, 0x24}, {0x583a, 0x26}, {0x583b, 0x28},
    {0x583c, 0x42}, {0x583d, 0xce}, {0x5000, 0xa7},
    {0x3820, 0x40}, {0x3821, 0x06},
    {0x3814, 0x31}, {0x3815, 0x31}, {0x3803, 0x04},
    {0x3808, (uint8_t)(RA8P1_CSI_CAMERA_IMAGE_WIDTH >> 8)}, {0x3809, (uint8_t)RA8P1_CSI_CAMERA_IMAGE_WIDTH},
    {0x380a, (uint8_t)(RA8P1_CSI_CAMERA_IMAGE_HEIGHT >> 8)}, {0x380b, (uint8_t)RA8P1_CSI_CAMERA_IMAGE_HEIGHT},
    {0x460c, 0x22}, {0x4837, 0x10}, {0x3824, 0x02}, {0x5001, 0xa3},
    {0x503d, 0x00}, {0x4741, 0x00},
    {RA8P1_CSI_OV5640_END, 0xff},
};

void vin0_callback(capture_callback_args_t *p_args) {
    ra8p1_mipi_csi_vin_callbacks++;
    if (p_args != NULL) {
        ra8p1_mipi_csi_last_vin_event = p_args->event;
        ra8p1_mipi_csi_last_vin_event_status = p_args->event_status;
        ra8p1_mipi_csi_last_vin_interrupt_status = p_args->interrupt_status;
    }
}

void mipi_csi0_callback(mipi_csi_callback_args_t *p_args) {
    ra8p1_mipi_csi_csi_callbacks++;
    if (p_args != NULL) {
        ra8p1_mipi_csi_last_csi_event = p_args->event;
        ra8p1_mipi_csi_last_csi_event_idx = p_args->event_idx;
        switch (p_args->event) {
            case MIPI_CSI_EVENT_FRAME_DATA:
                ra8p1_mipi_csi_last_csi_event_data = p_args->event_data.receive_status.mask;
                break;
            case MIPI_CSI_EVENT_DATA_LANE:
                ra8p1_mipi_csi_last_csi_event_data = p_args->event_data.data_lane_status.mask;
                break;
            case MIPI_CSI_EVENT_VIRTUAL_CHANNEL:
                ra8p1_mipi_csi_last_csi_event_data = p_args->event_data.virtual_channel_status.mask;
                break;
            case MIPI_CSI_EVENT_POWER:
                ra8p1_mipi_csi_last_csi_event_data = p_args->event_data.power_status.mask;
                break;
            case MIPI_CSI_EVENT_SHORT_PACKET_FIFO:
                ra8p1_mipi_csi_last_csi_event_data = p_args->event_data.fifo_status.mask;
                break;
        }
    }
}

static bool ra8p1_mipi_csi_ok_or_open(fsp_err_t err) {
    return (err == FSP_SUCCESS) || (err == FSP_ERR_ALREADY_OPEN);
}

static void ra8p1_mipi_csi_reset_runtime_state(void) {
    ra8p1_mipi_csi_vin_callbacks = 0;
    ra8p1_mipi_csi_csi_callbacks = 0;
    ra8p1_mipi_csi_last_vin_event = 0;
    ra8p1_mipi_csi_last_vin_event_status = 0;
    ra8p1_mipi_csi_last_vin_interrupt_status = 0;
    ra8p1_mipi_csi_last_csi_event = 0;
    ra8p1_mipi_csi_last_csi_event_idx = 0;
    ra8p1_mipi_csi_last_csi_event_data = 0;
    ra8p1_mipi_csi_vin_runtime_ready = false;
    ra8p1_mipi_csi_output_width = VIN_IMAGE_WIDTH;
    ra8p1_mipi_csi_output_height = VIN_IMAGE_HEIGHT;
}

static fsp_err_t ra8p1_mipi_csi_vin_scale_image(uint16_t new_width, uint16_t new_height) {
    if ((new_width == 0U) || (new_height == 0U)) {
        return FSP_ERR_INVALID_ARGUMENT;
    }

    ra8p1_mipi_csi_vin_runtime_extend = g_vin0_cfg_extend;

    const uint32_t old_v = (uint32_t)g_vin0_cfg_extend.conversion_data.uds_clipping_bits.cl_vsize;
    const uint32_t old_h = (uint32_t)g_vin0_cfg_extend.conversion_data.uds_clipping_bits.cl_hsize;
    const uint16_t old_vm = (uint16_t)g_vin0_cfg_extend.conversion_data.uds_scale_bits.vertical_mask;
    const uint16_t old_hm = (uint16_t)g_vin0_cfg_extend.conversion_data.uds_scale_bits.horizontal_mask;

    if ((old_v == 0U) || (old_h == 0U)) {
        return FSP_ERR_INVALID_ARGUMENT;
    }

    const uint16_t input_height = (uint16_t)g_vin0_cfg_extend.input_ctrl.preclip.line_end + 1U;
    const uint16_t input_width = (uint16_t)g_vin0_cfg_extend.input_ctrl.preclip.pixel_end + 1U;

    ra8p1_mipi_csi_vin_runtime_extend.conversion_data.uds_scale_bits.vertical_mask =
        (uint16_t)(((uint32_t)old_vm * old_v) / new_height);
    ra8p1_mipi_csi_vin_runtime_extend.conversion_data.uds_scale_bits.horizontal_mask =
        (uint16_t)(((uint32_t)old_hm * old_h) / new_width);
    ra8p1_mipi_csi_vin_runtime_extend.conversion_data.uds_clipping_bits.cl_vsize = new_height;
    ra8p1_mipi_csi_vin_runtime_extend.conversion_data.uds_clipping_bits.cl_hsize = new_width;
    ra8p1_mipi_csi_vin_runtime_extend.input_ctrl.cfg_bits.scaling_enable =
        ((input_height != new_height) || (input_width != new_width));

    ra8p1_mipi_csi_vin_runtime_cfg = g_vin0_cfg;
    ra8p1_mipi_csi_vin_runtime_cfg.p_extend = &ra8p1_mipi_csi_vin_runtime_extend;
    ra8p1_mipi_csi_vin_runtime_ready = true;
    ra8p1_mipi_csi_output_width = new_width;
    ra8p1_mipi_csi_output_height = new_height;
    return FSP_SUCCESS;
}

static capture_cfg_t const *ra8p1_mipi_csi_vin_active_cfg(void) {
    return ra8p1_mipi_csi_vin_runtime_ready ? &ra8p1_mipi_csi_vin_runtime_cfg : g_vin0.p_cfg;
}

static void ra8p1_mipi_csi_clear_frame_buffers(void) {
    memset(vin_image_buffer_1, 0, VIN_BYTES_PER_FRAME);
    memset(vin_image_buffer_2, 0, VIN_BYTES_PER_FRAME);
    memset(vin_image_buffer_3, 0, VIN_BYTES_PER_FRAME);
}

static uint32_t ra8p1_mipi_csi_pin_pfs(bsp_io_port_pin_t pin) {
    return R_PFS->PORT[pin >> 8].PIN[pin & RA8P1_CSI_PIN_INDEX_MASK].PmnPFS;
}

static void ra8p1_mipi_csi_i2c_peripheral_configure(void) {
    R_BSP_PinCfg(BSP_IO_PORT_05_PIN_11,
        (uint32_t)IOPORT_CFG_NMOS_ENABLE |
        (uint32_t)IOPORT_CFG_DRIVE_MID |
        (uint32_t)IOPORT_CFG_PERIPHERAL_PIN |
        (uint32_t)IOPORT_PERIPHERAL_IIC);
    R_BSP_PinCfg(BSP_IO_PORT_05_PIN_12,
        (uint32_t)IOPORT_CFG_NMOS_ENABLE |
        (uint32_t)IOPORT_CFG_DRIVE_MID |
        (uint32_t)IOPORT_CFG_PERIPHERAL_PIN |
        (uint32_t)IOPORT_PERIPHERAL_IIC);
}

#if RA8P1_CSI_USE_FSP_IIC
static void ra8p1_mipi_csi_fsp_i2c_callback(i2c_master_callback_args_t *p_args);

static iic_master_instance_ctrl_t ra8p1_mipi_csi_fsp_i2c_ctrl;
static const iic_master_extended_cfg_t ra8p1_mipi_csi_fsp_i2c_extend = {
    .timeout_mode = IIC_MASTER_TIMEOUT_MODE_SHORT,
    .timeout_scl_low = IIC_MASTER_TIMEOUT_SCL_LOW_ENABLED,
    .clock_settings.brl_value = 15,
    .clock_settings.brh_value = 15,
    .clock_settings.cks_value = 2,
    .smbus_operation = false,
};
static const i2c_master_cfg_t ra8p1_mipi_csi_fsp_i2c_cfg = {
    .channel = 1,
    .rate = I2C_MASTER_RATE_FAST,
    .slave = 0x00,
    .addr_mode = I2C_MASTER_ADDR_MODE_7BIT,
    .ipl = 4,
#if defined(VECTOR_NUMBER_IIC1_RXI)
    .rxi_irq = VECTOR_NUMBER_IIC1_RXI,
#else
    .rxi_irq = FSP_INVALID_VECTOR,
#endif
#if defined(VECTOR_NUMBER_IIC1_TXI)
    .txi_irq = VECTOR_NUMBER_IIC1_TXI,
#else
    .txi_irq = FSP_INVALID_VECTOR,
#endif
#if defined(VECTOR_NUMBER_IIC1_TEI)
    .tei_irq = VECTOR_NUMBER_IIC1_TEI,
#else
    .tei_irq = FSP_INVALID_VECTOR,
#endif
#if defined(VECTOR_NUMBER_IIC1_ERI)
    .eri_irq = VECTOR_NUMBER_IIC1_ERI,
#else
    .eri_irq = FSP_INVALID_VECTOR,
#endif
    .p_transfer_tx = NULL,
    .p_transfer_rx = NULL,
    .p_callback = ra8p1_mipi_csi_fsp_i2c_callback,
    .p_context = NULL,
    .p_extend = &ra8p1_mipi_csi_fsp_i2c_extend,
};
#endif

static void ra8p1_mipi_csi_pins_configure(void) {
    R_BSP_PinAccessEnable();
    R_BSP_PinCfg(BSP_IO_PORT_01_PIN_11,
        (uint32_t)IOPORT_CFG_IRQ_ENABLE |
        (uint32_t)IOPORT_CFG_PULLUP_ENABLE |
        (uint32_t)IOPORT_CFG_PORT_DIRECTION_INPUT);
    R_BSP_PinCfg(BSP_IO_PORT_05_PIN_01,
        (uint32_t)IOPORT_CFG_DRIVE_HS_HIGH |
        (uint32_t)IOPORT_CFG_PERIPHERAL_PIN |
        (uint32_t)IOPORT_PERIPHERAL_GPT1);
    ra8p1_mipi_csi_i2c_peripheral_configure();
    R_BSP_PinCfg(BSP_IO_PORT_07_PIN_09,
        (uint32_t)IOPORT_CFG_DRIVE_HIGH |
        (uint32_t)IOPORT_CFG_PORT_DIRECTION_OUTPUT |
        (uint32_t)IOPORT_CFG_PORT_OUTPUT_HIGH);
    R_BSP_PinAccessDisable();
}

static void ra8p1_mipi_csi_i2c_gpio_configure(void) {
    R_BSP_PinCfg(BSP_IO_PORT_05_PIN_11,
        (uint32_t)IOPORT_CFG_NMOS_ENABLE |
        (uint32_t)IOPORT_CFG_DRIVE_MID |
        (uint32_t)IOPORT_CFG_PORT_DIRECTION_OUTPUT |
        (uint32_t)IOPORT_CFG_PORT_OUTPUT_HIGH);
    R_BSP_PinCfg(BSP_IO_PORT_05_PIN_12,
        (uint32_t)IOPORT_CFG_NMOS_ENABLE |
        (uint32_t)IOPORT_CFG_DRIVE_MID |
        (uint32_t)IOPORT_CFG_PORT_DIRECTION_OUTPUT |
        (uint32_t)IOPORT_CFG_PORT_OUTPUT_HIGH);
}

static void ra8p1_mipi_csi_i2c_delay(void) {
    R_BSP_SoftwareDelay(RA8P1_CSI_I2C_DELAY_US, BSP_DELAY_UNITS_MICROSECONDS);
}

static void ra8p1_mipi_csi_i2c_scl_write(bool high) {
    R_BSP_PinWrite(BSP_IO_PORT_05_PIN_12, high ? BSP_IO_LEVEL_HIGH : BSP_IO_LEVEL_LOW);
}

static void ra8p1_mipi_csi_i2c_sda_write(bool high) {
    R_BSP_PinWrite(BSP_IO_PORT_05_PIN_11, high ? BSP_IO_LEVEL_HIGH : BSP_IO_LEVEL_LOW);
}

static bool ra8p1_mipi_csi_i2c_sda_read(void) {
    return R_BSP_PinRead(BSP_IO_PORT_05_PIN_11) != 0U;
}

static bool ra8p1_mipi_csi_i2c_scl_read(void) {
    return R_BSP_PinRead(BSP_IO_PORT_05_PIN_12) != 0U;
}

static bool ra8p1_mipi_csi_cam_xclk_pin_read(void) {
    return R_BSP_PinRead(BSP_IO_PORT_05_PIN_01) != 0U;
}

static bool ra8p1_mipi_csi_mipi_int_pin_read(void) {
    return R_BSP_PinRead(BSP_IO_PORT_01_PIN_11) != 0U;
}

static bool ra8p1_mipi_csi_camera_reset_pin_read(void) {
    return R_BSP_PinRead(BSP_IO_PORT_07_PIN_09) != 0U;
}

static fsp_err_t ra8p1_mipi_csi_i2c_scl_high(void) {
    ra8p1_mipi_csi_i2c_scl_write(true);
    for (uint32_t i = 0; i < 1000U; i++) {
        if (ra8p1_mipi_csi_i2c_scl_read()) {
            ra8p1_mipi_csi_i2c_delay();
            return FSP_SUCCESS;
        }
        ra8p1_mipi_csi_i2c_delay();
    }
    return FSP_ERR_TIMEOUT;
}

static void ra8p1_mipi_csi_i2c_start(void) {
    ra8p1_mipi_csi_i2c_sda_write(true);
    ra8p1_mipi_csi_i2c_scl_high();
    ra8p1_mipi_csi_i2c_sda_write(false);
    ra8p1_mipi_csi_i2c_delay();
    ra8p1_mipi_csi_i2c_scl_write(false);
    ra8p1_mipi_csi_i2c_delay();
}

static void ra8p1_mipi_csi_i2c_stop(void) {
    ra8p1_mipi_csi_i2c_sda_write(false);
    ra8p1_mipi_csi_i2c_delay();
    ra8p1_mipi_csi_i2c_scl_high();
    ra8p1_mipi_csi_i2c_sda_write(true);
    ra8p1_mipi_csi_i2c_delay();
}

static fsp_err_t ra8p1_mipi_csi_i2c_write_byte(uint8_t byte) {
    for (uint32_t bit = 0; bit < 8U; bit++) {
        ra8p1_mipi_csi_i2c_sda_write((byte & 0x80U) != 0U);
        ra8p1_mipi_csi_i2c_delay();
        fsp_err_t err = ra8p1_mipi_csi_i2c_scl_high();
        if (err != FSP_SUCCESS) {
            return err;
        }
        ra8p1_mipi_csi_i2c_scl_write(false);
        ra8p1_mipi_csi_i2c_delay();
        byte <<= 1;
    }

    ra8p1_mipi_csi_i2c_sda_write(true);
    ra8p1_mipi_csi_i2c_delay();
    fsp_err_t err = ra8p1_mipi_csi_i2c_scl_high();
    if (err != FSP_SUCCESS) {
        return err;
    }
    bool ack = !ra8p1_mipi_csi_i2c_sda_read();
    ra8p1_mipi_csi_i2c_scl_write(false);
    ra8p1_mipi_csi_i2c_delay();
    return ack ? FSP_SUCCESS : FSP_ERR_ABORTED;
}

static fsp_err_t ra8p1_mipi_csi_i2c_read_byte(uint8_t *out, bool ack) {
    uint8_t value = 0;
    ra8p1_mipi_csi_i2c_sda_write(true);
    for (uint32_t bit = 0; bit < 8U; bit++) {
        value <<= 1;
        fsp_err_t err = ra8p1_mipi_csi_i2c_scl_high();
        if (err != FSP_SUCCESS) {
            return err;
        }
        if (ra8p1_mipi_csi_i2c_sda_read()) {
            value |= 1U;
        }
        ra8p1_mipi_csi_i2c_scl_write(false);
        ra8p1_mipi_csi_i2c_delay();
    }

    ra8p1_mipi_csi_i2c_sda_write(!ack);
    ra8p1_mipi_csi_i2c_delay();
    fsp_err_t err = ra8p1_mipi_csi_i2c_scl_high();
    if (err != FSP_SUCCESS) {
        return err;
    }
    ra8p1_mipi_csi_i2c_scl_write(false);
    ra8p1_mipi_csi_i2c_sda_write(true);
    ra8p1_mipi_csi_i2c_delay();
    *out = value;
    return FSP_SUCCESS;
}

static void ra8p1_mipi_csi_i2c_begin(void) {
    R_BSP_PinAccessEnable();
    ra8p1_mipi_csi_i2c_gpio_configure();
    ra8p1_mipi_csi_i2c_delay();
}

static void ra8p1_mipi_csi_i2c_end(void) {
    ra8p1_mipi_csi_i2c_peripheral_configure();
    R_BSP_PinAccessDisable();
}

static fsp_err_t ra8p1_mipi_csi_ra_i2c_error(xaction_error_t error) {
    switch (error) {
        case RA_I2C_ERROR_OK:
            return FSP_SUCCESS;
        case RA_I2C_ERROR_TMOF:
            return FSP_ERR_TIMEOUT;
        case RA_I2C_ERROR_BUSY:
            return FSP_ERR_IN_USE;
        case RA_I2C_ERROR_AL:
        case RA_I2C_ERROR_NACK:
        default:
            return FSP_ERR_ABORTED;
    }
}

static void ra8p1_mipi_csi_ra_i2c_open(void) {
    if (ra8p1_mipi_csi_ra_i2c_ready) {
        return;
    }

    ra_i2c_init(R_IIC1, P512, P511, 400000);
    ra_i2c_priority(R_IIC1, 0);
    R_BSP_PinAccessEnable();
    ra8p1_mipi_csi_i2c_peripheral_configure();
    R_BSP_PinAccessDisable();
    ra8p1_mipi_csi_ra_i2c_ready = true;
}

static fsp_err_t ra8p1_mipi_csi_ra_i2c_transfer(uint8_t addr, uint8_t *data, size_t len, bool read, bool stop) {
    if (len == 0) {
        return FSP_SUCCESS;
    }

    xaction_t action;
    xaction_unit_t unit;
    ra_i2c_xunit_init(&unit, data, (uint32_t)len, read, NULL);
    ra_i2c_xaction_init(&action, &unit, 1, (uint32_t)addr, stop);
    if (ra_i2c_action_execute(R_IIC1, &action, false, RA8P1_CSI_I2C_TIMEOUT_MS)) {
        ra8p1_mipi_csi_last_ra_i2c_status = action.m_status;
        ra8p1_mipi_csi_last_ra_i2c_error = action.m_error;
        ra8p1_mipi_csi_last_ra_i2c_bytes_transferred = unit.m_bytes_transferred;
        ra8p1_mipi_csi_last_ra_i2c_bytes_remaining = unit.m_bytes_transfer;
        ra8p1_mipi_csi_last_ra_i2c_iccr1 = R_IIC1->ICCR1;
        ra8p1_mipi_csi_last_ra_i2c_iccr2 = R_IIC1->ICCR2;
        ra8p1_mipi_csi_last_ra_i2c_icsr2 = R_IIC1->ICSR2;
        ra8p1_mipi_csi_last_ra_i2c_icier = R_IIC1->ICIER;
        ra8p1_mipi_csi_last_ra_i2c_icser = R_IIC1->ICSER;
        return FSP_SUCCESS;
    }
    ra8p1_mipi_csi_last_ra_i2c_status = action.m_status;
    ra8p1_mipi_csi_last_ra_i2c_error = action.m_error;
    ra8p1_mipi_csi_last_ra_i2c_bytes_transferred = unit.m_bytes_transferred;
    ra8p1_mipi_csi_last_ra_i2c_bytes_remaining = unit.m_bytes_transfer;
    ra8p1_mipi_csi_last_ra_i2c_iccr1 = R_IIC1->ICCR1;
    ra8p1_mipi_csi_last_ra_i2c_iccr2 = R_IIC1->ICCR2;
    ra8p1_mipi_csi_last_ra_i2c_icsr2 = R_IIC1->ICSR2;
    ra8p1_mipi_csi_last_ra_i2c_icier = R_IIC1->ICIER;
    ra8p1_mipi_csi_last_ra_i2c_icser = R_IIC1->ICSER;
    return ra8p1_mipi_csi_ra_i2c_error(action.m_error);
}

static fsp_err_t ra8p1_mipi_csi_ra_i2c_write(uint8_t addr, const uint8_t *data, size_t len) {
    ra8p1_mipi_csi_ra_i2c_open();
    return ra8p1_mipi_csi_ra_i2c_transfer(addr, (uint8_t *)data, len, false, true);
}

static fsp_err_t ra8p1_mipi_csi_ra_i2c_read_reg8(uint8_t addr, uint8_t reg, uint8_t *out) {
    ra8p1_mipi_csi_ra_i2c_open();
    fsp_err_t err = ra8p1_mipi_csi_ra_i2c_transfer(addr, &reg, 1, false, false);
    if (err == FSP_SUCCESS) {
        err = ra8p1_mipi_csi_ra_i2c_transfer(addr, out, 1, true, true);
    }
    return err;
}

static fsp_err_t ra8p1_mipi_csi_ra_i2c_read_reg16(uint8_t addr, uint16_t reg, uint8_t *out) {
    uint8_t reg_bytes[2] = { (uint8_t)(reg >> 8), (uint8_t)reg };
    ra8p1_mipi_csi_ra_i2c_open();
    fsp_err_t err = ra8p1_mipi_csi_ra_i2c_transfer(addr, reg_bytes, sizeof(reg_bytes), false, false);
    if (err == FSP_SUCCESS) {
        err = ra8p1_mipi_csi_ra_i2c_transfer(addr, out, 1, true, true);
    }
    return err;
}

#if RA8P1_CSI_USE_FSP_IIC
static void ra8p1_mipi_csi_fsp_i2c_callback(i2c_master_callback_args_t *p_args) {
    if (p_args != NULL) {
        ra8p1_mipi_csi_fsp_i2c_event = p_args->event;
    }
}

static fsp_err_t ra8p1_mipi_csi_fsp_i2c_wait(i2c_master_event_t expected_event) {
    for (uint32_t timeout = 0; timeout < 100000U; timeout++) {
        if (ra8p1_mipi_csi_fsp_i2c_event == expected_event) {
            return FSP_SUCCESS;
        }
        if (ra8p1_mipi_csi_fsp_i2c_event == I2C_MASTER_EVENT_ABORTED) {
            return FSP_ERR_TRANSFER_ABORTED;
        }
        R_BSP_SoftwareDelay(10, BSP_DELAY_UNITS_MICROSECONDS);
    }
    if (ra8p1_mipi_csi_fsp_i2c_ready) {
        (void)R_IIC_MASTER_Abort((i2c_master_ctrl_t *)&ra8p1_mipi_csi_fsp_i2c_ctrl);
    }
    return FSP_ERR_TIMEOUT;
}

static fsp_err_t ra8p1_mipi_csi_fsp_i2c_open(void) {
    if (ra8p1_mipi_csi_fsp_i2c_ready) {
        return FSP_SUCCESS;
    }

    R_BSP_PinAccessEnable();
    ra8p1_mipi_csi_i2c_peripheral_configure();
    R_BSP_PinAccessDisable();
    ra8p1_mipi_csi_fsp_i2c_event = (i2c_master_event_t)0;

    fsp_err_t err = R_IIC_MASTER_Open((i2c_master_ctrl_t *)&ra8p1_mipi_csi_fsp_i2c_ctrl,
        &ra8p1_mipi_csi_fsp_i2c_cfg);
    if (err == FSP_SUCCESS) {
        ra8p1_mipi_csi_fsp_i2c_ready = true;
    }
    return err;
}

static fsp_err_t ra8p1_mipi_csi_fsp_i2c_set_addr(uint8_t addr) {
    fsp_err_t err = ra8p1_mipi_csi_fsp_i2c_open();
    if (err == FSP_SUCCESS) {
        err = R_IIC_MASTER_SlaveAddressSet((i2c_master_ctrl_t *)&ra8p1_mipi_csi_fsp_i2c_ctrl,
            addr, I2C_MASTER_ADDR_MODE_7BIT);
    }
    return err;
}

static fsp_err_t ra8p1_mipi_csi_fsp_i2c_write(uint8_t addr, const uint8_t *data, size_t len) {
    fsp_err_t err = ra8p1_mipi_csi_fsp_i2c_set_addr(addr);
    if (err != FSP_SUCCESS) {
        return err;
    }

    ra8p1_mipi_csi_fsp_i2c_event = (i2c_master_event_t)0;
    err = R_IIC_MASTER_Write((i2c_master_ctrl_t *)&ra8p1_mipi_csi_fsp_i2c_ctrl, (uint8_t *)data, len, false);
    if (err == FSP_SUCCESS) {
        err = ra8p1_mipi_csi_fsp_i2c_wait(I2C_MASTER_EVENT_TX_COMPLETE);
    }
    return err;
}

static fsp_err_t ra8p1_mipi_csi_fsp_i2c_read(uint8_t addr, uint8_t *reg, size_t reg_len, uint8_t *out) {
    fsp_err_t err = ra8p1_mipi_csi_fsp_i2c_set_addr(addr);
    if (err != FSP_SUCCESS) {
        return err;
    }

    ra8p1_mipi_csi_fsp_i2c_event = (i2c_master_event_t)0;
    err = R_IIC_MASTER_Write((i2c_master_ctrl_t *)&ra8p1_mipi_csi_fsp_i2c_ctrl, reg, reg_len, true);
    if (err == FSP_SUCCESS) {
        err = ra8p1_mipi_csi_fsp_i2c_wait(I2C_MASTER_EVENT_TX_COMPLETE);
    }
    if (err == FSP_SUCCESS) {
        ra8p1_mipi_csi_fsp_i2c_event = (i2c_master_event_t)0;
        err = R_IIC_MASTER_Read((i2c_master_ctrl_t *)&ra8p1_mipi_csi_fsp_i2c_ctrl, out, 1, false);
    }
    if (err == FSP_SUCCESS) {
        err = ra8p1_mipi_csi_fsp_i2c_wait(I2C_MASTER_EVENT_RX_COMPLETE);
    }
    return err;
}

static fsp_err_t ra8p1_mipi_csi_fsp_i2c_read_reg8(uint8_t addr, uint8_t reg, uint8_t *out) {
    return ra8p1_mipi_csi_fsp_i2c_read(addr, &reg, 1, out);
}

static fsp_err_t ra8p1_mipi_csi_fsp_i2c_read_reg16(uint8_t addr, uint16_t reg, uint8_t *out) {
    uint8_t reg_bytes[2] = { (uint8_t)(reg >> 8), (uint8_t)reg };
    return ra8p1_mipi_csi_fsp_i2c_read(addr, reg_bytes, sizeof(reg_bytes), out);
}
#endif

static fsp_err_t ra8p1_mipi_csi_i2c_write_bitbang(uint8_t addr, const uint8_t *data, size_t len) {
    fsp_err_t err = FSP_SUCCESS;
    ra8p1_mipi_csi_i2c_begin();
    ra8p1_mipi_csi_i2c_start();
    err = ra8p1_mipi_csi_i2c_write_byte((uint8_t)(addr << 1));
    for (size_t i = 0; (i < len) && (err == FSP_SUCCESS); i++) {
        err = ra8p1_mipi_csi_i2c_write_byte(data[i]);
    }
    ra8p1_mipi_csi_i2c_stop();
    ra8p1_mipi_csi_i2c_end();
    return err;
}

static fsp_err_t ra8p1_mipi_csi_i2c_read_reg8_bitbang(uint8_t addr, uint8_t reg, uint8_t *out) {
    fsp_err_t err = FSP_SUCCESS;
    ra8p1_mipi_csi_i2c_begin();
    ra8p1_mipi_csi_i2c_start();
    err = ra8p1_mipi_csi_i2c_write_byte((uint8_t)(addr << 1));
    if (err == FSP_SUCCESS) {
        err = ra8p1_mipi_csi_i2c_write_byte(reg);
    }
    if (err == FSP_SUCCESS) {
        ra8p1_mipi_csi_i2c_start();
        err = ra8p1_mipi_csi_i2c_write_byte((uint8_t)((addr << 1) | 1U));
    }
    if (err == FSP_SUCCESS) {
        err = ra8p1_mipi_csi_i2c_read_byte(out, false);
    }
    ra8p1_mipi_csi_i2c_stop();
    ra8p1_mipi_csi_i2c_end();
    return err;
}

static fsp_err_t ra8p1_mipi_csi_i2c_read_reg16_bitbang(uint8_t addr, uint16_t reg, uint8_t *out) {
    fsp_err_t err = FSP_SUCCESS;
    ra8p1_mipi_csi_i2c_begin();
    ra8p1_mipi_csi_i2c_start();
    err = ra8p1_mipi_csi_i2c_write_byte((uint8_t)(addr << 1));
    if (err == FSP_SUCCESS) {
        err = ra8p1_mipi_csi_i2c_write_byte((uint8_t)(reg >> 8));
    }
    if (err == FSP_SUCCESS) {
        err = ra8p1_mipi_csi_i2c_write_byte((uint8_t)reg);
    }
    if (err == FSP_SUCCESS) {
        ra8p1_mipi_csi_i2c_start();
        err = ra8p1_mipi_csi_i2c_write_byte((uint8_t)((addr << 1) | 1U));
    }
    if (err == FSP_SUCCESS) {
        err = ra8p1_mipi_csi_i2c_read_byte(out, false);
    }
    ra8p1_mipi_csi_i2c_stop();
    ra8p1_mipi_csi_i2c_end();
    return err;
}

static fsp_err_t ra8p1_mipi_csi_i2c_write(uint8_t addr, const uint8_t *data, size_t len) {
    ra8p1_mipi_csi_last_i2c_addr = addr;
    ra8p1_mipi_csi_last_i2c_reg = (len > 0) ? data[0] : 0U;
#if RA8P1_CSI_USE_FSP_IIC
    fsp_err_t err = ra8p1_mipi_csi_fsp_i2c_write(addr, data, len);
    ra8p1_mipi_csi_last_i2c_backend = RA8P1_CSI_I2C_BACKEND_FSP_IIC;
#elif !RA8P1_CSI_TRY_RA_I2C
    fsp_err_t err = ra8p1_mipi_csi_i2c_write_bitbang(addr, data, len);
    ra8p1_mipi_csi_last_i2c_backend = RA8P1_CSI_I2C_BACKEND_BITBANG;
#else
    fsp_err_t err = ra8p1_mipi_csi_ra_i2c_write(addr, data, len);
    ra8p1_mipi_csi_last_i2c_backend = RA8P1_CSI_I2C_BACKEND_RA_I2C;
    if (err != FSP_SUCCESS) {
        err = ra8p1_mipi_csi_i2c_write_bitbang(addr, data, len);
        ra8p1_mipi_csi_last_i2c_backend = RA8P1_CSI_I2C_BACKEND_RA_I2C_FALLBACK;
    }
#endif
    ra8p1_mipi_csi_last_i2c_err = err;
    return err;
}

static fsp_err_t ra8p1_mipi_csi_i2c_read_reg8(uint8_t addr, uint8_t reg, uint8_t *out) {
    ra8p1_mipi_csi_last_i2c_addr = addr;
    ra8p1_mipi_csi_last_i2c_reg = reg;
#if RA8P1_CSI_USE_FSP_IIC
    fsp_err_t err = ra8p1_mipi_csi_fsp_i2c_read_reg8(addr, reg, out);
    ra8p1_mipi_csi_last_i2c_backend = RA8P1_CSI_I2C_BACKEND_FSP_IIC;
#elif !RA8P1_CSI_TRY_RA_I2C
    fsp_err_t err = ra8p1_mipi_csi_i2c_read_reg8_bitbang(addr, reg, out);
    ra8p1_mipi_csi_last_i2c_backend = RA8P1_CSI_I2C_BACKEND_BITBANG;
#else
    fsp_err_t err = ra8p1_mipi_csi_ra_i2c_read_reg8(addr, reg, out);
    ra8p1_mipi_csi_last_i2c_backend = RA8P1_CSI_I2C_BACKEND_RA_I2C;
    if (err != FSP_SUCCESS) {
        err = ra8p1_mipi_csi_i2c_read_reg8_bitbang(addr, reg, out);
        ra8p1_mipi_csi_last_i2c_backend = RA8P1_CSI_I2C_BACKEND_RA_I2C_FALLBACK;
    }
#endif
    ra8p1_mipi_csi_last_i2c_err = err;
    return err;
}

static fsp_err_t ra8p1_mipi_csi_i2c_read_reg16(uint8_t addr, uint16_t reg, uint8_t *out) {
    ra8p1_mipi_csi_last_i2c_addr = addr;
    ra8p1_mipi_csi_last_i2c_reg = reg;
#if RA8P1_CSI_USE_FSP_IIC
    fsp_err_t err = ra8p1_mipi_csi_fsp_i2c_read_reg16(addr, reg, out);
    ra8p1_mipi_csi_last_i2c_backend = RA8P1_CSI_I2C_BACKEND_FSP_IIC;
#elif !RA8P1_CSI_TRY_RA_I2C
    fsp_err_t err = ra8p1_mipi_csi_i2c_read_reg16_bitbang(addr, reg, out);
    ra8p1_mipi_csi_last_i2c_backend = RA8P1_CSI_I2C_BACKEND_BITBANG;
#else
    fsp_err_t err = ra8p1_mipi_csi_ra_i2c_read_reg16(addr, reg, out);
    ra8p1_mipi_csi_last_i2c_backend = RA8P1_CSI_I2C_BACKEND_RA_I2C;
    if (err != FSP_SUCCESS) {
        err = ra8p1_mipi_csi_i2c_read_reg16_bitbang(addr, reg, out);
        ra8p1_mipi_csi_last_i2c_backend = RA8P1_CSI_I2C_BACKEND_RA_I2C_FALLBACK;
    }
#endif
    ra8p1_mipi_csi_last_i2c_err = err;
    return err;
}

static fsp_err_t ra8p1_mipi_csi_i2c_write_reg8(uint8_t addr, uint8_t reg, uint8_t value) {
    uint8_t data[2] = { reg, value };
    ra8p1_mipi_csi_last_i2c_reg = reg;
    return ra8p1_mipi_csi_i2c_write(addr, data, sizeof(data));
}

static fsp_err_t ra8p1_mipi_csi_i2c_write_reg16(uint8_t addr, uint16_t reg, uint8_t value) {
    uint8_t data[3] = { (uint8_t)(reg >> 8), (uint8_t)reg, value };
    ra8p1_mipi_csi_last_i2c_reg = reg;
    return ra8p1_mipi_csi_i2c_write(addr, data, sizeof(data));
}

static fsp_err_t ra8p1_mipi_csi_i2c_probe_addr(uint8_t addr) {
    fsp_err_t err;
    ra8p1_mipi_csi_last_i2c_addr = addr;
    ra8p1_mipi_csi_last_i2c_reg = 0;
    ra8p1_mipi_csi_i2c_begin();
    ra8p1_mipi_csi_i2c_start();
    err = ra8p1_mipi_csi_i2c_write_byte((uint8_t)(addr << 1));
    ra8p1_mipi_csi_i2c_stop();
    ra8p1_mipi_csi_i2c_end();
    ra8p1_mipi_csi_last_i2c_err = err;
    return err;
}

static fsp_err_t ra8p1_mipi_csi_switch_set_bit(uint8_t reg, uint8_t bit_position, bool value) {
    uint8_t reg_value = 0;
    fsp_err_t err = ra8p1_mipi_csi_i2c_read_reg8(RA8P1_CSI_I2C_SWITCH_ADDR, reg, &reg_value);
    if (err != FSP_SUCCESS) {
        return err;
    }

    if (value) {
        reg_value |= (uint8_t)(1U << bit_position);
    } else {
        reg_value &= (uint8_t)~(1U << bit_position);
    }

    err = ra8p1_mipi_csi_i2c_write_reg8(RA8P1_CSI_I2C_SWITCH_ADDR, reg, reg_value);
    if (err != FSP_SUCCESS) {
        return err;
    }

    err = ra8p1_mipi_csi_i2c_read_reg8(RA8P1_CSI_I2C_SWITCH_ADDR, reg, &reg_value);
    if (err != FSP_SUCCESS) {
        return err;
    }

    return (((reg_value >> bit_position) & 1U) == (value ? 1U : 0U)) ? FSP_SUCCESS : FSP_ERR_ABORTED;
}

static fsp_err_t ra8p1_mipi_csi_switch_select_do(bool mipi_sel_state) {
    uint8_t bit_position = RA8P1_CSI_SWITCH_MIPI_SEL_PIN - 1U;
    fsp_err_t err = ra8p1_mipi_csi_switch_set_bit(RA8P1_CSI_SWITCH_PIN_DIR_REG, bit_position, true);
    if (err == FSP_SUCCESS) {
        err = ra8p1_mipi_csi_switch_set_bit(RA8P1_CSI_SWITCH_OUTPUT_ENABLE_REG, bit_position, false);
    }
    if (err == FSP_SUCCESS) {
        err = ra8p1_mipi_csi_switch_set_bit(RA8P1_CSI_SWITCH_OUTPUT_STATE_REG, bit_position, mipi_sel_state);
    }
    ra8p1_mipi_csi_last_switch_err = err;
    return err;
}

static fsp_err_t ra8p1_mipi_csi_switch_mipi_do(void) {
    return ra8p1_mipi_csi_switch_select_do(RA8P1_CSI_MIPI_SEL_STATE != 0);
}

static void ra8p1_mipi_csi_camera_hw_reset(void) {
    R_BSP_PinAccessEnable();
    R_BSP_PinWrite(BSP_IO_PORT_07_PIN_09, BSP_IO_LEVEL_LOW);
    R_BSP_PinAccessDisable();
    R_BSP_SoftwareDelay(RA8P1_CSI_RESET_LOW_MS, BSP_DELAY_UNITS_MILLISECONDS);

    R_BSP_PinAccessEnable();
    R_BSP_PinWrite(BSP_IO_PORT_07_PIN_09, BSP_IO_LEVEL_HIGH);
    R_BSP_PinAccessDisable();
    R_BSP_SoftwareDelay(RA8P1_CSI_RESET_HIGH_MS, BSP_DELAY_UNITS_MILLISECONDS);
}

static fsp_err_t ra8p1_mipi_csi_camera_write_array(void) {
    fsp_err_t err = FSP_SUCCESS;
    ra8p1_mipi_csi_camera_writes = 0;
    ra8p1_mipi_csi_camera_readbacks = 0;
    ra8p1_mipi_csi_camera_mismatch_reg = 0;
    ra8p1_mipi_csi_camera_mismatch_expected = 0;
    ra8p1_mipi_csi_camera_mismatch_actual = 0;
    for (const ra8p1_mipi_csi_sensor_reg_t *p = ra8p1_mipi_csi_ov5640_mipi; p->reg != RA8P1_CSI_OV5640_END; p++) {
        if (p->reg == RA8P1_CSI_OV5640_WAIT) {
            R_BSP_SoftwareDelay(p->val, BSP_DELAY_UNITS_MILLISECONDS);
            continue;
        }
        err = ra8p1_mipi_csi_i2c_write_reg16(RA8P1_CSI_CAMERA_ADDR, p->reg, p->val);
        if (err != FSP_SUCCESS) {
            ra8p1_mipi_csi_last_camera_err = err;
            return err;
        }
        ra8p1_mipi_csi_camera_writes++;

#if RA8P1_CSI_VERIFY_CAMERA_WRITES
        uint8_t value = 0;
        err = ra8p1_mipi_csi_i2c_read_reg16(RA8P1_CSI_CAMERA_ADDR, p->reg, &value);
        if (err != FSP_SUCCESS) {
            ra8p1_mipi_csi_last_camera_err = err;
            return err;
        }
        ra8p1_mipi_csi_camera_readbacks++;
        if (value != p->val) {
            ra8p1_mipi_csi_camera_mismatch_reg = p->reg;
            ra8p1_mipi_csi_camera_mismatch_expected = p->val;
            ra8p1_mipi_csi_camera_mismatch_actual = value;
            ra8p1_mipi_csi_last_camera_err = FSP_ERR_ASSERTION;
            return FSP_ERR_ASSERTION;
        }
#endif
    }
    return FSP_SUCCESS;
}

static fsp_err_t ra8p1_mipi_csi_camera_configure_clocks(void) {
    fsp_err_t err = ra8p1_mipi_csi_i2c_write_reg16(RA8P1_CSI_CAMERA_ADDR, 0x3035, 0x12);
    if (err == FSP_SUCCESS) {
        err = ra8p1_mipi_csi_i2c_write_reg16(RA8P1_CSI_CAMERA_ADDR, 0x3036, 123);
    }
    if (err == FSP_SUCCESS) {
        err = ra8p1_mipi_csi_i2c_write_reg16(RA8P1_CSI_CAMERA_ADDR, 0x3037, 0x08);
    }
    if (err == FSP_SUCCESS) {
        err = ra8p1_mipi_csi_i2c_write_reg16(RA8P1_CSI_CAMERA_ADDR, 0x3108, 0x12);
    }
    return err;
}

static fsp_err_t ra8p1_mipi_csi_camera_fps_set(void) {
    fsp_err_t err = ra8p1_mipi_csi_i2c_write_reg16(RA8P1_CSI_CAMERA_ADDR, 0x380c,
        (uint8_t)(RA8P1_CSI_OV5640_FPS_HTS >> 8));
    if (err == FSP_SUCCESS) {
        err = ra8p1_mipi_csi_i2c_write_reg16(RA8P1_CSI_CAMERA_ADDR, 0x380d,
            (uint8_t)RA8P1_CSI_OV5640_FPS_HTS);
    }
    if (err == FSP_SUCCESS) {
        err = ra8p1_mipi_csi_i2c_write_reg16(RA8P1_CSI_CAMERA_ADDR, 0x380e,
            (uint8_t)(RA8P1_CSI_OV5640_FPS_VTS >> 8));
    }
    if (err == FSP_SUCCESS) {
        err = ra8p1_mipi_csi_i2c_write_reg16(RA8P1_CSI_CAMERA_ADDR, 0x380f,
            (uint8_t)RA8P1_CSI_OV5640_FPS_VTS);
    }
    return err;
}

static fsp_err_t ra8p1_mipi_csi_camera_set_mipi_vc(uint8_t vc) {
    uint8_t reg_value = 0;
    fsp_err_t err = ra8p1_mipi_csi_i2c_read_reg16(RA8P1_CSI_CAMERA_ADDR, 0x4814, &reg_value);
    if (err == FSP_SUCCESS) {
        reg_value = (uint8_t)((reg_value & ~(3U << 6)) | ((vc & 3U) << 6));
        err = ra8p1_mipi_csi_i2c_write_reg16(RA8P1_CSI_CAMERA_ADDR, 0x4814, reg_value);
    }
    return err;
}

static fsp_err_t ra8p1_mipi_csi_camera_open_do(void) {
    fsp_err_t err = ra8p1_mipi_csi_step_do(1);
    if (!ra8p1_mipi_csi_ok_or_open(err)) {
        ra8p1_mipi_csi_last_camera_err = err;
        return err;
    }

    err = ra8p1_mipi_csi_step_do(2);
    if ((err != FSP_SUCCESS) && (err != FSP_ERR_ALREADY_OPEN)) {
        ra8p1_mipi_csi_last_camera_err = err;
        return err;
    }

    R_BSP_SoftwareDelay(RA8P1_CSI_XCLK_SETTLE_MS, BSP_DELAY_UNITS_MILLISECONDS);
    ra8p1_mipi_csi_camera_hw_reset();
    err = ra8p1_mipi_csi_camera_write_array();
    if (err == FSP_SUCCESS) {
        R_BSP_SoftwareDelay(50, BSP_DELAY_UNITS_MILLISECONDS);
        err = ra8p1_mipi_csi_camera_configure_clocks();
    }
    if (err == FSP_SUCCESS) {
        err = ra8p1_mipi_csi_camera_fps_set();
    }
    if (err == FSP_SUCCESS) {
        err = ra8p1_mipi_csi_camera_set_mipi_vc(0);
    }
    ra8p1_mipi_csi_last_camera_err = err;
    return err;
}

static fsp_err_t ra8p1_mipi_csi_camera_stream_on_do(void) {
    fsp_err_t err = ra8p1_mipi_csi_i2c_write_reg16(RA8P1_CSI_CAMERA_ADDR, RA8P1_CSI_OV5640_SYS_CTRL0_REG,
        RA8P1_CSI_OV5640_SYS_CTRL0_SW_PWUP);
    if (err == FSP_SUCCESS) {
        err = ra8p1_mipi_csi_i2c_write_reg16(RA8P1_CSI_CAMERA_ADDR, 0x4202, 0x00);
    }
    ra8p1_mipi_csi_last_stream_err = err;
    return err;
}

static fsp_err_t ra8p1_mipi_csi_camera_stream_off_do(void) {
    fsp_err_t err = ra8p1_mipi_csi_i2c_write_reg16(RA8P1_CSI_CAMERA_ADDR, 0x4202, 0x0f);
    ra8p1_mipi_csi_last_stream_err = err;
    return err;
}

static fsp_err_t ra8p1_mipi_csi_camera_power_down_do(void) {
    fsp_err_t err = ra8p1_mipi_csi_i2c_write_reg16(RA8P1_CSI_CAMERA_ADDR, RA8P1_CSI_OV5640_SYS_CTRL0_REG,
        RA8P1_CSI_OV5640_SYS_CTRL0_SW_PWDN);
    ra8p1_mipi_csi_last_stream_err = err;
    return err;
}

static fsp_err_t ra8p1_mipi_csi_camera_test_pattern_do(bool enable) {
    fsp_err_t err = ra8p1_mipi_csi_i2c_write_reg16(RA8P1_CSI_CAMERA_ADDR, 0x503d, enable ? 0x80 : 0x00);
    if (err == FSP_SUCCESS) {
        R_BSP_SoftwareDelay(50, BSP_DELAY_UNITS_MILLISECONDS);
    }
    ra8p1_mipi_csi_last_test_pattern_err = err;
    return err;
}

static fsp_err_t ra8p1_mipi_csi_power_domain_up(void) {
    ra8p1_mipi_csi_pdctrgd_before = R_SYSTEM->PDCTRGD;
    if ((R_SYSTEM->PDCTRGD & (R_SYSTEM_PDCTRGD_PDDE_Msk | R_SYSTEM_PDCTRGD_PDCSF_Msk | R_SYSTEM_PDCTRGD_PDPGSF_Msk)) == 0) {
        ra8p1_mipi_csi_pdctrgd_after = R_SYSTEM->PDCTRGD;
        ra8p1_mipi_csi_last_power_err = FSP_SUCCESS;
        return FSP_SUCCESS;
    }

    uint16_t saved_prcr = R_SYSTEM->PRCR;
    R_SYSTEM->PRCR = RA8P1_CSI_PRCR_UNLOCK_LPM;
    R_SYSTEM->PDCTRGD_b.PDDE = 0;

    uint32_t wait_count = RA8P1_CSI_POWER_WAIT_TIMEOUT;
    while (((R_SYSTEM->PDCTRGD & (R_SYSTEM_PDCTRGD_PDCSF_Msk | R_SYSTEM_PDCTRGD_PDPGSF_Msk)) != 0) && wait_count) {
        wait_count--;
    }

    ra8p1_mipi_csi_pdctrgd_after = R_SYSTEM->PDCTRGD;
    R_SYSTEM->PRCR = (uint16_t)(RA8P1_CSI_PRCR_KEY | (saved_prcr & 0x00FFU));

    fsp_err_t err = ((R_SYSTEM->PDCTRGD & (R_SYSTEM_PDCTRGD_PDCSF_Msk | R_SYSTEM_PDCTRGD_PDPGSF_Msk)) == 0) ?
        FSP_SUCCESS : FSP_ERR_HARDWARE_TIMEOUT;
    ra8p1_mipi_csi_last_power_err = err;
    return err;
}

static fsp_err_t ra8p1_mipi_csi_step_do(mp_int_t step) {
    fsp_err_t err;
    switch (step) {
        case 0:
            ra8p1_mipi_csi_phase = 0;
            return FSP_SUCCESS;
        case 1:
            ra8p1_mipi_csi_phase = 10;
            ra8p1_mipi_csi_pins_configure();
            err = g_timer_periodic.p_api->open(g_timer_periodic.p_ctrl, g_timer_periodic.p_cfg);
            ra8p1_mipi_csi_last_gpt_open_err = err;
            if (ra8p1_mipi_csi_ok_or_open(err)) {
                ra8p1_mipi_csi_phase = 11;
            }
            return err;
        case 2:
            ra8p1_mipi_csi_phase = 20;
            err = g_timer_periodic.p_api->start(g_timer_periodic.p_ctrl);
            ra8p1_mipi_csi_last_gpt_start_err = err;
            if (err == FSP_SUCCESS || err == FSP_ERR_ALREADY_OPEN) {
            ra8p1_mipi_csi_phase = 21;
            }
            return err;
        case 3:
            ra8p1_mipi_csi_phase = 29;
            err = ra8p1_mipi_csi_power_domain_up();
            if (err != FSP_SUCCESS) {
                return err;
            }
            ra8p1_mipi_csi_phase = 30;
            err = g_vin0.p_api->open(g_vin0.p_ctrl, ra8p1_mipi_csi_vin_active_cfg());
            ra8p1_mipi_csi_last_vin_open_err = err;
            if (ra8p1_mipi_csi_ok_or_open(err)) {
                ra8p1_mipi_csi_phase = 31;
            }
            return err;
        case 4:
            ra8p1_mipi_csi_phase = 40;
            err = g_vin0.p_api->captureStart(g_vin0.p_ctrl, NULL);
            ra8p1_mipi_csi_last_capture_start_err = err;
            if (err == FSP_SUCCESS || err == FSP_ERR_INVALID_STATE) {
                ra8p1_mipi_csi_phase = 41;
                ra8p1_mipi_csi_open = true;
            }
            return err;
        default:
            return FSP_ERR_INVALID_ARGUMENT;
    }
}

static fsp_err_t ra8p1_mipi_csi_vin_camera_start(capture_cfg_t const *cfg) {
    if (cfg == NULL) {
        return FSP_ERR_INVALID_ARGUMENT;
    }

    fsp_err_t err = ra8p1_mipi_csi_camera_stream_off_do();
    if ((err == FSP_SUCCESS) && (g_vin0_ctrl.open != 0U)) {
        err = g_vin0.p_api->close(g_vin0.p_ctrl);
        ra8p1_mipi_csi_open = false;
    }
    if (err == FSP_SUCCESS) {
        err = ra8p1_mipi_csi_camera_power_down_do();
    }
    if (err == FSP_SUCCESS) {
        err = ra8p1_mipi_csi_power_domain_up();
    }
    if (err == FSP_SUCCESS) {
        ra8p1_mipi_csi_phase = 30;
        err = g_vin0.p_api->open(g_vin0.p_ctrl, cfg);
        ra8p1_mipi_csi_last_vin_open_err = err;
    }
    if (ra8p1_mipi_csi_ok_or_open(err)) {
        ra8p1_mipi_csi_phase = 40;
        err = g_vin0.p_api->captureStart(g_vin0.p_ctrl, NULL);
        ra8p1_mipi_csi_last_capture_start_err = err;
    }
    if ((err == FSP_SUCCESS) || (err == FSP_ERR_INVALID_STATE)) {
        ra8p1_mipi_csi_phase = 41;
        ra8p1_mipi_csi_open = true;
        err = ra8p1_mipi_csi_camera_stream_on_do();
    }
    return err;
}

static fsp_err_t ra8p1_mipi_csi_start_select_do(bool mipi_sel_state) {
    ra8p1_mipi_csi_reset_runtime_state();

    fsp_err_t err = ra8p1_mipi_csi_switch_select_do(mipi_sel_state);
    if (err == FSP_SUCCESS) {
        err = ra8p1_mipi_csi_camera_open_do();
    }
    if (err == FSP_SUCCESS) {
        err = ra8p1_mipi_csi_vin_camera_start(&g_vin0_cfg);
    }
    return err;
}

static fsp_err_t ra8p1_mipi_csi_start_default_do(void) {
    return ra8p1_mipi_csi_start_select_do(RA8P1_CSI_MIPI_SEL_STATE != 0);
}

static void ra8p1_mipi_csi_probe_result_clear(ra8p1_mipi_csi_probe_result_t *result) {
    memset(result, 0, sizeof(*result));
    result->err = FSP_ERR_NOT_OPEN;
    result->first_activity_ms = 0xffffffffU;
    result->source = RA8P1_CSI_PROBE_SOURCE_NONE;
}

static void ra8p1_mipi_csi_probe_sample(fsp_err_t err, uint32_t source) {
    ra8p1_mipi_csi_probe_result_t result;
    ra8p1_mipi_csi_probe_result_clear(&result);

    result.err = err;
    result.source = source;
    result.runs = ++ra8p1_mipi_csi_probe_runs;

    if (err == FSP_SUCCESS) {
        for (uint32_t elapsed_ms = 0; elapsed_ms <= RA8P1_CSI_START_PROBE_WINDOW_MS; elapsed_ms++) {
            uint32_t vin_ms = R_VIN->MS;
            uint32_t vin_lc = R_VIN->LC;
            uint32_t vin_ints = R_VIN->INTS;
            uint32_t csi_rxst = R_MIPI_CSI->RXST;
            uint32_t csi_dlst0 = R_MIPI_CSI->DLST0;
            uint32_t csi_dlst1 = R_MIPI_CSI->DLST1;
            uint32_t csi_vcst0 = R_MIPI_CSI->VCST0;
            uint32_t csi_gsst = R_MIPI_CSI->GSST;
            uint32_t vin_active = vin_ms & (R_VIN_MS_CA_Msk | R_VIN_MS_AV_Msk | R_VIN_MS_MA_Msk);
            uint32_t activity = vin_active | vin_lc | vin_ints | csi_rxst | csi_dlst0 | csi_dlst1 | csi_vcst0 | csi_gsst;

            result.samples++;
            result.vin_ms_or |= vin_ms;
            result.vin_ms_last = vin_ms;
            result.vin_ints_or |= vin_ints;
            result.csi_rxst_or |= csi_rxst;
            result.csi_rxst_last = csi_rxst;
            result.csi_dlst0_or |= csi_dlst0;
            result.csi_dlst1_or |= csi_dlst1;
            result.csi_vcst0_or |= csi_vcst0;
            result.csi_gsst_or |= csi_gsst;
            if (vin_lc > result.vin_lc_max) {
                result.vin_lc_max = vin_lc;
            }

            if (activity != 0U) {
                result.active_samples++;
                if (result.first_activity_ms == 0xffffffffU) {
                    result.first_activity_ms = elapsed_ms;
                    result.first_vin_ms = vin_ms;
                    result.first_vin_lc = vin_lc;
                    result.first_csi_rxst = csi_rxst;
                    result.first_csi_dlst0 = csi_dlst0;
                    result.first_csi_dlst1 = csi_dlst1;
                    result.first_csi_vcst0 = csi_vcst0;
                }
            }

            if (elapsed_ms < RA8P1_CSI_START_PROBE_WINDOW_MS) {
                R_BSP_SoftwareDelay(1, BSP_DELAY_UNITS_MILLISECONDS);
            }
        }
    }

    result.vin_callbacks = ra8p1_mipi_csi_vin_callbacks;
    result.csi_callbacks = ra8p1_mipi_csi_csi_callbacks;
    result.phase = ra8p1_mipi_csi_phase;
    ra8p1_mipi_csi_last_probe = result;
}

static mp_obj_t ra8p1_mipi_csi_probe_result_to_dict(ra8p1_mipi_csi_probe_result_t const *result) {
    mp_obj_t status = mp_obj_new_dict(30);
    RA8P1_CSI_DICT_STORE_INT(status, err, result->err);
    RA8P1_CSI_DICT_STORE_UINT(status, samples, result->samples);
    RA8P1_CSI_DICT_STORE_UINT(status, active_samples, result->active_samples);
    RA8P1_CSI_DICT_STORE_UINT(status, first_activity_ms, result->first_activity_ms);
    RA8P1_CSI_DICT_STORE_UINT(status, first_vin_ms, result->first_vin_ms);
    RA8P1_CSI_DICT_STORE_UINT(status, first_vin_lc, result->first_vin_lc);
    RA8P1_CSI_DICT_STORE_UINT(status, first_csi_rxst, result->first_csi_rxst);
    RA8P1_CSI_DICT_STORE_UINT(status, first_csi_dlst0, result->first_csi_dlst0);
    RA8P1_CSI_DICT_STORE_UINT(status, first_csi_dlst1, result->first_csi_dlst1);
    RA8P1_CSI_DICT_STORE_UINT(status, first_csi_vcst0, result->first_csi_vcst0);
    RA8P1_CSI_DICT_STORE_UINT(status, vin_ms_or, result->vin_ms_or);
    RA8P1_CSI_DICT_STORE_UINT(status, vin_ms_last, result->vin_ms_last);
    RA8P1_CSI_DICT_STORE_UINT(status, vin_lc_max, result->vin_lc_max);
    RA8P1_CSI_DICT_STORE_UINT(status, vin_ints_or, result->vin_ints_or);
    RA8P1_CSI_DICT_STORE_UINT(status, csi_rxst_or, result->csi_rxst_or);
    RA8P1_CSI_DICT_STORE_UINT(status, csi_rxst_last, result->csi_rxst_last);
    RA8P1_CSI_DICT_STORE_UINT(status, csi_dlst0_or, result->csi_dlst0_or);
    RA8P1_CSI_DICT_STORE_UINT(status, csi_dlst1_or, result->csi_dlst1_or);
    RA8P1_CSI_DICT_STORE_UINT(status, csi_vcst0_or, result->csi_vcst0_or);
    RA8P1_CSI_DICT_STORE_UINT(status, csi_gsst_or, result->csi_gsst_or);
    RA8P1_CSI_DICT_STORE_UINT(status, vin_callbacks, result->vin_callbacks);
    RA8P1_CSI_DICT_STORE_UINT(status, csi_callbacks, result->csi_callbacks);
    RA8P1_CSI_DICT_STORE_UINT(status, phase, result->phase);
    RA8P1_CSI_DICT_STORE_UINT(status, probe_source, result->source);
    RA8P1_CSI_DICT_STORE_UINT(status, probe_runs, result->runs);
    return status;
}

void ra8p1_mipi_csi_boot_probe(void) {
    fsp_err_t err = ra8p1_mipi_csi_start_default_do();
    ra8p1_mipi_csi_probe_sample(err, RA8P1_CSI_PROBE_SOURCE_BOOT);
}

static mp_obj_t ra8p1_mipi_csi_step(mp_obj_t step_in) {
    mp_int_t step = mp_obj_get_int(step_in);
    return mp_obj_new_int(ra8p1_mipi_csi_step_do(step));
}
static MP_DEFINE_CONST_FUN_OBJ_1(ra8p1_mipi_csi_step_obj, ra8p1_mipi_csi_step);

static mp_obj_t ra8p1_mipi_csi_i2c_probe(mp_obj_t addr_in) {
    mp_int_t addr = mp_obj_get_int(addr_in);
    if ((addr < 0) || (addr > 0x7f)) {
        mp_raise_ValueError(MP_ERROR_TEXT("addr must be 0..127"));
    }
    return mp_obj_new_bool(ra8p1_mipi_csi_i2c_probe_addr((uint8_t)addr) == FSP_SUCCESS);
}
static MP_DEFINE_CONST_FUN_OBJ_1(ra8p1_mipi_csi_i2c_probe_obj, ra8p1_mipi_csi_i2c_probe);

static mp_obj_t ra8p1_mipi_csi_switch_mipi(void) {
    return mp_obj_new_int(ra8p1_mipi_csi_switch_mipi_do());
}
static MP_DEFINE_CONST_FUN_OBJ_0(ra8p1_mipi_csi_switch_mipi_obj, ra8p1_mipi_csi_switch_mipi);

static mp_obj_t ra8p1_mipi_csi_switch_select(mp_obj_t state_in) {
    return mp_obj_new_int(ra8p1_mipi_csi_switch_select_do(mp_obj_is_true(state_in)));
}
static MP_DEFINE_CONST_FUN_OBJ_1(ra8p1_mipi_csi_switch_select_obj, ra8p1_mipi_csi_switch_select);

static mp_obj_t ra8p1_mipi_csi_switch_read(mp_obj_t reg_in) {
    mp_int_t reg = mp_obj_get_int(reg_in);
    if ((reg < 0) || (reg > 0xff)) {
        mp_raise_ValueError(MP_ERROR_TEXT("reg must be 0..255"));
    }

    uint8_t value = 0;
    fsp_err_t err = ra8p1_mipi_csi_i2c_read_reg8(RA8P1_CSI_I2C_SWITCH_ADDR, (uint8_t)reg, &value);
    if (err != FSP_SUCCESS) {
        return mp_obj_new_int(-((int)err));
    }
    return mp_obj_new_int(value);
}
static MP_DEFINE_CONST_FUN_OBJ_1(ra8p1_mipi_csi_switch_read_obj, ra8p1_mipi_csi_switch_read);

static mp_obj_t ra8p1_mipi_csi_switch_write(mp_obj_t reg_in, mp_obj_t value_in) {
    mp_int_t reg = mp_obj_get_int(reg_in);
    mp_int_t value = mp_obj_get_int(value_in);
    if ((reg < 0) || (reg > 0xff)) {
        mp_raise_ValueError(MP_ERROR_TEXT("reg must be 0..255"));
    }
    if ((value < 0) || (value > 0xff)) {
        mp_raise_ValueError(MP_ERROR_TEXT("value must be 0..255"));
    }

    fsp_err_t err = ra8p1_mipi_csi_i2c_write_reg8(RA8P1_CSI_I2C_SWITCH_ADDR, (uint8_t)reg, (uint8_t)value);
    ra8p1_mipi_csi_last_switch_err = err;
    return mp_obj_new_int(err);
}
static MP_DEFINE_CONST_FUN_OBJ_2(ra8p1_mipi_csi_switch_write_obj, ra8p1_mipi_csi_switch_write);

static mp_obj_t ra8p1_mipi_csi_camera_read(mp_obj_t reg_in) {
    mp_int_t reg = mp_obj_get_int(reg_in);
    if ((reg < 0) || (reg > 0xffff)) {
        mp_raise_ValueError(MP_ERROR_TEXT("reg must be 0..65535"));
    }

    uint8_t value = 0;
    fsp_err_t err = ra8p1_mipi_csi_i2c_read_reg16(RA8P1_CSI_CAMERA_ADDR, (uint16_t)reg, &value);
    if (err != FSP_SUCCESS) {
        return mp_obj_new_int(-((int)err));
    }
    return mp_obj_new_int(value);
}
static MP_DEFINE_CONST_FUN_OBJ_1(ra8p1_mipi_csi_camera_read_obj, ra8p1_mipi_csi_camera_read);

static mp_obj_t ra8p1_mipi_csi_camera_write(mp_obj_t reg_in, mp_obj_t value_in) {
    mp_int_t reg = mp_obj_get_int(reg_in);
    mp_int_t value = mp_obj_get_int(value_in);
    if ((reg < 0) || (reg > 0xffff)) {
        mp_raise_ValueError(MP_ERROR_TEXT("reg must be 0..65535"));
    }
    if ((value < 0) || (value > 0xff)) {
        mp_raise_ValueError(MP_ERROR_TEXT("value must be 0..255"));
    }

    fsp_err_t err = ra8p1_mipi_csi_i2c_write_reg16(RA8P1_CSI_CAMERA_ADDR, (uint16_t)reg, (uint8_t)value);
    ra8p1_mipi_csi_last_camera_err = err;
    return mp_obj_new_int(err);
}
static MP_DEFINE_CONST_FUN_OBJ_2(ra8p1_mipi_csi_camera_write_obj, ra8p1_mipi_csi_camera_write);

static mp_obj_t ra8p1_mipi_csi_reg_read(mp_obj_t addr_in) {
    uintptr_t addr = (uintptr_t)mp_obj_get_int_truncated(addr_in);
    if ((addr & 3U) != 0) {
        mp_raise_ValueError(MP_ERROR_TEXT("addr must be 32-bit aligned"));
    }
    return mp_obj_new_int_from_uint(*(volatile uint32_t *)addr);
}
static MP_DEFINE_CONST_FUN_OBJ_1(ra8p1_mipi_csi_reg_read_obj, ra8p1_mipi_csi_reg_read);

static mp_obj_t ra8p1_mipi_csi_reg_write(mp_obj_t addr_in, mp_obj_t value_in) {
    uintptr_t addr = (uintptr_t)mp_obj_get_int_truncated(addr_in);
    if ((addr & 3U) != 0) {
        mp_raise_ValueError(MP_ERROR_TEXT("addr must be 32-bit aligned"));
    }
    *(volatile uint32_t *)addr = (uint32_t)mp_obj_get_int_truncated(value_in);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(ra8p1_mipi_csi_reg_write_obj, ra8p1_mipi_csi_reg_write);

static mp_obj_t ra8p1_mipi_csi_camera_open(void) {
    return mp_obj_new_int(ra8p1_mipi_csi_camera_open_do());
}
static MP_DEFINE_CONST_FUN_OBJ_0(ra8p1_mipi_csi_camera_open_obj, ra8p1_mipi_csi_camera_open);

static mp_obj_t ra8p1_mipi_csi_stream_on(void) {
    return mp_obj_new_int(ra8p1_mipi_csi_camera_stream_on_do());
}
static MP_DEFINE_CONST_FUN_OBJ_0(ra8p1_mipi_csi_stream_on_obj, ra8p1_mipi_csi_stream_on);

static mp_obj_t ra8p1_mipi_csi_stream_off(void) {
    return mp_obj_new_int(ra8p1_mipi_csi_camera_stream_off_do());
}
static MP_DEFINE_CONST_FUN_OBJ_0(ra8p1_mipi_csi_stream_off_obj, ra8p1_mipi_csi_stream_off);

static mp_obj_t ra8p1_mipi_csi_test_pattern(size_t n_args, const mp_obj_t *args) {
    bool enable = true;
    if (n_args > 0) {
        enable = mp_obj_is_true(args[0]);
    }
    return mp_obj_new_int(ra8p1_mipi_csi_camera_test_pattern_do(enable));
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(ra8p1_mipi_csi_test_pattern_obj, 0, 1, ra8p1_mipi_csi_test_pattern);

static mp_obj_t ra8p1_mipi_csi_start(void) {
    ra8p1_mipi_csi_reset_runtime_state();

    fsp_err_t err = ra8p1_mipi_csi_switch_mipi_do();
    if (err == FSP_SUCCESS) {
        err = ra8p1_mipi_csi_camera_open_do();
    }
    if (err == FSP_SUCCESS) {
        err = ra8p1_mipi_csi_vin_camera_start(&g_vin0_cfg);
    }
    if (err == FSP_SUCCESS) {
        ra8p1_mipi_csi_clear_frame_buffers();
        err = ra8p1_mipi_csi_vin_scale_image(RA8P1_CSI_OUTPUT_IMAGE_WIDTH, RA8P1_CSI_OUTPUT_IMAGE_HEIGHT);
    }
    if (err == FSP_SUCCESS) {
        err = ra8p1_mipi_csi_vin_camera_start(&ra8p1_mipi_csi_vin_runtime_cfg);
    }
    return mp_obj_new_int(err);
}
static MP_DEFINE_CONST_FUN_OBJ_0(ra8p1_mipi_csi_start_obj, ra8p1_mipi_csi_start);

static mp_obj_t ra8p1_mipi_csi_start_default(void) {
    return mp_obj_new_int(ra8p1_mipi_csi_start_default_do());
}
static MP_DEFINE_CONST_FUN_OBJ_0(ra8p1_mipi_csi_start_default_obj, ra8p1_mipi_csi_start_default);

static mp_obj_t ra8p1_mipi_csi_start_select(mp_obj_t state_in) {
    return mp_obj_new_int(ra8p1_mipi_csi_start_select_do(mp_obj_is_true(state_in)));
}
static MP_DEFINE_CONST_FUN_OBJ_1(ra8p1_mipi_csi_start_select_obj, ra8p1_mipi_csi_start_select);

static mp_obj_t ra8p1_mipi_csi_start_probe(void) {
    fsp_err_t err = ra8p1_mipi_csi_start_default_do();
    ra8p1_mipi_csi_probe_sample(err, RA8P1_CSI_PROBE_SOURCE_REPL);
    return ra8p1_mipi_csi_probe_result_to_dict(&ra8p1_mipi_csi_last_probe);
}
static MP_DEFINE_CONST_FUN_OBJ_0(ra8p1_mipi_csi_start_probe_obj, ra8p1_mipi_csi_start_probe);

static mp_obj_t ra8p1_mipi_csi_start_probe_select(mp_obj_t state_in) {
    fsp_err_t err = ra8p1_mipi_csi_start_select_do(mp_obj_is_true(state_in));
    ra8p1_mipi_csi_probe_sample(err, RA8P1_CSI_PROBE_SOURCE_REPL);
    return ra8p1_mipi_csi_probe_result_to_dict(&ra8p1_mipi_csi_last_probe);
}
static MP_DEFINE_CONST_FUN_OBJ_1(ra8p1_mipi_csi_start_probe_select_obj, ra8p1_mipi_csi_start_probe_select);

static mp_obj_t ra8p1_mipi_csi_last_probe_get(void) {
    return ra8p1_mipi_csi_probe_result_to_dict(&ra8p1_mipi_csi_last_probe);
}
static MP_DEFINE_CONST_FUN_OBJ_0(ra8p1_mipi_csi_last_probe_obj, ra8p1_mipi_csi_last_probe_get);

static mp_obj_t ra8p1_mipi_csi_start_slow(void) {
    ra8p1_mipi_csi_reset_runtime_state();

    fsp_err_t err = ra8p1_mipi_csi_switch_mipi_do();
    if (err == FSP_SUCCESS) {
        err = ra8p1_mipi_csi_camera_open_do();
    }
    if (err == FSP_SUCCESS) {
        R_BSP_SoftwareDelay(1000, BSP_DELAY_UNITS_MILLISECONDS);
        err = ra8p1_mipi_csi_vin_camera_start(&g_vin0_cfg);
    }
    if (err == FSP_SUCCESS) {
        R_BSP_SoftwareDelay(1000, BSP_DELAY_UNITS_MILLISECONDS);
        ra8p1_mipi_csi_clear_frame_buffers();
        err = ra8p1_mipi_csi_vin_scale_image(RA8P1_CSI_OUTPUT_IMAGE_WIDTH, RA8P1_CSI_OUTPUT_IMAGE_HEIGHT);
    }
    if (err == FSP_SUCCESS) {
        err = ra8p1_mipi_csi_vin_camera_start(&ra8p1_mipi_csi_vin_runtime_cfg);
    }
    if (err == FSP_SUCCESS) {
        R_BSP_SoftwareDelay(500, BSP_DELAY_UNITS_MILLISECONDS);
    }
    return mp_obj_new_int(err);
}
static MP_DEFINE_CONST_FUN_OBJ_0(ra8p1_mipi_csi_start_slow_obj, ra8p1_mipi_csi_start_slow);

static mp_obj_t ra8p1_mipi_csi_start_once(void) {
    ra8p1_mipi_csi_reset_runtime_state();

    fsp_err_t err = ra8p1_mipi_csi_switch_mipi_do();
    if (err == FSP_SUCCESS) {
        err = ra8p1_mipi_csi_camera_open_do();
    }
    if (err == FSP_SUCCESS) {
        R_BSP_SoftwareDelay(1000, BSP_DELAY_UNITS_MILLISECONDS);
        ra8p1_mipi_csi_clear_frame_buffers();
        err = ra8p1_mipi_csi_vin_scale_image(RA8P1_CSI_OUTPUT_IMAGE_WIDTH, RA8P1_CSI_OUTPUT_IMAGE_HEIGHT);
    }
    if (err == FSP_SUCCESS) {
        err = ra8p1_mipi_csi_vin_camera_start(&ra8p1_mipi_csi_vin_runtime_cfg);
    }
    if (err == FSP_SUCCESS) {
        R_BSP_SoftwareDelay(500, BSP_DELAY_UNITS_MILLISECONDS);
    }
    return mp_obj_new_int(err);
}
static MP_DEFINE_CONST_FUN_OBJ_0(ra8p1_mipi_csi_start_once_obj, ra8p1_mipi_csi_start_once);

static mp_obj_t ra8p1_mipi_csi_init(void) {
    ra8p1_mipi_csi_phase = 1;
    ra8p1_mipi_csi_pins_configure();
    fsp_err_t err = g_timer_periodic.p_api->open(g_timer_periodic.p_ctrl, g_timer_periodic.p_cfg);
    ra8p1_mipi_csi_last_gpt_open_err = err;
    if (!ra8p1_mipi_csi_ok_or_open(err)) {
        mp_raise_OSError(err);
    }

    ra8p1_mipi_csi_phase = 2;
    err = g_timer_periodic.p_api->start(g_timer_periodic.p_ctrl);
    ra8p1_mipi_csi_last_gpt_start_err = err;
    if ((err != FSP_SUCCESS) && (err != FSP_ERR_ALREADY_OPEN)) {
        mp_raise_OSError(err);
    }

    ra8p1_mipi_csi_phase = 29;
    err = ra8p1_mipi_csi_power_domain_up();
    if (err != FSP_SUCCESS) {
        mp_raise_OSError(err);
    }

    err = ra8p1_mipi_csi_vin_scale_image(RA8P1_CSI_OUTPUT_IMAGE_WIDTH, RA8P1_CSI_OUTPUT_IMAGE_HEIGHT);
    if (err != FSP_SUCCESS) {
        mp_raise_OSError(err);
    }

    ra8p1_mipi_csi_phase = 3;
    err = g_vin0.p_api->open(g_vin0.p_ctrl, ra8p1_mipi_csi_vin_active_cfg());
    ra8p1_mipi_csi_last_vin_open_err = err;
    if (!ra8p1_mipi_csi_ok_or_open(err)) {
        mp_raise_OSError(err);
    }

    ra8p1_mipi_csi_phase = 4;
    err = g_vin0.p_api->captureStart(g_vin0.p_ctrl, NULL);
    ra8p1_mipi_csi_last_capture_start_err = err;
    if (err != FSP_SUCCESS && err != FSP_ERR_INVALID_STATE) {
        mp_raise_OSError(err);
    }

    ra8p1_mipi_csi_phase = 5;
    ra8p1_mipi_csi_open = true;
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_0(ra8p1_mipi_csi_init_obj, ra8p1_mipi_csi_init);

static mp_obj_t ra8p1_mipi_csi_deinit(void) {
    if (g_vin0_ctrl.open != 0U) {
        (void)g_vin0.p_api->close(g_vin0.p_ctrl);
    }
    if (g_timer_periodic_ctrl.open != 0U) {
        (void)g_timer_periodic.p_api->stop(g_timer_periodic.p_ctrl);
        (void)g_timer_periodic.p_api->close(g_timer_periodic.p_ctrl);
    }
    if (ra8p1_mipi_csi_ra_i2c_ready) {
        ra_i2c_deinit(R_IIC1);
        ra8p1_mipi_csi_ra_i2c_ready = false;
    }
#if RA8P1_CSI_USE_FSP_IIC
    if (ra8p1_mipi_csi_fsp_i2c_ready) {
        (void)R_IIC_MASTER_Close((i2c_master_ctrl_t *)&ra8p1_mipi_csi_fsp_i2c_ctrl);
        ra8p1_mipi_csi_fsp_i2c_ready = false;
    }
#endif
    ra8p1_mipi_csi_open = false;
    ra8p1_mipi_csi_phase = 0;
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_0(ra8p1_mipi_csi_deinit_obj, ra8p1_mipi_csi_deinit);

static mp_obj_t ra8p1_mipi_csi_status(void) {
    mp_obj_t status = mp_obj_new_dict(86);
    capture_status_t capture_status = { 0 };
    fsp_err_t capture_status_err = FSP_ERR_NOT_OPEN;
    bool mipi_powered = ((R_SYSTEM->PDCTRGD & (R_SYSTEM_PDCTRGD_PDDE_Msk | R_SYSTEM_PDCTRGD_PDCSF_Msk | R_SYSTEM_PDCTRGD_PDPGSF_Msk)) == 0);
    R_GPT0_Type *p_gpt = g_timer_periodic_ctrl.p_reg;
    uint32_t gpt_gtcnt0 = (p_gpt != NULL) ? p_gpt->GTCNT : 0xffffffffU;
    R_BSP_SoftwareDelay(1, BSP_DELAY_UNITS_MICROSECONDS);
    uint32_t gpt_gtcnt1 = (p_gpt != NULL) ? p_gpt->GTCNT : 0xffffffffU;

    if (g_vin0_ctrl.open != 0U) {
        capture_status_err = g_vin0.p_api->statusGet(g_vin0.p_ctrl, &capture_status);
    }

    RA8P1_CSI_DICT_STORE_INT(status, open, ra8p1_mipi_csi_open);
    RA8P1_CSI_DICT_STORE_INT(status, last_gpt_open_err, ra8p1_mipi_csi_last_gpt_open_err);
    RA8P1_CSI_DICT_STORE_INT(status, last_gpt_start_err, ra8p1_mipi_csi_last_gpt_start_err);
    RA8P1_CSI_DICT_STORE_INT(status, last_power_err, ra8p1_mipi_csi_last_power_err);
    RA8P1_CSI_DICT_STORE_INT(status, last_vin_open_err, ra8p1_mipi_csi_last_vin_open_err);
    RA8P1_CSI_DICT_STORE_INT(status, last_capture_start_err, ra8p1_mipi_csi_last_capture_start_err);
    RA8P1_CSI_DICT_STORE_INT(status, last_i2c_err, ra8p1_mipi_csi_last_i2c_err);
    RA8P1_CSI_DICT_STORE_INT(status, last_switch_err, ra8p1_mipi_csi_last_switch_err);
    RA8P1_CSI_DICT_STORE_INT(status, last_camera_err, ra8p1_mipi_csi_last_camera_err);
    RA8P1_CSI_DICT_STORE_INT(status, last_stream_err, ra8p1_mipi_csi_last_stream_err);
    RA8P1_CSI_DICT_STORE_INT(status, last_test_pattern_err, ra8p1_mipi_csi_last_test_pattern_err);
    RA8P1_CSI_DICT_STORE_UINT(status, last_i2c_backend, ra8p1_mipi_csi_last_i2c_backend);
    RA8P1_CSI_DICT_STORE_UINT(status, last_i2c_addr, ra8p1_mipi_csi_last_i2c_addr);
    RA8P1_CSI_DICT_STORE_UINT(status, last_i2c_reg, ra8p1_mipi_csi_last_i2c_reg);
    RA8P1_CSI_DICT_STORE_UINT(status, ra_i2c_ready, ra8p1_mipi_csi_ra_i2c_ready);
    RA8P1_CSI_DICT_STORE_UINT(status, ra_i2c_status, ra8p1_mipi_csi_last_ra_i2c_status);
    RA8P1_CSI_DICT_STORE_UINT(status, ra_i2c_error, ra8p1_mipi_csi_last_ra_i2c_error);
    RA8P1_CSI_DICT_STORE_UINT(status, ra_i2c_bytes_transferred, ra8p1_mipi_csi_last_ra_i2c_bytes_transferred);
    RA8P1_CSI_DICT_STORE_UINT(status, ra_i2c_bytes_remaining, ra8p1_mipi_csi_last_ra_i2c_bytes_remaining);
    RA8P1_CSI_DICT_STORE_UINT(status, ra_i2c_iccr1, ra8p1_mipi_csi_last_ra_i2c_iccr1);
    RA8P1_CSI_DICT_STORE_UINT(status, ra_i2c_iccr2, ra8p1_mipi_csi_last_ra_i2c_iccr2);
    RA8P1_CSI_DICT_STORE_UINT(status, ra_i2c_icsr2, ra8p1_mipi_csi_last_ra_i2c_icsr2);
    RA8P1_CSI_DICT_STORE_UINT(status, ra_i2c_icier, ra8p1_mipi_csi_last_ra_i2c_icier);
    RA8P1_CSI_DICT_STORE_UINT(status, ra_i2c_icser, ra8p1_mipi_csi_last_ra_i2c_icser);
#if RA8P1_CSI_USE_FSP_IIC
    RA8P1_CSI_DICT_STORE_UINT(status, fsp_i2c_ready, ra8p1_mipi_csi_fsp_i2c_ready);
    RA8P1_CSI_DICT_STORE_UINT(status, fsp_i2c_event, ra8p1_mipi_csi_fsp_i2c_event);
    RA8P1_CSI_DICT_STORE_UINT(status, fsp_i2c_open, ra8p1_mipi_csi_fsp_i2c_ctrl.open);
#endif
    RA8P1_CSI_DICT_STORE_UINT(status, camera_writes, ra8p1_mipi_csi_camera_writes);
    RA8P1_CSI_DICT_STORE_UINT(status, camera_readbacks, ra8p1_mipi_csi_camera_readbacks);
    RA8P1_CSI_DICT_STORE_UINT(status, camera_mismatch_reg, ra8p1_mipi_csi_camera_mismatch_reg);
    RA8P1_CSI_DICT_STORE_UINT(status, camera_mismatch_expected, ra8p1_mipi_csi_camera_mismatch_expected);
    RA8P1_CSI_DICT_STORE_UINT(status, camera_mismatch_actual, ra8p1_mipi_csi_camera_mismatch_actual);
    RA8P1_CSI_DICT_STORE_UINT(status, phase, ra8p1_mipi_csi_phase);
    RA8P1_CSI_DICT_STORE_INT(status, capture_status_err, capture_status_err);
    RA8P1_CSI_DICT_STORE_UINT(status, capture_state, capture_status.state);
    RA8P1_CSI_DICT_STORE_UINT(status, capture_data_size, capture_status.data_size);
    RA8P1_CSI_DICT_STORE_UINT(status, capture_buffer, (uintptr_t)capture_status.p_buffer);
    RA8P1_CSI_DICT_STORE_UINT(status, vin_runtime_ready, ra8p1_mipi_csi_vin_runtime_ready);
    RA8P1_CSI_DICT_STORE_UINT(status, output_width, ra8p1_mipi_csi_output_width);
    RA8P1_CSI_DICT_STORE_UINT(status, output_height, ra8p1_mipi_csi_output_height);
    RA8P1_CSI_DICT_STORE_UINT(status, cfg_mipi_sel_state, RA8P1_CSI_MIPI_SEL_STATE != 0);
    RA8P1_CSI_DICT_STORE_UINT(status, vin_open, g_vin0_ctrl.open);
    RA8P1_CSI_DICT_STORE_UINT(status, csi_open, g_mipi_csi0_ctrl.open);
    RA8P1_CSI_DICT_STORE_UINT(status, gpt_open, g_timer_periodic_ctrl.open);
    RA8P1_CSI_DICT_STORE_UINT(status, gpt_cfg_channel, g_timer_periodic_cfg.channel);
    RA8P1_CSI_DICT_STORE_UINT(status, gpt_cfg_period_counts, g_timer_periodic_cfg.period_counts);
    RA8P1_CSI_DICT_STORE_UINT(status, gpt_cfg_duty_counts, g_timer_periodic_cfg.duty_cycle_counts);
    RA8P1_CSI_DICT_STORE_UINT(status, gpt_cfg_source_div, g_timer_periodic_cfg.source_div);
    RA8P1_CSI_DICT_STORE_UINT(status, system_core_clock, SystemCoreClock);
    RA8P1_CSI_DICT_STORE_UINT(status, clock_iclk_hz, R_FSP_SystemClockHzGet(FSP_PRIV_CLOCK_ICLK));
    RA8P1_CSI_DICT_STORE_UINT(status, clock_pclka_hz, R_FSP_SystemClockHzGet(FSP_PRIV_CLOCK_PCLKA));
    RA8P1_CSI_DICT_STORE_UINT(status, clock_pclkb_hz, R_FSP_SystemClockHzGet(FSP_PRIV_CLOCK_PCLKB));
    RA8P1_CSI_DICT_STORE_UINT(status, clock_pclkd_hz, R_FSP_SystemClockHzGet(FSP_PRIV_CLOCK_PCLKD));
    RA8P1_CSI_DICT_STORE_UINT(status, clock_gptclk_hz, ra8p1_mipi_csi_gptclk_hz());
    RA8P1_CSI_DICT_STORE_UINT(status, cfg_gptclk_source, BSP_CFG_GPTCLK_SOURCE);
    RA8P1_CSI_DICT_STORE_UINT(status, cfg_gptclk_div, BSP_CFG_GPTCLK_DIV);
    RA8P1_CSI_DICT_STORE_UINT(status, cfg_pll2r_hz, BSP_CFG_PLL2R_FREQUENCY_HZ);
    RA8P1_CSI_DICT_STORE_UINT(status, system_sckdivcr, R_SYSTEM->SCKDIVCR);
    RA8P1_CSI_DICT_STORE_UINT(status, system_sckdivcr2, R_SYSTEM->SCKDIVCR2);
    RA8P1_CSI_DICT_STORE_UINT(status, system_sckscr, R_SYSTEM->SCKSCR);
    RA8P1_CSI_DICT_STORE_UINT(status, system_gptckcr, R_SYSTEM->GPTCKCR);
    RA8P1_CSI_DICT_STORE_UINT(status, system_gptckdivcr, R_SYSTEM->GPTCKDIVCR);
    RA8P1_CSI_DICT_STORE_UINT(status, system_oscsf, R_SYSTEM->OSCSF);
    RA8P1_CSI_DICT_STORE_UINT(status, system_pll2cr, R_SYSTEM->PLL2CR);
    RA8P1_CSI_DICT_STORE_UINT(status, system_pll2ccr, R_SYSTEM->PLL2CCR);
    RA8P1_CSI_DICT_STORE_UINT(status, system_pll2ccr2, R_SYSTEM->PLL2CCR2);
    RA8P1_CSI_DICT_STORE_UINT(status, mstpcrb, R_MSTP->MSTPCRB);
    RA8P1_CSI_DICT_STORE_UINT(status, mstpcrc, R_MSTP->MSTPCRC);
    RA8P1_CSI_DICT_STORE_UINT(status, mstpcrd, R_MSTP->MSTPCRD);
    RA8P1_CSI_DICT_STORE_UINT(status, gpt_gtcr, (p_gpt != NULL) ? p_gpt->GTCR : 0xffffffffU);
    RA8P1_CSI_DICT_STORE_UINT(status, gpt_gtior, (p_gpt != NULL) ? p_gpt->GTIOR : 0xffffffffU);
    RA8P1_CSI_DICT_STORE_UINT(status, gpt_gtcnt0, gpt_gtcnt0);
    RA8P1_CSI_DICT_STORE_UINT(status, gpt_gtcnt1, gpt_gtcnt1);
    RA8P1_CSI_DICT_STORE_UINT(status, gpt_gtccra, (p_gpt != NULL) ? p_gpt->GTCCR[0] : 0xffffffffU);
    RA8P1_CSI_DICT_STORE_UINT(status, gpt_gtccrc, (p_gpt != NULL) ? p_gpt->GTCCR[2] : 0xffffffffU);
    RA8P1_CSI_DICT_STORE_UINT(status, gpt_gtpr, (p_gpt != NULL) ? p_gpt->GTPR : 0xffffffffU);
    RA8P1_CSI_DICT_STORE_UINT(status, vin_callbacks, ra8p1_mipi_csi_vin_callbacks);
    RA8P1_CSI_DICT_STORE_UINT(status, csi_callbacks, ra8p1_mipi_csi_csi_callbacks);
    RA8P1_CSI_DICT_STORE_UINT(status, last_vin_event, ra8p1_mipi_csi_last_vin_event);
    RA8P1_CSI_DICT_STORE_UINT(status, last_vin_event_status, ra8p1_mipi_csi_last_vin_event_status);
    RA8P1_CSI_DICT_STORE_UINT(status, last_vin_interrupt_status, ra8p1_mipi_csi_last_vin_interrupt_status);
    RA8P1_CSI_DICT_STORE_UINT(status, last_csi_event, ra8p1_mipi_csi_last_csi_event);
    RA8P1_CSI_DICT_STORE_UINT(status, last_csi_event_idx, ra8p1_mipi_csi_last_csi_event_idx);
    RA8P1_CSI_DICT_STORE_UINT(status, last_csi_event_data, ra8p1_mipi_csi_last_csi_event_data);
    RA8P1_CSI_DICT_STORE_UINT(status, vin_ms, mipi_powered ? R_VIN->MS : 0xffffffffU);
    RA8P1_CSI_DICT_STORE_UINT(status, vin_ints, mipi_powered ? R_VIN->INTS : 0xffffffffU);
    RA8P1_CSI_DICT_STORE_UINT(status, vin_ie, mipi_powered ? R_VIN->IE : 0xffffffffU);
    RA8P1_CSI_DICT_STORE_UINT(status, vin_lc, mipi_powered ? R_VIN->LC : 0xffffffffU);
    RA8P1_CSI_DICT_STORE_UINT(status, csi_mcg, mipi_powered ? R_MIPI_CSI->MCG : 0xffffffffU);
    RA8P1_CSI_DICT_STORE_UINT(status, csi_mct0, mipi_powered ? R_MIPI_CSI->MCT0 : 0xffffffffU);
    RA8P1_CSI_DICT_STORE_UINT(status, csi_mct2, mipi_powered ? R_MIPI_CSI->MCT2 : 0xffffffffU);
    RA8P1_CSI_DICT_STORE_UINT(status, csi_mct3, mipi_powered ? R_MIPI_CSI->MCT3 : 0xffffffffU);
    RA8P1_CSI_DICT_STORE_UINT(status, csi_epct, mipi_powered ? R_MIPI_CSI->EPCT : 0xffffffffU);
    RA8P1_CSI_DICT_STORE_UINT(status, csi_emct, mipi_powered ? R_MIPI_CSI->EMCT : 0xffffffffU);
    RA8P1_CSI_DICT_STORE_UINT(status, csi_mist, mipi_powered ? R_MIPI_CSI->MIST : 0xffffffffU);
    RA8P1_CSI_DICT_STORE_UINT(status, csi_dtel, mipi_powered ? R_MIPI_CSI->DTEL : 0xffffffffU);
    RA8P1_CSI_DICT_STORE_UINT(status, csi_dteh, mipi_powered ? R_MIPI_CSI->DTEH : 0xffffffffU);
    RA8P1_CSI_DICT_STORE_UINT(status, csi_rxst, mipi_powered ? R_MIPI_CSI->RXST : 0xffffffffU);
    RA8P1_CSI_DICT_STORE_UINT(status, csi_rxsc, mipi_powered ? R_MIPI_CSI->RXSC : 0xffffffffU);
    RA8P1_CSI_DICT_STORE_UINT(status, csi_rxie, mipi_powered ? R_MIPI_CSI->RXIE : 0xffffffffU);
    RA8P1_CSI_DICT_STORE_UINT(status, csi_dlst0, mipi_powered ? R_MIPI_CSI->DLST0 : 0xffffffffU);
    RA8P1_CSI_DICT_STORE_UINT(status, csi_dlst1, mipi_powered ? R_MIPI_CSI->DLST1 : 0xffffffffU);
    RA8P1_CSI_DICT_STORE_UINT(status, csi_dlie0, mipi_powered ? R_MIPI_CSI->DLIE0 : 0xffffffffU);
    RA8P1_CSI_DICT_STORE_UINT(status, csi_dlie1, mipi_powered ? R_MIPI_CSI->DLIE1 : 0xffffffffU);
    RA8P1_CSI_DICT_STORE_UINT(status, csi_vcst0, mipi_powered ? R_MIPI_CSI->VCST0 : 0xffffffffU);
    RA8P1_CSI_DICT_STORE_UINT(status, csi_vcie0, mipi_powered ? R_MIPI_CSI->VCIE0 : 0xffffffffU);
    RA8P1_CSI_DICT_STORE_UINT(status, csi_pmst, mipi_powered ? R_MIPI_CSI->PMST : 0xffffffffU);
    RA8P1_CSI_DICT_STORE_UINT(status, csi_pmie, mipi_powered ? R_MIPI_CSI->PMIE : 0xffffffffU);
    RA8P1_CSI_DICT_STORE_UINT(status, csi_gsst, mipi_powered ? R_MIPI_CSI->GSST : 0xffffffffU);
    RA8P1_CSI_DICT_STORE_UINT(status, csi_gsie, mipi_powered ? R_MIPI_CSI->GSIE : 0xffffffffU);
    RA8P1_CSI_DICT_STORE_UINT(status, csi_gsct, mipi_powered ? R_MIPI_CSI->GSCT : 0xffffffffU);
    RA8P1_CSI_DICT_STORE_UINT(status, phy_dphyrefcr, mipi_powered ? R_MIPI_PHY->DPHYREFCR : 0xffffffffU);
    RA8P1_CSI_DICT_STORE_UINT(status, phy_dphyplfcr, mipi_powered ? R_MIPI_PHY->DPHYPLFCR : 0xffffffffU);
    RA8P1_CSI_DICT_STORE_UINT(status, phy_dphyplocr, mipi_powered ? R_MIPI_PHY->DPHYPLOCR : 0xffffffffU);
    RA8P1_CSI_DICT_STORE_UINT(status, phy_dphyesccr, mipi_powered ? R_MIPI_PHY->DPHYESCCR : 0xffffffffU);
    RA8P1_CSI_DICT_STORE_UINT(status, phy_dphysfr, mipi_powered ? R_MIPI_PHY->DPHYSFR : 0xffffffffU);
    RA8P1_CSI_DICT_STORE_UINT(status, phy_dphypwrcr, mipi_powered ? R_MIPI_PHY->DPHYPWRCR : 0xffffffffU);
    RA8P1_CSI_DICT_STORE_UINT(status, phy_dphyocr, mipi_powered ? R_MIPI_PHY->DPHYOCR : 0xffffffffU);
    RA8P1_CSI_DICT_STORE_UINT(status, phy_dphytim1, mipi_powered ? R_MIPI_PHY->DPHYTIM1 : 0xffffffffU);
    RA8P1_CSI_DICT_STORE_UINT(status, phy_dphytim2, mipi_powered ? R_MIPI_PHY->DPHYTIM2 : 0xffffffffU);
    RA8P1_CSI_DICT_STORE_UINT(status, phy_dphytim3, mipi_powered ? R_MIPI_PHY->DPHYTIM3 : 0xffffffffU);
    RA8P1_CSI_DICT_STORE_UINT(status, phy_dphytim4, mipi_powered ? R_MIPI_PHY->DPHYTIM4 : 0xffffffffU);
    RA8P1_CSI_DICT_STORE_UINT(status, phy_dphytim5, mipi_powered ? R_MIPI_PHY->DPHYTIM5 : 0xffffffffU);
    RA8P1_CSI_DICT_STORE_UINT(status, phy_dphytim6, mipi_powered ? R_MIPI_PHY->DPHYTIM6 : 0xffffffffU);
    RA8P1_CSI_DICT_STORE_UINT(status, phy_dphymdc, mipi_powered ? R_MIPI_PHY->DPHYMDC : 0xffffffffU);
    RA8P1_CSI_DICT_STORE_UINT(status, system_pdctrgd, R_SYSTEM->PDCTRGD);
    RA8P1_CSI_DICT_STORE_UINT(status, pdctrgd_before, ra8p1_mipi_csi_pdctrgd_before);
    RA8P1_CSI_DICT_STORE_UINT(status, pdctrgd_after, ra8p1_mipi_csi_pdctrgd_after);
    RA8P1_CSI_DICT_STORE_UINT(status, p111_pfs, ra8p1_mipi_csi_pin_pfs(BSP_IO_PORT_01_PIN_11));
    RA8P1_CSI_DICT_STORE_UINT(status, p111_level, ra8p1_mipi_csi_mipi_int_pin_read());
    RA8P1_CSI_DICT_STORE_UINT(status, p501_pfs, ra8p1_mipi_csi_pin_pfs(BSP_IO_PORT_05_PIN_01));
    RA8P1_CSI_DICT_STORE_UINT(status, p501_level, ra8p1_mipi_csi_cam_xclk_pin_read());
    RA8P1_CSI_DICT_STORE_UINT(status, p511_pfs, ra8p1_mipi_csi_pin_pfs(BSP_IO_PORT_05_PIN_11));
    RA8P1_CSI_DICT_STORE_UINT(status, p512_pfs, ra8p1_mipi_csi_pin_pfs(BSP_IO_PORT_05_PIN_12));
    RA8P1_CSI_DICT_STORE_UINT(status, p709_pfs, ra8p1_mipi_csi_pin_pfs(BSP_IO_PORT_07_PIN_09));
    RA8P1_CSI_DICT_STORE_UINT(status, p709_level, ra8p1_mipi_csi_camera_reset_pin_read());
    RA8P1_CSI_DICT_STORE_UINT(status, i2c_scl, ra8p1_mipi_csi_i2c_scl_read());
    RA8P1_CSI_DICT_STORE_UINT(status, i2c_sda, ra8p1_mipi_csi_i2c_sda_read());
    return status;
}
static MP_DEFINE_CONST_FUN_OBJ_0(ra8p1_mipi_csi_status_obj, ra8p1_mipi_csi_status);

static mp_obj_t ra8p1_mipi_csi_buffer(mp_obj_t idx_in) {
    mp_int_t idx = mp_obj_get_int(idx_in);
    return mp_obj_new_memoryview(0x80 | 'B', VIN_BYTES_PER_FRAME, ra8p1_mipi_csi_buffer_ptr(idx));
}
static MP_DEFINE_CONST_FUN_OBJ_1(ra8p1_mipi_csi_buffer_obj, ra8p1_mipi_csi_buffer);

static const mp_rom_map_elem_t ra8p1_mipi_csi_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_ra8p1_mipi_csi) },
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&ra8p1_mipi_csi_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_step), MP_ROM_PTR(&ra8p1_mipi_csi_step_obj) },
    { MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&ra8p1_mipi_csi_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR_status), MP_ROM_PTR(&ra8p1_mipi_csi_status_obj) },
    { MP_ROM_QSTR(MP_QSTR_buffer), MP_ROM_PTR(&ra8p1_mipi_csi_buffer_obj) },
    { MP_ROM_QSTR(MP_QSTR_i2c_probe), MP_ROM_PTR(&ra8p1_mipi_csi_i2c_probe_obj) },
    { MP_ROM_QSTR(MP_QSTR_switch_mipi), MP_ROM_PTR(&ra8p1_mipi_csi_switch_mipi_obj) },
    { MP_ROM_QSTR(MP_QSTR_switch_select), MP_ROM_PTR(&ra8p1_mipi_csi_switch_select_obj) },
    { MP_ROM_QSTR(MP_QSTR_switch_read), MP_ROM_PTR(&ra8p1_mipi_csi_switch_read_obj) },
    { MP_ROM_QSTR(MP_QSTR_switch_write), MP_ROM_PTR(&ra8p1_mipi_csi_switch_write_obj) },
    { MP_ROM_QSTR(MP_QSTR_camera_read), MP_ROM_PTR(&ra8p1_mipi_csi_camera_read_obj) },
    { MP_ROM_QSTR(MP_QSTR_camera_write), MP_ROM_PTR(&ra8p1_mipi_csi_camera_write_obj) },
    { MP_ROM_QSTR(MP_QSTR_reg_read), MP_ROM_PTR(&ra8p1_mipi_csi_reg_read_obj) },
    { MP_ROM_QSTR(MP_QSTR_reg_write), MP_ROM_PTR(&ra8p1_mipi_csi_reg_write_obj) },
    { MP_ROM_QSTR(MP_QSTR_camera_open), MP_ROM_PTR(&ra8p1_mipi_csi_camera_open_obj) },
    { MP_ROM_QSTR(MP_QSTR_stream_on), MP_ROM_PTR(&ra8p1_mipi_csi_stream_on_obj) },
    { MP_ROM_QSTR(MP_QSTR_stream_off), MP_ROM_PTR(&ra8p1_mipi_csi_stream_off_obj) },
    { MP_ROM_QSTR(MP_QSTR_test_pattern), MP_ROM_PTR(&ra8p1_mipi_csi_test_pattern_obj) },
    { MP_ROM_QSTR(MP_QSTR_start), MP_ROM_PTR(&ra8p1_mipi_csi_start_obj) },
    { MP_ROM_QSTR(MP_QSTR_start_default), MP_ROM_PTR(&ra8p1_mipi_csi_start_default_obj) },
    { MP_ROM_QSTR(MP_QSTR_start_select), MP_ROM_PTR(&ra8p1_mipi_csi_start_select_obj) },
    { MP_ROM_QSTR(MP_QSTR_start_probe), MP_ROM_PTR(&ra8p1_mipi_csi_start_probe_obj) },
    { MP_ROM_QSTR(MP_QSTR_start_probe_select), MP_ROM_PTR(&ra8p1_mipi_csi_start_probe_select_obj) },
    { MP_ROM_QSTR(MP_QSTR_last_probe), MP_ROM_PTR(&ra8p1_mipi_csi_last_probe_obj) },
    { MP_ROM_QSTR(MP_QSTR_start_slow), MP_ROM_PTR(&ra8p1_mipi_csi_start_slow_obj) },
    { MP_ROM_QSTR(MP_QSTR_start_once), MP_ROM_PTR(&ra8p1_mipi_csi_start_once_obj) },
    { MP_ROM_QSTR(MP_QSTR_WIDTH), MP_ROM_INT(VIN_IMAGE_WIDTH) },
    { MP_ROM_QSTR(MP_QSTR_HEIGHT), MP_ROM_INT(VIN_IMAGE_HEIGHT) },
    { MP_ROM_QSTR(MP_QSTR_OUTPUT_WIDTH), MP_ROM_INT(RA8P1_CSI_OUTPUT_IMAGE_WIDTH) },
    { MP_ROM_QSTR(MP_QSTR_OUTPUT_HEIGHT), MP_ROM_INT(RA8P1_CSI_OUTPUT_IMAGE_HEIGHT) },
    { MP_ROM_QSTR(MP_QSTR_BPP), MP_ROM_INT(16) },
    { MP_ROM_QSTR(MP_QSTR_FORMAT), MP_ROM_QSTR(MP_QSTR_YUV422) },
    { MP_ROM_QSTR(MP_QSTR_BUFFER_SIZE), MP_ROM_INT(VIN_BYTES_PER_FRAME) },
    { MP_ROM_QSTR(MP_QSTR_BUFFER_COUNT), MP_ROM_INT(2) },
};
static MP_DEFINE_CONST_DICT(ra8p1_mipi_csi_module_globals, ra8p1_mipi_csi_module_globals_table);

const mp_obj_module_t mp_module_ra8p1_mipi_csi = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&ra8p1_mipi_csi_module_globals,
};

MP_REGISTER_MODULE(MP_QSTR_ra8p1_mipi_csi, mp_module_ra8p1_mipi_csi);

#endif
