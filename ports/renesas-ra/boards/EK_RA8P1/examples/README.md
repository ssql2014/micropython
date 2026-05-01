# EK-RA8P1 Python examples

Per-peripheral smoke tests. Copy any of these to the board and run from REPL:

```
>>> exec(open("blink_pwm.py").read())
```

| File | Peripheral | Status |
|---|---|---|
| `blink_pwm.py`     | GPT/PWM (4 channels) | ready, runtime-blocked on REPL |
| `i2c_scan.py`      | I2C0 PMOD            | ready, runtime-blocked on REPL |
| `spi_loopback.py`  | SPI0 PMOD-A          | ready, runtime-blocked on REPL |
| `rtc_set_get.py`   | RTC                  | ready, runtime-blocked on REPL |
| `usrsw_led.py`     | GPIO + switch        | ready, runtime-blocked on REPL |
| `display_demo.py`  | GLCDC (`ra8p1_display`) | ready, runtime-blocked on REPL |

"Runtime-blocked on REPL" means: the C code paths are built clean, but the
serial REPL doesn't currently emit characters (see the README in the parent
directory for the open MSTP/TrustZone issue). Once REPL works these scripts
should run as-is.
