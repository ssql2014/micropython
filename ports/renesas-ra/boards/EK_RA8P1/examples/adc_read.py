# Read ADC channels on EK-RA8P1.
# RA8P1 uses the ADC_B dual-converter peripheral (r_adc_b FSP driver).
# Converter 0 handles AN000-AN013 (pins P000-P010, P014-P015).
# Converter 1 handles AN116-AN128 (pins P500-P508, P800-P803).
#
# The PMOD-A header exposes AN000 (P000) conveniently for bench testing.

from machine import ADC, Pin

def read_channel(pin_name):
    adc = ADC(Pin(pin_name))
    raw = adc.read_u16()          # 0–65535, scaled from 12-bit result
    volts = raw * 3.3 / 65535
    return raw, volts

def main():
    # Single-channel read on P000 (AN000, converter 0)
    raw, v = read_channel('P000')
    print("P000 (AN000): raw={:5d}  {:.3f} V".format(raw, v))

    # Read several converter-0 channels
    for pin in ('P001', 'P002', 'P003'):
        raw, v = read_channel(pin)
        print("{}:       raw={:5d}  {:.3f} V".format(pin, raw, v))

    # Converter-1 channel (P500 = AN116)
    raw, v = read_channel('P500')
    print("P500 (AN116): raw={:5d}  {:.3f} V".format(raw, v))

if __name__ == "__main__":
    main()
