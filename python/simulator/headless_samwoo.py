#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""콘솔 삼우 슬레이브. GUI 없이 한 포트만."""
import argparse
import os
import sys
import time

import serial

_PY = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
if _PY not in sys.path:
    sys.path.insert(0, _PY)

from samwoo_proto import QTY, RS485_BAUD, build_response, lrc8, make_defaults  # noqa: E402


def handle(ser, frame, regs, stats, slave, start1_is_first, length_mode):
    if len(frame) < 8 or frame[0] != 0x3A or frame[-2] != 0x0D or frame[-1] != 0x0A:
        stats["bad"] += 1
        return
    payload = frame[1:-2]
    body, got = payload[:-1], payload[-1]
    if lrc8(body) != got or len(body) < 6:
        stats["bad"] += 1
        return
    sid, fc = body[0], body[1]
    start = (body[2] << 8) | body[3]
    qty = (body[4] << 8) | body[5]
    if sid != slave or fc not in (3, 4) or qty < 1:
        stats["bad"] += 1
        return
    idx = start - 1 if (start1_is_first and start >= 1) else start
    values = []
    for i in range(qty):
        a = idx + i
        values.append(regs[a] if 0 <= a < QTY else 0)
    ser.write(build_response(sid, fc, values, length_mode=length_mode))
    ser.flush()
    stats["ok"] += 1
    if stats["ok"] <= 3 or stats["ok"] % 10 == 0:
        print("RX ok=%d bad=%d fc=%d start=%d idx=%d qty=%d" % (
            stats["ok"], stats["bad"], fc, start, idx, qty), flush=True)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("port", nargs="?", default="COM4")
    ap.add_argument("soc", nargs="?", type=int, default=85)
    ap.add_argument("--baud", type=int, default=RS485_BAUD)
    ap.add_argument("--slave", type=int, default=1)
    ap.add_argument("--len-mode", choices=("bytes", "regs"), default="regs")
    ap.add_argument("--start1-first", dest="start1_first", action="store_true")
    ap.add_argument("--no-start1-first", dest="start1_first", action="store_false")
    ap.set_defaults(start1_first=True)
    args = ap.parse_args()
    regs = make_defaults(0)
    regs[2] = args.soc
    ser = serial.Serial(args.port, args.baud, timeout=0.05)
    stats = {"ok": 0, "bad": 0}
    print("SAMWOO_SLAVE_READY port=%s baud=%s slave=%s SOC=%s len=%s start1=%s" % (
        args.port, args.baud, args.slave, args.soc, args.len_mode, args.start1_first), flush=True)
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
                handle(ser, frame, regs, stats, args.slave, args.start1_first, args.len_mode)
        else:
            time.sleep(0.002)


if __name__ == "__main__":
    main()
