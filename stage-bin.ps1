# Stage a verified TaskTrack build into a clean, copyable runtime bundle.
# This script does not compile or run tests; run verify.ps1 first.
param(
    [string]$RepoRoot = $PSScriptRoot,
    [string]$BuildDir = "",
    [string]$BinDir = "",
    [string]$VendorRuntime = ""
)

$ErrorActionPreference = "Stop"

function Full-Path {
    param([string]$Path, [string]$Base)
    if([IO.Path]::IsPathRooted($Path)) { return [IO.Path]::GetFullPath($Path) }
    return [IO.Path]::GetFullPath((Join-Path $Base $Path))
}

function Read-BuildVersion {
    param([string]$Root)
    $buildHeader = Join-Path $Root "TaskTrack\Core\TaskTrackBuild.h"
    if(!(Test-Path -LiteralPath $buildHeader)) { throw "TaskTrack build header not found: $buildHeader" }
    $text = Get-Content -LiteralPath $buildHeader -Raw
    $match = [regex]::Match($text, 'return\s+"([^"]+)"\s*;')
    if(!$match.Success) { throw "Unable to read TaskTrack build version from $buildHeader" }
    return $match.Groups[1].Value
}

function File-Identity {
    param([string]$Path, [string]$RelativeName)
    $item = Get-Item -LiteralPath $Path
    [ordered]@{
        name = $RelativeName.Replace("\", "/")
        size = $item.Length
        sha256 = (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant()
    }
}

function Copy-TreeMissing {
    param([string]$Source, [string]$Destination)
    if(!(Test-Path -LiteralPath $Source)) { return }
    New-Item -ItemType Directory -Force -Path $Destination | Out-Null
    Get-ChildItem -LiteralPath $Source -Force | ForEach-Object {
        $target = Join-Path $Destination $_.Name
        if($_.PSIsContainer) {
            Copy-TreeMissing -Source $_.FullName -Destination $target
        }
        elseif(!$_.Name.EndsWith(".tmp", [StringComparison]::OrdinalIgnoreCase) -and
               !$_.Name.EndsWith(".lock", [StringComparison]::OrdinalIgnoreCase) -and
               !(Test-Path -LiteralPath $target)) {
            Copy-Item -LiteralPath $_.FullName -Destination $target
        }
    }
}

$RepoRoot = [IO.Path]::GetFullPath($RepoRoot)
if([string]::IsNullOrWhiteSpace($BuildDir)) { $BuildDir = Join-Path $RepoRoot "build" }
else { $BuildDir = Full-Path -Path $BuildDir -Base $RepoRoot }
if([string]::IsNullOrWhiteSpace($BinDir)) { $BinDir = Join-Path $RepoRoot "bin\windows-x64" }
else { $BinDir = Full-Path -Path $BinDir -Base $RepoRoot }

$verificationManifestPath = Join-Path $BuildDir "verification-manifest.json"
if(!(Test-Path -LiteralPath $verificationManifestPath)) {
    throw "Verified build manifest not found: $verificationManifestPath. Run verify.ps1 against this output directory first."
}
$verificationManifest = Get-Content -LiteralPath $verificationManifestPath -Raw | ConvertFrom-Json
if($verificationManifest.status -ne "passed" -or $verificationManifest.platform -ne "windows-x64") {
    throw "Build verification manifest does not describe a passed windows-x64 verification."
}
if([bool]$verificationManifest.source_dirty) {
    throw "Refusing to stage a deployable bundle from a dirty source tree. Re-run verify.ps1 from a clean checkout."
}

$gitCommand = Get-Command git.exe -ErrorAction SilentlyContinue
if(!$gitCommand) { $gitCommand = Get-Command git -ErrorAction SilentlyContinue }
if(!$gitCommand) { throw "git is required to confirm build/source provenance before staging." }
$currentCommit = (& $gitCommand.Source -C $RepoRoot rev-parse HEAD 2>&1 | Out-String).Trim()
if($LASTEXITCODE -ne 0 -or $currentCommit -ne [string]$verificationManifest.source_commit) {
    throw "Current repository HEAD does not match the commit recorded by verify.ps1. Rebuild/verify current main before staging."
}
$currentStatus = (& $gitCommand.Source -C $RepoRoot status --porcelain=v1 --untracked-files=normal 2>&1 | Out-String)
if($LASTEXITCODE -ne 0 -or ![string]::IsNullOrWhiteSpace($currentStatus)) {
    throw "Refusing to stage from a dirty working tree."
}

$vendorManifestPath = Join-Path $RepoRoot "tunnel-client\runtime-manifest.json"
if(!(Test-Path -LiteralPath $vendorManifestPath)) {
    throw "Vendor runtime manifest not found: $vendorManifestPath"
}
$vendorManifest = Get-Content -LiteralPath $vendorManifestPath -Raw | ConvertFrom-Json
if($vendorManifest.platform -ne "windows-amd64") {
    throw "stage-bin.ps1 currently supports only the pinned windows-amd64 vendor runtime."
}

if([string]::IsNullOrWhiteSpace($VendorRuntime)) {
    $VendorRuntime = Join-Path (Join-Path $RepoRoot "tunnel-client") $vendorManifest.local_filename
}
else { $VendorRuntime = Full-Path -Path $VendorRuntime -Base $RepoRoot }

$productionExecutables = @(
    "TaskTrackMcp.exe",
    "TaskTrackGui.exe",
    "TaskTrackDashboardGui.exe",
    "TaskTrackTunnelGui.exe"
)

foreach($name in $productionExecutables) {
    $source = Join-Path $BuildDir $name
    if(!(Test-Path -LiteralPath $source)) {
        throw "Required verified build output is missing: $source"
    }
    $verified = @($verificationManifest.files | Where-Object { $_.name -eq $name })
    if($verified.Count -ne 1) {
        throw "Verification manifest does not contain exactly one identity for $name."
    }
    $actualHash = (Get-FileHash -LiteralPath $source -Algorithm SHA256).Hash.ToLowerInvariant()
    if($actualHash -ne ([string]$verified[0].sha256).ToLowerInvariant()) {
        throw "Build output changed after verification: $name"
    }
}
if(!(Test-Path -LiteralPath $VendorRuntime)) {
    throw "Pinned OpenAI tunnel runtime is missing: $VendorRuntime"
}
if($null -eq $verificationManifest.vendor_runtime) {
    throw "The verified build did not include vendor-runtime compatibility testing. Re-run verify.ps1 with the pinned runtime present."
}

$expectedVendorHash = ([string]$vendorManifest.sha256).ToLowerInvariant()
$actualVendorHash = (Get-FileHash -LiteralPath $VendorRuntime -Algorithm SHA256).Hash.ToLowerInvariant()
if($actualVendorHash -ne $expectedVendorHash) {
    throw "OpenAI tunnel runtime SHA-256 does not match the validated artifact. Expected $expectedVendorHash, got $actualVendorHash. Validate the new vendor artifact before updating tunnel-client/runtime-manifest.json."
}
$verifiedVendorHash = ([string]$verificationManifest.vendor_runtime.sha256).ToLowerInvariant()
if($actualVendorHash -ne $verifiedVendorHash) {
    throw "The vendor runtime changed after verify.ps1 compatibility testing."
}

$appDataRoot = [Environment]::GetFolderPath([Environment+SpecialFolder]::ApplicationData)
if([string]::IsNullOrWhiteSpace($appDataRoot)) {
    throw "Cannot migrate TaskTrack persistent state because the Windows ApplicationData folder is unavailable."
}

$legacyDashboardDir = Join-Path $BinDir "tasktrack_dashboard_data"
if(Test-Path -LiteralPath $legacyDashboardDir) {
    $persistentDashboardDir = Join-Path $appDataRoot "TaskTrack\dashboard_data"
    Write-Host "Migrating legacy dashboard data to $persistentDashboardDir"
    Copy-TreeMissing -Source $legacyDashboardDir -Destination $persistentDashboardDir
}

$persistentTunnelDir = Join-Path $appDataRoot "TaskTrack\tunnel"
New-Item -ItemType Directory -Force -Path $persistentTunnelDir | Out-Null
foreach($stateName in @(
    "tasktrack-tunnel-profiles.json",
    "tasktrack-tunnel-probe.json",
    "tasktrack-tunnel-activity.json"
)) {
    $legacyState = Join-Path $BinDir $stateName
    $persistentState = Join-Path $persistentTunnelDir $stateName
    if((Test-Path -LiteralPath $legacyState) -and !(Test-Path -LiteralPath $persistentState)) {
        Write-Host "Migrating legacy tunnel state $stateName to $persistentTunnelDir"
        Copy-Item -LiteralPath $legacyState -Destination $persistentState
    }
}

if(Test-Path -LiteralPath $BinDir) {
    Remove-Item -LiteralPath $BinDir -Recurse -Force
}
New-Item -ItemType Directory -Force -Path $BinDir | Out-Null
$thirdPartyDir = Join-Path $BinDir "third-party\openai-tunnel-client"
New-Item -ItemType Directory -Force -Path $thirdPartyDir | Out-Null

foreach($name in $productionExecutables) {
    Copy-Item -LiteralPath (Join-Path $BuildDir $name) -Destination (Join-Path $BinDir $name)
}

$stagedVendorName = [string]$vendorManifest.staged_filename
Copy-Item -LiteralPath $VendorRuntime -Destination (Join-Path $BinDir $stagedVendorName)

Copy-Item -LiteralPath (Join-Path $RepoRoot "LICENSE") -Destination (Join-Path $BinDir "LICENSE-TaskTrack.txt")
foreach($name in @("LICENSE", "NOTICE", [string]$vendorManifest.licenses_file, [string]$vendorManifest.sbom_file)) {
    $source = Join-Path (Join-Path $RepoRoot "tunnel-client") $name
    if(!(Test-Path -LiteralPath $source)) { throw "Required vendor notice file is missing: $source" }
    Copy-Item -LiteralPath $source -Destination (Join-Path $thirdPartyDir $name)
}

$runtimeReadme = @"
TaskTrack runtime bundle
========================

TaskTrack build: $(Read-BuildVersion -Root $RepoRoot)
Platform: windows-x64

Run TaskTrackTunnelGui.exe for the machine tunnel manager.
The staged OpenAI runtime is named tunnel-client.exe so the manager can find it
beside itself without an explicit --client path.

For a local coding-agent MCP connection, register TaskTrackMcp.exe.
Keep TaskTrackMcp.exe, TaskTrackGui.exe and TaskTrackDashboardGui.exe together.

This directory is a staged runtime bundle. Do not store API keys, vault files,
profile JSON, dashboards or logs here. Dashboard state belongs in the per-user
TaskTrack application-data folder.

See manifest.json for SHA-256 identities and third-party/openai-tunnel-client/
for the bundled vendor notices/SBOM.
"@
Set-Content -LiteralPath (Join-Path $BinDir "README.txt") -Value $runtimeReadme -Encoding UTF8

Copy-Item -LiteralPath $verificationManifestPath -Destination (Join-Path $BinDir "verification-manifest.json")

$sourceCommit = [string]$verificationManifest.source_commit

$files = @()
foreach($name in $productionExecutables + @($stagedVendorName)) {
    $files += File-Identity -Path (Join-Path $BinDir $name) -RelativeName $name
}

$bundleManifest = [ordered]@{
    schema_version = 1
    tasktrack_build = Read-BuildVersion -Root $RepoRoot
    source_commit = $sourceCommit
    platform = "windows-x64"
    generated_utc = [DateTime]::UtcNow.ToString("o")
    verification_manifest_sha256 = (Get-FileHash -LiteralPath (Join-Path $BinDir "verification-manifest.json") -Algorithm SHA256).Hash.ToLowerInvariant()
    vendor_runtime = [ordered]@{
        name = [string]$vendorManifest.name
        version = [string]$vendorManifest.version
        source_repository = [string]$vendorManifest.source_repository
        staged_name = $stagedVendorName
        sha256 = $actualVendorHash
        validated_with_tasktrack_commit = [string]$vendorManifest.validated_with_tasktrack_commit
    }
    files = $files
}
$manifestJson = $bundleManifest | ConvertTo-Json -Depth 6
$manifestPath = Join-Path $BinDir "manifest.json"
[System.IO.File]::WriteAllText($manifestPath, $manifestJson, [System.Text.UTF8Encoding]::new($false))

Write-Host ""
Write-Host "TaskTrack runtime bundle staged:"
Write-Host "  $BinDir"
Write-Host ""
foreach($file in $files) {
    Write-Host ("  {0,-30} {1}" -f $file.name, $file.sha256)
}
Write-Host ""
Write-Host "Vendor runtime hash verified against tunnel-client/runtime-manifest.json."
