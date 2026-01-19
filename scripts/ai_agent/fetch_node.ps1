# Fetch and extract a portable Node runtime (Windows x64) into a target directory.
#
# This script:
# - downloads node-v<Version>-win-x64.zip
# - verifies SHA256 against SHASUMS256.txt
# - extracts node.exe into <DestinationDir>\node.exe
#
# Usage example:
#   pwsh -c 'scripts/ai_agent/fetch_node.ps1 -Version 20.11.0 -DestinationDir x64/Development/ai/node'

param(
    [string]$Version = "20.11.0",
    [string]$DestinationDir
)

$ErrorActionPreference = "Stop"

if ([string]::IsNullOrWhiteSpace($DestinationDir)) {
    throw "DestinationDir is required."
}

$platform = "win-x64"
$distBase = "https://nodejs.org/dist/v$Version"
$archiveName = "node-v$Version-$platform.zip"
$archiveUrl = "$distBase/$archiveName"
$shasumsUrl = "$distBase/SHASUMS256.txt"

$tempRoot = Join-Path $env:TEMP "renderdoc-node-$Version-$platform"
New-Item -ItemType Directory -Force -Path $tempRoot | Out-Null

$zipPath = Join-Path $tempRoot $archiveName
$shaPath = Join-Path $tempRoot "SHASUMS256.txt"
$extractDir = Join-Path $tempRoot "extract"

Write-Host "Downloading $archiveUrl"
Invoke-WebRequest -UseBasicParsing -Uri $archiveUrl -OutFile $zipPath

Write-Host "Downloading $shasumsUrl"
Invoke-WebRequest -UseBasicParsing -Uri $shasumsUrl -OutFile $shaPath

$expected = (Select-String -Path $shaPath -Pattern ("\\s" + [Regex]::Escape($archiveName) + "$") | Select-Object -First 1).Line.Split(" ", [System.StringSplitOptions]::RemoveEmptyEntries)[0]
if ([string]::IsNullOrWhiteSpace($expected)) {
    throw "Failed to find expected SHA256 for $archiveName in SHASUMS256.txt"
}

$actual = (Get-FileHash -Algorithm SHA256 -Path $zipPath).Hash.ToLowerInvariant()
if ($expected.ToLowerInvariant() -ne $actual) {
    throw "SHA256 mismatch for $archiveName. expected=$expected actual=$actual"
}

if (Test-Path $extractDir) {
    Remove-Item -Recurse -Force $extractDir
}
New-Item -ItemType Directory -Force -Path $extractDir | Out-Null

Write-Host "Extracting archive"
Expand-Archive -Path $zipPath -DestinationPath $extractDir -Force

$rootDir = Join-Path $extractDir "node-v$Version-$platform"
$srcNodeExe = Join-Path $rootDir "node.exe"
if (!(Test-Path $srcNodeExe)) {
    throw "node.exe not found in extracted archive: $srcNodeExe"
}

New-Item -ItemType Directory -Force -Path $DestinationDir | Out-Null
Copy-Item -Force $srcNodeExe (Join-Path $DestinationDir "node.exe")

Write-Host "Node deployed to: $DestinationDir\\node.exe"

