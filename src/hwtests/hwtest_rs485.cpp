// THVD1406DR RS485 H/W 테스트.
//   pio run -e hwtest_rs485 -t upload
// USB Serial (PuTTY COM3, 115200): 로그. 입력은 485로 전달.
// Serial1 9600 8N1 TX=17 RX=18:
//   5초마다 "RS485 tick N" 송신
//   485로 수신되면 "ACK: ..." 응답 + USB에 출력
// USB-RS485 어댑터는 9600 8N1로 같은 버스에 붙이면 된다.

#include <Arduino.h>
#include "board_pins.h"

#ifndef BAUDRATEDEF
#define BAUDRATEDEF 115200
#endif

static uint32_t tickCount = 0;

void setup()
{
  Serial.begin(BAUDRATEDEF);
  Serial1.begin(RS485_UART_BAUD, SERIAL_8N1, PIN_RS485_RX, PIN_RS485_TX);
  delay(300);
  Serial.println();
  Serial.println("=== hwtest_rs485 THVD1406 ===");
  Serial.printf("USB 115200  |  RS485 %u 8N1 TX=%d RX=%d (auto DE)\n",
                (unsigned)RS485_UART_BAUD, PIN_RS485_TX, PIN_RS485_RX);
  Serial.println("USB 입력 → 485 송신. 485 수신 → ACK 회신.");
}

static void drainRs485ToAck()
{
  if (!Serial1.available())
  {
    return;
  }
  char buf[128];
  size_t n = 0;
  const uint32_t t0 = millis();
  while ((millis() - t0) < 50 && n < sizeof(buf) - 1)
  {
    if (Serial1.available())
    {
      buf[n++] = (char)Serial1.read();
    }
  }
  buf[n] = 0;
  Serial.print("[485 RX] ");
  Serial.write((const uint8_t *)buf, n);
  if (n == 0 || buf[n - 1] != '\n')
  {
    Serial.println();
  }
  Serial1.print("ACK: ");
  Serial1.write((const uint8_t *)buf, n);
  if (n == 0 || buf[n - 1] != '\n')
  {
    Serial1.print("\r\n");
  }
}

void loop()
{
  static uint32_t lastMs = 0;

  while (Serial.available())
  {
    const int c = Serial.read();
    Serial1.write((uint8_t)c);
  }

  drainRs485ToAck();

  if (millis() - lastMs >= 5000)
  {
    lastMs = millis();
    tickCount++;
    Serial1.printf("RS485 tick %lu\r\n", (unsigned long)tickCount);
    Serial.printf("[485 TX] RS485 tick %lu\n", (unsigned long)tickCount);
  }
}
