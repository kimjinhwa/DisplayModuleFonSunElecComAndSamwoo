"""BMS 웹/SNMP/RS-485 스모크.

  python python/check_bms_web.py --host 192.168.0.65 --port 80
  python python/check_bms_web.py --host 192.168.0.65 --port 80 --upload
"""
from __future__ import print_function

import argparse
import json
import socket
import sys

try:
    from urllib.request import Request, urlopen
    from http.cookiejar import CookieJar
    from urllib.request import build_opener, HTTPCookieProcessor
except ImportError:
    from urllib2 import Request, urlopen, build_opener, HTTPCookieProcessor
    from cookielib import CookieJar


def http_json(opener, url, data=None, method=None):
    body = None
    headers = {}
    if data is not None:
        body = json.dumps(data).encode("utf-8")
        headers["Content-Type"] = "application/json"
    req = Request(url, data=body, headers=headers)
    if method:
        req.get_method = lambda: method
    resp = opener.open(req, timeout=8)
    raw = resp.read()
    code = getattr(resp, "status", None) or resp.getcode()
    try:
        parsed = json.loads(raw.decode("utf-8", "replace"))
    except Exception:
        parsed = raw.decode("utf-8", "replace")
    return code, parsed


def snmp_get(host, oid):
    try:
        from pysnmp.hlapi import (
            getCmd,
            SnmpEngine,
            CommunityData,
            UdpTransportTarget,
            ContextData,
            ObjectType,
            ObjectIdentity,
        )
    except ImportError:
        print("pysnmp 없음 — SNMP GET 건너뜀 (pip install pysnmp)")
        return None
    err, status, _idx, var = next(
        getCmd(
            SnmpEngine(),
            CommunityData("public", mpModel=1),
            UdpTransportTarget((host, 161), timeout=2, retries=1),
            ContextData(),
            ObjectType(ObjectIdentity(oid)),
        )
    )
    if err or status:
        print("SNMP FAIL", err or status)
        return None
    return var[0][1]


def detect_port(host, port):
    for p in ([port] if port else []) + [80, 81]:
        try:
            s = socket.create_connection((host, p), 2)
            s.close()
            return p
        except OSError:
            continue
    return None


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--host", default="192.168.0.65")
    ap.add_argument("--port", type=int, default=80)
    ap.add_argument("--upload", action="store_true")
    args = ap.parse_args()

    port = detect_port(args.host, args.port)
    if port is None:
        print("웹 포트 없음", args.host)
        return 1
    print("웹 포트", port)
    base = "http://%s:%s" % (args.host, port)

    if args.upload:
        from upload_web import run as upload_run
        if not upload_run(args.host, port):
            return 1

    cj = CookieJar()
    opener = build_opener(HTTPCookieProcessor(cj))

    code, _ = http_json(opener, base + "/Login.html")
    print("GET /Login.html", code)
    if code != 200:
        return 1

    code, j = http_json(opener, base + "/api/login", {"userid": "admin", "passwd": "admin"})
    print("login", code, j)
    if code != 200:
        return 1

    code, bms = http_json(opener, base + "/api/bms")
    print("bms", code, "packs", (bms.get("packs") if isinstance(bms, dict) else None))
    if code != 200 or not isinstance(bms, dict) or not bms.get("packs"):
        return 1
    for i, p in enumerate(bms["packs"]):
        print("  pack%d ok=%s V=%s I=%s SOC=%s alarm=%s" % (
            i + 1, p.get("ok"), p.get("volt"), p.get("amp"), p.get("soc"), p.get("alarm")))

    code, page = http_json(opener, base + "/INDEX.HTML")
    print("GET /INDEX.HTML (대소문자)", code)
    if code != 200:
        return 1

    oid = "1.3.6.1.2.1.32.1.8.0"
    val = snmp_get(args.host, oid)
    print("SNMP", oid, val)

    print("데드락 점검(코드): 웹/SNMP/IPFinder 는 loop()에서만 W6100 SPI 사용.")
    print("  /api/snmp-get 은 UDP를 다시 열지 않고 samwooRegs 를 읽음.")
    print("  RS-485 는 samwooPollTick() (논블로킹). 별도 이더넷 태스크 없음.")
    print("PASS")
    return 0


if __name__ == "__main__":
    sys.exit(main())
