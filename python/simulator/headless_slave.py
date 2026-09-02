#!/usr/bin/env python
# Headless Modbus RTU slave for SNMPToModBus lab test (COM4, 9600).
import sys
import time
import serial
from modbus_tk import modbus_rtu
import modbus_tk.defines as cst

PORT = sys.argv[1] if len(sys.argv) > 1 else "COM4"
SOC = int(sys.argv[2]) if len(sys.argv) > 2 else 85
BAUD = 9600
QTY = 48
DEFAULTS = [0] * QTY
DEFAULTS[0] = 10
DEFAULTS[1] = 1000
DEFAULTS[2] = SOC
DEFAULTS[3] = 98
DEFAULTS[4] = 540
DEFAULTS[5] = 25
DEFAULTS[6] = 3400
DEFAULTS[7] = 3300
DEFAULTS[8] = 250
DEFAULTS[9] = 240
DEFAULTS[12] = 0x07
DEFAULTS[16] = 16
for i in range(16):
    DEFAULTS[17 + i] = 3350 + i
DEFAULTS[31] = 8
for i in range(8):
    DEFAULTS[32 + i] = 250

def main():
    ser = serial.Serial(PORT, BAUD, timeout=0.05)
    server = modbus_rtu.RtuServer(ser)
    server.start()
    slave = server.add_slave(1)
    slave.add_block("hr", cst.HOLDING_REGISTERS, 0, QTY)
    slave.add_block("ir", cst.READ_INPUT_REGISTERS, 0, QTY)
    slave.set_values("hr", 0, DEFAULTS)
    slave.set_values("ir", 0, DEFAULTS)
    print("SLAVE_READY port=%s baud=%s qty=%s SOC=%s" % (PORT, BAUD, QTY, DEFAULTS[2]), flush=True)
    while True:
        time.sleep(1)

if __name__ == "__main__":
    main()
