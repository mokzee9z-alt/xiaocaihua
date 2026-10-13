#!/usr/bin/env bash
# ============================================================
# 小菜花 一鍵啟動安裝程式 v4.0
#   git clone https://github.com/mokzee9z-alt/xiaocaihua && cd xiaocaihua && ./install.sh
#   （或）curl -fsSL https://raw.githubusercontent.com/mokzee9z-alt/xiaocaihua/main/install.sh | bash
#
# v4 重點：**Hermes 本體就在這個 repo 裡**（hermes_src.tar.gz），
#         不用另外 pip install hermes-agent，clone 完直接跑。
# 流程：解出 Hermes 原始碼 → 建 venv → 裝依賴 → 還原記憶/技能/設定 → 驗證
# ============================================================
set -euo pipefail

GITHUB_RAW="https://raw.githubusercontent.com/mokzee9z-alt/xiaocaihua/main"
SRC_TAR="hermes_src.tar.gz"
BACKUP_ENC="hermes_backup.tar.gz.enc"
BACKUP_PASS="${BACKUP_PASS:-xiaocaihua-2026}"

INSTALL_DIR="${INSTALL_DIR:-$HOME/.local/share/hermes-agent}"
HERMES_HOME="${HERMES_HOME:-$HOME/.hermes}"
BIN_DIR="${BIN_DIR:-$HOME/.local/bin}"

echo "🍅 小菜花 一鍵啟動 v4.0（Hermes 內含，免另外安裝）"
echo "=============================================="
echo "  Hermes 裝到 : $INSTALL_DIR"
echo "  設定/記憶在 : $HERMES_HOME"
echo ""

WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT

# 找這支腳本所在的目錄（在 clone 目錄裡跑就用本地檔，不然就上網抓）
SCRIPT_DIR=""
if [ -n "${BASH_SOURCE[0]:-}" ] && [ -f "${BASH_SOURCE[0]}" ]; then
  SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
fi

need() {  # need <檔名> → 確保 $WORK/<檔名> 存在
  [ -f "$WORK/$1" ] && return 0
  if [ -n "$SCRIPT_DIR" ] && [ -f "$SCRIPT_DIR/$1" ]; then
    cp "$SCRIPT_DIR/$1" "$WORK/$1"
  else
    echo "  → 下載 $1"
    curl -fsSL -o "$WORK/$1" "$GITHUB_RAW/$1"
  fi
}

# ── 1. Python
echo "[1/6] 檢查 Python..."
if command -v python3 >/dev/null 2>&1; then PY=python3
elif command -v python >/dev/null 2>&1; then PY=python
else
  echo "      → 沒有 Python，嘗試安裝"
  if command -v apt-get >/dev/null 2>&1; then sudo apt-get update -qq && sudo apt-get install -y -qq python3 python3-venv curl >/dev/null
  elif command -v brew >/dev/null 2>&1; then brew install python
  else echo "❌ 請先自己裝 Python 3.10+"; exit 1; fi
  PY=python3
fi
echo "      -> $($PY --version 2>&1)"

# ── 2. 解出內附的 Hermes 原始碼
echo "[2/6] 解出內附的 Hermes（$SRC_TAR）..."
need "$SRC_TAR"
mkdir -p "$INSTALL_DIR"
tar -xzf "$WORK/$SRC_TAR" -C "$INSTALL_DIR"
echo "      -> 解到 $INSTALL_DIR"

# ── 3. venv + 依賴
echo "[3/6] 建虛擬環境並安裝依賴（第一次約 2–5 分鐘）..."
cd "$INSTALL_DIR"
if command -v uv >/dev/null 2>&1; then
  uv venv --python 3.11 .venv >/dev/null 2>&1 || uv venv .venv >/dev/null 2>&1 || true
fi
if [ ! -x "$INSTALL_DIR/.venv/bin/python" ]; then
  "$PY" -m venv .venv
fi
VPY="$INSTALL_DIR/.venv/bin/python"
if command -v uv >/dev/null 2>&1; then
  uv pip install -q -e . || "$VPY" -m pip install -e .
else
  "$VPY" -m pip install -q --upgrade pip
  "$VPY" -m pip install -q -e .
fi
echo "      -> OK"

# ── 4. 放一個 hermes 指令
echo "[4/6] 建立 hermes 指令..."
mkdir -p "$BIN_DIR"
ln -sf "$INSTALL_DIR/.venv/bin/hermes" "$BIN_DIR/hermes"
case ":$PATH:" in *":$BIN_DIR:"*) ;; *) echo "      ! 提醒：$BIN_DIR 不在 PATH，用之前先 export PATH=\"\$HOME/.local/bin:\$PATH\"" ;; esac

# ── 5. 還原記憶 / 技能 / 設定
echo "[5/6] 還原小菜花（解密備份）..."
need "$BACKUP_ENC"
openssl enc -d -aes-256-cbc -pbkdf2 -pass pass:"$BACKUP_PASS" -in "$WORK/$BACKUP_ENC" -out "$WORK/backup.tar.gz"
mkdir -p "$HERMES_HOME"
tar -xzf "$WORK/backup.tar.gz" -C "$HERMES_HOME"
if [ -d "$HERMES_HOME/hermes" ]; then
  shopt -s dotglob; mv "$HERMES_HOME/hermes/"* "$HERMES_HOME/"; rmdir "$HERMES_HOME/hermes" 2>/dev/null || true; shopt -u dotglob
fi
chmod 600 "$HERMES_HOME/.env" 2>/dev/null || true
chmod 644 "$HERMES_HOME/config.yaml" 2>/dev/null || true

# ── 6. 驗證
echo "[6/6] 驗證..."
VER="$("$INSTALL_DIR/.venv/bin/hermes" --version 2>&1 | head -1 || true)"
MEM=$([ -f "$HERMES_HOME/memories/MEMORY.md" ] && wc -c < "$HERMES_HOME/memories/MEMORY.md" || echo 0)
SK=$([ -d "$HERMES_HOME/skills" ] && find "$HERMES_HOME/skills" -name SKILL.md | wc -l || echo 0)
echo "=============================================="
if [ "$MEM" -gt 500 ] && [ "$SK" -gt 10 ]; then
  echo "✅ 好了：$VER"
  echo "   記憶 ${MEM} bytes ／ 技能 ${SK} 個 ／ 設定在 $HERMES_HOME"
  echo ""
  echo "開始用："
  echo "  export PATH=\"\$HOME/.local/bin:\$PATH\""
  echo "  hermes gateway start     # 背景服務（Telegram / LINE / Email）"
  echo "  hermes chat              # 直接在終端機對話"
else
  echo "❌ 還原不完整（記憶 ${MEM} bytes／技能 ${SK}）"; exit 1
fi
echo "=============================================="
