#include "myBlueTooth.h"
#include "cli_commands.h"
#include <WiFi.h>

BLEServer *pServer = NULL;
BLECharacteristic *pTxCharacteristic;
bool deviceConnected = false;
bool oldDeviceConnected = false;

myBlueToothStream mySerialBT;

void MyServerCallbacks::onConnect(BLEServer *pServer)
{
  deviceConnected = true;
}

void MyServerCallbacks::onDisconnect(BLEServer *pServer)
{
  deviceConnected = false;
}

bool btDataReceived = false;
String btReceiveString = "";

void MyCallbacks::onWrite(BLECharacteristic *pCharacteristic)
{
  std::string rxValue = pCharacteristic->getValue();
  if (rxValue.length() > 0)
  {
    for (size_t i = 0; i < rxValue.length(); i++)
    {
      btReceiveString += rxValue[i];
      if (rxValue[i] == '\r' || rxValue[i] == '\n')
        btDataReceived = true;
      if (btReceiveString.length() > 30)
        btDataReceived = true;
    }
  }
}

myBlueToothStream::myBlueToothStream(void) {}
myBlueToothStream::~myBlueToothStream(void) {}

size_t myBlueToothStream::write(uint8_t c)
{
  return write(&c, 1);
}

size_t myBlueToothStream::write(const uint8_t *buffer, size_t size)
{
  if (buffer == nullptr || size == 0)
    return 0;
  if (pTxCharacteristic == nullptr || !deviceConnected)
    return Serial.write(buffer, size);
  const size_t chunkSize = 160;
  size_t sent = 0;
  while (sent < size)
  {
    size_t n = size - sent;
    if (n > chunkSize)
      n = chunkSize;
    pTxCharacteristic->setValue(const_cast<uint8_t *>(buffer + sent), n);
    pTxCharacteristic->notify(true);
    sent += n;
    if (sent < size)
      delay(8);
  }
  return size;
}

int myBlueToothStream::available(void)
{
  return btDataReceived;
}

int myBlueToothStream::read()
{
  if (btReceiveString.length())
  {
    uint8_t c = btReceiveString.charAt(0);
    btReceiveString.remove(0);
    if (btReceiveString.length() == 0)
      btDataReceived = 0;
    return c;
  }
  return -1;
}

int myBlueToothStream::peek(void)
{
  if (btReceiveString.length())
    return btReceiveString.charAt(0);
  return -1;
}

String myBlueToothStream::readString()
{
  String retString = btReceiveString;
  btReceiveString = "";
  btDataReceived = 0;
  return retString;
}

size_t myBlueToothStream::printf(const char *format, ...)
{
  char loc_buf[64];
  char *temp = loc_buf;
  va_list arg;
  va_list copy;
  va_start(arg, format);
  va_copy(copy, arg);
  int len = vsnprintf(temp, sizeof(loc_buf), format, copy);
  va_end(copy);
  if (len < 0)
  {
    va_end(arg);
    return 0;
  }
  if (len >= (int)sizeof(loc_buf))
  {
    temp = (char *)malloc(len + 1);
    if (temp == NULL)
    {
      va_end(arg);
      return 0;
    }
    len = vsnprintf(temp, len + 1, format, arg);
  }
  va_end(arg);
  len = write((uint8_t *)temp, len);
  if (temp != loc_buf)
    free(temp);
  return len;
}

void bleSetup()
{
  String bleName = "IFTECH_SW_" + WiFi.macAddress();
  BLEDevice::init(bleName.c_str());
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  BLEService *pService = pServer->createService(SERVICE_UUID);
  pTxCharacteristic = pService->createCharacteristic(
      CHARACTERISTIC_UUID_TX, BLECharacteristic::PROPERTY_NOTIFY);
  pTxCharacteristic->addDescriptor(new BLE2902());
  BLECharacteristic *pRxCharacteristic = pService->createCharacteristic(
      CHARACTERISTIC_UUID_RX, BLECharacteristic::PROPERTY_WRITE);
  pRxCharacteristic->setCallbacks(new MyCallbacks());
  pService->start();
  pServer->getAdvertising()->start();
  Serial.printf("[BLE] %s\n", bleName.c_str());
}

void bleCheck()
{
  if (deviceConnected)
  {
    mySerialBT.deviceConnected = true;
    if (mySerialBT.available())
    {
      String cmd = mySerialBT.readString();
      cliParseLine(cmd);
      mySerialBT.printf("*** %s", cmd.c_str());
    }
  }
  else
    mySerialBT.deviceConnected = false;

  static String serialReceiveString;
  while (Serial.available() > 0)
  {
    char c = (char)Serial.read();
    if (c == '\r')
      continue;
    if (c == '\n' || serialReceiveString.length() > 30)
    {
      if (serialReceiveString.length() > 0)
      {
        cliParseLine(serialReceiveString);
        Serial.printf("*** %s\r\n", serialReceiveString.c_str());
        serialReceiveString = "";
      }
    }
    else
      serialReceiveString += c;
  }

  if (!deviceConnected && oldDeviceConnected)
  {
    delay(500);
    pServer->startAdvertising();
    oldDeviceConnected = deviceConnected;
  }
  if (deviceConnected && !oldDeviceConnected)
    oldDeviceConnected = deviceConnected;
}
