import tkinter as tk
from tkinter import ttk
from tkinter import messagebox
import math
import socket
import sys
import threading
import time
import json
import uuid

def normalize_mac(mac: str) -> str:
    if not mac:
        return ""
    return mac.strip().upper().replace("-", ":")

def mac_oui(mac: str) -> str:
    mac = normalize_mac(mac)
    parts = [p for p in mac.split(":") if p]
    if len(parts) < 3:
        return ""
    return ":".join(parts[:3])

OUI_NOTE_MAP = {
    # Raspberry Pi Foundation / Trading
    "B8:27:EB": "Raspberry Pi",
    "DC:A6:32": "Raspberry Pi",
    "E4:5F:01": "Raspberry Pi",
    "28:CD:C1": "Raspberry Pi",
    "88:A2:9E": "Raspberry Pi",

    # Espressif (ESP32/ESP8266)
    "24:6F:28": "ESP32",
    "30:AE:A4": "ESP32",
    "3C:71:BF": "ESP32",
    "7C:DF:A1": "ESP32",
    "7C:9E:BD": "ESP32",
    "84:F3:EB": "ESP32",
    "44:1D:64": "ESP32/Espressif",
}

def get_note_for_mac(mac: str) -> str:
    return OUI_NOTE_MAP.get(mac_oui(mac), "")


# 카드·헤더 모서리 반경(px). 각 변에 닿는 **원호(진짜 필렛)** 근사.
CORNER_FILLET = 5


def _arc_flat_math(cx: float, cy: float, r: float, phi_a: float, phi_b: float, steps: int):
    """φ는 수학 기준(+y 위 반시계). 캔버스(+y 아래)는 y만 반전."""
    out = []
    for i in range(steps + 1):
        t = i / max(1, steps)
        phi = phi_a + (phi_b - phi_a) * t
        px = cx + r * math.cos(phi)
        py = cy - r * math.sin(phi)
        out.extend([px, py])
    return out


def _rounded_rect_smooth_polygon(
    ax: float, ay: float, bx: float, by: float, r: float, arc_steps_per_q: int = 18
):
    """
    네 모서리에 변과 접하는 원호 근사(폐합 다각형).
    폴리선으로 꼭짓점만 잘라낸 챔퍼/팔각과 달리, CAD의 ‘코너 R’처럼 둥글다.
    """
    x1, x2 = sorted((ax, bx))
    y1, y2 = sorted((ay, by))
    w, h = x2 - x1, y2 - y1
    if w <= 0 or h <= 0:
        return [x1, y1, x2, y1, x2, y2, x1, y2]
    rr = max(0.0, min(float(r), w / 2.0, h / 2.0))
    if rr < 0.01:
        return [x1, y1, x2, y1, x2, y2, x1, y2]
    n = max(6, arc_steps_per_q)

    tls, tlt = x1 + rr, y1 + rr
    trs, trt = x2 - rr, y1 + rr
    brs, brt = x2 - rr, y2 - rr
    bls, blt = x1 + rr, y2 - rr

    parts = [
        [x1 + rr, y1, x2 - rr, y1],
        _arc_flat_math(trs, trt, rr, math.pi / 2, 0.0, n),
        [x2, y1 + rr, x2, y2 - rr],
        _arc_flat_math(brs, brt, rr, 0.0, -math.pi / 2, n),
        [x2 - rr, y2, x1 + rr, y2],
        _arc_flat_math(bls, blt, rr, -math.pi / 2, -math.pi, n),
        [x1, y2 - rr, x1, y1 + rr],
        _arc_flat_math(tls, tlt, rr, math.pi, math.pi / 2, n),
    ]

    flat = []
    for seg in parts:
        if not seg:
            continue
        for i in range(0, len(seg), 2):
            xy = [seg[i], seg[i + 1]]
            if flat and abs(xy[0] - flat[-2]) < 1e-9 and abs(xy[1] - flat[-1]) < 1e-9:
                continue
            flat.extend(xy)
    if len(flat) >= 6 and abs(flat[0] - flat[-2]) < 1e-9 and abs(flat[1] - flat[-1]) < 1e-9:
        flat = flat[:-2]
    return flat


class RoundedHeader(tk.Canvas):
    """상단 바: 필렛이 적용된 헤더."""

    def __init__(
        self,
        parent,
        text: str,
        *,
        main_bg: str,
        bar_bg: str,
        radius: float = CORNER_FILLET,
        height: int = 52,
        font=None,
        **kwargs,
    ):
        if font is None:
            font = ("Segoe UI", 12, "bold")
        self._main_bg = main_bg
        self._bar_bg = bar_bg
        self._text = text
        self._radius = radius
        self._font = font
        super().__init__(
            parent,
            highlightthickness=0,
            bd=0,
            bg=main_bg,
            height=height,
            **kwargs,
        )
        self.bind("<Configure>", lambda _e: self._draw())

    def _draw(self):
        w = float(max(self.winfo_width(), 6))
        h = float(max(self.winfo_height(), 6))
        self.delete("all")
        inset = 1.0
        pts = _rounded_rect_smooth_polygon(
            inset, inset, w - inset - 1.0, h - inset - 1.0, self._radius
        )
        self.create_polygon(*pts, fill=self._bar_bg, outline="", tags=("bar",))
        self.create_text(
            14.0,
            h / 2.0,
            anchor="w",
            text=self._text,
            fill="#ffffff",
            font=self._font,
            tags=("t",),
        )


class RoundedSheet(tk.Frame):
    """LabelFrame 대체: 제목 + 필렛 카드 안에 body(ttk.Frame)."""

    # 테두리만 살짝 구분 (채움색은 바깥 main_bg 와 통일해 음영 차를 없앰)
    CARD_OUTLINE = "#cdd1d9"

    def __init__(
        self,
        parent,
        title: str,
        *,
        main_bg: str,
        title_fg: str,
        title_font=("Segoe UI", 10, "bold"),
        radius: float = CORNER_FILLET,
        inner_padding=(12, 10),
        **kwargs,
    ):
        tk.Frame.__init__(self, parent, bg=main_bg, highlightthickness=0, **kwargs)
        self._main_bg = main_bg
        self._card_fill = main_bg
        self._radius = radius
        self._pad_x, self._pad_y = inner_padding
        tk.Label(
            self,
            text=title,
            bg=main_bg,
            fg=title_fg,
            font=title_font,
            anchor="w",
        ).pack(fill=tk.X, padx=(2, 0), pady=(0, 4))
        self._canvas = tk.Canvas(
            self, highlightthickness=0, bd=0, bg=main_bg
        )
        self._canvas.pack(fill=tk.BOTH, expand=True)
        self.body = ttk.Frame(self._canvas)
        self._inner_win = self._canvas.create_window(
            self._pad_x, self._pad_y, window=self.body, anchor="nw"
        )
        self._canvas.bind("<Configure>", lambda _e: self._redraw())

    def _redraw(self):
        self._canvas.update_idletasks()
        cw = max(self._canvas.winfo_width(), int(self._radius * 2 + 24))
        ch = max(self._canvas.winfo_height(), int(self._radius * 2 + 24))
        self._canvas.delete("card")
        ox = 1.5
        oy = 1.5
        pts = _rounded_rect_smooth_polygon(
            ox, oy, float(cw) - ox, float(ch) - oy, self._radius
        )
        self._canvas.create_polygon(
            *pts,
            fill=self._card_fill,
            outline=self.CARD_OUTLINE,
            width=1,
            tags=("card",),
        )
        self._canvas.tag_lower("card", self._inner_win)
        iw = max(8, cw - 2 * self._pad_x)
        ih = max(8, ch - 2 * self._pad_y)
        self._canvas.coords(self._inner_win, self._pad_x, self._pad_y)
        self._canvas.itemconfigure(self._inner_win, width=iw, height=ih)


class IPFinder:
    UDP_PORT = 1234

    _BG_MAIN = "#e8eaee"
    _BG_HEADER = "#263343"
    _ACCENT = "#2f5f8f"
    _BTN_PADDING = (14, 6)

    def _apply_styles(self):
        """ttk 테마와 색·폰트를 맞춤 설정한다."""
        self.root.configure(bg=self._BG_MAIN)
        style = ttk.Style(self.root)
        try:
            if "clam" in style.theme_names():
                style.theme_use("clam")
        except tk.TclError:
            pass

        base_font = ("Segoe UI", 9)
        heading_font = ("Segoe UI", 10, "bold")

        style.configure(".", font=base_font, background=self._BG_MAIN)
        style.configure("TFrame", background=self._BG_MAIN)
        style.configure(
            "TLabelframe",
            background=self._BG_MAIN,
            relief="solid",
            borderwidth=1,
            labeloutside=False,
            labelmargins=[8, 0, 8, 4],
        )
        style.configure(
            "TLabelframe.Label",
            background=self._BG_MAIN,
            foreground=self._BG_HEADER,
            font=heading_font,
        )
        style.configure(
            "TCheckbutton",
            background=self._BG_MAIN,
            font=base_font,
        )
        style.configure(
            "Primary.TButton",
            font=("Segoe UI", 9, "bold"),
            padding=self._BTN_PADDING,
            background=self._ACCENT,
            foreground="#ffffff",
        )
        style.configure(
            "Secondary.TButton",
            font=base_font,
            padding=self._BTN_PADDING,
        )
        style.map(
            "Primary.TButton",
            background=[
                ("pressed", "#244a73"),
                ("active", "#3d7ab5"),
                ("disabled", "#9fb8d0"),
            ],
            foreground=[("disabled", "#f2f2f2")],
        )
        style.map(
            "Secondary.TButton",
            background=[("active", "#d8dde4")],
        )

        style.configure(
            "Treeview",
            font=("Consolas", 9),
            rowheight=24,
            fieldbackground="#ffffff",
        )
        style.configure(
            "Treeview.Heading",
            font=heading_font,
            background="#d6dbe2",
            foreground="#213a59",
            relief="raised",
            padding=[6, 4],
        )
        style.map("Treeview", background=[("selected", "#c5d9f0")])

    def __init__(self):
        self.root = tk.Tk()
        self.root.title("IP Finder — 리튬 BMS")
        self.root.geometry("880x560")
        self.root.minsize(720, 480)

        self._apply_styles()

        outer = tk.Frame(self.root, bg=self._BG_MAIN)
        outer.pack(fill=tk.BOTH, expand=True, padx=14, pady=12)

        header = RoundedHeader(
            outer,
            "리튬 BMS 검색 · 네트워크 설정",
            main_bg=self._BG_MAIN,
            bar_bg=self._BG_HEADER,
            radius=CORNER_FILLET,
        )
        header.pack(fill=tk.X, pady=(0, 12))

        # --- 상단: 네트워크 | TRAP·웹 (나란히) ---
        top_row = ttk.Frame(outer)
        top_row.pack(fill=tk.X, pady=(0, 10))
        top_row.columnconfigure(0, weight=1, uniform="twocol")
        top_row.columnconfigure(1, weight=1, uniform="twocol")

        sheet_net = RoundedSheet(
            top_row,
            "네트워크 정보",
            main_bg=self._BG_MAIN,
            title_fg=self._BG_HEADER,
            radius=CORNER_FILLET,
            inner_padding=(14, 10),
        )
        sheet_net.grid(row=0, column=0, sticky="nsew", padx=(0, 6))
        lf_net = sheet_net.body

        sheet_svc = RoundedSheet(
            top_row,
            "TRAP · 웹 서비스",
            main_bg=self._BG_MAIN,
            title_fg=self._BG_HEADER,
            radius=CORNER_FILLET,
            inner_padding=(14, 10),
        )
        sheet_svc.grid(row=0, column=1, sticky="nsew", padx=(6, 0))
        lf_svc = sheet_svc.body

        labels = ["IPADDRESS", "GATEWAY", "SUBNET", "MAC ADDR", "VERSION"]
        self.entries = {}
        for i, label in enumerate(labels):
            r_label = i * 2
            r_entry = i * 2 + 1
            ttk.Label(lf_net, text=f"{label}").grid(
                row=r_label,
                column=0,
                padx=(0, 6),
                pady=(8 if i else 2, 2),
                sticky="w",
            )
            entry = ttk.Entry(lf_net)
            if label == "VERSION":
                entry.configure(state="readonly")
            entry.grid(row=r_entry, column=0, sticky="ew")
            self.entries[label] = entry
        lf_net.columnconfigure(0, weight=1)

        self.trap_enabled_var = tk.BooleanVar(value=False)
        self.web_enabled_var = tk.BooleanVar(value=True)

        chk_row = ttk.Frame(lf_svc)
        chk_row.pack(fill=tk.X, pady=(0, 10))
        ttk.Checkbutton(chk_row, text="TRAP ENABLE", variable=self.trap_enabled_var).pack(
            side=tk.LEFT, padx=(0, 20)
        )
        ttk.Checkbutton(chk_row, text="WEB ENABLE", variable=self.web_enabled_var).pack(
            side=tk.LEFT, padx=(0, 20)
        )
        ttk.Label(chk_row, text="WEB PORT:").pack(side=tk.LEFT, padx=(8, 4))
        self.web_port_var = tk.StringVar(value="80")
        ttk.Entry(chk_row, textvariable=self.web_port_var, width=7).pack(side=tk.LEFT)

        trap_inner = ttk.Frame(lf_svc)
        trap_inner.pack(fill=tk.X)
        self.trap_entries = []
        for i in range(5):
            ttk.Label(trap_inner, text=f"TRAP IP{i + 1}:").grid(
                row=i, column=0, padx=(0, 10), pady=3, sticky="e"
            )
            trap_entry = ttk.Entry(trap_inner)
            trap_entry.grid(row=i, column=1, padx=(0, 6), pady=3, sticky="ew")
            self.trap_entries.append(trap_entry)
        trap_inner.columnconfigure(1, weight=1)

        trap_btn_row = ttk.Frame(lf_svc)
        trap_btn_row.pack(fill=tk.X, pady=(10, 0))
        trap_btn_row.columnconfigure(0, weight=1)
        trap_btn_row.columnconfigure(2, weight=1)
        trap_btns_mid = ttk.Frame(trap_btn_row)
        trap_btns_mid.grid(row=0, column=1)
        self.trap_test_btn = ttk.Button(
            trap_btns_mid,
            text="트랩 테스트",
            command=self.send_trap_test,
            style="Secondary.TButton",
        )
        self.trap_test_btn.pack(side=tk.LEFT, padx=(0, 8))
        self.setup_btn = ttk.Button(
            trap_btns_mid,
            text="설정 적용",
            command=self.setup_address,
            style="Secondary.TButton",
        )
        self.setup_btn.pack(side=tk.LEFT, padx=(0, 8))
        self.search_btn = ttk.Button(
            trap_btns_mid,
            text="검색",
            command=self.search_devices,
            style="Primary.TButton",
        )
        self.search_btn.pack(side=tk.LEFT)

        # --- 발견된 장비 ---
        sheet_list = RoundedSheet(
            outer,
            "발견된 장비 (행을 클릭하면 위 필드에 반영)",
            main_bg=self._BG_MAIN,
            title_fg=self._BG_HEADER,
            radius=CORNER_FILLET,
            inner_padding=(10, 8),
        )
        sheet_list.pack(fill=tk.BOTH, expand=True, pady=(0, 4))
        lf_list = sheet_list.body

        list_wrap = ttk.Frame(lf_list)
        list_wrap.pack(fill=tk.BOTH, expand=True)
        cols = ("IP Address", "MAC", "NOTE")
        self.tree = ttk.Treeview(
            list_wrap, columns=cols, show="headings", selectmode="browse"
        )
        self.tree.column("IP Address", width=130, anchor="w", stretch=False)
        self.tree.column("MAC", width=160, anchor="w")
        self.tree.column("NOTE", width=200, anchor="w")
        self.tree.heading("IP Address", text="IP Address")
        self.tree.heading("MAC", text="MAC")
        self.tree.heading("NOTE", text="NOTE")
        yscroll = ttk.Scrollbar(list_wrap, orient=tk.VERTICAL, command=self.tree.yview)
        self.tree.configure(yscrollcommand=yscroll.set)
        self.tree.grid(row=0, column=0, sticky="nsew")
        yscroll.grid(row=0, column=1, sticky="ns")
        list_wrap.rowconfigure(0, weight=1)
        list_wrap.columnconfigure(0, weight=1)

        # UDP 소켓 설정
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
        self.sock.bind(('', 0))  # 임의의 포트에 바인딩

        # 수신 스레드 시작
        self.running = True
        self.receiver_thread = threading.Thread(target=self.receive_responses)
        self.receiver_thread.daemon = True
        self.receiver_thread.start()

        # 트리뷰 선택 이벤트 바인딩
        self.tree.bind('<<TreeviewSelect>>', self.on_select)

        self.found_devices = set()
        # 검색 응답으로 받은 장비별 네트워크 정보 (행 선택 시 표시용, 키는 정규화 MAC)
        self.device_info_by_mac = {}
        self.pending_trap_test_msg_id = None

    def search_devices(self):
        # 리스트박스 클리어
        for item in self.tree.get_children():
            self.tree.delete(item)
        self.found_devices.clear()
        self.device_info_by_mac.clear()

        # JSON 형식의 검색 메시지 생성 및 전송
        discovery_message = create_discovery_message()
        self.sock.sendto(discovery_message.encode(), ('255.255.255.255', self.UDP_PORT))

    def receive_responses(self):
        while self.running:
            try:
                data, addr = self.sock.recvfrom(1024)
                response = json.loads(data.decode())
                #print(response)
                
                # JSON 응답 처리
                if response["cmd"] == "TRAP_TEST_RESPONSE":
                    pending = self.pending_trap_test_msg_id
                    if not pending or response.get("msgId") != pending:
                        continue
                    self.pending_trap_test_msg_id = None
                    mac = response.get("mac", "")
                    selected_mac = normalize_mac(self.entries["MAC ADDR"].get())
                    if mac and normalize_mac(mac) != selected_mac:
                        self.root.after(
                            0,
                            lambda: messagebox.showwarning(
                                "트랩 테스트",
                                "다른 장비에서 온 응답입니다. MAC을 확인하세요.",
                            ),
                        )
                        continue
                    status = response.get("status", "")
                    msg = response.get("message", "")
                    text = f"{status}\n{msg}"
                    self.root.after(0, lambda t=text: messagebox.showinfo("트랩 테스트", t))
                    continue

                if response["cmd"] == "DEVICE_RESPONSE":
                    device = response["device"]
                    ip = device["network"]["ip"]
                    mac = device["mac"]
                    version = response.get("ver", "unknown")
                    hostname = response.get("hostname","")
                    note = hostname
                    net = device.get("network") or {}
                    trap_info = (net.get("trap") or {})
                    web_info = (net.get("web") or {})
                    trap_destinations = trap_info.get("destinations") or []
                    trap_destinations = trap_destinations[:5]
                    try:
                        web_port = int(web_info.get("port", 80))
                    except (TypeError, ValueError):
                        web_port = 80
                    if web_port < 1 or web_port > 65535:
                        web_port = 80
                    self.device_info_by_mac[normalize_mac(mac)] = {
                        "subnet": net.get("subnet", ""),
                        "gateway": net.get("gateway", ""),
                        "version": version,
                        "web_enabled": bool(web_info.get("enabled", True)),
                        "web_port": web_port,
                        "trap_enabled": bool(trap_info.get("enabled", False)),
                        "trap_destinations": trap_destinations,
                    }

                    if ip not in self.found_devices:
                        self.found_devices.add(ip)
                        if(hostname==""):
                            note = get_note_for_mac(mac)
                        self.tree.insert('', 'end', values=(ip, mac, note))
                        
                        # 네트워크 정보가 있다면 저장
                        if device["network"].get("subnet"):
                            self.entries['SUBNET'].delete(0, tk.END)
                            self.entries['SUBNET'].insert(0, device["network"]["subnet"])
                        if device["network"].get("gateway"):
                            self.entries['GATEWAY'].delete(0, tk.END)
                            self.entries['GATEWAY'].insert(0, device["network"]["gateway"])
                            
                        # VERSION 정보 업데이트
                        self.entries['VERSION'].configure(state='normal')
                        self.entries['VERSION'].delete(0, tk.END)
                        self.entries['VERSION'].insert(0, version)
                        self.entries['VERSION'].configure(state='readonly')
                            
            except Exception as e:
                print(f"Error receiving response: {e}")
            time.sleep(0.1)

    def on_select(self, event):
        selected_item = self.tree.selection()
        if selected_item:
            item = self.tree.item(selected_item[0])
            values = item.get('values') or []
            ip = values[0] if len(values) > 0 else ""
            mac = values[1] if len(values) > 1 else ""
            # 선택된 항목의 정보를 상단 텍스트박스에 표시
            self.entries['IPADDRESS'].delete(0, tk.END)
            self.entries['IPADDRESS'].insert(0, ip)
            self.entries['MAC ADDR'].delete(0, tk.END)
            self.entries['MAC ADDR'].insert(0, mac)

            info = self.device_info_by_mac.get(normalize_mac(mac))
            if info:
                if info.get("subnet"):
                    self.entries['SUBNET'].delete(0, tk.END)
                    self.entries['SUBNET'].insert(0, info["subnet"])
                if info.get("gateway"):
                    self.entries['GATEWAY'].delete(0, tk.END)
                    self.entries['GATEWAY'].insert(0, info["gateway"])
                self.entries['VERSION'].configure(state='normal')
                self.entries['VERSION'].delete(0, tk.END)
                self.entries['VERSION'].insert(0, info.get("version", "unknown"))
                self.entries['VERSION'].configure(state='readonly')
                self.web_enabled_var.set(bool(info.get("web_enabled", True)))
                self.web_port_var.set(str(info.get("web_port", 80)))
                self.trap_enabled_var.set(bool(info.get("trap_enabled", False)))
                destinations = info.get("trap_destinations", [])
                for i, entry in enumerate(self.trap_entries):
                    entry.delete(0, tk.END)
                    if i < len(destinations):
                        entry.insert(0, destinations[i])

    def setup_address(self):
        # 설정 버튼 클릭 시 처리
        mac = self.entries['MAC ADDR'].get()
        ip = self.entries['IPADDRESS'].get()
        subnet = self.entries['SUBNET'].get()
        gateway = self.entries['GATEWAY'].get()
        web_enabled = self.web_enabled_var.get()
        try:
            web_port = int(self.web_port_var.get().strip() or "80")
        except ValueError:
            web_port = 80
        if web_port < 1 or web_port > 65535:
            web_port = 80
        trap_enabled = self.trap_enabled_var.get()
        trap_destinations = []
        for entry in self.trap_entries:
            value = entry.get().strip()
            if value:
                trap_destinations.append(value)
        
        # JSON 형식의 설정 메시지 생성 및 전송
        config_message = create_config_message(
            mac, ip, subnet, gateway, web_enabled, trap_enabled, trap_destinations, web_port
        )
        self.sock.sendto(config_message.encode(), ('255.255.255.255', self.UDP_PORT))

    def send_trap_test(self):
        mac = self.entries["MAC ADDR"].get().strip()
        if not mac:
            messagebox.showwarning("트랩 테스트", "MAC 주소를 입력하거나 목록에서 장비를 선택하세요.")
            return
        msg_id = generate_message_id()
        self.pending_trap_test_msg_id = msg_id
        payload = create_trap_test_message(mac, msg_id)
        self.sock.sendto(payload.encode(), ("255.255.255.255", self.UDP_PORT))

    def run(self):
        self.root.mainloop()
        self.running = False

def generate_message_id():
    return str(uuid.uuid4())

def create_discovery_message():
    return json.dumps({
        "cmd": "DEVICE_DISCOVERY",
        "ver": "1.0",
        "msgId": generate_message_id(),
        "timestamp": int(time.time())
    })

def create_trap_test_message(mac, msg_id):
    return json.dumps({
        "cmd": "TRAP_TEST",
        "ver": "1.0",
        "msgId": msg_id,
        "timestamp": int(time.time()),
        "target": mac,
    })


def create_config_message(mac, ip, subnet, gateway, web_enabled=True, trap_enabled=False, trap_destinations=None,
                          web_port=80):
    if trap_destinations is None:
        trap_destinations = []
    try:
        port = int(web_port)
    except (TypeError, ValueError):
        port = 80
    if port < 1 or port > 65535:
        port = 80
    return json.dumps({
        "cmd": "SET_NETWORK_CONFIG",
        "ver": "1.0",
        "msgId": generate_message_id(),
        "timestamp": int(time.time()),
        "target": mac,
        "config": {
            "network": {
                "ip": ip,
                "subnet": subnet,
                "gateway": gateway,
                "web": {
                    "enabled": bool(web_enabled),
                    "port": port
                },
                "trap": {
                    "enabled": bool(trap_enabled),
                    "destinations": trap_destinations[:5]
                }
            }
        }
    })

if __name__ == "__main__":
    app = IPFinder()
    if "--autosearch" in sys.argv:
        app.root.after(400, app.search_devices)
    app.run()
