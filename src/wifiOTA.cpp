#include "wifiOTA.h"
#include "esp32SelfUploader.h"
#include "main.h"
#include <Arduino_GFX_Library.h>

#ifndef FW_UPDATE_BASE
#define FW_UPDATE_BASE "http://ift.iptime.org:81/Esp32UploadFirmware"
#endif

extern ESP32SelfUploader selfUploader;
extern Arduino_RPi_DPI_RGBPanel *gfx;

void wifiStopAfterOta(void)
{
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  delay(50);
}

void wifiOTAsetup(bool isUpdate)
{
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  delay(100);

  if (ipAddress_struct.password[0] == '\0')
    WiFi.begin(ipAddress_struct.ssid);
  else
    WiFi.begin(ipAddress_struct.ssid, ipAddress_struct.password);

  Serial.println("");
  Serial.printf("WiFi connect %s ...\n", ipAddress_struct.ssid);
  int loopCount = 40;
  gfx->fillScreen(BLACK);
  gfx->setTextColor(WHITE);
  delay(100);
  gfx->setCursor(0, 10);
  gfx->println("Connecting to WiFi..");
  gfx->println(ipAddress_struct.ssid);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
    gfx->print(".");
    loopCount--;
    if (loopCount <= 0)
      break;
  }

  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("Failed to connect to WiFi");
    gfx->println("");
    gfx->println("Failed to connect to WiFi");
    delay(2000);
    wifiStopAfterOta();
    return;
  }

  Serial.println("");
  gfx->println("");
  Serial.print("Connected to ");
  gfx->println("Connected to ");
  Serial.println(ipAddress_struct.ssid);
  gfx->println(ipAddress_struct.ssid);
  Serial.print("IP address: ");
  gfx->println("IP address: ");
  Serial.println(WiFi.localIP());
  gfx->println(WiFi.localIP());
  if (!isUpdate)
  {
    wifiStopAfterOta();
    return;
  }

  selfUploader.begin(ipAddress_struct.ssid, ipAddress_struct.password, FW_UPDATE_BASE);
  Serial.printf("Free heap before OTA: %d\n", ESP.getFreeHeap());
  if (selfUploader.checkNewVersion(selfUploader.update_url))
  {
    if (selfUploader.tryAutoUpdate(selfUploader.updateFile_url.c_str()))
    {
      Serial.println("Update success");
      gfx->println("Update success");
      delay(2000);
      ESP.restart();
    }
    else
    {
      Serial.println("Update failed");
      gfx->println("Update failed");
      delay(3000);
    }
  }
  else
  {
    Serial.println("Already on latest version");
    gfx->println("Already on latest version");
    delay(2000);
  }
  wifiStopAfterOta();
}

void wifiOtaloop(void)
{
}
