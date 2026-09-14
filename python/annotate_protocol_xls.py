#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""업체 확인용. 원본 숫자는 지우지 않고, 실팩 분석 칸·색만 추가."""
import os
import shutil
import sys

SRC = os.path.join(
    os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
    "doc",
    "삼우에스비_통신_Protocol_20260816.xls",
)

# Excel Interior.Color 는 BGR
YEL = 0x99FFFF  # 제안(확인 필요)
ORG = 0x0066FF  # 원본과 불일치
GRN = 0x90EE90  # 실팩과 일치
BLU = 0xF0D2C8  # 새 칸 헤더
RED = 0x0000C0


def rgb(cell, color, bold=False):
    cell.Interior.Color = color
    if bold:
        cell.Font.Bold = True


def main():
    if not os.path.isfile(SRC):
        print("없음", SRC)
        return 1
    bak = SRC[:-4] + ".bak"
    if not os.path.exists(bak):
        shutil.copy2(SRC, bak)
        print("원본 보관", bak)

    import win32com.client

    excel = win32com.client.DispatchEx("Excel.Application")
    excel.Visible = False
    excel.DisplayAlerts = False
    wb = None
    try:
        wb = excel.Workbooks.Open(SRC)
        proto = rx = mp = None
        for i in range(1, wb.Worksheets.Count + 1):
            name = wb.Worksheets(i).Name
            if name == "프로토콜":
                proto = wb.Worksheets(i)
            elif "RX" in name:
                rx = wb.Worksheets(i)
            elif name == "통신맵":
                mp = wb.Worksheets(i)
        if not (proto and rx and mp):
            raise RuntimeError("시트 이름 확인 실패")

        _protocol(proto)
        _rx(rx)
        _map(mp)
        _review_sheet(wb)
        wb.Save()
        print("저장", SRC)
    finally:
        if wb is not None:
            wb.Close(SaveChanges=True)
        excel.Quit()
    return 0


def _protocol(ws):
    ws.Cells(1, 1).Value = (
        "【업체 확인 요청 2026-09-14】 PC↔실팩 RS-485. "
        "노랑=제안, 주황=원본과 다름, 초록=실팩과 같음. 원본 칸은 유지."
    )
    rgb(ws.Cells(1, 1), YEL, True)
    ws.Range("A1:O1").Merge()

    ws.Cells(2, 14).Value = "실팩 통신"
    ws.Cells(3, 14).Value = "19200 8N1"
    ws.Cells(4, 14).Value = "(문서에 속도 없음. 업체 구두 확인)"
    rgb(ws.Cells(2, 14), BLU, True)
    rgb(ws.Cells(3, 14), YEL)
    rgb(ws.Cells(4, 14), YEL)

    # TX 예제는 실팩과 동일
    for col in range(2, 12):
        rgb(ws.Cells(4, col), GRN)
    ws.Cells(5, 2).Value = "실팩: 위 TX 그대로 응답함. start 는 0x0001 만 사용."
    rgb(ws.Cells(5, 2), GRN)
    ws.Range("B5:L5").Merge()

    ws.Cells(9, 16).Value = "실팩 length=0x30 (레지스터 48). 표준 Modbus 바이트수 0x60 아님."
    rgb(ws.Cells(9, 16), GRN)

    ws.Cells(18, 5).Value = "실팩 제안: 위 줄 번호 <2> 중복 → <3> 로."
    rgb(ws.Cells(18, 5), YEL)
    rgb(ws.Cells(17, 2), ORG)


def _rx(ws):
    ws.Cells(1, 5).Value = "실팩 확인"
    ws.Cells(1, 6).Value = "판정"
    rgb(ws.Cells(1, 5), BLU, True)
    rgb(ws.Cells(1, 6), BLU, True)

    ws.Cells(6, 5).Value = "LENGTH(0x30) 레지스터 48개"
    ws.Cells(6, 6).Value = "원본 0xEF 불일치. 실팩은 0x30."
    rgb(ws.Cells(6, 4), ORG)
    rgb(ws.Cells(6, 5), YEL)
    rgb(ws.Cells(6, 6), YEL)

    # 데이터 배치는 실팩과 같음
    for row in range(7, 55):
        ws.Cells(row, 5).Value = "원본과 동일"
        rgb(ws.Cells(row, 5), GRN)
        rgb(ws.Cells(row, 6), GRN)
        ws.Cells(row, 6).Value = "일치"

    ws.Cells(56, 5).Value = "CR (0x0D)"
    ws.Cells(57, 5).Value = "LF (0x0A)"
    ws.Cells(56, 6).Value = "표기 /r → CR 제안"
    ws.Cells(57, 6).Value = "표기 /n → LF 제안"
    rgb(ws.Cells(56, 5), YEL)
    rgb(ws.Cells(57, 5), YEL)
    rgb(ws.Cells(56, 4), ORG)
    rgb(ws.Cells(57, 4), ORG)

    ws.Columns(5).ColumnWidth = 32
    ws.Columns(6).ColumnWidth = 36


def _map(ws):
    # K=11 실팩 ADDRESS, L=12 판정, M=13 비고
    ws.Cells(1, 11).Value = "실팩 ADDRESS"
    ws.Cells(1, 12).Value = "판정"
    ws.Cells(1, 13).Value = "비고 (업체 확인)"
    rgb(ws.Cells(1, 11), BLU, True)
    rgb(ws.Cells(1, 12), BLU, True)
    rgb(ws.Cells(1, 13), BLU, True)
    ws.Cells(2, 11).Value = "실팩"
    ws.Cells(2, 12).Value = "판정"
    ws.Cells(2, 13).Value = "비고"
    rgb(ws.Cells(2, 11), BLU, True)
    rgb(ws.Cells(2, 12), BLU, True)
    rgb(ws.Cells(2, 13), BLU, True)

    # 기존 ADDRESS → 실팩. 행은 엑셀 1-based.
    # 0~9 는 행 4~13, 값 그대로
    same = [
        (4, 0, "BMS Ver"),
        (5, 1, "Capacity. 단위 A→Ah 제안"),
        (6, 2, "SOC"),
        (7, 3, "SOH"),
        (8, 4, "Total Voltage"),
        (9, 5, "Current"),
        (10, 6, "Cell Vmax"),
        (11, 7, "Cell Vmin"),
        (12, 8, "Tmax"),
        (13, 9, "Tmin"),
    ]
    for row, addr, note in same:
        ws.Cells(row, 11).Value = addr
        ws.Cells(row, 12).Value = "일치"
        ws.Cells(row, 13).Value = note
        rgb(ws.Cells(row, 11), GRN)
        rgb(ws.Cells(row, 12), GRN)

    ws.Cells(5, 13).Value = "실팩 1000 → 100.0 Ah. 단위 A 가 아니라 Ah 제안."
    rgb(ws.Cells(5, 8), ORG)
    rgb(ws.Cells(5, 13), YEL)

    # Relay 블록 행 14~18 주소 12 → 10
    # Fault 19~27 주소 13 → 11
    # Protect 28~36 주소 14 → 12
    # Warning 37~45 주소 15 → 13
    blocks = [
        (14, 18, 10, 12, "Relay. 문서 12, 실팩 10 (10·11 공백 없음)"),
        (19, 27, 11, 13, "Fault. 문서 13, 실팩 11"),
        (28, 36, 12, 14, "Protect. 문서 14, 실팩 12"),
        (37, 45, 13, 15, "Warning. 문서 15, 실팩 13"),
    ]
    for r0, r1, new, old, note in blocks:
        for row in range(r0, r1 + 1):
            ws.Cells(row, 11).Value = new
            ws.Cells(row, 12).Value = "불일치"
            ws.Cells(row, 13).Value = note
            rgb(ws.Cells(row, 1), ORG)
            rgb(ws.Cells(row, 11), YEL)
            rgb(ws.Cells(row, 12), YEL)
            rgb(ws.Cells(row, 13), YEL)

    # Cell Num 행 46 주소 16 → 14
    ws.Cells(46, 11).Value = 14
    ws.Cells(46, 12).Value = "불일치"
    ws.Cells(46, 13).Value = "Cell Num. 문서 16, 실팩 14. 값은 16."
    rgb(ws.Cells(46, 1), ORG)
    rgb(ws.Cells(46, 11), YEL)
    rgb(ws.Cells(46, 12), YEL)
    rgb(ws.Cells(46, 13), YEL)

    # Cell[0] 행 47 주소 17 → 15
    ws.Cells(47, 11).Value = 15
    ws.Cells(47, 12).Value = "불일치"
    ws.Cells(47, 13).Value = "Cell[0]. 문서 17, 실팩 15."
    rgb(ws.Cells(47, 1), ORG)
    rgb(ws.Cells(47, 11), YEL)
    rgb(ws.Cells(47, 12), YEL)
    rgb(ws.Cells(47, 13), YEL)

    ws.Cells(48, 11).Value = "16~29"
    ws.Cells(48, 12).Value = "불일치"
    ws.Cells(48, 13).Value = "셀 16개면 15~30. 문서 17~30은 14칸인데 [0]~[15]로 적혀 있음."
    rgb(ws.Cells(48, 11), YEL)
    rgb(ws.Cells(48, 12), YEL)
    rgb(ws.Cells(48, 13), YEL)

    # Cell[15] 행 49 주소 30 — 끝 번호는 같음
    ws.Cells(49, 11).Value = 30
    ws.Cells(49, 12).Value = "끝번호 일치"
    ws.Cells(49, 13).Value = "실팩 Cell[15]=ADDRESS 30. 시작만 17→15."
    rgb(ws.Cells(49, 11), GRN)
    rgb(ws.Cells(49, 12), YEL)

    # Temp num 31, temps 32~39
    ws.Cells(50, 11).Value = 31
    ws.Cells(50, 12).Value = "일치"
    rgb(ws.Cells(50, 11), GRN)
    rgb(ws.Cells(50, 12), GRN)
    ws.Cells(51, 11).Value = 32
    ws.Cells(51, 12).Value = "일치"
    rgb(ws.Cells(51, 11), GRN)
    rgb(ws.Cells(51, 12), GRN)
    ws.Cells(52, 11).Value = "33~38"
    ws.Cells(52, 12).Value = "일치"
    rgb(ws.Cells(52, 11), GRN)
    rgb(ws.Cells(52, 12), GRN)
    ws.Cells(53, 11).Value = 39
    ws.Cells(53, 12).Value = "표기 확인"
    ws.Cells(53, 13).Value = "문서 LTC Temperature[8} → [7] 제안 (32~39 = 8개)"
    rgb(ws.Cells(53, 5), ORG)
    rgb(ws.Cells(53, 11), GRN)
    rgb(ws.Cells(53, 12), YEL)
    rgb(ws.Cells(53, 13), YEL)

    for row in range(54, 61):
        ws.Cells(row, 11).Value = int(ws.Cells(row, 1).Value) if ws.Cells(row, 1).Value != "" else ""
        ws.Cells(row, 12).Value = "예비 일치"
        rgb(ws.Cells(row, 11), GRN)
        rgb(ws.Cells(row, 12), GRN)

    # Fault ON/OFF 오타
    ws.Cells(25, 14).Value = "문서 OFF:1 → OFF:0 제안"
    rgb(ws.Cells(25, 9), ORG)
    rgb(ws.Cells(25, 14), YEL)

    ws.Columns(11).ColumnWidth = 14
    ws.Columns(12).ColumnWidth = 12
    ws.Columns(13).ColumnWidth = 56


def _review_sheet(wb):
    name = "실팩대조_20260914"
    for i in range(1, wb.Worksheets.Count + 1):
        if wb.Worksheets(i).Name == name:
            wb.Worksheets(i).Delete()
            break
    ws = wb.Worksheets.Add(After=wb.Worksheets(wb.Worksheets.Count))
    ws.Name = name

    ws.Cells(1, 1).Value = "삼우에스비 통신 프로토콜 — 실팩 대조 (업체 확인용)"
    rgb(ws.Cells(1, 1), YEL, True)
    ws.Range("A1:F1").Merge()
    ws.Cells(2, 1).Value = (
        "2026-09-14. PC COM4 CP210x, 19200 8N1, 슬레이브 1. "
        "요청 3A 01 04 00 01 00 30 CA 0D 0A 에 응답. 원본 시트 숫자는 그대로 두었음."
    )
    ws.Range("A2:F2").Merge()

    headers = ["항목", "문서(원본)", "실팩 분석", "판정", "근거", "업체 확인"]
    for i, h in enumerate(headers, 1):
        ws.Cells(4, i).Value = h
        rgb(ws.Cells(4, i), BLU, True)

    rows = [
        ("보드레이트", "없음", "19200 8N1", "추가", "업체 구두 + 실팩 응답", ""),
        ("프레이밍", "0x3A + 이진 + LRC + CR LF", "동일", "일치", "ASCII :0104... 는 무응답", ""),
        ("TX start", "0x0001", "0x0001", "일치", "start=1 만 사용. BMS Ver=3 이 첫 워드", ""),
        ("TX qty / LRC", "0x0030 / 0xCA", "동일", "일치", "01+04+00+01+00+30=36 → CA", ""),
        ("RX length", "프로토콜시트 0x30 / RX시트 0xEF", "0x30 (레지스터 48)", "RX시트 오타", "데이터 96B=48워드", ""),
        ("CMD 모니터", "0x04 Input", "0x04", "일치", "0x03도 답하나 설정값 맵으로 보임", ""),
        ("ADDRESS 0~9", "Ver~Tmin", "동일", "일치", "SOC 100, V 54.0, 셀max 3393", ""),
        ("Relay ADDRESS", "12", "10", "불일치", "실팩[10]=7 (충전+방전+시스템)", ""),
        ("Fault ADDRESS", "13", "11", "불일치", "통신_RX 시트와 동일, 통신맵만 2칸 뒤", ""),
        ("Protect ADDRESS", "14", "12", "불일치", "", ""),
        ("Warning ADDRESS", "15", "13", "불일치", "", ""),
        ("Cell Num ADDRESS", "16", "14", "불일치", "실팩[14]=16", ""),
        ("Cell[0] ADDRESS", "17", "15", "불일치", "실팩[15]=3382mV … [30]=3384mV 16개", ""),
        ("Cell[15] ADDRESS", "30 (17~30을 [0]~[15]로 표기)", "30 (15~30)", "시작 주소만 틀림", "14칸을 16개로 적은 표기 오류", ""),
        ("Temp Num / Temp[0]", "31 / 32", "31 / 32", "일치", "실팩 TempNum=8, 26.2~27.2℃", ""),
        ("용량 단위", "A", "Ah 제안", "확인", "1000 × 0.1 = 100.0 Ah", ""),
        ("LRC 설명 <2> 두 줄", "<2> 오버, <2> 2의 보수", "<2> 오버, <3> 2의 보수", "표기", "계산 자체는 맞음", ""),
    ]
    for i, row in enumerate(rows, 5):
        for c, val in enumerate(row, 1):
            ws.Cells(i, c).Value = val
        judge = row[3]
        color = GRN if judge == "일치" else YEL
        if judge == "불일치" or "오타" in judge or "틀림" in judge:
            color = ORG
            rgb(ws.Cells(i, 2), ORG)
            rgb(ws.Cells(i, 3), YEL)
        rgb(ws.Cells(i, 4), color)

    last = 4 + len(rows)
    ws.Cells(last + 2, 1).Value = (
        "요청: 노란/주황 칸이 맞는지 회신 부탁드립니다. "
        "맞으면 통신맵 ADDRESS를 실팩 열로 고치고, RX LENGTH를 0x30 으로 개정해 주시면 됩니다."
    )
    rgb(ws.Cells(last + 2, 1), YEL, True)
    ws.Range(ws.Cells(last + 2, 1), ws.Cells(last + 2, 6)).Merge()

    ws.Columns(1).ColumnWidth = 22
    ws.Columns(2).ColumnWidth = 42
    ws.Columns(3).ColumnWidth = 36
    ws.Columns(4).ColumnWidth = 16
    ws.Columns(5).ColumnWidth = 44
    ws.Columns(6).ColumnWidth = 16


if __name__ == "__main__":
    sys.exit(main())
