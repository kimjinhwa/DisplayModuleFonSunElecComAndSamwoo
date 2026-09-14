# 내일 실팩 RS-485 점검. 저장소 루트 또는 이 파일 위치에서 실행.
#   powershell -ExecutionPolicy Bypass -File python\run_samwoo_lab.ps1
#   powershell -ExecutionPolicy Bypass -File python\run_samwoo_lab.ps1 -Port COM14
#   powershell -ExecutionPolicy Bypass -File python\run_samwoo_lab.ps1 -Port COM14 -Baud 19200
param(
    [string]$Port = "",
    [int]$Baud = 0
)

$ErrorActionPreference = "Stop"
$here = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location -LiteralPath (Split-Path -Parent $here)

Write-Host "디스플레이는 버스에서 빼세요. USB-RS485만 팩에 연결합니다."
Write-Host ""

$argsList = @("python\samwoo_lab.py")
if ($Port) { $argsList += @("--port", $Port) }
if ($Baud -gt 0) { $argsList += @("--baud", "$Baud") }
python @argsList
exit $LASTEXITCODE
