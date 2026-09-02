#include "status_leds.h"

#include "src/ui.h"
#include "samwoo_poll.h"

static lv_obj_t *sLampComm;
static lv_obj_t *sLampChg;
static lv_obj_t *sLampDsg;
static lv_obj_t *sLampWarn;
static uint32_t sCommUntilMs;
static uint32_t sLastOkMs[2];
static bool sLastComm;
static bool sLastChg;
static bool sLastDsg;
static bool sLastWarn;

static void setLamp(lv_obj_t *lamp, bool on, uint32_t onColor)
{
  lv_obj_set_style_bg_color(lamp, lv_color_hex(on ? onColor : 0x3A3A3A), 0);
  lv_obj_set_style_bg_opa(lamp, on ? LV_OPA_COVER : LV_OPA_80, 0);
}

static lv_obj_t *makeItem(lv_obj_t *parent, const char *caption, lv_obj_t **lampOut)
{
  lv_obj_t *box = lv_obj_create(parent);
  lv_obj_remove_style_all(box);
  lv_obj_set_size(box, 52, 44);
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
  lv_obj_set_width(ui_CompanyLabel1, 300);

  lv_obj_t *bar = lv_obj_create(ui_MainBottomPannel);
  lv_obj_remove_style_all(bar);
  lv_obj_set_size(bar, 220, 44);
  lv_obj_set_flex_flow(bar, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(bar, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_clear_flag(bar, (lv_obj_flag_t)(LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE));
  lv_obj_set_style_translate_x(bar, -10, 0);
  lv_obj_set_style_translate_y(bar, -10, 0);
  lv_obj_move_to_index(bar, 1);

  makeItem(bar, "통신", &sLampComm);
  makeItem(bar, "충전", &sLampChg);
  makeItem(bar, "방전", &sLampDsg);
  makeItem(bar, "경고", &sLampWarn);

  sLastOkMs[0] = samwooLastOkMs[0];
  sLastOkMs[1] = samwooLastOkMs[1];
}

void statusLedsLoop()
{
  if (sLampComm == NULL)
  {
    return;
  }

  if (samwooLastOkMs[0] != sLastOkMs[0] || samwooLastOkMs[1] != sLastOkMs[1])
  {
    sLastOkMs[0] = samwooLastOkMs[0];
    sLastOkMs[1] = samwooLastOkMs[1];
    sCommUntilMs = millis() + 180;
  }

  const bool comm = (int32_t)(sCommUntilMs - millis()) > 0;
  const uint16_t relay = (uint16_t)(samwooReg(0, 12) | samwooReg(1, 12));
  const bool chg = (relay & 0x0001) != 0;
  const bool dsg = (relay & 0x0002) != 0;
  const uint16_t alarm = (uint16_t)(samwooReg(0, 13) | samwooReg(0, 14) | samwooReg(0, 15) |
                                    samwooReg(1, 13) | samwooReg(1, 14) | samwooReg(1, 15));
  const bool warn = alarm != 0;

  if (comm != sLastComm)
  {
    sLastComm = comm;
    setLamp(sLampComm, comm, 0x00E676);
  }
  if (chg != sLastChg)
  {
    sLastChg = chg;
    setLamp(sLampChg, chg, 0xFFC107);
  }
  if (dsg != sLastDsg)
  {
    sLastDsg = dsg;
    setLamp(sLampDsg, dsg, 0x29B6F6);
  }
  if (warn != sLastWarn)
  {
    sLastWarn = warn;
    setLamp(sLampWarn, warn, 0xFF1744);
  }
}
