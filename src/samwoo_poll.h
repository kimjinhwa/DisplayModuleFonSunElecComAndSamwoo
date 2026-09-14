#ifndef SAMWOO_POLL_H
#define SAMWOO_POLL_H

#include <Arduino.h>

#define SAMWOO_PACKS 2
#define SAMWOO_REG_MAX 48
#define SAMWOO_REQ_START 1
#define SAMWOO_REG_VER 0
#define SAMWOO_REG_CAP 1
#define SAMWOO_REG_SOC 2
#define SAMWOO_REG_SOH 3
#define SAMWOO_REG_VOLT 4
#define SAMWOO_REG_CUR 5
#define SAMWOO_REG_VMAX 6
#define SAMWOO_REG_VMIN 7
#define SAMWOO_REG_TMAX 8
#define SAMWOO_REG_TMIN 9
#define SAMWOO_REG_RELAY 10
#define SAMWOO_REG_FAULT 11
#define SAMWOO_REG_PROTECT 12
#define SAMWOO_REG_WARNING 13
#define SAMWOO_REG_CELLNUM 14
#define SAMWOO_REG_CELL0 15
#define SAMWOO_REG_CELLS 16
#define SAMWOO_REG_TEMPNUM 31
#define SAMWOO_REG_TEMP0 32
#define SAMWOO_REG_TEMPS 8

extern uint16_t samwooRegs[SAMWOO_PACKS][SAMWOO_REG_MAX];
extern bool samwooOk[SAMWOO_PACKS];
extern uint32_t samwooLastOkMs[SAMWOO_PACKS];
extern uint32_t samwooErrorCount;

void samwooBegin();
bool samwooPoll();
bool samwooPollTick();
void samwooFillNarada();
uint16_t samwooReg(uint8_t pack, uint16_t address);

#endif
