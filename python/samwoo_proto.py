#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""삼우에스비 통신 프로토콜 (xls 2026-08-16) + 펌웨어/시뮬레이터 공통.

프레이밍: STX 0x3A + 이진 PDU + LRC + CR LF
표준 Modbus ASCII/RTU 아님. LRC는 ':'·CRLF 제외 8비트 합의 2의 보수.
"""
from __future__ import print_function

import os

QTY = 48
SLAVE_LEFT = 1
SLAVE_RIGHT = 2
# 업체 확인. 엑셀에는 없음.
RS485_BAUD = 19200

# 문서 TX = 실팩 = 펌웨어 (start=1, qty=48, LRC CA)
DOC_TX_HEX = "3A 01 04 00 01 00 30 CA 0D 0A"
FW_TX_HEX = DOC_TX_HEX


def lrc8(data):
    return ((~sum(data) & 0xFF) + 1) & 0xFF


def hex_spaces(data):
    return " ".join("%02X" % b for b in data)


def build_request(slave, fc=0x04, start=1, qty=QTY):
    body = bytes(
        [
            slave & 0xFF,
            fc & 0xFF,
            (start >> 8) & 0xFF,
            start & 0xFF,
            (qty >> 8) & 0xFF,
            qty & 0xFF,
        ]
    )
    return bytes([0x3A]) + body + bytes([lrc8(body), 0x0D, 0x0A])


def build_response(slave, fc, values, length_mode="regs"):
    """length_mode: regs=레지스터수 0x30(실팩), bytes=바이트수 0x60."""
    data = bytearray()
    for v in values:
        v = int(v) & 0xFFFF
        data.append((v >> 8) & 0xFF)
        data.append(v & 0xFF)
    n = len(values) if length_mode == "regs" else len(data)
    body = bytes([slave & 0xFF, fc & 0xFF, n & 0xFF]) + bytes(data)
    return bytes([0x3A]) + body + bytes([lrc8(body), 0x0D, 0x0A])


def u16(v):
    return int(v) & 0xFFFF


def s16(v):
    v = u16(v)
    return v - 0x10000 if v >= 0x8000 else v


# 통신맵 시트. 주소 10·11 공백. 17~30을 Cell[15]까지로 적어 개수가 안 맞음.
MAP_XLS = (
    ["BMS Ver", "Capacity x0.1Ah", "SOC %", "SOH %", "PackV x0.1", "Current x0.1",
     "CellVmax mV", "CellVmin mV", "Tmax x0.1C", "Tmin x0.1C",
     "(맵 공백10)", "(맵 공백11)",
     "Relay", "Fault", "Protect", "Warning", "CellNum"]
    + ["CellV[%d]" % i for i in range(14)]  # 17..30
    + ["TempNum"]
    + ["Temp[%d]" % i for i in range(8)]  # 32..39
    + ["예비%d" % i for i in range(8)]  # 40..47
)

# 통신_RX 시트. Tmin 다음이 바로 Relay, 셀 16개.
MAP_RX = (
    ["BMS Ver", "Capacity x0.1Ah", "SOC %", "SOH %", "PackV x0.1", "Current x0.1",
     "CellVmax mV", "CellVmin mV", "Tmax x0.1C", "Tmin x0.1C",
     "Relay", "Fault", "Protect", "Warning", "CellNum"]
    + ["CellV[%d]" % i for i in range(16)]  # 15..30
    + ["TempNum"]
    + ["Temp[%d]" % i for i in range(8)]
    + ["예비%d" % i for i in range(8)]
)

# 펌웨어 = 실팩 = 통신_RX. 통신맵(10·11 공백)은 와이어와 다름.
MAP_FW = MAP_RX


def register_name(addr, mapping="rx"):
    table = {"xls": MAP_XLS, "rx": MAP_RX, "fw": MAP_FW}.get(mapping, MAP_RX)
    if 0 <= addr < len(table):
        return table[addr]
    return "R%d" % addr


_SIM_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "simulator")
PACK_DEFAULTS_PATH = os.path.join(_SIM_DIR, "pack_defaults.json")
PACK_STATE_PATH = os.path.join(_SIM_DIR, "pack_state.json")


def _normalize_regs(regs):
    out = [0] * QTY
    if not regs:
        return out
    for i, v in enumerate(regs[:QTY]):
        out[i] = int(v) & 0xFFFF
    return out


def _read_json(path):
    try:
        import json
        with open(path, "r", encoding="utf-8") as f:
            return json.load(f)
    except (OSError, ValueError):
        return None


def _factory_pack_regs(pack_index):
    data = _read_json(PACK_DEFAULTS_PATH)
    packs = (data or {}).get("packs") or []
    if pack_index < len(packs) and isinstance(packs[pack_index], dict):
        return _normalize_regs(packs[pack_index].get("regs"))
    return [0] * QTY


def make_defaults(pack_index):
    """pack_state.json(마지막 편집) → pack_defaults.json(실팩 캡처) 순."""
    state = _read_json(PACK_STATE_PATH)
    packs = (state or {}).get("packs") or []
    if pack_index < len(packs) and isinstance(packs[pack_index], dict):
        regs = packs[pack_index].get("regs")
        if regs:
            return _normalize_regs(regs)
    return _factory_pack_regs(pack_index)


def save_pack_state(pack_regs_list):
    import json
    packs = []
    for i, regs in enumerate(pack_regs_list):
        packs.append({"slave": i + 1, "regs": _normalize_regs(regs)})
    os.makedirs(_SIM_DIR, exist_ok=True)
    tmp = PACK_STATE_PATH + ".tmp"
    with open(tmp, "w", encoding="utf-8") as f:
        json.dump({"packs": packs}, f, indent=2)
        f.write("\n")
    os.replace(tmp, PACK_STATE_PATH)


def words_from_bytes(data):
    out = []
    for i in range(0, len(data) - 1, 2):
        out.append((data[i] << 8) | data[i + 1])
    return out


def split_frame(raw):
    """버퍼에서 첫 3A ... 0D 0A 프레임을 자른다. 없으면 (None, raw)."""
    try:
        i = raw.index(0x3A)
    except ValueError:
        return None, raw
    for j in range(i + 1, len(raw) - 1):
        if raw[j] == 0x0D and raw[j + 1] == 0x0A:
            return raw[i : j + 2], raw[j + 2 :]
    return None, raw


def parse_frame(raw):
    """요청/응답 공통. 성공 시 dict, 실패 시 error 키."""
    info = {"raw": bytes(raw), "hex": hex_spaces(raw), "ok": False}
    if len(raw) < 8:
        info["error"] = "짧음 %dB" % len(raw)
        return info
    if raw[0] != 0x3A:
        info["error"] = "STX 아님 첫바이트=%02X" % raw[0]
        return info
    if raw[-2] != 0x0D or raw[-1] != 0x0A:
        info["error"] = "CRLF 없음"
        return info
    payload = raw[1:-2]
    if len(payload) < 2:
        info["error"] = "PDU 짧음"
        return info
    body, got = payload[:-1], payload[-1]
    calc = lrc8(body)
    info["lrc_ok"] = calc == got
    info["lrc_got"] = got
    info["lrc_calc"] = calc
    info["slave"] = body[0]
    info["fc"] = body[1] if len(body) > 1 else None
    info["length"] = body[2] if len(body) > 2 else None
    info["pdu"] = body
    if len(body) >= 6 and body[1] in (3, 4) and len(body) == 6:
        info["kind"] = "request"
        info["start"] = (body[2] << 8) | body[3]
        info["qty"] = (body[4] << 8) | body[5]
        info["ok"] = info["lrc_ok"]
        return info
    info["kind"] = "response"
    data = body[3:] if len(body) > 3 else b""
    info["data"] = data
    info["regs"] = words_from_bytes(data)
    n = info["length"] if info["length"] is not None else -1
    if n == len(data):
        info["length_meaning"] = "bytes"
    elif n == len(info["regs"]):
        info["length_meaning"] = "regs"
    elif n == 0xEF:
        info["length_meaning"] = "xls_0xEF_불일치"
    else:
        info["length_meaning"] = "unknown"
    info["ok"] = info["lrc_ok"]
    if not info["lrc_ok"]:
        info["error"] = "LRC FAIL calc=%02X got=%02X" % (calc, got)
    return info


def _score_values(regs, soc_i, volt_i, cell0_i, celln_i, ncell_guess=16):
    hits = []
    miss = []
    if soc_i < len(regs) and 0 <= regs[soc_i] <= 100:
        hits.append("SOC[%d]=%d%%" % (soc_i, regs[soc_i]))
    else:
        miss.append("SOC")
    if volt_i < len(regs) and 80 <= regs[volt_i] <= 1200:
        hits.append("PackV[%d]=%.1fV" % (volt_i, regs[volt_i] / 10.0))
    else:
        miss.append("PackV")
    cells = []
    for i in range(ncell_guess):
        a = cell0_i + i
        if a < len(regs) and 2000 <= regs[a] <= 4500:
            cells.append(regs[a])
    if len(cells) >= 8:
        hits.append("셀%d개 %d~%dmV (시작[%d])" % (len(cells), min(cells), max(cells), cell0_i))
    else:
        miss.append("CellV")
    if celln_i < len(regs) and 1 <= regs[celln_i] <= 16:
        hits.append("CellNum[%d]=%d" % (celln_i, regs[celln_i]))
    return len(hits), hits, miss


def score_maps(regs):
    """어느 엑셀 시트가 실데이터에 가까운지."""
    cands = [
        ("통신맵 구문서 (셀@17, Relay@12)", 2, 4, 17, 16, 14),
        ("통신_RX·실팩 (셀@15, Relay@10)", 2, 4, 15, 14, 16),
    ]
    rows = []
    for name, soc, volt, c0, cn, ncell in cands:
        score, hits, miss = _score_values(regs, soc, volt, c0, cn, ncell)
        rows.append({"name": name, "score": score, "hits": hits, "miss": miss})
    rows.sort(key=lambda r: r["score"], reverse=True)
    return rows


def length_verdict(info):
    n = info.get("length")
    data = info.get("data") or b""
    regs = info.get("regs") or []
    notes = []
    if n == 0x30:
        notes.append("length=0x30 → 레지스터 48. 펌웨어 parseRegisters 는 0x30 또는 0x60 둘 다 허용.")
    elif n == 0x60:
        notes.append("length=0x60 → 표준 Modbus 바이트수(96). 지금 펌웨어·시뮬레이터와 같음.")
    elif n == 0xEF:
        notes.append("length=0xEF → 통신_RX 시트 값. 실제 데이터 %dB 와 안 맞으면 엑셀 오타." % len(data))
    else:
        notes.append("length=0x%02X (%d). 데이터 %dB / %dregs." % (n or 0, n or -1, len(data), len(regs)))
    meaning = info.get("length_meaning")
    if meaning:
        notes.append("해석: %s" % meaning)
    return notes


def format_reg_table(regs, limit=QTY):
    lines = []
    n = min(len(regs), limit)
    lines.append("idx  hex    u16   s16    통신맵/펌        통신_RX")
    for i in range(n):
        v = regs[i]
        lines.append(
            "%3d  %04X  %5d %5d  %-16s %s"
            % (i, v, v, s16(v), register_name(i, "xls"), register_name(i, "rx"))
        )
    return "\n".join(lines)
