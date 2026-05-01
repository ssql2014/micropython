# Scan the PMOD I2C bus on EK-RA8P1 and print any device addresses found.
# Pins (from FSP-generated config): default IIC0 — SCL=P408, SDA=P407.
# Adjust id/scl/sda if you've routed I2C through a different IIC unit.

from machine import I2C, Pin

def main():
    i2c = I2C(0, freq=100_000)
    devices = i2c.scan()
    if not devices:
        print("no I2C devices found")
        return
    print("found {} device(s):".format(len(devices)))
    for addr in devices:
        print("  0x{:02x}".format(addr))

if __name__ == "__main__":
    main()
