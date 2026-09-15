#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""펌웨어 USB 업로드 + 웹 파일 + 웹/팩1·2/SNMP 스모크.

  python python/deploy_and_test.py
  python python/deploy_and_test.py --host 192.168.0.65 --port 80
  python python/deploy_and_test.py --skip-fw
  python python/deploy_and_test.py --require-pack-ok
"""
from __future__ import print_function

import argparse
import os
import shutil
import socket
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, ".."))
if HERE not in sys.path:
    sys.path.insert(0, HERE)

from check_bms_web import detect_port, http_json, snmp_get
from upload_web import run as upload_web_run

try:
    from urllib.request import build_opener, HTTPCookieProcessor
    from http.cookiejar import CookieJar
except ImportError:
    from urllib2 import build_opener, HTTPCookieProcessor
    from cookielib import CookieJar

SNMP_OIDS = [
    ("sysDescr", "1.3.6.1.2.1.1.1.0"),
    ("pack1.SOC", "1.3.6.1.2.1.32.1.3.0"),
    ("pack1.PackV", "1.3.6.1.2.1.32.1.8.0"),
    ("pack2.SOC", "1.3.6.1.2.1.32.2.3.0"),
    ("pack2.PackV", "1.3.6.1.2.1.32.2.8.0"),
]


def wait_http(host, ports, seconds):
    """ICMP는 W610 칩이 setup()/loop() 전에 응답할 수 있다. TCP 웹은 loop() 이후."""
    if isinstance(ports, int):
        ports = [ports]
    seen = []
    for p in ports:
        if p and p not in seen:
            seen.append(p)
    end = time.time() + seconds
    last = None
    n = 0
    while time.time() < end:
        n += 1
        for p in seen:
            try:
                s = socket.create_connection((host, p), 2)
                s.close()
                print("웹 TCP %s:%s 열림 (시도 %d)" % (host, p, n), flush=True)
                return p
            except OSError as e:
                last = e
        left = int(end - time.time())
        if n == 1 or n % 5 == 0:
            print("  ping만 되는 중. HTTP는 setup(LCD/BLE) 후 loop에서 받음. TCP %s %s  남은 %ds" % (
                host, seen, left), flush=True)
        time.sleep(1.0)
    print("웹 대기 실패 %s %s (%s)" % (host, seen, last), flush=True)
    return None


def pio_upload(env):
    pio = shutil.which("pio") or shutil.which("platformio")
    if not pio:
        print("pio 없음 (PlatformIO CLI)")
        return False
    print("=== firmware pio run -e %s -t upload ===" % env)
    rc = subprocess.call([pio, "run", "-e", env, "-t", "upload"], cwd=ROOT)
    if rc != 0:
        print("업로드 실패 rc=%s  (pio device monitor 가 COM 을 잡고 있으면 끄세요)" % rc)
        return False
    return True


def test_web(opener, base):
    ok = True
    code, _ = http_json(opener, base + "/Login.html")
    print("[WEB] GET /Login.html %s" % code)
    if code != 200:
        ok = False
    code, j = http_json(opener, base + "/api/login", {"userid": "admin", "passwd": "admin"})
    print("[WEB] login %s %s" % (code, j))
    if code != 200:
        ok = False
    code, page = http_json(opener, base + "/INDEX.HTML")
    print("[WEB] GET /INDEX.HTML %s" % code)
    if code != 200:
        ok = False
    return ok


def test_packs(opener, base, require_ok):
    code, bms = http_json(opener, base + "/api/bms")
    print("[PACK] GET /api/bms %s" % code)
    if code != 200 or not isinstance(bms, dict) or not bms.get("packs"):
        print("[PACK] FAIL 본문")
        return False
    packs = bms["packs"]
    if len(packs) < 2:
        print("[PACK] FAIL 팩 개수 %d (1·2 필요)" % len(packs))
        return False
    all_ok = True
    for i in (0, 1):
        p = packs[i]
        comm = bool(p.get("ok"))
        print("[PACK] 모듈 %d ok=%s V=%s I=%s SOC=%s alarm=%s" % (
            i + 1, p.get("ok"), p.get("volt"), p.get("amp"), p.get("soc"), p.get("alarm")))
        if require_ok and not comm:
            all_ok = False
    if not require_ok:
        if not packs[0].get("ok"):
            print("[PACK] WARN 모듈 1 통신 없음")
        if not packs[1].get("ok"):
            print("[PACK] WARN 모듈 2 통신 없음 (슬레이브 미접속이면 정상)")
    return all_ok


def test_snmp(host):
    ok = True
    for name, oid in SNMP_OIDS:
        val = snmp_get(host, oid)
        if val is None:
            print("[SNMP] FAIL %s %s" % (name, oid))
            ok = False
        else:
            print("[SNMP] OK %s = %s" % (name, val))
    return ok


def main():
    ap = argparse.ArgumentParser(description="펌웨어+웹 업로드 후 웹/팩1·2/SNMP 테스트")
    ap.add_argument("--host", default="192.168.0.65")
    ap.add_argument("--port", type=int, default=80)
    ap.add_argument("--env", default="esp32_samwoo")
    ap.add_argument("--skip-fw", action="store_true", help="pio upload 생략")
    ap.add_argument("--skip-webfiles", action="store_true", help="UploadFiles SPIFFS 생략")
    ap.add_argument("--require-pack-ok", action="store_true",
                    help="팩1·2 RS-485 ok=true 필수 (기본은 조회만, 미접속은 WARN)")
    ap.add_argument("--wait", type=int, default=180, help="펌웨어 후 HTTP(TCP) 대기 초")
    args = ap.parse_args()

    if not args.skip_fw:
        if not pio_upload(args.env):
            return 1
        print("재부팅 후 HTTP 대기 %s 포트 %s/80/81 (%ds). ping 만으로는 웹이 아직 없을 수 있음." % (
            args.host, args.port, args.wait), flush=True)
        opened = wait_http(args.host, [args.port, 80, 81], args.wait)
        if not opened:
            return 1
        args.port = opened
    else:
        port = detect_port(args.host, args.port)
        if port is None:
            print("웹 포트 없음", args.host)
            return 1
        args.port = port
        print("웹 포트", args.port)

    if not args.skip_webfiles:
        print("=== UploadFiles → SPIFFS ===")
        if not upload_web_run(args.host, args.port):
            return 1

    base = "http://%s:%s" % (args.host, args.port)
    cj = CookieJar()
    opener = build_opener(HTTPCookieProcessor(cj))

    print("=== 웹 ===")
    web_ok = test_web(opener, base)
    print("=== 모듈 1·2 (디스플레이 RS-485) ===")
    pack_ok = test_packs(opener, base, args.require_pack_ok)
    print("=== SNMP ===")
    snmp_ok = test_snmp(args.host)

    failed = []
    if not web_ok:
        failed.append("web")
    if not pack_ok:
        failed.append("pack")
    if not snmp_ok:
        failed.append("snmp")
    if failed:
        print("FAIL %s" % ",".join(failed))
        return 1
    print("PASS firmware+web+pack1/2+snmp")
    return 0


if __name__ == "__main__":
    sys.exit(main())
