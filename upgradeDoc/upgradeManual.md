# 펌웨어 업그레이드 방법

자세한 화면 설명은 [사용설명서.html](../doc/사용설명서/manual/사용설명서.html) 제4부.

1. 펌웨어는 **블루투스 + Wi‑Fi** 로 올립니다. 유선 이더넷으로는 올리지 않습니다.
2. 앱 APK `IFTECH_SAMWOO_App/release/IFTECH_SAMWOO_App.apk` 를 설치하거나, Play 스토어 **Serial Bluetooth** 를 씁니다.
3. 앱을 열고 **연결** → Bluetooth LE SCAN. `IFTECH_SW_xx:xx:xx:xx:xx:xx` 가 이 디스플레이입니다. (`IFTECH_` 로 시작하면 검색됩니다.)
4. 휴대폰 핫스팟(WPA2) 또는 현장 공유기를 켭니다.
5. 앱 **Wi-Fi 설정**에서 AP를 고르고 암호를 넣은 뒤 **SSID / PASS 장비로 전송**. Serial Bluetooth면 아래를 직접 보냅니다.

```
ssid 핫스팟이름
pass 핫스팟암호
```

6. 앱에서 **version** 으로 지금 버전을 확인한 뒤 **update** 를 누릅니다. 재부팅되며 Wi‑Fi로 서버에서 펌웨어를 받습니다.
7. 성공하면 새 버전으로 다시 뜹니다. 실패하거나 이미 최신이면 메시지를 찍고 배터리 감시 화면으로 들어갑니다.
8. 폰을 다시 연결한 뒤 **version**. 나온 값이 올린 펌웨어와 같으면 완료입니다.

서버: `http://ift.iptime.org:81/Esp32UploadFirmware/esp32_samwoo.json`

웹 HTML만 바꿀 때는 브라우저 `http://장비IP:81/fileUpload` (포트는 설정값). 펌웨어 `.bin` 은 이 경로를 쓰지 마십시오.
