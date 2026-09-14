#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""샘플 HEX만 보거나 한 프레임만 보낸다. 실팩 전체 점검은 samwoo_lab.py."""
from __future__ import print_function

import argparse
import sys

from samwoo_proto import DOC_TX_HEX, build_request, hex_spaces


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--slave", type=int, default=1)
    args = ap.parse_args()
    tx = build_request(args.slave, 0x04, 1, 48)
    print("TX start=1  %s" % hex_spaces(tx))
    print("            기대 %s" % DOC_TX_HEX)
    print()
    print("실팩:  python python/samwoo_lab.py --port COM14")
    return 0


if __name__ == "__main__":
    sys.exit(main())
