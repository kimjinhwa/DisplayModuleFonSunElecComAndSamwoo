# 삼우 BMS 디스플레이 — 사용 설명서

PC용 **IP Finder**, 장비 **웹 화면**, **7인치**, **앱 펌웨어 업데이트**, **SNMP OID**를 한곳에 정리합니다.

## 본문 (HTML + 인쇄 + 스크립트)

폴더: [manual/](manual/)

- [manual/사용설명서.html](manual/사용설명서.html) — A4 미리보기, Chrome PDF 인쇄
- [manual/make_pdf.ps1](manual/make_pdf.ps1) — 헤드리스 PDF
- `manual/img/` — 캡처·도면
- 장비 `UploadFiles/help.html` — 그림 없는 현장 요약

```
cd doc\사용설명서\manual
powershell -ExecutionPolicy Bypass -File .\make_pdf.ps1
```

웹 캡처(장비 필요):

```
python capture_shots.py
powershell -ExecutionPolicy Bypass -File .\capture_finder.ps1
```

환경 변수 `SAMWOO_WEB_HOST` / `SAMWOO_WEB_PORT` (기본 192.168.0.65 / 81).
