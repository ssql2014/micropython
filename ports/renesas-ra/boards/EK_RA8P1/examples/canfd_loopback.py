# CANFD loopback test on EK-RA8P1.
# Requires a CAN transceiver connected to the CANFD pins and either:
#   - a second node on the bus, or
#   - CANTX shorted to CANRX for single-wire loopback (check transceiver datasheet).
#
# EK-RA8P1 CANFD0 pins: CTX0=P401, CRX0=P402 (verify against board schematic).
# Nominal bit rate: 1 Mbit/s. No bit-rate switching (FD frames at same rate).

from machine import CAN
import time

def main():
    can = CAN(0, baudrate=1_000_000)
    print(can)  # CAN(0, baudrate=1000000)

    # Send a standard 8-byte frame
    payload = bytes([0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08])
    print("sending id=0x123 data={}".format(payload.hex()))
    can.send(payload, 0x123, timeout=500)

    # Try to receive (requires loopback or a second node echoing)
    try:
        data, id_, ext, fd = can.recv(timeout=500)
        print("recv id=0x{:x} ext={} fd={} data={}".format(
            id_, ext, fd, data.hex()))
    except OSError:
        print("recv timeout — no loopback or second node present")

    # Extended-ID frame
    can.send(b'\xde\xad\xbe\xef', 0x12345678, ext=True, timeout=500)
    print("sent extended id=0x12345678")

    # CANFD frame (up to 64 bytes, same bit rate)
    can.send(bytes(range(16)), 0x7FF, fd=True, timeout=500)
    print("sent FD frame 16 bytes id=0x7FF")

    can.deinit()
    print("done")

if __name__ == "__main__":
    main()
