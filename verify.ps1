# Build the distributable TaskTrack human-decision, dashboard and optional tunnel
# supervisor targets, tests and example, then run deterministic Core and MCP checks.
param(
    [string]$UppRoot = $env:UPP_ROOT,
    [string]$RepoRoot = $PSScriptRoot,
    [string]$UiRoot = "",
    [string]$AnimationRoot = "",
    [string]$OutputDir = ""
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

function Get-FileIdentity {
    param([string]$Path, [string]$Name)
    $item = Get-Item -LiteralPath $Path
    [ordered]@{
        name = $Name
        size = $item.Length
        sha256 = (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant()
    }
}

function Read-GitIdentity {
    param([string]$Root)
    $gitCommand = Get-Command git.exe -ErrorAction SilentlyContinue
    if(!$gitCommand) { $gitCommand = Get-Command git -ErrorAction SilentlyContinue }
    if(!$gitCommand) { throw "git is required to record verified build provenance." }

    $commit = (& $gitCommand.Source -C $Root rev-parse HEAD 2>&1 | Out-String).Trim()
    if($LASTEXITCODE -ne 0 -or [string]::IsNullOrWhiteSpace($commit)) {
        throw "Unable to read repository HEAD for verification provenance."
    }
    $branchName = (& $gitCommand.Source -C $Root branch --show-current 2>&1 | Out-String).Trim()
    if($LASTEXITCODE -ne 0) { $branchName = "" }
    $status = (& $gitCommand.Source -C $Root status --porcelain=v1 --untracked-files=normal 2>&1 | Out-String)
    if($LASTEXITCODE -ne 0) { throw "Unable to read repository working-tree state." }

    [ordered]@{
        commit = $commit
        branch = $branchName
        dirty = ![string]::IsNullOrWhiteSpace($status)
    }
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
$buildDir = if([string]::IsNullOrWhiteSpace($OutputDir)) { Join-Path $RepoRoot "build" }
            else { [IO.Path]::GetFullPath($OutputDir) }
$buildVersion = Read-BuildVersion

if(!(Test-Path -LiteralPath $umk)) { throw "umk.exe not found at $umk" }
foreach($p in @($RepoRoot, $UiRoot, $AnimationRoot, (Join-Path $UppRoot "uppsrc"))) {
    if(!(Test-Path -LiteralPath $p)) { throw "Required assembly path not found: $p" }
}
New-Item -ItemType Directory -Force -Path $buildDir | Out-Null
Write-Host "TaskTrack build expected: $buildVersion"

Run-Step "Build TaskTrack GUI" { Build-UppPackage -Package "TaskTrack/App" -Target "TaskTrackGui" -Gui }
Run-Step "Build TaskTrack MCP" { Build-UppPackage -Package "TaskTrack/Mcp" -Target "TaskTrackMcp" }
Run-Step "Build TaskTrack tunnel GUI" { Build-UppPackage -Package "TaskTrack/TunnelApp" -Target "TaskTrackTunnelGui" -Gui }
Run-Step "Build MCP tunnel runtime tests" { Build-UppPackage -Package "tests/McpTunnelRuntimeTests" -Target "McpTunnelRuntimeTests" }
Run-Step "Build TaskTrack tests" { Build-UppPackage -Package "tests/TaskTrackTests" -Target "TaskTrackTests" }
Run-Step "Build TaskTrack example" { Build-UppPackage -Package "examples/TaskTrackExample" -Target "TaskTrackExample" }
Run-Step "Build Dashboard GUI" { Build-UppPackage -Package "TaskTrack/DashboardApp" -Target "TaskTrackDashboardGui" -Gui }
Run-Step "Build Dashboard tests" { Build-UppPackage -Package "tests/TaskTrackDashboardTests" -Target "TaskTrackDashboardTests" }

$expectedExecutables = @(
    "TaskTrackGui.exe",
    "TaskTrackMcp.exe",
    "TaskTrackTunnelGui.exe",
    "McpTunnelRuntimeTests.exe",
    "TaskTrackTests.exe",
    "TaskTrackExample.exe",
    "TaskTrackDashboardGui.exe",
    "TaskTrackDashboardTests.exe"
)
foreach($exe in $expectedExecutables) {
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
    $hashMatch = [regex]::Match($versionOutput, 'executable sha256\s+([0-9a-fA-F]{64})')
    if(!$hashMatch.Success) { throw "Fresh MCP binary does not report its executable SHA-256" }
    $reportedHash = $hashMatch.Groups[1].Value.ToLowerInvariant()
    $actualHash = (Get-FileHash -LiteralPath $mcpPath -Algorithm SHA256).Hash.ToLowerInvariant()
    if($reportedHash -ne $actualHash) { throw "MCP self-reported executable SHA-256 does not match the built file" }
}

$vendorRuntime = Join-Path $RepoRoot "tunnel-client\tunnel-client-runtime.exe"
Run-Step "MCP tunnel runtime tests (including vendor compatibility when available)" {
    if(Test-Path -LiteralPath $vendorRuntime) {
        & (Join-Path $buildDir "McpTunnelRuntimeTests.exe") --vendor-runtime $vendorRuntime
    }
    else { & (Join-Path $buildDir "McpTunnelRuntimeTests.exe") }
}
Run-Step "Core/persistence tests" { & (Join-Path $buildDir "TaskTrackTests.exe") }
Run-Step "Unified MCP selftest" { & $mcpPath --selftest }
Run-Step "Dashboard Core/persistence tests" { & (Join-Path $buildDir "TaskTrackDashboardTests.exe") }

$gitIdentity = Read-GitIdentity -Root $RepoRoot
$verifiedFiles = @()
foreach($exe in $expectedExecutables) {
    $verifiedFiles += Get-FileIdentity -Path (Join-Path $buildDir $exe) -Name $exe
}

$vendorIdentity = $null
if(Test-Path -LiteralPath $vendorRuntime) {
    $vendorIdentity = Get-FileIdentity -Path $vendorRuntime -Name "tunnel-client-runtime.exe"
}

$verificationManifest = [ordered]@{
    schema_version = 1
    status = "passed"
    tasktrack_build = $buildVersion
    source_commit = $gitIdentity.commit
    source_branch = $gitIdentity.branch
    source_dirty = $gitIdentity.dirty
    platform = "windows-x64"
    generated_utc = [DateTime]::UtcNow.ToString("o")
    files = $verifiedFiles
    vendor_runtime = $vendorIdentity
}
$verificationManifestPath = Join-Path $buildDir "verification-manifest.json"
$verificationManifest | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $verificationManifestPath -Encoding UTF8

Write-Host ""
Write-Host "Verification manifest: $verificationManifestPath"
if($gitIdentity.dirty) {
    Write-Warning "Verification passed, but the source working tree is dirty. stage-bin.ps1 will refuse to create a deployable bundle from this build."
}
Write-Host ""
Write-Host "verify.ps1: ok"
