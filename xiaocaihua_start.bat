@echo off
chcp 65001 >nul
title 🍅 小菜花 一鍵啟動 (Windows)
echo ==========================================
echo    🍅 小菜花 一鍵啟動 v1.0 (Windows)
echo ==========================================
echo.

set "GITHUB_RAW=https://raw.githubusercontent.com/mokzee9z-alt/xiaocaihua/main"
set "BACKUP_ENC=hermes_backup.tar.gz.enc"
set "BACKUP_PASS=xiaocaihua-2026"
set "HERMES_HOME=%USERPROFILE%\.hermes"
set "TMP_DIR=%TEMP%\xiaocaihua_start"

echo [1/5] 檢查 Python...
where python >nul 2>nul
if %errorlevel%==0 (
    echo       -^> Python 已安裝
) else (
    echo       -^> 未安裝 Python, 請先安裝: https://www.python.org/downloads/
    echo       安裝後勾選 "Add Python to PATH" 再執行本檔
    pause
    exit /b 1
)

echo [2/5] 檢查 Hermes Agent...
where hermes >nul 2>nul
if %errorlevel%==0 (
    echo       -^> Hermes 已安裝
) else (
    echo       -^> 安裝 Hermes Agent...
    python -m pip install --quiet hermes-agent
    if %errorlevel%==0 (echo       -^> 安裝完成) else (echo       ❌ 安裝失敗 & pause & exit /b 1)
)

echo [3/5] 下載小菜花備份...
if exist "%TMP_DIR%" rmdir /s /q "%TMP_DIR%"
mkdir "%TMP_DIR%"
curl -fsSL -o "%TMP_DIR%\%BACKUP_ENC%" "%GITHUB_RAW%/%BACKUP_ENC%"
if %errorlevel%==0 (echo       -^> 下載完成) else (echo       ❌ 下載失敗 & pause & exit /b 1)

echo [4/5] 解密 + 還原...
powershell -NoProfile -ExecutionPolicy Bypass -Command ^
  "$enc='%TMP_DIR%\%BACKUP_ENC%'; $out='%TMP_DIR%\hermes_backup.tar.gz';" ^
  "$pass='%BACKUP_PASS%'; $b=[IO.File]::ReadAllBytes($enc);" ^
  "if($b.Length -lt 16 -or [Text.Encoding]::ASCII.GetString($b,0,8) -ne 'Salted__'){Write-Host '❌ 格式錯誤'; exit 1};" ^
  "$salt=$b[8..15]; $ct=$b[16..($b.Length-1)];" ^
  "$pbkdf=New-Object Security.Cryptography.Rfc2898DeriveBytes($pass,$salt,10000,[Security.Cryptography.HashAlgorithmName]::SHA256);" ^
  "$key=$pbkdf.GetBytes(32); $iv=$pbkdf.GetBytes(16);" ^
  "$aes=[Security.Cryptography.Aes]::Create(); $aes.Key=$key; $aes.IV=$iv; $aes.Mode=[Security.Cryptography.CipherMode]::CBC; $aes.Padding=[Security.Cryptography.PaddingMode]::PKCS7;" ^
  "$dec=$aes.CreateDecryptor(); $ms=New-Object IO.MemoryStream; $cs=New-Object Security.Cryptography.CryptoStream($ms,$dec,[Security.Cryptography.CryptoStreamMode]::Write);" ^
  "$cs.Write($ct,0,$ct.Length); $cs.FlushFinalBlock(); [IO.File]::WriteAllBytes($out,$ms.ToArray()); Write-Host '       -^> 解密完成'"
if %errorlevel% neq 0 (echo       ❌ 解密失敗 & pause & exit /b 1)

mkdir "%HERMES_HOME%" >nul 2>nul
tar -xzf "%TMP_DIR%\hermes_backup.tar.gz" -C "%HERMES_HOME%"
if exist "%HERMES_HOME%\hermes\" (
    xcopy /E /Y /Q "%HERMES_HOME%\hermes\*" "%HERMES_HOME%\" >nul
    rmdir /s /q "%HERMES_HOME%\hermes"
)

echo [5/5] 驗證...
if exist "%HERMES_HOME%\memories\MEMORY.md" (
    for %%F in ("%HERMES_HOME%\memories\MEMORY.md") do set MEMSIZE=%%~zF
    echo ==========================================
    echo    ✅ 小菜花就位! 記憶 %MEMSIZE% bytes
    echo ==========================================
    echo.
    echo    啟動選項:
    echo      1. 直接對話:  hermes chat
    echo      2. 背景服務:  hermes gateway start
    echo      3. 關閉背景:  hermes gateway stop
    echo.
    rmdir /s /q "%TMP_DIR%"
    echo    按任一鍵開始對話...
    pause >nul
    hermes chat
) else (
    echo       ❌ 還原失敗, 檔案不完整
    pause
    exit /b 1
)
