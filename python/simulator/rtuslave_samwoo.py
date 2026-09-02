#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""BMS Modbus slave simulator — 표준 RTU / 삼우(STX+LRC), Pack1+Pack2."""
import threading
import time

import serial
from serial.tools import list_ports
import tkinter as tk
from tkinter import ttk, messagebox

import modbus_tk.defines as cst
from modbus_tk import modbus_rtu

PORT_DEFAULT = "COM4"
BAUD_DEFAULT = 9600
QTY = 48
SLAVE_LEFT = 1
SLAVE_RIGHT = 2

MODE_RTU = "rtu"
MODE_SAMWOO = "samwoo"

LABELS = {
    0: "BMS Ver", 1: "Capacity x0.1", 2: "SOC %", 3: "SOH %",
    4: "PackV x0.1", 5: "Current x0.1", 6: "CellVmax mV", 7: "CellVmin mV",
    8: "Tmax x0.1", 9: "Tmin x0.1", 12: "Relay", 13: "Fault",
    14: "Protect", 15: "Warning", 16: "CellNum",
}


def register_name(addr):
    if 17 <= addr <= 31:
        return "CellV[%d]" % (addr - 16)
    if 32 <= addr <= 39:
        return "Temp[%d]" % (addr - 31)
    return LABELS.get(addr, "R%d" % addr)


def make_defaults(pack_index):
    """pack_index 0=왼쪽(1번), 1=오른쪽(2번). 값이 달라야 구분이 됩니다."""
    d = [0] * QTY
    d[0] = 10 + pack_index
    d[1] = 1000
    d[2] = 85 if pack_index == 0 else 70
    d[3] = 98 if pack_index == 0 else 95
    d[4] = 540 if pack_index == 0 else 528
    d[5] = 25 if pack_index == 0 else 18
    d[6] = 3400 if pack_index == 0 else 3380
    d[7] = 3300 if pack_index == 0 else 3280
    d[8] = 250 if pack_index == 0 else 245
    d[9] = 240 if pack_index == 0 else 235
    d[12] = 0x07
    d[16] = 16
    for i in range(16):
        d[17 + i] = (3350 if pack_index == 0 else 3320) + i
    d[31] = 8
    for i in range(8):
        d[32 + i] = 250 if pack_index == 0 else 245
    return d


def lrc8(data):
    return ((~sum(data) & 0xFF) + 1) & 0xFF


def list_comports():
    return [p.device for p in list_ports.comports()] or [PORT_DEFAULT]


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

    def __init__(self, ser, packs_by_id):
        super().__init__(daemon=True)
        self.ser = ser
        self.packs_by_id = packs_by_id
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

    def _handle(self, frame):
        if len(frame) < 8 or frame[0] != 0x3A or frame[-2] != 0x0D or frame[-1] != 0x0A:
            self.rx_bad += 1
            return
        payload = frame[1:-2]
        if len(payload) < 2:
            self.rx_bad += 1
            return
        body, got_lrc = payload[:-1], payload[-1]
        if lrc8(body) != got_lrc:
            self.rx_bad += 1
            self.last_status = "LRC error"
            return
        slave = body[0]
        pack = self.packs_by_id.get(slave)
        if pack is None:
            return
        if len(body) < 6:
            self.rx_bad += 1
            return
        fc = body[1]
        start = (body[2] << 8) | body[3]
        qty = (body[4] << 8) | body[5]
        if fc not in (3, 4) or qty < 1 or qty > QTY:
            self.rx_bad += 1
            return
        values = pack.get_range(start, qty)
        data = bytearray()
        for v in values:
            data.append((v >> 8) & 0xFF)
            data.append(v & 0xFF)
        resp_body = bytes([slave, fc, len(data)]) + bytes(data)
        out = bytes([0x3A]) + resp_body + bytes([lrc8(resp_body), 0x0D, 0x0A])
        try:
            self.ser.write(out)
            self.ser.flush()
        except Exception:
            return
        self.rx_ok += 1
        self.last_status = "slave %d FC%02d start=%d qty=%d" % (slave, fc, start, qty)

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
            ttk.Label(row, text="%02d %s" % (addr, register_name(addr)), width=18).pack(side="left")
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
            if (addr in (6, 7) or 17 <= addr <= 31) and 1.0 <= abs(f) < 10.0:
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
        root.title("BMS Modbus Simulator — Pack1 / Pack2")
        root.geometry("1080x720")

        self.server = None
        self.samwoo = None
        self.serial_samwoo = None
        self.rtu_slaves = {}

        top = ttk.Frame(root, padding=8)
        top.pack(fill="x")
        ttk.Label(top, text="Port").pack(side="left")
        self.port_var = tk.StringVar(value=PORT_DEFAULT)
        self.port_cb = ttk.Combobox(top, textvariable=self.port_var, values=list_comports(), width=12)
        self.port_cb.pack(side="left", padx=4)
        ttk.Label(top, text="Baud").pack(side="left")
        self.baud_var = tk.StringVar(value=str(BAUD_DEFAULT))
        ttk.Entry(top, textvariable=self.baud_var, width=8).pack(side="left", padx=4)

        ttk.Label(top, text="모드").pack(side="left", padx=(12, 4))
        self.mode_var = tk.StringVar(value=MODE_RTU)
        ttk.Radiobutton(top, text="표준 Modbus RTU", variable=self.mode_var, value=MODE_RTU).pack(side="left")
        ttk.Radiobutton(top, text="삼우 (STX 0x3A + LRC + CR LF)", variable=self.mode_var, value=MODE_SAMWOO).pack(side="left")

        ttk.Button(top, text="Open", command=self.open_port).pack(side="left", padx=(12, 4))
        ttk.Button(top, text="Close", command=self.close_port).pack(side="left")

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
        self.pack1.on_change = self._sync_rtu_from_store
        self.pack2.on_change = self._sync_rtu_from_store
        self.packs = {SLAVE_LEFT: self.pack1.store, SLAVE_RIGHT: self.pack2.store}

        root.protocol("WM_DELETE_WINDOW", self.on_close)
        self._poll_status()

    def _poll_status(self):
        if self.samwoo and self.samwoo.is_alive():
            extra = self.samwoo.last_status or "-"
            self.status.set("삼우 모드  |  OK=%d BAD=%d  |  %s" % (self.samwoo.rx_ok, self.samwoo.rx_bad, extra))
        self.root.after(400, self._poll_status)

    def open_port(self):
        if self.server or self.samwoo:
            messagebox.showwarning("열림", "이미 포트가 열려 있습니다. Close 후 다시 여세요.")
            return
        port = self.port_var.get().strip()
        try:
            baud = int(self.baud_var.get())
        except ValueError:
            messagebox.showerror("Error", "Baud가 숫자가 아닙니다.")
            return
        self.pack1.apply_all()
        self.pack2.apply_all()
        mode = self.mode_var.get()
        try:
            if mode == MODE_RTU:
                self._open_rtu(port, baud)
            else:
                self._open_samwoo(port, baud)
        except Exception as e:
            self.close_port(silent=True)
            messagebox.showerror("Error", str(e))
            return
        label = "표준 Modbus RTU" if mode == MODE_RTU else "삼우 STX+LRC"
        self.status.set("%s  |  %s %d 8N1  |  slave %d / %d" % (label, port, baud, SLAVE_LEFT, SLAVE_RIGHT))
        messagebox.showinfo("OK", "%s\n%s %d 8N1\nPack1 slave=%d, Pack2 slave=%d" % (
            label, port, baud, SLAVE_LEFT, SLAVE_RIGHT))

    def _sync_rtu_from_store(self, slave_id, addr, value):
        slave = self.rtu_slaves.get(slave_id)
        if slave is None:
            return
        slave.set_values("ir", addr, value)
        slave.set_values("hr", addr, value)

    def _open_rtu(self, port, baud):
        self.server = modbus_rtu.RtuServer(serial.Serial(port, baud, timeout=0.05))
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
        self.samwoo = SamwooSlaveThread(self.serial_samwoo, self.packs)
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
        self.close_port(silent=True)
        self.root.destroy()


def build_ui():
    root = tk.Tk()
    SimulatorApp(root)
    root.mainloop()


if __name__ == "__main__":
    build_ui()
