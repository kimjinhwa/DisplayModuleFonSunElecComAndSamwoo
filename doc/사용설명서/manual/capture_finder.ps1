# IP Finder 창을 화면에 띄운 뒤 CopyFromScreen 으로 PNG 저장.
$ErrorActionPreference = 'Stop'
$here = Split-Path -Parent $MyInvocation.MyCommand.Path
$img = Join-Path $here 'img'
$repo = (Resolve-Path (Join-Path $here '..\..\..')).Path
$py = Join-Path $repo 'python\SnmpFinder.py'

Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.Drawing
Add-Type @"
using System;
using System.Runtime.InteropServices;
public class NativeWin2 {
  [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr hWnd, out RECT lpRect);
  [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr hWnd);
  [DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr hWnd, int nCmdShow);
  [StructLayout(LayoutKind.Sequential)] public struct RECT { public int Left; public int Top; public int Right; public int Bottom; }
}
"@

$p = Start-Process -FilePath 'python' -ArgumentList @($py, '--autosearch') -WorkingDirectory $repo -PassThru
$hwnd = [IntPtr]::Zero
for ($i = 0; $i -lt 50; $i++) {
  Start-Sleep -Milliseconds 200
  $proc = Get-Process -Id $p.Id -ErrorAction SilentlyContinue
  if ($proc -and $proc.MainWindowHandle -ne [IntPtr]::Zero) {
    $hwnd = $proc.MainWindowHandle
    break
  }
}
if ($hwnd -eq [IntPtr]::Zero) {
  if ($p -and !$p.HasExited) { $p.Kill() }
  throw 'IP Finder 창을 찾지 못했습니다.'
}

[void][NativeWin2]::ShowWindow($hwnd, 9)
[void][NativeWin2]::SetForegroundWindow($hwnd)

function Save-Screen([string]$path) {
  $r = New-Object NativeWin2+RECT
  [void][NativeWin2]::GetWindowRect($hwnd, [ref]$r)
  $w = [Math]::Max(100, $r.Right - $r.Left)
  $ht = [Math]::Max(100, $r.Bottom - $r.Top)
  $bmp = New-Object System.Drawing.Bitmap $w, $ht
  $g = [System.Drawing.Graphics]::FromImage($bmp)
  $g.CopyFromScreen($r.Left, $r.Top, 0, 0, (New-Object System.Drawing.Size($w, $ht)))
  $g.Dispose()
  $bmp.Save($path, [System.Drawing.Imaging.ImageFormat]::Png)
  $bmp.Dispose()
  Write-Host "saved $path"
}

Start-Sleep -Milliseconds 800
Save-Screen (Join-Path $img 'ipfinder_01_start.png')
Start-Sleep -Seconds 3
Save-Screen (Join-Path $img 'ipfinder_02_found.png')

if ($p -and !$p.HasExited) {
  $p.CloseMainWindow() | Out-Null
  Start-Sleep -Milliseconds 500
  if (!$p.HasExited) { $p.Kill() }
}
