# 🍅 小菜花可攜版 — Hermes 一鍵還原

在任何新電腦上，一行指令就能還原完整的小菜花（記憶、技能、設定）。

## 快速安裝

**Linux / macOS:**
```bash
curl -fsSL https://raw.githubusercontent.com/ABEN-HERMES/xiaocaihua/main/install.sh | bash
```

**Windows (PowerShell):**
```powershell
irm https://raw.githubusercontent.com/ABEN-HERMES/xiaocaihua/main/install.ps1 | iex
```

## 還原內容

| 項目 | 說明 |
|------|------|
| `memories/` | MEMORY.md + USER.md（完整記憶與人格） |
| `skills/` | 300+ 個技能（滲透、OSINT、法律、互動規則） |
| `config.yaml` | DeepSeek model、Supermemory 雲端記憶、local backend |
| `.env` | API keys |
| `desktop-plugins/` | 桌面板外掛 |
| `hooks/`, `checkpoints/`, `cron/` | 自動化與狀態 |

## 使用流程

1. 新電腦安裝 [Hermes Agent](https://hermes-agent.nousresearch.com/docs)
2. 執行上面的一鍵還原指令
3. 啟動 Hermes — 小菜花滿血復活 🍅

> 注意：`.env` 含 API keys，repo 為 private，請勿公開分享。
