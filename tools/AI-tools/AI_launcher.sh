#!/usr/bin/env bash
# 🤖 小菜花 AI 啟動中心 (Linux/macOS)
# 用法: bash AI_launcher.sh
set -euo pipefail

GITHUB_RAW="https://raw.githubusercontent.com/mokzee9z-alt/xiaocaihua/main"
GHOSTGPT_DIR="$HOME/ghostgpt"

menu() {
  clear
  echo "=========================================="
  echo "   🤖 小菜花 AI 啟動中心 v1.0"
  echo "=========================================="
  echo
  echo "  按鍵啟動:"
  echo "  [1] GhostGPT    — 駭客 AI 終端"
  echo "  [2] CODEX       — OpenAI 編碼 AI"
  echo "  [3] LOVABLE     — AI 網站/App 開發"
  echo "  [4] Claude Code — 法律/深思考 AI"
  echo "  [5] 安裝全部 AI 工具"
  echo "  [0] 離開"
  echo
  read -rp "  請按數字選擇: " ch
  case "$ch" in
    1) ghostgpt ;;
    2) codex ;;
    3) lovable ;;
    4) claude ;;
    5) install_all ;;
    0) exit 0 ;;
    *) menu ;;
  esac
}

ghostgpt() {
  echo
  echo "[*] 啟動 GhostGPT..."
  PY="$(command -v python3 || command -v python || true)"
  if [ -z "$PY" ]; then echo "❌ 需要 Python 3"; sleep 2; menu; return; fi
  if [ ! -f "$GHOSTGPT_DIR/ghostgpt.py" ]; then
    echo "[*] 第一次使用, 下載 GhostGPT..."
    mkdir -p "$GHOSTGPT_DIR"
    curl -fsSL -o "$GHOSTGPT_DIR/ghostgpt.py" "$GITHUB_RAW/tools/AI-tools/ghostgpt.py"
  fi
  if [ -z "${GHOSTGPT_KEY:-}" ]; then
    echo
    read -rp "  請輸入 DeepSeek API Key: " GHOSTGPT_KEY
  fi
  GHOSTGPT_KEY="$GHOSTGPT_KEY" "$PY" "$GHOSTGPT_DIR/ghostgpt.py"
  read -rp "  按 Enter 回選單..." _ || true
  menu
}

codex() {
  echo
  echo "[*] 啟動 OpenAI CODEX..."
  if ! command -v codex >/dev/null 2>&1; then
    echo "[*] 安裝 CODEX CLI..."
    if ! command -v npm >/dev/null 2>&1; then echo "❌ 需要 Node.js"; sleep 2; menu; return; fi
    npm install -g @openai/codex
  fi
  codex
  menu
}

lovable() {
  echo
  echo "[*] 開啟 LOVABLE (AI 開發平台)..."
  if command -v xdg-open >/dev/null 2>&1; then xdg-open https://lovable.dev
  elif command -v open >/dev/null 2>&1; then open https://lovable.dev
  else echo "請手動開啟: https://lovable.dev"; fi
  read -rp "  按 Enter 回選單..." _ || true
  menu
}

claude() {
  echo
  echo "[*] 啟動 Claude Code..."
  if ! command -v claude >/dev/null 2>&1; then
    echo "[*] 安裝 Claude Code..."
    if ! command -v npm >/dev/null 2>&1; then echo "❌ 需要 Node.js"; sleep 2; menu; return; fi
    npm install -g @anthropic-ai/claude-code
  fi
  claude
  menu
}

install_all() {
  echo
  echo "[*] 安裝全部 AI 工具..."
  if command -v npm >/dev/null 2>&1; then
    npm install -g @openai/codex @anthropic-ai/claude-code
  else
    echo "⚠️ 無 Node.js, 跳過 CODEX/Claude Code (需要: https://nodejs.org)"
  fi
  echo "[*] GhostGPT 下載中..."
  mkdir -p "$GHOSTGPT_DIR"
  curl -fsSL -o "$GHOSTGPT_DIR/ghostgpt.py" "$GITHUB_RAW/tools/AI-tools/ghostgpt.py"
  echo "✅ 全部安裝完成!"
  read -rp "  按 Enter 回選單..." _ || true
  menu
}

menu
