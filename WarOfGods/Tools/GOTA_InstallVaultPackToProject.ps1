# GOTA - Install Marketplace/Fab VaultCache packs into WarOfGods without Epic Launcher Add-to-Project.
# Fixes stuck-at-100% installs that target the wrong/full disk (e.g. K:\UnrealProjects\WarOfGods).
#
# Usage:
#   powershell -ExecutionPolicy Bypass -File Tools\GOTA_InstallVaultPackToProject.ps1 -Pack Iceland,Nordic,Moab
#
# Copies only missing files (never overwrites existing Megascans/MSPresets).
# Skips demo Maps by default. Does NOT delete VaultCache.

[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string[]]$Pack,

    [string]$VaultRoot = 'B:\Epic\VaultCache',
    [string]$ProjectRoot = 'B:\Unreal\UnitedPlanetsOnline\WarOfGods',
    [switch]$IncludeMaps,
    [switch]$WhatIf
)

$ErrorActionPreference = 'Stop'

function Get-FreeGB([string]$Path) {
    $item = Get-Item $Path
    $name = $item.PSDrive.Name
    $drive = Get-PSDrive -Name $name
    return [math]::Round($drive.Free / 1GB, 1)
}

function Get-DirSizeGB([string]$Path) {
    $sum = (Get-ChildItem $Path -Recurse -File -ErrorAction SilentlyContinue | Measure-Object Length -Sum).Sum
    if (-not $sum) { return 0 }
    return [math]::Round($sum / 1GB, 2)
}

$uproject = Join-Path $ProjectRoot 'WarOfGods.uproject'
if (-not (Test-Path $uproject)) {
    throw "Not a WarOfGods project root: $ProjectRoot"
}

$destContent = Join-Path $ProjectRoot 'Content'
$free = Get-FreeGB $ProjectRoot
Write-Host "Project: $ProjectRoot"
Write-Host "Free space: $free GB"
Write-Host "Mode: copy MISSING files only (no overwrite). IncludeMaps=$IncludeMaps"

$needed = 0.0
foreach ($name in $Pack) {
    $src = Join-Path $VaultRoot (Join-Path $name 'data\Content')
    if (-not (Test-Path $src)) {
        throw "Missing vault Content: $src"
    }
    $sz = Get-DirSizeGB $src
    $needed += $sz
    Write-Host ("  Pack {0}: ~{1} GB at {2}" -f $name, $sz, $src)
}

if ($free -lt (($needed * 0.35) + 20)) {
    Write-Warning "Free space ($free GB) may be tight vs pack sizes (~$needed GB). Continuing with missing-only copy."
}

foreach ($name in $Pack) {
    $srcContent = Join-Path $VaultRoot (Join-Path $name 'data\Content')
    Write-Host ""
    Write-Host "==== Installing $name ====" -ForegroundColor Cyan

    $robolog = Join-Path $env:TEMP ("GOTA_Install_{0}.log" -f $name)
    $argList = New-Object System.Collections.Generic.List[string]
    [void]$argList.Add($srcContent)
    [void]$argList.Add($destContent)
    [void]$argList.Add('/E')
    [void]$argList.Add('/XO')
    [void]$argList.Add('/XN')
    [void]$argList.Add('/XC')
    [void]$argList.Add('/COPY:DAT')
    [void]$argList.Add('/R:2')
    [void]$argList.Add('/W:2')
    [void]$argList.Add('/MT:8')
    [void]$argList.Add('/NFL')
    [void]$argList.Add('/NDL')
    [void]$argList.Add('/NP')
    [void]$argList.Add("/LOG:$robolog")
    if (-not $IncludeMaps) {
        [void]$argList.Add('/XD')
        [void]$argList.Add('Maps')
    }

    if ($WhatIf) {
        Write-Host ("WHATIF robocopy {0}" -f ($argList -join ' '))
        continue
    }

    & robocopy @($argList.ToArray()) | Out-Null
    $code = $LASTEXITCODE
    if ($code -ge 8) {
        Write-Host "robocopy FAILED exit=$code - see $robolog" -ForegroundColor Red
        throw "Install failed for $name (robocopy $code)"
    }

    $markerDir = Join-Path $ProjectRoot 'Content\_GOTA_VaultInstall'
    New-Item -ItemType Directory -Force -Path $markerDir | Out-Null
    $stamp = Get-Date -Format o
    $marker = Join-Path $markerDir ("{0}.txt" -f $name)
    Set-Content -Path $marker -Value "$stamp installed from $srcContent skip-existing IncludeMaps=$IncludeMaps robocopy=$code" -Encoding UTF8

    Write-Host "Installed $name (robocopy=$code). Log: $robolog" -ForegroundColor Green
}

Write-Host ""
Write-Host "Done. Open WarOfGods in UE 5.8 so it can discover new assets." -ForegroundColor Green
Write-Host "In Epic Launcher: cancel stuck Add-to-Project jobs for these packs."
Write-Host "Next time pick: B:\Unreal\UnitedPlanetsOnline\WarOfGods"
Write-Host "Do NOT pick: K:\UnrealProjects\WarOfGods (only ~68 GB free; packs need ~80 GB)."
