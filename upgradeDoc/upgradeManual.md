# 펌웨어 업그레이드 방법

1. firmware 업그레이드는 블루투스와 와이파이를 사용하여 진행합니다. 유선 이더넷은 사용하지 않습니다.
2. 핸드폰 Play 스토어에서 **Serial Bluetooth** 를 검색해 설치하거나, `IFTECH_SAMWOO_App` 을 사용합니다.
3. 앱을 열고 장치 검색을 합니다. **Bluetooth LE** 로 SCAN 합니다.
4. `IFTECH_SW_xx:xx:xx:xx:xx:xx` 가 보이면 이 디스플레이입니다. (이름 앞이 `IFTECH_` 이면 기존 IFTECH UPS 앱으로도 검색됩니다.)
5. `IFTECH_SAMWOO_App` 이면 제어 화면 **Wi-Fi 설정**에서 **주변 Wi-Fi 검색해서 선택** 으로 AP를 고르고 암호를 넣은 뒤 전송합니다. Serial Bluetooth면 `ssid` / `pass` 를 직접 보냅니다.
6. 핸드폰 모바일 핫스팟을 켜고, 보안은 WPA2 로 맞춥니다. 또는 현장 공유기를 씁니다.
7. 핫스팟/공유기 이름과 암호를 맞춥니다.

```
ssid 핫스팟이름
pass 핫스팟암호
```

8. `update` 를 보냅니다. 재부팅 메시지가 나오고 화면이 리셋되며 Wi‑Fi로 서버에서 펌웨어를 받습니다.
9. 성공하면 새 버전으로 다시 뜹니다. 실패하거나 이미 최신이면 화면에 메시지를 찍고 배터리 감시 화면으로 들어갑니다.
10. 폰을 다시 연결한 뒤 `version` 을 보냅니다. 나온 값이 올린 펌웨어와 같으면 업데이트가 된 것입니다.

서버 주소: `http://ift.iptime.org:81/Esp32UploadFirmware/esp32_samwoo.json`
