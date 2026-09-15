#include "status_leds.h"

#include <stdio.h>
#include <string.h>

#include "ui.h"
#include "samwoo_poll.h"
#include "SerialProtocalParse.h"

static lv_obj_t *sLampComm;
static lv_obj_t *sLampChg;
static lv_obj_t *sLampDsg;
static lv_obj_t *sLampWarn;
static uint32_t sCommUntilMs;
static uint32_t sLastOkMs[2];
static int sLedPack = -1;
static bool sLastComm;
static bool sLastChg;
static bool sLastDsg;
static bool sLastWarn;
static char sLastStatus[48];
static uint32_t sLastStatusColor;

static void setLamp(lv_obj_t *lamp, bool on, uint32_t onColor)
{
  lv_obj_set_style_bg_color(lamp, lv_color_hex(on ? onColor : 0x3A3A3A), 0);
  lv_obj_set_style_bg_opa(lamp, on ? LV_OPA_COVER : LV_OPA_80, 0);
}

static uint16_t packAlarm(int pack)
{
  return (uint16_t)(samwooReg(pack, SAMWOO_REG_FAULT) | samwooReg(pack, SAMWOO_REG_PROTECT) | samwooReg(pack, SAMWOO_REG_WARNING));
}

bool statusLedsHasAlarm(int pack)
{
  return packAlarm(pack) != 0;
}

/* 실팩=통신_RX 시트. Protect/Warning 동일 비트, Fault는 별도. */
static const char *alarmCause(int pack)
{
  const uint16_t fault = samwooReg(pack, SAMWOO_REG_FAULT);
  const uint16_t pw = (uint16_t)(samwooReg(pack, SAMWOO_REG_PROTECT) | samwooReg(pack, SAMWOO_REG_WARNING));
  if (pw == 0 && fault == 0)
  {
    return NULL;
  }
  if ((pw & (1u << 6)) || (fault & (1u << 5)))
  {
    return "과온";
  }
  if (pw & (1u << 7))
  {
    return "저온";
  }
  if (pw & ((1u << 0) | (1u << 2)))
  {
    return "과압";
  }
  if (pw & ((1u << 1) | (1u << 3)))
  {
    return "저압";
  }
  if ((pw & ((1u << 4) | (1u << 5))) || (fault & (1u << 2)))
  {
    return "과전류";
  }
  if (fault & (1u << 0))
  {
    return "과충전";
  }
  if (fault & (1u << 1))
  {
    return "과방전";
  }
  if (fault & (1u << 3))
  {
    return "MCCB";
  }
  if (fault & (1u << 4))
  {
    return "SOC";
  }
  if (fault & (1u << 6))
  {
    return "통신";
  }
  return NULL;
}

const char *statusLedsAlarmCause(int pack)
{
  return alarmCause(pack);
}

static void appendAlarm(char *out, size_t cap, int pack)
{
  const char *cause = alarmCause(pack);
  char one[24];
  if (cause)
  {
    snprintf(one, sizeof(one), "경보#%d:%s", pack + 1, cause);
  }
  else
  {
    snprintf(one, sizeof(one), "경보#%d", pack + 1);
  }
  if (out[0] != '\0')
  {
    strncat(out, " ", cap - strlen(out) - 1);
  }
  strncat(out, one, cap - strlen(out) - 1);
}

static void updateStatusText(int selected, bool warn)
{
  char text[48];
  text[0] = '\0';
  if (packAlarm(0))
  {
    appendAlarm(text, sizeof(text), 0);
  }
  if (packAlarm(1))
  {
    appendAlarm(text, sizeof(text), 1);
  }
  if (text[0] == '\0')
  {
    if (samwooOk[selected])
    {
      snprintf(text, sizeof(text), "STATUS:#%d 정상", selected + 1);
    }
    else
    {
      snprintf(text, sizeof(text), "STATUS:#%d 통신끊김", selected + 1);
    }
  }

  const uint32_t color = warn ? 0xFF5252 : 0xFFFFFF;
  if (strcmp(text, sLastStatus) == 0 && color == sLastStatusColor)
  {
    return;
  }
  strncpy(sLastStatus, text, sizeof(sLastStatus) - 1);
  sLastStatus[sizeof(sLastStatus) - 1] = '\0';
  sLastStatusColor = color;
  lv_label_set_text(ui_CompanyLabel1, text);
  lv_obj_set_style_text_color(ui_CompanyLabel1, lv_color_hex(color), 0);
}

static lv_obj_t *makeItem(lv_obj_t *parent, const char *caption, lv_obj_t **lampOut)
{
  lv_obj_t *box = lv_obj_create(parent);
  lv_obj_remove_style_all(box);
  lv_obj_set_size(box, 48, 44);
  lv_obj_set_flex_flow(box, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(box, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_clear_flag(box, (lv_obj_flag_t)(LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE));
  lv_obj_set_style_pad_row(box, 2, 0);

  lv_obj_t *lamp = lv_obj_create(box);
  lv_obj_remove_style_all(lamp);
  lv_obj_set_size(lamp, 14, 14);
  lv_obj_set_style_radius(lamp, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_color(lamp, lv_color_hex(0x3A3A3A), 0);
  lv_obj_set_style_bg_opa(lamp, LV_OPA_80, 0);
  lv_obj_set_style_border_width(lamp, 1, 0);
  lv_obj_set_style_border_color(lamp, lv_color_hex(0x888888), 0);
  lv_obj_clear_flag(lamp, (lv_obj_flag_t)(LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE));
  *lampOut = lamp;

  lv_obj_t *lbl = lv_label_create(box);
  lv_label_set_text(lbl, caption);
  lv_obj_set_style_text_color(lbl, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_text_font(lbl, &ui_font_malgunFont1, 0);
  return box;
}

void statusLedsBegin()
{
  lv_obj_set_flex_align(ui_MainBottomPannel, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_column(ui_MainBottomPannel, 4, 0);
  lv_obj_set_style_pad_left(ui_MainBottomPannel, 6, 0);
  lv_obj_set_style_pad_right(ui_MainBottomPannel, 6, 0);

  lv_obj_set_width(ui_CompanyLabel, 108);
  lv_obj_set_height(ui_CompanyLabel, 44);
  lv_obj_set_style_text_align(ui_CompanyLabel, LV_TEXT_ALIGN_LEFT, 0);

  lv_obj_t *bar = lv_obj_create(ui_MainBottomPannel);
  lv_obj_remove_style_all(bar);
  lv_obj_set_size(bar, 200, 44);
  lv_obj_set_flex_flow(bar, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(bar, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_clear_flag(bar, (lv_obj_flag_t)(LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE));

  makeItem(bar, "통신", &sLampComm);
  makeItem(bar, "충전", &sLampChg);
  makeItem(bar, "방전", &sLampDsg);
  makeItem(bar, "경고", &sLampWarn);

  lv_obj_set_width(ui_CompanyLabel1, LV_SIZE_CONTENT);
  lv_obj_set_height(ui_CompanyLabel1, 44);
  lv_obj_set_flex_grow(ui_CompanyLabel1, 1);
  lv_obj_set_style_text_align(ui_CompanyLabel1, LV_TEXT_ALIGN_LEFT, 0);
  lv_obj_set_style_text_font(ui_CompanyLabel1, &ui_font_malgunFont1, 0);
  lv_obj_set_style_pad_left(ui_CompanyLabel1, 2, 0);
  lv_label_set_text(ui_CompanyLabel1, "STATUS:#1 정상");

  lv_obj_set_width(ui_VersionLabel, 72);
  lv_obj_set_height(ui_VersionLabel, 44);

  lv_obj_move_to_index(ui_CompanyLabel, 0);
  lv_obj_move_to_index(bar, 1);
  lv_obj_move_to_index(ui_CompanyLabel1, 2);
  lv_obj_move_to_index(ui_VersionLabel, 3);

  sLastOkMs[0] = samwooLastOkMs[0];
  sLastOkMs[1] = samwooLastOkMs[1];
  sLastStatus[0] = '\0';
}

void statusLedsLoop()
{
  if (sLampComm == NULL)
  {
    return;
  }

  const int pack = selectedPackIndex();
  const bool packChanged = (pack != sLedPack);
  if (packChanged)
  {
    sLedPack = pack;
    sLastOkMs[pack] = samwooLastOkMs[pack];
  }
  if (samwooLastOkMs[pack] != sLastOkMs[pack])
  {
    sLastOkMs[pack] = samwooLastOkMs[pack];
    sCommUntilMs = millis() + 180;
  }

  const bool comm = (int32_t)(sCommUntilMs - millis()) > 0;
  const uint16_t relay = samwooReg(pack, SAMWOO_REG_RELAY);
  const bool chg = (relay & 0x0001) != 0;
  const bool dsg = (relay & 0x0002) != 0;
  const bool warn = (packAlarm(0) | packAlarm(1)) != 0;

  if (packChanged || comm != sLastComm)
  {
    sLastComm = comm;
    setLamp(sLampComm, comm, 0x00E676);
  }
  if (packChanged || chg != sLastChg)
  {
    sLastChg = chg;
    setLamp(sLampChg, chg, 0xFFC107);
  }
  if (packChanged || dsg != sLastDsg)
  {
    sLastDsg = dsg;
    setLamp(sLampDsg, dsg, 0x29B6F6);
  }
  if (warn != sLastWarn)
  {
    sLastWarn = warn;
    setLamp(sLampWarn, warn, 0xFF1744);
  }
  updateStatusText(pack, warn);
}
