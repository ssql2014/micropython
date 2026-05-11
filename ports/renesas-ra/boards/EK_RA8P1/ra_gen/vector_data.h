/* generated vector header file - do not edit */
        #ifndef VECTOR_DATA_H
        #define VECTOR_DATA_H
        #ifdef __cplusplus
        extern "C" {
        #endif
                /* Number of interrupts allocated */
        #ifndef VECTOR_DATA_IRQ_COUNT
        #define VECTOR_DATA_IRQ_COUNT    (5)
        #endif
        /* ISR prototypes */
        void glcdc_line_detect_isr(void);
        void iic_master_rxi_isr(void);
        void iic_master_txi_isr(void);
        void iic_master_tei_isr(void);
        void iic_master_eri_isr(void);
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
        /* CANFD vectors defined for r_canfd.c to compile but NOT in the ICU table.
         * ra8p1_canfd.c passes FSP_INVALID_VECTOR for all IRQs so these are never
         * written to IELSR and the NVIC is never enabled for these channels. */
        #define VECTOR_NUMBER_CAN_RXF           ((IRQn_Type) 5) /* CANFD Global RX FIFO */
        #define CAN_RXF_IRQn                    ((IRQn_Type) 5)
        #define VECTOR_NUMBER_CAN_GLERR         ((IRQn_Type) 6) /* CANFD Global Error */
        #define CAN_GLERR_IRQn                  ((IRQn_Type) 6)
        /* The number of entries required for the ICU vector table (GLCDC + IIC0 x4). */
        #define BSP_ICU_VECTOR_NUM_ENTRIES (5)

        #ifdef __cplusplus
        }
        #endif
        #endif /* VECTOR_DATA_H */
