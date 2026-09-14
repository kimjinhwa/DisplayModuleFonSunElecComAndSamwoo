#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""내일 실팩 RS-485 점검. PC가 마스터.

디스플레이는 버스에서 빼고, USB-RS485만 팩에 연결한 뒤 실행한다.
팩이 응답하면 엑셀 / 펌웨어 / 시뮬레이터 어디가 틀린지 리포트를 남긴다.

  python python/samwoo_lab.py
  python python/samwoo_lab.py --port COM14
  python python/samwoo_lab.py --port COM14 --baud 19200
"""
from __future__ import print_function

import argparse
import os
import sys
import time
from datetime import datetime

from samwoo_proto import (
    DOC_TX_HEX,
    QTY,
    RS485_BAUD,
    build_request,
    format_reg_table,
    hex_spaces,
    length_verdict,
    parse_frame,
    score_maps,
    split_frame,
)

HERE = os.path.dirname(os.path.abspath(__file__))
LOG_ROOT = os.path.join(HERE, "lab_logs")


def list_ports():
    try:
        from serial.tools.list_ports import comports
    except ImportError:
        print("pip install pyserial", file=sys.stderr)
        return []
    return list(comports())


def read_until_frame(ser, timeout):
    deadline = time.time() + timeout
    buf = bytearray()
    while time.time() < deadline:
        n = ser.in_waiting
        chunk = ser.read(n or 1)
        if chunk:
            buf.extend(chunk)
            frame, rest = split_frame(bytes(buf))
            if frame is not None:
                return frame, rest
        else:
            time.sleep(0.01)
    return None, bytes(buf)


def send_case(ser, name, frame, timeout):
    ser.reset_input_buffer()
    time.sleep(0.03)
    ser.write(frame)
    ser.flush()
    got, leftover = read_until_frame(ser, timeout)
    row = {
        "name": name,
        "tx": bytes(frame),
        "rx": got,
        "leftover": leftover,
    }
    if got:
        row["parsed"] = parse_frame(got)
    elif leftover:
        row["parsed"] = parse_frame(leftover) if leftover[:1] == b"\x3a" else {
            "ok": False,
            "error": "프레임 아님",
            "hex": hex_spaces(leftover),
            "raw": leftover,
        }
    else:
        row["parsed"] = {"ok": False, "error": "무응답", "hex": "", "raw": b""}
    return row


def cases_for(slave):
    return [
        ("문서·펌웨어 TX start=1 qty=48 FC04", build_request(slave, 0x04, 1, QTY)),
        ("짧은 읽기 start=1 qty=1", build_request(slave, 0x04, 1, 1)),
        ("FC03 start=1 qty=48 (표준 Holding)", build_request(slave, 0x03, 1, QTY)),
        ("ASCII Modbus (:0104... 문자)", b":010400010030CA\r\n"),
    ]


def write_log(path, text):
    with open(path, "a", encoding="utf-8") as f:
        f.write(text)
        if not text.endswith("\n"):
            f.write("\n")


def print_and_log(log_path, text):
    print(text)
    write_log(log_path, text)


def analyze_ok_rows(ok_rows):
    lines = ["", "======== 판정 (실팩 기준) ========", ""]
    bauds = sorted(set(r["baud"] for r in ok_rows))
    slaves = sorted(set(r["slave"] for r in ok_rows))
    names = [r["name"] for r in ok_rows]
    lines.append("응답 보드레이트: %s" % ", ".join(str(b) for b in bauds))
    lines.append("응답 슬레이브: %s" % ", ".join(str(s) for s in slaves))
    lines.append("응답한 요청: %s" % ", ".join(names))
    lines.append("")

    lengths = []
    best_maps = []
    for r in ok_rows:
        p = r["parsed"]
        if p.get("kind") != "response":
            continue
        lengths.append(p.get("length"))
        for note in length_verdict(p):
            lines.append("- %s / slave %d: %s" % (r["name"], r["slave"], note))
        regs = p.get("regs") or []
        if len(regs) >= 8:
            ranked = score_maps(regs)
            best_maps.append(ranked[0])
            lines.append("  맵 점수:")
            for m in ranked:
                lines.append("    %d  %s  %s" % (m["score"], m["name"], ", ".join(m["hits"]) or "-"))

    ascii_hit = any("ASCII" in r["name"] and r["parsed"].get("ok") for r in ok_rows)

    lines.append("")
    lines.append("고칠 곳")
    if 19200 in bauds and 9600 not in bauds:
        lines.append("- 보드레이트 19200. 업체 확인·단말기·시뮬레이터 기본과 같다.")
    elif 9600 in bauds and 19200 not in bauds:
        lines.append("- 팩이 9600만 응답. 업체는 19200이라고 했으니 배선/슬레이브부터 다시 본다.")
    elif bauds:
        lines.append("- 9600·19200 둘 다 응답. 업체는 19200. 펌웨어는 19200.")

    if 0x30 in lengths:
        lines.append("- 응답 length=0x30. 단말기 parseRegisters 가 바이트수(>=96)를 요구하면 실패 → 레지스터수로 고친다.")
        lines.append("- 시뮬레이터 응답 length를 'regs' 로. 엑셀 프로토콜 시트 0x30 이 맞다.")
        lines.append("- 엑셀 통신_RX 의 LENGTH(0xEF) 는 오타로 본다.")
    if 0x60 in lengths:
        lines.append("- 응답 length=0x60. 단말기·시뮬레이터 유지. 엑셀 예제 0x30 과 RX시트 0xEF 가 틀림.")
    if 0xEF in lengths:
        lines.append("- 응답 length=0xEF. 엑셀 RX 시트는 맞을 수 있음. 단말기는 0xEF>=96 이어도 프레임 길이 검사에서 실패할 수 있다.")

    if ascii_hit:
        lines.append("- 팩이 ASCII Modbus 도 받음. 다만 이진 프레임을 쓰는 쪽이 문서와 같다.")
    else:
        lines.append("- ASCII ':0104...' 는 무응답이어야 정상(문서는 이진). 단말기로 글자를 치면 안 된다.")

    if best_maps:
        top = best_maps[0]["name"]
        lines.append("- 값 배치 1순위: %s" % top)
        if "통신_RX" in top:
            lines.append("  단말기 samwooReg 인덱스(Relay 12, 셀 17)를 RX 시트(Relay 10, 셀 15)에 맞출지 검토.")
        if "통신맵" in top:
            lines.append("  단말기 인덱스(Relay 12, 셀 17)는 통신맵과 맞다. RX 시트가 밀려 있다.")

    lines.append("")
    lines.append("리포트를 보고 단말기·시뮬레이터·엑셀을 고친다. 추측으로 미리 바꾸지 말 것.")
    return "\n".join(lines)


def run_lab(port, bauds, slaves, timeout, outdir):
    import serial

    os.makedirs(outdir, exist_ok=True)
    log_path = os.path.join(outdir, "report.txt")
    raw_path = os.path.join(outdir, "raw.hex.txt")
    print_and_log(log_path, "삼우 실팩 랩  %s" % datetime.now().isoformat(timespec="seconds"))
    print_and_log(log_path, "port=%s baud=%s slave=%s" % (port, bauds, slaves))
    print_and_log(log_path, "TX start=1 %s" % DOC_TX_HEX)
    print_and_log(log_path, "디스플레이는 버스에서 뺄 것. PC만 마스터.")
    print_and_log(log_path, "")

    ok_rows = []
    for baud in bauds:
        print_and_log(log_path, "---- %s %d 8N1 ----" % (port, baud))
        try:
            ser = serial.Serial(port, baud, bytesize=8, parity="N", stopbits=1, timeout=0.05)
        except Exception as e:
            print_and_log(log_path, "포트 열기 실패: %s" % e)
            continue
        try:
            for slave in slaves:
                for name, frame in cases_for(slave):
                    row = send_case(ser, name, frame, timeout)
                    row["baud"] = baud
                    row["slave"] = slave
                    p = row["parsed"]
                    txh = hex_spaces(row["tx"])
                    rxh = hex_spaces(row["rx"] or row.get("leftover") or b"")
                    write_log(raw_path, "BAUD %d SLAVE %d %s\nTX %s\nRX %s\n" % (baud, slave, name, txh, rxh or "(없음)"))
                    mark = "OK" if p.get("ok") else "NO"
                    err = p.get("error") or ""
                    extra = ""
                    if p.get("kind") == "response" and p.get("ok"):
                        extra = " length=0x%02X regs=%d" % (p.get("length") or 0, len(p.get("regs") or []))
                        ok_rows.append(row)
                    line = "[%s] baud=%d slave=%d %s %s%s" % (mark, baud, slave, name, err, extra)
                    print_and_log(log_path, line)
                    print_and_log(log_path, "     TX %s" % txh)
                    if rxh:
                        print_and_log(log_path, "     RX %s" % rxh)
                    if p.get("kind") == "response" and p.get("regs"):
                        print_and_log(log_path, format_reg_table(p["regs"]))
                    time.sleep(0.15)
        finally:
            ser.close()

    if ok_rows:
        print_and_log(log_path, analyze_ok_rows(ok_rows))
    else:
        print_and_log(
            log_path,
            "\n응답 없음.\n"
            "- 포트/A·B/GND, 종단저항\n"
            "- 디스플레이가 같이 물려 있으면 충돌\n"
            "- --baud 19200 이 기본. 안 되면 --baud 9600\n"
            "- 슬레이브가 1이 아니면 --slave 2\n",
        )
    print_and_log(log_path, "\n저장: %s" % outdir)
    return 0 if ok_rows else 1


def pick_port(cli):
    ports = list_ports()
    print("COM 포트")
    if not ports:
        print("  (없음)")
    for p in ports:
        print("  %s  %s" % (p.device, p.description))
    if cli:
        return cli
    try:
        typed = input("포트 (예 COM14): ").strip()
    except EOFError:
        typed = ""
    return typed


def main():
    ap = argparse.ArgumentParser(description="삼우 실팩 RS-485 랩")
    ap.add_argument("--port", help="USB-RS485 COM. 없으면 목록 보여 주고 묻는다")
    ap.add_argument("--baud", type=int, action="append", help="지정하면 그 속도만. 없으면 19200")
    ap.add_argument("--slave", type=int, action="append", help="기본 1 그리고 2")
    ap.add_argument("--timeout", type=float, default=1.2)
    args = ap.parse_args()

    port = pick_port(args.port)
    if not port:
        print("예: python python/samwoo_lab.py --port COM14")
        return 2
    bauds = args.baud if args.baud else [RS485_BAUD]
    slaves = args.slave if args.slave else [1, 2]
    stamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    outdir = os.path.join(LOG_ROOT, stamp)
    print()
    print("디스플레이는 빼고 USB-RS485만 팩에 연결했는지 확인.")
    return run_lab(port, bauds, slaves, args.timeout, outdir)


if __name__ == "__main__":
    sys.exit(main())
