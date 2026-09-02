#define USE_ETHERNET_GENERIC
#define ETHERNET_USE_ESP32
#define BOARD_TYPE "ESP32-S3"
#define _ETG_LOGLEVEL_ 1
#define USE_THIS_SS_PIN 10

#include "eth_w610.h"
#include "board_pins.h"

#include <SPI.h>
#include <WiFi.h>
#include <Ethernet_Generic.h>

static EthernetUDP sSnmpUdp;

UDP *ethW610SnmpUdp()
{
  return &sSnmpUdp;
}

static void fillMac(uint8_t mac[6])
{
  WiFi.macAddress(mac);
  mac[0] = (uint8_t)((mac[0] & 0xFE) | 0x02);
}

bool ethW610Begin(IPAddress ip, IPAddress gateway, IPAddress subnet, IPAddress dns)
{
  uint8_t mac[6];
  fillMac(mac);
  SPI.begin(PIN_W610_SCLK, PIN_W610_MISO, PIN_W610_MOSI, PIN_W610_CS);
  Ethernet.init(PIN_W610_CS);
  Ethernet.begin(mac, ip, dns, gateway, subnet);
  delay(200);
  ethW610PrintStatus();
  return Ethernet.getChip() == w6100 || Ethernet.localIP() == ip;
}

void ethW610PrintStatus()
{
  Serial.print("[W610] chip=");
  const auto chip = Ethernet.getChip();
  if (chip == w6100)
  {
    Serial.print("W6100");
  }
  else
  {
    Serial.printf("%d", (int)chip);
  }
  Serial.print(" IP=");
  Serial.print(Ethernet.localIP());
  Serial.print(" link=");
  Serial.println(Ethernet.linkReport());
}
