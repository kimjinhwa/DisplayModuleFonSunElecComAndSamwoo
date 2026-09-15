#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""삼우 BMS 슬레이브 시뮬레이터 — 팩1/팩2.

실팩이 없을 때 디스플레이를 붙인다. 실팩 결과는 python/samwoo_lab.py 로 확인한 뒤
아래 옵션을 맞춘다.

  삼우: STX 0x3A + 이진 + LRC + CR LF  (xls 2026-08-16)
  응답 length: regs=0x30(실팩·지금 펌웨어) / bytes=0x60(구 펌웨어)
  start=1을 첫 레지스터로: 문서 TX 00 01
"""
import os
import queue
import sys
import threading
import time
from datetime import datetime

_PY = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
if _PY not in sys.path:
    sys.path.insert(0, _PY)

from samwoo_proto import (  # noqa: E402
    QTY,
    RS485_BAUD,
    SLAVE_LEFT,
    SLAVE_RIGHT,
    build_response,
    lrc8,
    make_defaults,
    register_name,
    save_pack_state,
)

import serial
from serial.tools import list_ports
import tkinter as tk
from tkinter import ttk, messagebox
from tkinter.scrolledtext import ScrolledText

import modbus_tk.defines as cst
from modbus_tk import modbus_rtu

PORT_DEFAULT = "COM4"
BAUD_DEFAULT = RS485_BAUD
MODE_RTU = "rtu"
MODE_SAMWOO = "samwoo"


def list_comports():
    return [p.device for p in list_ports.comports()] or [PORT_DEFAULT]


def fmt_hex(data):
    return " ".join("%02X" % b for b in data)


class FrameLogger:
    """스레드에서 Rx/Tx 한 줄을 넣고, UI가 drain 한다."""

    def __init__(self):
        self.q = queue.Queue(maxsize=800)

    def push(self, direction, data, note=""):
        ts = datetime.now().strftime("%H:%M:%S.%f")[:-3]
        item = (ts, direction, bytes(data), note)
        try:
            self.q.put_nowait(item)
        except queue.Full:
            try:
                self.q.get_nowait()
            except queue.Empty:
                pass
            try:
                self.q.put_nowait(item)
            except queue.Full:
                pass

    def drain(self, limit=80):
        out = []
        while len(out) < limit:
            try:
                out.append(self.q.get_nowait())
            except queue.Empty:
                break
        return out


class LoggingSerial:
    """RTU용. 요청이 모이면 write 직전에 RX 한 프레임으로 기록."""

    def __init__(self, ser, logger):
        self._ser = ser
        self._logger = logger
        self._rx = bytearray()
        self._lock = threading.Lock()

    def read(self, size=1):
        data = self._ser.read(size)
        if data:
            with self._lock:
                self._rx.extend(data)
        return data

    def write(self, data):
        self.flush_rx()
        raw = data if isinstance(data, (bytes, bytearray)) else bytes(data)
        if raw:
            self._logger.push("TX", raw, "RTU")
        return self._ser.write(data)

    def flush_rx(self):
        with self._lock:
            raw = bytes(self._rx)
            self._rx.clear()
        if raw:
            self._logger.push("RX", raw, "RTU")

    def __getattr__(self, name):
        return getattr(self._ser, name)


class DebugWindow:
    def __init__(self, parent, on_close):
        self.on_close = on_close
        self.win = tk.Toplevel(parent)
        self.win.title("통신 디버그 — Rx / Tx")
        self.win.geometry("980x420")
        self.win.protocol("WM_DELETE_WINDOW", self._close)

        bar = ttk.Frame(self.win, padding=6)
        bar.pack(fill="x")
        ttk.Label(bar, text="ModPoll 형식  |  RX=마스터→슬레이브  TX=슬레이브→마스터").pack(side="left")
        self.pause_var = tk.BooleanVar(value=False)
        ttk.Checkbutton(bar, text="Pause", variable=self.pause_var).pack(side="right", padx=6)
        ttk.Button(bar, text="Clear", command=self.clear).pack(side="right")

        self.text = ScrolledText(self.win, font=("Consolas", 10), wrap="none", bg="#111111", fg="#EEEEEE")
        self.text.pack(fill="both", expand=True, padx=6, pady=(0, 6))
        self.text.tag_configure("RX", foreground="#7CFF7C")
        self.text.tag_configure("TX", foreground="#7CC8FF")
        self.text.tag_configure("ERR", foreground="#FF6B6B")
        self.text.configure(state="disabled")
        self._lines = 0

    def append(self, ts, direction, data, note):
        if self.pause_var.get():
            return
        tag = "ERR" if direction == "ERR" else direction
        line = "[%s]  %s  [%s]" % (ts, direction.ljust(3), fmt_hex(data))
        if note:
            line += "  %s" % note
        line += "\n"
        self.text.configure(state="normal")
        self.text.insert("end", line, tag)
        self._lines += 1
        if self._lines > 400:
            self.text.delete("1.0", "80.0")
            self._lines -= 79
        self.text.see("end")
        self.text.configure(state="disabled")

    def clear(self):
        self.text.configure(state="normal")
        self.text.delete("1.0", "end")
        self.text.configure(state="disabled")
        self._lines = 0

    def _close(self):
        self.win.destroy()
        self.on_close()


class PackStore:
    def __init__(self, slave_id, defaults):
        self.slave_id = slave_id
        self.lock = threading.Lock()
        self.regs = list(defaults)

    def get_range(self, start, qty):
        with self.lock:
            out = []
            for i in range(qty):
                addr = start + i
                out.append(self.regs[addr] if 0 <= addr < QTY else 0)
            return out

    def set_one(self, addr, value):
        if 0 <= addr < QTY:
            with self.lock:
                self.regs[addr] = value & 0xFFFF

    def snapshot(self):
        with self.lock:
            return list(self.regs)


class SamwooSlaveThread(threading.Thread):
    """STX(0x3A) + binary PDU + LRC + CR LF. FC03/FC04."""

    def __init__(self, ser, packs_by_id, logger=None, start1_is_first=True, length_mode="bytes"):
        super().__init__(daemon=True)
        self.ser = ser
        self.packs_by_id = packs_by_id
        self.logger = logger
        self.start1_is_first = start1_is_first
        self.length_mode = length_mode
        self.stop_event = threading.Event()
        self.last_status = ""
        self.rx_ok = 0
        self.rx_bad = 0

    def run(self):
        buf = bytearray()
        while not self.stop_event.is_set():
            try:
                waiting = self.ser.in_waiting
            except Exception:
                break
            if waiting:
                buf.extend(self.ser.read(waiting))
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
                    self._handle(frame)
            else:
                time.sleep(0.005)

    def _log(self, direction, data, note=""):
        if self.logger:
            self.logger.push(direction, data, note)

    def _handle(self, frame):
        if len(frame) < 8 or frame[0] != 0x3A or frame[-2] != 0x0D or frame[-1] != 0x0A:
            self.rx_bad += 1
            self._log("ERR", frame, "bad frame")
            return
        payload = frame[1:-2]
        if len(payload) < 2:
            self.rx_bad += 1
            self._log("ERR", frame, "short payload")
            return
        body, got_lrc = payload[:-1], payload[-1]
        if lrc8(body) != got_lrc:
            self.rx_bad += 1
            self.last_status = "LRC error"
            self._log("ERR", frame, "LRC error")
            return
        slave = body[0]
        pack = self.packs_by_id.get(slave)
        if pack is None:
            self._log("ERR", frame, "unknown slave %d" % slave)
            return
        if len(body) < 6:
            self.rx_bad += 1
            self._log("ERR", frame, "short PDU")
            return
        fc = body[1]
        start = (body[2] << 8) | body[3]
        qty = (body[4] << 8) | body[5]
        if fc not in (3, 4) or qty < 1 or qty > QTY:
            self.rx_bad += 1
            self._log("ERR", frame, "bad FC/qty")
            return
        idx = start - 1 if (self.start1_is_first and start >= 1) else start
        values = pack.get_range(idx, qty)
        out = build_response(slave, fc, values, length_mode=self.length_mode)
        note = "slave %d FC%02d start=%d idx=%d qty=%d len=%s" % (
            slave, fc, start, idx, qty, self.length_mode)
        self._log("RX", frame, note)
        try:
            self.ser.write(out)
            self.ser.flush()
        except Exception:
            return
        self._log("TX", out, note)
        self.rx_ok += 1
        self.last_status = note

    def stop(self):
        self.stop_event.set()


class PackPanel(ttk.LabelFrame):
    def __init__(self, parent, title, slave_id, defaults):
        super().__init__(parent, text=title, padding=4)
        self.store = PackStore(slave_id, defaults)
        self.on_change = None
        self.entries = {}
        canvas = tk.Canvas(self, highlightthickness=0)
        scroll = ttk.Scrollbar(self, orient="vertical", command=canvas.yview)
        inner = ttk.Frame(canvas)
        inner.bind("<Configure>", lambda e: canvas.configure(scrollregion=canvas.bbox("all")))
        canvas.create_window((0, 0), window=inner, anchor="nw")
        canvas.configure(yscrollcommand=scroll.set)
        canvas.pack(side="left", fill="both", expand=True)
        scroll.pack(side="right", fill="y")

        def _wheel(event):
            canvas.yview_scroll(int(-1 * (event.delta / 120)), "units")

        canvas.bind("<Enter>", lambda e: canvas.bind_all("<MouseWheel>", _wheel))
        canvas.bind("<Leave>", lambda e: canvas.unbind_all("<MouseWheel>"))

        for addr in range(QTY):
            row = ttk.Frame(inner)
            row.pack(fill="x", padx=2, pady=1)
            ttk.Label(row, text="%02d %s" % (addr, register_name(addr, "rx")), width=20).pack(side="left")
            ent = ttk.Entry(row, width=7)
            ent.insert(0, str(defaults[addr]))
            ent.pack(side="left")
            self.entries[addr] = ent
            ttk.Button(row, text="Set", width=4, command=lambda a=addr: self.apply(a)).pack(side="left", padx=3)

    def apply(self, addr):
        raw = self.entries[addr].get().strip().replace(",", ".")
        try:
            val = int(raw, 10)
        except ValueError:
            try:
                f = float(raw)
            except ValueError:
                return
            # 셀전압(mV) 칸에 3.550 처럼 볼트를 넣으면 mV로 변환
            if (addr in (6, 7) or 15 <= addr <= 30) and 1.0 <= abs(f) < 10.0:
                f *= 1000.0
            val = int(round(f))
        val &= 0xFFFF
        self.store.set_one(addr, val)
        self.entries[addr].delete(0, "end")
        self.entries[addr].insert(0, str(val))
        if self.on_change:
            self.on_change(self.store.slave_id, addr, val)

    def apply_all(self):
        for addr in range(QTY):
            self.apply(addr)


class SimulatorApp:
    def __init__(self, root):
        self.root = root
        root.title("삼우 BMS 시뮬레이터 — Pack1 / Pack2")
        root.geometry("1180x760")

        self.server = None
        self.samwoo = None
        self.serial_samwoo = None
        self.rtu_slaves = {}
        self.logger = FrameLogger()
        self.debug_win = None

        top = ttk.Frame(root, padding=8)
        top.pack(fill="x")
        ttk.Label(top, text="Port").pack(side="left")
        self.port_var = tk.StringVar(value=PORT_DEFAULT)
        self.port_cb = ttk.Combobox(top, textvariable=self.port_var, values=list_comports(), width=12)
        self.port_cb.pack(side="left", padx=4)
        ttk.Label(top, text="Baud").pack(side="left")
        self.baud_var = tk.StringVar(value=str(BAUD_DEFAULT))
        ttk.Combobox(top, textvariable=self.baud_var, values=("19200", "9600", "38400"), width=8).pack(side="left", padx=4)

        ttk.Label(top, text="모드").pack(side="left", padx=(12, 4))
        self.mode_var = tk.StringVar(value=MODE_SAMWOO)
        ttk.Radiobutton(top, text="삼우 STX+LRC", variable=self.mode_var, value=MODE_SAMWOO).pack(side="left")
        ttk.Radiobutton(top, text="표준 RTU", variable=self.mode_var, value=MODE_RTU).pack(side="left")

        ttk.Button(top, text="Open", command=self.open_port).pack(side="left", padx=(12, 4))
        ttk.Button(top, text="Close", command=self.close_port).pack(side="left")
        self.debug_var = tk.BooleanVar(value=False)
        ttk.Checkbutton(top, text="Debug", variable=self.debug_var, command=self._toggle_debug).pack(side="left", padx=(16, 0))

        opt = ttk.Frame(root, padding=(8, 0))
        opt.pack(fill="x")
        self.length_var = tk.StringVar(value="regs")
        ttk.Label(opt, text="응답 length").pack(side="left")
        ttk.Radiobutton(opt, text="레지스터수 0x30 (실팩)", variable=self.length_var, value="regs").pack(side="left")
        ttk.Radiobutton(opt, text="바이트수 0x60 (구 펌웨어)", variable=self.length_var, value="bytes").pack(side="left")

        self.status = tk.StringVar(value="대기 — 포트를 여세요")
        ttk.Label(root, textvariable=self.status, padding=(8, 0)).pack(fill="x")

        body = ttk.Frame(root, padding=6)
        body.pack(fill="both", expand=True)
        body.columnconfigure(0, weight=1)
        body.columnconfigure(1, weight=1)
        body.rowconfigure(0, weight=1)

        self.pack1 = PackPanel(body, "1번 배터리팩  (Slave %d)" % SLAVE_LEFT, SLAVE_LEFT, make_defaults(0))
        self.pack2 = PackPanel(body, "2번 배터리팩  (Slave %d)" % SLAVE_RIGHT, SLAVE_RIGHT, make_defaults(1))
        self.pack1.grid(row=0, column=0, sticky="nsew", padx=(0, 4))
        self.pack2.grid(row=0, column=1, sticky="nsew", padx=(4, 0))
        self.pack1.on_change = self._on_pack_change
        self.pack2.on_change = self._on_pack_change
        self._persist_packs()
        self.packs = {SLAVE_LEFT: self.pack1.store, SLAVE_RIGHT: self.pack2.store}

        root.protocol("WM_DELETE_WINDOW", self.on_close)
        self._poll_status()

    def _toggle_debug(self):
        if self.debug_var.get():
            if self.debug_win is None:
                self.debug_win = DebugWindow(self.root, self._debug_closed)
        else:
            if self.debug_win is not None:
                self.debug_win.win.destroy()
                self.debug_win = None

    def _debug_closed(self):
        self.debug_win = None
        self.debug_var.set(False)

    def _poll_status(self):
        if self.samwoo and self.samwoo.is_alive():
            extra = self.samwoo.last_status or "-"
            self.status.set("삼우 모드  |  OK=%d BAD=%d  |  %s" % (self.samwoo.rx_ok, self.samwoo.rx_bad, extra))
        if self.debug_win is not None and not self.debug_win.pause_var.get():
            for ts, direction, data, note in self.logger.drain():
                self.debug_win.append(ts, direction, data, note)
        self.root.after(120, self._poll_status)

    def open_port(self, silent=False):
        if self.server or self.samwoo:
            if not silent:
                messagebox.showwarning("열림", "이미 포트가 열려 있습니다. Close 후 다시 여세요.")
            return
        port = self.port_var.get().strip()
        try:
            baud = int(self.baud_var.get())
        except ValueError:
            if not silent:
                messagebox.showerror("Error", "Baud가 숫자가 아닙니다.")
            return
        self.pack1.apply_all()
        self.pack2.apply_all()
        self._persist_packs()
        mode = self.mode_var.get()
        try:
            if mode == MODE_RTU:
                self._open_rtu(port, baud)
            else:
                self._open_samwoo(port, baud)
        except Exception as e:
            self.close_port(silent=True)
            if not silent:
                messagebox.showerror("Error", str(e))
            else:
                self.status.set("열기 실패: %s" % e)
            return
        if mode == MODE_RTU:
            label = "표준 Modbus RTU"
        else:
            label = "삼우 STX+LRC length=%s start=1" % self.length_var.get()
        self.status.set("%s  |  %s %d 8N1  |  slave %d / %d" % (label, port, baud, SLAVE_LEFT, SLAVE_RIGHT))
        if not silent:
            messagebox.showinfo("OK", "%s\n%s %d 8N1\nPack1 slave=%d, Pack2 slave=%d" % (
                label, port, baud, SLAVE_LEFT, SLAVE_RIGHT))

    def _persist_packs(self):
        save_pack_state([self.pack1.store.snapshot(), self.pack2.store.snapshot()])

    def _on_pack_change(self, slave_id, addr, value):
        self._persist_packs()
        self._sync_rtu_from_store(slave_id, addr, value)

    def _sync_rtu_from_store(self, slave_id, addr, value):
        slave = self.rtu_slaves.get(slave_id)
        if slave is None:
            return
        slave.set_values("ir", addr, value)
        slave.set_values("hr", addr, value)

    def _open_rtu(self, port, baud):
        raw = serial.Serial(port, baud, timeout=0.05)
        tapped = LoggingSerial(raw, self.logger)
        self.server = modbus_rtu.RtuServer(tapped)
        self.server.start()
        for store in self.packs.values():
            sl = self.server.add_slave(store.slave_id)
            sl.add_block("hr", cst.HOLDING_REGISTERS, 0, QTY)
            sl.add_block("ir", cst.READ_INPUT_REGISTERS, 0, QTY)
            snap = store.snapshot()
            sl.set_values("hr", 0, snap)
            sl.set_values("ir", 0, snap)
            self.rtu_slaves[store.slave_id] = sl

    def _open_samwoo(self, port, baud):
        self.serial_samwoo = serial.Serial(port, baud, timeout=0.05)
        self.samwoo = SamwooSlaveThread(
            self.serial_samwoo,
            self.packs,
            self.logger,
            start1_is_first=True,
            length_mode=self.length_var.get(),
        )
        self.samwoo.start()

    def close_port(self, silent=False):
        try:
            if self.samwoo:
                self.samwoo.stop()
                self.samwoo.join(timeout=1.0)
                self.samwoo = None
            if self.serial_samwoo:
                self.serial_samwoo.close()
                self.serial_samwoo = None
            if self.server:
                self.server.stop()
                self.server = None
            self.rtu_slaves = {}
            self.status.set("닫힘")
            if not silent:
                messagebox.showinfo("OK", "Closed")
        except Exception as e:
            if not silent:
                messagebox.showerror("Error", str(e))

    def on_close(self):
        self.pack1.apply_all()
        self.pack2.apply_all()
        self._persist_packs()
        self.close_port(silent=True)
        self.root.destroy()


def build_ui(port=None, mode=None, auto_open=False):
    root = tk.Tk()
    app = SimulatorApp(root)
    if port:
        app.port_var.set(port)
    if mode:
        app.mode_var.set(mode)
    if auto_open:
        root.after(200, lambda: app.open_port(silent=True))
    root.mainloop()


if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser(description="BMS Modbus / Samwoo slave simulator")
    parser.add_argument("--port", default=PORT_DEFAULT)
    parser.add_argument("--mode", choices=(MODE_RTU, MODE_SAMWOO), default=MODE_SAMWOO)
    parser.add_argument("--open", action="store_true", help="시작 시 포트 자동 Open")
    args = parser.parse_args()
    build_ui(port=args.port, mode=args.mode, auto_open=args.open)
