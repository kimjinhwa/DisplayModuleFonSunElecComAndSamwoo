"""UploadFiles/ 를 장비 SPIFFS로 올린다. POST /upload, 필드명 update.

  python python/upload_web.py --host 192.168.0.65 --port 81
"""
from __future__ import print_function

import argparse
import os
import socket
import sys
import time

try:
    from urllib.request import Request, urlopen
    from urllib.error import URLError, HTTPError
except ImportError:
    from urllib2 import Request, urlopen, URLError, HTTPError


SKIP_EXT = (".lnk", ".bak", ".tmp", ".md")
SKIP_NAME = {"deploy", "readme.txt"}
BOUNDARY = "----SamwooWebUpload7f3a"


def should_skip(name):
    lower = name.lower()
    if name.startswith(".") or name.endswith("~"):
        return True
    if lower in SKIP_NAME:
        return True
    if os.path.splitext(lower)[1] in SKIP_EXT:
        return True
    return False


def collect_files(folder):
    out = []
    for name in sorted(os.listdir(folder)):
        path = os.path.join(folder, name)
        if not os.path.isfile(path) or should_skip(name):
            continue
        out.append(path)
    return out


def wait_http(host, port, seconds=8):
    end = time.time() + seconds
    last = None
    while time.time() < end:
        try:
            s = socket.create_connection((host, port), 2)
            s.close()
            return
        except OSError as e:
            last = e
            time.sleep(0.4)
    raise last


def _post_once(host, port, path):
    name = os.path.basename(path)
    with open(path, "rb") as f:
        data = f.read()
    head = (
        "--%s\r\n" % BOUNDARY
        + 'Content-Disposition: form-data; name="update"; filename="%s"\r\n' % name
        + "Content-Type: application/octet-stream\r\n\r\n"
    )
    body = head.encode("ascii") + data + ("\r\n--%s--\r\n" % BOUNDARY).encode("ascii")
    url = "http://%s:%s/upload" % (host, port)
    req = Request(url, data=body)
    req.add_header("Content-Type", "multipart/form-data; boundary=%s" % BOUNDARY)
    req.add_header("Content-Length", str(len(body)))
    req.add_header("Connection", "close")
    resp = urlopen(req, timeout=30)
    code = getattr(resp, "status", None) or resp.getcode()
    text = resp.read()
    resp.close()
    return code, text


def post_file(host, port, path, retries=3):
    last = None
    for _ in range(retries):
        try:
            return _post_once(host, port, path)
        except (URLError, HTTPError, OSError) as e:
            last = e
            time.sleep(1.0)
    raise last


def run(host, port, folder=None):
    here = os.path.dirname(os.path.abspath(__file__))
    if folder is None:
        folder = os.path.normpath(os.path.join(here, "..", "UploadFiles"))
    if not os.path.isdir(folder):
        print("폴더 없음: %s" % folder, file=sys.stderr)
        return False
    files = collect_files(folder)
    if not files:
        print("올릴 파일 없음: %s" % folder, file=sys.stderr)
        return False
    try:
        wait_http(host, port, 10)
    except OSError as e:
        print("연결 안 됨 %s:%s  (%s)" % (host, port, e), file=sys.stderr)
        return False
    print("대상 http://%s:%s/upload  (%d개)" % (host, port, len(files)))
    ok = 0
    for path in files:
        name = os.path.basename(path)
        size = os.path.getsize(path)
        try:
            code, text = post_file(host, port, path)
            body = text.decode("utf-8", "replace").strip()
            good = code == 200
            print("  %s  %5d B  HTTP %s  %s" % (name, size, code, body or "(empty)"))
            if good:
                ok += 1
        except (URLError, HTTPError, OSError) as e:
            print("  %s  FAIL  %s" % (name, e), file=sys.stderr)
    print("%d/%d 성공" % (ok, len(files)))
    return ok == len(files)


def main():
    here = os.path.dirname(os.path.abspath(__file__))
    default_dir = os.path.normpath(os.path.join(here, "..", "UploadFiles"))
    ap = argparse.ArgumentParser(description="SPIFFS 웹 파일 업로드")
    ap.add_argument("--host", default="192.168.0.65")
    ap.add_argument("--port", type=int, default=81)
    ap.add_argument("--dir", default=default_dir)
    args = ap.parse_args()
    return 0 if run(args.host, args.port, args.dir) else 1


if __name__ == "__main__":
    sys.exit(main())
