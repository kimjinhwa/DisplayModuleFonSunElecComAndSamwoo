#include "SerialProtocalParse.h"
#include "ui.h"
#include "ui_events.h"
#include "main.h"
#include "samwoo_poll.h"
#include "settings_kb.h"
#include "snmp_battery.h"
#include "board_rtc.h"
#include <EEPROM.h>
#include <stdio.h>
#include <string.h>

static int sSel = 0;
extern nvsSystemSet ipAddress_struct;

static lv_obj_t *ui_packVoltage[2];
static lv_obj_t *ui_cellVoltage[16];

static void bindWidgets(void)
{
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
  ui_cellVoltage[15] = ui_lblvoltage16;
  ui_packVoltage[0] = ui_lblPack1;
  ui_packVoltage[1] = ui_lblPack2;
}

static void setLabelIfChanged(lv_obj_t *lbl, const char *text)
{
  if (lbl == NULL || text == NULL)
  {
    return;
  }
  const char *cur = lv_label_get_text(lbl);
  if (cur != NULL && strcmp(cur, text) == 0)
  {
    return;
  }
  lv_label_set_text(lbl, text);
}

static void paintPackButtons(int selected)
{
  char buf[24];
  for (int p = 0; p < 2; ++p)
  {
    if (samwooOk[p])
    {
      snprintf(buf, sizeof(buf), "#%d  %.1fV", p + 1, samwooReg(p, SAMWOO_REG_VOLT) / 10.0f);
    }
    else
    {
      snprintf(buf, sizeof(buf), "#%d  --V", p + 1);
    }
    setLabelIfChanged(ui_packVoltage[p], buf);
  }
  static int sBtnSel = -1;
  if (sBtnSel == selected)
  {
    return;
  }
  sBtnSel = selected;
  lv_obj_set_style_bg_color(ui_btnPack1, lv_color_hex(selected == 0 ? 0x1B7A3A : 0x332222),
                            LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(ui_btnPack2, lv_color_hex(selected == 1 ? 0x1B7A3A : 0x332222),
                            LV_PART_MAIN | LV_STATE_DEFAULT);
}

static void paintMain(int pack)
{
  char buf[40];
  snprintf(buf, sizeof(buf), "%s-%d", ipAddress_struct.deviceName, pack + 1);
  setLabelIfChanged(ui_HeaderTitle, buf);

  float sum = 0;
  int nOk = 0;
  for (int p = 0; p < SAMWOO_PACKS; ++p)
  {
    if (samwooOk[p])
    {
      sum += samwooReg(p, SAMWOO_REG_VOLT) / 10.0f;
      nOk++;
    }
  }
  if (nOk > 0)
  {
    snprintf(buf, sizeof(buf), "AVG  :%.1f", sum / nOk);
  }
  else
  {
    snprintf(buf, sizeof(buf), "AVG  :--");
  }
  setLabelIfChanged(ui_lblOutputVoltage, buf);

  if (!samwooOk[pack])
  {
    setLabelIfChanged(ui_lblTotalAmpere, "AMP  :--");
    setLabelIfChanged(ui_lblTotalTemperature, "TEMP : --");
    setLabelIfChanged(ui_lblHighVoltage, "HVOL :--");
    setLabelIfChanged(ui_lblLowVoltage, "LVOL :--");
    setLabelIfChanged(ui_lblDiff, "DIFF :--");
    for (int i = 0; i < 16; ++i)
    {
      setLabelIfChanged(ui_cellVoltage[i], "--");
    }
    paintPackButtons(pack);
    return;
  }

  const int16_t amp = (int16_t)samwooReg(pack, SAMWOO_REG_CUR);
  snprintf(buf, sizeof(buf), "AMP  :%.1f", amp / 10.0f);
  setLabelIfChanged(ui_lblTotalAmpere, buf);

  int tsum = 0;
  for (int i = 0; i < 4; ++i)
  {
    tsum += (int16_t)samwooReg(pack, (uint16_t)(SAMWOO_REG_TEMP0 + i));
  }
  snprintf(buf, sizeof(buf), "TEMP : %.1f", (tsum / 4) / 10.0f);
  setLabelIfChanged(ui_lblTotalTemperature, buf);

  int nCell = (int)samwooReg(pack, SAMWOO_REG_CELLNUM);
  if (nCell < 1 || nCell > 16)
  {
    nCell = 16;
  }
  uint16_t hi = 0;
  uint16_t lo = 0xFFFF;
  for (int i = 0; i < nCell; ++i)
  {
    const uint16_t mv = samwooReg(pack, (uint16_t)(SAMWOO_REG_CELL0 + i));
    if (mv > hi)
    {
      hi = mv;
    }
    if (mv > 0 && mv < lo)
    {
      lo = mv;
    }
  }
  if (lo == 0xFFFF)
  {
    lo = 0;
  }
  snprintf(buf, sizeof(buf), "HVOL :%.3fV", hi / 1000.0f);
  setLabelIfChanged(ui_lblHighVoltage, buf);
  snprintf(buf, sizeof(buf), "LVOL :%.3fV", lo / 1000.0f);
  setLabelIfChanged(ui_lblLowVoltage, buf);
  snprintf(buf, sizeof(buf), "DIFF :%dmV", (int)hi - (int)lo);
  setLabelIfChanged(ui_lblDiff, buf);

  const int tAt[4] = {0, 3, 7, 11};
  for (int i = 0; i < 16; ++i)
  {
    if (i >= nCell)
    {
      setLabelIfChanged(ui_cellVoltage[i], "--");
      continue;
    }
    const uint16_t mv = samwooReg(pack, (uint16_t)(SAMWOO_REG_CELL0 + i));
    int slot = -1;
    for (int k = 0; k < 4; ++k)
    {
      if (tAt[k] == i)
      {
        slot = k;
        break;
      }
    }
    if (slot >= 0)
    {
      const float tc = ((int16_t)samwooReg(pack, (uint16_t)(SAMWOO_REG_TEMP0 + slot))) / 10.0f;
      snprintf(buf, sizeof(buf), "%.3f\n(%.0f)", mv / 1000.0f, tc);
    }
    else
    {
      snprintf(buf, sizeof(buf), "%.3f", mv / 1000.0f);
    }
    setLabelIfChanged(ui_cellVoltage[i], buf);
  }
  paintPackButtons(pack);
}

static void loadScreenNow(lv_obj_t *scr)
{
  if (scr != NULL && lv_scr_act() != scr)
  {
    lv_scr_load(scr);
  }
}

static void onSettingsOpen(lv_event_t *e)
{
  const lv_event_code_t c = lv_event_get_code(e);
  if (c == LV_EVENT_CLICKED || c == LV_EVENT_PRESSED)
  {
    settingsKbHide();
    setMemoryDataToLCD();
    loadScreenNow(ui_SettingScreen);
  }
}

static void onSettingsBack(lv_event_t *e)
{
  const lv_event_code_t c = lv_event_get_code(e);
  if (c == LV_EVENT_CLICKED || c == LV_EVENT_PRESSED)
  {
    settingsKbHide();
    loadScreenNow(ui_MainScreen);
  }
}

void initSamwooPackUi()
{
  bindWidgets();
  lv_obj_clear_flag(ui_MainMidPannel, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(ui_Button1, onSettingsOpen, LV_EVENT_CLICKED, NULL);
  lv_obj_add_event_cb(ui_Button1, onSettingsOpen, LV_EVENT_PRESSED, NULL);
  lv_obj_add_event_cb(ui_Button2, onSettingsBack, LV_EVENT_CLICKED, NULL);
  lv_obj_add_event_cb(ui_Button2, onSettingsBack, LV_EVENT_PRESSED, NULL);
  lv_obj_move_foreground(ui_Button1);
  lv_obj_move_foreground(ui_Button2);
  lv_obj_add_flag(ui_btnPack3, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(ui_btnPack4, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(ui_btnPack5, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(ui_btnPack6, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(ui_btnPack7, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(ui_btnPack8, LV_OBJ_FLAG_HIDDEN);
  lv_obj_set_height(ui_btnPack1, lv_pct(28));
  lv_obj_set_height(ui_btnPack2, lv_pct(28));
  paintPackButtons(0);
  settingsKbInit();
}

void serialProtocalparse()
{
  if (!samwooPollTick())
  {
    return;
  }
  snmpBatteryRefresh();
  paintMain(sSel);
}

int selectedPackIndex()
{
  return sSel;
}

void btnPackChange(lv_event_t *e)
{
  (void)e;
}

void btnEventPack1(lv_event_t *e)
{
  (void)e;
  sSel = 0;
  paintMain(0);
}

void btnEventPack2(lv_event_t *e)
{
  (void)e;
  sSel = 1;
  paintMain(1);
}

void btnEventPack3(lv_event_t *e) { (void)e; }
void btnEventPack4(lv_event_t *e) { (void)e; }
void btnEventPack5(lv_event_t *e) { (void)e; }
void btnEventPack6(lv_event_t *e) { (void)e; }
void btnEventPack7(lv_event_t *e) { (void)e; }
void btnEventPack8(lv_event_t *e) { (void)e; }

void saveButtenEvent(lv_event_t *e)
{
  (void)e;
  const uint32_t oldIp = ipAddress_struct.IPADDRESS;
  const uint32_t oldSn = ipAddress_struct.SUBNETMASK;
  const uint32_t oldGw = ipAddress_struct.GATEWAY;

  IPAddress ipaddress(
      String(lv_textarea_get_text(ui_txtIPADDRESS1)).toInt(),
      String(lv_textarea_get_text(ui_txtIPADDRESS2)).toInt(),
      String(lv_textarea_get_text(ui_txtIPADDRESS3)).toInt(),
      String(lv_textarea_get_text(ui_txtIPADDRESS4)).toInt());
  IPAddress subnet(
      String(lv_textarea_get_text(ui_txtSUBNET1)).toInt(),
      String(lv_textarea_get_text(ui_txtSUBNET2)).toInt(),
      String(lv_textarea_get_text(ui_txtSUBNET3)).toInt(),
      String(lv_textarea_get_text(ui_txtSUBNET4)).toInt());
  IPAddress gateway(
      String(lv_textarea_get_text(ui_txtGATEWAY1)).toInt(),
      String(lv_textarea_get_text(ui_txtGATEWAY2)).toInt(),
      String(lv_textarea_get_text(ui_txtGATEWAY3)).toInt(),
      String(lv_textarea_get_text(ui_txtGATEWAY4)).toInt());
  ipAddress_struct.IPADDRESS = (uint32_t)ipaddress;
  ipAddress_struct.SUBNETMASK = (uint32_t)subnet;
  ipAddress_struct.GATEWAY = (uint32_t)gateway;
  snprintf(ipAddress_struct.deviceName, 20, "%s", lv_textarea_get_text(ui_txtDEVICENAME));
  uint32_t saveMin = (uint32_t)String(lv_textarea_get_text(ui_txtScreenSaveTime)).toInt();
  if (saveMin > 999)
  {
    saveMin = 999;
  }
  ipAddress_struct.screenSaveMin = (uint16_t)saveMin;
  EEPROM.writeBytes(1, (const byte *)&ipAddress_struct, sizeof(nvsSystemSet));
  EEPROM.commit();

  BoardRtcTime want = {};
  want.year = (uint16_t)String(lv_textarea_get_text(ui_txtYear)).toInt();
  want.month = (uint8_t)String(lv_textarea_get_text(ui_txtMonth)).toInt();
  want.day = (uint8_t)String(lv_textarea_get_text(ui_txtDay)).toInt();
  want.hour = (uint8_t)String(lv_textarea_get_text(ui_txtTime)).toInt();
  want.minute = (uint8_t)String(lv_textarea_get_text(ui_txtMinute)).toInt();
  want.second = (uint8_t)String(lv_textarea_get_text(ui_txtSecond)).toInt();
  const bool timeOk = (want.year >= 2000 && want.year <= 2099 &&
                       want.month >= 1 && want.month <= 12 &&
                       want.day >= 1 && want.day <= 31 &&
                       want.hour <= 23 && want.minute <= 59 && want.second <= 59);
  if (timeOk)
  {
    boardRtcWrite(want);
  }

  setMemoryDataToLCD();

  const bool netChanged = (oldIp != ipAddress_struct.IPADDRESS) ||
                          (oldSn != ipAddress_struct.SUBNETMASK) ||
                          (oldGw != ipAddress_struct.GATEWAY);
  if (!netChanged)
  {
    lv_label_set_text(ui_CompanyLabel3, "저장 완료");
    lv_obj_set_style_text_font(ui_CompanyLabel3, &ui_font_malgunFont1, 0);
    return;
  }

  const char *msg = "시스템을 재 부팅후 적용합니다.";
  lv_label_set_text(ui_CompanyLabel3, msg);
  lv_obj_set_style_text_font(ui_CompanyLabel3, &ui_font_malgunFont1, 0);

  lv_obj_t *box = lv_obj_create(lv_scr_act());
  lv_obj_set_size(box, 720, 140);
  lv_obj_center(box);
  lv_obj_set_style_bg_color(box, lv_color_hex(0x202020), 0);
  lv_obj_set_style_bg_opa(box, LV_OPA_COVER, 0);
  lv_obj_set_style_border_color(box, lv_color_hex(0xFFC107), 0);
  lv_obj_set_style_border_width(box, 2, 0);
  lv_obj_clear_flag(box, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *lbl = lv_label_create(box);
  lv_label_set_text(lbl, msg);
  lv_obj_set_style_text_color(lbl, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_text_font(lbl, &ui_font_malgunFont1, 0);
  lv_obj_center(lbl);

  Serial.printf("\n[SET] IP %s reboot\n", ipaddress.toString().c_str());
  lv_timer_handler();
  delay(1800);
  ESP.restart();
}
