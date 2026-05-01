# MicroPython port — Renesas EK-RA8P1

This board target builds MicroPython for the Renesas **EK-RA8P1** evaluation kit
(MCU: `R7KA8P1KFLCAC`, Cortex-M85 primary core; the Cortex-M33 secondary core
is unused).

**Status: working interactive REPL.** Six peripherals validated live:
GPIO, PWM, I2C (instantiation), SPI (instantiation), RTC (set/get with tick
verified), and a custom GLCDC framebuffer module (`ra8p1_display`). REPL is
routed via SEGGER RTT over SWD because the on-board J-Link OB CDC bridge does
not deliver bytes to the host VCOM (confirmed against stock Renesas factory
firmware — not a software issue).

---

## Hardware

- Board: EK-RA8P1, FSP 6.4.0 BSP, J-Link OB on USB-C (provides SWD only —
  the CDC VCOM does **not** work on this board, see Quirks below).
- MCU: `R7KA8P1KFLCAC` — RA8P1-specific code is gated on
  `defined(BSP_MCU_R7KA8P1KFLCAC)`.

## Toolchain

| Component | Version | Source |
|---|---|---|
| FSP | 6.4.0 | e²studio installer |
| GCC | gcc-arm 15.2.0 (Homebrew) | `brew install --cask gcc-arm-embedded` |
| Bundled GCC | gcc-arm 13.2.1 | inside e²studio install (only used for setjmp.h reference) |

Homebrew's gcc-arm 15.2.0 ships **without newlib**, so `<setjmp.h>` is missing.
This port provides `stub_inc/setjmp.h` + `setjmp_arm.S` (minimal ARM setjmp
implementation) so `MICROPY_NLR_SETJMP=1` compiles cleanly.

## Build

From `ports/renesas-ra/`:

```sh
make BOARD=EK_RA8P1 USE_FSP_QSPI=0 -j8
```

`USE_FSP_QSPI=0` is required: the QSPI flash driver paths are not yet
RA8P1-aware. The Makefile filters `ra_adc.c`, `ra_dac.c`, `ra_flash.c`,
`ra_icu.c` out of the RA8P1 build.

## Flashing

```sh
JLINK=/Users/alex/jlink_v938a_extract/Applications/SEGGER/JLink_V938a/JLinkExe
OSPI_SCRIPT="/Users/alex/ra-fsp-examples/example_projects/ek_ra8p1/_quickstart/quickstart_ek_ra8p1_ep/e2studio/script/RA8x1_Reset_OSPI.JLinkScript"
"$JLINK" -device R7KA8P1KF_CPU0 -if SWD -speed 4000 -autoconnect 1 \
    -JLinkScriptFile "$OSPI_SCRIPT" -CommanderScript /tmp/flash_mpy_rtt.jlink
```

`/tmp/flash_mpy_rtt.jlink`:
```
loadfile /Users/alex/micropython/ports/renesas-ra/build-EK_RA8P1/firmware.bin 0x02000000
r
g
qc
```

**Critical:** use `R_7KA8P1KF_CPU0` as the J-Link device name — NOT
`R7KA8P1KFLCAC` (the package code). The package name causes JLink V9.38a to
hang ~10+ minutes on OSPI flash region writes. Also `-JLinkScriptFile` with
the OSPI reset script is required for the OSPI bank.

## Driving the REPL (host side)

```sh
python3 /Users/alex/ek-ra8p1-handoff/scripts/rtt_terminal.py
```

This is an interactive terminal that uses pylink-square to keep a J-Link
session open and drains/feeds the SEGGER RTT control block. Ctrl-] to quit.

## Pin map (board fixed peripherals)

| Function | Pin(s) |
|---|---|
| LED1 (Blue)   | P600 (also `GTIOC6B`) |
| LED2 (Green)  | P303 (also `GTIOC7B`) |
| LED3 (Red)    | PA07 (also `GTIOC7A`) |
| User switch   | P415 (active low) |
| I2C0 (PMOD)   | SCL=P408, SDA=P407 |
| PMOD-A SPI0   | SSL=P103, RSPCK=P102, MISO=P100, MOSI=P101 |
| (UART REPL)   | not used — see Quirks |

## Peripheral status

| Peripheral | Status | Validation |
|---|---|---|
| GPIO   | working | `Pin('P600', Pin.OUT).value(1)` toggles LED1 |
| RTC    | working | `rtc.datetime()` ticks correctly |
| PWM    | working | `PWM(Pin('P600'))` + `freq()`/`duty_u16()` ; `repr` shows `GTIOC 6B[#96], active=1` |
| I2C    | instantiation OK | `I2C(0, freq=100000)` ; `repr` shows `scl=P408, sda=P407` ; `i2c.scan()` will lock chip if no device on bus |
| SPI    | instantiation OK | `SPI(0, baudrate=1000000)` ; correct pin map in repr ; loopback test pending |
| GLCDC  | C HAL up; Python module exposed | `ra8p1_display` module: `init()`, `framebuffer(idx)`, `flip(idx)`, `fill(idx, color)`, `pixel(idx, x, y, color)`, plus constants `WIDTH=1024, HEIGHT=600, STRIDE=4096, BPP=32, FORMAT='XRGB8888'` |
| UART (RTT REPL) | working | Interactive REPL via SWD; banner + eval confirmed |
| ADC, DAC, CANFD | not yet ported | needs FSP regen for module instances |
| Internal flash, ESWM Ethernet, USB FS/HS, MIPI camera/display | not started | — |

## Quirks specific to this board

### J-Link OB CDC VCOM does not work
On this EK-RA8P1, the J-Link OB CDC bridge does NOT deliver bytes to the host
serial port. We confirmed by flashing Renesas's own factory `quickstart_ek_ra8p1_ep`
firmware: it also produces 0 bytes on `/dev/cu.usbmodem*`, despite the chip
emitting bytes correctly out of SCI8 (verified via JTAG: TDRE/TEND set, MSTPCRB
SCI8 bit clear). The fault is in the J-Link OB firmware or board-level
GreenPAK CPLD routing — not in our code.

**Workaround**: REPL routed through SEGGER RTT (SWD-based, no UART chain).
`MICROPY_HW_USE_RTT_REPL=1` enables this in `mpconfigboard.h`.

### Six bring-up fixes that must NOT be reverted

1. **PLLs sourced from HOCO, not MAIN_OSC** in `ra_gen/bsp_clock_cfg.h`.
   The 24 MHz external crystal does not stabilize; `bsp_clock_init` hangs
   forever on `OSCSF.MOSCSF` (bit 3) wait. Switched to HOCO 48 MHz with
   recomputed PLL multipliers (PLL1 x250→x125, PLL2 x300→x150) so output
   stays at 2000/2400 MHz.

2. **`mp_cstack_init_with_top` uses `__get_MSP()` at runtime, not `_estack`**
   in `main.c`. FSP startup leaves MSP at the bootstrap stack near 0x22002000;
   passing the linker `_estack` (0x22100000) makes every `mp_cstack_check`
   raise RuntimeError, locking the REPL into a fatal-exception loop.

3. **GC heap in SDRAM at 0x68500000..0x68700000** (2 MB).
   Internal SRAM `gc_init` faults at heaps > ~128 KB through some pattern
   that's still not understood (`mem32[]` writes to those addresses succeed
   from CPU at runtime, ruling out a simple attribution boundary). SDRAM
   works fine, leaving 6 MB SDRAM for the framebuffer + future use.

4. **`MICROPY_HW_USE_RTT_REPL=1`** in `mpconfigboard.h` and
   `mphalport.c` routes `mp_hal_stdout_tx_strn` / `mp_hal_stdin_rx_chr`
   through SEGGER RTT. Files in `lib/SEGGER_RTT/` are vendored from a
   Renesas EK-RA8D1 example.

5. **`MICROPY_NLR_SETJMP=1`** with custom `setjmp_arm.S`. Homebrew gcc-arm
   ships without newlib so the standard setjmp is unavailable. The custom
   implementation saves r4-r11, sp, lr (ARM EABI standard).

6. **`MICROPY_PY_MACHINE_I2C/SPI/PWM=1` + I2C pin defines** in
   `mpconfigboard.h`. Without these, `from machine import I2C` raises
   ImportError even though the type is compiled in.

## Files of interest

- `mpconfigboard.h` — board pin assignments + the six bring-up flags above
- `mpconfigboard.mk` — build flags + `RA8P1_BRINGUP_DISPLAY_TEST` gate
- `ra_gen/bsp_clock_cfg.h` — HOCO PLL config
- `ra_gen/{common_data,hal_data,pin_data,vector_data}.{c,h}` — FSP-generated
- `ra_cfg/` — FSP-generated module configs
- `ra8p1_ek.ld` — linker script (heap split, 64KB top-of-RAM C stack)
- `examples/` — Python smoke tests for each peripheral

## Resumption / future work

- **ADC/DAC/CANFD**: blocked on FSP regen to add `g_adc`, `g_dac`,
  `g_canfd` instances. Two paths: open `configuration.xml` in e²studio +
  click "Generate Project Content", or write direct register drivers using
  the user manual + `R7KA8P1KF_core0.h` device header.
- **Hardware loopback validation**: PWM blink visual, SPI MOSI↔MISO
  loopback (`spi.write_readinto`), I2C scan with actual device, display
  Python pixel paint.
- **Heap size investigation**: figure out why `gc_init` faults internal
  SRAM > 128 KB. SRAM is ~5x faster than SDRAM, so this is performance
  work — not a blocker.
- **Strip build artifact `.bak-*` files** in working tree.
- **Loose-end fixes**: `i2c.scan()` should time out gracefully on empty bus
  (currently locks the chip).

## See also

- `/Users/alex/ek-ra8p1-handoff/melissa-spec-porting-plan.md` — full porting
  scope, task tracking, and per-peripheral effort estimates.
- `/Users/alex/ek-ra8p1-handoff/scripts/{ci_build_ra8p1,flash_ra8p1,rtt_terminal}.{sh,py}` — convenience scripts
- `/Users/alex/ek-ra8p1-handoff/ra8p1-gtioc-pin-table.txt` — extracted GPT pin
  table used by `ra/ra_gpt.c`.
