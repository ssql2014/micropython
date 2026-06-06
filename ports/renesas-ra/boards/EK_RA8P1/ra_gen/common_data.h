/* generated common header file - do not edit */
#ifndef COMMON_DATA_H_
#define COMMON_DATA_H_
#include <stdint.h>
#include "bsp_api.h"
#include "r_glcdc.h"
            #include "r_display_api.h"
#if defined(MICROPY_RA8P1_BRINGUP_MIPI_DSI_TEST) && MICROPY_RA8P1_BRINGUP_MIPI_DSI_TEST
#include "r_mipi_dsi.h"
#endif
#if defined(MICROPY_RA8P1_BRINGUP_MIPI_CSI_TEST) && MICROPY_RA8P1_BRINGUP_MIPI_CSI_TEST
#include "r_mipi_csi.h"
#include "r_mipi_csi_api.h"
#include "r_vin.h"
#include "r_capture_api.h"
#include "r_gpt.h"
#include "r_timer_api.h"
#endif
#include "r_ioport.h"
#include "bsp_pin_cfg.h"
FSP_HEADER
#if defined(MICROPY_RA8P1_BRINGUP_MIPI_DSI_TEST) && MICROPY_RA8P1_BRINGUP_MIPI_DSI_TEST
            /* MIPI PHY on MIPI PHY Instance. */
            extern const mipi_phy_instance_t g_mipi_phy0;

            /** Access the MIPI PHY instance using these structures when calling API functions directly (::p_api is not used). */
            extern mipi_phy_ctrl_t g_mipi_phy0_ctrl;
            extern const mipi_phy_cfg_t g_mipi_phy0_cfg;

            /* MIPI DSI on MIPI DSI Instance. */
            extern const mipi_dsi_instance_t g_mipi_dsi0;

            /** Access the MIPI DSI instance using these structures when calling API functions directly (::p_api is not used). */
            extern mipi_dsi_instance_ctrl_t g_mipi_dsi0_ctrl;
            extern const mipi_dsi_cfg_t g_mipi_dsi0_cfg;

            void mipi_dsi_callback(mipi_dsi_callback_args_t * p_args);
#endif
#if defined(MICROPY_RA8P1_BRINGUP_MIPI_CSI_TEST) && MICROPY_RA8P1_BRINGUP_MIPI_CSI_TEST
            /* MIPI PHY on MIPI PHY Instance. */
            extern const mipi_phy_instance_t g_mipi_phy0;
            extern mipi_phy_ctrl_t g_mipi_phy0_ctrl;
            extern const mipi_phy_cfg_t g_mipi_phy0_cfg;

            /* MIPI CSI on MIPI CSI Instance. */
            extern const mipi_csi_instance_t g_mipi_csi0;
            extern mipi_csi_instance_ctrl_t g_mipi_csi0_ctrl;
            extern const mipi_csi_cfg_t g_mipi_csi0_cfg;

            void mipi_csi0_callback(mipi_csi_callback_args_t * p_args);

            /* MIPI VIN on MIPI VIN Instance. */
            extern const capture_instance_t g_vin0;
            extern vin_instance_ctrl_t g_vin0_ctrl;
            extern const capture_cfg_t g_vin0_cfg;
            extern const vin_extended_cfg_t g_vin0_cfg_extend;

            void vin0_callback(capture_callback_args_t * p_args);

            #define VIN_CFG_BYTES_PER_PIXEL (2)
            #define VIN_IMAGE_WIDTH (1024)
            #define VIN_IMAGE_HEIGHT (600)
            #define VIN_BYTES_PER_FRAME (VIN_IMAGE_WIDTH * VIN_IMAGE_HEIGHT * VIN_CFG_BYTES_PER_PIXEL)

            extern uint8_t vin_image_buffer_1[VIN_BYTES_PER_FRAME];
            extern uint8_t vin_image_buffer_2[VIN_BYTES_PER_FRAME];
            extern uint8_t vin_image_buffer_3[VIN_BYTES_PER_FRAME];

            /* Timer on GPT12 Instance for camera XCLK. */
            extern const timer_instance_t g_timer_periodic;
            extern gpt_instance_ctrl_t g_timer_periodic_ctrl;
            extern const timer_cfg_t g_timer_periodic_cfg;
#endif
#define GLCDC_CFG_LAYER_1_ENABLE (true)
            #define GLCDC_CFG_LAYER_2_ENABLE (false)

            #define GLCDC_CFG_CLUT_ENABLE (false)

            #define GLCDC_CFG_CORRECTION_GAMMA_ENABLE_R       (false)
            #define GLCDC_CFG_CORRECTION_GAMMA_ENABLE_G       (false)
            #define GLCDC_CFG_CORRECTION_GAMMA_ENABLE_B       (false)

            /* Display on GLCDC Instance. */
            extern const display_instance_t g_display;
            extern display_runtime_cfg_t g_display_runtime_cfg_fg;
            extern display_runtime_cfg_t g_display_runtime_cfg_bg;

            /** Access the GLCDC instance using these structures when calling API functions directly (::p_api is not used). */
            extern glcdc_instance_ctrl_t g_display_ctrl;
            extern const display_cfg_t g_display_cfg;

            #if ((GLCDC_CFG_CORRECTION_GAMMA_ENABLE_R | GLCDC_CFG_CORRECTION_GAMMA_ENABLE_G | GLCDC_CFG_CORRECTION_GAMMA_ENABLE_B) && GLCDC_CFG_COLOR_CORRECTION_ENABLE && !(false))
            extern display_gamma_correction_t g_display_gamma_cfg;
            #endif

            #if GLCDC_CFG_CLUT_ENABLE
            extern display_clut_cfg_t g_display_clut_cfg_glcdc;
            #endif

            #ifndef NULL
            void NULL(display_callback_args_t * p_args);
            #endif

            #define DISPLAY_IN_FORMAT_32BITS_RGB888_0
            #if defined (DISPLAY_IN_FORMAT_32BITS_RGB888_0) || defined (DISPLAY_IN_FORMAT_32BITS_ARGB8888_0)
            #define DISPLAY_BITS_PER_PIXEL_INPUT0 (32)
            #elif defined (DISPLAY_IN_FORMAT_16BITS_RGB565_0) || defined (DISPLAY_IN_FORMAT_16BITS_ARGB1555_0) || defined (DISPLAY_IN_FORMAT_16BITS_ARGB4444_0)
            #define DISPLAY_BITS_PER_PIXEL_INPUT0 (16)
            #elif defined (DISPLAY_IN_FORMAT_CLUT8_0)
            #define DISPLAY_BITS_PER_PIXEL_INPUT0 (8)
            #elif defined (DISPLAY_IN_FORMAT_CLUT4_0)
            #define DISPLAY_BITS_PER_PIXEL_INPUT0 (4)
            #else
            #define DISPLAY_BITS_PER_PIXEL_INPUT0 (1)
            #endif
            #define DISPLAY_HSIZE_INPUT0                 (1024)
            #define DISPLAY_VSIZE_INPUT0                 (600)
            #define DISPLAY_BUFFER_STRIDE_BYTES_INPUT0   (((DISPLAY_HSIZE_INPUT0 * DISPLAY_BITS_PER_PIXEL_INPUT0 + 0x1FF) >> 9) << 6)
            #define DISPLAY_BUFFER_STRIDE_PIXELS_INPUT0  ((DISPLAY_BUFFER_STRIDE_BYTES_INPUT0 * 8) / DISPLAY_BITS_PER_PIXEL_INPUT0)
            #if defined(MICROPY_RA8P1_BRINGUP_MIPI_CSI_TEST) && MICROPY_RA8P1_BRINGUP_MIPI_CSI_TEST
            #define DISPLAY_FRAMEBUFFER_COUNT_INPUT0     (1)
            #else
            #define DISPLAY_FRAMEBUFFER_COUNT_INPUT0     (2)
            #endif
            #if GLCDC_CFG_LAYER_1_ENABLE
            extern uint8_t g_framebuffer[DISPLAY_FRAMEBUFFER_COUNT_INPUT0][DISPLAY_BUFFER_STRIDE_BYTES_INPUT0 * DISPLAY_VSIZE_INPUT0];
            #endif

            #define DISPLAY_IN_FORMAT_16BITS_RGB565_1
            #if defined (DISPLAY_IN_FORMAT_32BITS_RGB888_1) || defined (DISPLAY_IN_FORMAT_32BITS_ARGB8888_1)
            #define DISPLAY_BITS_PER_PIXEL_INPUT1 (32)
            #elif defined (DISPLAY_IN_FORMAT_16BITS_RGB565_1) || defined (DISPLAY_IN_FORMAT_16BITS_ARGB1555_1) || defined (DISPLAY_IN_FORMAT_16BITS_ARGB4444_1)
            #define DISPLAY_BITS_PER_PIXEL_INPUT1 (16)
            #elif defined (DISPLAY_IN_FORMAT_CLUT8_1)
            #define DISPLAY_BITS_PER_PIXEL_INPUT1 (8)
            #elif defined (DISPLAY_IN_FORMAT_CLUT4_1)
            #define DISPLAY_BITS_PER_PIXEL_INPUT1 (4)
            #else
            #define DISPLAY_BITS_PER_PIXEL_INPUT1 (1)
            #endif
            #define DISPLAY_HSIZE_INPUT1                 (480)
            #define DISPLAY_VSIZE_INPUT1                 (854)
            #define DISPLAY_BUFFER_STRIDE_BYTES_INPUT1   (((DISPLAY_HSIZE_INPUT1 * DISPLAY_BITS_PER_PIXEL_INPUT1 + 0x1FF) >> 9) << 6)
            #define DISPLAY_BUFFER_STRIDE_PIXELS_INPUT1  ((DISPLAY_BUFFER_STRIDE_BYTES_INPUT1 * 8) / DISPLAY_BITS_PER_PIXEL_INPUT1)
            #if GLCDC_CFG_LAYER_2_ENABLE
            extern uint8_t fb_foreground[2][DISPLAY_BUFFER_STRIDE_BYTES_INPUT1 * DISPLAY_VSIZE_INPUT1];
            #endif
#define IOPORT_CFG_NAME g_bsp_pin_cfg
#define IOPORT_CFG_OPEN R_IOPORT_Open
#define IOPORT_CFG_CTRL g_ioport_ctrl

/* IOPORT Instance */
extern const ioport_instance_t g_ioport;

/* IOPORT control structure. */
extern ioport_instance_ctrl_t g_ioport_ctrl;
void g_common_init(void);
FSP_FOOTER
#endif /* COMMON_DATA_H_ */
