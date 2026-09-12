# -*- coding: utf-8 -*-
"""장비 웹을 로그인해서 Chrome/Edge 헤드리스로 캡처한다. 시스템 한글 폰트를 쓴다."""
from __future__ import print_function

import json
import os
import socket
import subprocess
import sys
import threading
import time

try:
    from http.server import BaseHTTPRequestHandler, HTTPServer
    from urllib.request import Request, urlopen
    from urllib.error import URLError, HTTPError
except ImportError:
    from BaseHTTPServer import BaseHTTPRequestHandler, HTTPServer
    from urllib2 import Request, urlopen, URLError, HTTPError

HERE = os.path.dirname(os.path.abspath(__file__))
IMG = os.path.join(HERE, "img")
HOST = os.environ.get("SAMWOO_WEB_HOST", "192.168.0.65")
PORT = int(os.environ.get("SAMWOO_WEB_PORT", "81"))
PROXY_PORT = 18765
SESSION = [""]


def find_browser():
    cands = [
        os.path.join(os.environ.get("ProgramFiles", r"C:\Program Files"), r"Google\Chrome\Application\chrome.exe"),
        os.path.join(os.environ.get("ProgramFiles(x86)", r"C:\Program Files (x86)"), r"Google\Chrome\Application\chrome.exe"),
        os.path.join(os.environ.get("LOCALAPPDATA", ""), r"Google\Chrome\Application\chrome.exe"),
        os.path.join(os.environ.get("ProgramFiles", r"C:\Program Files"), r"Microsoft\Edge\Application\msedge.exe"),
        os.path.join(os.environ.get("ProgramFiles(x86)", r"C:\Program Files (x86)"), r"Microsoft\Edge\Application\msedge.exe"),
    ]
    for p in cands:
        if p and os.path.isfile(p):
            return p
    raise SystemExit("Chrome 또는 Edge를 찾을 수 없습니다.")


def login():
    body = json.dumps({"userid": "admin", "passwd": "admin"}).encode("utf-8")
    req = Request("http://%s:%s/api/login" % (HOST, PORT), data=body)
    req.add_header("Content-Type", "application/json")
    try:
        resp = urlopen(req, timeout=8)
    except (URLError, HTTPError) as e:
        raise SystemExit("로그인 실패: %s" % e)
    cookie = resp.headers.get("Set-Cookie") or ""
    session = ""
    for part in cookie.split(";"):
        part = part.strip()
        if part.startswith("session="):
            session = part.split("=", 1)[1]
    if not session:
        raise SystemExit("세션 쿠키가 없습니다.")
    SESSION[0] = session
    print("login ok session=%s..." % session[:8])


class Proxy(BaseHTTPRequestHandler):
    def log_message(self, fmt, *args):
        return

    def do_GET(self):
        self._fwd()

    def do_POST(self):
        self._fwd()

    def _fwd(self):
        length = int(self.headers.get("Content-Length") or 0)
        data = self.rfile.read(length) if length else None
        url = "http://%s:%s%s" % (HOST, PORT, self.path)
        req = Request(url, data=data)
        for k in ("Content-Type", "Accept"):
            if self.headers.get(k):
                req.add_header(k, self.headers.get(k))
        if SESSION[0]:
            req.add_header("Cookie", "session=" + SESSION[0])
        try:
            resp = urlopen(req, timeout=12)
            raw = resp.read()
            code = getattr(resp, "status", None) or resp.getcode()
            ctype = resp.headers.get("Content-Type") or "text/html"
        except HTTPError as e:
            raw = e.read()
            code = e.code
            ctype = e.headers.get("Content-Type") or "text/html"
        except URLError as e:
            self.send_error(502, str(e))
            return
        self.send_response(code)
        self.send_header("Content-Type", ctype)
        self.send_header("Content-Length", str(len(raw)))
        self.end_headers()
        self.wfile.write(raw)


def start_proxy():
    httpd = HTTPServer(("127.0.0.1", PROXY_PORT), Proxy)
    t = threading.Thread(target=httpd.serve_forever)
    t.daemon = True
    t.start()
    return httpd


def chrome_shot(browser, url, out_path, w=1280, h=900):
    profile = os.path.join(os.environ.get("TEMP", "."), "samwoo_manual_shot")
    if not os.path.isdir(profile):
        os.makedirs(profile)
    args = [
        browser,
        "--headless=new",
        "--disable-gpu",
        "--no-sandbox",
        "--hide-scrollbars",
        "--force-device-scale-factor=1",
        "--window-size=%d,%d" % (w, h),
        "--user-data-dir=" + profile,
        "--virtual-time-budget=5000",
        "--run-all-compositor-stages-before-draw",
        "--screenshot=" + os.path.abspath(out_path),
        url,
    ]
    print("shot", os.path.basename(out_path), url)
    subprocess.check_call(args)
    if not os.path.isfile(out_path):
        raise SystemExit("캡처 실패: " + out_path)


def main():
    if not os.path.isdir(IMG):
        os.makedirs(IMG)
    login()
    start_proxy()
    time.sleep(0.3)
    browser = find_browser()
    base = "http://127.0.0.1:%d" % PROXY_PORT
    shots = [
        ("http://%s:%s/login.html" % (HOST, PORT), "web_01_login.png", 900, 700),
        (base + "/index.html", "web_02_monitor.png", 1280, 900),
        (base + "/settings.html", "web_03_settings.png", 1280, 980),
        (base + "/snmpTest.html", "web_04_snmp.png", 1280, 980),
        (base + "/help.html", "web_05_help.png", 1280, 980),
        (base + "/fileUpload", "web_06_fileupload.png", 1000, 720),
    ]
    for url, name, w, h in shots:
        chrome_shot(browser, url, os.path.join(IMG, name), w, h)
    print("done", IMG)


if __name__ == "__main__":
    main()
