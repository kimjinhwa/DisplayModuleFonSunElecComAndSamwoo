#ifndef BOARD_RTC_H
#define BOARD_RTC_H

#include <Arduino.h>

struct BoardRtcTime
{
  uint16_t year;
  uint8_t month;
  uint8_t day;
  uint8_t hour;
  uint8_t minute;
  uint8_t second;
  bool halted;
};

bool boardRtcRead(BoardRtcTime *t);
bool boardRtcWrite(const BoardRtcTime &t);
void boardRtcApplyToEsp(const BoardRtcTime &t);
bool boardRtcSyncEsp(void);

#endif
