#ifndef _NARADAV13_H
#define _NARADAV13_H

#include <functional>
#include <map>
#include "options.h"
#include "ModbusMessage.h"

#if HAS_FREERTOS
extern "C" {
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
}
#endif

#define OP_LED 33
#define TX2_PIN 12   // Serial2 RX 핀 (GPIO12)
#define RX2_PIN 14   // Serial2 TX 핀 (GPIO14)
#define MAX_PACK_NUMBER     8

typedef struct {
    int voltageNumber;//설치된 배터리의 수 
    int voltage[16]; // 상위 3비트는 균등화 플레그, 과전압플레그, 배터리전전압 플레그    
    int ampere;   // ofset 30000, (30000 - (data0*256 + data1) )/100
    int soc;    // 0.01
    int Capacity;    //0.01
    int TempreatureNumber; //설치된 온도계의 수
    int Tempreature[6];   // offset -50 
    int packStatus[10];    
    int readCycleCount;    
    int totalVoltage; //0.01   
    int SOH; //0.01   % SOH = data0*256 + data1
    int BMS_PROTECT_STATUS; //0.01   % SOH = data0*256 + data1
}batteryInofo_t; 
class McMessage {
    public:
        McMessage(){};
    private:
};

typedef std::function<void(McMessage msg, uint32_t token)> NCOnData;
class NaradaClient232{
public:
    NaradaClient232();
    bool onDataHandler(NCOnData handler); // Accept onData handler
    void begin();
    void getPackData(int packNumber);
    void makeDataClear(int packNumber);
    Error readAnswerData();
    Error readAnswerData(uint8_t *rData);
    int dataParse(const uint8_t *revData);
    int dataParseExt(int packNumber,const uint8_t *revData);
    uint8_t checksum(unsigned char* buf, size_t len) ;
    void makeInt(uint16_t *dest,const uint8_t *src,byte len);
    void makeInt(int *dest,const uint8_t *src,byte len);
    void copyBatInfoData(int packNumber,batteryInofo_t* dest );
    void initBatInfo(int packNumber);
    void initBatInfo();
    SemaphoreHandle_t revDataMutex;
    batteryInofo_t batInfo[8];
private:
    //virtual void isInstance() = 0; // make class abstract
protected:
    NCOnData onData;    //Data response handler
};

extern NaradaClient232 naradaClient485;
void h_pxNaradaV13Request(void *parameter);
// #ifdef __cplusplus
// }
// #endif

#endif