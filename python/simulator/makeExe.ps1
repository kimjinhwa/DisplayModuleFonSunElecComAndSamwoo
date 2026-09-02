# rtuslave_samwoo.py → 콘솔 없는 GUI exe
# 사용: 이 폴더에서
#   pip install -r requirements.txt pyinstaller
#   powershell -ExecutionPolicy Bypass -File .\makeExe.ps1
#
# 산출: .\dist\rtuslave_samwoo.exe

$ErrorActionPreference = 'Stop'
Set-Location -LiteralPath $PSScriptRoot

py -m pip install -q -r requirements.txt pyinstaller

py -m PyInstaller `
  --onefile `
  --noconsole `
  --name rtuslave_samwoo `
  --distpath .\dist `
  --workpath .\build `
  --specpath .\build `
  --hidden-import serial `
  --hidden-import serial.tools.list_ports `
  --hidden-import modbus_tk `
  --hidden-import modbus_tk.modbus_rtu `
  --hidden-import modbus_tk.defines `
  .\rtuslave_samwoo.py

Write-Host ("완료 : {0}" -f (Join-Path (Get-Location) 'dist\rtuslave_samwoo.exe'))
