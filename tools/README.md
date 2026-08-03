# 🛠️ 小菜花工具庫

阿本專用工具庫 — 分類收錄滲透、偵察、密碼、網頁、DDoS、AI 助手與 Windows GUI 工具。
**三平台共用，Windows 有 GUI 版和 EXE 版。**

---

## 🤖 AI 助手

| 工具 | 用途 | 取得方式 |
|------|------|----------|
| [LOVABLE](https://lovable.dev) | AI 網站/App 開發平台 | Web 版 |
| [OpenAI CODEX](https://github.com/openai/codex) | AI 編碼 CLI（終端駭客助手） | `npm install -g @openai/codex` |
| GhostGPT | 駭客 AI 終端 | 部署腳本見 `AI-tools/ghostgpt/` |
| Claude Code | 法律文書/深思考 | `npm install -g @anthropic-ai/claude-code` |

## 🔍 偵察與枚舉 (Recon)

| 工具 | 用途 | 安裝 |
|------|------|------|
| nmap | 網路掃描之王 | `apt install nmap` |
| masscan | 超高速掃描 | `apt install masscan` |
| subfinder | 子域名枚舉 | `apt install subfinder` |
| amass | OWASP 攻擊面 | `apt install amass` |
| theHarvester | OSINT 蒐集 | `apt install theharvester` |
| recon-ng | 偵察框架 | `apt install recon-ng` |
| Shodan | 物聯網搜尋引擎 | Web/API |
| Censys | 攻擊面搜尋 | Web/API |

## 💣 漏洞利用 (Exploit)

| 工具 | 用途 | 安裝 |
|------|------|------|
| Metasploit | 利用框架之王 | `apt install metasploit-framework` |
| searchsploit | ExploitDB 搜尋 | `apt install exploitdb` |
| nuclei | 模板掃描器 | `apt install nuclei` |
| sqlmap | SQLi 自動化 | `apt install sqlmap` |
| Nikto | 網頁伺服器掃描 | `apt install nikto` |
| ffuf | 目錄/參數暴力 | `apt install ffuf` |
| gobuster | 目錄/子域暴力 | `apt install gobuster` |

## 🔑 密碼工具 (Password)

| 工具 | 用途 | 安裝 |
|------|------|------|
| Hydra | 線上暴力破解 | `apt install hydra` |
| hashcat | GPU 密碼破解 | `apt install hashcat` |
| John the Ripper | CPU 密碼破解 | `apt install john` |
| Crunch | 字典產生器 | `apt install crunch` |
| CeWL | 網站字典產生 | `apt install cewl` |

## 🌐 網頁滲透 (Web)

| 工具 | 用途 | 安裝 |
|------|------|------|
| Burp Suite | 網頁滲透整合平台 | 官方安裝檔 |
| OWASP ZAP | 免費網頁掃描 | `apt install zaproxy` |
| WPScan | WordPress 掃描 | `apt install wpscan` |
| XSStrike | XSS 探測 | GitHub |
| wafw00f | WAF 偵測 | `apt install wafw00f` |
| dnsrecon | DNS 偵察 | `apt install dnsrecon` |

## 📡 DDoS / 壓力測試（僅限授權測試）

| 工具 | 用途 | 安裝 |
|------|------|------|
| hping3 | 封包產生/壓力測試 | `apt install hping3` |
| LOIC | 低軌道離子砲 | GitHub |
| HOIC | 高軌道離子砲 | GitHub |
| GoldenEye | HTTP DoS 測試 | GitHub |
| slowloris | 慢速攻擊 | `apt install slowloris` |

## 🪟 Windows GUI 工具（推薦）

| 工具 | 用途 | 型態 |
|------|------|------|
| [Zenmap](https://nmap.org/zenmap/) | nmap GUI 版 | 安裝檔 |
| [Burp Suite CE](https://portswigger.net/burp) | 網頁滲透 GUI | 安裝檔 |
| [OWASP ZAP](https://www.zaproxy.org/) | 網頁掃描 GUI | 安裝檔 |
| [Wireshark](https://www.wireshark.org/) | 封包分析 GUI | 安裝檔 |
| [Hashcat GUI](https://hashcat.net/hashcat/) | 密碼破解 GUI | 安裝檔 |
| [Johnny](https://openwall.info/john/johnny) | John GUI | 安裝檔 |
| [CrackMapExec](https://github.com/Pennyw0rth/NetExec) | AD 橫向 GUI | 安裝檔 |
| LOIC | DDoS GUI | 安裝檔 |
| Wifite2 | WiFi 攻擊 GUI | 安裝檔 |
| Nessus | 漏洞掃描 GUI | 安裝檔 |

## 💾 Windows EXE（交叉編譯版）

`tools/EXE/` — 用 mingw-w64 / PyInstaller 從 Linux 交叉編譯的 Windows 可執行檔。

| EXE | 來源工具 | 說明 |
|-----|----------|------|
| 更多陸續製作中... | | |

---

### 📥 快速安裝 Kali 全套（Linux）
```bash
apt install -y nmap masscan hydra hashcat john sqlmap nikto ffuf gobuster wpscan nuclei searchsploit hping3 subfinder amass theharvester recon-ng zaproxy wafw00f dnsrecon slowloris
```

### 🪟 Windows 快速安裝
```powershell
winget install nmap.nmap                    # Zenmap GUI
winget install PortSwigger.BurpSuite.Community
winget install OWASP.ZAP
winget install WiresharkFoundation.Wireshark
winget install Hashcat.Hashcat
```
