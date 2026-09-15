#include "settings_kb.h"
#include "ui.h"
#include <Arduino.h>
#include <string.h>

static lv_obj_t *sTarget = NULL;
static lv_obj_t *sEdit = NULL;

static void hideKb(void)
{
  if (ui_Keyboard1 != NULL)
  {
    lv_keyboard_set_textarea(ui_Keyboard1, NULL);
    lv_obj_add_flag(ui_Keyboard1, LV_OBJ_FLAG_HIDDEN);
  }
  if (sEdit != NULL)
  {
    lv_obj_add_flag(sEdit, LV_OBJ_FLAG_HIDDEN);
  }
  sTarget = NULL;
}

void settingsKbHide()
{
  hideKb();
}

static void applyAndHide(void)
{
  if (sTarget != NULL && sEdit != NULL)
  {
    lv_textarea_set_text(sTarget, lv_textarea_get_text(sEdit));
  }
  hideKb();
}

static void onKbValue(lv_event_t *e)
{
  if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED || sEdit == NULL)
  {
    return;
  }
  lv_obj_t *kb = lv_event_get_target(e);
  if (lv_keyboard_get_mode(kb) != LV_KEYBOARD_MODE_NUMBER)
  {
    lv_keyboard_def_event_cb(e);
    return;
  }
  const char *txt = lv_btnmatrix_get_btn_text(kb, lv_btnmatrix_get_selected_btn(kb));
  if (txt == NULL)
  {
    return;
  }
  if (strcmp(txt, "ENTER") == 0 || strcmp(txt, LV_SYMBOL_OK) == 0 ||
      strcmp(txt, LV_SYMBOL_NEW_LINE) == 0)
  {
    applyAndHide();
    return;
  }
  if (strcmp(txt, "ESC") == 0 || strcmp(txt, "CANCEL") == 0 ||
      strcmp(txt, LV_SYMBOL_CLOSE) == 0)
  {
    hideKb();
    return;
  }
  if (strcmp(txt, "DEL") == 0 || strcmp(txt, LV_SYMBOL_BACKSPACE) == 0)
  {
    lv_textarea_del_char(sEdit);
    return;
  }
  if (strcmp(txt, LV_SYMBOL_LEFT) == 0 || strcmp(txt, "<") == 0)
  {
    lv_textarea_cursor_left(sEdit);
    return;
  }
  if (strcmp(txt, LV_SYMBOL_RIGHT) == 0 || strcmp(txt, ">") == 0)
  {
    lv_textarea_cursor_right(sEdit);
    return;
  }
  if (strcmp(txt, "+/-") == 0)
  {
    const char *cur = lv_textarea_get_text(sEdit);
    if (cur != NULL && cur[0] == '-')
    {
      lv_textarea_set_cursor_pos(sEdit, 0);
      lv_textarea_del_char_forward(sEdit);
    }
    else
    {
      lv_textarea_set_cursor_pos(sEdit, 0);
      lv_textarea_add_char(sEdit, '-');
    }
    lv_textarea_set_cursor_pos(sEdit, LV_TEXTAREA_CURSOR_LAST);
    return;
  }
  if ((txt[0] >= '0' && txt[0] <= '9') || txt[0] == '.')
  {
    lv_textarea_add_text(sEdit, txt);
  }
}

static void showFor(lv_obj_t *field, bool number)
{
  if (field == NULL || sEdit == NULL || ui_Keyboard1 == NULL)
  {
    return;
  }
  sTarget = field;
  lv_textarea_set_text(sEdit, lv_textarea_get_text(field));
  lv_textarea_set_max_length(sEdit, lv_textarea_get_max_length(field));
  lv_textarea_set_accepted_chars(sEdit, number ? "0123456789" : NULL);
  lv_keyboard_set_mode(ui_Keyboard1, number ? LV_KEYBOARD_MODE_NUMBER : LV_KEYBOARD_MODE_TEXT_LOWER);
  lv_keyboard_set_textarea(ui_Keyboard1, sEdit);
  lv_obj_clear_flag(sEdit, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(ui_Keyboard1, LV_OBJ_FLAG_HIDDEN);
  lv_obj_move_foreground(sEdit);
  lv_obj_move_foreground(ui_Keyboard1);
  lv_textarea_set_cursor_pos(sEdit, LV_TEXTAREA_CURSOR_LAST);
  Serial.printf("[KB] show number=%d\n", (int)number);
}

static void onField(lv_event_t *e)
{
  const lv_event_code_t c = lv_event_get_code(e);
  if (c != LV_EVENT_CLICKED && c != LV_EVENT_PRESSED)
  {
    return;
  }
  lv_obj_t *field = lv_event_get_target(e);
  showFor(field, field != ui_txtDEVICENAME);
}

static void bindField(lv_obj_t *field, uint32_t maxLen)
{
  if (field == NULL)
  {
    return;
  }
  lv_obj_clear_flag(field, LV_OBJ_FLAG_CLICK_FOCUSABLE);
  lv_obj_add_flag(field, LV_OBJ_FLAG_CLICKABLE);
  lv_textarea_set_max_length(field, maxLen);
  lv_obj_add_event_cb(field, onField, LV_EVENT_CLICKED, NULL);
  lv_obj_add_event_cb(field, onField, LV_EVENT_PRESSED, NULL);
}

void settingsKbInit()
{
  if (ui_SettingScreen == NULL || ui_Keyboard1 == NULL)
  {
    return;
  }

  sEdit = lv_textarea_create(ui_SettingScreen);
  lv_obj_set_size(sEdit, 760, 56);
  lv_obj_align(sEdit, LV_ALIGN_TOP_MID, 0, 8);
  lv_textarea_set_one_line(sEdit, true);
  lv_obj_set_style_text_font(sEdit, LV_FONT_DEFAULT, 0);
  lv_obj_set_style_bg_color(sEdit, lv_color_hex(0x1A1A1A), 0);
  lv_obj_set_style_text_color(sEdit, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_border_color(sEdit, lv_color_hex(0xFFC107), 0);
  lv_obj_set_style_border_width(sEdit, 2, 0);
  lv_obj_add_flag(sEdit, LV_OBJ_FLAG_HIDDEN);

  static const char *num_map[] = {
      "1", "2", "3", "ESC", "\n",
      "4", "5", "6", "ENTER", "\n",
      "7", "8", "9", "CANCEL", "\n",
      "+/-", "0", LV_SYMBOL_BACKSPACE, LV_SYMBOL_LEFT, LV_SYMBOL_RIGHT, ""};
  static const lv_btnmatrix_ctrl_t num_ctrl[] = {
      1, 1, 1, 2,
      1, 1, 1, 2,
      1, 1, 1, 2,
      1, 1, 1, 1, 1};

  lv_obj_set_size(ui_Keyboard1, 800, 280);
  lv_obj_align(ui_Keyboard1, LV_ALIGN_BOTTOM_MID, 0, 0);
  lv_obj_set_style_text_font(ui_Keyboard1, LV_FONT_DEFAULT, LV_PART_ITEMS);
  lv_obj_set_style_text_font(ui_Keyboard1, LV_FONT_DEFAULT, LV_PART_MAIN);
  lv_keyboard_set_map(ui_Keyboard1, LV_KEYBOARD_MODE_NUMBER, num_map, num_ctrl);
  lv_obj_remove_event_cb(ui_Keyboard1, lv_keyboard_def_event_cb);
  lv_obj_add_event_cb(ui_Keyboard1, onKbValue, LV_EVENT_VALUE_CHANGED, NULL);
  lv_obj_add_flag(ui_Keyboard1, LV_OBJ_FLAG_HIDDEN);

  bindField(ui_txtIPADDRESS1, 3);
  bindField(ui_txtIPADDRESS2, 3);
  bindField(ui_txtIPADDRESS3, 3);
  bindField(ui_txtIPADDRESS4, 3);
  bindField(ui_txtSUBNET1, 3);
  bindField(ui_txtSUBNET2, 3);
  bindField(ui_txtSUBNET3, 3);
  bindField(ui_txtSUBNET4, 3);
  bindField(ui_txtGATEWAY1, 3);
  bindField(ui_txtGATEWAY2, 3);
  bindField(ui_txtGATEWAY3, 3);
  bindField(ui_txtGATEWAY4, 3);
  bindField(ui_txtDEVICENAME, 20);
  bindField(ui_txtYear, 4);
  bindField(ui_txtMonth, 2);
  bindField(ui_txtDay, 2);
  bindField(ui_txtTime, 2);
  bindField(ui_txtMinute, 2);
  bindField(ui_txtSecond, 2);
  bindField(ui_txtScreenSaveTime, 3);
}
