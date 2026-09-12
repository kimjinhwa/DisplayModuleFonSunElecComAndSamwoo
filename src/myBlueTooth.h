#ifndef MY_BLUETOOTH_H
#define MY_BLUETOOTH_H
#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>

#define SERVICE_UUID "6E400001-B5A3-F393-E0A9-E50E24DCCA9B"
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9B"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9B"

void bleSetup();
void bleCheck();

class MyServerCallbacks : public BLEServerCallbacks
{
  void onConnect(BLEServer *pServer);
  void onDisconnect(BLEServer *pServer);
};

class MyCallbacks : public BLECharacteristicCallbacks
{
  void onWrite(BLECharacteristic *pCharacteristic);
};

class myBlueToothStream : public Stream
{
public:
  bool deviceConnected = false;
  myBlueToothStream(void);
  ~myBlueToothStream(void);
  int peek(void);
  size_t write(uint8_t c);
  size_t write(const uint8_t *buffer, size_t size);
  int available(void);
  int read(void);
  String readString();
  size_t printf(const char *format, ...);
};
extern myBlueToothStream mySerialBT;
#endif
