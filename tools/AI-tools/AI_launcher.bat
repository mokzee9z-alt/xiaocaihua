@echo off
chcp 65001 >nul
title 🤖 小菜花 AI 啟動中心 (Windows)
:menu
cls
echo ==========================================
echo    🤖 小菜花 AI 啟動中心 v1.0
echo ==========================================
echo.
echo    按鍵啟動:
echo    [1] GhostGPT  — 駭客 AI 終端
echo    [2] CODEX     — OpenAI 編碼 AI
echo    [3] LOVABLE   — AI 網站/App 開發
echo    [4] Claude Code — 法律/深思考 AI
echo    [5] 安裝全部 AI 工具
echo    [0] 離開
echo.
set /p ch=請按數字選擇: 

if "%ch%"=="1" goto ghostgpt
if "%ch%"=="2" goto codex
if "%ch%"=="3" goto lovable
if "%ch%"=="4" goto claude
if "%ch%"=="5" goto install_all
if "%ch%"=="0" exit /b 0
goto menu

:ghostgpt
echo.
echo [*] 啟動 GhostGPT...
where python >nul 2>nul
if %errorlevel% neq 0 (echo ❌ 需要 Python & pause & goto menu)
if not exist "%USERPROFILE%\ghostgpt\ghostgpt.py" (
    echo [*] 第一次使用, 下載 GhostGPT...
    curl -fsSL -o "%USERPROFILE%\ghostgpt\ghostgpt.py" "https://raw.githubusercontent.com/mokzee9z-alt/xiaocaihua/main/tools/AI-tools/ghostgpt.py"
    mkdir "%USERPROFILE%\ghostgpt" >nul 2>nul
)
if "%GHOSTGPT_KEY%"=="" (
    echo.
    echo    需要 API 金鑰才能啟動 GhostGPT
    set /p GHOSTGPT_KEY=請輸入 DeepSeek API Key: 
)
python "%USERPROFILE%\ghostgpt\ghostgpt.py"
pause
goto menu

:codex
echo.
echo [*] 啟動 OpenAI CODEX...
where codex >nul 2>nul
if %errorlevel% neq 0 (
    echo [*] 安裝 CODEX CLI...
    where npm >nul 2>nul
    if %errorlevel% neq 0 (echo ❌ 需要 Node.js & pause & goto menu)
    npm install -g @openai/codex
)
codex
pause
goto menu

:lovable
echo.
echo [*] 開啟 LOVABLE (AI 開發平台)...
start https://lovable.dev
pause
goto menu

:claude
echo.
echo [*] 啟動 Claude Code...
where claude >nul 2>nul
if %errorlevel% neq 0 (
    echo [*] 安裝 Claude Code...
    where npm >nul 2>nul
    if %errorlevel% neq 0 (echo ❌ 需要 Node.js & pause & goto menu)
    npm install -g @anthropic-ai/claude-code
)
claude
pause
goto menu

:install_all
echo.
echo [*] 安裝全部 AI 工具...
where npm >nul 2>nul
if %errorlevel% neq 0 (echo ❌ 需要 Node.js: https://nodejs.org & pause & goto menu)
npm install -g @openai/codex @anthropic-ai/claude-code
echo [*] GhostGPT 下載中...
mkdir "%USERPROFILE%\ghostgpt" >nul 2>nul
curl -fsSL -o "%USERPROFILE%\ghostgpt\ghostgpt.py" "https://raw.githubusercontent.com/mokzee9z-alt/xiaocaihua/main/tools/AI-tools/ghostgpt.py"
echo ✅ 全部安裝完成!
pause
goto menu
