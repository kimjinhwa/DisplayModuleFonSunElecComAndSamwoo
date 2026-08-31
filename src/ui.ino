#include <Arduino.h>
#include <lvgl.h>
#include <Arduino_GFX_Library.h>
#include <TFT_eSPI.h>
#include "src/ui.h"
#include <EEPROM.h>
#include "SerialProtocalParse.h"
#include "main.h"
#include "src/ui.h"
#include "wifiOTA.h"
#include <esp_task_wdt.h>
#include "naradav13.h"
#include <esp_wifi.h>
//#include "lv_i18n/lv_i18n.h" 
#include <Wire.h>
#define GFX_BL DF_GFX_BL // default backlight pin, you may replace DF_GFX_BL to actual backlight pin
#define TFT_BL 2
#define BRIGHT  155 
#define WDT_TIMEOUT 5 

static uint32_t screenWidth;
static uint32_t screenHeight;

static lv_disp_draw_buf_t draw_buf;
static lv_color_t *disp_draw_buf;
static lv_disp_drv_t disp_drv;
static unsigned long last_ms;
//static lv_obj_t *led;
#define LED_OFF_TIME 600
uint16_t lcdOntime=0;

extern NaradaClient232 naradaClient;

Arduino_ESP32RGBPanel *bus = new Arduino_ESP32RGBPanel(
    GFX_NOT_DEFINED /* CS */, GFX_NOT_DEFINED /* SCK */, GFX_NOT_DEFINED /* SDA */,
    41 /* DE */, 40 /* VSYNC */, 39 /* HSYNC */, 42 /* PCLK */,
    14 /* R0 */, 21 /* R1 */, 47 /* R2 */, 48 /* R3 */, 45 /* R4 */,
    9 /* G0 */, 46 /* G1 */, 3 /* G2 */, 8 /* G3 */, 16 /* G4 */, 1 /* G5 */,
    15 /* B0 */, 7 /* B1 */, 6 /* B2 */, 5 /* B3 */, 4 /* B4 */
);
// option 1:
// 7寸 50PIN 800*480
Arduino_RPi_DPI_RGBPanel *gfx = new Arduino_RPi_DPI_RGBPanel(
  bus,
//  800 /* width */, 0 /* hsync_polarity */, 8/* hsync_front_porch */, 2 /* hsync_pulse_width */, 43/* hsync_back_porch */,
//  480 /* height */, 0 /* vsync_polarity */, 8 /* vsync_front_porch */, 2/* vsync_pulse_width */, 12 /* vsync_back_porch */,
//  1 /* pclk_active_neg */, 16000000 /* prefer_speed */, true /* auto_flush */);

    800 /* width */, 0 /* hsync_polarity */, 210 /* hsync_front_porch */, 30 /* hsync_pulse_width */, 16 /* hsync_back_porch */,
    480 /* height */, 0 /* vsync_polarity */, 22 /* vsync_front_porch */, 13 /* vsync_pulse_width */, 10 /* vsync_back_porch */,
    1 /* pclk_active_neg */, 12000000 /* prefer_speed */, true /* auto_flush */);
  
#include "touch.h"
#if LV_USE_LOG != 0
/* Serial debugging */
void my_print(const char * buf)
{
    Serial.printf(buf);
    Serial.flush();
}
#endif

/* Display flushing */
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
   uint32_t w = (area->x2 - area->x1 + 1);
   uint32_t h = (area->y2 - area->y1 + 1);
   gfx->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w, h);
   lv_disp_flush_ready(disp);
}
static lv_obj_t *cursor_obj;
void init_cursor() {
    // 커서 객체 생성
    cursor_obj = lv_obj_create(lv_scr_act());
    
    // 커서 스타일 설정
    lv_obj_set_size(cursor_obj, 20, 20);  // 크기 설정
    lv_obj_set_style_radius(cursor_obj, LV_RADIUS_CIRCLE, 0);  // 원형으로 설정
    lv_obj_set_style_bg_color(cursor_obj, lv_color_hex(0xFF0000), 0);  // 빨간색
    lv_obj_set_style_bg_opa(cursor_obj, LV_OPA_50, 0);  // 반투명
    
    // 초기에는 숨김
    lv_obj_add_flag(cursor_obj, LV_OBJ_FLAG_HIDDEN);
}
static unsigned long last_touch_time = 0;  // 마지막 터치 시간을 저장할 변수
#define TOUCH_TIMEOUT (3 * 60 * 1000)    // 10분을 밀리초로 변환
void my_touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data)
{
  unsigned long current_time = millis();
  // 10분 이상 터치가 없었는지 확인
  if ((current_time - last_touch_time) > TOUCH_TIMEOUT)
  {
    last_touch_time = current_time; // 타이머 리셋
    lv_obj_clear_flag(cursor_obj, LV_OBJ_FLAG_HIDDEN);
    touch_init();
    Serial.println("\ntouch_init ok ");
  }

  if (touch_has_signal())
  {
    last_touch_time = current_time; // 타이머 리셋
    if (touch_touched())
    {
      data->state = LV_INDEV_STATE_PR;

      /*Set the coordinates*/
      data->point.x = touch_last_x;
      data->point.y = touch_last_y;
      lv_obj_clear_flag(cursor_obj, LV_OBJ_FLAG_HIDDEN);
      lv_obj_set_pos(cursor_obj, 
                    touch_last_x - 10,  // 커서 중심이 터치 포인트에 오도록
                    touch_last_y - 10); // 크기의 절
      ledcWrite(0,BRIGHT);
      lcdOntime=0;
      // Serial.print( "Data xx " );
      // Serial.println( data->point.x );
      // Serial.print( "Data yy " );
      // Serial.println( data->point.y );
    }
    else if (touch_released())
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

  // String HeaderText = ipAddress_struct.deviceName;
  // HeaderText ;
  lv_label_set_text(ui_HeaderTitle,ipAddress_struct.deviceName);
  lv_textarea_set_text(ui_txtDEVICENAME,ipAddress_struct.deviceName);
  lv_textarea_set_text(ui_txtYear,"");
  lv_textarea_set_text(ui_txtMonth,"");
  lv_textarea_set_text(ui_txtDay,"");
  lv_textarea_set_text(ui_txtTime,"");
  lv_textarea_set_text(ui_txtMinute,"");
  lv_textarea_set_text(ui_txtMinute,"");
  lv_textarea_set_text(ui_txtSecond,"");

  lv_label_set_text(ui_DateLabel1,"");
  lv_label_set_text(ui_TimeLabel1,"");
}
void displayToLcd(int packNumber,bool isSucess);
extern uint isModuleExgist[8];

void setup()
{
  // WiFi 끄기
  esp_wifi_deinit();
  // 또는
  esp_wifi_stop();

  // WiFi 모듈의 전원 자체를 끄기
  esp_wifi_set_mode(WIFI_MODE_NULL);

  // 추가로 전력 소비를 더 줄이려면
  btStop(); // Bluetooth도 끄기`
  Serial.begin(BAUDRATEDEF);
  Serial1.begin(BAUDRATEDEF, 134217756U, 18, 17);
  EEPROM.begin(120);
  Wire.setClock(400000);
  if (EEPROM.read(0) != 0x55)
  {
    ipAddress_struct.IPADDRESS = (uint32_t)IPAddress(192, 168, 0, 57);
    ipAddress_struct.GATEWAY = (uint32_t)IPAddress(192, 168, 0, 1);
    ipAddress_struct.SUBNETMASK = (uint32_t)IPAddress(255, 255, 255, 0);
    ipAddress_struct.WEBSOCKETSERVER = (uint32_t)IPAddress(192, 168, 0, 57);
    ipAddress_struct.DNS1 = (uint32_t)IPAddress(8, 8, 8, 8);
    ipAddress_struct.DNS2 = (uint32_t)IPAddress(164, 124, 101, 2);
    ipAddress_struct.WEBSERVERPORT = 81;
    ipAddress_struct.NTP_1 = (uint32_t)IPAddress(203, 248, 240, 140); //(203, 248, 240, 140);
    ipAddress_struct.NTP_2 = (uint32_t)IPAddress(13, 209, 84, 50);
    ipAddress_struct.ntpuse = false;

    ipAddress_struct.HighVoltage = 36500;
    ipAddress_struct.LowVoltage = 26000;
    ipAddress_struct.HighImp = 80000;
    ipAddress_struct.HighTemp = 70;
    ipAddress_struct.alarmSetStatus = 0;
    strncpy(ipAddress_struct.deviceName, "BAT RACK1", 9);

    EEPROM.writeByte(0, 0x55);
    EEPROM.commit();
    EEPROM.writeBytes(1, (const byte *)&ipAddress_struct, sizeof(nvsSystemSet));
    EEPROM.commit();
    Serial.println("Memory Initialized first booting....");
  }
  ipAddress_struct.HighVoltage = 0;
  ipAddress_struct.LowVoltage = 0;
  ipAddress_struct.HighImp = 0;
  ipAddress_struct.HighTemp = 0;
  EEPROM.readBytes(1, (byte *)&ipAddress_struct, sizeof(ipAddress_struct));
  Serial.printf("\ninit data \n%d %d %d %d", ipAddress_struct.HighVoltage, ipAddress_struct.LowVoltage, ipAddress_struct.HighTemp, ipAddress_struct.HighImp);
  // while (!Serial);
  Serial.println("LVGL Benchmark Demo");

  // Init Display
  // Add
  gfx->begin();
  gfx->fillScreen(BLACK);

  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);

  ledcSetup(0, 300, 8);
  ledcAttachPin(TFT_BL, 0);
  ledcWrite(0, BRIGHT); /* Screen brightness can be modified by adjusting this parameter. (0-255) */

  gfx->fillScreen(RED);
  delay(500);
  gfx->fillScreen(GREEN);
  delay(500);
  gfx->fillScreen(BLUE);
  delay(500);
  gfx->fillScreen(BLACK);
  delay(500);
  lv_init();

  // led = lv_led_create(lv_scr_act());

  // Init touch device
  pinMode(TOUCH_GT911_RST, OUTPUT);
  digitalWrite(TOUCH_GT911_RST, LOW);
  delay(10);
  digitalWrite(TOUCH_GT911_RST, HIGH);
  delay(10);
  touch_init();

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

    /* Initialize the display */
    lv_disp_drv_init(&disp_drv);
    /*Change the following line to your display resolution*/
    disp_drv.hor_res = screenWidth;
    disp_drv.ver_res = screenHeight;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    /*Initialize the (dummy) input device driver*/
    /* Initialize the (dummy) input device driver */
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = my_touchpad_read;
    lv_indev_drv_register(&indev_drv);

    ui_init();

    lv_label_set_text(ui_DateLabel, "");
    lv_label_set_text(ui_DateLabel1, "");
    lv_label_set_text(ui_TimeLabel, "");
    lv_label_set_text(ui_TimeLabel1, "");

    Serial.println("Setup done");
  }
  init_cursor();
  struct tm tm;
  tm.tm_year = 2023 - 1900;
  tm.tm_mon = 11;
  tm.tm_mday = 13;
  tm.tm_hour = 15;
  tm.tm_min = 13;
  tm.tm_sec = 00;
  struct timeval tv;
  tv.tv_sec = mktime(&tm);
  tv.tv_usec = 0;
  settimeofday(&tv, NULL);
#ifdef USEWIFI
  wifiOTAsetup();
#endif
  pinMode(13, OUTPUT);
  pinMode(12, OUTPUT);
  EEPROM.readBytes(1, (byte *)&ipAddress_struct, sizeof(ipAddress_struct));
  setMemoryDataToLCD();
  esp_task_wdt_init(WDT_TIMEOUT, true);
  esp_task_wdt_add(NULL);
  naradaClient.initBatInfo();
  // for(int i=0;i<8;i++)displayToLcd(i,true);
};
static int interval = 1000;
static unsigned long previousmills = 0;
static int everySecondInterval = 1000;
static int every100ms= 100;
static unsigned long now;
unsigned long incTime=1;

void loop()
{
  void *parameters;
  //wifiOtaloop();
  now = millis();
  esp_task_wdt_reset();
  serialProtocalparse();
  if ((now - previousmills > everySecondInterval))
  {
    previousmills = now;
    incTime++;
    lcdOntime++;
  }
  if( incTime % 20 ==0){
    //매 20초마다 검사한다.
    incTime++;
  }
  //if ((incTime % 10) == 0) // 100*10 = 1S
  if(lcdOntime >= LED_OFF_TIME) //lv_led_off(led);
  {
      //ledcWrite(0,0);
      lv_obj_add_flag(cursor_obj, LV_OBJ_FLAG_HIDDEN);
  }
  lv_timer_handler(); /* let the GUI do its work */
  vTaskDelay(50);
}