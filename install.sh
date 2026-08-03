#!/usr/bin/env bash
# ============================================================
# 小菜花 Hermes 一鍵還原安裝程式
# 用法: curl -fsSL <URL>/install.sh | bash
# 在任何新電腦執行, 自動下載並還原完整 Hermes 設定
# ============================================================
set -euo pipefail

GITHUB_RAW="https://raw.githubusercontent.com/ABEN-HERMES/xiaocaihua/main"
BACKUP_FILE="hermes_backup.tar.gz"

echo "🍅 小菜花還原程式 v1.0"
echo "=========================================="

# --- 偵測系統 ---
OS="$(uname -s)"
case "$OS" in
  Linux*)   HERMES_HOME="${HERMES_HOME:-$HOME/.hermes}";;
  Darwin*)  HERMES_HOME="${HERMES_HOME:-$HOME/.hermes}";;
  MINGW*|MSYS*|CYGWIN*) HERMES_HOME="${HERMES_HOME:-$USERPROFILE\\.hermes}"; echo "[!] Windows 請改用 install.ps1"; exit 1;;
  *) echo "[!] 不支援的系統: $OS"; exit 1;;
esac

echo "[1/4] 下載備份..."
TMPDIR_T=$(mktemp -d)
curl -fsSL -o "$TMPDIR_T/$BACKUP_FILE" "$GITHUB_RAW/$BACKUP_FILE"
echo "      -> $(du -h "$TMPDIR_T/$BACKUP_FILE" | cut -f1)"

echo "[2/4] 解壓縮..."
mkdir -p "$HERMES_HOME"
tar -xzf "$TMPDIR_T/$BACKUP_FILE" -C "$HERMES_HOME"
echo "      -> $HERMES_HOME"

echo "[3/4] 套用設定..."
# 從備份移動 hermes/ 內容到正確位置 (備份結構為 hermes/...)
if [ -d "$HERMES_HOME/hermes" ]; then
  shopt -s dotglob
  mv "$HERMES_HOME/hermes/"* "$HERMES_HOME/"
  rmdir "$HERMES_HOME/hermes" 2>/dev/null || true
  shopt -u dotglob
fi

# 確保權限
chmod 600 "$HERMES_HOME/.env" 2>/dev/null || true
chmod 644 "$HERMES_HOME/config.yaml" 2>/dev/null || true

echo "[4/4] 驗證..."
if [ -f "$HERMES_HOME/memories/MEMORY.md" ] && [ -f "$HERMES_HOME/config.yaml" ]; then
  echo "=========================================="
  echo "✅ 還原完成! 小菜花已就位"
  echo "   - 記憶: $(wc -c < "$HERMES_HOME/memories/MEMORY.md") bytes"
  echo "   - 技能: $(find "$HERMES_HOME/skills" -name SKILL.md 2>/dev/null | wc -l) 個"
  echo "   - 設定: config.yaml"
  echo ""
  echo "下一步: 安裝 Hermes 本體後啟動即可"
else
  echo "❌ 還原失敗, 檔案不完整"
  exit 1
fi
rm -rf "$TMPDIR_T"
