#include "cli_commands.h"
#include "main.h"
#include "myBlueTooth.h"
#include "Version.h"
#include "board_rtc.h"
#include "eth_w610.h"
#include "samwoo_poll.h"
#include <EEPROM.h>
#include <IPAddress.h>
#include <WiFi.h>

#ifndef FW_UPDATE_BASE
#define FW_UPDATE_BASE "http://ift.iptime.org:81/Esp32UploadFirmware"
#endif

static void startUpdate(void)
{
  mySerialBT.printf("\r\nUpdate를 시작합니다.");
  mySerialBT.printf("\r\nNow System reboot for update!");
  ipAddress_struct.isUpdate = true;
  nvsSave();
  delay(1000);
  ESP.restart();
}

static void printStoredWifi(void)
{
  mySerialBT.printf("\r\nSSID : %s\r\n", ipAddress_struct.ssid);
  if (ipAddress_struct.password[0] == '\0')
    mySerialBT.printf("PASS : (open)\r\n");
  else
    mySerialBT.printf("PASS : %s\r\n", ipAddress_struct.password);
}

static void printIpU32(const char *tag, uint32_t v)
{
  IPAddress a(v);
  mySerialBT.printf("%s : %s\r\n", tag, a.toString().c_str());
}

static bool parseIpToken(const String &s, IPAddress *out)
{
  return out != nullptr && out->fromString(s);
}

static bool validRtc(const BoardRtcTime &t)
{
  if (t.year < 2000 || t.year > 2099)
    return false;
  if (t.month < 1 || t.month > 12)
    return false;
  if (t.day < 1 || t.day > 31)
    return false;
  if (t.hour > 23 || t.minute > 59 || t.second > 59)
    return false;
  return true;
}

static void printRtc(const char *tag, const BoardRtcTime &t)
{
  mySerialBT.printf("\r\n%s : %04u-%02u-%02u %02u:%02u:%02u%s\r\n",
                    tag, t.year, t.month, t.day, t.hour, t.minute, t.second,
                    t.halted ? " HALTED" : "");
}

static void cmdStatus(void)
{
  mySerialBT.printf("\r\nNAME : %s\r\n", ipAddress_struct.deviceName);
  mySerialBT.printf("VERSION : %s\r\n", VERSION);
  mySerialBT.printf("ERR : %lu\r\n", (unsigned long)samwooErrorCount);
  for (uint8_t i = 0; i < SAMWOO_PACKS; ++i)
  {
    if (!samwooOk[i])
    {
      mySerialBT.printf("PACK%u : FAIL\r\n", (unsigned)(i + 1));
      continue;
    }
    const uint16_t soc = samwooReg(i, 2);
    const float volt = samwooReg(i, 4) / 10.0f;
    const float amp = ((int16_t)samwooReg(i, 5)) / 10.0f;
    mySerialBT.printf("PACK%u : OK SOC=%u V=%.1f I=%.1f\r\n",
                      (unsigned)(i + 1), (unsigned)soc, volt, amp);
  }
}

void cliParseLine(const String &raw)
{
  String line = raw;
  line.trim();
  if (line.length() == 0)
    return;

  int sp = line.indexOf(' ');
  String cmd = (sp < 0) ? line : line.substring(0, sp);
  String arg = (sp < 0) ? "" : line.substring(sp + 1);
  cmd.toLowerCase();
  arg.trim();

  if (cmd == "version")
  {
    mySerialBT.printf("\r\nVERSION : %s\r\n", VERSION);
    return;
  }
  if (cmd == "help")
  {
    mySerialBT.printf("\r\nversion | ssid [name] | pass [pw|none] | update\r\n");
    mySerialBT.printf("ip [ip [sn gw]] | time [Y M D h m s] | name [text]\r\n");
    mySerialBT.printf("status | mac | reboot\r\n");
    return;
  }
  if (cmd == "ssid")
  {
    if (arg.length() > 0)
    {
      strncpy(ipAddress_struct.ssid, arg.c_str(), sizeof(ipAddress_struct.ssid) - 1);
      ipAddress_struct.ssid[sizeof(ipAddress_struct.ssid) - 1] = '\0';
      nvsSave();
    }
    printStoredWifi();
    return;
  }
  if (cmd == "pass")
  {
    if (arg.length() > 0)
    {
      if (arg.equalsIgnoreCase("none") || arg.equalsIgnoreCase("clear") ||
          arg.equalsIgnoreCase("open"))
        ipAddress_struct.password[0] = '\0';
      else
      {
        strncpy(ipAddress_struct.password, arg.c_str(), sizeof(ipAddress_struct.password) - 1);
        ipAddress_struct.password[sizeof(ipAddress_struct.password) - 1] = '\0';
      }
      nvsSave();
    }
    printStoredWifi();
    return;
  }
  if (cmd == "update")
  {
    mySerialBT.printf("\r\nSSID: %s", ipAddress_struct.ssid);
    mySerialBT.printf("\r\nOLD VERSION: %s", VERSION);
    mySerialBT.printf("\r\n%s", FW_UPDATE_BASE);
    mySerialBT.printf("\r\nWiFi는 재부팅 후 연결합니다. Update 시작합니다...");
    startUpdate();
    return;
  }
  if (cmd == "reboot")
  {
    mySerialBT.printf("\r\nNow System Rebooting...\r\n");
    delay(300);
    ESP.restart();
    return;
  }
  if (cmd == "mac")
  {
    mySerialBT.printf("\r\nWIFI MAC : %s\r\n", WiFi.macAddress().c_str());
    mySerialBT.printf("ETH MAC : %s\r\n", ethW610MacString().c_str());
    return;
  }
  if (cmd == "name")
  {
    if (arg.length() > 0)
    {
      strncpy(ipAddress_struct.deviceName, arg.c_str(), sizeof(ipAddress_struct.deviceName) - 1);
      ipAddress_struct.deviceName[sizeof(ipAddress_struct.deviceName) - 1] = '\0';
      nvsSave();
      setMemoryDataToLCD();
    }
    mySerialBT.printf("\r\nNAME : %s\r\n", ipAddress_struct.deviceName);
    return;
  }
  if (cmd == "status")
  {
    cmdStatus();
    return;
  }
  if (cmd == "ip")
  {
    if (arg.length() > 0)
    {
      String t1, t2, t3;
      int p1 = arg.indexOf(' ');
      if (p1 < 0)
        t1 = arg;
      else
      {
        t1 = arg.substring(0, p1);
        String rest = arg.substring(p1 + 1);
        rest.trim();
        int p2 = rest.indexOf(' ');
        if (p2 < 0)
          t2 = rest;
        else
        {
          t2 = rest.substring(0, p2);
          t3 = rest.substring(p2 + 1);
          t3.trim();
        }
      }
      IPAddress nip, nsn, ngw;
      if (!parseIpToken(t1, &nip))
      {
        mySerialBT.printf("\r\nERROR: ip a.b.c.d [sn gw]\r\n");
        return;
      }
      ipAddress_struct.IPADDRESS = (uint32_t)nip;
      if (t2.length() && parseIpToken(t2, &nsn))
        ipAddress_struct.SUBNETMASK = (uint32_t)nsn;
      if (t3.length() && parseIpToken(t3, &ngw))
        ipAddress_struct.GATEWAY = (uint32_t)ngw;
      nvsSave();
      setMemoryDataToLCD();
      mySerialBT.printf("\r\nIP saved. reboot to apply.\r\n");
    }
    printIpU32("SET IP", ipAddress_struct.IPADDRESS);
    printIpU32("SET SN", ipAddress_struct.SUBNETMASK);
    printIpU32("SET GW", ipAddress_struct.GATEWAY);
    ethW610CliStatus(mySerialBT);
    return;
  }
  if (cmd == "time")
  {
    if (arg.length() == 0)
    {
      BoardRtcTime t;
      if (!boardRtcRead(&t))
        mySerialBT.printf("\r\nTIME : FAIL\r\n");
      else
        printRtc("TIME", t);
      return;
    }
    int y = 0, mo = 0, d = 0, h = 0, mi = 0, s = 0;
    if (sscanf(arg.c_str(), "%d %d %d %d %d %d", &y, &mo, &d, &h, &mi, &s) != 6)
    {
      mySerialBT.printf("\r\nUsage: time [YYYY MM DD HH MM SS]\r\n");
      return;
    }
    BoardRtcTime want = {};
    want.year = (uint16_t)y;
    want.month = (uint8_t)mo;
    want.day = (uint8_t)d;
    want.hour = (uint8_t)h;
    want.minute = (uint8_t)mi;
    want.second = (uint8_t)s;
    if (!validRtc(want))
    {
      mySerialBT.printf("\r\nERROR: invalid datetime\r\n");
      return;
    }
    if (!boardRtcWrite(want))
    {
      mySerialBT.printf("\r\nERROR: RTC set failed\r\n");
      return;
    }
    BoardRtcTime got;
    if (boardRtcRead(&got))
      printRtc("OK RTC", got);
    else
      printRtc("SET", want);
    return;
  }

  mySerialBT.printf("\r\nunknown: %s\r\n", cmd.c_str());
}
