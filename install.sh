#!/usr/bin/env bash
# ============================================================
# 小菜花 一鍵啟動安裝程式 v3.0
# 任何新電腦: curl -fsSL <URL>/install.sh | bash
# 自動完成: 安裝 Hermes → 還原記憶/技能 → 啟動 gateway
# ============================================================
set -euo pipefail

GITHUB_RAW="https://raw.githubusercontent.com/mokzee9z-alt/xiaocaihua/main"
BACKUP_ENC="hermes_backup.tar.gz.enc"
BACKUP_PASS="${BACKUP_PASS:-xiaocaihua-2026}"

echo "🍅 小菜花 一鍵啟動 v3.0"
echo "=========================================="

OS="$(uname -s)"
case "$OS" in
  Linux*)   HERMES_HOME="${HERMES_HOME:-$HOME/.hermes}";;
  Darwin*)  HERMES_HOME="${HERMES_HOME:-$HOME/.hermes}";;
  MINGW*|MSYS*|CYGWIN*) echo "[!] Windows 請改用 install.ps1"; exit 1;;
  *) echo "[!] 不支援的系統: $OS"; exit 1;;
esac

echo "[1/5] 安裝 Hermes Agent..."
if command -v hermes >/dev/null 2>&1; then
  echo "      -> 已有 Hermes: $(hermes --version 2>/dev/null | head -1)"
else
  if ! command -v python3 >/dev/null 2>&1 && ! command -v python >/dev/null 2>&1; then
    echo "      -> 安裝 Python..."
    if command -v apt-get >/dev/null 2>&1; then sudo apt-get update -qq && sudo apt-get install -y -qq python3 python3-pip >/dev/null
    elif command -v brew >/dev/null 2>&1; then brew install python
    else echo "❌ 請先安裝 Python 3"; exit 1; fi
  fi
  PY="$(command -v python3 || command -v python)"
  "$PY" -m pip install --quiet --break-system-packages hermes-agent 2>/dev/null || "$PY" -m pip install --quiet hermes-agent
  echo "      -> Hermes 安裝完成"
fi

echo "[2/5] 下載小菜花備份 (加密)..."
TMPDIR_T="$(mktemp -d)"
curl -fsSL -o "$TMPDIR_T/$BACKUP_ENC" "$GITHUB_RAW/$BACKUP_ENC"
echo "      -> $(du -h "$TMPDIR_T/$BACKUP_ENC" | cut -f1)"

echo "[3/5] 解密..."
openssl enc -d -aes-256-cbc -pbkdf2 -pass pass:"$BACKUP_PASS" \
  -in "$TMPDIR_T/$BACKUP_ENC" -out "$TMPDIR_T/hermes_backup.tar.gz"
echo "      -> OK"

echo "[4/5] 還原到 $HERMES_HOME ..."
mkdir -p "$HERMES_HOME"
tar -xzf "$TMPDIR_T/hermes_backup.tar.gz" -C "$HERMES_HOME"
if [ -d "$HERMES_HOME/hermes" ]; then
  shopt -s dotglob
  mv "$HERMES_HOME/hermes/"* "$HERMES_HOME/"
  rmdir "$HERMES_HOME/hermes" 2>/dev/null || true
  shopt -u dotglob
fi
chmod 600 "$HERMES_HOME/.env" 2>/dev/null || true
chmod 644 "$HERMES_HOME/config.yaml" 2>/dev/null || true

echo "[5/5] 驗證..."
if [ -f "$HERMES_HOME/memories/MEMORY.md" ] && [ -f "$HERMES_HOME/config.yaml" ]; then
  MEM="$(wc -c < "$HERMES_HOME/memories/MEMORY.md")"
  SK="$(find "$HERMES_HOME/skills" -name SKILL.md 2>/dev/null | wc -l)"
  echo "=========================================="
  echo "✅ 小菜花就位! 記憶 ${MEM} bytes / 技能 ${SK} 個"
  echo ""
  echo "啟動方式:"
  echo "  hermes gateway start    # 背景服務 (Telegram/LINE)"
  echo "  hermes chat             # 直接對話"
  echo "=========================================="
else
  echo "❌ 還原失敗"
  exit 1
fi
rm -rf "$TMPDIR_T"
