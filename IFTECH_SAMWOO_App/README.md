# IFTECH_SAMWOO_App

삼우 리튬 디스플레이 BLE·Wi‑Fi OTA용 Flutter 앱입니다.  
`Display4_3_INVT_MegaTec/IFTECH_UPS_App` 을 이 제품용으로 복사했습니다.

장비 BLE 이름: `IFTECH_SW_<MAC>`  
검색 prefix는 `IFTECH_` 그대로입니다.

```bash
cd IFTECH_SAMWOO_App
flutter pub get
flutter run
```

연결 후 `version` 으로 현재 펌웨어를 확인하고, `ssid` / `pass` / `update` 로 OTA를 시작합니다. 재접속 후 다시 `version` 을 비교하면 업데이트 여부를 알 수 있습니다.
