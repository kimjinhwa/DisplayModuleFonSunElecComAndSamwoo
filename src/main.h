#ifndef MAIN_H
#define MAIN_H

typedef struct
{
    uint32_t IPADDRESS;
    uint32_t GATEWAY;
    uint32_t SUBNETMASK;
    uint32_t WEBSOCKETSERVER;
    uint32_t DNS1;
    uint32_t DNS2;
    uint32_t NTP_1;
    uint32_t NTP_2;
    uint16_t WEBSERVERPORT;
    bool ntpuse;
    uint32_t BAUDRATE;
    uint16_t Q_INTERVAL;
    uint16_t HighVoltage;
    uint16_t LowVoltage;
    uint16_t HighTemp;
    uint16_t HighImp;
    uint16_t alarmSetStatus;
    char deviceName[20];
} nvsSystemSet;
void setMemoryDataToLCD();

#endif