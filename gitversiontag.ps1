# Check if tag option is provided
$createTag = $false
$uploadTag = $false
if ($args -contains "-t") {
    $createTag = $true
}
if ($args -contains "-u") {
    $uploadTag = $true
}

$versionFile = ".\Version.h"
if (-not (Test-Path $versionFile)) {
    Write-Host "Version.h 파일을 찾을 수 없습니다."
    exit 1
}

$raw = Get-Content $versionFile -Encoding utf8 -Raw
$lines = Get-Content $versionFile -Encoding utf8

$version = $null
$comment = $null

function Get-BlockComment([string]$rawText, [int]$searchFrom) {
    $blockStart = $rawText.IndexOf('/*', $searchFrom)
    $blockEnd = if ($blockStart -ge 0) { $rawText.IndexOf('*/', $blockStart + 2) } else { -1 }
    if ($blockStart -lt 0 -or $blockEnd -le $blockStart) {
        return $null
    }
    $block = $rawText.Substring($blockStart + 2, $blockEnd - $blockStart - 2)
    return ((($block -split "`r?`n") | ForEach-Object {
        ($_ -replace '^\s*\*\s?', '' -replace '^\s+', '' -replace '\s+$', '')
    } | Where-Object { $_ -ne '' }) -join "`n")
}

for ($i = 0; $i -lt $lines.Count; $i++) {
    $line = $lines[$i]
    # 주석 처리된 #define 은 건너뜀
    if ($line -notmatch '^\s*#define\s+VERSION\s+"(.*?)"(.*)$') {
        continue
    }

    $version = "v" + $Matches[1].Trim()
    $after = $Matches[2]

    if ($after -match '//(.*)$' -and $after -notmatch '/\*') {
        # // 한 줄만 (같은 줄에 /* 가 없을 때)
        $comment = $Matches[1].Trim()
    }
    else {
        # /* 같은 줄 또는 바로 다음 줄(들)에 있는 블록 주석
        $definePos = $raw.IndexOf($line)
        if ($definePos -lt 0) {
            $definePos = $raw.IndexOf('#define VERSION')
        }
        $searchFrom = $definePos + $line.Length
        # 같은 줄에 /* 가 있으면 줄 시작부터 검색
        if ($after -match '/\*') {
            $searchFrom = $definePos
        }
        $comment = Get-BlockComment $raw $searchFrom
        if ([string]::IsNullOrWhiteSpace($comment)) {
            # 다음 줄부터 나오는 /* 도 허용
            $comment = Get-BlockComment $raw $definePos
        }
    }
    break
}

if (-not $version -or [string]::IsNullOrWhiteSpace($comment)) {
    Write-Host "Version.h 첫 #define VERSION 에서 버전/커밋 메시지를 찾지 못했습니다."
    Write-Host '  // 한줄  : #define VERSION "x.y.z" // 커밋 메시지'
    Write-Host '  /* 여러줄 (같은 줄 또는 다음 줄):'
    Write-Host '     #define VERSION "x.y.z"'
    Write-Host '     /*'
    Write-Host '       커밋 메시지'
    Write-Host '     */'
    exit 1
}

$currentBranch = git rev-parse --abbrev-ref HEAD

Write-Host "Version: $version"
Write-Host "Comment:"
Write-Host $comment

# 여러 줄 커밋 메시지를 위해 -F 사용
$msgFile = [System.IO.Path]::GetTempFileName()
try {
    $utf8NoBom = New-Object System.Text.UTF8Encoding $false
    [System.IO.File]::WriteAllText($msgFile, $comment.Trim() + "`n", $utf8NoBom)

    git add -A
    git commit -a -F $msgFile
    git push origin $currentBranch

    if ($createTag) {
        Write-Host "Creating tag: $version"
        git tag -a $version -F $msgFile
    }
    if ($uploadTag) {
        Write-Host "Uploading tag: $version"
        git push origin $version
    }
    elseif (-not $createTag) {
        Write-Host 'Tag skipped. Use -t to create, -u to upload (e.g. -t -u).'
    }
}
finally {
    if (Test-Path $msgFile) { Remove-Item $msgFile -Force }
}
