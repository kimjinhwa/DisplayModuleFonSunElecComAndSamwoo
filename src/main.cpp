#include <Arduino.h>
#include <lvgl.h>
#include <Arduino_GFX_Library.h>
#include <TFT_eSPI.h>
#include "ui.h"
#include <EEPROM.h>
#include "SerialProtocalParse.h"
#include "main.h"
#include "wifiOTA.h"
#include <esp_task_wdt.h>
#include <esp_wifi.h>
#include <Wire.h>
#include "board_pins.h"
#include "eth_w610.h"
#include "samwoo_poll.h"
#include "snmp_battery.h"
#include "ip_finder.h"
#include "web_fs.h"
#include "status_leds.h"
#include "myBlueTooth.h"
#include "Version.h"
#include "board_rtc.h"
#include <time.h>
#define GFX_BL DF_GFX_BL
#define TFT_BL 2
#define BRIGHT  155
#define WDT_TIMEOUT 15
#define BUTTON_ERASE 0

static uint32_t screenWidth;
static uint32_t screenHeight;

static lv_disp_draw_buf_t draw_buf;
static lv_color_t *disp_draw_buf;
static lv_disp_drv_t disp_drv;
static unsigned long last_ms;
uint16_t lcdOntime = 0;
static bool sLcdDim = false;

static uint16_t screenSaveSec(void)
{
  uint16_t m = ipAddress_struct.screenSaveMin;
  if (m > 999)
  {
    m = 0;
  }
  if (m == 0)
  {
    return 0;
  }
  return (uint16_t)(m * 60u);
}

Arduino_ESP32RGBPanel *bus = new Arduino_ESP32RGBPanel(
    GFX_NOT_DEFINED /* CS */, GFX_NOT_DEFINED /* SCK */, GFX_NOT_DEFINED /* SDA */,
    41 /* DE */, 40 /* VSYNC */, 39 /* HSYNC */, 42 /* PCLK */,
    14 /* R0 */, 21 /* R1 */, 47 /* R2 */, 48 /* R3 */, 45 /* R4 */,
    9 /* G0 */, 46 /* G1 */, 3 /* G2 */, 8 /* G3 */, 16 /* G4 */, 1 /* G5 */,
    15 /* B0 */, 7 /* B1 */, 6 /* B2 */, 5 /* B3 */, 4 /* B4 */
);
Arduino_RPi_DPI_RGBPanel *gfx = new Arduino_RPi_DPI_RGBPanel(
  bus,
    800 /* width */, 0 /* hsync_polarity */, 210 /* hsync_front_porch */, 30 /* hsync_pulse_width */, 16 /* hsync_back_porch */,
    480 /* height */, 0 /* vsync_polarity */, 22 /* vsync_front_porch */, 13 /* vsync_pulse_width */, 10 /* vsync_back_porch */,
    1 /* pclk_active_neg */, 12000000 /* prefer_speed */, true /* auto_flush */);

#include "touch.h"
#if LV_USE_LOG != 0
void my_print(const char * buf)
{
    Serial.printf(buf);
    Serial.flush();
}
#endif

void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
   uint32_t w = (area->x2 - area->x1 + 1);
   uint32_t h = (area->y2 - area->y1 + 1);
   gfx->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w, h);
   lv_disp_flush_ready(disp);
}
static lv_obj_t *cursor_obj;
static lv_indev_t *sTouchIndev = NULL;
void init_cursor() {
    cursor_obj = lv_obj_create(lv_layer_top());
    lv_obj_remove_style_all(cursor_obj);
    lv_obj_set_size(cursor_obj, 5, 5);
    lv_obj_set_style_radius(cursor_obj, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(cursor_obj, lv_color_hex(0xFF0000), 0);
    lv_obj_set_style_bg_opa(cursor_obj, LV_OPA_50, 0);
    lv_obj_set_style_border_width(cursor_obj, 0, 0);
    lv_obj_set_style_pad_all(cursor_obj, 0, 0);
    lv_obj_clear_flag(cursor_obj, (lv_obj_flag_t)(LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE));
    lv_obj_add_flag(cursor_obj, (lv_obj_flag_t)(LV_OBJ_FLAG_FLOATING | LV_OBJ_FLAG_IGNORE_LAYOUT | LV_OBJ_FLAG_HIDDEN));
    if (sTouchIndev)
    {
      lv_indev_set_cursor(sTouchIndev, cursor_obj);
    }
}
static unsigned long last_touch_time = 0;
#define TOUCH_TIMEOUT (3 * 60 * 1000)
void my_touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data)
{
  unsigned long current_time = millis();
  if ((current_time - last_touch_time) > TOUCH_TIMEOUT)
  {
    last_touch_time = current_time;
    lv_obj_clear_flag(cursor_obj, LV_OBJ_FLAG_HIDDEN);
    touch_init();
    Serial.println("\ntouch_init ok ");
  }

  if (touch_has_signal())
  {
    if (touch_touched())
    {
      last_touch_time = current_time;
      data->state = LV_INDEV_STATE_PR;
      data->point.x = touch_last_x;
      data->point.y = touch_last_y;
      lv_obj_clear_flag(cursor_obj, LV_OBJ_FLAG_HIDDEN);
      lv_obj_set_pos(cursor_obj,
                    touch_last_x - 10,
                    touch_last_y - 10);
      ledcWrite(0, BRIGHT);
      lcdOntime = 0;
      sLcdDim = false;
    }
    else
    {
      data->state = LV_INDEV_STATE_REL;
    }
  }
  else
  {
    data->state = LV_INDEV_STATE_REL;
  }
};

nvsSystemSet ipAddress_struct;
static bool sEthOk = false;
static bool sEthSvc = false;
static uint8_t sEraseHold = 0;
static unsigned long sEthRetryMs = 0;

static void ethServicesBeginIfNeeded(void)
{
  if (!sEthOk || sEthSvc)
  {
    return;
  }
  snmpBatteryBegin();
  ipFinderBegin();
  webHttpBegin();
  sEthSvc = true;
}

void nvsSave()
{
  EEPROM.writeBytes(1, (const byte *)&ipAddress_struct, sizeof(nvsSystemSet));
  EEPROM.commit();
}

static void nvsFactoryDefaults(void)
{
  memset(&ipAddress_struct, 0, sizeof(ipAddress_struct));
  ipAddress_struct.IPADDRESS = (uint32_t)IPAddress(192, 168, 0, 57);
  ipAddress_struct.GATEWAY = (uint32_t)IPAddress(192, 168, 0, 1);
  ipAddress_struct.SUBNETMASK = (uint32_t)IPAddress(255, 255, 255, 0);
  ipAddress_struct.WEBSOCKETSERVER = (uint32_t)IPAddress(192, 168, 0, 57);
  ipAddress_struct.DNS1 = (uint32_t)IPAddress(8, 8, 8, 8);
  ipAddress_struct.DNS2 = (uint32_t)IPAddress(164, 124, 101, 2);
  ipAddress_struct.WEBSERVERPORT = 80;
  ipAddress_struct.NTP_1 = (uint32_t)IPAddress(203, 248, 240, 140);
  ipAddress_struct.NTP_2 = (uint32_t)IPAddress(13, 209, 84, 50);
  ipAddress_struct.ntpuse = false;
  ipAddress_struct.HighVoltage = 36500;
  ipAddress_struct.LowVoltage = 26000;
  ipAddress_struct.HighImp = 80000;
  ipAddress_struct.HighTemp = 70;
  ipAddress_struct.alarmSetStatus = 0;
  strncpy(ipAddress_struct.deviceName, "BAT RACK1", 9);
  ipAddress_struct.isUpdate = false;
  strncpy(ipAddress_struct.ssid, "iftech", 6);
  strncpy(ipAddress_struct.password, "iftech0273", 10);
  ipAddress_struct.screenSaveMin = 0;
}

static void factoryResetNow(void)
{
  nvsFactoryDefaults();
  EEPROM.writeByte(0, 0x57);
  nvsSave();
  Serial.println("[IO] factory reset IP=192.168.0.57 web=80 reboot");
  if (ui_CompanyLabel3 != NULL)
  {
    lv_label_set_text(ui_CompanyLabel3, "초기화 후 재부팅");
    lv_obj_set_style_text_font(ui_CompanyLabel3, &ui_font_malgunFont1, 0);
    lv_timer_handler();
  }
  delay(1500);
  ESP.restart();
}
static lv_obj_t *sSettingsMacLabel;

static void settingsShowMac(void)
{
  if (ui_Panel4 == NULL)
  {
    return;
  }
  if (sSettingsMacLabel == NULL)
  {
    sSettingsMacLabel = lv_label_create(ui_SetRightPannel);
    lv_obj_set_width(sSettingsMacLabel, LV_SIZE_CONTENT);
    lv_obj_set_style_text_color(sSettingsMacLabel, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_text_opa(sSettingsMacLabel, 255, LV_PART_MAIN);
    lv_obj_align_to(sSettingsMacLabel, ui_Panel4, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 4);
  }
  char buf[56];
  snprintf(buf, sizeof(buf), "MacAddress : %s", ethW610MacString().c_str());
  lv_label_set_text(sSettingsMacLabel, buf);
}

void setMemoryDataToLCD(){

  IPAddress ipaddress(ipAddress_struct.IPADDRESS);
  lv_textarea_set_text(ui_txtIPADDRESS1,String(ipaddress[0]).c_str());
  lv_textarea_set_text(ui_txtIPADDRESS2,String(ipaddress[1]).c_str());
  lv_textarea_set_text(ui_txtIPADDRESS3,String(ipaddress[2]).c_str());
  lv_textarea_set_text(ui_txtIPADDRESS4,String(ipaddress[3]).c_str());

  IPAddress subnet(ipAddress_struct.SUBNETMASK);
  lv_textarea_set_text(ui_txtSUBNET1,String(subnet[0]).c_str());
  lv_textarea_set_text(ui_txtSUBNET2,String(subnet[1]).c_str());
  lv_textarea_set_text(ui_txtSUBNET3,String(subnet[2]).c_str());
  lv_textarea_set_text(ui_txtSUBNET4,String(subnet[3]).c_str());

  IPAddress gateway(ipAddress_struct.GATEWAY);
  lv_textarea_set_text(ui_txtGATEWAY1,String(gateway[0]).c_str());
  lv_textarea_set_text(ui_txtGATEWAY2,String(gateway[1]).c_str());
  lv_textarea_set_text(ui_txtGATEWAY3,String(gateway[2]).c_str());
  lv_textarea_set_text(ui_txtGATEWAY4,String(gateway[3]).c_str());

  lv_label_set_text(ui_HeaderTitle,ipAddress_struct.deviceName);
  lv_textarea_set_text(ui_txtDEVICENAME,ipAddress_struct.deviceName);
  settingsShowMac();

  BoardRtcTime rtc = {};
  bool have = false;
  time_t nowSec = time(NULL);
  struct tm tmNow;
  localtime_r(&nowSec, &tmNow);
  if (tmNow.tm_year + 1900 >= 2020)
  {
    rtc.year = (uint16_t)(tmNow.tm_year + 1900);
    rtc.month = (uint8_t)(tmNow.tm_mon + 1);
    rtc.day = (uint8_t)tmNow.tm_mday;
    rtc.hour = (uint8_t)tmNow.tm_hour;
    rtc.minute = (uint8_t)tmNow.tm_min;
    rtc.second = (uint8_t)tmNow.tm_sec;
    have = true;
  }
  char n[8];
  if (have)
  {
    snprintf(n, sizeof(n), "%04u", rtc.year);
    lv_textarea_set_text(ui_txtYear, n);
    snprintf(n, sizeof(n), "%02u", rtc.month);
    lv_textarea_set_text(ui_txtMonth, n);
    snprintf(n, sizeof(n), "%02u", rtc.day);
    lv_textarea_set_text(ui_txtDay, n);
    snprintf(n, sizeof(n), "%02u", rtc.hour);
    lv_textarea_set_text(ui_txtTime, n);
    snprintf(n, sizeof(n), "%02u", rtc.minute);
    lv_textarea_set_text(ui_txtMinute, n);
    snprintf(n, sizeof(n), "%02u", rtc.second);
    lv_textarea_set_text(ui_txtSecond, n);
  }
  if (ui_txtScreenSaveTime != NULL)
  {
    uint16_t m = ipAddress_struct.screenSaveMin;
    if (m > 999)
    {
      m = 0;
    }
    snprintf(n, sizeof(n), "%u", m);
    lv_textarea_set_text(ui_txtScreenSaveTime, n);
  }
}

static void styleClockLabels(void)
{
  lv_obj_t *labs[] = {ui_DateLabel, ui_TimeLabel, ui_DateLabel1, ui_TimeLabel1};
  for (unsigned i = 0; i < sizeof(labs) / sizeof(labs[0]); ++i)
  {
    if (labs[i] == NULL)
    {
      continue;
    }
    lv_obj_set_style_text_font(labs[i], &ui_font_arial16, 0);
    lv_obj_set_style_text_color(labs[i], lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_height(labs[i], LV_SIZE_CONTENT);
    lv_obj_set_width(labs[i], LV_SIZE_CONTENT);
    lv_obj_clear_flag(labs[i], LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(labs[i]);
  }
}

static void paintClock(void)
{
  time_t nowSec = time(NULL);
  struct tm tmNow;
  localtime_r(&nowSec, &tmNow);
  char d[16];
  char t[16];
  if (tmNow.tm_year + 1900 < 2020)
  {
    snprintf(d, sizeof(d), "--");
    snprintf(t, sizeof(t), "--");
  }
  else
  {
    snprintf(d, sizeof(d), "%04d-%02d-%02d", tmNow.tm_year + 1900, tmNow.tm_mon + 1, tmNow.tm_mday);
    snprintf(t, sizeof(t), "%02d:%02d:%02d", tmNow.tm_hour, tmNow.tm_min, tmNow.tm_sec);
  }
  if (ui_DateLabel)
    lv_label_set_text(ui_DateLabel, d);
  if (ui_TimeLabel)
    lv_label_set_text(ui_TimeLabel, t);
  if (ui_DateLabel1)
    lv_label_set_text(ui_DateLabel1, d);
  if (ui_TimeLabel1)
    lv_label_set_text(ui_TimeLabel1, t);
}

void setup()
{
  Serial.begin(BAUDRATEDEF);
  Serial.printf("\n[BOOT] VERSION %s env=esp32_samwoo\n", VERSION);
  EEPROM.begin(256);
  pinMode(BUTTON_ERASE, INPUT_PULLUP);
  if (EEPROM.read(0) != 0x57)
  {
    nvsFactoryDefaults();
    EEPROM.writeByte(0, 0x57);
    EEPROM.commit();
    nvsSave();
    Serial.println("Memory Initialized first booting....");
  }
  ipAddress_struct.HighVoltage = 0;
  ipAddress_struct.LowVoltage = 0;
  ipAddress_struct.HighImp = 0;
  ipAddress_struct.HighTemp = 0;
  EEPROM.readBytes(1, (byte *)&ipAddress_struct, sizeof(ipAddress_struct));
  if (ipAddress_struct.screenSaveMin > 999)
  {
    ipAddress_struct.screenSaveMin = 0;
  }
  Serial.printf("\ninit data \n%d %d %d %u", ipAddress_struct.HighVoltage, ipAddress_struct.LowVoltage, ipAddress_struct.HighTemp, ipAddress_struct.HighImp);
  Serial.println("LVGL Benchmark Demo");

  Serial.println("[BOOT] W610 before LCD");
  sEthOk = ethW610Begin(IPAddress(ipAddress_struct.IPADDRESS), IPAddress(ipAddress_struct.GATEWAY),
                        IPAddress(ipAddress_struct.SUBNETMASK), IPAddress(ipAddress_struct.DNS1));
  if (!sEthOk)
  {
    Serial.println("[ETH] no W610 at boot - will retry");
  }
  else
  {
    ethServicesBeginIfNeeded();
  }

  gfx->begin();
  gfx->fillScreen(BLACK);

  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);

  ledcSetup(0, 300, 8);
  ledcAttachPin(TFT_BL, 0);
  ledcWrite(0, BRIGHT);

  if (ipAddress_struct.isUpdate)
  {
    ipAddress_struct.isUpdate = false;
    nvsSave();
    Serial.println("Update....");
    gfx->setTextColor(WHITE);
    gfx->setCursor(0, 10);
    gfx->println("Update....");
    wifiOTAsetup(true);
    Serial.println("[OTA] finished, continue normal boot");
  }
  else
  {
    Serial.println("No Update....");
    gfx->fillScreen(RED);
    delay(500);
    gfx->fillScreen(GREEN);
    delay(500);
    gfx->fillScreen(BLUE);
    delay(500);
    gfx->fillScreen(BLACK);
    delay(500);
  }

  bleSetup();
  lv_init();

  pinMode(TOUCH_GT911_RST, OUTPUT);
  digitalWrite(TOUCH_GT911_RST, LOW);
  delay(10);
  digitalWrite(TOUCH_GT911_RST, HIGH);
  delay(10);
  touch_init();
  Wire.setClock(400000);

  screenWidth = gfx->width();
  screenHeight = gfx->height();

  disp_draw_buf = (lv_color_t *)malloc(sizeof(lv_color_t) * screenWidth * screenHeight / 6);

  if (!disp_draw_buf)
  {
    Serial.println("LVGL disp_draw_buf allocate failed!");
  }
  else
  {
    lv_disp_draw_buf_init(&draw_buf, disp_draw_buf, NULL, screenWidth * screenHeight / 6);

    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = screenWidth;
    disp_drv.ver_res = screenHeight;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = my_touchpad_read;
    sTouchIndev = lv_indev_drv_register(&indev_drv);

    ui_init();
    initSamwooPackUi();
    statusLedsBegin();
    styleClockLabels();

    Serial.println("Setup done");
  }
  init_cursor();
  if (boardRtcSyncEsp())
  {
    Serial.println("[RTC] ESP clock from DS1307");
  }
  else
  {
    Serial.println("[RTC] no time (halted or missing). BLE: time YYYY MM DD HH MM SS");
  }
  EEPROM.readBytes(1, (byte *)&ipAddress_struct, sizeof(ipAddress_struct));
  Serial.println("[BOOT] fill LCD");
  setMemoryDataToLCD();
  paintClock();
  Serial.println("[BOOT] samwoo");
  samwooBegin();
  webFsBegin();
  ethServicesBeginIfNeeded();
  esp_task_wdt_init(WDT_TIMEOUT, true);
  esp_task_wdt_add(NULL);
};
static int interval = 1000;
static unsigned long previousmills = 0;
static int everySecondInterval = 1000;
static int every100ms= 100;
static unsigned long now;
unsigned long incTime=1;


void loop()
{
  now = millis();
  esp_task_wdt_reset();
  serialProtocalparse();
  if (!sEthOk && (now - sEthRetryMs > 5000))
  {
    sEthRetryMs = now;
    Serial.println("[ETH] retry W610");
    sEthOk = ethW610Begin(IPAddress(ipAddress_struct.IPADDRESS), IPAddress(ipAddress_struct.GATEWAY),
                          IPAddress(ipAddress_struct.SUBNETMASK), IPAddress(ipAddress_struct.DNS1));
  }
  ethServicesBeginIfNeeded();
  if (sEthSvc)
  {
    ipFinderPoll();
    const bool ioOk = ethW610IpUsable() && !ipFinderIsHeld();
    if (ioOk)
    {
      snmpBatteryLoop();
      webHttpLoop();
    }
  }
  statusLedsLoop();
  bleCheck();
  if ((now - previousmills > everySecondInterval))
  {
    previousmills = now;
    incTime++;
    lcdOntime++;
    paintClock();
    if (digitalRead(BUTTON_ERASE) == 0)
    {
      sEraseHold++;
      Serial.printf("[IO] reset btn %u\n", (unsigned)sEraseHold);
      if (sEraseHold >= 3)
      {
        factoryResetNow();
      }
    }
    else
    {
      sEraseHold = 0;
    }
  }
  if( incTime % 20 ==0){
    incTime++;
  }
  const uint16_t offSec = screenSaveSec();
  if (offSec == 0)
  {
    if (sLcdDim)
    {
      ledcWrite(0, BRIGHT);
      sLcdDim = false;
    }
  }
  else if (lcdOntime >= offSec)
  {
    if (!sLcdDim)
    {
      ledcWrite(0, 0);
      lv_obj_add_flag(cursor_obj, LV_OBJ_FLAG_HIDDEN);
      sLcdDim = true;
    }
  }
  lv_timer_handler();
  vTaskDelay(15);
}
