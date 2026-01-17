@echo off
REM RenderDoc Full Build Script (All Configurations)
REM Usage: build_all.bat

setlocal enabledelayedexpansion

echo ============================================
echo  RenderDoc Full Build (All Configurations)
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

set BUILD_ERROR=0

echo [1/2] Building Development x64...
%MSBUILD% renderdoc.sln /p:Configuration=Development /p:Platform=x64 /m /v:minimal
if %errorlevel% neq 0 set BUILD_ERROR=1

echo.
echo [2/2] Building Release x64...
%MSBUILD% renderdoc.sln /p:Configuration=Release /p:Platform=x64 /m /v:minimal
if %errorlevel% neq 0 set BUILD_ERROR=1

echo.
if %BUILD_ERROR% equ 0 (
    echo ============================================
    echo  All builds completed successfully!
    echo  Output:
    echo    - x64\Development\
    echo    - x64\Release\
    echo ============================================
) else (
    echo ============================================
    echo  Some builds failed. Check output above.
    echo ============================================
    exit /b 1
)
