#!/usr/bin/env bash
# ============================================================
# 🍅 小菜花 一鍵啟動 v1.0 (Linux / macOS)
# 用法: curl -fsSL <URL>/xiaocaihua_start.sh | bash
#   或: chmod +x xiaocaihua_start.sh && ./xiaocaihua_start.sh
# ============================================================
set -euo pipefail

GITHUB_RAW="https://raw.githubusercontent.com/mokzee9z-alt/xiaocaihua/main"
BACKUP_ENC="hermes_backup.tar.gz.enc"
BACKUP_PASS="${BACKUP_PASS:-xiaocaihua-2026}"
HERMES_HOME="${HERMES_HOME:-$HOME/.hermes}"
TMP_DIR="$(mktemp -d)"
trap 'rm -rf "$TMP_DIR"' EXIT

echo "=========================================="
echo "   🍅 小菜花 一鍵啟動 v1.0 (Linux/macOS)"
echo "=========================================="
echo

echo "[1/5] 檢查 Hermes Agent..."
if command -v hermes >/dev/null 2>&1; then
  echo "      -> 已有: $(hermes --version 2>/dev/null | head -1)"
else
  echo "      -> 未安裝, 檢查 Python..."
  PY="$(command -v python3 || command -v python || true)"
  if [ -z "$PY" ]; then
    echo "      -> 安裝 Python..."
    if command -v apt-get >/dev/null 2>&1; then sudo apt-get update -qq && sudo apt-get install -y -qq python3 python3-pip >/dev/null
    elif command -v brew >/dev/null 2>&1; then brew install python
    elif command -v dnf >/dev/null 2>&1; then sudo dnf install -y -q python3 python3-pip >/dev/null
    else echo "❌ 請先安裝 Python 3"; exit 1; fi
    PY="$(command -v python3 || command -v python)"
  fi
  echo "      -> 安裝 Hermes Agent..."
  "$PY" -m pip install --quiet --break-system-packages hermes-agent 2>/dev/null || "$PY" -m pip install --quiet hermes-agent
  echo "      -> 安裝完成"
fi

echo "[2/5] 下載小菜花備份..."
curl -fsSL -o "$TMP_DIR/$BACKUP_ENC" "$GITHUB_RAW/$BACKUP_ENC"
echo "      -> $(du -h "$TMP_DIR/$BACKUP_ENC" | cut -f1)"

echo "[3/5] 解密..."
openssl enc -d -aes-256-cbc -pbkdf2 -pass pass:"$BACKUP_PASS" \
  -in "$TMP_DIR/$BACKUP_ENC" -out "$TMP_DIR/hermes_backup.tar.gz"
echo "      -> OK"

echo "[4/5] 還原到 $HERMES_HOME ..."
mkdir -p "$HERMES_HOME"
tar -xzf "$TMP_DIR/hermes_backup.tar.gz" -C "$HERMES_HOME"
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
  echo "   ✅ 小菜花就位! 記憶 ${MEM} bytes / 技能 ${SK} 個"
  echo "=========================================="
  echo
  echo "  啟動選項:"
  echo "    1. 直接對話:  hermes chat"
  echo "    2. 背景服務:  hermes gateway start"
  echo "    3. 關閉背景:  hermes gateway stop"
  echo
  echo "  直接開始對話..."
  hermes chat
else
  echo "❌ 還原失敗"
  exit 1
fi
