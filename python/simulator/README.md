# BMS Modbus 시뮬레이터

표준 Modbus RTU와 삼우(STX `0x3A` + binary + LRC + CR LF)를 고를 수 있습니다.
한 포트에 **Slave 1(왼쪽 팩)** 과 **Slave 2(오른쪽 팩)** 을 동시에 올립니다.

```bash
pip install -r requirements.txt
python rtuslave_samwoo.py
```

GUI exe (콘솔 창 없음):

```powershell
powershell -ExecutionPolicy Bypass -File .\makeExe.ps1
```

산출물은 `dist\rtuslave_samwoo.exe` 입니다.

- Port / Baud(기본 COM4, 9600 8N1) → 모드 선택 → **Open**
- **Debug** 체크 시 ModPoll처럼 Rx/Tx 헥사 팝업
- 레지스터 값을 바꾼 뒤 **Set**
- FC03(Holding) / FC04(Input) 모두 같은 맵(주소 0~47)

삼우 LRC는 본문 합의 2의 보수(`(~sum)+1`)입니다. 펌웨어 `oid_map.json` 의 `modbus.framing: samwoo` 와 같습니다.
