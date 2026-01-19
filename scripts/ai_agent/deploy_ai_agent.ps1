# Deploy AI Agent runtime assets into a qrenderdoc output directory.
#
# This script copies:
# - Agent Host bundle: qrenderdoc/CodeBuddyAgentHost/dist/bundle/main.js -> <out>/ai/agent-host/main.js
# - CodeBuddy CLI JS:  node_modules/@tencent-ai/agent-sdk/cli/dist/codebuddy.js -> <out>/ai/agent-host/codebuddy.js
# - Wrapper command:   <out>/ai/agent-host/codebuddy.cmd
# - Optional Node:     <out>/ai/node/node.exe
#
# Notes:
# - This script does not fetch Node automatically. Use fetch_node.ps1 first, then pass -NodeDir.
# - All paths are relative to repo root unless -OutputDir is absolute.

param(
    [ValidateSet("Development", "Release")]
    [string]$Config = "Development",

    [ValidateSet("x64")]
    [string]$Platform = "x64",

    [string]$OutputDir = "",

    # Directory containing node.exe (e.g. extracted by fetch_node.ps1).
    [string]$NodeDir = ""
)

$ErrorActionPreference = "Stop"

function Repo-Root {
    return Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
}

$root = Repo-Root
Set-Location $root

if ([string]::IsNullOrWhiteSpace($OutputDir)) {
    $OutputDir = Join-Path $root "$Platform\\$Config"
}

$outAi = Join-Path $OutputDir "ai"
$outAgentHost = Join-Path $outAi "agent-host"
$outNode = Join-Path $outAi "node"

New-Item -ItemType Directory -Force -Path $outAgentHost | Out-Null
New-Item -ItemType Directory -Force -Path $outNode | Out-Null

$hostBundle = Join-Path $root "qrenderdoc\\CodeBuddyAgentHost\\dist\\bundle\\main.js"
if (!(Test-Path $hostBundle)) {
    throw "Agent Host bundle not found: $hostBundle. Run: cd qrenderdoc/CodeBuddyAgentHost; npm run build; npx --yes esbuild@0.20.2 src/main.ts --bundle --platform=node --format=cjs --target=node18 --outfile=dist/bundle/main.js"
}

Copy-Item -Force $hostBundle (Join-Path $outAgentHost "main.js")

$cliJsSource = Join-Path $root "qrenderdoc\\CodeBuddyAgentHost\\node_modules\\@tencent-ai\\agent-sdk\\cli\\dist\\codebuddy.js"
if (!(Test-Path $cliJsSource)) {
    throw "CodeBuddy CLI JS not found: $cliJsSource. Run: cd qrenderdoc/CodeBuddyAgentHost; npm install"
}

Copy-Item -Force $cliJsSource (Join-Path $outAgentHost "codebuddy.js")

# Write codebuddy.cmd wrapper (Windows) that uses the bundled Node runtime.
$cmdPath = Join-Path $outAgentHost "codebuddy.cmd"
$cmdText = @"
@echo off
setlocal

set SCRIPT_DIR=%~dp0
set NODE_EXE=%SCRIPT_DIR%..\node\node.exe
set CLI_JS=%SCRIPT_DIR%codebuddy.js

if not exist "%NODE_EXE%" (
  echo Bundled Node not found: "%NODE_EXE%" 1>&2
  exit /b 1
)

if not exist "%CLI_JS%" (
  echo CodeBuddy CLI JS not found: "%CLI_JS%" 1>&2
  exit /b 1
)

"%NODE_EXE%" "%CLI_JS%" %*
exit /b %errorlevel%
"@

[System.IO.File]::WriteAllText($cmdPath, $cmdText, [System.Text.Encoding]::ASCII)

# Optional: copy node.exe into output.
if (![string]::IsNullOrWhiteSpace($NodeDir)) {
    $srcNodeExe = Join-Path $NodeDir "node.exe"
    if (!(Test-Path $srcNodeExe)) {
        throw "node.exe not found in NodeDir: $NodeDir"
    }
    Copy-Item -Force $srcNodeExe (Join-Path $outNode "node.exe")
}

Write-Host "AI assets deployed:"
Write-Host "  Output:     $OutputDir"
Write-Host "  Agent Host: $outAgentHost"
Write-Host "  Node:       $outNode"
