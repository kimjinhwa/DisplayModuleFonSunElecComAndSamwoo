# 삼우 BMS 웹 / SNMP 도구

현장 웹·업데이트 설명은 `doc/사용설명서/manual/사용설명서.html` 제3·4부.

## IP Finder

`Esp32SNMPforSUN` 과 같은 Finder 입니다. WEB ENABLE / WEB PORT / TRAP 필드가 있습니다.
이 장비는 트랩을 보내지 않습니다. 웹 on/off 와 포트는 EEPROM `WEBSERVERPORT` 입니다 (0=꺼짐).

실행 파일 (설치 없이 더블 클릭):

`python/release/IPFinder.exe`

다시 만들 때:

```
cd python
powershell -ExecutionPolicy Bypass -File .\makeExe.ps1
```

소스에서 바로 실행:

```
python python/SnmpFinder.py
```

## 웹 파일 업로드

```
python python/upload_web.py --host 192.168.0.65 --port 81
```

기본 웹 포트는 설정값(공장 81)입니다. Finder에서 80으로 바꿀 수 있습니다.

올리는 파일은 `UploadFiles/` 만 씁니다. `fileUpload.html` / jQuery / svg 는 넣지 않습니다.
`/fileUpload` 페이지는 펌웨어에 들어 있습니다.

## 스모크

```
python python/check_bms_web.py --host 192.168.0.65 --port 81 --upload
```

- 대소문자 URL (`/Login.html`, `/INDEX.HTML`)
- 로그인 후 `/api/bms`
- SNMP GET pack1 전압
- 웹·SNMP·RS-485 가 같은 loop 에서만 도는 것을 안내

## 1시간 소크

```
python -u python/soak_bms.py --host 192.168.0.65 --port 81 --hours 1
```

로그: `python/soak_logs/`
