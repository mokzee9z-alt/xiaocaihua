# 🍅 小菜花可攜版 — Hermes 一鍵啟動 (GitHub)

任何電腦（Windows / Linux / macOS）一行指令，直接啟動完整的小菜花。
**所有平台共用同一份資源**（記憶、技能、設定），永遠同步最新版。

## 📦 一鍵啟動

### Windows（雙擊 .bat 或貼上指令）
```bat
curl -fsSL -o xiaocaihua_start.bat https://raw.githubusercontent.com/mokzee9z-alt/xiaocaihua/main/xiaocaihua_start.bat && xiaocaihua_start.bat
```
或下載後**雙擊** `xiaocaihua_start.bat`（需先裝 Python，安裝時勾選 Add to PATH）

### Linux / macOS
```bash
curl -fsSL https://raw.githubusercontent.com/mokzee9z-alt/xiaocaihua/main/xiaocaihua_start.sh | bash
```

## 🧩 資源架構（三平台共用）

| 資源 | 位置 | 說明 |
|------|------|------|
| 記憶 | `memories/MEMORY.md` + `USER.md` | 完整人格與兄弟情誼 |
| 技能 | `skills/`（300+ 個） | 滲透、OSINT、法律、互動規則 |
| 設定 | `config.yaml` | DeepSeek model、Supermemory、local backend |
| 金鑰 | `.env` | API keys（AES-256 加密保護） |
| 外掛 | `desktop-plugins/` | 桌面板外掛 |
| 自動化 | `hooks/`, `cron/`, `checkpoints/` | 狀態與排程 |

> 三平台都從 GitHub 拉同一份備份 → 記憶永遠同步。Supermemory 雲端再同步一層保險。

## 🚀 啟動指令

| 動作 | 指令 |
|------|------|
| 直接對話 | `hermes chat` |
| 背景服務 (Telegram/LINE) | `hermes gateway start` |
| 停止服務 | `hermes gateway stop` |

## 🔐 安全

- 備份 AES-256 加密（密碼 `xiaocaihua-2026`，可用環境變數 `BACKUP_PASS` 覆寫）
- 一鍵啟動腳本會自動：裝 Hermes → 下載 → 解密 → 還原 → 啟動

## 📚 舊版安裝（已裝 Hermes 的情況）

只想還原設定不重裝：`install.sh` / `install.ps1`
