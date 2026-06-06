/* generated configuration header file - do not edit */
#ifndef R_GLCDC_CFG_H_
#define R_GLCDC_CFG_H_
#ifdef __cplusplus
            extern "C" {
            #endif

            #define GLCDC_CFG_PARAM_CHECKING_ENABLE   (BSP_CFG_PARAM_CHECKING_ENABLE)
            #define GLCDC_CFG_COLOR_CORRECTION_ENABLE (false)

            /* Enable DSI function handling */
            #if defined(MICROPY_RA8P1_BRINGUP_MIPI_DSI_TEST) && MICROPY_RA8P1_BRINGUP_MIPI_DSI_TEST
            #define GLCDC_CFG_USING_DSI
            #endif

            #ifdef __cplusplus
            }
            #endif
#endif /* R_GLCDC_CFG_H_ */
