@echo off
setlocal

set SCRIPT_DIR=%~dp0
set SDK_CLI=%SCRIPT_DIR%..\node_modules\@tencent-ai\agent-sdk\cli\dist\codebuddy.js

if not exist "%SDK_CLI%" (
  echo CodeBuddy SDK CLI not found: "%SDK_CLI%" 1>&2
  exit /b 1
)

node "%SDK_CLI%" %*
exit /b %errorlevel%

