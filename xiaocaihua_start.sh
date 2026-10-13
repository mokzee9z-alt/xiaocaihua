#!/usr/bin/env bash
# ============================================================
# 🍅 小菜花 一鍵啟動 (Linux / macOS) v4
# 用法: curl -fsSL https://raw.githubusercontent.com/mokzee9z-alt/xiaocaihua/main/xiaocaihua_start.sh | bash
#   或: git clone https://github.com/mokzee9z-alt/xiaocaihua && cd xiaocaihua && ./install.sh
#
# v4：Hermes 本體已內含在 repo（hermes_src.tar.gz），不用另外安裝。
# ============================================================
set -euo pipefail

GITHUB_RAW="https://raw.githubusercontent.com/mokzee9z-alt/xiaocaihua/main"
INSTALL_DIR="${INSTALL_DIR:-$HOME/.local/share/hermes-agent}"
HERMES_BIN="$INSTALL_DIR/.venv/bin/hermes"

echo "=========================================="
echo "   🍅 小菜花 一鍵啟動 v4 (Linux/macOS)"
echo "=========================================="
echo

TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

echo "[1/2] 取得 Hermes 原始碼 + 備份..."
for f in hermes_src.tar.gz hermes_backup.tar.gz.enc install.sh; do
  echo "      → $f"
  curl -fsSL -o "$TMP/$f" "$GITHUB_RAW/$f"
done

echo "[2/2] 安裝（Hermes + 記憶/技能/設定）..."
bash "$TMP/install.sh"

export PATH="$HOME/.local/bin:$PATH"
echo
if [ "${1:-}" = "--gateway" ]; then
  echo "啟動背景服務（Telegram / LINE / Email）..."
  exec "$HERMES_BIN" gateway start
else
  echo "開始對話（要背景服務請用 ./xiaocaihua_start.sh --gateway）..."
  exec "$HERMES_BIN" chat
fi
