#include "samwoo_poll.h"

#include "board_pins.h"
#include "naradav13.h"

uint16_t samwooRegs[SAMWOO_PACKS][SAMWOO_REG_MAX] = {{0}};
bool samwooOk[SAMWOO_PACKS] = {false, false};
uint32_t samwooLastOkMs[SAMWOO_PACKS] = {0, 0};
uint32_t samwooErrorCount = 0;

extern NaradaClient232 naradaClient;

static uint8_t lrc8(const uint8_t *data, size_t len)
{
  uint8_t sum = 0;
  for (size_t i = 0; i < len; ++i)
  {
    sum = (uint8_t)(sum + data[i]);
  }
  return (uint8_t)((~sum) + 1);
}

static bool parseRegisters(const uint8_t *pdu, size_t pduLen, uint8_t pack, uint16_t expectedQty)
{
  if (pduLen < 3)
  {
    return false;
  }
  const uint8_t byteCount = pdu[2];
  if (byteCount < expectedQty * 2 || (size_t)(3 + byteCount) > pduLen)
  {
    return false;
  }
  for (uint16_t i = 0; i < expectedQty && i < SAMWOO_REG_MAX; ++i)
  {
    samwooRegs[pack][i] = (uint16_t)((pdu[3 + i * 2] << 8) | pdu[4 + i * 2]);
  }
  return true;
}

static const uint32_t kPollTimeoutMs = 250;
static const uint32_t kCycleGapMs = 1000;

enum
{
  ST_IDLE = 0,
  ST_WAIT = 1
};

static uint8_t sState = ST_IDLE;
static uint8_t sPack = 0;
static uint32_t sWaitStartMs = 0;
static uint32_t sNextCycleMs = 0;
static uint8_t sRx[160];
static size_t sRxN = 0;
static uint32_t sLogCount = 0;

static void sendRequest(uint8_t slave)
{
  uint8_t body[6];
  body[0] = slave;
  body[1] = 0x04;
  body[2] = 0;
  body[3] = 0;
  body[4] = (uint8_t)(SAMWOO_REG_MAX >> 8);
  body[5] = (uint8_t)(SAMWOO_REG_MAX & 0xFF);
  const uint8_t lrc = lrc8(body, sizeof(body));

  while (Serial1.available())
  {
    Serial1.read();
  }
  Serial1.write(0x3A);
  Serial1.write(body, sizeof(body));
  Serial1.write(lrc);
  Serial1.write(0x0D);
  Serial1.write(0x0A);
  sRxN = 0;
  sWaitStartMs = millis();
}

static bool parseFrame(uint8_t slave, uint8_t pack)
{
  if (sRxN < 8 || sRx[0] != 0x3A || sRx[sRxN - 2] != 0x0D || sRx[sRxN - 1] != 0x0A)
  {
    return false;
  }
  const size_t payloadLen = sRxN - 3;
  if (payloadLen < 4)
  {
    return false;
  }
  const uint8_t gotLrc = sRx[1 + payloadLen - 1];
  if (lrc8(&sRx[1], payloadLen - 1) != gotLrc)
  {
    return false;
  }
  if (sRx[1] != slave)
  {
    return false;
  }
  return parseRegisters(&sRx[1], payloadLen - 1, pack, SAMWOO_REG_MAX);
}

static bool frameComplete()
{
  while (Serial1.available() && sRxN < sizeof(sRx))
  {
    sRx[sRxN++] = (uint8_t)Serial1.read();
    if (sRxN >= 2 && sRx[sRxN - 2] == 0x0D && sRx[sRxN - 1] == 0x0A)
    {
      return true;
    }
  }
  return false;
}

static void finishPack(bool ok)
{
  samwooOk[sPack] = ok;
  if (ok)
  {
    samwooLastOkMs[sPack] = millis();
  }
  else
  {
    samwooErrorCount++;
  }
}

static void logStatus()
{
  if ((++sLogCount % 5) != 0)
  {
    return;
  }
  Serial.printf("[SAMWOO] p1 %s SOC=%u V=%u C1=%u  p2 %s SOC=%u V=%u C1=%u err=%lu\n",
                samwooOk[0] ? "OK" : "FAIL", samwooReg(0, 2), samwooReg(0, 4), samwooReg(0, 17),
                samwooOk[1] ? "OK" : "FAIL", samwooReg(1, 2), samwooReg(1, 4), samwooReg(1, 17),
                (unsigned long)samwooErrorCount);
}

void samwooBegin()
{
  Serial1.begin(RS485_UART_BAUD, SERIAL_8N1, PIN_RS485_RX, PIN_RS485_TX);
  Serial.printf("[SAMWOO] Serial1 %u 8N1 TX=%d RX=%d slaves=1,2\n",
                (unsigned)RS485_UART_BAUD, PIN_RS485_TX, PIN_RS485_RX);
}

bool samwooPollTick()
{
  if (sState == ST_IDLE)
  {
    if ((int32_t)(millis() - sNextCycleMs) < 0)
    {
      return false;
    }
    sPack = 0;
    sendRequest(1);
    sState = ST_WAIT;
    return false;
  }

  const uint8_t slave = (uint8_t)(sPack + 1);
  bool done = false;
  bool ok = false;
  if (frameComplete())
  {
    ok = parseFrame(slave, sPack);
    done = true;
  }
  else if ((millis() - sWaitStartMs) >= kPollTimeoutMs)
  {
    done = true;
    ok = false;
  }
  if (!done)
  {
    return false;
  }

  finishPack(ok);
  sPack++;
  if (sPack < SAMWOO_PACKS)
  {
    sendRequest((uint8_t)(sPack + 1));
    sState = ST_WAIT;
    return false;
  }

  sState = ST_IDLE;
  sNextCycleMs = millis() + kCycleGapMs;
  if (samwooOk[0] || samwooOk[1])
  {
    samwooFillNarada();
  }
  logStatus();
  return true;
}

bool samwooPoll()
{
  const uint32_t t0 = millis();
  while ((millis() - t0) < 800)
  {
    if (samwooPollTick())
    {
      return samwooOk[0] || samwooOk[1];
    }
  }
  return false;
}

uint16_t samwooReg(uint8_t pack, uint16_t address)
{
  if (pack >= SAMWOO_PACKS || address >= SAMWOO_REG_MAX)
  {
    return 0;
  }
  return samwooRegs[pack][address];
}

void samwooFillNarada()
{
  for (uint8_t pack = 0; pack < SAMWOO_PACKS; ++pack)
  {
    batteryInofo_t *b = &naradaClient.batInfo[pack];
    if (!samwooOk[pack])
    {
      continue;
    }
    const uint16_t *r = samwooRegs[pack];
    b->Capacity = (int)r[1];
    b->soc = (int)r[2] * 100;
    b->SOH = (int)r[3] * 100;
    b->totalVoltage = (int)r[4] * 10;
    b->ampere = 30000 + (int16_t)r[5] * 10;
    b->voltageNumber = r[16] > 15 ? 15 : (int)r[16];
    if (b->voltageNumber < 1)
    {
      b->voltageNumber = 15;
    }
    for (int i = 0; i < 15; ++i)
    {
      b->voltage[i] = (int)r[17 + i];
    }
    b->TempreatureNumber = 4;
    for (int i = 0; i < 4; ++i)
    {
      b->Tempreature[i] = 50 + ((int16_t)r[32 + i] / 10);
    }
    b->BMS_PROTECT_STATUS = (int)r[14];
  }
}
