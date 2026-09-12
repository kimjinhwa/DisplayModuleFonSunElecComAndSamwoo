#include <Arduino.h>
#ifndef _WIFIOTA_H
#define _WIFIOTA_H

#include <WiFi.h>
#include <WiFiClient.h>
#include <Update.h>

void wifiOTAsetup(bool isUpdate);
void wifiOtaloop(void);
void wifiStopAfterOta(void);

#endif
