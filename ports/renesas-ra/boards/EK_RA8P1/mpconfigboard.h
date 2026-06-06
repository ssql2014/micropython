// MCU config
#define MICROPY_HW_BOARD_NAME       "EK-RA8P1"
#define MICROPY_HW_MCU_NAME         "RA8P1"
#define MICROPY_HW_MCU_SYSCLK       48000000
#define MICROPY_HW_MCU_PCLK         48000000

// module config
#define MICROPY_EMIT_THUMB          (1)
#define MICROPY_EMIT_INLINE_THUMB   (1)
#define MICROPY_PY_BUILTINS_COMPLEX (1)
#define MICROPY_PY_GENERATOR_PEND_THROW (1)
#define MICROPY_PY_MATH             (1)
#define MICROPY_PY_UHEAPQ           (1)
#define MICROPY_PY_UTIMEQ           (1)
#define MICROPY_PY_THREAD           (0) // disable ARM_THUMB_FP using vldr due to RA has single float only

// peripheral config
#define MICROPY_HW_ENABLE_RTC       (1)
#define MICROPY_HW_ENABLE_HW_PWM    (1)
#define MICROPY_HW_ENABLE_DISPLAY   (1)
#define MICROPY_HW_USE_RTT_REPL     (1)
#define MICROPY_NLR_SETJMP          (1)
#define MICROPY_PY_MACHINE_I2C      (1)
#define MICROPY_PY_MACHINE_SPI      (1)
#define MICROPY_PY_MACHINE_PWM      (1)
// PWM channel pin assignments for EK-RA8P1.
// LED pins are dual-function (GPIO LED + GPT alt) — calling machine.PWM(pin)
// switches the pin from GPIO to PWM output. Restored to GPIO via deinit.
#define MICROPY_HW_PWM_6B           (pin_P600)  // GTIOC6B = LED1 (Blue)
#define MICROPY_HW_PWM_7A           (pin_PA07)  // GTIOC7A = LED3 (Red)
#define MICROPY_HW_PWM_7B           (pin_P303)  // GTIOC7B = LED2 (Green)
#define MICROPY_HW_PWM_4A           (pin_P302)  // GTIOC4A — free pin, scope/probe friendly
#define MICROPY_HW_RTC_SOURCE       (1)     // 0: subclock, 1: mainclock
/* ADC and DAC enabled via ra8p1_adc.c / ra8p1_dac.c (r_adc_b / r_dac_b FSP drivers). */
#define MICROPY_HW_ENABLE_ADC       (1)
#define MICROPY_HW_ENABLE_HW_DAC    (1)
/* DAC output pins on EK-RA8P1 (verify against schematic). */
#define MICROPY_HW_DAC0             (pin_P014)
#define MICROPY_HW_DAC1             (pin_P015)
/* CAN: appended to the port-level MICROPY_PY_MACHINE_EXTRA_GLOBALS in
 * modmachine.c via MICROPY_BOARD_MACHINE_EXTRA_GLOBALS.
 * machine_can_type is defined in machine_can.c. */
#define MICROPY_BOARD_MACHINE_EXTRA_GLOBALS \
    { MP_ROM_QSTR(MP_QSTR_CAN), MP_ROM_PTR(&machine_can_type) },
#define MICROPY_HW_HAS_FLASH        (0)
#define MICROPY_HW_ENABLE_INTERNAL_FLASH_STORAGE (0)
#define MICROPY_HW_HAS_QSPI_FLASH   (0)
#define MICROPY_RA8P1_BRINGUP_NO_EXTINT (1)
#define MICROPY_RA8P1_BRINGUP_NO_STORAGE (1)
#define MICROPY_RA8P1_BRINGUP_NO_DUPTERM (1)
#define MICROPY_RA8P1_BRINGUP_POLLED_TX (1)
#define MICROPY_RA8P1_BRINGUP_LED_TEST (0)
#ifndef MICROPY_RA8P1_BRINGUP_DISPLAY_TEST
#define MICROPY_RA8P1_BRINGUP_DISPLAY_TEST (0)
#endif
#ifndef MICROPY_RA8P1_BRINGUP_MIPI_DSI_TEST
#define MICROPY_RA8P1_BRINGUP_MIPI_DSI_TEST (0)
#endif
#ifndef MICROPY_RA8P1_BRINGUP_CEU_TEST
#define MICROPY_RA8P1_BRINGUP_CEU_TEST (0)
#endif
#ifndef MICROPY_RA8P1_BRINGUP_MIPI_CSI_TEST
#define MICROPY_RA8P1_BRINGUP_MIPI_CSI_TEST (0)
#endif
#ifndef MICROPY_RA8P1_BRINGUP_MIPI_CSI_BOOT_PROBE
#define MICROPY_RA8P1_BRINGUP_MIPI_CSI_BOOT_PROBE (0)
#endif
#define MICROPY_HW_FLASH_MOUNT_AT_BOOT (0)

// board config

// UART pin assignments.
// SCI0 = external Pmod USBUART path (P602/P603 — per FSP sci_uart_ek_ra8p1_ep
// example readme).  Useful when REPL is wanted over an external USB-to-UART
// adapter rather than the on-board J-Link OB CDC (which doesn't deliver bytes
// on this board — see board README "Quirks").
#define MICROPY_HW_UART0_TX         (pin_P603)  // J4:4
#define MICROPY_HW_UART0_RX         (pin_P602)  // J4:8
// SCI8 = the on-board VCOM path via J-Link OB (PD02/PD03).  Pins are mapped
// for completeness even though the OB CDC bridge is dead on this board.
#define MICROPY_HW_UART8_TX         (pin_PD02)
#define MICROPY_HW_UART8_RX         (pin_PD03)
// REPL is routed via SEGGER RTT, not UART (see MICROPY_HW_USE_RTT_REPL).
// Keeping these defines here so people who want to repurpose UART_REPL via
// the Pmod USBUART path can flip MICROPY_HW_USE_RTT_REPL=0 and pick a UART.
#define MICROPY_HW_UART_REPL        HW_UART_8
#define MICROPY_HW_UART_REPL_BAUD   115200

// I2C pin assignments (per ra_i2c.c RA8P1 pin table — extracted from FSP cfg)
#define MICROPY_HW_I2C0_SCL         (pin_P408)
#define MICROPY_HW_I2C0_SDA         (pin_P407)
#define MICROPY_HW_I2C1_SCL         (pin_P512)
#define MICROPY_HW_I2C1_SDA         (pin_P511)
#define MICROPY_HW_I2C2_SCL         (pin_P515)
#define MICROPY_HW_I2C2_SDA         (pin_P514)

// SPI (SPI0 on PMOD A)
#define MICROPY_HW_SPI0_SSL         (pin_P103)
#define MICROPY_HW_SPI0_RSPCK       (pin_P102)
#define MICROPY_HW_SPI0_MISO        (pin_P100)
#define MICROPY_HW_SPI0_MOSI        (pin_P101)

// Switch (USER SW on EK-RA8P1)
#define MICROPY_HW_HAS_SWITCH       (1)
#define MICROPY_HW_USRSW_PIN        (pin_P415)
#define MICROPY_HW_USRSW_PULL       (MP_HAL_PIN_PULL_NONE)
#define MICROPY_HW_USRSW_EXTI_MODE  (MP_HAL_PIN_TRIGGER_FALLING)
#define MICROPY_HW_USRSW_PRESSED    (0)

// LEDs
#define MICROPY_HW_LED1             (pin_P600)
#define MICROPY_HW_LED2             (pin_P303)
#define MICROPY_HW_LED3             (pin_PA07)
#define MICROPY_HW_LED_ON(pin)      mp_hal_pin_high(pin)
#define MICROPY_HW_LED_OFF(pin)    mp_hal_pin_low(pin)
#define MICROPY_HW_LED_TOGGLE(pin) mp_hal_pin_toggle(pin)

#define MICROPY_RA8P1_REPL_ATTACH_PROBE (1)

#ifndef MICROPY_RA8P1_SAFE_BOOT
#define MICROPY_RA8P1_SAFE_BOOT (1)
#endif

#ifndef MICROPY_RA8P1_SAFE_BOOT_CLOCKS
#define MICROPY_RA8P1_SAFE_BOOT_CLOCKS (0)
#endif

#ifndef MICROPY_RA8P1_SAFE_BOOT_SDRAM
#define MICROPY_RA8P1_SAFE_BOOT_SDRAM (0)
#endif

#ifndef MICROPY_RA8P1_SAFE_BOOT_SDRAM_HEAP
#define MICROPY_RA8P1_SAFE_BOOT_SDRAM_HEAP (0)
#endif

/* Reserve 64KB at top of RAM for C stack during RA8P1 bring-up. */
#define MICROPY_HEAP_END ((char *)(&_estack) - 0x10000)
