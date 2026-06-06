/* generated vector source file - do not edit */
        #include "bsp_api.h"
        /* Do not build these data structures if no interrupts are currently allocated because IAR will have build errors. */
        #if VECTOR_DATA_IRQ_COUNT > 0
        BSP_DONT_REMOVE const fsp_vector_t g_vector_table[BSP_ICU_VECTOR_NUM_ENTRIES] BSP_PLACE_IN_SECTION(BSP_SECTION_APPLICATION_VECTORS) =
        {
                        [0] = glcdc_line_detect_isr, /* GLCDC LINE DETECT (Specified line) */
                        [1] = iic_master_rxi_isr,    /* IIC0 RXI (Receive data full) */
                        [2] = iic_master_txi_isr,    /* IIC0 TXI (Transmit data empty) */
                        [3] = iic_master_tei_isr,    /* IIC0 TEI (Transmit end) */
                        [4] = iic_master_eri_isr,    /* IIC0 ERI (Transfer error) */
        #if defined(MICROPY_RA8P1_BRINGUP_MIPI_DSI_TEST) && MICROPY_RA8P1_BRINGUP_MIPI_DSI_TEST
                        [5] = mipi_dsi_seq0_isr,      /* MIPI DSI Sequence channel 0 */
                        [6] = mipi_dsi_seq1_isr,      /* MIPI DSI Sequence channel 1 */
                        [7] = mipi_dsi_vin1_isr,      /* MIPI DSI Video input channel 1 */
                        [8] = mipi_dsi_rcv_isr,       /* MIPI DSI packet receive */
                        [9] = mipi_dsi_ferr_isr,      /* MIPI DSI fatal error */
                        [10] = mipi_dsi_ppi_isr,      /* MIPI DSI D-PHY PPI */
        #endif
        #if defined(MICROPY_RA8P1_BRINGUP_MIPI_CSI_TEST) && MICROPY_RA8P1_BRINGUP_MIPI_CSI_TEST
                        [5] = iic_master_rxi_isr,     /* IIC1 RXI (Receive data full) */
                        [6] = iic_master_txi_isr,     /* IIC1 TXI (Transmit data empty) */
                        [7] = iic_master_tei_isr,     /* IIC1 TEI (Transmit end) */
                        [8] = iic_master_eri_isr,     /* IIC1 ERI (Transfer error) */
                        [9] = vin_status_isr,         /* VIN IRQ (Interrupt Request) */
                        [10] = mipi_csi_rx_isr,       /* MIPICSI RX (Receive interrupt) */
                        [11] = mipi_csi_dl_isr,       /* MIPICSI DL (Data Lane interrupt) */
                        [12] = mipi_csi_vc_isr,       /* MIPICSI VC (Virtual Channel interrupt) */
                        [13] = vin_error_isr,         /* VIN ERR (Interrupt Request for SYNC Error) */
        #endif
        #if defined(MICROPY_RA8P1_BRINGUP_CEU_TEST) && MICROPY_RA8P1_BRINGUP_CEU_TEST
                        [5] = ceu_isr,                 /* CEU interrupt */
        #endif
        };
        #if BSP_FEATURE_ICU_HAS_IELSR
        const bsp_interrupt_event_t g_interrupt_event_link_select[BSP_ICU_VECTOR_NUM_ENTRIES] =
        {
            [0] = BSP_PRV_VECT_ENUM(EVENT_GLCDC_LINE_DETECT,GROUP0), /* GLCDC LINE DETECT (Specified line) */
            [1] = BSP_PRV_VECT_ENUM(EVENT_IIC0_RXI,GROUP0),          /* IIC0 RXI (Receive data full) */
            [2] = BSP_PRV_VECT_ENUM(EVENT_IIC0_TXI,GROUP0),          /* IIC0 TXI (Transmit data empty) */
            [3] = BSP_PRV_VECT_ENUM(EVENT_IIC0_TEI,GROUP0),          /* IIC0 TEI (Transmit end) */
            [4] = BSP_PRV_VECT_ENUM(EVENT_IIC0_ERI,GROUP0),          /* IIC0 ERI (Transfer error) */
        #if defined(MICROPY_RA8P1_BRINGUP_MIPI_DSI_TEST) && MICROPY_RA8P1_BRINGUP_MIPI_DSI_TEST
            [5] = BSP_PRV_VECT_ENUM(EVENT_MIPIDSI_SEQ0,GROUP0),       /* MIPI DSI Sequence channel 0 */
            [6] = BSP_PRV_VECT_ENUM(EVENT_MIPIDSI_SEQ1,GROUP0),       /* MIPI DSI Sequence channel 1 */
            [7] = BSP_PRV_VECT_ENUM(EVENT_MIPIDSI_VIN1,GROUP0),       /* MIPI DSI Video input channel 1 */
            [8] = BSP_PRV_VECT_ENUM(EVENT_MIPIDSI_RCV,GROUP0),        /* MIPI DSI packet receive */
            [9] = BSP_PRV_VECT_ENUM(EVENT_MIPIDSI_FERR,GROUP0),       /* MIPI DSI fatal error */
            [10] = BSP_PRV_VECT_ENUM(EVENT_MIPIDSI_PPI,GROUP0),       /* MIPI DSI D-PHY PPI */
        #endif
        #if defined(MICROPY_RA8P1_BRINGUP_MIPI_CSI_TEST) && MICROPY_RA8P1_BRINGUP_MIPI_CSI_TEST
            [5] = BSP_PRV_VECT_ENUM(EVENT_IIC1_RXI,GROUP0),           /* IIC1 RXI (Receive data full) */
            [6] = BSP_PRV_VECT_ENUM(EVENT_IIC1_TXI,GROUP1),           /* IIC1 TXI (Transmit data empty) */
            [7] = BSP_PRV_VECT_ENUM(EVENT_IIC1_TEI,GROUP2),           /* IIC1 TEI (Transmit end) */
            [8] = BSP_PRV_VECT_ENUM(EVENT_IIC1_ERI,GROUP3),           /* IIC1 ERI (Transfer error) */
            [9] = BSP_PRV_VECT_ENUM(EVENT_VIN_IRQ,FIXED),             /* VIN IRQ (Interrupt Request) */
            [10] = BSP_PRV_VECT_ENUM(EVENT_MIPICSI_RX,FIXED),         /* MIPICSI RX (Receive interrupt) */
            [11] = BSP_PRV_VECT_ENUM(EVENT_MIPICSI_DL,FIXED),         /* MIPICSI DL (Data Lane interrupt) */
            [12] = BSP_PRV_VECT_ENUM(EVENT_MIPICSI_VC,FIXED),         /* MIPICSI VC (Virtual Channel interrupt) */
            [13] = BSP_PRV_VECT_ENUM(EVENT_VIN_ERR,FIXED),            /* VIN ERR (Interrupt Request for SYNC Error) */
        #endif
        #if defined(MICROPY_RA8P1_BRINGUP_CEU_TEST) && MICROPY_RA8P1_BRINGUP_CEU_TEST
            [5] = BSP_PRV_VECT_ENUM(EVENT_CEU_CEUI,FIXED),             /* CEU interrupt */
        #endif
        };
        #endif
        #endif
