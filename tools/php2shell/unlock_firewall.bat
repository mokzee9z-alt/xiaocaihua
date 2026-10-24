@echo off
REM PHP2Shell Check - USB unlock helper
REM Adds Windows Firewall allow rules + Defender exclusion for this USB tool.
REM Requires administrator. Safe to run repeatedly. Use restore_firewall.bat to undo.
setlocal
cd /d "%~dp0"

net session >nul 2>&1
if %errorLevel% neq 0 (
    echo Requesting administrator rights...
    powershell -NoProfile -Command "Start-Process -FilePath '%~f0' -Verb RunAs"
    exit /b
)

echo ============================================================
echo  PHP2Shell Check - Unlock Firewall / Defender for this USB
echo ============================================================
echo.

set "EXE=%~dp0php2shell_check.exe"
set "CLI=%~dp0php2shell_check_cli.exe"
set "DIR=%~dp0"

echo [1/4] Adding Windows Firewall allow rules (outbound + inbound)...
netsh advfirewall firewall delete rule name="PHP2Shell-Check-Out" >nul 2>&1
netsh advfirewall firewall delete rule name="PHP2Shell-Check-In" >nul 2>&1
netsh advfirewall firewall add rule name="PHP2Shell-Check-Out" dir=out action=allow program="%EXE%" enable=yes profile=any >nul
if errorlevel 1 (echo    FAILED outbound rule) else (echo    OK outbound allow for php2shell_check.exe)
netsh advfirewall firewall add rule name="PHP2Shell-Check-In" dir=in action=allow program="%EXE%" enable=yes profile=any >nul
if errorlevel 1 (echo    FAILED inbound rule) else (echo    OK inbound allow for php2shell_check.exe)
if exist "%CLI%" (
    netsh advfirewall firewall delete rule name="PHP2Shell-Check-CLI-Out" >nul 2>&1
    netsh advfirewall firewall add rule name="PHP2Shell-Check-CLI-Out" dir=out action=allow program="%CLI%" enable=yes profile=any >nul
    if errorlevel 1 (echo    FAILED cli rule) else (echo    OK outbound allow for php2shell_check_cli.exe)
)

echo.
echo [2/4] Adding Defender exclusion for this USB folder...
powershell -NoProfile -Command "try { Add-MpPreference -ExclusionPath '%DIR%' -ErrorAction Stop; Write-Host '   OK Defender exclusion added' } catch { Write-Host '   SKIP Defender exclusion (not available or blocked)' }"

echo.
echo [3/4] Checking current firewall profile state...
netsh advfirewall show allprofiles state | findstr /i "Profile State"
echo.

echo [4/4] Verifying the tool can be reached by the firewall (rule listing)...
netsh advfirewall firewall show rule name="PHP2Shell-Check-Out" | findstr /i "Rule Name Enabled Direction Action Program"

echo.
echo ------------------------------------------------------------
echo  Done. You can close this window and run START.bat.
echo  To undo everything: run restore_firewall.bat
echo ------------------------------------------------------------
echo.
pause
endlocal
