# Fade the four PWM-capable LED pins on EK-RA8P1.
# Each LED pin doubles as a GPT GTIOC output:
#   P600 → GTIOC6B (LED1 Blue)
#   P303 → GTIOC7B (LED2 Green)
#   PA07 → GTIOC7A (LED3 Red)
#   P302 → GTIOC4A (free pin / scope-friendly)

import time
from machine import PWM, Pin

PINS = ["P600", "P303", "PA07", "P302"]

def main(freq_hz=1000, step=0.02):
    chans = [PWM(Pin(p), freq=freq_hz) for p in PINS]
    try:
        while True:
            for d in [i * step for i in range(int(1 / step) + 1)]:
                for c in chans:
                    c.duty_u16(int(d * 65535))
                time.sleep_ms(10)
            for d in [(1 - i * step) for i in range(int(1 / step) + 1)]:
                for c in chans:
                    c.duty_u16(int(d * 65535))
                time.sleep_ms(10)
    finally:
        for c in chans:
            c.deinit()

if __name__ == "__main__":
    main()
