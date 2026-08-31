#include <Arduino.h>
#include "SerialProtocalParse.h"
#include "BLEDevice.h"
#include "ArduinoJson.h"
#include "src/ui.h"
#include "src/ui_events.h"
#include "main.h"
#include <EEPROM.h>
#include "naradav13.h"

#define USE_SERIAL Serial
#define DEVICE_LCD


#define MODULE_1  1
#define MODULE_2  2

NaradaClient232 naradaClient;
//batteryInofo_t naradaClient.batInfo[8];

static const char *TAG ="protocal";
bool isHighVoltage=false;
bool isLowVoltage=false;
bool isHighImpedance=false;

StaticJsonDocument<3072> doc;

static uint8_t nowWindows = MODULE_1;
extern nvsSystemSet ipAddress_struct;
static uint8_t revData[255];
int readSerial1Data();
int delayCount =0;
int readSerial1Data()
{
  int timeout=2000;
  int readSerialCount =0;
  while (Serial1.available())// 일단 데이타가 도착하면 전부 다 읽는다.
  {
    if (Serial1.available())
    {
      revData[readSerialCount] = Serial1.read();
      readSerialCount++;
      if(readSerialCount>254)readSerialCount=254;
    }
    while(!Serial1.available()){
      delayMicroseconds(1);
      timeout--;
      if(!timeout)break;
      // if (!Serial1.available())
      //   delay(2);
    }
  }
  if (readSerialCount > 254) readSerialCount = 0;
  if (readSerialCount > 4)
  {
    if ((revData[0] == 0x7E) && (readSerialCount >= revData[3] + 4 + 2))
    {
      // LOG_I("\n-----Data count is %d %d\n",readSerialCount ,revData[3]+4+2);
      Serial.printf("\nModule %d Data count is %d %d\n",revData[1],readSerialCount ,revData[3]+4+2);
      // for (int i = 0; i < readSerialCount; i++)
      //   Serial.printf(" %02x", revData[i]);
      return 1;
    }
    else if ((revData[0] == 0x7D) && (readSerialCount >= revData[3] + 4 + 2))
    {
      Serial.printf("\nModule %d read failed ",  revData[1]);
      for (int i = 3; i < 255; i++)revData[i]=0;
      readSerialCount = 0;
      return 2;
    }
    else{
      Serial.printf("\nBegin Receive\n");
      for (int i = 0; i < readSerialCount ; i++){
        Serial.printf("%c",  revData[i]);
      }
      if(strstr((const char *)revData,"Begin") != NULL){
        Serial.printf("Screen Changed \n\r");
        lv_obj_t *current_screen = lv_scr_act();
        if(current_screen != ui_MainScreen )
        _ui_screen_change( &ui_MainScreen, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 0, 10, &ui_MainScreen_screen_init);

      }
      ;
      if(strstr((const char *)revData,"ipaddress:") != NULL){
        const char *ipPos = strstr((const char *)revData,"ipaddress:") ;
        ipPos += strlen("ipaddress:");
        char ipAddressStr[16];
        sscanf(ipPos, "%15s", ipAddressStr); 
        int octet1, octet2, octet3, octet4;
        sscanf(ipAddressStr, "%d.%d.%d.%d", &octet1, &octet2, &octet3, &octet4);

        // IPAddress 객체에 옥텟 값 설정
        IPAddress ipaddr = IPAddress(octet1, octet2, octet3, octet4);
        ipAddress_struct.IPADDRESS = ipaddr;
        Serial.printf("\nReceived Ipaddress is %s",ipaddr.toString().c_str());
        EEPROM.writeBytes(1, (const byte *)&ipAddress_struct, sizeof(nvsSystemSet));
        EEPROM.commit();
        setMemoryDataToLCD();
      }
      if(strstr((const char *)revData,"gateway:") != NULL){
        const char *ipPos = strstr((const char *)revData,"gateway:") ;
        ipPos += strlen("gateway:");
        char ipAddressStr[16];
        sscanf(ipPos, "%15s", ipAddressStr); 
        int octet1, octet2, octet3, octet4;
        sscanf(ipAddressStr, "%d.%d.%d.%d", &octet1, &octet2, &octet3, &octet4);

        // IPAddress 객체에 옥텟 값 설정
        IPAddress ipaddr = IPAddress(octet1, octet2, octet3, octet4);
        ipAddress_struct.GATEWAY= ipaddr;
        Serial.printf("\nReceived Gateway is %s",ipaddr.toString().c_str());
        EEPROM.writeBytes(1, (const byte *)&ipAddress_struct, sizeof(nvsSystemSet));
        EEPROM.commit();
        setMemoryDataToLCD();
      }
      if(strstr((const char *)revData,"subnetmask:") != NULL){
        const char *ipPos = strstr((const char *)revData,"subnetmask:") ;
        ipPos += strlen("subnetmask:");
        char ipAddressStr[16];
        sscanf(ipPos, "%15s", ipAddressStr); 
        int octet1, octet2, octet3, octet4;
        sscanf(ipAddressStr, "%d.%d.%d.%d", &octet1, &octet2, &octet3, &octet4);

        // IPAddress 객체에 옥텟 값 설정
        IPAddress ipaddr = IPAddress(octet1, octet2, octet3, octet4);
        ipAddress_struct.SUBNETMASK= ipaddr;
        Serial.printf("\nReceived Subnetmask is %s",ipaddr.toString().c_str());
        EEPROM.writeBytes(1, (const byte *)&ipAddress_struct, sizeof(nvsSystemSet));
        EEPROM.commit();
        setMemoryDataToLCD();
      }

      Serial.printf("\n\r");
    }

  }
  return 0;
}
lv_obj_t* ui_packVoltage[8] = {
  ui_lblPack1, ui_lblPack2, ui_lblPack3, ui_lblPack4,
  ui_lblPack5, ui_lblPack6, ui_lblPack7, ui_lblPack8,
};
lv_obj_t* ui_cellVoltage[15] = {
  ui_lblvoltage1, ui_lblvoltage2, ui_lblvoltage3, ui_lblvoltage4, ui_lblvoltage5, 
  ui_lblvoltage6, ui_lblvoltage7, ui_lblvoltage8, ui_lblvoltage9, ui_lblvoltage10, 
  ui_lblvoltage11, ui_lblvoltage12, ui_lblvoltage13, ui_lblvoltage14, ui_lblvoltage15, 
};

float avgVoltage =0.0;
//lblOutputVoltage
int ModuleVoltage[8]={0,0,0,0,0,0,0,0};

void initUI_Ptr(){
    ui_cellVoltage[0] = ui_lblvoltage1;
    ui_cellVoltage[1] = ui_lblvoltage2;
    ui_cellVoltage[2] = ui_lblvoltage3;
    ui_cellVoltage[3] = ui_lblvoltage4;
    ui_cellVoltage[4] = ui_lblvoltage5;
    ui_cellVoltage[5] = ui_lblvoltage6;
    ui_cellVoltage[6] = ui_lblvoltage7;
    ui_cellVoltage[7] = ui_lblvoltage8;
    ui_cellVoltage[8] = ui_lblvoltage9;
    ui_cellVoltage[9] = ui_lblvoltage10;
    ui_cellVoltage[10] = ui_lblvoltage11;
    ui_cellVoltage[11] = ui_lblvoltage12;
    ui_cellVoltage[12] = ui_lblvoltage13;
    ui_cellVoltage[13] = ui_lblvoltage14;
    ui_cellVoltage[14] = ui_lblvoltage15;

   ui_packVoltage[0]=ui_lblPack1;
   ui_packVoltage[1]=ui_lblPack2;
   ui_packVoltage[2]=ui_lblPack3;
   ui_packVoltage[3]=ui_lblPack4;
   ui_packVoltage[4]=ui_lblPack5;
   ui_packVoltage[5]=ui_lblPack6;
   ui_packVoltage[6]=ui_lblPack7;
   ui_packVoltage[7]=ui_lblPack8;
}
void displayToLcd(int packNumber,bool isSucess)
{
  initUI_Ptr();
  String HeaderText = ipAddress_struct.deviceName;
  int packCount=0;
  String strTemp;
  float highCellVoltage;
  float lowCellVoltage ;
  float differential;

  HeaderText += "-";
  HeaderText += packNumber+1;
  if(isSucess)lv_label_set_text(ui_HeaderTitle,HeaderText.c_str() );

  ModuleVoltage[packNumber] = naradaClient.batInfo[packNumber].totalVoltage;

  avgVoltage  = 0.0;
  int moduleCount=0; 
  for(int i=0;i< 8 ;i++){
      avgVoltage += naradaClient.batInfo[i].totalVoltage;
      //Serial.printf("\ntotalVoltage %d" ,naradaClient.batInfo[i].totalVoltage);
      if(naradaClient.batInfo[i].totalVoltage>1.0)moduleCount++;
  }
  //Serial.printf("\navgVoltage  %f" ,avgVoltage  );
  avgVoltage /= moduleCount;
  //Serial.printf("\navgVoltage  %f" ,avgVoltage  );
  //avgVoltage /= 100.0;
  //Serial.printf("\navgVoltage  %f" ,avgVoltage  );

  strTemp = "AVG  :" + String(avgVoltage/100);
  if(isSucess)lv_label_set_text(ui_lblOutputVoltage, strTemp.c_str());

  strTemp = "AMP  :" + String((naradaClient.batInfo[packNumber].ampere - 30000) / 100.0f);
  if(isSucess)lv_label_set_text(ui_lblTotalAmpere, strTemp.c_str());

  strTemp = "TEMP : " + String(
                            (naradaClient.batInfo[packNumber].Tempreature[0] - 50 +
                             naradaClient.batInfo[packNumber].Tempreature[1] - 50 +
                             naradaClient.batInfo[packNumber].Tempreature[2] - 50 +
                             naradaClient.batInfo[packNumber].Tempreature[3] - 50) /
                            4);
  if(isSucess)lv_label_set_text(ui_lblTotalTemperature, strTemp.c_str());

  highCellVoltage = naradaClient.batInfo[packNumber].voltage[0];
  lowCellVoltage = naradaClient.batInfo[packNumber].voltage[0];

  for (int i = 1; i < naradaClient.batInfo[packNumber].voltageNumber; i++)
  {
    highCellVoltage = naradaClient.batInfo[packNumber].voltage[i] > highCellVoltage ? naradaClient.batInfo[packNumber].voltage[i] : highCellVoltage;
    lowCellVoltage = naradaClient.batInfo[packNumber].voltage[i] < lowCellVoltage ? naradaClient.batInfo[packNumber].voltage[i] : lowCellVoltage;
  }
  highCellVoltage /= 1000.0f;
  lowCellVoltage /= 1000.0f;
  
  differential = highCellVoltage - lowCellVoltage;
  strTemp = "HVOL :" + String(highCellVoltage) + "V";
  if(isSucess)lv_label_set_text(ui_lblHighVoltage, strTemp.c_str());

  strTemp = "LVOL :" + String(lowCellVoltage) + "V";
  if(isSucess)lv_label_set_text(ui_lblLowVoltage, strTemp.c_str());

  strTemp = "DIFF :" + String(int(differential * 1000)) + "mV";
  if(isSucess)lv_label_set_text(ui_lblDiff, strTemp.c_str());
  if(naradaClient.batInfo[packNumber].Tempreature[0]< 50) naradaClient.batInfo[packNumber].Tempreature[0] =0;
  if(naradaClient.batInfo[packNumber].Tempreature[1]< 50) naradaClient.batInfo[packNumber].Tempreature[1] =0;
  if(naradaClient.batInfo[packNumber].Tempreature[2]< 50) naradaClient.batInfo[packNumber].Tempreature[2] =0;
  if(naradaClient.batInfo[packNumber].Tempreature[3]< 50) naradaClient.batInfo[packNumber].Tempreature[3] =0;
  String tTemperature1(naradaClient.batInfo[packNumber].Tempreature[0] - 50);
  String tTemperature2(naradaClient.batInfo[packNumber].Tempreature[1] - 50);
  String tTemperature3(naradaClient.batInfo[packNumber].Tempreature[2] - 50);
  String tTemperature4(naradaClient.batInfo[packNumber].Tempreature[3] - 50);

  String cellVoltage = "";

 for (int i = 0; i < 15; i++)
  {
    cellVoltage = "";
    cellVoltage += String(naradaClient.batInfo[packNumber].voltage[i] / 1000.0f);
    // 특정 셀에만 온도 표시
    switch (i)
    {
    case 0: // 첫 번째 셀
      cellVoltage += "\n(" + tTemperature1 + ")";
      break;
    case 3: // 4번째 셀
      cellVoltage += "\n(" + tTemperature2 + ")";
      break;
    case 7: // 8번째 셀
      cellVoltage += "\n(" + tTemperature3 + ")";
      break;
    case 11: // 12번째 셀
      cellVoltage += "\n(" + tTemperature4 + ")";
      break;
    }
    if (isSucess)
      lv_label_set_text(ui_cellVoltage[i], cellVoltage.c_str());
  }
  // if(isSucess)lv_label_set_text((lv_obj_t *)ui_packVoltage[packNumber], String( naradaClient.batInfo[packNumber].totalVoltage!=0 ? naradaClient.batInfo[packNumber].totalVoltage/100.0f:0).c_str());
  String tVoltage = "";
  tVoltage = "#";
  tVoltage += packNumber + 1;
  tVoltage += " ";
  if (naradaClient.batInfo[packNumber].totalVoltage != 0)
    tVoltage += String(float(naradaClient.batInfo[packNumber].totalVoltage) / 100.0f);
  else
    tVoltage += 0;
  // 배열이 유효한 범위 내에 있는지 확인
  if (packNumber >= 0 && packNumber < 8) {
      lv_label_set_text(ui_packVoltage[packNumber], tVoltage.c_str());
  }
};
void printPackData(int packNumber){
  //batteryInofo_t dest;
  //naradaClient.copynaradaClient.batInfoData(packNumber,&dest);
  
  Serial.printf("\nnaradaClient.batInfo[packNumber].voltage %d",naradaClient.batInfo[packNumber].voltageNumber );
    for(int j=0;j<15;j++)
      Serial.printf(" %d",naradaClient.batInfo[packNumber].voltage[j]);
  Serial.printf("\nnaradaClient.batInfo[packNumber].ampere %d",naradaClient.batInfo[packNumber].ampere  );
  Serial.printf("\nnaradaClient.batInfo[packNumber].soc%d",naradaClient.batInfo[packNumber].soc);
  Serial.printf("\nnaradaClient.batInfo[packNumber].Capacity  %d",naradaClient.batInfo[packNumber].Capacity );
  Serial.printf("\nnaradaClient.batInfo[packNumber].TempreatureNumber %d",naradaClient.batInfo[packNumber].TempreatureNumber);
  Serial.printf("\nnaradaClient.batInfo[packNumber].Tempreature ");
    for(int j=0;j<4;j++) Serial.printf("\n->naradaClient.batInfo[packNumber].Tempreature %d",naradaClient.batInfo[packNumber].Tempreature[j]);

  Serial.printf("\nnaradaClient.batInfo[packNumber].packStatus");
    for(int j=0;j<5;j++)Serial.printf("\n->naradaClient.batInfo[packNumber].packStatus%d",naradaClient.batInfo[packNumber].packStatus[j]);
  Serial.printf("\nnaradaClient.batInfo[packNumber].readCycleCount %d",naradaClient.batInfo[packNumber].readCycleCount );
  Serial.printf("\nnaradaClient.batInfo[packNumber].totalVoltage %d",naradaClient.batInfo[packNumber].totalVoltage);
  Serial.printf("\nnaradaClient.batInfo[packNumber].SOH %d",naradaClient.batInfo[packNumber].SOH );
  Serial.printf("\nnaradaClient.batInfo[packNumber].BMS_PROTECT_STATUS %d",naradaClient.batInfo[packNumber].BMS_PROTECT_STATUS );
}
bool isFirst=true;

void serialProtocalparse()
{
  int packNumber = 0;
  int ValidData = 0;
  ValidData = readSerial1Data();
  if (ValidData == 1)
  {
    packNumber = revData[1];

    if (naradaClient.readAnswerData(&revData[0]) == 0)
    {
      Serial.printf("\nData Received OK Pack : %d", packNumber);
      //printPackData(packNumber);
      String msg("STATUS:Module On #"); 
      msg += packNumber+1;
      lv_label_set_text(ui_CompanyLabel1, msg.c_str());
      if(isFirst){ //처음 한번만 화면에 뿌려준다. 
      // 이후에는 사이드 팩정보만 업데이트 한다.
        displayToLcd(packNumber,true);
        isFirst=false;
      }
      else{
        displayToLcd(packNumber,false);
      } 

    }
    ValidData = 0;
    memset(revData, 0x00, 255);
  }
  else if (ValidData == 2)
  {
      packNumber = revData[1];
      String msg("STATUS:Module Off #"); 
      msg += packNumber+1;
      lv_label_set_text(ui_CompanyLabel1, msg.c_str());
      naradaClient.initBatInfo(packNumber);
      displayToLcd(packNumber,false);
      //printPackData(packNumber);
  }
}

// while(Serial1.available()){
//   c=Serial1.read();
//   Serial.printf(" %02x",c);
// };
//Serial1.print("hello..");

void btnPackChange(lv_event_t * e)
{
	// Your code here
}

void btnEventPack1(lv_event_t * e)
{

  // while(Serial1.available());
  // delay(10);
  // Serial1.printf("datareq 1 \n\r");
  displayToLcd(0,true);
  printPackData(0);
}

// void saveButtenEvent(lv_event_t * e)
// {
// 	// Your code here
// }

void btnEventPack2(lv_event_t * e)
{
  // while(Serial1.available());
  // delay(10);
  // Serial1.printf("datareq 0 \n\r");
  displayToLcd(1,true);
  printPackData(1);
}

void btnEventPack3(lv_event_t * e)
{
  displayToLcd(2,true);
  printPackData(2);
}

void btnEventPack4(lv_event_t * e)
{
  displayToLcd(3,true);
  printPackData(3);
}

void btnEventPack5(lv_event_t * e)
{
  displayToLcd(4,true);
  printPackData(4);
}

void btnEventPack6(lv_event_t * e)
{
  displayToLcd(5,true);
  printPackData(5);
}

void btnEventPack7(lv_event_t * e)
{
  displayToLcd(6,true);
  printPackData(6);
}

void btnEventPack8(lv_event_t * e)
{
  displayToLcd(7,true);
  printPackData(7);
}
void saveButtenEvent(lv_event_t * e)
{
  char buf[255];
  while(Serial1.available());
  Serial1.printf("datareq 0 \n\r");
  delay(100);
  while (Serial1.available() ) Serial1.read();

  IPAddress ipaddress(
    String(lv_textarea_get_text(ui_txtIPADDRESS1)).toInt(),
    String(lv_textarea_get_text(ui_txtIPADDRESS2)).toInt(),
    String(lv_textarea_get_text(ui_txtIPADDRESS3)).toInt(),
    String(lv_textarea_get_text(ui_txtIPADDRESS4)).toInt()
  );
  ipAddress_struct.IPADDRESS = (uint32_t)ipaddress;
  IPAddress subnet(
    String(lv_textarea_get_text(ui_txtSUBNET1)).toInt(),
    String(lv_textarea_get_text(ui_txtSUBNET2)).toInt(),
    String(lv_textarea_get_text(ui_txtSUBNET3)).toInt(),
    String(lv_textarea_get_text(ui_txtSUBNET4)).toInt()
  );
  ipAddress_struct.SUBNETMASK= (uint32_t)subnet;
  IPAddress gateway(
    String(lv_textarea_get_text(ui_txtGATEWAY1)).toInt(),
    String(lv_textarea_get_text(ui_txtGATEWAY2)).toInt(),
    String(lv_textarea_get_text(ui_txtGATEWAY3)).toInt(),
    String(lv_textarea_get_text(ui_txtGATEWAY4)).toInt()
  );
  ipAddress_struct.GATEWAY= (uint32_t)gateway;
  String devicName(lv_textarea_get_text(ui_txtDEVICENAME));
  Serial.printf("\nlv device name is %s",devicName);
  Serial.printf("\nIPADDRESS %d.%d.%d.%d.",IPAddress(ipaddress)[0],IPAddress(ipaddress)[1],IPAddress(ipaddress)[2],IPAddress(ipaddress)[3]);
  Serial.printf("\nsubnet %d.%d.%d.%d.",IPAddress(subnet)[0],IPAddress(subnet)[1],IPAddress(subnet)[2],IPAddress(subnet)[3]);
  Serial.printf("\ngateway %d.%d.%d.%d.",IPAddress(gateway)[0],IPAddress(gateway)[1],IPAddress(gateway)[2],IPAddress(gateway)[3]);

  snprintf(ipAddress_struct.deviceName,20,devicName.c_str());
  Serial.printf("\nWrite device name to eeprom that is %s",ipAddress_struct.deviceName);
  EEPROM.writeBytes(1, (const byte *)&ipAddress_struct, sizeof(nvsSystemSet));
  EEPROM.commit();
  EEPROM.readBytes(1, (byte *)&ipAddress_struct, sizeof(ipAddress_struct));
  Serial.println("\nEEPROM Memory modified and Reread it now..");
  //Serial1.printf("ipaddress -set -i %s -s %s -g %s \r",ipaddress.toString(),gateway.toString(),subnet.toString());
  Serial.printf("ipaddress -set -i %s -s %s  -g %s \n\r",ipaddress.toString(),subnet.toString(),gateway.toString());
  Serial1.printf("ipaddress -set -i %s -s %s  -g %s \n\r",ipaddress.toString(),subnet.toString(),gateway.toString());
  int i=0;
  memset(buf,0x00,255);
  int timeout =0;
  while (!Serial1.available() && timeout < 100){
    timeout++;
    delay(1);
  }
  Serial.printf("\nTimeout count is %d\n",timeout);

  while (Serial1.available() && i<254  )
  {
    buf[i++]=Serial1.read();
    if(!Serial1.available()){
      delay(1);
    }
  }
  Serial.printf("\nbuf:%s",buf);
  String msg("\nSTATUS:"); 
  msg += buf;
  msg += " Now System reboot ! Waiting....";
  Serial.printf(msg.c_str());
  setMemoryDataToLCD();
  lv_label_set_text(ui_CompanyLabel3, msg.c_str());

  Serial1.printf("reboot \r");
}

// void ChangeModuleEvent(lv_event_t * e)
// {
// }



static unsigned long previousMillis_1 = 0;  
const long interval_1s = 1000;  
static unsigned long previousMillis_5 = 0;  
const long interval_5s = 5000;  
void WebSocketJsonProtocalLoop(void *parameters)
{
  unsigned long currentMillis;
  //for (;;)
  {
    //webSocket.loop();
    currentMillis = millis();
    //
    if (currentMillis - previousMillis_5 >= interval_5s)
    {
 
      previousMillis_5 = currentMillis;
    }
    vTaskDelay(10);
  };
}
