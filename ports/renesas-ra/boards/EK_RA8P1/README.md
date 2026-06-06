# MicroPython port — Renesas EK-RA8P1

MicroPython running on the Renesas **EK-RA8P1** evaluation kit
(MCU: `R7KA8P1KFLCAC`, Cortex-M85 @ 240 MHz, 1 MB SRAM, 2 MB internal flash,
64 MB external SDRAM, 1024×600 LCD).

**Status: interactive REPL working. All machine peripherals implemented.**

---

## Peripheral support

| Peripheral | API | Status |
|---|---|---|
| GPIO | `machine.Pin` | ✅ runtime validated |
| PWM | `machine.PWM` | ✅ runtime validated |
| I2C | `machine.I2C` | ✅ runtime validated (IIC0, P408/P407) |
| SPI | `machine.SPI` | ✅ runtime validated (PMOD-A) |
| RTC | `machine.RTC` | ✅ runtime validated |
| UART | via RTT REPL | ✅ full interactive REPL |
| ADC | `machine.ADC` | ✅ compiled, awaiting board runtime test |
| DAC | `machine.DAC` | ✅ compiled, awaiting board runtime test |
| CAN/CANFD | `machine.CAN` | ✅ compiled, awaiting board runtime test |
| Display | `ra8p1_display` module | ✅ runtime validated (1024×600 XRGB8888) |
| CEU/DVP camera | `ra8p1_ceu` module | ✅ gated bring-up, OV5640 single-frame capture validated |

> **Note:** The on-board J-Link OB CDC/VCOM bridge delivers no bytes to the
> host. This is a board-level hardware issue (confirmed with Renesas factory
> firmware too). The REPL is routed over **SEGGER RTT** via SWD instead.

---

## Quick start

### 1. Prerequisites

```sh
# FSP v6.4.0 — included as lib/fsp submodule (already at correct version)
git submodule update --init lib/fsp

# ARM toolchain (macOS)
brew install --cask gcc-arm-embedded   # gcc-arm 15.2.0
```

### 2. Build

```sh
cd ports/renesas-ra
make BOARD=EK_RA8P1 USE_FSP_QSPI=0 -j8
# → build-EK_RA8P1/firmware.{elf,bin,hex}
```

### 3. Flash

```sh
JLINK=/path/to/JLinkExe   # e.g. /Applications/SEGGER/JLink_V938a/JLinkExe
OSPI_SCRIPT=/path/to/RA8x1_Reset_OSPI.JLinkScript   # from FSP examples package

cat > /tmp/flash.jlink <<'EOF'
loadfile build-EK_RA8P1/firmware.bin 0x02000000
r
g
qc
EOF

"$JLINK" -device R7KA8P1KF_CPU0 -if SWD -speed 4000 -autoconnect 1 \
    -JLinkScriptFile "$OSPI_SCRIPT" \
    -CommanderScript /tmp/flash.jlink
```

> **Critical:** the device name must be `R7KA8P1KF_CPU0` (the core name).
> Using the package code `R7KA8P1KFLCAC` causes J-Link to hang ~10 minutes
> on OSPI region probing. The `-JLinkScriptFile` OSPI reset script is also
> required.

### 4. Connect to REPL

```sh
# requires: pip install pylink-square
python3 tools/rtt_terminal.py
```

Or use any SEGGER RTT Viewer (J-Link RTT Viewer, Ozone, etc.).

---

## Pin map

| Function | Pin |
|---|---|
| LED1 (Blue) | P600 |
| LED2 (Green) | P303 |
| LED3 (Red) | PA07 |
| User switch | P415 (active low) |
| I2C0 SCL | P408 |
| I2C0 SDA | P407 |
| PMOD-A SPI SSL | P103 |
| PMOD-A SPI RSPCK | P102 |
| PMOD-A SPI MISO | P100 |
| PMOD-A SPI MOSI | P101 |
| CANFD CTX0 | P401 |
| CANFD CRX0 | P402 |
| ADC ch0 (AN000) | P000 |
| DAC ch0 | P014 |
| DAC ch1 | P015 |

---

## Examples

See [`examples/`](examples/) for per-peripheral Python scripts:

| Script | Tests |
|---|---|
| `blink_pwm.py` | PWM on 4 channels |
| `i2c_scan.py` | I2C bus scan |
| `spi_loopback.py` | SPI write/readinto loopback |
| `rtc_set_get.py` | RTC set and read-back |
| `usrsw_led.py` | GPIO + user switch |
| `adc_read.py` | ADC_B multi-channel read |
| `dac_write.py` | DAC_B write + readback via ADC |
| `canfd_loopback.py` | CANFD send/recv |
| `display_demo.py` | GLCDC framebuffer paint |

---

## CEU/DVP camera bring-up

The EK-RA8P1 CEU driver is currently a gated board bring-up module. It is not
compiled into the default image unless `RA8P1_BRINGUP_CEU_TEST=1` is set.

```sh
cd ports/renesas-ra
make BOARD=EK_RA8P1 BUILD=build-EK_RA8P1-ceu-fullclocks \
    RA8P1_BRINGUP_CEU_TEST=1 \
    RA8P1_SAFE_BOOT_CLOCKS=1 \
    RA8P1_SAFE_BOOT_SDRAM=1 \
    RA8P1_SAFE_BOOT_SDRAM_HEAP=0 \
    -j8
```

Hardware route for CEU/DVP:

| Item | Required state |
|---|---|
| Camera connector | OV5640 module on `J35` |
| `J41` | Open/removed |
| U15 / PI4IOE5V6408, I2C `0x43` | bit5 driven output-low: `DIR=0x20`, `OUT=0x00`, `OE=0xdf` |

`ra8p1_ceu.init()`, `ra8p1_ceu.camera_open()`, and `ra8p1_ceu.capture()`
select the CEU route in software before touching the camera. This matters
after MIPI CSI/DSI tests: the physical `SW4-6` switch can be overridden when
U15 bit5 is configured as an enabled output.

The CEU/DVP route shares several MCU pins with the parallel LCD path
(`P09_02` and `P11_02`..`P11_04`). Continuous `ra8p1_ceu.live()` capture can
therefore corrupt the current parallel GLCDC display output while CEU is using
those pins. Use `ra8p1_ceu.snapshot_display()` for a low-rate proof that
captures one frame, stops CEU/XCLK, restores the shared pins to
`LCD_GRAPHICS`, and then flips the framebuffer. True simultaneous camera
preview needs a display route that does not share the CEU DVP pins, such as
the MIPI-DSI display path.

Minimal runtime smoke:

```python
import ra8p1_ceu as ceu

print(ceu.camera_id())       # expected: (86, 64)
c = ceu.capture(3000)
print(c["last_capture_start_err"], c["capture_ready"], c["sample_nonzero"])
ceu.snapshot_display(3000)   # one-frame display proof, not realtime preview
b = ceu.buffer()
print(len(b), b[0], b[1])    # expected length: 614400 bytes
```

Validated board result from the current bring-up:

```text
SMOKE12_ID (86, 64)
SMOKE12_SW 0 32 0 223 0
SMOKE12_CAP 0 1 8 16777216 3079 2453830625 0
SMOKE12_BUF 614400 [0, 252, 0, 252, 0, 252, 0, 252]
```

---

## Key bring-up notes

Six RA8P1-specific issues were resolved during bring-up. Reverting any of
these will break the port:

1. **HOCO-sourced PLLs** — the 24 MHz external crystal does not stabilise;
   `bsp_clock_init` hangs on `MOSCSF`. Switched to HOCO 48 MHz with
   recalculated PLL multipliers in `ra_gen/bsp_clock_cfg.h`.

2. **Runtime MSP for stack top** — FSP startup leaves MSP far below the
   linker `_estack`. Passing `_estack` to `mp_cstack_init_with_top` triggers
   a RuntimeError reset loop. Fixed to read `__get_MSP()` at runtime.

3. **GC heap in SDRAM** — `gc_init` faults internal SRAM above ~128 KB
   (cause not fully understood; likely IDAU/MPU boundary near 0x22020000).
   Heap uses 2 MB of SDRAM at `0x68500000..0x68700000`.

4. **SEGGER RTT REPL** — J-Link OB CDC bridge is non-functional on this
   board variant. REPL routed through RTT (`MICROPY_HW_USE_RTT_REPL=1`).

5. **Custom setjmp** — Homebrew gcc-arm 15.2 ships without newlib.
   `stub_inc/setjmp.h` + `setjmp_arm.S` provide the minimal ARM EABI
   implementation needed by `MICROPY_NLR_SETJMP=1`.

6. **IIC0 interrupt vectors** — `vector_data.h/c` originally contained only
   GLCDC in the ICU table. IIC0 RXI/TXI/TEI/ERI ISRs added at IRQn 1–4;
   `ra_i2c.c` timeout path now issues a stop condition (`ICCR2_b.SP=1`)
   to release the bus before returning, preventing a 100k-iteration BBSY
   spin-loop on the next I2C operation.

---

## FSP submodule

`lib/fsp` is pinned to **Renesas FSP v6.4.0** — the first FSP release with
RA8P1 BSP support. Upstream MicroPython master uses FSP v6.2.0 which predates
RA8P1. Building `BOARD=EK_RA8P1` against upstream master without advancing
this submodule will fail with `bsp_override.h: No such file or directory`.

---

## Voltage-scaling fix (bsp_clocks.c)

FSP's `bsp_prv_clock_init_pre` had a `&& 0` guard that silently skipped the
MRAM voltage-scaling step (VSCM setup). On RA8P1 this causes `bsp_clocks.c`
to configure MRAM for high-speed without first raising the supply voltage,
which can result in MRAM read errors at 240 MHz. Fixed in
`lib/fsp/ra/fsp/src/bsp/mcu/all/bsp_clocks.c`: the guard is replaced with
`R_BSP_SoftwareDelay(50, BSP_DELAY_UNITS_MICROSECONDS)` (voltage transition
takes ~3 µs per datasheet; 50 µs is a safe bound).
