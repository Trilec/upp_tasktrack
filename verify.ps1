# Build the distributable TaskTrack human-decision and dashboard targets, tests
# and example, then run deterministic Core and unified MCP checks.
param(
    [string]$UppRoot = $env:UPP_ROOT,
    [string]$RepoRoot = $PSScriptRoot,
    [string]$UiRoot = "",
    [string]$AnimationRoot = ""
)

$ErrorActionPreference = "Stop"

function Run-Step {
    param([string]$Name, [scriptblock]$Body)
    Write-Host ""
    Write-Host "== $Name =="
    & $Body
    if($LASTEXITCODE -ne 0) { throw "$Name failed with exit code $LASTEXITCODE" }
}

function Read-BuildVersion {
    $buildHeader = Join-Path $RepoRoot "TaskTrack\Core\TaskTrackBuild.h"
    if(!(Test-Path -LiteralPath $buildHeader)) { throw "TaskTrack build header not found: $buildHeader" }
    $text = Get-Content -LiteralPath $buildHeader -Raw
    $match = [regex]::Match($text, 'return\s+"([^"]+)"\s*;')
    if(!$match.Success) { throw "Unable to read TaskTrack build version from $buildHeader" }
    return $match.Groups[1].Value
}

function Remove-OldTarget {
    param([string]$Target)
    $targetPath = Join-Path $buildDir $Target
    foreach($candidate in @($targetPath, "$targetPath.exe")) {
        if(Test-Path -LiteralPath $candidate) { Remove-Item -LiteralPath $candidate -Force }
    }
}

function Build-UppPackage {
    param([string]$Package, [string]$Target, [switch]$Gui)
    Remove-OldTarget -Target $Target
    $targetPath = Join-Path $buildDir $Target
    if($Gui) { & $umk $assembly $Package CLANGx64 --out-dir $buildDir -br $targetPath }
    else { & $umk $assembly $Package CLANGx64 --out-dir $buildDir -br +CONSOLE $targetPath }
}

function Show-BinaryIdentity {
    param([string]$Path)
    if(!(Test-Path -LiteralPath $Path)) { throw "Expected build output is missing: $Path" }
    $item = Get-Item -LiteralPath $Path
    $hash = (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash
    Write-Host "Executable: $($item.FullName)"
    Write-Host "LastWriteTimeUtc: $($item.LastWriteTimeUtc.ToString('o'))"
    Write-Host "Length: $($item.Length)"
    Write-Host "SHA256: $hash"
}

$repoParent = Split-Path -Parent (Resolve-Path -LiteralPath $RepoRoot).Path
if([string]::IsNullOrWhiteSpace($UiRoot)) { $UiRoot = Join-Path $repoParent "upp_Ui" }
if([string]::IsNullOrWhiteSpace($AnimationRoot)) { $AnimationRoot = Join-Path $repoParent "upp_animation" }
if([string]::IsNullOrWhiteSpace($UppRoot)) {
    $umkCommand = Get-Command umk.exe -ErrorAction SilentlyContinue
    if($umkCommand) { $UppRoot = Split-Path -Parent $umkCommand.Source }
    else { throw "U++ root is unknown. Pass -UppRoot, set UPP_ROOT, or put umk.exe on PATH." }
}

$umk = Join-Path $UppRoot "umk.exe"
$assembly = "$RepoRoot,$UiRoot,$AnimationRoot,$UppRoot\uppsrc"
$buildDir = Join-Path $RepoRoot "build"
$buildVersion = Read-BuildVersion

if(!(Test-Path -LiteralPath $umk)) { throw "umk.exe not found at $umk" }
foreach($p in @($RepoRoot, $UiRoot, $AnimationRoot, (Join-Path $UppRoot "uppsrc"))) {
    if(!(Test-Path -LiteralPath $p)) { throw "Required assembly path not found: $p" }
}
New-Item -ItemType Directory -Force -Path $buildDir | Out-Null
Write-Host "TaskTrack build expected: $buildVersion"

Run-Step "Build TaskTrack GUI" { Build-UppPackage -Package "TaskTrack/App" -Target "TaskTrackGui" -Gui }
Run-Step "Build TaskTrack MCP" { Build-UppPackage -Package "TaskTrack/Mcp" -Target "TaskTrackMcp" }
Run-Step "Build TaskTrack tests" { Build-UppPackage -Package "tests/TaskTrackTests" -Target "TaskTrackTests" }
Run-Step "Build TaskTrack example" { Build-UppPackage -Package "examples/TaskTrackExample" -Target "TaskTrackExample" }
Run-Step "Build Dashboard GUI" { Build-UppPackage -Package "TaskTrack/DashboardApp" -Target "TaskTrackDashboardGui" -Gui }
Run-Step "Build Dashboard tests" { Build-UppPackage -Package "tests/TaskTrackDashboardTests" -Target "TaskTrackDashboardTests" }

foreach($exe in @("TaskTrackGui.exe", "TaskTrackMcp.exe", "TaskTrackTests.exe", "TaskTrackExample.exe",
                  "TaskTrackDashboardGui.exe", "TaskTrackDashboardTests.exe")) {
    if(!(Test-Path -LiteralPath (Join-Path $buildDir $exe))) { throw "Expected build output is missing: $exe" }
}

$mcpPath = Join-Path $buildDir "TaskTrackMcp.exe"
Run-Step "Unified MCP binary identity" {
    Show-BinaryIdentity -Path $mcpPath
    $versionOutput = (& $mcpPath --version 2>&1 | Out-String).TrimEnd()
    Write-Host $versionOutput
    if($versionOutput -notmatch [regex]::Escape($buildVersion)) { throw "Fresh MCP binary does not report expected build '$buildVersion'" }
    if($versionOutput -notmatch 'task schema version\s+2') { throw "Fresh MCP binary does not report task schema 2" }
    if($versionOutput -notmatch 'dashboard schema version\s+1') { throw "Fresh MCP binary does not report dashboard schema 1" }
}

Run-Step "Core/persistence tests" { & (Join-Path $buildDir "TaskTrackTests.exe") }
Run-Step "Unified MCP selftest" { & $mcpPath --selftest }
Run-Step "Dashboard Core/persistence tests" { & (Join-Path $buildDir "TaskTrackDashboardTests.exe") }

Write-Host ""
Write-Host "verify.ps1: ok"
