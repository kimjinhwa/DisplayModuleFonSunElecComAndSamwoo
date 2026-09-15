#include "board_rtc.h"
#include "board_pins.h"
#include <Wire.h>
#include <sys/time.h>
#include <time.h>

static uint8_t bin2bcd(uint8_t v) { return (uint8_t)(((v / 10) << 4) | (v % 10)); }
static uint8_t bcd2bin(uint8_t v) { return (uint8_t)(((v >> 4) * 10) + (v & 0x0F)); }

bool boardRtcRead(BoardRtcTime *t)
{
  if (t == nullptr)
    return false;
  Wire.setClock(100000);
  Wire.beginTransmission(DS1307_I2C_ADDR);
  Wire.write(0x00);
  if (Wire.endTransmission(false) != 0)
  {
    Wire.setClock(400000);
    return false;
  }
  if (Wire.requestFrom((int)DS1307_I2C_ADDR, 7) != 7)
  {
    Wire.setClock(400000);
    return false;
  }
  const uint8_t sec = Wire.read();
  const uint8_t minute = Wire.read();
  const uint8_t hour = Wire.read();
  Wire.read();
  const uint8_t day = Wire.read();
  const uint8_t month = Wire.read();
  const uint8_t year = Wire.read();
  t->halted = (sec & 0x80) != 0;
  t->second = bcd2bin(sec & 0x7F);
  t->minute = bcd2bin(minute);
  t->hour = bcd2bin(hour & 0x3F);
  t->day = bcd2bin(day);
  t->month = bcd2bin(month);
  t->year = (uint16_t)(2000 + bcd2bin(year));
  Wire.setClock(400000);
  return true;
}

bool boardRtcWrite(const BoardRtcTime &t)
{
  Wire.setClock(100000);
  Wire.beginTransmission(DS1307_I2C_ADDR);
  Wire.write(0x00);
  Wire.write(bin2bcd(t.second) & 0x7F);
  Wire.write(bin2bcd(t.minute));
  Wire.write(bin2bcd(t.hour));
  Wire.write(0x01);
  Wire.write(bin2bcd(t.day));
  Wire.write(bin2bcd(t.month));
  Wire.write(bin2bcd((uint8_t)(t.year % 100)));
  const bool ok = Wire.endTransmission() == 0;
  Wire.setClock(400000);
  if (ok)
  {
    boardRtcApplyToEsp(t);
  }
  return ok;
}

void boardRtcApplyToEsp(const BoardRtcTime &t)
{
  struct tm tm = {};
  tm.tm_year = (int)t.year - 1900;
  tm.tm_mon = (int)t.month - 1;
  tm.tm_mday = t.day;
  tm.tm_hour = t.hour;
  tm.tm_min = t.minute;
  tm.tm_sec = t.second;
  tm.tm_isdst = -1;
  const time_t sec = mktime(&tm);
  if (sec == (time_t)-1)
  {
    return;
  }
  struct timeval tv;
  tv.tv_sec = sec;
  tv.tv_usec = 0;
  settimeofday(&tv, NULL);
}

bool boardRtcSyncEsp(void)
{
  BoardRtcTime t;
  if (!boardRtcRead(&t))
  {
    return false;
  }
  if (t.halted)
  {
    return false;
  }
  if (t.year < 2000 || t.month < 1 || t.month > 12)
  {
    return false;
  }
  boardRtcApplyToEsp(t);
  return true;
}
