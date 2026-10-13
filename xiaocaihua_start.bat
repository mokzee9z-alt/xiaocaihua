@echo off
chcp 65001 >nul
title 小菜花 一鍵啟動 (Windows)
echo ==========================================
echo    小菜花 一鍵啟動 v4 (Windows)
echo    Hermes 本體已內含，不用另外安裝
echo ==========================================
echo.

where powershell >nul 2>nul
if %errorlevel% neq 0 (
    echo [!] 找不到 PowerShell，請用 Windows 10 以上版本
    pause
    exit /b 1
)

echo 開始安裝（第一次約 2-5 分鐘）...
echo.
powershell -NoProfile -ExecutionPolicy Bypass -Command "irm https://raw.githubusercontent.com/mokzee9z-alt/xiaocaihua/main/install.ps1 | iex"

echo.
echo ==========================================
echo 完成。指令在: %USERPROFILE%\.local\share\hermes-agent\.venv\Scripts\hermes.exe
echo   hermes gateway start    ^<- 背景服務
echo   hermes chat             ^<- 直接對話
echo ==========================================
pause
