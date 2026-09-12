# IP Finder 실행 파일
# 필요: pip install pyinstaller
#
# 사용:
#   cd python
#   powershell -ExecutionPolicy Bypass -File .\makeExe.ps1
#
$ErrorActionPreference = 'Stop'
$here = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location -LiteralPath $here

$src = Join-Path $here 'SnmpFinder.py'
if (-not (Test-Path -LiteralPath $src)) { throw "SnmpFinder.py 가 없습니다 : $src" }

Write-Host 'PyInstaller 로 IPFinder.exe 를 만듭니다...'
py -m PyInstaller --noconfirm --clean --onefile --noconsole --name IPFinder $src
if ($LASTEXITCODE -ne 0) { throw "PyInstaller 실패 (exit $LASTEXITCODE)" }

$built = Join-Path $here 'dist\IPFinder.exe'
if (-not (Test-Path -LiteralPath $built)) { throw "산출물이 없습니다 : $built" }

$relDir = Join-Path $here 'release'
New-Item -ItemType Directory -Force -Path $relDir | Out-Null
$dest = Join-Path $relDir 'IPFinder.exe'
Copy-Item -Force -LiteralPath $built -Destination $dest

$item = Get-Item -LiteralPath $dest
Write-Host ('완료 : {0}  ({1:N0} bytes)' -f $dest, $item.Length)
