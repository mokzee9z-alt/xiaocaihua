# 🍅 小菜花 可攜版 — clone 完就能跑（Hermes 已內含）

這個 repo 就是**整包小菜花**：Hermes Agent 本體、記憶、技能、人格、設定，全都在裡面。
用任何一台新電腦（Linux / macOS / Windows），**clone 下來跑一行就開始用，不用另外安裝 Hermes**。

> 打包時間：**2026-10-13**
> 內容：Hermes Agent v0.21.2（`hermes_src.tar.gz`，42 MB）＋ 記憶 260 行 ＋ 技能 134 個／3,134 檔 ＋ 人格／設定／cron（`hermes_backup.tar.gz.enc`，6 MB，AES-256 加密）

## 📦 最快的用法

### Linux / macOS
```bash
git clone https://github.com/mokzee9z-alt/xiaocaihua && cd xiaocaihua && ./install.sh
```
一行版（不用先 clone）：
```bash
curl -fsSL https://raw.githubusercontent.com/mokzee9z-alt/xiaocaihua/main/xiaocaihua_start.sh | bash
```
（想直接開背景服務：加 `--gateway`）

### Windows
```powershell
irm https://raw.githubusercontent.com/mokzee9z-alt/xiaocaihua/main/install.ps1 | iex
```
或雙擊 `xiaocaihua_start.bat`（需先裝 Python，安裝時勾 Add to PATH）

## 🔧 install.sh 幫你做的事

1. 檢查 Python（沒有就試著裝）
2. **解出 repo 內附的 Hermes 原始碼** → `~/.local/share/hermes-agent`
3. 建 venv ＋ 安裝依賴
4. 產生 `~/.local/bin/hermes` 指令
5. 解密備份 → 還原記憶／技能／設定到 `~/.hermes`
6. 驗證：印出 Hermes 版本、記憶 bytes、技能數量

裝完：
```bash
export PATH="$HOME/.local/bin:$PATH"
hermes gateway start     # 背景服務（Telegram / LINE / Email）
hermes chat              # 終端機直接對話
```

## 🗂 repo 內容

| 檔案 | 說明 |
|------|------|
| `hermes_src.tar.gz` | **Hermes Agent 本體原始碼**（v0.21.2，已去掉 venv／node_modules／測試） |
| `hermes_backup.tar.gz.enc` | 小菜花的記憶、技能、人格（prefill.json）、config、cron — AES-256 加密 |
| `install.sh` / `install.ps1` | 一鍵安裝（Linux/macOS ／ Windows） |
| `xiaocaihua_start.sh` / `.bat` | 更短的一行版入口 |
| `tools/` | AI 啟動中心、分類工具庫、交叉編譯的 Windows EXE |

- 備份密碼：`xiaocaihua-2026`（可用環境變數 `BACKUP_PASS` 覆寫）
- 要換安裝位置：`INSTALL_DIR=... HERMES_HOME=... ./install.sh`

## 🛠 維護（更新這個 repo）

在 VPS 上：
```bash
bash /root/work/rebuild_github_backup.sh    # 重打包記憶/技能/設定 → 加密
tar czf hermes_src.tar.gz -C /usr/local/lib/hermes-agent \
  --exclude=venv --exclude=.git --exclude=node_modules --exclude=tests --exclude=website .
cd /tmp/hermes_portable && git add -A && git commit -m "更新" 
T=$(gh auth token) && git push "https://x-access-token:${T}@github.com/mokzee9z-alt/xiaocaihua.git" main
```
