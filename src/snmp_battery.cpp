#include "snmp_battery.h"
#include "samwoo_poll.h"
#include "eth_w610.h"

#include <SNMP_Agent.h>

static SNMPAgent snmp("public", "private");

static uint32_t sysUptime = 0;
static const char *sysDescr = "Samwoo BMS Display Pack1+Pack2";
static const char *sysName = "BAT-DISPLAY";
static const char *sysObjectID = "1.3.6.1.4.1.99999.1";
static int sysServices = 72;
static int sWalkEnd = 0;

enum
{
  K_U16 = 0,
  K_I16 = 1,
  K_BIT = 2
};

struct BatOid
{
  const char *oid;
  uint8_t pack;
  uint16_t addr;
  uint8_t kind;
  int8_t bit;
  int value;
};

static BatOid sFixed[] = {
    {".1.3.6.1.2.1.32.1.7.0", 0, 0, K_U16, -1, 0},
    {".1.3.6.1.2.1.32.1.4.0", 0, 1, K_U16, -1, 0},
    {".1.3.6.1.2.1.32.1.3.0", 0, 2, K_U16, -1, 0},
    {".1.3.6.1.2.1.32.1.9.0", 0, 3, K_U16, -1, 0},
    {".1.3.6.1.2.1.32.1.8.0", 0, 4, K_I16, -1, 0},
    {".1.3.6.1.2.1.32.1.2.0", 0, 5, K_I16, -1, 0},
    {".1.3.6.1.2.1.32.1.11.0", 0, 6, K_U16, -1, 0},
    {".1.3.6.1.2.1.32.1.12.0", 0, 7, K_U16, -1, 0},
    {".1.3.6.1.2.1.32.1.13.0", 0, 8, K_I16, -1, 0},
    {".1.3.6.1.2.1.32.1.14.0", 0, 9, K_I16, -1, 0},
    {".1.3.6.1.2.1.32.1.6.4.0", 0, SAMWOO_REG_RELAY, K_BIT, 0, 0},
    {".1.3.6.1.2.1.32.1.6.5.0", 0, SAMWOO_REG_RELAY, K_BIT, 1, 0},
    {".1.3.6.1.2.1.32.1.6.1.0", 0, SAMWOO_REG_FAULT, K_U16, -1, 0},
    {".1.3.6.1.2.1.32.1.10.0", 0, SAMWOO_REG_PROTECT, K_U16, -1, 0},
    {".1.3.6.1.2.1.32.1.6.2.0", 0, SAMWOO_REG_WARNING, K_U16, -1, 0},
    {".1.3.6.1.2.1.32.1.15.0", 0, SAMWOO_REG_CELLNUM, K_U16, -1, 0},
    {".1.3.6.1.2.1.32.2.7.0", 1, 0, K_U16, -1, 0},
    {".1.3.6.1.2.1.32.2.4.0", 1, 1, K_U16, -1, 0},
    {".1.3.6.1.2.1.32.2.3.0", 1, 2, K_U16, -1, 0},
    {".1.3.6.1.2.1.32.2.9.0", 1, 3, K_U16, -1, 0},
    {".1.3.6.1.2.1.32.2.8.0", 1, 4, K_I16, -1, 0},
    {".1.3.6.1.2.1.32.2.2.0", 1, 5, K_I16, -1, 0},
    {".1.3.6.1.2.1.32.2.11.0", 1, 6, K_U16, -1, 0},
    {".1.3.6.1.2.1.32.2.12.0", 1, 7, K_U16, -1, 0},
    {".1.3.6.1.2.1.32.2.13.0", 1, 8, K_I16, -1, 0},
    {".1.3.6.1.2.1.32.2.14.0", 1, 9, K_I16, -1, 0},
    {".1.3.6.1.2.1.32.2.6.4.0", 1, SAMWOO_REG_RELAY, K_BIT, 0, 0},
    {".1.3.6.1.2.1.32.2.6.5.0", 1, SAMWOO_REG_RELAY, K_BIT, 1, 0},
    {".1.3.6.1.2.1.32.2.6.1.0", 1, SAMWOO_REG_FAULT, K_U16, -1, 0},
    {".1.3.6.1.2.1.32.2.10.0", 1, SAMWOO_REG_PROTECT, K_U16, -1, 0},
    {".1.3.6.1.2.1.32.2.6.2.0", 1, SAMWOO_REG_WARNING, K_U16, -1, 0},
    {".1.3.6.1.2.1.32.2.15.0", 1, SAMWOO_REG_CELLNUM, K_U16, -1, 0},
};

static char sCellOid[2][16][40];
static int sCellVal[2][16];
static char sTempOid[2][8][40];
static int sTempVal[2][8];

static int decode(uint8_t pack, uint16_t addr, uint8_t kind, int8_t bit)
{
  const uint16_t raw = samwooReg(pack, addr);
  if (kind == K_I16)
  {
    return (int)(int16_t)raw;
  }
  if (kind == K_BIT)
  {
    return (raw >> bit) & 1;
  }
  return (int)raw;
}

void snmpBatteryRefresh()
{
  sysUptime = millis() / 10;
  for (size_t i = 0; i < sizeof(sFixed) / sizeof(sFixed[0]); ++i)
  {
    sFixed[i].value = decode(sFixed[i].pack, sFixed[i].addr, sFixed[i].kind, sFixed[i].bit);
  }
  for (uint8_t p = 0; p < 2; ++p)
  {
    for (uint8_t i = 0; i < 16; ++i)
    {
      sCellVal[p][i] = (int)samwooReg(p, (uint16_t)(SAMWOO_REG_CELL0 + i));
    }
    for (uint8_t i = 0; i < 8; ++i)
    {
      sTempVal[p][i] = (int)(int16_t)samwooReg(p, (uint16_t)(SAMWOO_REG_TEMP0 + i));
    }
  }
}

void snmpBatteryBegin()
{
  snmp.setUDP(ethW610SnmpUdp());
  snmp.addReadOnlyStaticStringHandler(".1.3.6.1.2.1.1.1.0", sysDescr);
  snmp.addReadOnlyStaticStringHandler(".1.3.6.1.2.1.1.2.0", sysObjectID);
  snmp.addTimestampHandler(".1.3.6.1.2.1.1.3.0", &sysUptime);
  snmp.addReadOnlyStaticStringHandler(".1.3.6.1.2.1.1.5.0", sysName);
  snmp.addIntegerHandler(".1.3.6.1.2.1.1.7.0", &sysServices);

  for (size_t i = 0; i < sizeof(sFixed) / sizeof(sFixed[0]); ++i)
  {
    snmp.addIntegerHandler(sFixed[i].oid, &sFixed[i].value);
  }
  for (uint8_t p = 0; p < 2; ++p)
  {
    for (uint8_t i = 0; i < 16; ++i)
    {
      snprintf(sCellOid[p][i], sizeof(sCellOid[p][i]), ".1.3.6.1.2.1.32.%u.1.%u.0", (unsigned)(p + 1),
               (unsigned)(i + 1));
      snmp.addIntegerHandler(sCellOid[p][i], &sCellVal[p][i]);
    }
    for (uint8_t i = 0; i < 8; ++i)
    {
      snprintf(sTempOid[p][i], sizeof(sTempOid[p][i]), ".1.3.6.1.2.1.32.%u.5.%u.0", (unsigned)(p + 1),
               (unsigned)(i + 1));
      snmp.addIntegerHandler(sTempOid[p][i], &sTempVal[p][i]);
    }
  }
  /* Walk terminators: next GETNEXT leaves the walked subtree (NMS ignores endOfMibView). */
  snmp.addIntegerHandler(".1.3.6.1.2.1.32.3.1.0", &sWalkEnd); /* after pack2Value */
  snmp.addIntegerHandler(".1.3.6.1.2.1.33.0", &sWalkEnd);     /* after batStatus */
  snmp.addIntegerHandler(".1.3.6.1.2.2.0", &sWalkEnd);        /* after batCommon */
  snmp.sortHandlers();
  snmpBatteryRefresh();
  Serial.println("[SNMP] UDP 161 community=public  pack1=1.3.6.1.2.1.32.1  pack2=...32.2");
}

void snmpBatteryLoop()
{
  snmp.loop();
}
