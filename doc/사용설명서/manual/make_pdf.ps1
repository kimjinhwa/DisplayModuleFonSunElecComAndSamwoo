# ============================================================
#  사용설명서.html  →  사용설명서.pdf
#
#  Chrome 헤드리스로 변환한다. 인쇄 대화상자를 쓰지 않으므로
#  "배경 그래픽 / 머리글·바닥글 / 배율" 설정 실수가 생기지 않는다.
#
#  사용법 : 이 폴더에서 PowerShell 실행 후
#           .\make_pdf.ps1
#  (실행 정책 오류가 나면)
#           powershell -ExecutionPolicy Bypass -File .\make_pdf.ps1
# ============================================================

$ErrorActionPreference = 'Stop'

$here = Split-Path -Parent $MyInvocation.MyCommand.Path

$src = Get-ChildItem -LiteralPath $here -Filter '사용설명서.html' | Select-Object -First 1
if (-not $src) {
  $src = Get-ChildItem -LiteralPath $here -Filter '*.html' | Select-Object -First 1
}
if (-not $src) { throw "HTML 원본을 찾을 수 없습니다 : $here" }

$html = $src.FullName
$pdf  = Join-Path $here ($src.BaseName + '.pdf')

$candidates = @(
  "$env:ProgramFiles\Google\Chrome\Application\chrome.exe",
  "${env:ProgramFiles(x86)}\Google\Chrome\Application\chrome.exe",
  "$env:LOCALAPPDATA\Google\Chrome\Application\chrome.exe",
  "$env:ProgramFiles\Microsoft\Edge\Application\msedge.exe",
  "${env:ProgramFiles(x86)}\Microsoft\Edge\Application\msedge.exe"
)
$browser = $candidates | Where-Object { Test-Path $_ } | Select-Object -First 1
if (-not $browser) { throw 'Chrome 또는 Edge를 찾을 수 없습니다.' }

$profileDir = Join-Path $env:TEMP 'samwoo_manual_pdf_profile'
$uri = ([uri]$html).AbsoluteUri

$arguments = @(
  '--headless=new'
  '--disable-gpu'
  '--no-sandbox'
  "--user-data-dir=$profileDir"
  '--no-pdf-header-footer'
  '--run-all-compositor-stages-before-draw'
  '--virtual-time-budget=20000'
  "--print-to-pdf=$pdf"
  $uri
)

Write-Host "브라우저 : $browser"
Write-Host "입력     : $html"
Write-Host "출력     : $pdf"
Write-Host '변환 중 ...'

$p = Start-Process -FilePath $browser -ArgumentList $arguments -Wait -PassThru -NoNewWindow
if ($p.ExitCode -ne 0) { throw "변환 실패 (exit code $($p.ExitCode))" }
if (-not (Test-Path $pdf)) { throw '변환은 끝났지만 PDF가 생성되지 않았습니다.' }

$item = Get-Item $pdf
Write-Host ('완료 : {0:N0} bytes  ({1})' -f $item.Length, $item.LastWriteTime)
