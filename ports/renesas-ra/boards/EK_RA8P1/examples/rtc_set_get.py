# Set and read back the on-chip RTC on EK-RA8P1.
# RTC source on this board is the main clock (MICROPY_HW_RTC_SOURCE=1).

from machine import RTC
import time

def main():
    rtc = RTC()
    rtc.datetime((2026, 4, 30, 4, 9, 0, 0, 0))
    for _ in range(5):
        print(rtc.datetime())
        time.sleep(1)

if __name__ == "__main__":
    main()
