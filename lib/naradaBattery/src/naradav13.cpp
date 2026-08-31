#include <Arduino.h>
#include <naradav13.h>
#include <bitset>
#include <Logging.h>
//#include "ModbusClient.h"
#define SET_TIMEOUT 300
#define OPLED_ON false
#define OPLED_OFF true
//이제 나라다 전용의 프로토콜 클레스를 하나 만들자

static const unsigned long UPTIME_UPDATE_INTERVAL = 1000; // ms = 1 second
static unsigned long lastUptimeUpdateTime = 0;
static int readSerialCount =0;
static uint8_t revData[255];
static int ValidData=0;

NaradaClient232::NaradaClient232()
{
    onData=nullptr;
    revDataMutex = xSemaphoreCreateMutex();
//OP_LED 
    
    //onData((NCOnData)nullptr,0);
    //onData(nullptr);
}
bool NaradaClient232::onDataHandler(NCOnData handler) // Accept onData handler
{
    if(onData){
        LOG_W("onData handler was already claimed\n");
    }
    else{
        LOG_I("onData handler was claimed\n");
        onData = handler;
    }
    return true;
}
uint8_t NaradaClient232::checksum(unsigned char* buf, size_t len) {
    unsigned char i, chk = 0;
    int sum = 0;
    for (i = 0; i < len; i++)
    {
        chk ^= buf[i];
        sum += buf[i];
    }
    unsigned char bChecksum = (chk ^ sum) & 0xFF;
    return (bChecksum);
}
void NaradaClient232::begin(){
};


void NaradaClient232::makeInt(int *dest,const uint8_t *src,byte len){
    memset(dest,0x00,len*2);
    for(int i=0;i<len;i++)dest[i]= src[i*2]<<8 | src[i*2+1];    
}
void NaradaClient232::makeInt(uint16_t *dest,const uint8_t *src,byte len){
    memset(dest,0x00,len*2);
    for(int i=0;i<len;i++)dest[i]= src[i*2]<<8 | src[i*2+1];    
}


void NaradaClient232::initBatInfo()
{
    for(int packNumber=0;packNumber<8;packNumber++)
        for(int j=0;j<15;j++)batInfo[packNumber].voltage[j]=0;
    for(int packNumber=0;packNumber<8;packNumber++)
        batInfo[packNumber].voltageNumber =15;
    for(int packNumber=0;packNumber<8;packNumber++)
        batInfo[packNumber].ampere=0;   // ofset 30000, (30000 - (data0*256 + data1) )/100
    for(int packNumber=0;packNumber<8;packNumber++)
        batInfo[packNumber].soc=0;    // 0.01
    for(int packNumber=0;packNumber<8;packNumber++)
        batInfo[packNumber].Capacity=0;    //0.01
    for(int packNumber=0;packNumber<8;packNumber++)
        batInfo[packNumber].TempreatureNumber=4; //설치된 온도계의 수
    for(int packNumber=0;packNumber<8;packNumber++)
        for(int j=0;j<6;j++)batInfo[packNumber].Tempreature[j]=50;   // offset -50 
    for(int packNumber=0;packNumber<8;packNumber++)
        for(int j=0;j<10;j++)batInfo[packNumber].packStatus[j]=0;    
    for(int packNumber=0;packNumber<8;packNumber++)
        batInfo[packNumber].readCycleCount=0;    
    for(int packNumber=0;packNumber<8;packNumber++)
        batInfo[packNumber].totalVoltage=0; //0.01   
    for(int packNumber=0;packNumber<8;packNumber++)
        batInfo[packNumber].SOH=0; //0.01   % SOH = data0*256 + data1
    for(int packNumber=0;packNumber<8;packNumber++)
        batInfo[packNumber].BMS_PROTECT_STATUS=0; //0.01   % SOH = data0*256 + data1
}
void NaradaClient232::initBatInfo(int packNumber)
{
    for(int j=0;j<15;j++)batInfo[packNumber].voltage[j]=0;
    batInfo[packNumber].voltageNumber =15;
    batInfo[packNumber].ampere=30000;   // ofset 30000, (30000 - (data0*256 + data1) )/100
    batInfo[packNumber].soc=0;    // 0.01
    batInfo[packNumber].Capacity=0;    //0.01
    batInfo[packNumber].TempreatureNumber=4; //설치된 온도계의 수
    for(int j=0;j<6;j++)batInfo[packNumber].Tempreature[j]=50;   // offset -50 
    for(int j=0;j<10;j++)batInfo[packNumber].packStatus[j]=0;    
    batInfo[packNumber].readCycleCount=0;    
    batInfo[packNumber].totalVoltage=0; //0.01   
    batInfo[packNumber].SOH=0; //0.01   % SOH = data0*256 + data1
    batInfo[packNumber].BMS_PROTECT_STATUS=0; //0.01   % SOH = data0*256 + data1
}
/* 팩정보를 복사해 준다*/
void NaradaClient232::copyBatInfoData(int packNumber, batteryInofo_t* dest){
    xSemaphoreTake(revDataMutex, portMAX_DELAY);
    dest->voltageNumber = batInfo[packNumber].voltageNumber ;
    for(int j=0;j<15;j++)dest->voltage[j]= batInfo[packNumber].voltage[j];
    dest->ampere =  batInfo[packNumber].ampere;
    dest->soc =  batInfo[packNumber].soc;
    dest->Capacity = batInfo[packNumber].Capacity;
    dest->TempreatureNumber=  batInfo[packNumber].TempreatureNumber;
    for(int j=0;j<4;j++)dest->Tempreature[j]=  batInfo[packNumber].Tempreature[j];
    for(int j=0;j<5;j++)dest->packStatus[j] = batInfo[packNumber].packStatus[j];
    dest->readCycleCount = batInfo[packNumber].readCycleCount;
    dest->totalVoltage=  batInfo[packNumber].totalVoltage;
    dest->SOH = batInfo[packNumber].SOH;
    dest->BMS_PROTECT_STATUS =  batInfo[packNumber].BMS_PROTECT_STATUS;
    xSemaphoreGive(revDataMutex);
}
int NaradaClient232::dataParse(const uint8_t *revData){
    int packNumber;
    packNumber = revData[1];
    revData = revData+4;
    
    if (xSemaphoreTake(revDataMutex, portMAX_DELAY)) {
        for(int i=0 ;i<11;i++){
            dataParseExt(packNumber, revData);
            revData = revData + revData[1]*2 +2;
        }
        xSemaphoreGive(revDataMutex);
    }
    return 0;
}
int NaradaClient232::dataParseExt(int packNumber,const uint8_t *revData){
    uint8_t dataLen = revData[1];
    uint8_t command = revData[0];
    revData = revData +2;
    switch(command){
        case 1:// 전압
            makeInt(batInfo[packNumber].voltage,revData,dataLen);  //0xF가 올것이다 
            //  01 0F 
            //      0     1     2     3     4     4     6     7     8     9     A     B     C     D     E
            //  0C C9 0C DD 0C BC 0C DC 0C DC 0C C2 0C C7 0C BF 0C B5 0C C4 0C C3 0A E3 0C E1 0C E1 0C D6 
        break;
        case 2:// 전류
            makeInt(&batInfo[packNumber].ampere,revData,dataLen);  //0xF가 올것이다 
            // 02 01 75 30 
        break;
        case 3:// SOC 
            makeInt(&batInfo[packNumber].soc,revData,dataLen);  //0xF가 올것이다 
            //  03 01 13 88 
        break;
        case 4:// Battery Capacity 
            makeInt(&batInfo[packNumber].Capacity,revData,dataLen);  //0xF가 올것이다 
            //  04 01 27 10 
        break;
        case 5:// Temperature
            makeInt(batInfo[packNumber].Tempreature,revData,dataLen);  //0xF가 올것이다 
            //  05 06 00 50 00 4F 00 51 00 50 40 52 20 51 
        break;
        case 6:// Battery Pack Status
            makeInt(batInfo[packNumber].packStatus,revData,dataLen);  //0xF가 올것이다 
            //  06 05 00 00 10 00 00 00 00 00 00 02 
        break;
        case 7:// 읽기 사이클 카운트  
            makeInt(&batInfo[packNumber].readCycleCount,revData,dataLen);  //0xF가 올것이다 
        break;
        case 8:// 총 전압 읽기 
            makeInt(&batInfo[packNumber].totalVoltage,revData,dataLen);  //0xF가 올것이다 
            //  08 01 13 02 
        break;
        case 9:// SOH 읽기 
            makeInt(&batInfo[packNumber].SOH,revData,dataLen);  //0xF가 올것이다 
            //  09 01 27 10 
        break;
        case 10:// BMS보호 상태 비트 읽기; 보호 상태 비트 = data0 * 256 + dual 
            makeInt(&batInfo[packNumber].BMS_PROTECT_STATUS,revData,dataLen);  //0xF가 올것이다 
            //  0A 01 00 00 
        break;
        default:
        break;
    }
    return SUCCESS;
}

Error NaradaClient232::readAnswerData(uint8_t *rData){
    memcpy(revData,rData,255);
    delay(5);
    if(revData[4+revData[3]] != checksum(revData,4+revData[3])) return CRC_ERROR;
    // Serial.printf("\nchecksum%d %02x %02x \n" ,revData[3],
    //         revData[4+revData[3]] ,checksum(revData,4+revData[3]));
    dataParse(revData);
    return SUCCESS;
}
Error NaradaClient232::readAnswerData(){
    if(revData[4+revData[3]] != checksum(revData,4+revData[3])) return CRC_ERROR;
    //Serial.printf("\nChecksum is %02x %02x\n",checksum(revData,4+revData[4]),revData[4+revData[4]]);
    //if(revData[4+revData[3]] !=  checksum(revData,4+revData[3]))return CRC_ERROR;
    dataParse(revData);
    return SUCCESS;
}

void NaradaClient232::makeDataClear(int packNumber)
{
    if (xSemaphoreTake(revDataMutex, portMAX_DELAY)) {
        batInfo[packNumber].voltageNumber = 0;
        for (int j = 0; j < 15; j++)
            batInfo[packNumber].voltage[j] = 0;
        batInfo[packNumber].ampere = 0;
        batInfo[packNumber].soc = 0;
        batInfo[packNumber].Capacity = 0;
        batInfo[packNumber].TempreatureNumber = 0;
        for (int j = 0; j < 4; j++)
            batInfo[packNumber].Tempreature[j] = 0;
        for (int j = 0; j < 5; j++)
            batInfo[packNumber].packStatus[j] = 0;
        batInfo[packNumber].readCycleCount = 0;
        batInfo[packNumber].voltageNumber = 0;
        batInfo[packNumber].SOH = 0;
        batInfo[packNumber].BMS_PROTECT_STATUS = 0;
        xSemaphoreGive(revDataMutex);
    }
}
void NaradaClient232::getPackData(int packNumber){
    readSerialCount =0;
    memset(revData,0x00,255);
    makeDataClear(packNumber);// 값을 읽어 오기 전에 팩데이타를 클리어 한다
    byte sendData[7]; memset(sendData,0x00,7);
    byte index=0;
    while(Serial2.available())Serial2.read();
    //7E 01 01 00 FE 0D`
    sendData[index++] = 0x7E;
    sendData[index++] = packNumber;//x01;
    sendData[index++] = 01;//x01;
    sendData[index++] = 00;//x01;
    sendData[index++] =checksum(sendData,index);
    sendData[index++] = 0x0D;//x01;
    digitalWrite(OP_LED, 1); // Write mode
    delay(1);
    Serial2.write(sendData,index);
    Serial2.flush();
    digitalWrite(OP_LED, 0); // Receive mode
    delay(10);
};