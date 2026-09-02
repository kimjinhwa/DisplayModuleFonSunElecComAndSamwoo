#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""SNMPv2c GET 확인. 기본 IP 192.168.0.57 community public."""
import sys

HOST = sys.argv[1] if len(sys.argv) > 1 else "192.168.0.57"
OIDS = [
    ("sysDescr", "1.3.6.1.2.1.1.1.0"),
    ("pack1.SOC", "1.3.6.1.2.1.32.1.3.0"),
    ("pack1.PackV", "1.3.6.1.2.1.32.1.8.0"),
    ("pack1.cell1", "1.3.6.1.2.1.32.1.1.1.0"),
    ("pack2.SOC", "1.3.6.1.2.1.32.2.3.0"),
    ("pack2.PackV", "1.3.6.1.2.1.32.2.8.0"),
]


def main():
    try:
        from pysnmp.hlapi import (
            CommunityData,
            ContextData,
            ObjectIdentity,
            ObjectType,
            SnmpEngine,
            UdpTransportTarget,
            getCmd,
        )
    except ImportError:
        print("pip install pysnmp")
        sys.exit(1)

    ok = 0
    for name, oid in OIDS:
        err, status, idx, varBinds = next(
            getCmd(
                SnmpEngine(),
                CommunityData("public", mpModel=1),
                UdpTransportTarget((HOST, 161), timeout=2, retries=1),
                ContextData(),
                ObjectType(ObjectIdentity(oid)),
            )
        )
        if err:
            print("FAIL %s %s : %s" % (name, oid, err))
            continue
        if status:
            print("FAIL %s %s : %s" % (name, oid, status.prettyPrint()))
            continue
        for vb in varBinds:
            print("OK   %s = %s" % (name, vb[1].prettyPrint()))
            ok += 1
    print("got %d / %d" % (ok, len(OIDS)))
    sys.exit(0 if ok == len(OIDS) else 2)


if __name__ == "__main__":
    main()
