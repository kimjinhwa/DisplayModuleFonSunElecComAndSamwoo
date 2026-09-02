#ifndef ETH_W610_H
#define ETH_W610_H

#include <Arduino.h>
#include <IPAddress.h>
#include <Udp.h>

bool ethW610Begin(IPAddress ip, IPAddress gateway, IPAddress subnet, IPAddress dns);
void ethW610PrintStatus();
UDP *ethW610SnmpUdp();

#endif
