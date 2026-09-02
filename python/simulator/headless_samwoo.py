#!/usr/bin/env python
# Headless Samwoo slave (STX 0x3A + LRC + CR LF) for COM4 lab test.
import sys
import time
import serial

PORT = sys.argv[1] if len(sys.argv) > 1 else "COM4"
SOC = int(sys.argv[2]) if len(sys.argv) > 2 else 85
BAUD = 9600
QTY = 48
SLAVE = 1

regs = [0] * QTY
regs[0] = 10
regs[1] = 1000
regs[2] = SOC
regs[3] = 98
regs[4] = 540
regs[5] = 25
regs[6] = 3400
regs[7] = 3300
regs[8] = 250
regs[9] = 240
regs[12] = 0x07
regs[16] = 16
for i in range(16):
    regs[17 + i] = 3350 + i
regs[31] = 8
for i in range(8):
    regs[32 + i] = 250


def lrc8(data):
    return ((~sum(data) & 0xFF) + 1) & 0xFF


def handle(ser, frame, stats):
    if len(frame) < 8 or frame[0] != 0x3A or frame[-2] != 0x0D or frame[-1] != 0x0A:
        stats["bad"] += 1
        return
    payload = frame[1:-2]
    body, got = payload[:-1], payload[-1]
    if lrc8(body) != got or len(body) < 6:
        stats["bad"] += 1
        return
    slave, fc = body[0], body[1]
    start = (body[2] << 8) | body[3]
    qty = (body[4] << 8) | body[5]
    if slave != SLAVE or fc not in (3, 4) or qty < 1:
        stats["bad"] += 1
        return
    data = bytearray()
    for i in range(qty):
        v = regs[start + i] if 0 <= start + i < QTY else 0
        data.append((v >> 8) & 0xFF)
        data.append(v & 0xFF)
    resp_body = bytes([slave, fc, len(data)]) + bytes(data)
    out = bytes([0x3A]) + resp_body + bytes([lrc8(resp_body), 0x0D, 0x0A])
    ser.write(out)
    ser.flush()
    stats["ok"] += 1
    if stats["ok"] <= 3 or stats["ok"] % 10 == 0:
        print("RX ok=%d bad=%d fc=%d start=%d qty=%d rsp=%d" % (
            stats["ok"], stats["bad"], fc, start, qty, len(out)), flush=True)


def main():
    ser = serial.Serial(PORT, BAUD, timeout=0.05)
    stats = {"ok": 0, "bad": 0}
    print("SAMWOO_SLAVE_READY port=%s baud=%s slave=%s SOC=%s" % (PORT, BAUD, SLAVE, SOC), flush=True)
    buf = bytearray()
    while True:
        n = ser.in_waiting
        if n:
            buf.extend(ser.read(n))
            while True:
                try:
                    stx = buf.index(0x3A)
                except ValueError:
                    buf.clear()
                    break
                if stx:
                    del buf[:stx]
                crlf = -1
                for i in range(1, len(buf) - 1):
                    if buf[i] == 0x0D and buf[i + 1] == 0x0A:
                        crlf = i
                        break
                if crlf < 0:
                    if len(buf) > 512:
                        del buf[0]
                    break
                frame = bytes(buf[: crlf + 2])
                del buf[: crlf + 2]
                handle(ser, frame, stats)
        else:
            time.sleep(0.002)


if __name__ == "__main__":
    main()
