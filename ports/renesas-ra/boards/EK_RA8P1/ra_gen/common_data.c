/* generated common source file - do not edit */
#include "common_data.h"
#if defined(MICROPY_RA8P1_BRINGUP_MIPI_DSI_TEST) && MICROPY_RA8P1_BRINGUP_MIPI_DSI_TEST
/* MIPI PHY Macros */
#define MIPI_PHY_CLKSTPT   ((uint16_t)1183)
#define MIPI_PHY_CLKBFHT   ((uint8_t)10 + 1)
#define MIPI_PHY_CLKKPT    ((uint8_t)22 + 4)
#define MIPI_PHY_GOLPBKT   ((uint16_t)40)

#define MIPI_PHY_TINIT     (74999)
#define MIPI_PHY_TCLKPREP  (9)
#define MIPI_PHY_TCLKSETT  (62)
#define MIPI_PHY_TCLKMISS  (37)
#define MIPI_PHY_THSPREP   (6)
#define MIPI_PHY_THSETT    (24)
#define MIPI_PHY_TCLKTRAIL (7)
#define MIPI_PHY_TCLKPOST  (20)
#define MIPI_PHY_TCLKPRE   (1)
#define MIPI_PHY_TCLKZERO  (28)
#define MIPI_PHY_THSEXIT   (12)
#define MIPI_PHY_THSTRAIL  (8)
#define MIPI_PHY_THSZERO   (19)
#define MIPI_PHY_TLPEXIT   (7)

/* MIPI PHY Structures */
const mipi_phy_timing_t g_mipi_phy0_timing =
{ .t_init = 0x3FFFF & (uint32_t)MIPI_PHY_TINIT,
  .dphytim2_b.t_clk_prep = (uint32_t)MIPI_PHY_TCLKPREP,
  .dphytim2_b.t_clk_settle = (uint32_t)MIPI_PHY_TCLKSETT,
  .dphytim2_b.t_clk_miss = (uint32_t)MIPI_PHY_TCLKMISS,
  .dphytim3_b.t_hs_prep = (uint32_t)MIPI_PHY_THSPREP,
  .dphytim3_b.t_hs_sett = (uint32_t)MIPI_PHY_THSETT,
  .dphytim4_b.t_clk_trail = (uint32_t)MIPI_PHY_TCLKTRAIL,
  .dphytim4_b.t_clk_post = (uint32_t)MIPI_PHY_TCLKPOST,
  .dphytim4_b.t_clk_pre = (uint32_t)MIPI_PHY_TCLKPRE,
  .dphytim4_b.t_clk_zero = (uint32_t)MIPI_PHY_TCLKZERO,
  .dphytim5_b.t_hs_exit = (uint32_t)MIPI_PHY_THSEXIT,
  .dphytim5_b.t_hs_trail = (uint32_t)MIPI_PHY_THSTRAIL,
  .dphytim5_b.t_hs_zero = (uint32_t)MIPI_PHY_THSZERO,
  .t_lp_exit = (uint32_t)MIPI_PHY_TLPEXIT, };

mipi_phy_ctrl_t g_mipi_phy0_ctrl;
const mipi_phy_cfg_t g_mipi_phy0_cfg =
{ .pll_settings = /* Calculated MIPI PHY PLL frequency: 1000000000 Hz (error 0.00%) = (24000000 Hz / 3) * 125.00 / 1 */
  { .div = 3 - 1, .pll_div = 0, .mul_int = 125 - 1, .mul_frac = 0 },
  .lp_divisor = 5 - 1, .p_timing = &g_mipi_phy0_timing, .dsi_mode = true, };

const mipi_phy_instance_t g_mipi_phy0 =
{ .p_ctrl = &g_mipi_phy0_ctrl, .p_cfg = &g_mipi_phy0_cfg, .p_api = &g_mipi_phy };

const mipi_dsi_timing_t g_mipi_dsi0_timing =
{ .clock_stop_time = (uint32_t)MIPI_PHY_CLKSTPT,
  .clock_beforehand_time = (uint32_t)MIPI_PHY_CLKBFHT,
  .clock_keep_time = (uint32_t)MIPI_PHY_CLKKPT,
  .go_lp_and_back = (uint32_t)MIPI_PHY_GOLPBKT, };

mipi_dsi_instance_ctrl_t g_mipi_dsi0_ctrl;

const mipi_dsi_extended_cfg_t g_mipi_dsi0_cfg_extend =
{ .dsi_seq0 = { .ipl = (12), .irq = VECTOR_NUMBER_MIPIDSI_SEQ0 },
  .dsi_seq1 = { .ipl = (12), .irq = VECTOR_NUMBER_MIPIDSI_SEQ1 },
  .dsi_ferr = { .ipl = (12), .irq = VECTOR_NUMBER_MIPIDSI_FERR },
  .dsi_ppi = { .ipl = (12), .irq = VECTOR_NUMBER_MIPIDSI_PPI },
  .dsi_rcv = { .ipl = (12), .irq = VECTOR_NUMBER_MIPIDSI_RCV },
  .dsi_vin1 = { .ipl = (12), .irq = VECTOR_NUMBER_MIPIDSI_VIN1 },
  .dsi_rxie = 0,
  .dsi_ferrie = 0,
  .dsi_plie = R_MIPI_DSI_PLIER_CLULPENT_Msk | R_MIPI_DSI_PLIER_CLULPEXT_Msk |
      R_MIPI_DSI_PLIER_DLULPENT_Msk | R_MIPI_DSI_PLIER_DLULPEXT_Msk,
  .dsi_vmie = R_MIPI_DSI_VMIER_VIRDY_Msk | R_MIPI_DSI_VMIER_VBUFUDF_Msk | R_MIPI_DSI_VMIER_VBUFOVF_Msk,
  .dsi_sqch0ie = 0,
  .dsi_sqch1ie = 0, };

const mipi_dsi_cfg_t g_mipi_dsi0_cfg =
{ .p_mipi_phy_instance = &g_mipi_phy0,
  .p_timing = &g_mipi_dsi0_timing,
  .hsa_no_lp = false,
  .hbp_no_lp = false,
  .hfp_no_lp = false,
  .num_lanes = 2,
  .ulps_wakeup_period = (uint8_t)1000,
  .continuous_clock = true,
  .hs_tx_timeout = 0,
  .lp_rx_timeout = 0,
  .turnaround_timeout = 0,
  .bta_timeout = 0,
  .lprw_timeout = 0,
  .hsrw_timeout = 0,
  .max_return_packet_size = 1,
  .ecc_enable = true,
  .crc_check_mask = MIPI_DSI_VC_NONE,
  .scramble_enable = false,
  .tearing_detect = true,
  .eotp_enable = true,
  .sync_pulse = false,
  .data_type = MIPI_DSI_VIDEO_DATA_24RGB_PIXEL_STREAM,
  .virtual_channel_id = 0,
  .vertical_sync_lines = 1,
  .vertical_sync_polarity = DISPLAY_SIGNAL_POLARITY_LOACTIVE,
  .vertical_active_lines = DISPLAY_VSIZE_INPUT0,
  .vertical_back_porch = 20,
  .vertical_front_porch = 14,
  .horizontal_sync_lines = 1,
  .horizontal_sync_polarity = DISPLAY_SIGNAL_POLARITY_LOACTIVE,
  .horizontal_active_lines = DISPLAY_HSIZE_INPUT0,
  .horizontal_back_porch = 140,
  .horizontal_front_porch = 179,
  .video_mode_delay = 0,
  .p_callback = mipi_dsi_callback,
  .p_context = NULL,
  .p_extend = &g_mipi_dsi0_cfg_extend, };

const mipi_dsi_instance_t g_mipi_dsi0 =
{ .p_ctrl = &g_mipi_dsi0_ctrl, .p_cfg = &g_mipi_dsi0_cfg, .p_api = &g_mipi_dsi };

void mipi_dsi_callback(mipi_dsi_callback_args_t * p_args) {
    (void)p_args;
}
#endif
#if defined(MICROPY_RA8P1_BRINGUP_MIPI_CSI_TEST) && MICROPY_RA8P1_BRINGUP_MIPI_CSI_TEST
/* MIPI PHY Macros */
#define MIPI_CSI_PHY_TINIT     (74999)
#define MIPI_CSI_PHY_TCLKPREP  (9)
#define MIPI_CSI_PHY_TCLKSETT  (62)
#define MIPI_CSI_PHY_TCLKMISS  (37)
#define MIPI_CSI_PHY_THSPREP   (6)
#define MIPI_CSI_PHY_THSETT    (24)
#define MIPI_CSI_PHY_TCLKTRAIL (7)
#define MIPI_CSI_PHY_TCLKPOST  (20)
#define MIPI_CSI_PHY_TCLKPRE   (1)
#define MIPI_CSI_PHY_TCLKZERO  (28)
#define MIPI_CSI_PHY_THSEXIT   (12)
#define MIPI_CSI_PHY_THSTRAIL  (8)
#define MIPI_CSI_PHY_THSZERO   (19)
#define MIPI_CSI_PHY_TLPEXIT   (7)
#define RA8P1_CSI_VIN_INPUT_WIDTH   (1024)
#define RA8P1_CSI_VIN_INPUT_HEIGHT  (600)
#define RA8P1_CSI_VIN_SCALE_MASK_H  (0x1000)
#define RA8P1_CSI_VIN_SCALE_MASK_V  (0x1000)

const mipi_phy_timing_t g_mipi_phy0_timing =
{ .t_init = 0x3FFFF & (uint32_t)MIPI_CSI_PHY_TINIT,
  .dphytim2_b.t_clk_prep = (uint32_t)MIPI_CSI_PHY_TCLKPREP,
  .dphytim2_b.t_clk_settle = (uint32_t)MIPI_CSI_PHY_TCLKSETT,
  .dphytim2_b.t_clk_miss = (uint32_t)MIPI_CSI_PHY_TCLKMISS,
  .dphytim3_b.t_hs_prep = (uint32_t)MIPI_CSI_PHY_THSPREP,
  .dphytim3_b.t_hs_sett = (uint32_t)MIPI_CSI_PHY_THSETT,
  .dphytim4_b.t_clk_trail = (uint32_t)MIPI_CSI_PHY_TCLKTRAIL,
  .dphytim4_b.t_clk_post = (uint32_t)MIPI_CSI_PHY_TCLKPOST,
  .dphytim4_b.t_clk_pre = (uint32_t)MIPI_CSI_PHY_TCLKPRE,
  .dphytim4_b.t_clk_zero = (uint32_t)MIPI_CSI_PHY_TCLKZERO,
  .dphytim5_b.t_hs_exit = (uint32_t)MIPI_CSI_PHY_THSEXIT,
  .dphytim5_b.t_hs_trail = (uint32_t)MIPI_CSI_PHY_THSTRAIL,
  .dphytim5_b.t_hs_zero = (uint32_t)MIPI_CSI_PHY_THSZERO,
  .t_lp_exit = (uint32_t)MIPI_CSI_PHY_TLPEXIT, };

mipi_phy_ctrl_t g_mipi_phy0_ctrl;
const mipi_phy_cfg_t g_mipi_phy0_cfg =
{ .pll_settings =
  { .div = 3 - 1, .pll_div = 0, .mul_int = 125 - 1, .mul_frac = 0 },
  .lp_divisor = 5 - 1, .p_timing = &g_mipi_phy0_timing, .dsi_mode = false, };

const mipi_phy_instance_t g_mipi_phy0 =
{ .p_ctrl = &g_mipi_phy0_ctrl, .p_cfg = &g_mipi_phy0_cfg, .p_api = &g_mipi_phy };

mipi_csi_instance_ctrl_t g_mipi_csi0_ctrl;
const mipi_csi_cfg_t g_mipi_csi0_cfg =
{ .p_mipi_phy_instance = &g_mipi_phy0,

  .ctrl_data.control_0_bits.lane_count = 2,
  .ctrl_data.control_0_bits.zero_length_packet_output = true,
  .ctrl_data.control_0_bits.err_frame_notify = 1,
  .ctrl_data.control_0_bits.reserved_packet_reception = 1,
  .ctrl_data.control_0_bits.generic_rule_mode = 1,
  .ctrl_data.control_0_bits.ecc_check_24_bits = 1,
  .ctrl_data.control_0_bits.descramble_enable = 0,

  .ctrl_data.control_2_bits.frrclk = 0,
  .ctrl_data.control_2_bits.frrskw = 0,

  .option_data.epd_option_0_bits.long_packet_spacers = 0,
  .option_data.data_type_enable = (mipi_csi_rx_data_enable_t) (MIPI_CSI_RX_DATA_ENABLE_YUV422_8_BIT | 0x0),

  .interrupt_cfg.receive_cfg.ipl = (12),
#if defined(VECTOR_NUMBER_MIPICSI_RX)
  .interrupt_cfg.receive_cfg.irq = VECTOR_NUMBER_MIPICSI_RX,
#else
  .interrupt_cfg.receive_cfg.irq = FSP_INVALID_VECTOR,
#endif

  .interrupt_cfg.data_lane_cfg.ipl = (12),
#if defined(VECTOR_NUMBER_MIPICSI_DL)
  .interrupt_cfg.data_lane_cfg.irq = VECTOR_NUMBER_MIPICSI_DL,
#else
  .interrupt_cfg.data_lane_cfg.irq = FSP_INVALID_VECTOR,
#endif

  .interrupt_cfg.virtual_channel_cfg.ipl = (12),
#if defined(VECTOR_NUMBER_MIPICSI_VC)
  .interrupt_cfg.virtual_channel_cfg.irq = VECTOR_NUMBER_MIPICSI_VC,
#else
  .interrupt_cfg.virtual_channel_cfg.irq = FSP_INVALID_VECTOR,
#endif

  .interrupt_cfg.power_management_cfg.ipl = (BSP_IRQ_DISABLED),
#if defined(VECTOR_NUMBER_MIPICSI_PM)
  .interrupt_cfg.power_management_cfg.irq = VECTOR_NUMBER_MIPICSI_PM,
#else
  .interrupt_cfg.power_management_cfg.irq = FSP_INVALID_VECTOR,
#endif
  .interrupt_cfg.short_packet_cfg.ipl = (BSP_IRQ_DISABLED),
#if defined(VECTOR_NUMBER_MIPICSI_GST)
  .interrupt_cfg.short_packet_cfg.irq = VECTOR_NUMBER_MIPICSI_GST,
#else
  .interrupt_cfg.short_packet_cfg.irq = FSP_INVALID_VECTOR,
#endif

  .interrupt_cfg.receive_enable_mask = R_MIPI_CSI_RXST_RACTDET_Msk,
  .interrupt_cfg.data_lane_enable_mask[0] = 0,
  .interrupt_cfg.data_lane_enable_mask[1] = 0,
  .interrupt_cfg.virtual_channel_enable_mask[0] = R_MIPI_CSI_VCST0_ETR_Msk,
  .interrupt_cfg.virtual_channel_enable_mask[1] = 0,
  .interrupt_cfg.virtual_channel_enable_mask[2] = 0,
  .interrupt_cfg.virtual_channel_enable_mask[3] = 0,
  .interrupt_cfg.virtual_channel_enable_mask[4] = 0,
  .interrupt_cfg.virtual_channel_enable_mask[5] = 0,
  .interrupt_cfg.virtual_channel_enable_mask[6] = 0,
  .interrupt_cfg.virtual_channel_enable_mask[7] = 0,
  .interrupt_cfg.virtual_channel_enable_mask[8] = 0,
  .interrupt_cfg.virtual_channel_enable_mask[9] = 0,
  .interrupt_cfg.virtual_channel_enable_mask[10] = 0,
  .interrupt_cfg.virtual_channel_enable_mask[11] = 0,
  .interrupt_cfg.virtual_channel_enable_mask[12] = 0,
  .interrupt_cfg.virtual_channel_enable_mask[13] = 0,
  .interrupt_cfg.virtual_channel_enable_mask[14] = 0,
  .interrupt_cfg.virtual_channel_enable_mask[15] = 0,
  .interrupt_cfg.power_management_enable_mask = 0,
  .interrupt_cfg.short_packet_enable_mask = 0,

  .p_callback = mipi_csi0_callback,
  .p_context = NULL,
  .p_extend = NULL, };

const mipi_csi_instance_t g_mipi_csi0 =
{ .p_ctrl = &g_mipi_csi0_ctrl, .p_cfg = &g_mipi_csi0_cfg, .p_api = &g_mipi_csi };

uint8_t vin_image_buffer_1[VIN_BYTES_PER_FRAME] BSP_ALIGN_VARIABLE(128) BSP_PLACE_IN_SECTION(BSP_UNINIT_SECTION_PREFIX ".sdram_noinit");
uint8_t vin_image_buffer_2[VIN_BYTES_PER_FRAME] BSP_ALIGN_VARIABLE(128) BSP_PLACE_IN_SECTION(BSP_UNINIT_SECTION_PREFIX ".sdram_noinit");
uint8_t vin_image_buffer_3[VIN_BYTES_PER_FRAME] BSP_ALIGN_VARIABLE(128) BSP_PLACE_IN_SECTION(BSP_UNINIT_SECTION_PREFIX ".sdram_noinit");

vin_instance_ctrl_t g_vin0_ctrl;
const vin_extended_cfg_t g_vin0_cfg_extend =
{ .p_mipi_csi_instance = &g_mipi_csi0,

  .input_ctrl.cfg_bits.color_space_convert_bypass = 0,
  .input_ctrl.cfg_bits.interlace_mode = VIN_INTERLACE_MODE_ODD_EVEN_FIELD_CAPTURE,
  .input_ctrl.cfg_bits.big_endian = 0,
  .input_ctrl.cfg_bits.dithering_mode = VIN_DITHERING_MODE_WITH_ADDITION,
  .input_ctrl.cfg_bits.input_mode = VIN_INPUT_FORMAT_YCBCR422_8_BIT,
  .input_ctrl.cfg_bits.lut_enable = 0,
  .input_ctrl.cfg_bits.dithering_direction = true,
  .input_ctrl.cfg_bits.yuv444_conversion = VIN_YUV444_CONVERSION_MODE_DATA_EXTEND,
  .input_ctrl.cfg_bits.scaling_enable = 0,
  .input_ctrl.cfg_bits.pixel_data_clipping = VIN_PIXEL_DATA_CLIPPING_DEFAULT,
  .input_ctrl.preclip.line_start = 1,
  .input_ctrl.preclip.line_end = RA8P1_CSI_VIN_INPUT_HEIGHT - 1,
  .input_ctrl.preclip.pixel_start = 1,
  .input_ctrl.preclip.pixel_end = RA8P1_CSI_VIN_INPUT_WIDTH - 1,
  .input_ctrl.csi_mode_bits.virtual_channel = 0,
  .input_ctrl.csi_mode_bits.data_type = VIN_DATA_TYPE_YUV422_8_BIT,
  .input_ctrl.csi_mode_bits.sign_extend_disable = 1,
  .input_ctrl.csi_detect_bits.field_detect_enable = 1,
  .input_ctrl.csi_detect_bits.even_field_detect_enable = 1,
  .input_ctrl.csi_detect_bits.even_field_number = 0,
  .input_ctrl.image_stride = RA8P1_CSI_VIN_INPUT_WIDTH,

  .output_ctrl.image_buffer = { vin_image_buffer_3, vin_image_buffer_2, vin_image_buffer_1 },
  .output_ctrl.use_runtime_buffer = false,

  .conversion_ctrl.data_mode_bits.data_conversion_mode = VIN_CONVERSION_MODE_NONE,
  .conversion_ctrl.data_mode_bits.alpha_bit_value = 0,
  .conversion_ctrl.data_mode_bits.output_data_byte_swap = 1,
  .conversion_ctrl.data_mode_bits.extend_rgb_converted_data = 0,
  .conversion_ctrl.data_mode_bits.yc_data_transform_enable = 0,
  .conversion_ctrl.data_mode_bits.yc_transform_mode = VIN_YC_TRANSFORM_MODE_Y_CBCR,
  .conversion_ctrl.data_mode_bits.rgb8888_alpha_value = 0xAA,

  .conversion_data.uv_address = 0x0,
  .conversion_data.yc_rgb_conversion_setting_1_bits.y_mul = 4767,
  .conversion_data.yc_rgb_conversion_setting_1_bits.round_down_disable = 0,
  .conversion_data.yc_rgb_conversion_setting_2_bits.csub2 = 2048,
  .conversion_data.yc_rgb_conversion_setting_2_bits.ysub2 = 256,
  .conversion_data.yc_rgb_conversion_setting_3_bits.cgrmul2 = 3330,
  .conversion_data.yc_rgb_conversion_setting_3_bits.rcrmul2 = 6537,
  .conversion_data.yc_rgb_conversion_setting_4_bits.gcbmul2 = 1605,
  .conversion_data.yc_rgb_conversion_setting_4_bits.bcbmul2 = 8261,
  .conversion_data.uds_ctrl_bits.ne_bcb = 1,
  .conversion_data.uds_ctrl_bits.ne_gy = 1,
  .conversion_data.uds_ctrl_bits.ne_rcr = 1,
  .conversion_data.uds_ctrl_bits.pixel_interpolation = 0,
  .conversion_data.uds_ctrl_bits.bilinear_advanced = 0,
  .conversion_data.uds_ctrl_bits.scale_up_pixel_count = 1,
  .conversion_data.uds_scale_bits.vertical_mask = RA8P1_CSI_VIN_SCALE_MASK_V,
  .conversion_data.uds_scale_bits.horizontal_mask = RA8P1_CSI_VIN_SCALE_MASK_H,
  .conversion_data.uds_bwidth_bits.bwidth_v = 64,
  .conversion_data.uds_bwidth_bits.bwidth_h = 64,
  .conversion_data.uds_clipping_bits.cl_vsize = RA8P1_CSI_VIN_INPUT_HEIGHT,
  .conversion_data.uds_clipping_bits.cl_hsize = RA8P1_CSI_VIN_INPUT_WIDTH,
  .conversion_data.rgb_to_yuv_conversion_settings[0].setting_1_bits.lrp = 263,
  .conversion_data.rgb_to_yuv_conversion_settings[0].setting_2_bits.lgp = 516,
  .conversion_data.rgb_to_yuv_conversion_settings[0].setting_2_bits.lbp = 100,
  .conversion_data.rgb_to_yuv_conversion_settings[0].setting_3_bits.lap = 256,
  .conversion_data.rgb_to_yuv_conversion_settings[0].setting_3_bits.lhen = 0,
  .conversion_data.rgb_to_yuv_conversion_settings[0].setting_3_bits.lsft = 10,
  .conversion_data.rgb_to_yuv_conversion_settings[0].setting_3_bits.persistent_bit0 = 1,
  .conversion_data.rgb_to_yuv_conversion_settings[0].setting_3_bits.persistent_bit1 = 1,
  .conversion_data.rgb_to_yuv_conversion_settings[1].setting_1_bits.lrp = -152,
  .conversion_data.rgb_to_yuv_conversion_settings[1].setting_2_bits.lgp = -298,
  .conversion_data.rgb_to_yuv_conversion_settings[1].setting_2_bits.lbp = 450,
  .conversion_data.rgb_to_yuv_conversion_settings[1].setting_3_bits.lap = 2048,
  .conversion_data.rgb_to_yuv_conversion_settings[1].setting_3_bits.lhen = 0,
  .conversion_data.rgb_to_yuv_conversion_settings[1].setting_3_bits.lsft = 10,
  .conversion_data.rgb_to_yuv_conversion_settings[1].setting_3_bits.persistent_bit0 = 1,
  .conversion_data.rgb_to_yuv_conversion_settings[1].setting_3_bits.persistent_bit1 = 1,
  .conversion_data.rgb_to_yuv_conversion_settings[2].setting_1_bits.lrp = 450,
  .conversion_data.rgb_to_yuv_conversion_settings[2].setting_2_bits.lgp = -377,
  .conversion_data.rgb_to_yuv_conversion_settings[2].setting_2_bits.lbp = -73,
  .conversion_data.rgb_to_yuv_conversion_settings[2].setting_3_bits.lap = 2048,
  .conversion_data.rgb_to_yuv_conversion_settings[2].setting_3_bits.lhen = 0,
  .conversion_data.rgb_to_yuv_conversion_settings[2].setting_3_bits.lsft = 10,
  .conversion_data.rgb_to_yuv_conversion_settings[2].setting_3_bits.persistent_bit0 = 1,
  .conversion_data.rgb_to_yuv_conversion_settings[2].setting_3_bits.persistent_bit1 = 1,
  .interrupt_cfg.status_enable_mask = (R_VIN_IE_FME_Msk | R_VIN_IE_EFE_Msk),
  .interrupt_cfg.scanline_compare_value = 0,
  .interrupt_cfg.status.ipl = (2),
#if defined(VECTOR_NUMBER_VIN_IRQ)
  .interrupt_cfg.status.irq = VECTOR_NUMBER_VIN_IRQ,
#else
  .interrupt_cfg.status.irq = FSP_INVALID_VECTOR,
#endif
  .interrupt_cfg.error.irq = FSP_INVALID_VECTOR,
};

const capture_cfg_t g_vin0_cfg =
{ .x_capture_start_pixel = 0xFFFF,
  .x_capture_pixels = 0xFFFF,
  .y_capture_start_pixel = 0xFFFF,
  .y_capture_pixels = 0xFFFF,
  .bytes_per_pixel = 0xFF,
  .p_callback = vin0_callback,
  .p_context = NULL,
  .p_extend = &g_vin0_cfg_extend, };

const capture_instance_t g_vin0 =
{ .p_ctrl = &g_vin0_ctrl, .p_cfg = &g_vin0_cfg, .p_api = &g_capture_on_vin };

gpt_instance_ctrl_t g_timer_periodic_ctrl;
const gpt_extended_cfg_t g_timer_periodic_extend =
{ .gtioca = { .output_enabled = true, .stop_level = GPT_PIN_LEVEL_LOW },
  .gtiocb = { .output_enabled = false, .stop_level = GPT_PIN_LEVEL_LOW },
  .start_source = (gpt_source_t) (GPT_SOURCE_NONE),
  .stop_source = (gpt_source_t) (GPT_SOURCE_NONE),
  .clear_source = (gpt_source_t) (GPT_SOURCE_NONE),
  .capture_a_source = (gpt_source_t) (GPT_SOURCE_NONE),
  .capture_b_source = (gpt_source_t) (GPT_SOURCE_NONE),
  .count_up_source = (gpt_source_t) (GPT_SOURCE_NONE),
  .count_down_source = (gpt_source_t) (GPT_SOURCE_NONE),
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
  .gtiocb_polarity = GPT_GTIOC_POLARITY_NORMAL, };

const timer_cfg_t g_timer_periodic_cfg =
{ .mode = TIMER_MODE_PERIODIC,
  .period_counts = (uint32_t)0xa,
  .source_div = (timer_source_div_t)0,
  .duty_cycle_counts = 0x5,
  .channel = 12,
  .cycle_end_ipl = (BSP_IRQ_DISABLED),
  .cycle_end_irq = FSP_INVALID_VECTOR,
  .p_callback = NULL,
  .p_context = NULL,
  .p_extend = &g_timer_periodic_extend, };

const timer_instance_t g_timer_periodic =
{ .p_ctrl = &g_timer_periodic_ctrl, .p_cfg = &g_timer_periodic_cfg, .p_api = &g_timer_on_gpt };
#endif
/** Display framebuffer */
        #if GLCDC_CFG_LAYER_1_ENABLE
        uint8_t g_framebuffer[DISPLAY_FRAMEBUFFER_COUNT_INPUT0][DISPLAY_BUFFER_STRIDE_BYTES_INPUT0 * DISPLAY_VSIZE_INPUT0] BSP_ALIGN_VARIABLE(64) BSP_PLACE_IN_SECTION(BSP_UNINIT_SECTION_PREFIX ".sdram_noinit");
        #else
        /** Graphics Layer 1 is specified not to be used when starting */
        #endif
        #if GLCDC_CFG_LAYER_2_ENABLE
        uint8_t fb_foreground[2][DISPLAY_BUFFER_STRIDE_BYTES_INPUT1 * DISPLAY_VSIZE_INPUT1] BSP_ALIGN_VARIABLE(64) BSP_PLACE_IN_SECTION(BSP_UNINIT_SECTION_PREFIX ".sdram_noinit");
        #else
        /** Graphics Layer 2 is specified not to be used when starting */
        #endif

        #if GLCDC_CFG_CLUT_ENABLE
        /** Display CLUT buffer to be used for updating CLUT */
        static uint32_t CLUT_buffer[256];

        /** Display CLUT configuration(only used if using CLUT format) */
        display_clut_cfg_t g_display_clut_cfg_glcdc =
        {
            .p_base              = (uint32_t *)CLUT_buffer,
            .start               = 0,   /* User have to update this setting when using */
            .size                = 256  /* User have to update this setting when using */
        };
        #else
        /** CLUT is specified not to be used */
        #endif

        #if (false)
         #define GLCDC_CFG_CORRECTION_GAMMA_TABLE_CONST   const
         #define GLCDC_CFG_CORRECTION_GAMMA_TABLE_CAST    (uint16_t *)
         #define GLCDC_CFG_CORRECTION_GAMMA_CFG_CAST      (display_gamma_correction_t *)
        #else
         #define GLCDC_CFG_CORRECTION_GAMMA_TABLE_CONST
         #define GLCDC_CFG_CORRECTION_GAMMA_TABLE_CAST
         #define GLCDC_CFG_CORRECTION_GAMMA_CFG_CAST
        #endif

        #if ((GLCDC_CFG_CORRECTION_GAMMA_ENABLE_R | GLCDC_CFG_CORRECTION_GAMMA_ENABLE_G | GLCDC_CFG_CORRECTION_GAMMA_ENABLE_B) && GLCDC_CFG_COLOR_CORRECTION_ENABLE)
        /** Gamma correction tables */
        #if GLCDC_CFG_CORRECTION_GAMMA_ENABLE_R
        static GLCDC_CFG_CORRECTION_GAMMA_TABLE_CONST uint16_t glcdc_gamma_r_gain[DISPLAY_GAMMA_CURVE_ELEMENT_NUM] = {1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024};
        static GLCDC_CFG_CORRECTION_GAMMA_TABLE_CONST uint16_t glcdc_gamma_r_threshold[DISPLAY_GAMMA_CURVE_ELEMENT_NUM] = {0, 64, 128, 192, 256, 320, 384, 448, 512, 576, 640, 704, 768, 832, 896, 960};
        #endif
        #if GLCDC_CFG_CORRECTION_GAMMA_ENABLE_G
        static GLCDC_CFG_CORRECTION_GAMMA_TABLE_CONST uint16_t glcdc_gamma_g_gain[DISPLAY_GAMMA_CURVE_ELEMENT_NUM] = {1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024};
        static GLCDC_CFG_CORRECTION_GAMMA_TABLE_CONST uint16_t glcdc_gamma_g_threshold[DISPLAY_GAMMA_CURVE_ELEMENT_NUM] = {0, 64, 128, 192, 256, 320, 384, 448, 512, 576, 640, 704, 768, 832, 896, 960};
        #endif
        #if GLCDC_CFG_CORRECTION_GAMMA_ENABLE_B
        static GLCDC_CFG_CORRECTION_GAMMA_TABLE_CONST uint16_t glcdc_gamma_b_gain[DISPLAY_GAMMA_CURVE_ELEMENT_NUM] = {1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024};
        static GLCDC_CFG_CORRECTION_GAMMA_TABLE_CONST uint16_t glcdc_gamma_b_threshold[DISPLAY_GAMMA_CURVE_ELEMENT_NUM] = {0, 64, 128, 192, 256, 320, 384, 448, 512, 576, 640, 704, 768, 832, 896, 960};
        #endif
        GLCDC_CFG_CORRECTION_GAMMA_TABLE_CONST display_gamma_correction_t g_display_gamma_cfg =
        {
            .r =
            {
                .enable      = GLCDC_CFG_CORRECTION_GAMMA_ENABLE_R,
        #if GLCDC_CFG_CORRECTION_GAMMA_ENABLE_R
                .gain        = GLCDC_CFG_CORRECTION_GAMMA_TABLE_CAST glcdc_gamma_r_gain,
                .threshold   = GLCDC_CFG_CORRECTION_GAMMA_TABLE_CAST glcdc_gamma_r_threshold
        #else
                .gain        = NULL,
                .threshold   = NULL
        #endif
            },
            .g =
            {
                .enable      = GLCDC_CFG_CORRECTION_GAMMA_ENABLE_G,
        #if GLCDC_CFG_CORRECTION_GAMMA_ENABLE_G
                .gain        = GLCDC_CFG_CORRECTION_GAMMA_TABLE_CAST glcdc_gamma_g_gain,
                .threshold   = GLCDC_CFG_CORRECTION_GAMMA_TABLE_CAST glcdc_gamma_g_threshold
        #else
                .gain        = NULL,
                .threshold   = NULL
        #endif
            },
            .b =
            {
                .enable      = GLCDC_CFG_CORRECTION_GAMMA_ENABLE_B,
        #if GLCDC_CFG_CORRECTION_GAMMA_ENABLE_B
                .gain        = GLCDC_CFG_CORRECTION_GAMMA_TABLE_CAST glcdc_gamma_b_gain,
                .threshold   = GLCDC_CFG_CORRECTION_GAMMA_TABLE_CAST glcdc_gamma_b_threshold
        #else
                .gain        = NULL,
                .threshold   = NULL
        #endif
            }
        };
        #endif

        #if defined(MICROPY_RA8P1_BRINGUP_MIPI_DSI_TEST) && MICROPY_RA8P1_BRINGUP_MIPI_DSI_TEST
          #define RA8P1_GLCDC_PHY_LAYER (&g_mipi_dsi0)
        #else
          #define RA8P1_GLCDC_PHY_LAYER (NULL)
        #endif
        /** Display device extended configuration */
        const glcdc_extended_cfg_t g_display_extend_cfg =
        {
            .tcon_hsync            = GLCDC_TCON_PIN_1,
            .tcon_vsync            = GLCDC_TCON_PIN_0,
            .tcon_de               = GLCDC_TCON_PIN_2,
            .correction_proc_order = GLCDC_CORRECTION_PROC_ORDER_BRIGHTNESS_CONTRAST2GAMMA,
            .clksrc                = GLCDC_CLK_SRC_INTERNAL,
            .clock_div_ratio       = GLCDC_PANEL_CLK_DIVISOR_8,
            .dithering_mode        = GLCDC_DITHERING_MODE_TRUNCATE,
            .dithering_pattern_A   = GLCDC_DITHERING_PATTERN_11,
            .dithering_pattern_B   = GLCDC_DITHERING_PATTERN_11,
            .dithering_pattern_C   = GLCDC_DITHERING_PATTERN_11,
            .dithering_pattern_D   = GLCDC_DITHERING_PATTERN_11,
            .phy_layer             = (void *)RA8P1_GLCDC_PHY_LAYER
        };
        #undef RA8P1_GLCDC_PHY_LAYER

        /** Display control block instance */
        glcdc_instance_ctrl_t g_display_ctrl;

        /** Display interface configuration */
        const display_cfg_t g_display_cfg =
        {
            /** Input1(Graphics1 layer) configuration */
            .input[0] =
            {
                #if GLCDC_CFG_LAYER_1_ENABLE
                .p_base              = (uint32_t *)&g_framebuffer[0],
                #else
                .p_base              = NULL,
                #endif
                .hsize               = DISPLAY_HSIZE_INPUT0,
                .vsize               = DISPLAY_VSIZE_INPUT0,
                .hstride             = DISPLAY_BUFFER_STRIDE_PIXELS_INPUT0,
                .format              = DISPLAY_IN_FORMAT_32BITS_RGB888,
                .line_descending_enable = false,
                .lines_repeat_enable = false,
                .lines_repeat_times  = 0
            },

            /** Input2(Graphics2 layer) configuration */
            .input[1] =
            {
                #if GLCDC_CFG_LAYER_2_ENABLE
                .p_base              = (uint32_t *)&fb_foreground[0],
                #else
                .p_base              = NULL,
                #endif
                .hsize               = DISPLAY_HSIZE_INPUT1,
                .vsize               = DISPLAY_VSIZE_INPUT1,
                .hstride             = DISPLAY_BUFFER_STRIDE_PIXELS_INPUT1,
                .format              = DISPLAY_IN_FORMAT_16BITS_RGB565,
                .line_descending_enable = false,
                .lines_repeat_enable = false,
                .lines_repeat_times  = 0
             },

            /** Input1(Graphics1 layer) layer configuration */
            .layer[0] =
            {
                .coordinate = {
                        .x           = 0,
                        .y           = 0
                },
                .fade_control        = DISPLAY_FADE_CONTROL_NONE,
                .fade_speed          = 0
            },

            /** Input2(Graphics2 layer) layer configuration */
            .layer[1] =
            {
                .coordinate = {
                        .x           = 0,
                        .y           = 0
                },
                .fade_control        = DISPLAY_FADE_CONTROL_NONE,
                .fade_speed          = 0
            },

            /** Output configuration */
            .output =
            {
                .htiming =
                {
                    .total_cyc       = 1344,
                    .display_cyc     = 1024,
                    .back_porch      = 140,
                    .sync_width       = 1,
                    .sync_polarity   = DISPLAY_SIGNAL_POLARITY_LOACTIVE
                },
                .vtiming =
                {
                    .total_cyc       = 635,
                    .display_cyc     = 600,
                    .back_porch      = 20,
                    .sync_width       = 1,
                    .sync_polarity   = DISPLAY_SIGNAL_POLARITY_LOACTIVE
                },
                .format              = DISPLAY_OUT_FORMAT_24BITS_RGB888,
                .endian              = DISPLAY_ENDIAN_LITTLE,
                .color_order         = DISPLAY_COLOR_ORDER_RGB,
                .data_enable_polarity = DISPLAY_SIGNAL_POLARITY_HIACTIVE,
                .sync_edge           = DISPLAY_SIGNAL_SYNC_EDGE_FALLING,
                .bg_color =
                {
                    .byte = {
                        .a           = 255,
                        .r           = 0,
                        .g           = 0,
                        .b           = 0
                    }
                },
#if (GLCDC_CFG_COLOR_CORRECTION_ENABLE)
                .brightness =
                {
                    .enable          = false,
                    .r               = 512,
                    .g               = 512,
                    .b               = 512
                },
                .contrast =
                {
                    .enable          = false,
                    .r               = 128,
                    .g               = 128,
                    .b               = 128
                },
#if (GLCDC_CFG_CORRECTION_GAMMA_ENABLE_R | GLCDC_CFG_CORRECTION_GAMMA_ENABLE_G | GLCDC_CFG_CORRECTION_GAMMA_ENABLE_B)
 #if false
                .p_gamma_correction  = GLCDC_CFG_CORRECTION_GAMMA_CFG_CAST (&g_display_gamma_cfg),
 #else
                .p_gamma_correction  = &g_display_gamma_cfg,
 #endif
#else
                .p_gamma_correction  = NULL,
#endif
#endif
                .dithering_on        = false
            },

            /** Display device callback function pointer */
            .p_callback              = NULL,
            .p_context               = NULL,

            /** Display device extended configuration */
            .p_extend                = (void *)(&g_display_extend_cfg),

            .line_detect_ipl        = (12),
            .underflow_1_ipl        = (BSP_IRQ_DISABLED),
            .underflow_2_ipl        = (BSP_IRQ_DISABLED),

#if defined(VECTOR_NUMBER_GLCDC_LINE_DETECT)
            .line_detect_irq        = VECTOR_NUMBER_GLCDC_LINE_DETECT,
#else
            .line_detect_irq        = FSP_INVALID_VECTOR,
#endif
#if defined(VECTOR_NUMBER_GLCDC_UNDERFLOW_1)
            .underflow_1_irq        = VECTOR_NUMBER_GLCDC_UNDERFLOW_1,
#else
            .underflow_1_irq        = FSP_INVALID_VECTOR,
#endif
#if defined(VECTOR_NUMBER_GLCDC_UNDERFLOW_2)
            .underflow_2_irq        = VECTOR_NUMBER_GLCDC_UNDERFLOW_2,
#else
            .underflow_2_irq        = FSP_INVALID_VECTOR,
#endif
        };

#if GLCDC_CFG_LAYER_1_ENABLE
        /** Display on GLCDC run-time configuration(for the graphics1 layer) */
        display_runtime_cfg_t g_display_runtime_cfg_bg =
        {
            .input =
            {
                #if (true)
                .p_base              = (uint32_t *)&g_framebuffer[0],
                #else
                .p_base              = NULL,
                #endif
                .hsize               = DISPLAY_HSIZE_INPUT0,
                .vsize               = DISPLAY_VSIZE_INPUT0,
                .hstride             = DISPLAY_BUFFER_STRIDE_PIXELS_INPUT0,
                .format              = DISPLAY_IN_FORMAT_32BITS_RGB888,
                .line_descending_enable = false,
                .lines_repeat_enable = false,
                .lines_repeat_times  = 0
            },
            .layer =
            {
                .coordinate = {
                        .x           = 0,
                        .y           = 0
                },
                .fade_control        = DISPLAY_FADE_CONTROL_NONE,
                .fade_speed          = 0
            }
        };
#endif
#if GLCDC_CFG_LAYER_2_ENABLE
        /** Display on GLCDC run-time configuration(for the graphics2 layer) */
        display_runtime_cfg_t g_display_runtime_cfg_fg =
        {
            .input =
            {
                #if (false)
                .p_base              = (uint32_t *)&fb_foreground[0],
                #else
                .p_base              = NULL,
                #endif
                .hsize               = DISPLAY_HSIZE_INPUT1,
                .vsize               = DISPLAY_VSIZE_INPUT1,
                .hstride             = DISPLAY_BUFFER_STRIDE_PIXELS_INPUT1,
                .format              = DISPLAY_IN_FORMAT_16BITS_RGB565,
                .line_descending_enable = false,
                .lines_repeat_enable = false,
                .lines_repeat_times  = 0
             },
            .layer =
            {
                .coordinate = {
                        .x           = 0,
                        .y           = 0
                },
                .fade_control        = DISPLAY_FADE_CONTROL_NONE,
                .fade_speed          = 0
            }
        };
#endif

/* Instance structure to use this module. */
const display_instance_t g_display =
{
    .p_ctrl        = &g_display_ctrl,
    .p_cfg         = (display_cfg_t *)&g_display_cfg,
    .p_api         = (display_api_t *)&g_display_on_glcdc
};
ioport_instance_ctrl_t g_ioport_ctrl;
const ioport_instance_t g_ioport =
        {
            .p_api = &g_ioport_on_ioport,
            .p_ctrl = &g_ioport_ctrl,
            .p_cfg = &g_bsp_pin_cfg,
        };
void g_common_init(void) {
}
