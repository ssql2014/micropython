# Write and read back DAC channels on EK-RA8P1.
# RA8P1 uses the r_dac_b FSP driver (12-bit, two channels: DAC0=P014, DAC1=P015).
# Note: DAC output pins overlap with ADC channels AN012/AN013, so don't enable
# both ADC and DAC on the same pin simultaneously.

from machine import DAC, ADC, Pin
import time

def dac_sweep(ch):
    dac = DAC(ch)
    print("DAC{}: sweeping 0 → 4095 → 0".format(ch))
    for v in range(0, 4096, 256):
        dac.write(v)
        time.sleep_ms(10)
    for v in range(4096, -1, -256):
        dac.write(v)
        time.sleep_ms(10)
    dac.write(0)

def dac_readback(ch, pin_name):
    """Write a value, read it back via ADC on the same pin."""
    dac = DAC(ch)
    target_mv = 1650  # ~half of 3.3V rail
    dac.write_mv(target_mv)
    time.sleep_ms(1)  # settle

    adc = ADC(Pin(pin_name))
    raw = adc.read_u16()
    measured_mv = raw * 3300 // 65535
    print("DAC{} set {}mV  →  ADC reads {}mV (raw {})".format(
        ch, target_mv, measured_mv, raw))
    dac.write(0)

def main():
    dac_sweep(0)
    # DAC0 (P014 = AN012): write and read back on same pin
    # Requires P014 jumpered to an ADC input or measured with a meter.
    dac_readback(0, 'P014')

if __name__ == "__main__":
    main()
