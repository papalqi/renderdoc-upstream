@echo off
REM RenderDoc Development Build Script (x64)
REM Usage: build_dev_x64.bat

setlocal enabledelayedexpansion

echo ============================================
echo  RenderDoc Development Build (x64)
echo ============================================

REM Find MSBuild
set "MSBUILD="

REM Try VS2022
for %%p in (
    "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\MSBuild\Current\Bin\MSBuild.exe"
    "C:\Program Files\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe"
    "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe"
) do (
    if exist %%p (
        set "MSBUILD=%%p"
        goto :found
    )
)

REM Try VS2019
for %%p in (
    "C:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise\MSBuild\Current\Bin\MSBuild.exe"
    "C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\MSBuild\Current\Bin\MSBuild.exe"
    "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\MSBuild\Current\Bin\MSBuild.exe"
) do (
    if exist %%p (
        set "MSBUILD=%%p"
        goto :found
    )
)

REM Try vswhere
where vswhere >nul 2>&1
if %errorlevel% equ 0 (
    for /f "usebackq tokens=*" %%i in (`vswhere -latest -requires Microsoft.Component.MSBuild -find MSBuild\**\Bin\MSBuild.exe`) do (
        set "MSBUILD=%%i"
        goto :found
    )
)

echo [ERROR] MSBuild not found. Please install Visual Studio 2019 or later.
exit /b 1

:found
echo Found MSBuild: %MSBUILD%
echo.

cd /d "%~dp0.."

echo Building RenderDoc (Development, x64)...
echo.

%MSBUILD% renderdoc.sln /p:Configuration=Development /p:Platform=x64 /m /v:minimal

if %errorlevel% neq 0 (
    echo.
    echo [ERROR] Build failed!
    exit /b %errorlevel%
)

echo.
echo ============================================
echo  Build completed successfully!
echo  Output: x64\Development\
echo ============================================
