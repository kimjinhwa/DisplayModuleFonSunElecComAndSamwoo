// DS1307Z H/W 테스트. USB Serial(PuTTY 115200)만 사용.
//   pio run -e hwtest_rtc -t upload
// 부팅 시에는 읽기만 한다. 배터리 백업 확인:
//   1) set 으로 시각 기록
//   2) 전원 OFF → ON
//   3) 부팅 직후 시각이 꺼진 동안 흐른 값이면 VBAT 정상
// USB:
//   set 2026-08-31 18:30:00

#include <Arduino.h>
#include <Wire.h>
#include "board_pins.h"

#ifndef BAUDRATEDEF
#define BAUDRATEDEF 115200
#endif

static uint8_t bin2bcd(uint8_t v) { return (uint8_t)(((v / 10) << 4) | (v % 10)); }
static uint8_t bcd2bin(uint8_t v) { return (uint8_t)(((v >> 4) * 10) + (v & 0x0F)); }

struct RtcTime
{
  uint16_t year;
  uint8_t month;
  uint8_t day;
  uint8_t hour;
  uint8_t minute;
  uint8_t second;
  bool halted;
};

static bool rtcRead(RtcTime *t)
{
  Wire.beginTransmission(DS1307_I2C_ADDR);
  Wire.write(0x00);
  if (Wire.endTransmission(false) != 0)
  {
    return false;
  }
  if (Wire.requestFrom((int)DS1307_I2C_ADDR, 7) != 7)
  {
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
  return true;
}

static bool rtcWrite(const RtcTime &t)
{
  Wire.beginTransmission(DS1307_I2C_ADDR);
  Wire.write(0x00);
  Wire.write(bin2bcd(t.second) & 0x7F);
  Wire.write(bin2bcd(t.minute));
  Wire.write(bin2bcd(t.hour));
  Wire.write(0x01);
  Wire.write(bin2bcd(t.day));
  Wire.write(bin2bcd(t.month));
  Wire.write(bin2bcd((uint8_t)(t.year % 100)));
  return Wire.endTransmission() == 0;
}

static void printTime(const char *tag, const RtcTime &t)
{
  Serial.printf("%s %04u-%02u-%02u %02u:%02u:%02u%s\n", tag, t.year, t.month, t.day,
                t.hour, t.minute, t.second, t.halted ? "  HALTED(CH=1)" : "");
}

static void i2cScan()
{
  Serial.println("[RTC] I2C scan SCL=20 SDA=19");
  uint8_t found = 0;
  for (uint8_t addr = 1; addr < 127; addr++)
  {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0)
    {
      Serial.printf("  found 0x%02X", addr);
      if (addr == DS1307_I2C_ADDR)
      {
        Serial.print(" (DS1307)");
      }
      if (addr == 0x5D || addr == 0x14)
      {
        Serial.print(" (GT911?)");
      }
      Serial.println();
      found++;
    }
  }
  if (found == 0)
  {
    Serial.println("  no devices");
  }
}

static bool parseSetLine(const char *line, RtcTime *t)
{
  int year, month, day, hour, minute, second;
  if (sscanf(line, "set %d-%d-%d %d:%d:%d", &year, &month, &day, &hour, &minute, &second) != 6)
  {
    return false;
  }
  t->year = (uint16_t)year;
  t->month = (uint8_t)month;
  t->day = (uint8_t)day;
  t->hour = (uint8_t)hour;
  t->minute = (uint8_t)minute;
  t->second = (uint8_t)second;
  return true;
}

void setup()
{
  Serial.begin(BAUDRATEDEF);
  delay(300);
  Serial.println();
  Serial.println("=== hwtest_rtc DS1307Z ===");
  Serial.println("boot: read only. USB: set YYYY-MM-DD HH:MM:SS");

  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  Wire.setClock(100000);
  i2cScan();

  RtcTime nowTime;
  if (!rtcRead(&nowTime))
  {
    Serial.println("[RTC] FAIL: 0x68 read (chip missing or bus error)");
    return;
  }
  printTime("[RTC] power-on", nowTime);
  if (nowTime.halted)
  {
    Serial.println("[RTC] oscillator stopped. set time once to start, then power-cycle to test VBAT.");
  }
  else
  {
    Serial.println("[RTC] oscillator running. power-cycle without set — time should keep advancing.");
  }
}

void loop()
{
  static uint32_t lastMs = 0;
  static char line[40];
  static uint8_t lineLen = 0;

  while (Serial.available())
  {
    const char c = (char)Serial.read();
    if (c == '\r')
    {
      continue;
    }
    if (c == '\n')
    {
      line[lineLen] = 0;
      RtcTime t;
      if (parseSetLine(line, &t) && rtcWrite(t))
      {
        t.halted = false;
        printTime("[RTC] set OK", t);
      }
      else if (lineLen > 0)
      {
        Serial.println("[RTC] use: set YYYY-MM-DD HH:MM:SS");
      }
      lineLen = 0;
    }
    else if (lineLen < sizeof(line) - 1)
    {
      line[lineLen++] = c;
    }
  }

  if (millis() - lastMs >= 5000)
  {
    lastMs = millis();
    RtcTime t;
    if (rtcRead(&t))
    {
      printTime("[RTC]", t);
    }
    else
    {
      Serial.println("[RTC] read fail");
    }
  }
}
