# Release APK → IFTECH_SAMWOO_App/release/
$ErrorActionPreference = "Stop"
Set-Location $PSScriptRoot
flutter pub get
flutter build apk --release
New-Item -ItemType Directory -Force -Path "release" | Out-Null
$src = Join-Path $PSScriptRoot "build\app\outputs\flutter-apk\app-release.apk"
$ver = (Select-String -Path "pubspec.yaml" -Pattern "^version:\s*(.+)$").Matches[0].Groups[1].Value.Trim()
Copy-Item $src (Join-Path $PSScriptRoot "release\IFTECH_SAMWOO_App.apk") -Force
Copy-Item $src (Join-Path $PSScriptRoot "release\IFTECH_SAMWOO_App_$ver.apk") -Force
Write-Host "APK: release\IFTECH_SAMWOO_App.apk  ($ver)"
