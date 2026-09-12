# 삼우 리튬 배터리 디스플레이 (SunElec / Samwoo)

7인치 ESP32-S3 터치 디스플레이입니다. RS-485로 삼우 리튬 팩 2대를 읽고, 화면과 SNMPv2c로 값을 보여 줍니다.  
펌웨어 현장 업데이트는 **블루투스 CLI → 재부팅 → Wi‑Fi OTA** 입니다. 유선 이더넷은 SNMP용이며 OTA에는 쓰지 않습니다.

현재 펌웨어 버전은 `Version.h`를 보면 됩니다. `esp32_samwoo` 빌드마다 patch가 1 올라갑니다.

OTA 파이프라인은 `Display4_3_INVT_MegaTec`과 같고, 스킬 `esp32-wifi-ota-pipeline`을 따릅니다.

---

## 1. 목적

- 팩 1·2의 전압·전류·셀·온도를 7인치 화면에 표시한다.
- 이더넷이 있으면 SNMPv2c(UDP 161, community `public`)로 같은 값을 제공한다.
- 같은 랜에서 `SnmpFinder` / IPFinder가 UDP **1234**로 검색하면 응답하고, IP/서브넷/게이트웨이를 바꿀 수 있다.
- BLE(`IFTECH_SW_<MAC>`)로 SSID를 넣고, 재부팅 후 Wi‑Fi로 펌웨어를 받는다.

---

## 2. 구조

```
src/                 펌웨어 (ui.ino, 삼우 폴링, SNMP, BLE, OTA)
lib/                 LVGL, GFX, Arduino_SNMP, Ethernet_Generic …
mib/                 SAMWOO_BAT_MIB_VER_01.MIB
manual/              사용자 매뉴얼
upgradeDoc/          현장 펌웨어 업그레이드 절차
python/simulator/    삼우 RS-485 시뮬레이터
IFTECH_SAMWOO_App/   BLE + OTA용 Flutter 앱 (MegaTec 앱 파생)
Version.h            활성 VERSION + 주석 이력
pre_build.py         빌드 시 patch +1
post_build.py        firmware.bin / esp32_samwoo.json → IIS + uploadFirmware/
```

| 경로 | 역할 |
|------|------|
| `src/ui.ino` | 부팅. `isUpdate`면 BLE보다 먼저 Wi‑Fi OTA |
| `src/samwoo_poll.cpp` | 슬레이브 1·2 RS-485 폴링 |
| `src/snmp_battery.cpp` | SNMPv2c |
| `src/eth_w610.cpp` | W6100 유선 이더넷 (SNMP/IPFinder). OTA와 무관 |
| `src/ip_finder.cpp` | IPFinder UDP 1234 (`SnmpFinder.py`와 동일 JSON) |
| `src/wifiOTA.cpp` | STA 연결 + SelfUploader |
| `src/esp32SelfUploader.cpp` | JSON 조회, 버전 비교, HTTP OTA |
| `src/myBlueTooth.cpp` | BLE Nordic UART |
| `src/cli_commands.cpp` | `ssid` / `pass` / `update` / `version` |
| `platformio.ini` | env `esp32_samwoo` |

---

## 3. 동작 방식

1. ESP32-S3가 RGB 패널을 직접 구동하고 LVGL로 그린다.
2. Serial1 RS-485로 삼우 팩 주소 1·2를 폴링한다 (9600 8N1).
3. 유선 랜이 있으면 W6100으로 SNMP(UDP 161)와 IPFinder(UDP 1234)를 연다.
4. BLE로 `update`를 받으면 `isUpdate`만 저장하고 재부팅한다. **BLE와 Wi‑Fi를 동시에 켜지 않는다.**
5. 재부팅 직후 Wi‑Fi로 `{FW_UPDATE_BASE}/{FW_UPDATE_META}` 를 보고, `latest`가 장치 `VERSION`보다 크면 펌웨어를 받아 플래시한다. 실패·이미 최신이면 메시지를 찍고 정상 가동한다.

빌드 env:

| env | 용도 |
|-----|------|
| `esp32_samwoo` | 제품 펌웨어 (기본) |
| `hwtest_rtc` / `hwtest_rs485` / `hwtest_w610` | 칩 단독 확인 |

OTA 메타 파일: **`esp32_samwoo.json`**  
공개 URL: `http://ift.iptime.org:81/Esp32UploadFirmware/`

---

## 4. 빌드 · 업로드

```
pio run -e esp32_samwoo
pio run -e esp32_samwoo -t upload
```

업로드 포트는 `COM3`. 매 빌드마다 `Version.h` patch가 올라가므로 불필요한 빌드는 하지 않는다.

산출물 (IIS 및 `uploadFirmware/`):

- `firmware_<version>_esp32_samwoo.bin`
- `firmware.bin`
- `esp32_samwoo.json` — `{ "latest": "x.y.z", "filename": "firmware_x.y.z_esp32_samwoo.bin" }`

파티션: `default_8MB.csv` (`app0`/`app1` OTA 슬롯).

---

## 5. CLI (USB Serial 또는 BLE)

| 명령 | 동작 |
|------|------|
| `version` | `VERSION : x.y.z` |
| `ssid` | 저장된 SSID/PASS 표시 |
| `ssid 이름` | 핫스팟/AP SSID 저장 |
| `pass 암호` | 비밀번호 저장 (`none`이면 개방) |
| `ip` | 저장 IP/SN/GW + 현재 이더넷 |
| `ip a.b.c.d [sn gw]` | 이더넷 주소 저장 (reboot 후 적용) |
| `time` | DS1307 RTC 시각 |
| `time Y M D h m s` | RTC 설정 |
| `name` / `name 텍스트` | 화면 장치 이름 |
| `status` | 팩1·2 통신/SOC/전압/전류 |
| `mac` | Wi‑Fi MAC |
| `update` | `isUpdate=true` 후 재부팅 → Wi‑Fi OTA |
| `reboot` | 재부팅 |
| `help` | 명령 목록 |

현장 절차는 `upgradeDoc/upgradeManual.md`.  
폰 앱은 Play 스토어 Serial Bluetooth 또는 `IFTECH_SAMWOO_App`. 재접속 후 `version`으로 업데이트 성공 여부를 본다.

---

## 6. SNMP (유선 이더넷이 있을 때)

기본 IP `192.168.0.57`, community `public`, UDP 161.  
팩1 `1.3.6.1.2.1.32.1` / 팩2 `1.3.6.1.2.1.32.2`. MIB: `mib/SAMWOO_BAT_MIB_VER_01.MIB`.

```
[12:49:40.131]  TX   [3A 01 04 60 00 0A 03 E8 00 55 00 62 02 1C 00 19 0D 48 0C E4 00 FA 00 F0 00 00 00 00 00 07 00 00 00 00 00 00 00 10 0D 16 0D 17 0D 18 0D 19 0D 1A 0D 1B 0D 1C 0D 1D 0D 1E 0D 1F 0D 20 0D 21 0D 22 0D 23 00 08 00 FA 00 FA 00 FA 00 FA 00 FA 00 FA 00 FA 00 FA 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 55 0D 0A]  slave 1 FC04 start=0 qty=48
[12:49:40.262]  RX   [3A 02 04 00 00 00 30 CA 0D 0A]  slave 2 FC04 start=0 qty=48
[12:49:40.262]  TX   [3A 02 04 60 00 0B 03 E8 00 46 00 5F 02 10 00 12 0D 34 0C D0 00 F5 00 EB 00 00 00 00 00 07 00 00 00 00 00 00 00 10 0C F8 0C F9 0C FA 0C FB 0C FC 0C FD 0C FE 0C FF 0D 00 0D 01 0D 02 0D 03 0D 04 0D 05 00 08 00 F5 00 F5 00 F5 00 F5 00 F5 00 F5 00 F5 00 F5 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 7E 0D 0A]  slave 2 FC04 start=0 qty=48
[12:49:41.392]  RX   [3A 01 04 00 00 00 30 CB 0D 0A]  slave 1 FC04 start=0 qty=48
```