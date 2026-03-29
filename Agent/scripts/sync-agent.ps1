<#
.SYNOPSIS
    Syncs Agent/skills/ and Agent/rules/ to .cursor/skills/ and .cursor/rules/
.DESCRIPTION
    One-way sync from the plugin's Agent/ directory (source of truth) to the
    project's .cursor/ directory (Cursor's runtime location).
    Run from the project root or pass -ProjectRoot explicitly.
#>
param(
    [string]$ProjectRoot = (Get-Location).Path
)

$ErrorActionPreference = 'Stop'

$pluginAgent = Join-Path $ProjectRoot 'Plugins\UnrealClientProtocol\Agent'
$cursorDir   = Join-Path $ProjectRoot '.cursor'

if (-not (Test-Path $pluginAgent)) {
    Write-Error "Agent directory not found at: $pluginAgent"
    exit 1
}

function Sync-Directory {
    param(
        [string]$Source,
        [string]$Destination,
        [string]$Label
    )

    if (-not (Test-Path $Source)) {
        Write-Host "  [$Label] Source not found, skipping: $Source" -ForegroundColor Yellow
        return
    }

    if (-not (Test-Path $Destination)) {
        New-Item -ItemType Directory -Path $Destination -Force | Out-Null
    }

    $sourceItems = Get-ChildItem $Source -Recurse -File
    $copied = 0
    $skipped = 0

    foreach ($item in $sourceItems) {
        $relativePath = $item.FullName.Substring($Source.Length).TrimStart('\', '/')
        $destPath = Join-Path $Destination $relativePath
        $destDir = Split-Path $destPath -Parent

        if (-not (Test-Path $destDir)) {
            New-Item -ItemType Directory -Path $destDir -Force | Out-Null
        }

        $needsCopy = $true
        if (Test-Path $destPath) {
            $srcHash  = (Get-FileHash $item.FullName -Algorithm MD5).Hash
            $destHash = (Get-FileHash $destPath -Algorithm MD5).Hash
            if ($srcHash -eq $destHash) {
                $needsCopy = $false
                $skipped++
            }
        }

        if ($needsCopy) {
            Copy-Item $item.FullName $destPath -Force
            $copied++
        }
    }

    # Remove files in destination that no longer exist in source
    $removed = 0
    if (Test-Path $Destination) {
        $destItems = Get-ChildItem $Destination -Recurse -File
        foreach ($item in $destItems) {
            $relativePath = $item.FullName.Substring($Destination.Length).TrimStart('\', '/')
            $srcPath = Join-Path $Source $relativePath
            if (-not (Test-Path $srcPath)) {
                Remove-Item $item.FullName -Force
                $removed++
            }
        }
    }

    Write-Host "  [$Label] Copied: $copied | Unchanged: $skipped | Removed: $removed" -ForegroundColor Cyan
}

Write-Host "Syncing Agent -> .cursor ..." -ForegroundColor Green
Write-Host "  Source: $pluginAgent" -ForegroundColor DarkGray
Write-Host "  Target: $cursorDir" -ForegroundColor DarkGray

Sync-Directory `
    -Source (Join-Path $pluginAgent 'skills') `
    -Destination (Join-Path $cursorDir 'skills') `
    -Label 'skills'

Sync-Directory `
    -Source (Join-Path $pluginAgent 'rules') `
    -Destination (Join-Path $cursorDir 'rules') `
    -Label 'rules'

Write-Host "Sync complete." -ForegroundColor Green
