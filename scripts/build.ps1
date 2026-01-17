# RenderDoc Build Script (PowerShell)
# Usage:
#   .\build.ps1                    # Build Development x64
#   .\build.ps1 -Config Release    # Build Release x64
#   .\build.ps1 -Clean             # Clean
#   .\build.ps1 -Rebuild           # Full rebuild
#   .\build.ps1 -Verbose           # Show detailed output

param(
    [ValidateSet("Development", "Release")]
    [string]$Config = "Development",
    
    [ValidateSet("x64")]
    [string]$Platform = "x64",
    
    [switch]$Clean,
    [switch]$Rebuild,
    [switch]$Verbose
)

$ErrorActionPreference = "Stop"

Write-Host "============================================" -ForegroundColor Cyan
Write-Host " RenderDoc Build Script" -ForegroundColor Cyan
Write-Host "============================================" -ForegroundColor Cyan
Write-Host ""

# Find MSBuild
function Find-MSBuild {
    # Try vswhere first (most reliable)
    $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    if (Test-Path $vswhere) {
        $msbuild = & $vswhere -latest -requires Microsoft.Component.MSBuild -find MSBuild\**\Bin\MSBuild.exe | Select-Object -First 1
        if ($msbuild -and (Test-Path $msbuild)) {
            return $msbuild
        }
    }
    
    # Try known paths
    $paths = @(
        "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\MSBuild\Current\Bin\MSBuild.exe",
        "C:\Program Files\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe",
        "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe",
        "C:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise\MSBuild\Current\Bin\MSBuild.exe",
        "C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\MSBuild\Current\Bin\MSBuild.exe",
        "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\MSBuild\Current\Bin\MSBuild.exe"
    )
    
    foreach ($path in $paths) {
        if (Test-Path $path) {
            return $path
        }
    }
    
    throw "MSBuild not found. Please install Visual Studio 2019 or later."
}

$msbuild = Find-MSBuild
Write-Host "Using MSBuild: $msbuild" -ForegroundColor Green
Write-Host ""

# Change to project root
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$projectRoot = Split-Path -Parent $scriptDir
Set-Location $projectRoot

$solutionFile = "renderdoc.sln"

# Determine build target
$target = "Build"
if ($Clean) {
    $target = "Clean"
    Write-Host "Cleaning $Config $Platform..." -ForegroundColor Yellow
} elseif ($Rebuild) {
    $target = "Rebuild"
    Write-Host "Rebuilding $Config $Platform..." -ForegroundColor Yellow
} else {
    Write-Host "Building $Config $Platform..." -ForegroundColor Yellow
}
Write-Host ""

# Run MSBuild
$verbosity = if ($Verbose) { "normal" } else { "minimal" }
$args = @(
    $solutionFile,
    "/t:$target",
    "/p:Configuration=$Config",
    "/p:Platform=$Platform",
    "/m",
    "/v:$verbosity"
)

& $msbuild @args

if ($LASTEXITCODE -ne 0) {
    Write-Host ""
    Write-Host "[ERROR] Build failed!" -ForegroundColor Red
    exit $LASTEXITCODE
}

Write-Host ""
Write-Host "============================================" -ForegroundColor Green
Write-Host " Build completed successfully!" -ForegroundColor Green
Write-Host " Output: $Platform\$Config\" -ForegroundColor Green
Write-Host "============================================" -ForegroundColor Green
