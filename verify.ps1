# Windows convenience wrapper for TaskTrack.
# The normal U++ package build remains authoritative; this script repeats the
# release builds and runs the deterministic core/MCP checks in one command.
param(
    [string]$UppRoot = "E:\upp-18468",
    [string]$RepoRoot = $PSScriptRoot,
    [string]$UiRoot = "E:\apps\github\upp_Ui",
    [string]$StateMachineRoot = "E:\apps\github\upp_statemachine",
    [string]$AnimationRoot = "E:\apps\github\upp_animation"
)

$ErrorActionPreference = "Stop"

function Run-Step {
    param(
        [string]$Name,
        [scriptblock]$Body
    )

    Write-Host ""
    Write-Host "== $Name =="
    & $Body
    if($LASTEXITCODE -ne 0) {
        throw "$Name failed with exit code $LASTEXITCODE"
    }
}

function Read-ValidationBuild {
    $buildHeader = Join-Path $RepoRoot "TaskTrack\Core\TaskTrackBuild.h"
    if(!(Test-Path -LiteralPath $buildHeader)) {
        throw "TaskTrack build header not found: $buildHeader"
    }

    $text = Get-Content -LiteralPath $buildHeader -Raw
    $match = [regex]::Match($text, 'return\s+"([^"]+)"\s*;')
    if(!$match.Success) {
        throw "Unable to read TaskTrack validation build from $buildHeader"
    }
    return $match.Groups[1].Value
}

function Remove-OldTarget {
    param([string]$Target)

    $targetPath = Join-Path $buildDir $Target
    foreach($candidate in @($targetPath, "$targetPath.exe")) {
        if(Test-Path -LiteralPath $candidate) {
            Write-Host "Removing old target: $candidate"
            Remove-Item -LiteralPath $candidate -Force
        }
    }
}

function Build-UppPackage {
    param(
        [string]$Package,
        [string]$Target,
        [switch]$Gui
    )

    Remove-OldTarget -Target $Target
    $targetPath = Join-Path $buildDir $Target
    if($Gui) {
        & $umk $assembly $Package CLANGx64 --out-dir $outDir -br $targetPath
    }
    else {
        & $umk $assembly $Package CLANGx64 --out-dir $outDir -br +CONSOLE $targetPath
    }
}

function Show-BinaryIdentity {
    param([string]$Path)

    if(!(Test-Path -LiteralPath $Path)) {
        throw "Expected build output is missing: $Path"
    }

    $item = Get-Item -LiteralPath $Path
    $hash = (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash
    Write-Host "Executable: $($item.FullName)"
    Write-Host "LastWriteTimeUtc: $($item.LastWriteTimeUtc.ToString('o'))"
    Write-Host "Length: $($item.Length)"
    Write-Host "SHA256: $hash"
}

$umk = Join-Path $UppRoot "umk.exe"
$assembly = "$RepoRoot,$UiRoot,$StateMachineRoot,$AnimationRoot,$UppRoot\uppsrc"
$outDir = Join-Path $RepoRoot "out"
$buildDir = Join-Path $RepoRoot "build"
$validationBuild = Read-ValidationBuild

if(!(Test-Path -LiteralPath $umk)) {
    throw "umk.exe not found at $umk"
}
foreach($path in @($RepoRoot, $UiRoot, $StateMachineRoot, $AnimationRoot, (Join-Path $UppRoot "uppsrc"))) {
    if(!(Test-Path -LiteralPath $path)) {
        throw "Required assembly path not found: $path"
    }
}

New-Item -ItemType Directory -Force -Path $buildDir | Out-Null
New-Item -ItemType Directory -Force -Path $outDir | Out-Null

Write-Host "TaskTrack validation build expected: $validationBuild"

Run-Step "Build TaskTrack GUI" {
    Build-UppPackage -Package "TaskTrack/App" -Target "TaskTrackGui" -Gui
}

Run-Step "Build TaskTrack MCP" {
    Build-UppPackage -Package "TaskTrack/Mcp" -Target "TaskTrackMcp"
}

Run-Step "Build TaskTrack tests" {
    Build-UppPackage -Package "tests/TaskTrackTests" -Target "TaskTrackTests"
}

Run-Step "Build TaskTrack example" {
    Build-UppPackage -Package "examples/TaskTrackExample" -Target "TaskTrackExample"
}

foreach($exe in @("TaskTrackGui.exe", "TaskTrackMcp.exe", "TaskTrackTests.exe", "TaskTrackExample.exe")) {
    $path = Join-Path $buildDir $exe
    if(!(Test-Path -LiteralPath $path)) {
        throw "Expected build output is missing: $path"
    }
}

$mcpPath = Join-Path $buildDir "TaskTrackMcp.exe"
Run-Step "MCP binary identity" {
    Show-BinaryIdentity -Path $mcpPath
    $versionOutput = (& $mcpPath --version 2>&1 | Out-String).TrimEnd()
    Write-Host $versionOutput
    if($LASTEXITCODE -ne 0) {
        throw "TaskTrackMcp.exe --version failed with exit code $LASTEXITCODE"
    }
    $required = "validation build $validationBuild"
    if($versionOutput -notmatch [regex]::Escape($required)) {
        throw "Fresh MCP binary does not report expected identity '$required'"
    }
}

Run-Step "Core/persistence tests" {
    & (Join-Path $buildDir "TaskTrackTests.exe")
}

Run-Step "MCP selftest" {
    & $mcpPath --selftest
}

Write-Host ""
Write-Host "verify.ps1: ok"
