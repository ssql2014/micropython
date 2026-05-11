# MicroPython — Renesas EK-RA8P1 Port

This is a fork of [micropython/micropython](https://github.com/micropython/micropython)
adding a full MicroPython port for the **Renesas EK-RA8P1** evaluation kit
(R7FA8P1AH, Cortex-M85 @ 240 MHz).

The port lives on the `ek-ra8p1-port` branch and is merged into `master`.

## What's added

- **Board definition**: `ports/renesas-ra/boards/EK_RA8P1/`
- **RA8P1-specific drivers** (in `ports/renesas-ra/ra/`):
  - `ra8p1_adc.c` — ADC_B dual-converter (`machine.ADC`)
  - `ra8p1_dac.c` — DAC_B (`machine.DAC`)
  - `ra8p1_canfd.c` — CANFD polling driver (`machine.CAN`)
- **SEGGER RTT REPL** — `lib/SEGGER_RTT/` (J-Link OB CDC bridge non-functional on this board)
- **machine.CAN** — `ports/renesas-ra/machine_can.c`
- **GLCDC display module** — `ports/renesas-ra/ra8p1_display.c` (`ra8p1_display` Python module, 1024×600 XRGB8888)
- **FSP v6.4.0** — `lib/fsp` submodule bumped from v6.2.0 (upstream) to v6.4.0 (first release with RA8P1 BSP)
- **newlib shim** — `stub_inc/` + `setjmp_arm.S` for gcc-arm 15.x which ships without newlib

## Peripheral status

| Peripheral | Status |
|---|---|
| GPIO, PWM, I2C, SPI, RTC, UART (RTT), Display | ✅ runtime validated |
| ADC (ADC_B), DAC (DAC_B), CANFD | ✅ compiled, pending hardware test |

## Getting started

See [`ports/renesas-ra/boards/EK_RA8P1/README.md`](ports/renesas-ra/boards/EK_RA8P1/README.md)
for build, flash, and REPL connection instructions.

## Companion repo

[ssql2014/ek-ra8p1-handoff](https://github.com/ssql2014/ek-ra8p1-handoff) —
CI build script, flash helper, RTT terminal, and porting notes.
