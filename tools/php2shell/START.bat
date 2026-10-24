@echo off
REM PHP2Shell Check - one click launcher
REM 1) raises admin  2) adds firewall allow + Defender exclusion  3) starts the GUI
REM Everything it changes can be undone with restore_firewall.bat
setlocal
cd /d "%~dp0"

set "EXE=%~dp0php2shell_check.exe"
set "STAMP=%~dp0.unlock_done"

if not exist "%EXE%" (
    echo ERROR: php2shell_check.exe not found next to this file.
    echo Keep START.bat in the same folder as the exe and try again.
    pause
    exit /b 1
)

net session >nul 2>&1
if %errorLevel% neq 0 (
    echo Asking for administrator rights ^(needed once to unblock the firewall^)...
    powershell -NoProfile -Command "Start-Process -FilePath '%~f0' -Verb RunAs"
    exit /b
)

if not exist "%STAMP%" (
    echo ============================================================
    echo  First run - unblocking firewall and Defender for this tool
    echo ============================================================
    echo.
    netsh advfirewall firewall delete rule name="PHP2Shell-Check-Out" >nul 2>&1
    netsh advfirewall firewall delete rule name="PHP2Shell-Check-In" >nul 2>&1
    netsh advfirewall firewall add rule name="PHP2Shell-Check-Out" dir=out action=allow program="%EXE%" enable=yes profile=any >nul
    if errorlevel 1 (echo  [x] outbound rule failed) else (echo  [OK] outbound allowed)
    netsh advfirewall firewall add rule name="PHP2Shell-Check-In" dir=in action=allow program="%EXE%" enable=yes profile=any >nul
    if errorlevel 1 (echo  [x] inbound rule failed) else (echo  [OK] inbound allowed)
    if exist "%~dp0php2shell_check_cli.exe" (
        netsh advfirewall firewall delete rule name="PHP2Shell-Check-CLI-Out" >nul 2>&1
        netsh advfirewall firewall add rule name="PHP2Shell-Check-CLI-Out" dir=out action=allow program="%~dp0php2shell_check_cli.exe" enable=yes profile=any >nul
        echo  [OK] cli allowed
    )
    powershell -NoProfile -Command "try { Add-MpPreference -ExclusionPath '%~dp0' -ErrorAction Stop; Write-Host '  [OK] Defender exclusion added' } catch { Write-Host '  [--] Defender exclusion skipped' }"
    echo done> "%STAMP%"
    echo.
    echo Unblock finished. Starting the tool...
    echo To undo later, run restore_firewall.bat
    echo.
    timeout /t 2 >nul
) else (
    echo Tool already unblocked, starting...
)

start "" "%EXE%"
exit /b 0
endlocal
