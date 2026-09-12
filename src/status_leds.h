#ifndef STATUS_LEDS_H
#define STATUS_LEDS_H

#include <Arduino.h>

void statusLedsBegin();
void statusLedsLoop();
bool statusLedsHasAlarm(int pack);
const char *statusLedsAlarmCause(int pack);

#endif
