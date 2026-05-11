# SPI loopback test on PMOD-A header of EK-RA8P1.
# Short MISO (P100) to MOSI (P101) on the PMOD-A connector and run.
# Successful run prints "loopback OK".

from machine import SPI

PATTERN = bytes(range(64))

def main():
    spi = SPI(0, baudrate=1_000_000, polarity=0, phase=0)
    rx = bytearray(len(PATTERN))
    spi.write_readinto(PATTERN, rx)
    if rx == PATTERN:
        print("loopback OK ({} bytes)".format(len(PATTERN)))
    else:
        print("loopback MISMATCH")
        print("  tx:", PATTERN.hex())
        print("  rx:", bytes(rx).hex())

if __name__ == "__main__":
    main()
