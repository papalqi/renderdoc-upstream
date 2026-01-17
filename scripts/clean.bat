@echo off
REM RenderDoc Clean Script
REM Usage: clean.bat

setlocal enabledelayedexpansion

echo ============================================
echo  RenderDoc Clean Build Artifacts
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

echo Cleaning all build artifacts...
echo.

%MSBUILD% renderdoc.sln /t:Clean /p:Configuration=Development /p:Platform=x64 /v:minimal
%MSBUILD% renderdoc.sln /t:Clean /p:Configuration=Release /p:Platform=x64 /v:minimal

echo.
echo ============================================
echo  Clean completed!
echo ============================================
