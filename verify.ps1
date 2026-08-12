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

function Build-UppPackage {
    param(
        [string]$Package,
        [string]$Target,
        [switch]$Gui
    )

    $targetPath = Join-Path $buildDir $Target
    if($Gui) {
        & $umk $assembly $Package CLANGx64 --out-dir $outDir -br $targetPath
    }
    else {
        & $umk $assembly $Package CLANGx64 --out-dir $outDir -br +CONSOLE $targetPath
    }
}

$umk = Join-Path $UppRoot "umk.exe"
$assembly = "$RepoRoot,$UiRoot,$StateMachineRoot,$AnimationRoot,$UppRoot\uppsrc"
$outDir = Join-Path $RepoRoot "out"
$buildDir = Join-Path $RepoRoot "build"

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

Run-Step "Build TaskTrack GUI" {
    Build-UppPackage -Package "TaskTrack/App" -Target "TaskTrack" -Gui
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

Run-Step "Core/persistence tests" {
    & (Join-Path $buildDir "TaskTrackTests.exe")
}

Run-Step "MCP selftest" {
    & (Join-Path $buildDir "TaskTrackMcp.exe") --selftest
}

foreach($exe in @("TaskTrack.exe", "TaskTrackMcp.exe", "TaskTrackTests.exe", "TaskTrackExample.exe")) {
    $path = Join-Path $buildDir $exe
    if(!(Test-Path -LiteralPath $path)) {
        throw "Expected build output is missing: $path"
    }
}

Write-Host ""
Write-Host "verify.ps1: ok"
