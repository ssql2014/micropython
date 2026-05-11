# EK-RA8P1 Python examples

Per-peripheral smoke tests. Run from the RTT REPL:

```python
>>> exec(open("blink_pwm.py").read())
```

Or paste directly — the RTT terminal accepts multi-line paste.

## Peripheral status

| File | Peripheral | Build | Runtime |
|---|---|---|---|
| `blink_pwm.py`     | GPT/PWM (4 channels)        | ✅ | needs replug + flash |
| `i2c_scan.py`      | I2C0 PMOD (P408/P407)       | ✅ | needs replug + flash |
| `spi_loopback.py`  | SPI0 PMOD-A                 | ✅ | needs replug + flash |
| `rtc_set_get.py`   | RTC                         | ✅ | needs replug + flash |
| `usrsw_led.py`     | GPIO + user switch          | ✅ | needs replug + flash |
| `display_demo.py`  | GLCDC 1024×600 XRGB8888     | ✅ | needs replug + flash |
| `adc_read.py`      | ADC_B (converter 0 + 1)     | ✅ | needs replug + flash |
| `dac_write.py`     | DAC_B (ch0=P014, ch1=P015)  | ✅ | needs replug + flash |

## REPL access

The REPL runs over SEGGER RTT (not UART — the J-Link OB CDC bridge passes no
bytes). Connect with:

```sh
python3 /Users/alex/ek-ra8p1-handoff/scripts/rtt_terminal.py
```

## Flash command

```sh
JLINK=/Users/alex/jlink_v938a_extract/Applications/SEGGER/JLink_V938a/JLinkExe
OSPI=/Users/alex/ra-fsp-examples/example_projects/ek_ra8p1/_quickstart/quickstart_ek_ra8p1_ep/e2studio/script/RA8x1_Reset_OSPI.JLinkScript
cat > /tmp/flash.jlink <<'EOF'
loadfile /Users/alex/micropython/ports/renesas-ra/build-EK_RA8P1/firmware.bin 0x02000000
r
g
qc
EOF
"$JLINK" -device R7KA8P1KF_CPU0 -if SWD -speed 4000 -autoconnect 1 \
    -JLinkScriptFile "$OSPI" -CommanderScript /tmp/flash.jlink
```

**Note:** device must be `R7KA8P1KF_CPU0` (not the package code). The OSPI
script is required to avoid a 10-minute hang on the OSPI flash region.

## Known hardware note

J-Link OB on the board carries firmware from May 2021. The SEGGER V9.38a DLL
reports "Out of sync" with this older firmware. Use OpenOCD as fallback:

```sh
openocd -f interface/jlink.cfg -f /tmp/ra8p1.cfg \
  -c "program build-EK_RA8P1/firmware.bin 0x02000000 reset exit"
```
