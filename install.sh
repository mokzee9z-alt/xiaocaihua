#!/usr/bin/env bash
# ============================================================
# 小菜花 Hermes 一鍵還原安裝程式 v2.0 (加密版)
# 用法: curl -fsSL <URL>/install.sh | bash
# 任何新電腦執行, 自動下載解密並還原完整 Hermes 設定
# ============================================================
set -euo pipefail

GITHUB_RAW="https://raw.githubusercontent.com/mokzee9z-alt/xiaocaihua/main"
BACKUP_ENC="hermes_backup.tar.gz.enc"
BACKUP_PASS="${BACKUP_PASS:-xiaocaihua-2026}"

echo "🍅 小菜花還原程式 v2.0"
echo "=========================================="

OS="$(uname -s)"
case "$OS" in
  Linux*)   HERMES_HOME="${HERMES_HOME:-$HOME/.hermes}";;
  Darwin*)  HERMES_HOME="${HERMES_HOME:-$HOME/.hermes}";;
  MINGW*|MSYS*|CYGWIN*) echo "[!] Windows 請改用 install.ps1"; exit 1;;
  *) echo "[!] 不支援的系統: $OS"; exit 1;;
esac

echo "[1/4] 下載備份 (加密)..."
TMPDIR_T=$(mktemp -d)
curl -fsSL -o "$TMPDIR_T/$BACKUP_ENC" "$GITHUB_RAW/$BACKUP_ENC"
echo "      -> $(du -h "$TMPDIR_T/$BACKUP_ENC" | cut -f1)"

echo "[2/4] 解密..."
openssl enc -d -aes-256-cbc -pbkdf2 -pass pass:"$BACKUP_PASS" \
  -in "$TMPDIR_T/$BACKUP_ENC" -out "$TMPDIR_T/hermes_backup.tar.gz"
echo "      -> OK"

echo "[3/4] 解壓縮 + 套用..."
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

echo "[4/4] 驗證..."
if [ -f "$HERMES_HOME/memories/MEMORY.md" ] && [ -f "$HERMES_HOME/config.yaml" ]; then
  echo "=========================================="
  echo "✅ 還原完成! 小菜花已就位"
  echo "   - 記憶: $(wc -c < "$HERMES_HOME/memories/MEMORY.md") bytes"
  echo "   - 技能: $(find "$HERMES_HOME/skills" -name SKILL.md 2>/dev/null | wc -l) 個"
  echo "   - 設定: config.yaml"
else
  echo "❌ 還原失敗, 檔案不完整"
  exit 1
fi
rm -rf "$TMPDIR_T"
