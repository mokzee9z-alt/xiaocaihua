@echo off
REM PHP2Shell Check - undo everything unlock_firewall.bat changed.
REM Removes the firewall rules and the Defender exclusion. Nothing else is touched.
setlocal
cd /d "%~dp0"

net session >nul 2>&1
if %errorLevel% neq 0 (
    echo Requesting administrator rights...
    powershell -NoProfile -Command "Start-Process -FilePath '%~f0' -Verb RunAs"
    exit /b
)

echo ============================================================
echo  PHP2Shell Check - Restore / Undo
echo ============================================================
echo.

set "DIR=%~dp0"

echo [1/3] Removing firewall rules...
for %%R in ("PHP2Shell-Check-Out" "PHP2Shell-Check-In" "PHP2Shell-Check-CLI-Out") do (
    netsh advfirewall firewall delete rule name=%%R >nul 2>&1
    if errorlevel 1 (echo    not present: %%R) else (echo    removed: %%R)
)

echo.
echo [2/3] Removing Defender exclusion...
powershell -NoProfile -Command "try { Remove-MpPreference -ExclusionPath '%DIR%' -ErrorAction Stop; Write-Host '   OK Defender exclusion removed' } catch { Write-Host '   SKIP (not present or blocked)' }"

echo.
echo [3/3] Confirming rules are gone...
netsh advfirewall firewall show rule name="PHP2Shell-Check-Out" >nul 2>&1
if errorlevel 1 (echo    OK rule no longer exists) else (echo    WARNING rule still exists)

echo.
echo ------------------------------------------------------------
echo  Restored. Firewall and Defender are back to how they were.
echo ------------------------------------------------------------
echo.
pause
endlocal
