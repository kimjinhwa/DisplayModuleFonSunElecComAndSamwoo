# IFTECH_SAMWOO_App

삼우 리튬 디스플레이 BLE·Wi‑Fi OTA용 Flutter 앱입니다.  
앱 이름 **IFTECH SAMWOO**, 패키지 `com.iftech.iftech_samwoo_app`.

장비 BLE 이름: `IFTECH_SW_<MAC>`  
검색 prefix는 `IFTECH_` 입니다.

## 배포 APK

이미 빌드된 파일:

`IFTECH_SAMWOO_App/release/IFTECH_SAMWOO_App.apk`

휴대폰에 복사해 설치합니다. Android에서 알 수 없는 출처 설치를 허용해야 합니다.

다시 빌드:

```
cd IFTECH_SAMWOO_App
.\build_release.ps1
```

또는 `flutter pub get` 후 `flutter build apk --release`.  
산출물은 `build/app/outputs/flutter-apk/app-release.apk` 이며 스크립트가 `release/` 로 복사합니다.

개발 실행:

```
flutter pub get
flutter run
```

## 현장 사용

1. **연결** → `IFTECH_SW_` 장비 선택
2. **version** 으로 현재 펌웨어 확인
3. **Wi-Fi 설정**에서 AP 선택·암호 입력 → **SSID / PASS 장비로 전송**
4. **update** → 장비가 재부팅되며 Wi‑Fi OTA
5. 다시 연결한 뒤 **version** 비교

사용자 매뉴얼 제4부: `../doc/사용설명서/manual/사용설명서.html`
