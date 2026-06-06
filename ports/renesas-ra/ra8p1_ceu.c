/*
 * Minimal EK-RA8P1 CEU/DVP camera bring-up module.
 *
 * This is intentionally gated behind RA8P1_BRINGUP_CEU_TEST because CEU pins
 * share board routes with display/audio/ethernet signals on EK-RA8P1.
 */

#include "py/runtime.h"
#include "py/objarray.h"
#include "py/mphal.h"

#if defined(BSP_MCU_R7KA8P1KFLCAC) && defined(MICROPY_RA8P1_BRINGUP_CEU_TEST) && MICROPY_RA8P1_BRINGUP_CEU_TEST

#include <string.h>

#include "common_data.h"
#include "r_ceu.h"
#include "r_gpt.h"
#include "r_ioport.h"

#define RA8P1_CEU_WIDTH (640U)
#define RA8P1_CEU_HEIGHT (480U)
#define RA8P1_CEU_ACTIVE_WIDTH (1280U)
#define RA8P1_CEU_ACTIVE_HEIGHT (960U)
#define RA8P1_CEU_BPP (2U)
#define RA8P1_CEU_BUFFER_SIZE (RA8P1_CEU_WIDTH * RA8P1_CEU_HEIGHT * RA8P1_CEU_BPP)
#define RA8P1_CEU_CAMERA_ADDR (0x3CU)
#define RA8P1_CEU_SWITCH_ADDR (0x43U)
#define RA8P1_CEU_SWITCH_INPUT_REG (0x00U)
#define RA8P1_CEU_SWITCH_PIN_DIR_REG (0x03U)
#define RA8P1_CEU_SWITCH_OUTPUT_STATE_REG (0x05U)
#define RA8P1_CEU_SWITCH_OUTPUT_ENABLE_REG (0x07U)
#define RA8P1_CEU_SWITCH_SW4_6_BIT (5U)
#define RA8P1_CEU_SWITCH_ROUTE_MASK (1U << RA8P1_CEU_SWITCH_SW4_6_BIT)
#define RA8P1_CEU_I2C_DELAY_US (5U)
#define RA8P1_CEU_PIN_INDEX_MASK (0xffU)
#define RA8P1_CEU_OV5640_END (0xffffU)
#define RA8P1_CEU_OV5640_WAIT (0xaaaaU)
#define RA8P1_CEU_CAPTURE_TIMEOUT_MS (3000U)
#define RA8P1_CEU_STATUS_SAMPLE_BYTES (4096U)
#define RA8P1_CEU_REG_PIDH (0x300aU)
#define RA8P1_CEU_REG_PIDL (0x300bU)
#define RA8P1_CEU_REG_RESET (0x3008U)
#define RA8P1_CEU_XCLK_TARGET_HZ (24000000U)
#define RA8P1_CEU_XCLK_FALLBACK_PERIOD_COUNTS (13U)
#define RA8P1_CEU_OV5640_DVP_DATA_ORDER (0x06U)
#define RA8P1_CEU_OV5640_PCLK_MANUAL_DIVIDER (0x02U)
#define RA8P1_CEU_OV5640_AEC_MANUAL_ENABLE (0x07U)
#define RA8P1_CEU_OV5640_AEC_AUTO_ENABLE (0x00U)
#define RA8P1_CEU_OV5640_CIP_CTRL_AUTO (0x25U)
#define RA8P1_CEU_TUNING_DEFAULT_EXPOSURE (0x500U)
#define RA8P1_CEU_TUNING_DEFAULT_GAIN (0x30U)
#define RA8P1_CEU_TUNING_DEFAULT_EDGE_CTRL (0x65U)
#define RA8P1_CEU_MAX_PIN_SAMPLES (1000000U)
#define RA8P1_CEU_MAX_CAPTURE_TIMEOUT_MS (60000U)
#define RA8P1_CEU_MAX_LIVE_FRAMES (10000U)
#define RA8P1_CEU_MAX_OV5640_EXPOSURE (0xfffffU)
#define RA8P1_CEU_MAX_OV5640_GAIN (0x3ffU)
#define RA8P1_CEU_MAX_OV5640_REG8 (0xffU)
#define RA8P1_CEU_DISPLAY_PIXEL_BYTES (4U)
#define RA8P1_CEU_DISPLAY_STARTED_STATE (2U)
#define RA8P1_CEU_CACHE_LINE_BYTES (32U)
#define RA8P1_CEU_YUV_ORDER_CBYCRY (0U)
#define RA8P1_CEU_YUV_ORDER_CRYCBY (1U)
#define RA8P1_CEU_YUV_ORDER_YCBYCR (2U)
#define RA8P1_CEU_YUV_ORDER_YCRYCB (3U)
#define RA8P1_CEU_YUV_ORDER_MAX (3U)
#define RA8P1_CEU_SRAM_BUFFER_ADDR (0x22030000UL)
#define RA8P1_CEU_LCD_SHARED_PIN_CFG \
    ((uint32_t)IOPORT_CFG_DRIVE_MID | \
     (uint32_t)IOPORT_CFG_PERIPHERAL_PIN | \
     (uint32_t)IOPORT_PERIPHERAL_LCD_GRAPHICS)

#define RA8P1_CEU_DICT_STORE_INT(dict, key, value) \
    mp_obj_dict_store((dict), MP_OBJ_NEW_QSTR(MP_QSTR_##key), mp_obj_new_int(value))

#define RA8P1_CEU_DICT_STORE_UINT(dict, key, value) \
    mp_obj_dict_store((dict), MP_OBJ_NEW_QSTR(MP_QSTR_##key), mp_obj_new_int_from_uint(value))

typedef struct {
    uint16_t reg;
    uint8_t val;
} ra8p1_ceu_sensor_reg_t;

#ifndef MICROPY_RA8P1_CEU_CAPTURE_BUFFER_SRAM
#define MICROPY_RA8P1_CEU_CAPTURE_BUFFER_SRAM (1)
#endif

#if MICROPY_RA8P1_CEU_CAPTURE_BUFFER_SRAM
static uint8_t * const ra8p1_ceu_image_vga = (uint8_t *)RA8P1_CEU_SRAM_BUFFER_ADDR;
#else
static uint8_t ra8p1_ceu_image_vga[RA8P1_CEU_BUFFER_SIZE] BSP_ALIGN_VARIABLE(128)
    BSP_PLACE_IN_SECTION(BSP_UNINIT_SECTION_PREFIX ".sdram_noinit");
#endif

static ceu_instance_ctrl_t ra8p1_ceu_ctrl;
static gpt_instance_ctrl_t ra8p1_ceu_xclk_ctrl;

static bool ra8p1_ceu_capture_open;
static bool ra8p1_ceu_xclk_open;
static volatile bool ra8p1_ceu_capture_ready;
static volatile uint32_t ra8p1_ceu_callbacks;
static volatile uint32_t ra8p1_ceu_last_event;
static volatile uint32_t ra8p1_ceu_last_event_status;
static volatile uint32_t ra8p1_ceu_last_interrupt_status;
static int ra8p1_ceu_last_pin_err = FSP_SUCCESS;
static int ra8p1_ceu_last_gpt_open_err = FSP_SUCCESS;
static int ra8p1_ceu_last_gpt_period_err = FSP_SUCCESS;
static int ra8p1_ceu_last_gpt_start_err = FSP_SUCCESS;
static int ra8p1_ceu_last_i2c_err = FSP_SUCCESS;
static int ra8p1_ceu_last_switch_err = FSP_SUCCESS;
static int ra8p1_ceu_last_camera_err = FSP_SUCCESS;
static int ra8p1_ceu_last_ceu_open_err = FSP_SUCCESS;
static int ra8p1_ceu_last_capture_start_err = FSP_SUCCESS;
static int ra8p1_ceu_last_capture_status_err = FSP_SUCCESS;
static int ra8p1_ceu_last_capture_frame_err = FSP_SUCCESS;
static int ra8p1_ceu_last_display_open_err = FSP_SUCCESS;
static int ra8p1_ceu_last_display_start_err = FSP_SUCCESS;
static int ra8p1_ceu_last_display_stop_err = FSP_SUCCESS;
static int ra8p1_ceu_last_display_flip_err = FSP_SUCCESS;
static int ra8p1_ceu_last_display_backlight_err = FSP_SUCCESS;
static int ra8p1_ceu_last_display_pin_restore_err = FSP_SUCCESS;
static int ra8p1_ceu_last_tuning_err = FSP_SUCCESS;
static uint32_t ra8p1_ceu_last_i2c_addr;
static uint32_t ra8p1_ceu_last_i2c_reg;
static uint32_t ra8p1_ceu_camera_writes;
static uint32_t ra8p1_ceu_display_frames;
static uint32_t ra8p1_ceu_last_live_requested;
static uint32_t ra8p1_ceu_last_live_completed;
static uint32_t ra8p1_ceu_yuv_order = RA8P1_CEU_YUV_ORDER_CBYCRY;
static bool ra8p1_ceu_display_open;
static bool ra8p1_ceu_test_pattern_enabled;
static bool ra8p1_ceu_display_pins_restored = true;
static bool ra8p1_ceu_display_backlight_on = true;
static bool ra8p1_ceu_tuning_enabled;
static uint32_t ra8p1_ceu_tuning_exposure = RA8P1_CEU_TUNING_DEFAULT_EXPOSURE;
static uint32_t ra8p1_ceu_tuning_gain = RA8P1_CEU_TUNING_DEFAULT_GAIN;
static uint32_t ra8p1_ceu_tuning_edge_ctrl = RA8P1_CEU_TUNING_DEFAULT_EDGE_CTRL;

static void ra8p1_ceu_callback(capture_callback_args_t *p_args);
static uint32_t ra8p1_ceu_gptclk_hz(void);
static uint32_t ra8p1_ceu_xclk_period_counts_for(uint32_t clock_hz);
static uint32_t ra8p1_ceu_xclk_period_counts(void);
static fsp_err_t ra8p1_ceu_display_backlight_set(bool on);
static fsp_err_t ra8p1_ceu_camera_apply_tuning(void);

static const gpt_extended_cfg_t ra8p1_ceu_xclk_extend = {
    .gtioca = { .output_enabled = true, .stop_level = GPT_PIN_LEVEL_LOW },
    .gtiocb = { .output_enabled = false, .stop_level = GPT_PIN_LEVEL_LOW },
    .start_source = (gpt_source_t)(GPT_SOURCE_NONE),
    .stop_source = (gpt_source_t)(GPT_SOURCE_NONE),
    .clear_source = (gpt_source_t)(GPT_SOURCE_NONE),
    .capture_a_source = (gpt_source_t)(GPT_SOURCE_NONE),
    .capture_b_source = (gpt_source_t)(GPT_SOURCE_NONE),
    .count_up_source = (gpt_source_t)(GPT_SOURCE_NONE),
    .count_down_source = (gpt_source_t)(GPT_SOURCE_NONE),
    .capture_filter_gtioca = GPT_CAPTURE_FILTER_NONE,
    .capture_filter_gtiocb = GPT_CAPTURE_FILTER_NONE,
    .capture_a_ipl = (BSP_IRQ_DISABLED),
    .capture_b_ipl = (BSP_IRQ_DISABLED),
    .compare_match_c_ipl = (BSP_IRQ_DISABLED),
    .compare_match_d_ipl = (BSP_IRQ_DISABLED),
    .compare_match_e_ipl = (BSP_IRQ_DISABLED),
    .compare_match_f_ipl = (BSP_IRQ_DISABLED),
    .capture_a_irq = FSP_INVALID_VECTOR,
    .capture_b_irq = FSP_INVALID_VECTOR,
    .compare_match_c_irq = FSP_INVALID_VECTOR,
    .compare_match_d_irq = FSP_INVALID_VECTOR,
    .compare_match_e_irq = FSP_INVALID_VECTOR,
    .compare_match_f_irq = FSP_INVALID_VECTOR,
    .compare_match_value = { 0, 0, 0x10000, 0x10000, 0x10000, 0x10000 },
    .compare_match_status = 0,
    .p_pwm_cfg = NULL,
    .gtior_setting.gtior = 0U,
    .gtioca_polarity = GPT_GTIOC_POLARITY_NORMAL,
    .gtiocb_polarity = GPT_GTIOC_POLARITY_NORMAL,
};

static const timer_cfg_t ra8p1_ceu_xclk_cfg = {
    .mode = TIMER_MODE_PERIODIC,
    .period_counts = RA8P1_CEU_XCLK_FALLBACK_PERIOD_COUNTS,
    .source_div = TIMER_SOURCE_DIV_1,
    .duty_cycle_counts = RA8P1_CEU_XCLK_FALLBACK_PERIOD_COUNTS / 2U,
    .channel = 12,
    .cycle_end_ipl = (BSP_IRQ_DISABLED),
    .cycle_end_irq = FSP_INVALID_VECTOR,
    .p_callback = NULL,
    .p_context = NULL,
    .p_extend = &ra8p1_ceu_xclk_extend,
};

static const timer_instance_t ra8p1_ceu_xclk = {
    .p_ctrl = &ra8p1_ceu_xclk_ctrl,
    .p_cfg = &ra8p1_ceu_xclk_cfg,
    .p_api = &g_timer_on_gpt,
};

static ceu_extended_cfg_t ra8p1_ceu_vga_extend = {
    .capture_format = CEU_CAPTURE_FORMAT_DATA_SYNCHRONOUS,
    .input_order = CEU_INPUT_ORDER_CB0Y0CR0Y1,
    .output_format = CEU_OUTPUT_FORMAT_YCBCR422,
    .data_bus_width = CEU_DATA_BUS_SIZE_8_BIT,
    .edge_info = { .dsel = 0, .hdsel = 0, .vdsel = 0 },
    .hsync_polarity = CEU_HSYNC_POLARITY_HIGH,
    .vsync_polarity = CEU_VSYNC_POLARITY_HIGH,
    .image_area_size = 0,
    .byte_swapping = { .swap_8bit_units = 0, .swap_16bit_units = 1, .swap_32bit_units = 1 },
    .burst_mode = CEU_BURST_TRANSFER_MODE_X8,
    .scale_down_factor = 0,
    .h_output_size = 0,
    .v_output_size = 0,
    .interrupts_enabled = CEU_EVENT_FRAME_END | CEU_EVENT_VD | CEU_EVENT_CRAM_OVERFLOW |
        CEU_EVENT_VD_ERROR | CEU_EVENT_HD_MISSING | CEU_EVENT_VD_MISSING,
    .ceu_ipl = 12,
    .ceu_irq = VECTOR_NUMBER_CEU_CEUI,
};

static const capture_cfg_t ra8p1_ceu_vga_cfg = {
    .x_capture_start_pixel = 0,
    .x_capture_pixels = RA8P1_CEU_WIDTH,
    .y_capture_start_pixel = 0,
    .y_capture_pixels = RA8P1_CEU_HEIGHT,
    .bytes_per_pixel = RA8P1_CEU_BPP,
    .p_callback = ra8p1_ceu_callback,
    .p_context = NULL,
    .p_extend = &ra8p1_ceu_vga_extend,
};

static const capture_instance_t ra8p1_ceu = {
    .p_ctrl = &ra8p1_ceu_ctrl,
    .p_cfg = &ra8p1_ceu_vga_cfg,
    .p_api = &g_ceu_on_capture,
};

static const ra8p1_ceu_sensor_reg_t ra8p1_ceu_ov5640_dvp_yuv422[] = {
    {0x3103, 0x11},
    {0x4740, 0x20},
    {0x4050, 0x6e}, {0x4051, 0x8f},
    {0x3103, 0x02},
    {0x3017, 0x7f}, {0x3018, 0xff}, {0x302c, 0xc2}, {0x3108, 0x01},
    {0x3630, 0x2e}, {0x3632, 0xe2}, {0x3633, 0x23}, {0x3621, 0xe0},
    {0x3704, 0xa0}, {0x3703, 0x5a}, {0x3715, 0x78}, {0x3717, 0x01},
    {0x370b, 0x60}, {0x3705, 0x1a}, {0x3905, 0x02}, {0x3906, 0x10},
    {0x3901, 0x0a}, {0x3731, 0x12}, {0x3600, 0x08}, {0x3601, 0x33},
    {0x302d, 0x60}, {0x3620, 0x52}, {0x371b, 0x20}, {0x471c, 0x50},
    {0x3a18, 0x00}, {0x3a19, 0xf8},
    {0x3635, 0x1c}, {0x3634, 0x40}, {0x3622, 0x01},
    {0x3c04, 0x28}, {0x3c05, 0x98}, {0x3c06, 0x00}, {0x3c07, 0x08},
    {0x3c08, 0x00}, {0x3c09, 0x1c}, {0x3c0a, 0x9c}, {0x3c0b, 0x40},
    {0x3820, 0x41}, {0x3821, 0x01},
    {0x3808, (uint8_t)(RA8P1_CEU_ACTIVE_WIDTH >> 8)}, {0x3809, (uint8_t)RA8P1_CEU_ACTIVE_WIDTH},
    {0x380a, (uint8_t)(RA8P1_CEU_ACTIVE_HEIGHT >> 8)}, {0x380b, (uint8_t)RA8P1_CEU_ACTIVE_HEIGHT},
    {0x3814, 0x31}, {0x3815, 0x31},
    {0x3034, 0x18}, {0x3035, 0x21}, {0x3036, 0x46}, {0x3037, 0x13},
    {0x3038, 0x00}, {0x3039, 0x00},
    {0x380c, 0x07}, {0x380d, 0x68}, {0x380e, 0x03}, {0x380f, 0xd8},
    {0x3c01, 0xb4}, {0x3c00, 0x04}, {0x3a08, 0x00}, {0x3a09, 0x93},
    {0x3a0e, 0x06}, {0x3a0a, 0x00}, {0x3a0b, 0x7b}, {0x3a0d, 0x08},
    {0x3a00, 0x3c}, {0x3a02, 0x05}, {0x3a03, 0xc4}, {0x3a14, 0x05},
    {0x3a15, 0xc4},
    {0x3618, 0x00}, {0x3612, 0x29}, {0x3708, 0x64}, {0x3709, 0x52},
    {0x370c, 0x03},
    {0x4001, 0x02}, {0x4004, 0x02},
    {0x3000, 0x00}, {0x3002, 0x1c}, {0x3004, 0xff}, {0x3006, 0xc3},
    {0x300e, 0x58}, {0x302e, 0x00}, {0x4300, 0x30}, {0x501f, 0x00},
    {0x4713, 0x03}, {0x4407, 0x04}, {0x460c, 0x22}, {0x3824, RA8P1_CEU_OV5640_PCLK_MANUAL_DIVIDER},
    {0x5001, 0xa3},
    {0x3406, 0x01}, {0x3400, 0x06}, {0x3401, 0x80}, {0x3402, 0x04},
    {0x3403, 0x00}, {0x3404, 0x06}, {0x3405, 0x00},
    {0x5180, 0xff}, {0x5181, 0xf2}, {0x5182, 0x00}, {0x5183, 0x14},
    {0x5184, 0x25}, {0x5185, 0x24}, {0x5186, 0x16}, {0x5187, 0x16},
    {0x5188, 0x16}, {0x5189, 0x62}, {0x518a, 0x62}, {0x518b, 0xf0},
    {0x518c, 0xb2}, {0x518d, 0x50}, {0x518e, 0x30}, {0x518f, 0x30},
    {0x5190, 0x50}, {0x5191, 0xf8}, {0x5192, 0x04}, {0x5193, 0x70},
    {0x5194, 0xf0}, {0x5195, 0xf0}, {0x5196, 0x03}, {0x5197, 0x01},
    {0x5198, 0x04}, {0x5199, 0x12}, {0x519a, 0x04}, {0x519b, 0x00},
    {0x519c, 0x06}, {0x519d, 0x82}, {0x519e, 0x38},
    {0x5381, 0x1e}, {0x5382, 0x5b}, {0x5383, 0x14}, {0x5384, 0x06},
    {0x5385, 0x82}, {0x5386, 0x88}, {0x5387, 0x7c}, {0x5388, 0x60},
    {0x5389, 0x1c}, {0x538a, 0x01}, {0x538b, 0x98},
    {0x5300, 0x08}, {0x5301, 0x30}, {0x5302, 0x3f}, {0x5303, 0x10},
    {0x5304, 0x08}, {0x5305, 0x30}, {0x5306, 0x18}, {0x5307, 0x28},
    {0x5309, 0x08}, {0x530a, 0x30}, {0x530b, 0x04}, {0x530c, 0x06},
    {0x5480, 0x01}, {0x5481, 0x06}, {0x5482, 0x12}, {0x5483, 0x24},
    {0x5484, 0x4a}, {0x5485, 0x58}, {0x5486, 0x65}, {0x5487, 0x72},
    {0x5488, 0x7d}, {0x5489, 0x88}, {0x548a, 0x92}, {0x548b, 0xa3},
    {0x548c, 0xb2}, {0x548d, 0xc8}, {0x548e, 0xdd}, {0x548f, 0xf0},
    {0x5490, 0x15},
    {0x5580, 0x04}, {0x5583, 0x40}, {0x5584, 0x20}, {0x5589, 0x10},
    {0x558a, 0x00}, {0x558b, 0xf8}, {0x5585, 0x00}, {0x5586, 0x20},
    {0x5587, 0x00}, {0x5588, 0x01}, {0x501d, 0x40},
    {0x5000, 0xa7}, {0x5800, 0x20}, {0x5801, 0x19}, {0x5802, 0x17},
    {0x5803, 0x16}, {0x5804, 0x18}, {0x5805, 0x21}, {0x5806, 0x0f},
    {0x5807, 0x0a}, {0x5808, 0x07}, {0x5809, 0x07}, {0x580a, 0x0a},
    {0x580b, 0x0c}, {0x580c, 0x0a}, {0x580d, 0x03}, {0x580e, 0x01},
    {0x580f, 0x01}, {0x5810, 0x03}, {0x5811, 0x09}, {0x5812, 0x0a},
    {0x5813, 0x03}, {0x5814, 0x01}, {0x5815, 0x01}, {0x5816, 0x03},
    {0x5817, 0x08}, {0x5818, 0x10}, {0x5819, 0x0a}, {0x581a, 0x06},
    {0x581b, 0x06}, {0x581c, 0x08}, {0x581d, 0x0e}, {0x581e, 0x22},
    {0x581f, 0x18}, {0x5820, 0x13}, {0x5821, 0x12}, {0x5822, 0x16},
    {0x5823, 0x1e}, {0x5824, 0x64}, {0x5825, 0x2a}, {0x5826, 0x2c},
    {0x5827, 0x2a}, {0x5828, 0x46}, {0x5829, 0x2a}, {0x582a, 0x26},
    {0x582b, 0x24}, {0x582c, 0x26}, {0x582d, 0x2a}, {0x582e, 0x28},
    {0x582f, 0x42}, {0x5830, 0x40}, {0x5831, 0x42}, {0x5832, 0x08},
    {0x5833, 0x28}, {0x5834, 0x26}, {0x5835, 0x24}, {0x5836, 0x26},
    {0x5837, 0x2a}, {0x5838, 0x44}, {0x5839, 0x4a}, {0x583a, 0x2c},
    {0x583b, 0x2a}, {0x583c, 0x46}, {0x583d, 0xce},
    {0x5688, 0x22}, {0x5689, 0x22}, {0x568a, 0x42}, {0x568b, 0x24},
    {0x568c, 0x42}, {0x568d, 0x24}, {0x568e, 0x22}, {0x568f, 0x22},
    {0x5025, 0x00},
    {0x3a0f, 0x30}, {0x3a10, 0x28}, {0x3a1b, 0x30}, {0x3a1e, 0x28},
    {0x3a11, 0x61}, {0x3a1f, 0x10},
    {0x4005, 0x1a}, {0x3406, 0x00}, {0x3503, 0x00}, {0x3008, 0x02},
    {0x4745, RA8P1_CEU_OV5640_DVP_DATA_ORDER}, {0x4301, 0x01},
    {0x503d, 0x00}, {0x4741, 0x00},
    {RA8P1_CEU_OV5640_END, 0xff},
};

static const ra8p1_ceu_sensor_reg_t ra8p1_ceu_ov5640_vga[] = {
    {0x3800, 0x00}, {0x3801, 0x00}, {0x3802, 0x00}, {0x3803, 0x00},
    {0x3804, 0x0a}, {0x3805, 0x3f}, {0x3806, 0x07}, {0x3807, 0x9b},
    {0x3808, (uint8_t)(RA8P1_CEU_WIDTH >> 8)}, {0x3809, (uint8_t)RA8P1_CEU_WIDTH},
    {0x380a, (uint8_t)(RA8P1_CEU_HEIGHT >> 8)}, {0x380b, (uint8_t)RA8P1_CEU_HEIGHT},
    {0x380c, 0x0c}, {0x380d, 0x80}, {0x380e, 0x07}, {0x380f, 0xd0},
    {0x5001, 0xa3},
    {0x5680, 0x00}, {0x5681, 0x00}, {0x5682, 0x0a}, {0x5683, 0x20},
    {0x5684, 0x00}, {0x5685, 0x00}, {0x5686, 0x07}, {0x5687, 0x98},
    {RA8P1_CEU_OV5640_END, 0xff},
};

static void ra8p1_ceu_callback(capture_callback_args_t *p_args) {
    if (p_args != NULL) {
        ra8p1_ceu_callbacks++;
        ra8p1_ceu_last_event = p_args->event;
        ra8p1_ceu_last_event_status = p_args->event_status;
        ra8p1_ceu_last_interrupt_status = p_args->interrupt_status;
        if ((p_args->event & CEU_EVENT_FRAME_END) != 0U) {
            ra8p1_ceu_capture_ready = true;
        }
    }
}

static void ra8p1_ceu_raise_fsp(const char *op, fsp_err_t err) {
    if (err != FSP_SUCCESS) {
        mp_raise_msg_varg(&mp_type_OSError, MP_ERROR_TEXT("%s failed: %d"), op, err);
    }
}

static uint32_t ra8p1_ceu_pin_pfs(bsp_io_port_pin_t pin) {
    return R_PFS->PORT[pin >> 8].PIN[pin & RA8P1_CEU_PIN_INDEX_MASK].PmnPFS;
}

static bool ra8p1_ceu_pin_read(bsp_io_port_pin_t pin) {
    return R_BSP_PinRead(pin) != 0U;
}

static void ra8p1_ceu_i2c_gpio_configure(void) {
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

static void ra8p1_ceu_i2c_delay(void) {
    R_BSP_SoftwareDelay(RA8P1_CEU_I2C_DELAY_US, BSP_DELAY_UNITS_MICROSECONDS);
}

static void ra8p1_ceu_i2c_scl_write(bool high) {
    R_BSP_PinWrite(BSP_IO_PORT_05_PIN_12, high ? BSP_IO_LEVEL_HIGH : BSP_IO_LEVEL_LOW);
}

static void ra8p1_ceu_i2c_sda_write(bool high) {
    R_BSP_PinWrite(BSP_IO_PORT_05_PIN_11, high ? BSP_IO_LEVEL_HIGH : BSP_IO_LEVEL_LOW);
}

static bool ra8p1_ceu_i2c_scl_read(void) {
    return R_BSP_PinRead(BSP_IO_PORT_05_PIN_12) != 0U;
}

static bool ra8p1_ceu_i2c_sda_read(void) {
    return R_BSP_PinRead(BSP_IO_PORT_05_PIN_11) != 0U;
}

static fsp_err_t ra8p1_ceu_i2c_scl_high(void) {
    ra8p1_ceu_i2c_scl_write(true);
    for (uint32_t i = 0; i < 1000U; i++) {
        if (ra8p1_ceu_i2c_scl_read()) {
            ra8p1_ceu_i2c_delay();
            return FSP_SUCCESS;
        }
        ra8p1_ceu_i2c_delay();
    }
    return FSP_ERR_TIMEOUT;
}

static void ra8p1_ceu_i2c_start(void) {
    ra8p1_ceu_i2c_sda_write(true);
    (void)ra8p1_ceu_i2c_scl_high();
    ra8p1_ceu_i2c_sda_write(false);
    ra8p1_ceu_i2c_delay();
    ra8p1_ceu_i2c_scl_write(false);
    ra8p1_ceu_i2c_delay();
}

static void ra8p1_ceu_i2c_stop(void) {
    ra8p1_ceu_i2c_sda_write(false);
    ra8p1_ceu_i2c_delay();
    (void)ra8p1_ceu_i2c_scl_high();
    ra8p1_ceu_i2c_sda_write(true);
    ra8p1_ceu_i2c_delay();
}

static void ra8p1_ceu_i2c_begin(void) {
    R_BSP_PinAccessEnable();
    ra8p1_ceu_i2c_gpio_configure();
    ra8p1_ceu_i2c_delay();
}

static void ra8p1_ceu_i2c_end(void) {
    R_BSP_PinAccessDisable();
}

static fsp_err_t ra8p1_ceu_i2c_write_byte(uint8_t byte) {
    for (uint32_t bit = 0; bit < 8U; bit++) {
        ra8p1_ceu_i2c_sda_write((byte & 0x80U) != 0U);
        ra8p1_ceu_i2c_delay();
        fsp_err_t err = ra8p1_ceu_i2c_scl_high();
        if (err != FSP_SUCCESS) {
            return err;
        }
        ra8p1_ceu_i2c_scl_write(false);
        ra8p1_ceu_i2c_delay();
        byte <<= 1;
    }

    ra8p1_ceu_i2c_sda_write(true);
    ra8p1_ceu_i2c_delay();
    fsp_err_t err = ra8p1_ceu_i2c_scl_high();
    if (err != FSP_SUCCESS) {
        return err;
    }
    bool ack = !ra8p1_ceu_i2c_sda_read();
    ra8p1_ceu_i2c_scl_write(false);
    ra8p1_ceu_i2c_delay();
    return ack ? FSP_SUCCESS : FSP_ERR_ABORTED;
}

static fsp_err_t ra8p1_ceu_i2c_read_byte(uint8_t *out, bool ack) {
    uint8_t value = 0;
    ra8p1_ceu_i2c_sda_write(true);
    for (uint32_t bit = 0; bit < 8U; bit++) {
        value <<= 1;
        fsp_err_t err = ra8p1_ceu_i2c_scl_high();
        if (err != FSP_SUCCESS) {
            return err;
        }
        if (ra8p1_ceu_i2c_sda_read()) {
            value |= 1U;
        }
        ra8p1_ceu_i2c_scl_write(false);
        ra8p1_ceu_i2c_delay();
    }

    ra8p1_ceu_i2c_sda_write(!ack);
    ra8p1_ceu_i2c_delay();
    fsp_err_t err = ra8p1_ceu_i2c_scl_high();
    if (err != FSP_SUCCESS) {
        return err;
    }
    ra8p1_ceu_i2c_scl_write(false);
    ra8p1_ceu_i2c_sda_write(true);
    ra8p1_ceu_i2c_delay();
    *out = value;
    return FSP_SUCCESS;
}

static fsp_err_t ra8p1_ceu_i2c_write(uint8_t addr, const uint8_t *data, size_t len) {
    fsp_err_t err;
    ra8p1_ceu_last_i2c_addr = addr;
    ra8p1_ceu_last_i2c_reg = (len > 1U) ? (((uint32_t)data[0] << 8) | data[1]) : 0U;
    ra8p1_ceu_i2c_begin();
    ra8p1_ceu_i2c_start();
    err = ra8p1_ceu_i2c_write_byte((uint8_t)(addr << 1));
    for (size_t i = 0; (i < len) && (err == FSP_SUCCESS); i++) {
        err = ra8p1_ceu_i2c_write_byte(data[i]);
    }
    ra8p1_ceu_i2c_stop();
    ra8p1_ceu_i2c_end();
    ra8p1_ceu_last_i2c_err = err;
    return err;
}

static fsp_err_t ra8p1_ceu_i2c_write_reg16(uint8_t addr, uint16_t reg, uint8_t value) {
    uint8_t data[3] = { (uint8_t)(reg >> 8), (uint8_t)reg, value };
    return ra8p1_ceu_i2c_write(addr, data, sizeof(data));
}

static fsp_err_t ra8p1_ceu_i2c_write_reg8(uint8_t addr, uint8_t reg, uint8_t value) {
    uint8_t data[2] = { reg, value };
    return ra8p1_ceu_i2c_write(addr, data, sizeof(data));
}

static fsp_err_t ra8p1_ceu_i2c_read_reg8(uint8_t addr, uint8_t reg, uint8_t *out) {
    fsp_err_t err;
    ra8p1_ceu_last_i2c_addr = addr;
    ra8p1_ceu_last_i2c_reg = reg;
    ra8p1_ceu_i2c_begin();
    ra8p1_ceu_i2c_start();
    err = ra8p1_ceu_i2c_write_byte((uint8_t)(addr << 1));
    if (err == FSP_SUCCESS) {
        err = ra8p1_ceu_i2c_write_byte(reg);
    }
    if (err == FSP_SUCCESS) {
        ra8p1_ceu_i2c_start();
        err = ra8p1_ceu_i2c_write_byte((uint8_t)((addr << 1) | 1U));
    }
    if (err == FSP_SUCCESS) {
        err = ra8p1_ceu_i2c_read_byte(out, false);
    }
    ra8p1_ceu_i2c_stop();
    ra8p1_ceu_i2c_end();
    ra8p1_ceu_last_i2c_err = err;
    return err;
}

static fsp_err_t ra8p1_ceu_i2c_read_reg16(uint8_t addr, uint16_t reg, uint8_t *out) {
    fsp_err_t err;
    ra8p1_ceu_last_i2c_addr = addr;
    ra8p1_ceu_last_i2c_reg = reg;
    ra8p1_ceu_i2c_begin();
    ra8p1_ceu_i2c_start();
    err = ra8p1_ceu_i2c_write_byte((uint8_t)(addr << 1));
    if (err == FSP_SUCCESS) {
        err = ra8p1_ceu_i2c_write_byte((uint8_t)(reg >> 8));
    }
    if (err == FSP_SUCCESS) {
        err = ra8p1_ceu_i2c_write_byte((uint8_t)reg);
    }
    if (err == FSP_SUCCESS) {
        ra8p1_ceu_i2c_start();
        err = ra8p1_ceu_i2c_write_byte((uint8_t)((addr << 1) | 1U));
    }
    if (err == FSP_SUCCESS) {
        err = ra8p1_ceu_i2c_read_byte(out, false);
    }
    ra8p1_ceu_i2c_stop();
    ra8p1_ceu_i2c_end();
    ra8p1_ceu_last_i2c_err = err;
    return err;
}

static fsp_err_t ra8p1_ceu_i2c_probe_addr(uint8_t addr) {
    fsp_err_t err;
    ra8p1_ceu_last_i2c_addr = addr;
    ra8p1_ceu_last_i2c_reg = 0;
    ra8p1_ceu_i2c_begin();
    ra8p1_ceu_i2c_start();
    err = ra8p1_ceu_i2c_write_byte((uint8_t)(addr << 1));
    ra8p1_ceu_i2c_stop();
    ra8p1_ceu_i2c_end();
    ra8p1_ceu_last_i2c_err = err;
    return err;
}

static fsp_err_t ra8p1_ceu_switch_select_parallel_camera(void) {
    uint8_t output = 0;
    uint8_t direction = 0;
    uint8_t output_enable = 0;
    const uint8_t route_mask = (uint8_t)RA8P1_CEU_SWITCH_ROUTE_MASK;

    fsp_err_t err = ra8p1_ceu_i2c_read_reg8(RA8P1_CEU_SWITCH_ADDR, RA8P1_CEU_SWITCH_OUTPUT_STATE_REG, &output);
    if (err == FSP_SUCCESS) {
        err = ra8p1_ceu_i2c_write_reg8(
            RA8P1_CEU_SWITCH_ADDR, RA8P1_CEU_SWITCH_OUTPUT_STATE_REG, (uint8_t)(output & ~route_mask));
    }
    if (err == FSP_SUCCESS) {
        err = ra8p1_ceu_i2c_read_reg8(RA8P1_CEU_SWITCH_ADDR, RA8P1_CEU_SWITCH_PIN_DIR_REG, &direction);
    }
    if (err == FSP_SUCCESS) {
        err = ra8p1_ceu_i2c_write_reg8(
            RA8P1_CEU_SWITCH_ADDR, RA8P1_CEU_SWITCH_PIN_DIR_REG, (uint8_t)(direction | route_mask));
    }
    if (err == FSP_SUCCESS) {
        err = ra8p1_ceu_i2c_read_reg8(RA8P1_CEU_SWITCH_ADDR, RA8P1_CEU_SWITCH_OUTPUT_ENABLE_REG, &output_enable);
    }
    if (err == FSP_SUCCESS) {
        err = ra8p1_ceu_i2c_write_reg8(
            RA8P1_CEU_SWITCH_ADDR, RA8P1_CEU_SWITCH_OUTPUT_ENABLE_REG, (uint8_t)(output_enable & ~route_mask));
    }
    ra8p1_ceu_last_switch_err = err;
    return err;
}

static void ra8p1_ceu_pins_configure(void) {
    const uint32_t ceu_pin_cfg =
        (uint32_t)IOPORT_CFG_DRIVE_MID |
        (uint32_t)IOPORT_CFG_PERIPHERAL_PIN |
        (uint32_t)IOPORT_PERIPHERAL_CEU;

    R_BSP_PinAccessEnable();
    R_BSP_PinCfg(BSP_IO_PORT_05_PIN_01,
        (uint32_t)IOPORT_CFG_DRIVE_HS_HIGH |
        (uint32_t)IOPORT_CFG_PERIPHERAL_PIN |
        (uint32_t)IOPORT_PERIPHERAL_GPT1);
    R_BSP_PinCfg(BSP_IO_PORT_04_PIN_00, ceu_pin_cfg);
    R_BSP_PinCfg(BSP_IO_PORT_09_PIN_02, ceu_pin_cfg);
    R_BSP_PinCfg(BSP_IO_PORT_04_PIN_05, ceu_pin_cfg);
    R_BSP_PinCfg(BSP_IO_PORT_04_PIN_06, ceu_pin_cfg);
    R_BSP_PinCfg(BSP_IO_PORT_07_PIN_00, ceu_pin_cfg);
    R_BSP_PinCfg(BSP_IO_PORT_07_PIN_01, ceu_pin_cfg);
    R_BSP_PinCfg(BSP_IO_PORT_07_PIN_02, ceu_pin_cfg);
    R_BSP_PinCfg(BSP_IO_PORT_07_PIN_03, ceu_pin_cfg);
    R_BSP_PinCfg(BSP_IO_PORT_11_PIN_02, ceu_pin_cfg);
    R_BSP_PinCfg(BSP_IO_PORT_11_PIN_03, ceu_pin_cfg);
    R_BSP_PinCfg(BSP_IO_PORT_11_PIN_04, ceu_pin_cfg);
    R_BSP_PinCfg(BSP_IO_PORT_07_PIN_09,
        (uint32_t)IOPORT_CFG_DRIVE_HIGH |
        (uint32_t)IOPORT_CFG_PORT_DIRECTION_OUTPUT |
        (uint32_t)IOPORT_CFG_PORT_OUTPUT_HIGH);
    ra8p1_ceu_i2c_gpio_configure();
    R_BSP_PinAccessDisable();
    ra8p1_ceu_last_pin_err = FSP_SUCCESS;
    ra8p1_ceu_display_pins_restored = false;
}

static void ra8p1_ceu_display_shared_pins_restore(void) {
    R_BSP_PinAccessEnable();
    R_BSP_PinCfg(BSP_IO_PORT_09_PIN_02, RA8P1_CEU_LCD_SHARED_PIN_CFG);
    R_BSP_PinCfg(BSP_IO_PORT_11_PIN_02, RA8P1_CEU_LCD_SHARED_PIN_CFG);
    R_BSP_PinCfg(BSP_IO_PORT_11_PIN_03, RA8P1_CEU_LCD_SHARED_PIN_CFG);
    R_BSP_PinCfg(BSP_IO_PORT_11_PIN_04, RA8P1_CEU_LCD_SHARED_PIN_CFG);
    R_BSP_PinAccessDisable();
    ra8p1_ceu_last_display_pin_restore_err = FSP_SUCCESS;
    ra8p1_ceu_display_pins_restored = true;
}

static fsp_err_t ra8p1_ceu_display_backlight_set(bool on) {
    R_BSP_PinAccessEnable();
    R_BSP_PinCfg(BSP_IO_PORT_05_PIN_14,
        (uint32_t)IOPORT_CFG_PORT_DIRECTION_OUTPUT |
        (on ? (uint32_t)IOPORT_CFG_PORT_OUTPUT_HIGH : (uint32_t)IOPORT_CFG_PORT_OUTPUT_LOW));
    R_BSP_PinAccessDisable();
    ra8p1_ceu_display_backlight_on = on;
    ra8p1_ceu_last_display_backlight_err = FSP_SUCCESS;
    return FSP_SUCCESS;
}

static fsp_err_t ra8p1_ceu_xclk_start(void) {
    if (!ra8p1_ceu_xclk_open) {
        ra8p1_ceu_last_gpt_open_err = ra8p1_ceu_xclk.p_api->open(ra8p1_ceu_xclk.p_ctrl, ra8p1_ceu_xclk.p_cfg);
        if ((ra8p1_ceu_last_gpt_open_err != FSP_SUCCESS) && (ra8p1_ceu_last_gpt_open_err != FSP_ERR_ALREADY_OPEN)) {
            return ra8p1_ceu_last_gpt_open_err;
        }
        ra8p1_ceu_xclk_open = true;
    }

    timer_info_t xclk_info = { 0 };
    fsp_err_t info_err = ra8p1_ceu_xclk.p_api->infoGet(ra8p1_ceu_xclk.p_ctrl, &xclk_info);
    uint32_t xclk_clock_hz = ((info_err == FSP_SUCCESS) && (xclk_info.clock_frequency != 0U)) ?
        xclk_info.clock_frequency : ra8p1_ceu_gptclk_hz();
    uint32_t xclk_period_counts = ra8p1_ceu_xclk_period_counts_for(xclk_clock_hz);
    ra8p1_ceu_last_gpt_period_err = ra8p1_ceu_xclk.p_api->periodSet(ra8p1_ceu_xclk.p_ctrl, xclk_period_counts);
    if (ra8p1_ceu_last_gpt_period_err != FSP_SUCCESS) {
        return ra8p1_ceu_last_gpt_period_err;
    }

    ra8p1_ceu_last_gpt_start_err = ra8p1_ceu_xclk.p_api->start(ra8p1_ceu_xclk.p_ctrl);
    if ((ra8p1_ceu_last_gpt_start_err != FSP_SUCCESS) && (ra8p1_ceu_last_gpt_start_err != FSP_ERR_ALREADY_OPEN)) {
        return ra8p1_ceu_last_gpt_start_err;
    }
    return FSP_SUCCESS;
}

static void ra8p1_ceu_camera_hw_reset(void) {
    R_BSP_PinAccessEnable();
    R_BSP_PinWrite(BSP_IO_PORT_07_PIN_09, BSP_IO_LEVEL_LOW);
    R_BSP_PinAccessDisable();
    R_BSP_SoftwareDelay(20, BSP_DELAY_UNITS_MILLISECONDS);
    R_BSP_PinAccessEnable();
    R_BSP_PinWrite(BSP_IO_PORT_07_PIN_09, BSP_IO_LEVEL_HIGH);
    R_BSP_PinAccessDisable();
    R_BSP_SoftwareDelay(20, BSP_DELAY_UNITS_MILLISECONDS);
}

static fsp_err_t ra8p1_ceu_camera_access_prepare(void) {
    ra8p1_ceu_pins_configure();
    fsp_err_t err = ra8p1_ceu_switch_select_parallel_camera();
    if (err == FSP_SUCCESS) {
        err = ra8p1_ceu_xclk_start();
    }
    if (err == FSP_SUCCESS) {
        R_BSP_SoftwareDelay(20, BSP_DELAY_UNITS_MILLISECONDS);
    }
    ra8p1_ceu_last_camera_err = err;
    return err;
}

static fsp_err_t ra8p1_ceu_camera_write_array(const ra8p1_ceu_sensor_reg_t *array) {
    fsp_err_t err = FSP_SUCCESS;
    for (const ra8p1_ceu_sensor_reg_t *p = array; p->reg != RA8P1_CEU_OV5640_END; p++) {
        if (p->reg == RA8P1_CEU_OV5640_WAIT) {
            R_BSP_SoftwareDelay(p->val, BSP_DELAY_UNITS_MILLISECONDS);
            continue;
        }
        err = ra8p1_ceu_i2c_write_reg16(RA8P1_CEU_CAMERA_ADDR, p->reg, p->val);
        if (err != FSP_SUCCESS) {
            return err;
        }
        ra8p1_ceu_camera_writes++;
    }
    return err;
}

static fsp_err_t ra8p1_ceu_camera_apply_tuning(void) {
    fsp_err_t err = FSP_SUCCESS;
    if (!ra8p1_ceu_tuning_enabled) {
        err = ra8p1_ceu_i2c_write_reg16(
            RA8P1_CEU_CAMERA_ADDR, 0x3503, RA8P1_CEU_OV5640_AEC_AUTO_ENABLE);
        if (err == FSP_SUCCESS) {
            err = ra8p1_ceu_i2c_write_reg16(
                RA8P1_CEU_CAMERA_ADDR, 0x5308, RA8P1_CEU_OV5640_CIP_CTRL_AUTO);
        }
        ra8p1_ceu_last_tuning_err = err;
        return err;
    }

    uint32_t exposure = ra8p1_ceu_tuning_exposure;
    uint32_t gain = ra8p1_ceu_tuning_gain;
    err = ra8p1_ceu_i2c_write_reg16(
        RA8P1_CEU_CAMERA_ADDR, 0x3503, RA8P1_CEU_OV5640_AEC_MANUAL_ENABLE);
    if (err == FSP_SUCCESS) {
        err = ra8p1_ceu_i2c_write_reg16(RA8P1_CEU_CAMERA_ADDR, 0x3500, (uint8_t)((exposure >> 12) & 0x0fU));
    }
    if (err == FSP_SUCCESS) {
        err = ra8p1_ceu_i2c_write_reg16(RA8P1_CEU_CAMERA_ADDR, 0x3501, (uint8_t)((exposure >> 4) & 0xffU));
    }
    if (err == FSP_SUCCESS) {
        err = ra8p1_ceu_i2c_write_reg16(RA8P1_CEU_CAMERA_ADDR, 0x3502, (uint8_t)((exposure << 4) & 0xf0U));
    }
    if (err == FSP_SUCCESS) {
        err = ra8p1_ceu_i2c_write_reg16(RA8P1_CEU_CAMERA_ADDR, 0x350a, (uint8_t)((gain >> 8) & 0x03U));
    }
    if (err == FSP_SUCCESS) {
        err = ra8p1_ceu_i2c_write_reg16(RA8P1_CEU_CAMERA_ADDR, 0x350b, (uint8_t)(gain & 0xffU));
    }
    if (err == FSP_SUCCESS) {
        err = ra8p1_ceu_i2c_write_reg16(
            RA8P1_CEU_CAMERA_ADDR, 0x5308, (uint8_t)(ra8p1_ceu_tuning_edge_ctrl & 0xffU));
    }
    ra8p1_ceu_last_tuning_err = err;
    return err;
}

static fsp_err_t ra8p1_ceu_camera_open_do(void) {
    fsp_err_t err = ra8p1_ceu_xclk_start();
    if (err != FSP_SUCCESS) {
        ra8p1_ceu_last_camera_err = err;
        return err;
    }

    R_BSP_SoftwareDelay(10, BSP_DELAY_UNITS_MILLISECONDS);
    ra8p1_ceu_camera_hw_reset();

    err = ra8p1_ceu_i2c_write_reg16(RA8P1_CEU_CAMERA_ADDR, RA8P1_CEU_REG_RESET, 0x80);
    if (err == FSP_SUCCESS) {
        R_BSP_SoftwareDelay(100, BSP_DELAY_UNITS_MILLISECONDS);
        ra8p1_ceu_camera_writes = 0;
        err = ra8p1_ceu_camera_write_array(ra8p1_ceu_ov5640_dvp_yuv422);
    }
    if (err == FSP_SUCCESS) {
        R_BSP_SoftwareDelay(50, BSP_DELAY_UNITS_MILLISECONDS);
        err = ra8p1_ceu_camera_write_array(ra8p1_ceu_ov5640_vga);
    }
    if (err == FSP_SUCCESS) {
        err = ra8p1_ceu_i2c_write_reg16(
            RA8P1_CEU_CAMERA_ADDR, 0x503d, ra8p1_ceu_test_pattern_enabled ? 0x80 : 0x00);
    }
    if (err == FSP_SUCCESS) {
        err = ra8p1_ceu_camera_apply_tuning();
    }
    if (err == FSP_SUCCESS) {
        R_BSP_SoftwareDelay(50, BSP_DELAY_UNITS_MILLISECONDS);
    }
    ra8p1_ceu_last_camera_err = err;
    return err;
}

static fsp_err_t ra8p1_ceu_open_do(void) {
    if (ra8p1_ceu_capture_open) {
        return FSP_SUCCESS;
    }

    ra8p1_ceu_last_ceu_open_err = ra8p1_ceu.p_api->open(ra8p1_ceu.p_ctrl, ra8p1_ceu.p_cfg);
    if ((ra8p1_ceu_last_ceu_open_err != FSP_SUCCESS) && (ra8p1_ceu_last_ceu_open_err != FSP_ERR_ALREADY_OPEN)) {
        return ra8p1_ceu_last_ceu_open_err;
    }
    ra8p1_ceu_capture_open = true;
    return FSP_SUCCESS;
}

static fsp_err_t ra8p1_ceu_init_do(void) {
    ra8p1_ceu_pins_configure();
    fsp_err_t err = ra8p1_ceu_switch_select_parallel_camera();
    if (err != FSP_SUCCESS) {
        return err;
    }
    err = ra8p1_ceu_camera_open_do();
    if (err != FSP_SUCCESS) {
        return err;
    }
    return ra8p1_ceu_open_do();
}

static void ra8p1_ceu_capture_close_do(void) {
    if (ra8p1_ceu_capture_open) {
        (void)ra8p1_ceu.p_api->close(ra8p1_ceu.p_ctrl);
        ra8p1_ceu_capture_open = false;
    }
}

static void ra8p1_ceu_xclk_close_do(void) {
    if (ra8p1_ceu_xclk_open) {
        (void)ra8p1_ceu_xclk.p_api->stop(ra8p1_ceu_xclk.p_ctrl);
        (void)ra8p1_ceu_xclk.p_api->close(ra8p1_ceu_xclk.p_ctrl);
        ra8p1_ceu_xclk_open = false;
    }
}

static void ra8p1_ceu_buffer_clear(void) {
    memset(ra8p1_ceu_image_vga, 0, RA8P1_CEU_BUFFER_SIZE);
}

static uint32_t ra8p1_ceu_buffer_checksum(uint32_t *nonzero_out) {
    uint32_t checksum = 5381U;
    uint32_t nonzero = 0;
    uint32_t bytes = (RA8P1_CEU_BUFFER_SIZE < RA8P1_CEU_STATUS_SAMPLE_BYTES) ?
        RA8P1_CEU_BUFFER_SIZE : RA8P1_CEU_STATUS_SAMPLE_BYTES;
    for (uint32_t i = 0; i < bytes; i++) {
        uint8_t value = ra8p1_ceu_image_vga[i];
        checksum = ((checksum << 5) + checksum) + value;
        if (value != 0) {
            nonzero++;
        }
    }
    *nonzero_out = nonzero;
    return checksum;
}

static void ra8p1_ceu_dcache_clean(void *addr, size_t size) {
#if defined(MP_HAL_CLEAN_DCACHE)
    uintptr_t start = (uintptr_t)addr & ~((uintptr_t)RA8P1_CEU_CACHE_LINE_BYTES - 1U);
    uintptr_t end = ((uintptr_t)addr + size + RA8P1_CEU_CACHE_LINE_BYTES - 1U) &
        ~((uintptr_t)RA8P1_CEU_CACHE_LINE_BYTES - 1U);
    MP_HAL_CLEAN_DCACHE((void *)start, (int32_t)(end - start));
#else
    (void)addr;
    (void)size;
#endif
}

static void ra8p1_ceu_dcache_clean_invalidate(void *addr, size_t size) {
#if defined(MP_HAL_CLEANINVALIDATE_DCACHE)
    uintptr_t start = (uintptr_t)addr & ~((uintptr_t)RA8P1_CEU_CACHE_LINE_BYTES - 1U);
    uintptr_t end = ((uintptr_t)addr + size + RA8P1_CEU_CACHE_LINE_BYTES - 1U) &
        ~((uintptr_t)RA8P1_CEU_CACHE_LINE_BYTES - 1U);
    MP_HAL_CLEANINVALIDATE_DCACHE((void *)start, (int32_t)(end - start));
#else
    (void)addr;
    (void)size;
#endif
}

static void ra8p1_ceu_dcache_invalidate(void *addr, size_t size) {
#if defined(RA8P1)
    uintptr_t start = (uintptr_t)addr & ~((uintptr_t)RA8P1_CEU_CACHE_LINE_BYTES - 1U);
    uintptr_t end = ((uintptr_t)addr + size + RA8P1_CEU_CACHE_LINE_BYTES - 1U) &
        ~((uintptr_t)RA8P1_CEU_CACHE_LINE_BYTES - 1U);
    SCB_InvalidateDCache_by_Addr((volatile void *)start, (int32_t)(end - start));
#else
    (void)addr;
    (void)size;
#endif
}

static fsp_err_t ra8p1_ceu_capture_frame(uint32_t timeout_ms, bool force_reopen) {
    if (force_reopen && ra8p1_ceu_capture_open) {
        (void)ra8p1_ceu.p_api->close(ra8p1_ceu.p_ctrl);
        ra8p1_ceu_capture_open = false;
    }

    fsp_err_t err = ra8p1_ceu_capture_open ? FSP_SUCCESS : ra8p1_ceu_init_do();
    if (err == FSP_SUCCESS) {
        ra8p1_ceu_capture_ready = false;
        ra8p1_ceu_callbacks = 0;
        ra8p1_ceu_last_event = 0;
        ra8p1_ceu_last_event_status = 0;
        ra8p1_ceu_last_interrupt_status = 0;
        ra8p1_ceu_buffer_clear();
        ra8p1_ceu_dcache_clean_invalidate(ra8p1_ceu_image_vga, RA8P1_CEU_BUFFER_SIZE);
        ra8p1_ceu_last_capture_start_err = ra8p1_ceu.p_api->captureStart(ra8p1_ceu.p_ctrl, ra8p1_ceu_image_vga);
        err = ra8p1_ceu_last_capture_start_err;
        if (err == FSP_SUCCESS) {
            while ((timeout_ms > 0U) && !ra8p1_ceu_capture_ready) {
                R_BSP_SoftwareDelay(1, BSP_DELAY_UNITS_MILLISECONDS);
                timeout_ms--;
                if ((timeout_ms & 0x1fU) == 0U) {
                    mp_handle_pending(MP_HANDLE_PENDING_CALLBACKS_AND_EXCEPTIONS);
                }
            }
            if (!ra8p1_ceu_capture_ready) {
                err = FSP_ERR_TIMEOUT;
            } else {
                ra8p1_ceu_dcache_invalidate(ra8p1_ceu_image_vga, RA8P1_CEU_BUFFER_SIZE);
            }
        }
    } else {
        ra8p1_ceu_last_capture_start_err = err;
    }
    ra8p1_ceu_last_capture_frame_err = err;
    return err;
}

static inline uint32_t ra8p1_ceu_display_stride_pixels(void) {
    return (uint32_t)(DISPLAY_BUFFER_STRIDE_BYTES_INPUT0 / RA8P1_CEU_DISPLAY_PIXEL_BYTES);
}

static inline uint32_t ra8p1_ceu_display_pixel_count(void) {
    return ra8p1_ceu_display_stride_pixels() * (uint32_t)DISPLAY_VSIZE_INPUT0;
}

static uint32_t *ra8p1_ceu_display_buf(mp_int_t idx) {
    if ((idx < 0) || (idx >= DISPLAY_FRAMEBUFFER_COUNT_INPUT0)) {
        mp_raise_ValueError(MP_ERROR_TEXT("buffer index out of range"));
    }
    return (uint32_t *)&g_framebuffer[idx][0];
}

static fsp_err_t ra8p1_ceu_display_prepare(void) {
    R_BSP_SdramInit(true);
    ra8p1_ceu_last_display_open_err = g_display.p_api->open(g_display.p_ctrl, g_display.p_cfg);
    if ((ra8p1_ceu_last_display_open_err != FSP_SUCCESS) &&
        (ra8p1_ceu_last_display_open_err != FSP_ERR_ALREADY_OPEN)) {
        return ra8p1_ceu_last_display_open_err;
    }

    if (g_display_ctrl.state != RA8P1_CEU_DISPLAY_STARTED_STATE) {
        ra8p1_ceu_last_display_start_err = g_display.p_api->start(g_display.p_ctrl);
        if (ra8p1_ceu_last_display_start_err != FSP_SUCCESS) {
            return ra8p1_ceu_last_display_start_err;
        }
    } else {
        ra8p1_ceu_last_display_start_err = FSP_SUCCESS;
    }
    ra8p1_ceu_display_open = true;
    (void)ra8p1_ceu_display_backlight_set(true);
    return FSP_SUCCESS;
}

static fsp_err_t ra8p1_ceu_display_stop_if_active(void) {
    if (g_display_ctrl.state == RA8P1_CEU_DISPLAY_STARTED_STATE) {
        for (uint32_t attempt = 0; attempt < 50U; attempt++) {
            ra8p1_ceu_last_display_stop_err = g_display.p_api->stop(g_display.p_ctrl);
            if (ra8p1_ceu_last_display_stop_err != FSP_ERR_INVALID_UPDATE_TIMING) {
                break;
            }
            R_BSP_SoftwareDelay(1, BSP_DELAY_UNITS_MILLISECONDS);
        }
    } else {
        ra8p1_ceu_last_display_stop_err = FSP_SUCCESS;
    }
    return ra8p1_ceu_last_display_stop_err;
}

static void ra8p1_ceu_display_fill(uint32_t *buf, uint32_t color) {
    uint32_t pixels = ra8p1_ceu_display_pixel_count();
    for (uint32_t i = 0; i < pixels; i++) {
        buf[i] = color;
    }
}

static inline uint8_t ra8p1_ceu_clamp_u8(int32_t value) {
    if (value < 0) {
        return 0;
    }
    if (value > 255) {
        return 255;
    }
    return (uint8_t)value;
}

static inline uint32_t ra8p1_ceu_yuv_to_xrgb(uint8_t y, uint8_t cb, uint8_t cr) {
    int32_t c_b = (int32_t)cb - 128;
    int32_t c_r = (int32_t)cr - 128;
    uint8_t r = ra8p1_ceu_clamp_u8((int32_t)y + ((91881 * c_r) >> 16));
    uint8_t g = ra8p1_ceu_clamp_u8((int32_t)y - (((22554 * c_b) + (46802 * c_r)) >> 16));
    uint8_t b = ra8p1_ceu_clamp_u8((int32_t)y + ((116130 * c_b) >> 16));
    return ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
}

static inline void ra8p1_ceu_unpack_yuv422(const uint8_t *src, uint8_t *y0, uint8_t *cb, uint8_t *y1, uint8_t *cr) {
    switch (ra8p1_ceu_yuv_order) {
        case RA8P1_CEU_YUV_ORDER_CRYCBY:
            *cr = src[0];
            *y0 = src[1];
            *cb = src[2];
            *y1 = src[3];
            break;
        case RA8P1_CEU_YUV_ORDER_YCBYCR:
            *y0 = src[0];
            *cb = src[1];
            *y1 = src[2];
            *cr = src[3];
            break;
        case RA8P1_CEU_YUV_ORDER_YCRYCB:
            *y0 = src[0];
            *cr = src[1];
            *y1 = src[2];
            *cb = src[3];
            break;
        case RA8P1_CEU_YUV_ORDER_CBYCRY:
        default:
            *cb = src[0];
            *y0 = src[1];
            *cr = src[2];
            *y1 = src[3];
            break;
    }
}

static fsp_err_t ra8p1_ceu_render_to_display(mp_int_t idx, mp_int_t x, mp_int_t y, bool clear) {
    if ((x < 0) || (y < 0) ||
        ((uint32_t)x + RA8P1_CEU_WIDTH > DISPLAY_HSIZE_INPUT0) ||
        ((uint32_t)y + RA8P1_CEU_HEIGHT > DISPLAY_VSIZE_INPUT0)) {
        mp_raise_ValueError(MP_ERROR_TEXT("image position out of range"));
    }

    if (!ra8p1_ceu_display_open || (g_display_ctrl.state != RA8P1_CEU_DISPLAY_STARTED_STATE)) {
        fsp_err_t err = ra8p1_ceu_display_prepare();
        if (err != FSP_SUCCESS) {
            ra8p1_ceu_last_display_flip_err = err;
            return err;
        }
    }

    uint32_t *buf = ra8p1_ceu_display_buf(idx);
    if (clear) {
        ra8p1_ceu_display_fill(buf, 0x00000000U);
    }

    uint32_t stride = ra8p1_ceu_display_stride_pixels();
    for (uint32_t row = 0; row < RA8P1_CEU_HEIGHT; row++) {
        const uint8_t *src = &ra8p1_ceu_image_vga[row * RA8P1_CEU_WIDTH * RA8P1_CEU_BPP];
        uint32_t *dst = &buf[((uint32_t)y + row) * stride + (uint32_t)x];
        for (uint32_t col = 0; col < RA8P1_CEU_WIDTH; col += 2U) {
            uint8_t y0;
            uint8_t cb;
            uint8_t y1;
            uint8_t cr;
            ra8p1_ceu_unpack_yuv422(src, &y0, &cb, &y1, &cr);
            dst[col] = ra8p1_ceu_yuv_to_xrgb(y0, cb, cr);
            dst[col + 1U] = ra8p1_ceu_yuv_to_xrgb(y1, cb, cr);
            src += 4U;
        }
    }

    ra8p1_ceu_dcache_clean(buf, ra8p1_ceu_display_pixel_count() * RA8P1_CEU_DISPLAY_PIXEL_BYTES);
    ra8p1_ceu_last_display_flip_err = g_display.p_api->bufferChange(g_display.p_ctrl,
        (uint8_t *)buf, DISPLAY_FRAME_LAYER_1);
    if (ra8p1_ceu_last_display_flip_err == FSP_SUCCESS) {
        ra8p1_ceu_display_frames++;
    }
    return ra8p1_ceu_last_display_flip_err;
}

static uint32_t ra8p1_ceu_gptclk_hz(void) {
#if defined(BSP_CFG_GPTCLK_SOURCE) && defined(BSP_CFG_PLL2R_FREQUENCY_HZ) && \
    (BSP_CFG_GPTCLK_SOURCE == BSP_CLOCKS_SOURCE_CLOCK_PLL2R)
    return BSP_CFG_PLL2R_FREQUENCY_HZ / R_FSP_ClockDividerGet(BSP_CFG_GPTCLK_DIV);
#elif defined(BSP_CFG_GPTCLK_SOURCE) && defined(BSP_CFG_PLL2P_FREQUENCY_HZ) && \
    (BSP_CFG_GPTCLK_SOURCE == BSP_CLOCKS_SOURCE_CLOCK_PLL2P)
    return BSP_CFG_PLL2P_FREQUENCY_HZ / R_FSP_ClockDividerGet(BSP_CFG_GPTCLK_DIV);
#else
    return 0;
#endif
}

static uint32_t ra8p1_ceu_xclk_period_counts_for(uint32_t clock_hz) {
    if (clock_hz == 0U) {
        return RA8P1_CEU_XCLK_FALLBACK_PERIOD_COUNTS;
    }

    uint32_t period = (clock_hz + (RA8P1_CEU_XCLK_TARGET_HZ / 2U)) / RA8P1_CEU_XCLK_TARGET_HZ;
    if (period < 2U) {
        period = 2U;
    }
    return period;
}

static uint32_t ra8p1_ceu_xclk_period_counts(void) {
    return ra8p1_ceu_xclk_period_counts_for(ra8p1_ceu_gptclk_hz());
}

static void ra8p1_ceu_sample_pin(
    bsp_io_port_pin_t pin, uint32_t samples, uint32_t *high_out, uint32_t *edges_out, uint32_t *last_out) {
    uint32_t high = 0;
    uint32_t edges = 0;
    bool last = ra8p1_ceu_pin_read(pin);
    high += last ? 1U : 0U;
    for (uint32_t i = 1; i < samples; i++) {
        bool value = ra8p1_ceu_pin_read(pin);
        high += value ? 1U : 0U;
        edges += (value != last) ? 1U : 0U;
        last = value;
    }
    *high_out = high;
    *edges_out = edges;
    *last_out = last ? 1U : 0U;
}

static void ra8p1_ceu_dict_store_pin_sample(mp_obj_t dict, qstr key, bsp_io_port_pin_t pin, uint32_t samples) {
    uint32_t high = 0;
    uint32_t edges = 0;
    uint32_t last = 0;
    ra8p1_ceu_sample_pin(pin, samples, &high, &edges, &last);
    mp_obj_t tuple[4] = {
        mp_obj_new_int_from_uint(high),
        mp_obj_new_int_from_uint(edges),
        mp_obj_new_int_from_uint(last),
        mp_obj_new_int_from_uint(ra8p1_ceu_pin_pfs(pin)),
    };
    mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(key), mp_obj_new_tuple(4, tuple));
}

static mp_obj_t ra8p1_ceu_status_dict(void) {
    mp_obj_t status = mp_obj_new_dict(64);
    capture_status_t capture_status = { 0 };
    timer_info_t xclk_info = { 0 };
    fsp_err_t xclk_info_err = FSP_ERR_NOT_OPEN;
    R_GPT0_Type *p_gpt = ra8p1_ceu_xclk_ctrl.p_reg;
    uint32_t gpt_gtcnt0 = (p_gpt != NULL) ? p_gpt->GTCNT : 0xffffffffU;
    R_BSP_SoftwareDelay(1, BSP_DELAY_UNITS_MICROSECONDS);
    uint32_t gpt_gtcnt1 = (p_gpt != NULL) ? p_gpt->GTCNT : 0xffffffffU;

    if (ra8p1_ceu_capture_open) {
        ra8p1_ceu_last_capture_status_err = ra8p1_ceu.p_api->statusGet(ra8p1_ceu.p_ctrl, &capture_status);
    } else {
        ra8p1_ceu_last_capture_status_err = FSP_ERR_NOT_OPEN;
    }
    if (ra8p1_ceu_xclk_open) {
        xclk_info_err = ra8p1_ceu_xclk.p_api->infoGet(ra8p1_ceu_xclk.p_ctrl, &xclk_info);
    }

    uint32_t sample_nonzero = 0;
    uint32_t sample_checksum = ra8p1_ceu_buffer_checksum(&sample_nonzero);

    RA8P1_CEU_DICT_STORE_INT(status, open, ra8p1_ceu_capture_open);
    RA8P1_CEU_DICT_STORE_INT(status, xclk_open, ra8p1_ceu_xclk_open);
    RA8P1_CEU_DICT_STORE_INT(status, capture_ready, ra8p1_ceu_capture_ready);
    RA8P1_CEU_DICT_STORE_INT(status, last_pin_err, ra8p1_ceu_last_pin_err);
    RA8P1_CEU_DICT_STORE_INT(status, last_gpt_open_err, ra8p1_ceu_last_gpt_open_err);
    RA8P1_CEU_DICT_STORE_INT(status, last_gpt_period_err, ra8p1_ceu_last_gpt_period_err);
    RA8P1_CEU_DICT_STORE_INT(status, last_gpt_start_err, ra8p1_ceu_last_gpt_start_err);
    RA8P1_CEU_DICT_STORE_INT(status, last_i2c_err, ra8p1_ceu_last_i2c_err);
    RA8P1_CEU_DICT_STORE_INT(status, last_switch_err, ra8p1_ceu_last_switch_err);
    RA8P1_CEU_DICT_STORE_INT(status, last_camera_err, ra8p1_ceu_last_camera_err);
    RA8P1_CEU_DICT_STORE_INT(status, last_ceu_open_err, ra8p1_ceu_last_ceu_open_err);
    RA8P1_CEU_DICT_STORE_INT(status, last_capture_start_err, ra8p1_ceu_last_capture_start_err);
    RA8P1_CEU_DICT_STORE_INT(status, last_capture_status_err, ra8p1_ceu_last_capture_status_err);
    RA8P1_CEU_DICT_STORE_INT(status, last_capture_frame_err, ra8p1_ceu_last_capture_frame_err);
    RA8P1_CEU_DICT_STORE_INT(status, last_display_open_err, ra8p1_ceu_last_display_open_err);
    RA8P1_CEU_DICT_STORE_INT(status, last_display_start_err, ra8p1_ceu_last_display_start_err);
    RA8P1_CEU_DICT_STORE_INT(status, last_display_stop_err, ra8p1_ceu_last_display_stop_err);
    RA8P1_CEU_DICT_STORE_INT(status, last_display_flip_err, ra8p1_ceu_last_display_flip_err);
    RA8P1_CEU_DICT_STORE_INT(status, last_display_backlight_err, ra8p1_ceu_last_display_backlight_err);
    RA8P1_CEU_DICT_STORE_INT(status, last_display_pin_restore_err, ra8p1_ceu_last_display_pin_restore_err);
    RA8P1_CEU_DICT_STORE_INT(status, last_tuning_err, ra8p1_ceu_last_tuning_err);
    RA8P1_CEU_DICT_STORE_UINT(status, callbacks, ra8p1_ceu_callbacks);
    RA8P1_CEU_DICT_STORE_UINT(status, last_event, ra8p1_ceu_last_event);
    RA8P1_CEU_DICT_STORE_UINT(status, last_event_status, ra8p1_ceu_last_event_status);
    RA8P1_CEU_DICT_STORE_UINT(status, last_interrupt_status, ra8p1_ceu_last_interrupt_status);
    RA8P1_CEU_DICT_STORE_UINT(status, last_i2c_addr, ra8p1_ceu_last_i2c_addr);
    RA8P1_CEU_DICT_STORE_UINT(status, last_i2c_reg, ra8p1_ceu_last_i2c_reg);
    RA8P1_CEU_DICT_STORE_UINT(status, camera_writes, ra8p1_ceu_camera_writes);
    RA8P1_CEU_DICT_STORE_UINT(status, capture_state, capture_status.state);
    RA8P1_CEU_DICT_STORE_UINT(status, capture_data_size, capture_status.data_size);
    RA8P1_CEU_DICT_STORE_UINT(status, capture_buffer, (uintptr_t)capture_status.p_buffer);
    RA8P1_CEU_DICT_STORE_UINT(status, buffer_addr, (uintptr_t)ra8p1_ceu_image_vga);
    RA8P1_CEU_DICT_STORE_UINT(status, buffer_size, RA8P1_CEU_BUFFER_SIZE);
    RA8P1_CEU_DICT_STORE_UINT(status, buffer_in_sram, MICROPY_RA8P1_CEU_CAPTURE_BUFFER_SRAM);
    RA8P1_CEU_DICT_STORE_UINT(status, display_width, DISPLAY_HSIZE_INPUT0);
    RA8P1_CEU_DICT_STORE_UINT(status, display_height, DISPLAY_VSIZE_INPUT0);
    RA8P1_CEU_DICT_STORE_UINT(status, display_stride, DISPLAY_BUFFER_STRIDE_BYTES_INPUT0);
    RA8P1_CEU_DICT_STORE_UINT(status, display_frames, ra8p1_ceu_display_frames);
    RA8P1_CEU_DICT_STORE_UINT(status, last_live_requested, ra8p1_ceu_last_live_requested);
    RA8P1_CEU_DICT_STORE_UINT(status, last_live_completed, ra8p1_ceu_last_live_completed);
    RA8P1_CEU_DICT_STORE_UINT(status, display_open, ra8p1_ceu_display_open);
    RA8P1_CEU_DICT_STORE_UINT(status, display_pins_restored, ra8p1_ceu_display_pins_restored);
    RA8P1_CEU_DICT_STORE_UINT(status, display_backlight_on, ra8p1_ceu_display_backlight_on);
    RA8P1_CEU_DICT_STORE_UINT(status, glcdc_state, g_display_ctrl.state);
    RA8P1_CEU_DICT_STORE_UINT(status, test_pattern, ra8p1_ceu_test_pattern_enabled);
    RA8P1_CEU_DICT_STORE_UINT(status, tuning_enabled, ra8p1_ceu_tuning_enabled);
    RA8P1_CEU_DICT_STORE_UINT(status, tuning_exposure, ra8p1_ceu_tuning_exposure);
    RA8P1_CEU_DICT_STORE_UINT(status, tuning_gain, ra8p1_ceu_tuning_gain);
    RA8P1_CEU_DICT_STORE_UINT(status, tuning_edge_ctrl, ra8p1_ceu_tuning_edge_ctrl);
    RA8P1_CEU_DICT_STORE_UINT(status, yuv_order, ra8p1_ceu_yuv_order);
    RA8P1_CEU_DICT_STORE_UINT(status, ceu_dsel, ra8p1_ceu_vga_extend.edge_info.dsel);
    RA8P1_CEU_DICT_STORE_UINT(status, ceu_hdsel, ra8p1_ceu_vga_extend.edge_info.hdsel);
    RA8P1_CEU_DICT_STORE_UINT(status, ceu_vdsel, ra8p1_ceu_vga_extend.edge_info.vdsel);
    RA8P1_CEU_DICT_STORE_UINT(status, ceu_hsync_polarity, ra8p1_ceu_vga_extend.hsync_polarity);
    RA8P1_CEU_DICT_STORE_UINT(status, ceu_vsync_polarity, ra8p1_ceu_vga_extend.vsync_polarity);
    RA8P1_CEU_DICT_STORE_UINT(status, sample_bytes, RA8P1_CEU_STATUS_SAMPLE_BYTES);
    RA8P1_CEU_DICT_STORE_UINT(status, sample_nonzero, sample_nonzero);
    RA8P1_CEU_DICT_STORE_UINT(status, sample_checksum, sample_checksum);
    RA8P1_CEU_DICT_STORE_UINT(status, system_core_clock, SystemCoreClock);
    RA8P1_CEU_DICT_STORE_UINT(status, clock_pclkd_hz, R_FSP_SystemClockHzGet(FSP_PRIV_CLOCK_PCLKD));
    RA8P1_CEU_DICT_STORE_UINT(status, clock_gptclk_hz, ra8p1_ceu_gptclk_hz());
    RA8P1_CEU_DICT_STORE_INT(status, xclk_info_err, xclk_info_err);
    RA8P1_CEU_DICT_STORE_UINT(status, xclk_target_hz, RA8P1_CEU_XCLK_TARGET_HZ);
    RA8P1_CEU_DICT_STORE_UINT(status, xclk_config_period_counts, ra8p1_ceu_xclk_period_counts());
    RA8P1_CEU_DICT_STORE_UINT(status, xclk_info_clock_hz, xclk_info.clock_frequency);
    RA8P1_CEU_DICT_STORE_UINT(status, xclk_info_period_counts, xclk_info.period_counts);
    RA8P1_CEU_DICT_STORE_UINT(status, xclk_actual_hz,
        (xclk_info.period_counts != 0U) ? (xclk_info.clock_frequency / xclk_info.period_counts) : 0U);
    RA8P1_CEU_DICT_STORE_UINT(status, gpt_gtcr, (p_gpt != NULL) ? p_gpt->GTCR : 0xffffffffU);
    RA8P1_CEU_DICT_STORE_UINT(status, gpt_gtior, (p_gpt != NULL) ? p_gpt->GTIOR : 0xffffffffU);
    RA8P1_CEU_DICT_STORE_UINT(status, gpt_gtcnt0, gpt_gtcnt0);
    RA8P1_CEU_DICT_STORE_UINT(status, gpt_gtcnt1, gpt_gtcnt1);
    RA8P1_CEU_DICT_STORE_UINT(status, gpt_gtccra, (p_gpt != NULL) ? p_gpt->GTCCR[0] : 0xffffffffU);
    RA8P1_CEU_DICT_STORE_UINT(status, gpt_gtpr, (p_gpt != NULL) ? p_gpt->GTPR : 0xffffffffU);
    RA8P1_CEU_DICT_STORE_UINT(status, ceu_capsr, R_CEU->CAPSR);
    RA8P1_CEU_DICT_STORE_UINT(status, ceu_cstsr, R_CEU->CSTSR);
    RA8P1_CEU_DICT_STORE_UINT(status, ceu_cetcr, R_CEU->CETCR);
    RA8P1_CEU_DICT_STORE_UINT(status, ceu_ceier, R_CEU->CEIER);
    RA8P1_CEU_DICT_STORE_UINT(status, ceu_cdssr, R_CEU->CDSSR);
    RA8P1_CEU_DICT_STORE_UINT(status, ceu_cdayr, R_CEU->CDAYR);
    RA8P1_CEU_DICT_STORE_UINT(status, ceu_camcr, R_CEU->CAMCR);
    RA8P1_CEU_DICT_STORE_UINT(status, p400_pfs, ra8p1_ceu_pin_pfs(BSP_IO_PORT_04_PIN_00));
    RA8P1_CEU_DICT_STORE_UINT(status, p902_pfs, ra8p1_ceu_pin_pfs(BSP_IO_PORT_09_PIN_02));
    RA8P1_CEU_DICT_STORE_UINT(status, p405_pfs, ra8p1_ceu_pin_pfs(BSP_IO_PORT_04_PIN_05));
    RA8P1_CEU_DICT_STORE_UINT(status, p406_pfs, ra8p1_ceu_pin_pfs(BSP_IO_PORT_04_PIN_06));
    RA8P1_CEU_DICT_STORE_UINT(status, p700_pfs, ra8p1_ceu_pin_pfs(BSP_IO_PORT_07_PIN_00));
    RA8P1_CEU_DICT_STORE_UINT(status, p701_pfs, ra8p1_ceu_pin_pfs(BSP_IO_PORT_07_PIN_01));
    RA8P1_CEU_DICT_STORE_UINT(status, p702_pfs, ra8p1_ceu_pin_pfs(BSP_IO_PORT_07_PIN_02));
    RA8P1_CEU_DICT_STORE_UINT(status, p703_pfs, ra8p1_ceu_pin_pfs(BSP_IO_PORT_07_PIN_03));
    RA8P1_CEU_DICT_STORE_UINT(status, pb02_pfs, ra8p1_ceu_pin_pfs(BSP_IO_PORT_11_PIN_02));
    RA8P1_CEU_DICT_STORE_UINT(status, pb03_pfs, ra8p1_ceu_pin_pfs(BSP_IO_PORT_11_PIN_03));
    RA8P1_CEU_DICT_STORE_UINT(status, pb04_pfs, ra8p1_ceu_pin_pfs(BSP_IO_PORT_11_PIN_04));
    RA8P1_CEU_DICT_STORE_UINT(status, p501_pfs, ra8p1_ceu_pin_pfs(BSP_IO_PORT_05_PIN_01));
    RA8P1_CEU_DICT_STORE_UINT(status, p501_level, ra8p1_ceu_pin_read(BSP_IO_PORT_05_PIN_01));
    RA8P1_CEU_DICT_STORE_UINT(status, p709_pfs, ra8p1_ceu_pin_pfs(BSP_IO_PORT_07_PIN_09));
    RA8P1_CEU_DICT_STORE_UINT(status, p709_level, ra8p1_ceu_pin_read(BSP_IO_PORT_07_PIN_09));
    RA8P1_CEU_DICT_STORE_UINT(status, i2c_scl, ra8p1_ceu_i2c_scl_read());
    RA8P1_CEU_DICT_STORE_UINT(status, i2c_sda, ra8p1_ceu_i2c_sda_read());
    return status;
}

static mp_obj_t ra8p1_ceu_init(void) {
    fsp_err_t err = ra8p1_ceu_init_do();
    ra8p1_ceu_raise_fsp("ceu init", err);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_0(ra8p1_ceu_init_obj, ra8p1_ceu_init);

static mp_obj_t ra8p1_ceu_deinit(void) {
    ra8p1_ceu_capture_close_do();
    ra8p1_ceu_xclk_close_do();
    ra8p1_ceu_display_shared_pins_restore();
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_0(ra8p1_ceu_deinit_obj, ra8p1_ceu_deinit);

static mp_obj_t ra8p1_ceu_restore_display_pins(void) {
    ra8p1_ceu_capture_close_do();
    ra8p1_ceu_xclk_close_do();
    ra8p1_ceu_display_shared_pins_restore();
    return ra8p1_ceu_status_dict();
}
static MP_DEFINE_CONST_FUN_OBJ_0(ra8p1_ceu_restore_display_pins_obj, ra8p1_ceu_restore_display_pins);

static mp_obj_t ra8p1_ceu_status(void) {
    return ra8p1_ceu_status_dict();
}
static MP_DEFINE_CONST_FUN_OBJ_0(ra8p1_ceu_status_obj, ra8p1_ceu_status);

static mp_obj_t ra8p1_ceu_sample_pins(size_t n_args, const mp_obj_t *args) {
    mp_int_t samples_in = (n_args == 0) ? 10000 : mp_obj_get_int(args[0]);
    if (samples_in < 2) {
        samples_in = 2;
    }
    if ((uint32_t)samples_in > RA8P1_CEU_MAX_PIN_SAMPLES) {
        mp_raise_ValueError(MP_ERROR_TEXT("samples too large"));
    }
    uint32_t samples = (uint32_t)samples_in;
    mp_obj_t result = mp_obj_new_dict(16);
    RA8P1_CEU_DICT_STORE_UINT(result, samples, samples);
    ra8p1_ceu_dict_store_pin_sample(result, MP_QSTR_pclk, BSP_IO_PORT_11_PIN_04, samples);
    ra8p1_ceu_dict_store_pin_sample(result, MP_QSTR_hsync, BSP_IO_PORT_11_PIN_03, samples);
    ra8p1_ceu_dict_store_pin_sample(result, MP_QSTR_vsync, BSP_IO_PORT_11_PIN_02, samples);
    ra8p1_ceu_dict_store_pin_sample(result, MP_QSTR_d0, BSP_IO_PORT_04_PIN_00, samples);
    ra8p1_ceu_dict_store_pin_sample(result, MP_QSTR_d1, BSP_IO_PORT_09_PIN_02, samples);
    ra8p1_ceu_dict_store_pin_sample(result, MP_QSTR_d2, BSP_IO_PORT_04_PIN_05, samples);
    ra8p1_ceu_dict_store_pin_sample(result, MP_QSTR_d3, BSP_IO_PORT_04_PIN_06, samples);
    ra8p1_ceu_dict_store_pin_sample(result, MP_QSTR_d4, BSP_IO_PORT_07_PIN_00, samples);
    ra8p1_ceu_dict_store_pin_sample(result, MP_QSTR_d5, BSP_IO_PORT_07_PIN_01, samples);
    ra8p1_ceu_dict_store_pin_sample(result, MP_QSTR_d6, BSP_IO_PORT_07_PIN_02, samples);
    ra8p1_ceu_dict_store_pin_sample(result, MP_QSTR_d7, BSP_IO_PORT_07_PIN_03, samples);
    return result;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(ra8p1_ceu_sample_pins_obj, 0, 1, ra8p1_ceu_sample_pins);

static mp_obj_t ra8p1_ceu_i2c_probe(size_t n_args, const mp_obj_t *args) {
    mp_int_t addr_in = (n_args == 0) ? RA8P1_CEU_CAMERA_ADDR : mp_obj_get_int(args[0]);
    if ((addr_in < 0) || (addr_in > 0x7f)) {
        mp_raise_ValueError(MP_ERROR_TEXT("addr must be 0..127"));
    }
    uint8_t addr = (uint8_t)addr_in;
    return mp_obj_new_bool(ra8p1_ceu_i2c_probe_addr(addr) == FSP_SUCCESS);
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(ra8p1_ceu_i2c_probe_obj, 0, 1, ra8p1_ceu_i2c_probe);

static mp_obj_t ra8p1_ceu_switch_read(mp_obj_t reg_in) {
    mp_int_t reg = mp_obj_get_int(reg_in);
    if ((reg < 0) || (reg > 0xff)) {
        mp_raise_ValueError(MP_ERROR_TEXT("reg must be 0..255"));
    }

    uint8_t value = 0;
    fsp_err_t err = ra8p1_ceu_i2c_read_reg8(RA8P1_CEU_SWITCH_ADDR, (uint8_t)reg, &value);
    if (err != FSP_SUCCESS) {
        return mp_obj_new_int(-((int)err));
    }
    return mp_obj_new_int_from_uint(value);
}
static MP_DEFINE_CONST_FUN_OBJ_1(ra8p1_ceu_switch_read_obj, ra8p1_ceu_switch_read);

static mp_obj_t ra8p1_ceu_switch_write(mp_obj_t reg_in, mp_obj_t value_in) {
    mp_int_t reg = mp_obj_get_int(reg_in);
    mp_int_t value = mp_obj_get_int(value_in);
    if ((reg < 0) || (reg > 0xff)) {
        mp_raise_ValueError(MP_ERROR_TEXT("reg must be 0..255"));
    }
    if ((value < 0) || (value > 0xff)) {
        mp_raise_ValueError(MP_ERROR_TEXT("value must be 0..255"));
    }

    fsp_err_t err = ra8p1_ceu_i2c_write_reg8(RA8P1_CEU_SWITCH_ADDR, (uint8_t)reg, (uint8_t)value);
    return mp_obj_new_int(err);
}
static MP_DEFINE_CONST_FUN_OBJ_2(ra8p1_ceu_switch_write_obj, ra8p1_ceu_switch_write);

static mp_obj_t ra8p1_ceu_switch_state(void) {
    uint8_t saved_dir = 0;
    uint8_t saved_output = 0;
    uint8_t saved_enable = 0;
    uint8_t input = 0;
    fsp_err_t restore_err = FSP_SUCCESS;
    fsp_err_t err = ra8p1_ceu_i2c_read_reg8(RA8P1_CEU_SWITCH_ADDR, RA8P1_CEU_SWITCH_PIN_DIR_REG, &saved_dir);
    bool have_dir = err == FSP_SUCCESS;
    bool have_output = false;
    bool have_enable = false;
    bool restore_needed = false;
    if (err == FSP_SUCCESS) {
        err = ra8p1_ceu_i2c_read_reg8(RA8P1_CEU_SWITCH_ADDR, RA8P1_CEU_SWITCH_OUTPUT_STATE_REG, &saved_output);
        have_output = err == FSP_SUCCESS;
    }
    if (err == FSP_SUCCESS) {
        err = ra8p1_ceu_i2c_read_reg8(RA8P1_CEU_SWITCH_ADDR, RA8P1_CEU_SWITCH_OUTPUT_ENABLE_REG, &saved_enable);
        have_enable = err == FSP_SUCCESS;
    }
    if (err == FSP_SUCCESS) {
        err = ra8p1_ceu_i2c_write_reg8(RA8P1_CEU_SWITCH_ADDR, RA8P1_CEU_SWITCH_PIN_DIR_REG, 0x00);
        restore_needed = err == FSP_SUCCESS;
    }
    if (err == FSP_SUCCESS) {
        err = ra8p1_ceu_i2c_read_reg8(RA8P1_CEU_SWITCH_ADDR, RA8P1_CEU_SWITCH_INPUT_REG, &input);
    }
    if (restore_needed || have_dir || have_output || have_enable) {
        if (have_output) {
            restore_err = ra8p1_ceu_i2c_write_reg8(
                RA8P1_CEU_SWITCH_ADDR, RA8P1_CEU_SWITCH_OUTPUT_STATE_REG, saved_output);
        }
        if (have_enable) {
            fsp_err_t restore_next = ra8p1_ceu_i2c_write_reg8(
                RA8P1_CEU_SWITCH_ADDR, RA8P1_CEU_SWITCH_OUTPUT_ENABLE_REG, saved_enable);
            if ((restore_err == FSP_SUCCESS) && (restore_next != FSP_SUCCESS)) {
                restore_err = restore_next;
            }
        }
        if (have_dir) {
            fsp_err_t restore_next = ra8p1_ceu_i2c_write_reg8(
                RA8P1_CEU_SWITCH_ADDR, RA8P1_CEU_SWITCH_PIN_DIR_REG, saved_dir);
            if ((restore_err == FSP_SUCCESS) && (restore_next != FSP_SUCCESS)) {
                restore_err = restore_next;
            }
        }
        if ((err == FSP_SUCCESS) && (restore_err != FSP_SUCCESS)) {
            err = restore_err;
        }
    }
    ra8p1_ceu_last_i2c_err = err;

    mp_obj_t dict = mp_obj_new_dict(8);
    RA8P1_CEU_DICT_STORE_INT(dict, err, err);
    RA8P1_CEU_DICT_STORE_UINT(dict, input, input);
    RA8P1_CEU_DICT_STORE_UINT(dict, direction, saved_dir);
    RA8P1_CEU_DICT_STORE_UINT(dict, output_state, saved_output);
    RA8P1_CEU_DICT_STORE_UINT(dict, output_enable, saved_enable);
    RA8P1_CEU_DICT_STORE_UINT(dict, sw4_6_bit, (input >> RA8P1_CEU_SWITCH_SW4_6_BIT) & 1U);
    RA8P1_CEU_DICT_STORE_INT(dict, restore_err, restore_err);
    return dict;
}
static MP_DEFINE_CONST_FUN_OBJ_0(ra8p1_ceu_switch_state_obj, ra8p1_ceu_switch_state);

static mp_obj_t ra8p1_ceu_camera_read(mp_obj_t reg_in) {
    uint8_t value = 0;
    mp_int_t reg_arg = mp_obj_get_int(reg_in);
    if ((reg_arg < 0) || (reg_arg > 0xffff)) {
        mp_raise_ValueError(MP_ERROR_TEXT("reg must be 0..65535"));
    }
    uint16_t reg = (uint16_t)reg_arg;
    fsp_err_t err = ra8p1_ceu_camera_access_prepare();
    if (err == FSP_SUCCESS) {
        err = ra8p1_ceu_i2c_read_reg16(RA8P1_CEU_CAMERA_ADDR, reg, &value);
    }
    ra8p1_ceu_raise_fsp("camera read", err);
    return mp_obj_new_int_from_uint(value);
}
static MP_DEFINE_CONST_FUN_OBJ_1(ra8p1_ceu_camera_read_obj, ra8p1_ceu_camera_read);

static mp_obj_t ra8p1_ceu_camera_write(mp_obj_t reg_in, mp_obj_t value_in) {
    mp_int_t reg_arg = mp_obj_get_int(reg_in);
    mp_int_t value_arg = mp_obj_get_int(value_in);
    if ((reg_arg < 0) || (reg_arg > 0xffff)) {
        mp_raise_ValueError(MP_ERROR_TEXT("reg must be 0..65535"));
    }
    if ((value_arg < 0) || (value_arg > 0xff)) {
        mp_raise_ValueError(MP_ERROR_TEXT("value must be 0..255"));
    }
    uint16_t reg = (uint16_t)reg_arg;
    uint8_t value = (uint8_t)value_arg;
    fsp_err_t err = ra8p1_ceu_camera_access_prepare();
    if (err == FSP_SUCCESS) {
        err = ra8p1_ceu_i2c_write_reg16(RA8P1_CEU_CAMERA_ADDR, reg, value);
    }
    ra8p1_ceu_raise_fsp("camera write", err);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(ra8p1_ceu_camera_write_obj, ra8p1_ceu_camera_write);

static mp_obj_t ra8p1_ceu_camera_id(void) {
    uint8_t idh = 0;
    uint8_t idl = 0;
    fsp_err_t err = ra8p1_ceu_camera_access_prepare();
    if (err == FSP_SUCCESS) {
        err = ra8p1_ceu_i2c_read_reg16(RA8P1_CEU_CAMERA_ADDR, RA8P1_CEU_REG_PIDH, &idh);
    }
    if (err == FSP_SUCCESS) {
        err = ra8p1_ceu_i2c_read_reg16(RA8P1_CEU_CAMERA_ADDR, RA8P1_CEU_REG_PIDL, &idl);
    }
    ra8p1_ceu_raise_fsp("camera id", err);
    mp_obj_t tuple[2] = { mp_obj_new_int_from_uint(idh), mp_obj_new_int_from_uint(idl) };
    return mp_obj_new_tuple(2, tuple);
}
static MP_DEFINE_CONST_FUN_OBJ_0(ra8p1_ceu_camera_id_obj, ra8p1_ceu_camera_id);

static mp_obj_t ra8p1_ceu_camera_open(void) {
    ra8p1_ceu_pins_configure();
    fsp_err_t err = ra8p1_ceu_switch_select_parallel_camera();
    if (err == FSP_SUCCESS) {
        err = ra8p1_ceu_camera_open_do();
    }
    ra8p1_ceu_raise_fsp("camera open", err);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_0(ra8p1_ceu_camera_open_obj, ra8p1_ceu_camera_open);

static mp_obj_t ra8p1_ceu_test_pattern(mp_obj_t enable_in) {
    bool enable = mp_obj_is_true(enable_in);
    ra8p1_ceu_test_pattern_enabled = enable;
    fsp_err_t err = ra8p1_ceu_camera_access_prepare();
    if (err == FSP_SUCCESS) {
        err = ra8p1_ceu_i2c_write_reg16(RA8P1_CEU_CAMERA_ADDR, 0x503d, enable ? 0x80 : 0x00);
    }
    ra8p1_ceu_raise_fsp("test pattern", err);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(ra8p1_ceu_test_pattern_obj, ra8p1_ceu_test_pattern);

static mp_obj_t ra8p1_ceu_capture(size_t n_args, const mp_obj_t *args) {
    mp_int_t timeout_arg = (n_args == 0) ? RA8P1_CEU_CAPTURE_TIMEOUT_MS : mp_obj_get_int(args[0]);
    if ((timeout_arg < 0) || ((uint32_t)timeout_arg > RA8P1_CEU_MAX_CAPTURE_TIMEOUT_MS)) {
        mp_raise_ValueError(MP_ERROR_TEXT("timeout must be 0..60000 ms"));
    }
    (void)ra8p1_ceu_capture_frame((uint32_t)timeout_arg, true);
    return ra8p1_ceu_status_dict();
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(ra8p1_ceu_capture_obj, 0, 1, ra8p1_ceu_capture);

static mp_obj_t ra8p1_ceu_capture_reuse(size_t n_args, const mp_obj_t *args) {
    mp_int_t timeout_arg = (n_args == 0) ? RA8P1_CEU_CAPTURE_TIMEOUT_MS : mp_obj_get_int(args[0]);
    if ((timeout_arg < 0) || ((uint32_t)timeout_arg > RA8P1_CEU_MAX_CAPTURE_TIMEOUT_MS)) {
        mp_raise_ValueError(MP_ERROR_TEXT("timeout must be 0..60000 ms"));
    }
    if (!ra8p1_ceu_capture_open) {
        mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("ceu not open; call init first"));
    }
    (void)ra8p1_ceu_capture_frame((uint32_t)timeout_arg, false);
    return ra8p1_ceu_status_dict();
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(ra8p1_ceu_capture_reuse_obj, 0, 1, ra8p1_ceu_capture_reuse);

static mp_obj_t ra8p1_ceu_display_frame(size_t n_args, const mp_obj_t *args) {
    mp_int_t idx = (n_args > 0) ? mp_obj_get_int(args[0]) : 0;
    mp_int_t x = (DISPLAY_HSIZE_INPUT0 > RA8P1_CEU_WIDTH) ?
        (mp_int_t)((DISPLAY_HSIZE_INPUT0 - RA8P1_CEU_WIDTH) / 2U) : 0;
    mp_int_t y = (DISPLAY_VSIZE_INPUT0 > RA8P1_CEU_HEIGHT) ?
        (mp_int_t)((DISPLAY_VSIZE_INPUT0 - RA8P1_CEU_HEIGHT) / 2U) : 0;
    if (n_args > 1) {
        x = mp_obj_get_int(args[1]);
    }
    if (n_args > 2) {
        y = mp_obj_get_int(args[2]);
    }

    fsp_err_t err = ra8p1_ceu_render_to_display(idx, x, y, true);
    ra8p1_ceu_raise_fsp("display frame", err);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(ra8p1_ceu_display_frame_obj, 0, 3, ra8p1_ceu_display_frame);

static mp_obj_t ra8p1_ceu_snapshot_display(size_t n_args, const mp_obj_t *args) {
    mp_int_t timeout_arg = (n_args > 0) ? mp_obj_get_int(args[0]) : RA8P1_CEU_CAPTURE_TIMEOUT_MS;
    if ((timeout_arg < 0) || ((uint32_t)timeout_arg > RA8P1_CEU_MAX_CAPTURE_TIMEOUT_MS)) {
        mp_raise_ValueError(MP_ERROR_TEXT("timeout must be 0..60000 ms"));
    }

    mp_int_t x = (DISPLAY_HSIZE_INPUT0 > RA8P1_CEU_WIDTH) ?
        (mp_int_t)((DISPLAY_HSIZE_INPUT0 - RA8P1_CEU_WIDTH) / 2U) : 0;
    mp_int_t y = (DISPLAY_VSIZE_INPUT0 > RA8P1_CEU_HEIGHT) ?
        (mp_int_t)((DISPLAY_VSIZE_INPUT0 - RA8P1_CEU_HEIGHT) / 2U) : 0;

    ra8p1_ceu_last_live_requested = 1;
    ra8p1_ceu_last_live_completed = 0;
    fsp_err_t err = ra8p1_ceu_display_backlight_set(false);
    if (err == FSP_SUCCESS) {
        err = ra8p1_ceu_display_stop_if_active();
    }
    if (err == FSP_SUCCESS) {
        err = ra8p1_ceu_capture_frame((uint32_t)timeout_arg, true);
    }
    ra8p1_ceu_capture_close_do();
    ra8p1_ceu_xclk_close_do();
    ra8p1_ceu_display_shared_pins_restore();
    if (err == FSP_SUCCESS) {
        err = ra8p1_ceu_render_to_display(0, x, y, true);
    }
    (void)ra8p1_ceu_display_backlight_set(true);
    if (err == FSP_SUCCESS) {
        ra8p1_ceu_last_live_completed = 1;
    } else {
        ra8p1_ceu_last_capture_frame_err = err;
    }
    return ra8p1_ceu_status_dict();
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(ra8p1_ceu_snapshot_display_obj, 0, 1, ra8p1_ceu_snapshot_display);

static mp_obj_t ra8p1_ceu_live(size_t n_args, const mp_obj_t *args) {
    mp_int_t frames_arg = (n_args > 0) ? mp_obj_get_int(args[0]) : 30;
    mp_int_t timeout_arg = (n_args > 1) ? mp_obj_get_int(args[1]) : RA8P1_CEU_CAPTURE_TIMEOUT_MS;
    if ((frames_arg <= 0) || ((uint32_t)frames_arg > RA8P1_CEU_MAX_LIVE_FRAMES)) {
        mp_raise_ValueError(MP_ERROR_TEXT("frames must be 1..10000"));
    }
    if ((timeout_arg < 0) || ((uint32_t)timeout_arg > RA8P1_CEU_MAX_CAPTURE_TIMEOUT_MS)) {
        mp_raise_ValueError(MP_ERROR_TEXT("timeout must be 0..60000 ms"));
    }

    mp_int_t x = (DISPLAY_HSIZE_INPUT0 > RA8P1_CEU_WIDTH) ?
        (mp_int_t)((DISPLAY_HSIZE_INPUT0 - RA8P1_CEU_WIDTH) / 2U) : 0;
    mp_int_t y = (DISPLAY_VSIZE_INPUT0 > RA8P1_CEU_HEIGHT) ?
        (mp_int_t)((DISPLAY_VSIZE_INPUT0 - RA8P1_CEU_HEIGHT) / 2U) : 0;
    uint32_t frames = (uint32_t)frames_arg;
    uint32_t timeout_ms = (uint32_t)timeout_arg;
    ra8p1_ceu_last_live_requested = frames;
    ra8p1_ceu_last_live_completed = 0;

    fsp_err_t err = ra8p1_ceu_display_prepare();
    if (err == FSP_SUCCESS) {
        ra8p1_ceu_display_fill(ra8p1_ceu_display_buf(0), 0x00000000U);
        for (uint32_t i = 0; i < frames; i++) {
            err = ra8p1_ceu_display_backlight_set(false);
            if (err != FSP_SUCCESS) {
                break;
            }
            err = ra8p1_ceu_display_stop_if_active();
            if (err != FSP_SUCCESS) {
                break;
            }
            err = ra8p1_ceu_capture_frame(timeout_ms, true);
            ra8p1_ceu_capture_close_do();
            ra8p1_ceu_xclk_close_do();
            ra8p1_ceu_display_shared_pins_restore();
            if (err != FSP_SUCCESS) {
                break;
            }
            err = ra8p1_ceu_render_to_display(0, x, y, false);
            (void)ra8p1_ceu_display_backlight_set(true);
            if (err != FSP_SUCCESS) {
                break;
            }
            ra8p1_ceu_last_live_completed++;
            mp_handle_pending(MP_HANDLE_PENDING_CALLBACKS_AND_EXCEPTIONS);
        }
    } else {
        ra8p1_ceu_last_capture_frame_err = err;
    }
    (void)ra8p1_ceu_display_backlight_set(true);
    return ra8p1_ceu_status_dict();
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(ra8p1_ceu_live_obj, 0, 2, ra8p1_ceu_live);

static mp_obj_t ra8p1_ceu_yuv_order_fn(size_t n_args, const mp_obj_t *args) {
    if (n_args > 0) {
        mp_int_t order = mp_obj_get_int(args[0]);
        if ((order < 0) || ((uint32_t)order > RA8P1_CEU_YUV_ORDER_MAX)) {
            mp_raise_ValueError(MP_ERROR_TEXT("order must be 0..3"));
        }
        ra8p1_ceu_yuv_order = (uint32_t)order;
    }
    return mp_obj_new_int_from_uint(ra8p1_ceu_yuv_order);
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(ra8p1_ceu_yuv_order_obj, 0, 1, ra8p1_ceu_yuv_order_fn);

static mp_obj_t ra8p1_ceu_timing(size_t n_args, const mp_obj_t *args) {
    if (n_args > 0) {
        if (n_args != 5) {
            mp_raise_TypeError(MP_ERROR_TEXT("timing expects dsel, hdsel, vdsel, hsync_polarity, vsync_polarity"));
        }
        mp_int_t dsel = mp_obj_get_int(args[0]);
        mp_int_t hdsel = mp_obj_get_int(args[1]);
        mp_int_t vdsel = mp_obj_get_int(args[2]);
        mp_int_t hpol = mp_obj_get_int(args[3]);
        mp_int_t vpol = mp_obj_get_int(args[4]);
        if ((dsel < 0) || (dsel > 1) || (hdsel < 0) || (hdsel > 1) ||
            (vdsel < 0) || (vdsel > 1) || (hpol < 0) || (hpol > 1) || (vpol < 0) || (vpol > 1)) {
            mp_raise_ValueError(MP_ERROR_TEXT("timing values must be 0 or 1"));
        }
        ra8p1_ceu_capture_close_do();
        ra8p1_ceu_vga_extend.edge_info.dsel = (uint8_t)dsel;
        ra8p1_ceu_vga_extend.edge_info.hdsel = (uint8_t)hdsel;
        ra8p1_ceu_vga_extend.edge_info.vdsel = (uint8_t)vdsel;
        ra8p1_ceu_vga_extend.hsync_polarity = (ceu_hsync_polarity_t)hpol;
        ra8p1_ceu_vga_extend.vsync_polarity = (ceu_vsync_polarity_t)vpol;
    }
    mp_obj_t tuple[5] = {
        mp_obj_new_int_from_uint(ra8p1_ceu_vga_extend.edge_info.dsel),
        mp_obj_new_int_from_uint(ra8p1_ceu_vga_extend.edge_info.hdsel),
        mp_obj_new_int_from_uint(ra8p1_ceu_vga_extend.edge_info.vdsel),
        mp_obj_new_int_from_uint(ra8p1_ceu_vga_extend.hsync_polarity),
        mp_obj_new_int_from_uint(ra8p1_ceu_vga_extend.vsync_polarity),
    };
    return mp_obj_new_tuple(5, tuple);
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(ra8p1_ceu_timing_obj, 0, 5, ra8p1_ceu_timing);

static mp_obj_t ra8p1_ceu_tuning(size_t n_args, const mp_obj_t *args) {
    if (n_args > 0) {
        mp_int_t enabled = mp_obj_get_int(args[0]);
        if ((enabled < 0) || (enabled > 1)) {
            mp_raise_ValueError(MP_ERROR_TEXT("enabled must be 0 or 1"));
        }
        uint32_t exposure = (n_args > 1) ? (uint32_t)mp_obj_get_int(args[1]) : RA8P1_CEU_TUNING_DEFAULT_EXPOSURE;
        uint32_t gain = (n_args > 2) ? (uint32_t)mp_obj_get_int(args[2]) : RA8P1_CEU_TUNING_DEFAULT_GAIN;
        uint32_t edge_ctrl = (n_args > 3) ? (uint32_t)mp_obj_get_int(args[3]) : RA8P1_CEU_TUNING_DEFAULT_EDGE_CTRL;
        if (exposure > RA8P1_CEU_MAX_OV5640_EXPOSURE) {
            mp_raise_ValueError(MP_ERROR_TEXT("exposure must be 0..0xfffff"));
        }
        if (gain > RA8P1_CEU_MAX_OV5640_GAIN) {
            mp_raise_ValueError(MP_ERROR_TEXT("gain must be 0..0x3ff"));
        }
        if (edge_ctrl > RA8P1_CEU_MAX_OV5640_REG8) {
            mp_raise_ValueError(MP_ERROR_TEXT("edge_ctrl must be 0..0xff"));
        }
        ra8p1_ceu_tuning_enabled = (enabled != 0);
        ra8p1_ceu_tuning_exposure = exposure;
        ra8p1_ceu_tuning_gain = gain;
        ra8p1_ceu_tuning_edge_ctrl = edge_ctrl;
    }
    mp_obj_t tuple[4] = {
        mp_obj_new_int(ra8p1_ceu_tuning_enabled ? 1 : 0),
        mp_obj_new_int_from_uint(ra8p1_ceu_tuning_exposure),
        mp_obj_new_int_from_uint(ra8p1_ceu_tuning_gain),
        mp_obj_new_int_from_uint(ra8p1_ceu_tuning_edge_ctrl),
    };
    return mp_obj_new_tuple(4, tuple);
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(ra8p1_ceu_tuning_obj, 0, 4, ra8p1_ceu_tuning);

static mp_obj_t ra8p1_ceu_buffer(void) {
    return mp_obj_new_memoryview(0x80 | 'B', RA8P1_CEU_BUFFER_SIZE, ra8p1_ceu_image_vga);
}
static MP_DEFINE_CONST_FUN_OBJ_0(ra8p1_ceu_buffer_obj, ra8p1_ceu_buffer);

static const mp_rom_map_elem_t ra8p1_ceu_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_ra8p1_ceu) },
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&ra8p1_ceu_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&ra8p1_ceu_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR_restore_display_pins), MP_ROM_PTR(&ra8p1_ceu_restore_display_pins_obj) },
    { MP_ROM_QSTR(MP_QSTR_status), MP_ROM_PTR(&ra8p1_ceu_status_obj) },
    { MP_ROM_QSTR(MP_QSTR_sample_pins), MP_ROM_PTR(&ra8p1_ceu_sample_pins_obj) },
    { MP_ROM_QSTR(MP_QSTR_capture), MP_ROM_PTR(&ra8p1_ceu_capture_obj) },
    { MP_ROM_QSTR(MP_QSTR_capture_reuse), MP_ROM_PTR(&ra8p1_ceu_capture_reuse_obj) },
    { MP_ROM_QSTR(MP_QSTR_display_frame), MP_ROM_PTR(&ra8p1_ceu_display_frame_obj) },
    { MP_ROM_QSTR(MP_QSTR_snapshot_display), MP_ROM_PTR(&ra8p1_ceu_snapshot_display_obj) },
    { MP_ROM_QSTR(MP_QSTR_live), MP_ROM_PTR(&ra8p1_ceu_live_obj) },
    { MP_ROM_QSTR(MP_QSTR_yuv_order), MP_ROM_PTR(&ra8p1_ceu_yuv_order_obj) },
    { MP_ROM_QSTR(MP_QSTR_timing), MP_ROM_PTR(&ra8p1_ceu_timing_obj) },
    { MP_ROM_QSTR(MP_QSTR_tuning), MP_ROM_PTR(&ra8p1_ceu_tuning_obj) },
    { MP_ROM_QSTR(MP_QSTR_buffer), MP_ROM_PTR(&ra8p1_ceu_buffer_obj) },
    { MP_ROM_QSTR(MP_QSTR_i2c_probe), MP_ROM_PTR(&ra8p1_ceu_i2c_probe_obj) },
    { MP_ROM_QSTR(MP_QSTR_switch_read), MP_ROM_PTR(&ra8p1_ceu_switch_read_obj) },
    { MP_ROM_QSTR(MP_QSTR_switch_write), MP_ROM_PTR(&ra8p1_ceu_switch_write_obj) },
    { MP_ROM_QSTR(MP_QSTR_switch_state), MP_ROM_PTR(&ra8p1_ceu_switch_state_obj) },
    { MP_ROM_QSTR(MP_QSTR_camera_read), MP_ROM_PTR(&ra8p1_ceu_camera_read_obj) },
    { MP_ROM_QSTR(MP_QSTR_camera_write), MP_ROM_PTR(&ra8p1_ceu_camera_write_obj) },
    { MP_ROM_QSTR(MP_QSTR_camera_id), MP_ROM_PTR(&ra8p1_ceu_camera_id_obj) },
    { MP_ROM_QSTR(MP_QSTR_camera_open), MP_ROM_PTR(&ra8p1_ceu_camera_open_obj) },
    { MP_ROM_QSTR(MP_QSTR_test_pattern), MP_ROM_PTR(&ra8p1_ceu_test_pattern_obj) },
    { MP_ROM_QSTR(MP_QSTR_WIDTH), MP_ROM_INT(RA8P1_CEU_WIDTH) },
    { MP_ROM_QSTR(MP_QSTR_HEIGHT), MP_ROM_INT(RA8P1_CEU_HEIGHT) },
    { MP_ROM_QSTR(MP_QSTR_BPP), MP_ROM_INT(RA8P1_CEU_BPP * 8) },
    { MP_ROM_QSTR(MP_QSTR_FORMAT), MP_ROM_QSTR(MP_QSTR_YUV422) },
    { MP_ROM_QSTR(MP_QSTR_BUFFER_SIZE), MP_ROM_INT(RA8P1_CEU_BUFFER_SIZE) },
    { MP_ROM_QSTR(MP_QSTR_DISPLAY_WIDTH), MP_ROM_INT(DISPLAY_HSIZE_INPUT0) },
    { MP_ROM_QSTR(MP_QSTR_DISPLAY_HEIGHT), MP_ROM_INT(DISPLAY_VSIZE_INPUT0) },
    { MP_ROM_QSTR(MP_QSTR_YUV_ORDER_CBYCRY), MP_ROM_INT(RA8P1_CEU_YUV_ORDER_CBYCRY) },
    { MP_ROM_QSTR(MP_QSTR_YUV_ORDER_CRYCBY), MP_ROM_INT(RA8P1_CEU_YUV_ORDER_CRYCBY) },
    { MP_ROM_QSTR(MP_QSTR_YUV_ORDER_YCBYCR), MP_ROM_INT(RA8P1_CEU_YUV_ORDER_YCBYCR) },
    { MP_ROM_QSTR(MP_QSTR_YUV_ORDER_YCRYCB), MP_ROM_INT(RA8P1_CEU_YUV_ORDER_YCRYCB) },
};
static MP_DEFINE_CONST_DICT(ra8p1_ceu_module_globals, ra8p1_ceu_module_globals_table);

const mp_obj_module_t mp_module_ra8p1_ceu = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&ra8p1_ceu_module_globals,
};

MP_REGISTER_MODULE(MP_QSTR_ra8p1_ceu, mp_module_ra8p1_ceu);

#endif
