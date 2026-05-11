# Mirror the user switch (P415) onto LED1 (P600) on EK-RA8P1.
# Press the switch — LED follows.

from machine import Pin
import time

def main():
    sw = Pin("P415", Pin.IN, Pin.PULL_UP)
    led = Pin("P600", Pin.OUT)
    while True:
        led.value(0 if sw.value() else 1)  # switch is active-low
        time.sleep_ms(10)

if __name__ == "__main__":
    main()
