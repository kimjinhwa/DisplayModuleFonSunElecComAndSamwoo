#ifndef ETH_W610_H
#define ETH_W610_H

#include <Arduino.h>
#include <IPAddress.h>
#include <Udp.h>

bool ethW610Begin(IPAddress ip, IPAddress gateway, IPAddress subnet, IPAddress dns);
void ethW610PrintStatus();
void ethW610CliStatus(Print &out);
UDP *ethW610SnmpUdp();
UDP *ethW610IpFinderUdp();
String ethW610MacString();
IPAddress ethW610LocalIP();
IPAddress ethW610Subnet();
IPAddress ethW610Gateway();

#endif
