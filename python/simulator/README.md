# 삼우 BMS 시뮬레이터

실팩이 없을 때 디스플레이를 붙이는 **슬레이브**입니다.  
실팩을 PC에 연결해 읽을 때는 시뮬레이터가 아니라 `python/samwoo_lab.py` 를 씁니다.

프레이밍: STX `0x3A` + 이진 PDU + LRC + CR LF (`doc/삼우에스비_통신_Protocol_20260816.xls`).

```powershell
pip install -r requirements.txt
python rtuslave_samwoo.py
```

exe:

```powershell
powershell -ExecutionPolicy Bypass -File .\makeExe.ps1
```

산출: `dist\rtuslave_samwoo.exe`

- 기본 모드 **삼우 STX+LRC**, 19200 8N1 (업체 확인), slave 1 / 2
- **응답 length**: 레지스터수 `0x30`(실팩). 구 펌웨어용 바이트수 `0x60` 은 옵션.
- 요청 start는 **1만** 사용. 맵은 통신_RX (Relay@10, 셀@15).
- Debug 체크 시 Rx/Tx 헥스
- 표준 Modbus RTU 는 비교용으로만 둔다.

LRC는 `samwoo_proto.py` 와 같다. 엑셀·펌웨어와 어긋난 부분은 랩 리포트를 보고 고친다.
