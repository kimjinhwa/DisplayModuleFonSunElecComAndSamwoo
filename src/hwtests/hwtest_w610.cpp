// WIZ610 / W6100 SPI H/W 테스트. INT/RST GPIO 없음.
//   pio run -e hwtest_w610 -t upload
// 고정 IP 192.168.0.57  (설정 화면 기본값)
//   ping 192.168.0.57
//   telnet 192.168.0.57 23
// USB Serial 115200: 링크/IP 로그. 텔넷은 5초마다 상태, 입력은 에코.
// USE_W5100 은 정의하지 말 것. Ethernet_Generic 이 정의만 있으면 true 로 바꿔 버린다.

#include <Arduino.h>
#include <SPI.h>
#include <WiFi.h>
#include <Ethernet_Generic.h>
#include <esp_wifi.h>
#include "board_pins.h"

#ifndef BAUDRATEDEF
#define BAUDRATEDEF 115200
#endif

class HwTelnetServer : public EthernetServer
{
public:
  explicit HwTelnetServer(uint16_t port) : EthernetServer(port) {}
  void begin(uint16_t port = 0) override
  {
    (void)port;
    EthernetServer::begin();
  }
};

static HwTelnetServer telnet(23);
static EthernetClient telnetClient;
static uint32_t tickCount = 0;

static const IPAddress kIp(192, 168, 0, 57);
static const IPAddress kGw(192, 168, 0, 1);
static const IPAddress kSn(255, 255, 255, 0);
static const IPAddress kDns(8, 8, 8, 8);

static void fillMac(uint8_t mac[6])
{
  WiFi.macAddress(mac);
  mac[0] = (uint8_t)((mac[0] & 0xFE) | 0x02);
}

static void printLink()
{
  Serial.print("[W610] chip=");
  const auto chip = Ethernet.getChip();
  if (chip == w6100)
  {
    Serial.print("W6100");
  }
  else if (chip == w5500)
  {
    Serial.print("W5500");
  }
  else if (chip == w5100)
  {
    Serial.print("W5100");
  }
  else
  {
    Serial.printf("unknown(%d)", (int)chip);
  }
  Serial.print("  IP=");
  Serial.print(Ethernet.localIP());
  Serial.print("  link=");
  Serial.println(Ethernet.linkReport());
}

void setup()
{
  Serial.begin(BAUDRATEDEF);
  delay(300);
  Serial.println();
  Serial.println("=== hwtest_w610 WIZ610 ===");
  Serial.printf("SPI MOSI=%d SCLK=%d MISO=%d CS=%d (no INT/RST GPIO)\n",
                PIN_W610_MOSI, PIN_W610_SCLK, PIN_W610_MISO, PIN_W610_CS);

  esp_wifi_stop();
  btStop();

  uint8_t mac[6];
  fillMac(mac);
  Serial.printf("MAC %02X:%02X:%02X:%02X:%02X:%02X\n",
                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

  SPI.begin(PIN_W610_SCLK, PIN_W610_MISO, PIN_W610_MOSI, PIN_W610_CS);
  Ethernet.init(PIN_W610_CS);
  Ethernet.begin(mac, kIp, kDns, kGw, kSn);
  delay(200);

  printLink();
  Serial.println("Ping : ping 192.168.0.57");
  Serial.println("Telnet: telnet 192.168.0.57 23");
  telnet.begin();
}

static void serviceTelnet()
{
  if (!telnetClient || !telnetClient.connected())
  {
    telnetClient = telnet.available();
    if (telnetClient)
    {
      Serial.print("[W610] telnet connect ");
      Serial.println(telnetClient.remoteIP());
      telnetClient.println("W610 telnet OK. Echo on. Status every 5s.");
    }
  }
  if (!telnetClient)
  {
    return;
  }
  while (telnetClient.available())
  {
    const int c = telnetClient.read();
    telnetClient.write((uint8_t)c);
    Serial.write((uint8_t)c);
  }
}

void loop()
{
  static uint32_t lastMs = 0;
  serviceTelnet();

  if (millis() - lastMs >= 5000)
  {
    lastMs = millis();
    tickCount++;
    printLink();
    if (telnetClient && telnetClient.connected())
    {
      telnetClient.printf("tick %lu IP=%s link=%s\r\n", (unsigned long)tickCount,
                          Ethernet.localIP().toString().c_str(), Ethernet.linkReport());
    }
  }
}
