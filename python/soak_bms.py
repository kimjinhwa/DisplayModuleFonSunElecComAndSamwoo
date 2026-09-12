"""웹 + SNMP + RS-485 동시 부하. SNMPToModBusCommon soak_test 와 같은 목적.

  python -u python/soak_bms.py --host 192.168.0.65 --port 81 --hours 1
"""
from __future__ import print_function

import argparse
import csv
import json
import os
import time
from datetime import datetime

try:
    from urllib.request import Request, urlopen, build_opener, HTTPCookieProcessor
    from http.cookiejar import CookieJar
except ImportError:
    from urllib2 import Request, urlopen, build_opener, HTTPCookieProcessor
    from cookielib import CookieJar


def snmp_get(host, oid):
    from pysnmp.hlapi import (
        getCmd, SnmpEngine, CommunityData, UdpTransportTarget,
        ContextData, ObjectType, ObjectIdentity,
    )
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
        raise RuntimeError(str(err or status))
    return int(var[0][1])


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--host", default="192.168.0.65")
    ap.add_argument("--port", type=int, default=81)
    ap.add_argument("--hours", type=float, default=1.0)
    ap.add_argument("--interval", type=float, default=2.0)
    ap.add_argument("--snmp-every", type=int, default=5)
    args = ap.parse_args()

    here = os.path.dirname(os.path.abspath(__file__))
    logdir = os.path.join(here, "soak_logs")
    os.makedirs(logdir, exist_ok=True)
    stamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    log_path = os.path.join(logdir, "soak_%s.log" % stamp)
    csv_path = os.path.join(logdir, "soak_%s.csv" % stamp)
    hb_path = os.path.join(logdir, "heartbeat.txt")

    cj = CookieJar()
    opener = build_opener(HTTPCookieProcessor(cj))
    base = "http://%s:%s" % (args.host, args.port)
    req = Request(base + "/api/login", data=json.dumps({"userid": "admin", "passwd": "admin"}).encode("utf-8"))
    req.add_header("Content-Type", "application/json")
    opener.open(req, timeout=8).read()

    end = time.time() + args.hours * 3600
    n = 0
    http_err = 0
    snmp_err = 0
    start = time.time()
    with open(csv_path, "w", newline="") as cf, open(log_path, "w") as lf:
        w = csv.writer(cf)
        w.writerow(["t", "http_ms", "snmp_ms", "pack1_v", "ok1", "ok2", "err"])
        while time.time() < end:
            n += 1
            t0 = time.time()
            pack1_v = ""
            ok1 = ok2 = ""
            err_n = ""
            http_ms = -1
            try:
                r = opener.open(base + "/api/bms", timeout=6)
                raw = r.read()
                http_ms = (time.time() - t0) * 1000
                j = json.loads(raw.decode("utf-8", "replace"))
                packs = j.get("packs") or []
                if packs:
                    ok1 = packs[0].get("ok")
                    pack1_v = packs[0].get("volt")
                if len(packs) > 1:
                    ok2 = packs[1].get("ok")
                err_n = j.get("err")
            except Exception as e:
                http_ms = -1
                http_err += 1
                lf.write("HTTP %s\n" % e)
            snmp_ms = ""
            if n % args.snmp_every == 0:
                t1 = time.time()
                try:
                    snmp_get(args.host, "1.3.6.1.2.1.32.1.8.0")
                    snmp_ms = (time.time() - t1) * 1000
                except Exception as e:
                    snmp_ms = -1
                    snmp_err += 1
                    lf.write("SNMP %s\n" % e)
            w.writerow([round(time.time() - start, 1), http_ms, snmp_ms, pack1_v, ok1, ok2, err_n])
            cf.flush()
            open(hb_path, "w").write("%s n=%d http_err=%d snmp_err=%d\n" % (
                datetime.now().isoformat(), n, http_err, snmp_err))
            if n % 30 == 0:
                print("n=%d http_err=%d snmp_err=%d last_http=%.0fms" % (n, http_err, snmp_err, http_ms))
            time.sleep(args.interval)

    result = "PASS" if http_err == 0 and snmp_err == 0 else "FAIL"
    print("RESULT %s  http_err=%d snmp_err=%d  log=%s" % (result, http_err, snmp_err, log_path))
    return 0 if result == "PASS" else 1


if __name__ == "__main__":
    raise SystemExit(main())
