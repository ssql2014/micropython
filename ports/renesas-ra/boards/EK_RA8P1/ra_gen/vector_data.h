/* generated vector header file - do not edit */
        #ifndef VECTOR_DATA_H
        #define VECTOR_DATA_H
        #ifdef __cplusplus
        extern "C" {
        #endif
                /* Number of interrupts allocated */
        #ifndef VECTOR_DATA_IRQ_COUNT
        #if defined(MICROPY_RA8P1_BRINGUP_MIPI_DSI_TEST) && MICROPY_RA8P1_BRINGUP_MIPI_DSI_TEST
        #define VECTOR_DATA_IRQ_COUNT    (11)
        #elif defined(MICROPY_RA8P1_BRINGUP_MIPI_CSI_TEST) && MICROPY_RA8P1_BRINGUP_MIPI_CSI_TEST
        #define VECTOR_DATA_IRQ_COUNT    (14)
        #elif defined(MICROPY_RA8P1_BRINGUP_CEU_TEST) && MICROPY_RA8P1_BRINGUP_CEU_TEST
        #define VECTOR_DATA_IRQ_COUNT    (6)
        #else
        #define VECTOR_DATA_IRQ_COUNT    (5)
        #endif
        #endif
        /* ISR prototypes */
        void glcdc_line_detect_isr(void);
        void iic_master_rxi_isr(void);
        void iic_master_txi_isr(void);
        void iic_master_tei_isr(void);
        void iic_master_eri_isr(void);
        void mipi_dsi_seq0_isr(void);
        void mipi_dsi_seq1_isr(void);
        void mipi_dsi_vin1_isr(void);
        void mipi_dsi_rcv_isr(void);
        void mipi_dsi_ferr_isr(void);
        void mipi_dsi_ppi_isr(void);
        void vin_status_isr(void);
        void vin_error_isr(void);
        void mipi_csi_rx_isr(void);
        void mipi_csi_dl_isr(void);
        void mipi_csi_vc_isr(void);
        void ceu_isr(void);
        void canfd_rx_fifo_isr(void);
        void canfd_error_isr(void);

        /* Vector table allocations */
        #define VECTOR_NUMBER_GLCDC_LINE_DETECT ((IRQn_Type) 0) /* GLCDC LINE DETECT (Specified line) */
        #define GLCDC_LINE_DETECT_IRQn          ((IRQn_Type) 0) /* GLCDC LINE DETECT (Specified line) */
        #define VECTOR_NUMBER_IIC0_RXI          ((IRQn_Type) 1) /* IIC0 RXI (Receive data full) */
        #define IIC0_RXI_IRQn                   ((IRQn_Type) 1)
        #define VECTOR_NUMBER_IIC0_TXI          ((IRQn_Type) 2) /* IIC0 TXI (Transmit data empty) */
        #define IIC0_TXI_IRQn                   ((IRQn_Type) 2)
        #define VECTOR_NUMBER_IIC0_TEI          ((IRQn_Type) 3) /* IIC0 TEI (Transmit end) */
        #define IIC0_TEI_IRQn                   ((IRQn_Type) 3)
        #define VECTOR_NUMBER_IIC0_ERI          ((IRQn_Type) 4) /* IIC0 ERI (Transfer error) */
        #define IIC0_ERI_IRQn                   ((IRQn_Type) 4)
        #if defined(MICROPY_RA8P1_BRINGUP_MIPI_DSI_TEST) && MICROPY_RA8P1_BRINGUP_MIPI_DSI_TEST
        #define VECTOR_NUMBER_MIPIDSI_SEQ0      ((IRQn_Type) 5)  /* MIPI DSI Sequence channel 0 */
        #define MIPIDSI_SEQ0_IRQn               ((IRQn_Type) 5)
        #define VECTOR_NUMBER_MIPIDSI_SEQ1      ((IRQn_Type) 6)  /* MIPI DSI Sequence channel 1 */
        #define MIPIDSI_SEQ1_IRQn               ((IRQn_Type) 6)
        #define VECTOR_NUMBER_MIPIDSI_VIN1      ((IRQn_Type) 7)  /* MIPI DSI Video input channel 1 */
        #define MIPIDSI_VIN1_IRQn               ((IRQn_Type) 7)
        #define VECTOR_NUMBER_MIPIDSI_RCV       ((IRQn_Type) 8)  /* MIPI DSI packet receive */
        #define MIPIDSI_RCV_IRQn                ((IRQn_Type) 8)
        #define VECTOR_NUMBER_MIPIDSI_FERR      ((IRQn_Type) 9)  /* MIPI DSI fatal error */
        #define MIPIDSI_FERR_IRQn               ((IRQn_Type) 9)
        #define VECTOR_NUMBER_MIPIDSI_PPI       ((IRQn_Type) 10) /* MIPI DSI D-PHY PPI */
        #define MIPIDSI_PPI_IRQn                ((IRQn_Type) 10)
        #endif
        #if defined(MICROPY_RA8P1_BRINGUP_MIPI_CSI_TEST) && MICROPY_RA8P1_BRINGUP_MIPI_CSI_TEST
        #define VECTOR_NUMBER_IIC1_RXI          ((IRQn_Type) 5)  /* IIC1 RXI (Receive data full) */
        #define IIC1_RXI_IRQn                   ((IRQn_Type) 5)
        #define VECTOR_NUMBER_IIC1_TXI          ((IRQn_Type) 6)  /* IIC1 TXI (Transmit data empty) */
        #define IIC1_TXI_IRQn                   ((IRQn_Type) 6)
        #define VECTOR_NUMBER_IIC1_TEI          ((IRQn_Type) 7)  /* IIC1 TEI (Transmit end) */
        #define IIC1_TEI_IRQn                   ((IRQn_Type) 7)
        #define VECTOR_NUMBER_IIC1_ERI          ((IRQn_Type) 8)  /* IIC1 ERI (Transfer error) */
        #define IIC1_ERI_IRQn                   ((IRQn_Type) 8)
        #define VECTOR_NUMBER_VIN_IRQ           ((IRQn_Type) 9)  /* VIN IRQ (Interrupt Request) */
        #define VIN_IRQ_IRQn                    ((IRQn_Type) 9)
        #define VECTOR_NUMBER_MIPICSI_RX        ((IRQn_Type) 10) /* MIPICSI RX (Receive interrupt) */
        #define MIPICSI_RX_IRQn                 ((IRQn_Type) 10)
        #define VECTOR_NUMBER_MIPICSI_DL        ((IRQn_Type) 11) /* MIPICSI DL (Data Lane interrupt) */
        #define MIPICSI_DL_IRQn                 ((IRQn_Type) 11)
        #define VECTOR_NUMBER_MIPICSI_VC        ((IRQn_Type) 12) /* MIPICSI VC (Virtual Channel interrupt) */
        #define MIPICSI_VC_IRQn                 ((IRQn_Type) 12)
        #define VECTOR_NUMBER_VIN_ERR           ((IRQn_Type) 13) /* VIN ERR (Interrupt Request for SYNC Error) */
        #define VIN_ERR_IRQn                    ((IRQn_Type) 13)
        #endif
        #if defined(MICROPY_RA8P1_BRINGUP_CEU_TEST) && MICROPY_RA8P1_BRINGUP_CEU_TEST
        #define VECTOR_NUMBER_CEU_CEUI          ((IRQn_Type) 5) /* CEU interrupt */
        #define CEU_CEUI_IRQn                   ((IRQn_Type) 5)
        #endif
        /* CANFD vectors defined for r_canfd.c to compile but NOT in the ICU table.
         * ra8p1_canfd.c passes FSP_INVALID_VECTOR for all IRQs so these are never
         * written to IELSR and the NVIC is never enabled for these channels. */
        #define VECTOR_NUMBER_CAN_RXF           ((IRQn_Type) 5) /* CANFD Global RX FIFO */
        #define CAN_RXF_IRQn                    ((IRQn_Type) 5)
        #define VECTOR_NUMBER_CAN_GLERR         ((IRQn_Type) 6) /* CANFD Global Error */
        #define CAN_GLERR_IRQn                  ((IRQn_Type) 6)
        /* The number of entries required for the ICU vector table (GLCDC + IIC0 x4). */
        #if defined(MICROPY_RA8P1_BRINGUP_MIPI_DSI_TEST) && MICROPY_RA8P1_BRINGUP_MIPI_DSI_TEST
        #define BSP_ICU_VECTOR_NUM_ENTRIES (11)
        #elif defined(MICROPY_RA8P1_BRINGUP_MIPI_CSI_TEST) && MICROPY_RA8P1_BRINGUP_MIPI_CSI_TEST
        #define BSP_ICU_VECTOR_NUM_ENTRIES (14)
        #elif defined(MICROPY_RA8P1_BRINGUP_CEU_TEST) && MICROPY_RA8P1_BRINGUP_CEU_TEST
        #define BSP_ICU_VECTOR_NUM_ENTRIES (6)
        #else
        #define BSP_ICU_VECTOR_NUM_ENTRIES (5)
        #endif

        #ifdef __cplusplus
        }
        #endif
        #endif /* VECTOR_DATA_H */
