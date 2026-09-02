#ifndef SAMWOO_POLL_H
#define SAMWOO_POLL_H

#include <Arduino.h>

#define SAMWOO_PACKS 2
#define SAMWOO_REG_MAX 48

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
