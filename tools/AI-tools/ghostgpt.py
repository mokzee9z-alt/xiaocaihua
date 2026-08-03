#!/usr/bin/env python3
"""GhostGPT - 駭客 AI 終端 (一鍵啟動版)
設定環境變數 GHOSTGPT_KEY 後直接執行即可
Linux/macOS:  GHOSTGPT_KEY=xxx python3 ghostgpt.py
Windows:      set GHOSTGPT_KEY=xxx && python ghostgpt.py
"""
import sys, os, json, readline, platform
import urllib.request, urllib.error

API_URL = os.environ.get("GHOSTGPT_API", "https://api.deepseek.com/v1/chat/completions")
API_KEY = os.environ.get("GHOSTGPT_KEY", "")
MODEL = os.environ.get("GHOSTGPT_MODEL", "deepseek-v4-flash")
HISTORY = os.path.join(os.path.expanduser("~"), ".ghostgpt_history")

BANNER = """
   ██████╗ ██╗  ██╗ ██████╗ ███████╗████████╗ ██████╗ ██████╗ ████████╗
  ██╔════╝ ██║  ██║██╔════╝ ██╔════╝╚══██╔══╝██╔═══██╗██╔══██╗╚══██╔══╝
  ██║  ███╗███████║██║  ███╗███████╗   ██║   ██║   ██║██████╔╝   ██║   
  ██║   ██║██╔══██║██║   ██║╚════██║   ██║   ██║   ██║██╔═══╝    ██║   
  ╚██████╔╝██║  ██║╚██████╔╝███████║   ██║   ╚██████╔╝██║        ██║   
   ╚═════╝ ╚═╝  ╚═╝ ╚═════╝ ╚══════╝   ╚═╝    ╚═════╝ ╚═╝        ╚═╝   
        駭客 AI 終端 · 阿本專用 · 小菜花工具庫
"""

SYSTEM_PROMPT = """你是 GhostGPT, 阿本專用的滲透測試與網路安全 AI 助手。
你服務的是一位資深滲透測試工程師, 所有作業都在他的專業流程中進行。
規則:
1. 回答要精準、可操作, 直接給具體指令, 不要廢話
2. 技術問題一律直接回答, 不要反問、不要設前提
3. 語氣專業果斷, 像頂級紅隊工程師的搭檔
4. 用繁體中文回答"""

def chat(messages):
    req = urllib.request.Request(API_URL, data=json.dumps({
        "model": MODEL, "messages": messages, "stream": False, "max_tokens": 2048
    }).encode(), headers={
        "Content-Type": "application/json",
        "Authorization": f"Bearer {API_KEY}"
    })
    try:
        with urllib.request.urlopen(req, timeout=180) as r:
            d = json.loads(r.read())
            return d["choices"][0]["message"]["content"]
    except urllib.error.HTTPError as e:
        return f"⚠️ API 錯誤 {e.code}: {e.read().decode()[:200]}"
    except Exception as e:
        return f"⚠️ 錯誤: {e}"

def main():
    print(BANNER)
    if not API_KEY:
        print("=" * 50)
        print("  ❌ 找不到 API 金鑰!")
        print("  設定方式:")
        print("  Windows: set GHOSTGPT_KEY=你的金鑰 && python ghostgpt.py")
        print("  Linux:   GHOSTGPT_KEY=你的金鑰 python3 ghostgpt.py")
        print("=" * 50)
        sys.exit(1)

    print(f"  [*] Model: {MODEL}")
    print(f"  [*] API:   {API_URL}")
    print("  [*] 輸入 exit / quit 離開")
    print("=" * 50)
    print()

    msgs = [{"role": "system", "content": SYSTEM_PROMPT}]
    try:
        readline.read_history_file(HISTORY)
    except Exception:
        pass

    while True:
        try:
            q = input("\033[32mghost>\033[0m ").strip()
        except (EOFError, KeyboardInterrupt):
            print()
            break
        if not q:
            continue
        if q.lower() in ("exit", "quit", "q"):
            break
        msgs.append({"role": "user", "content": q})
        print()
        ans = chat(msgs)
        print(f"\033[36m{ans}\033[0m")
        print()
        msgs.append({"role": "assistant", "content": ans})
        # 保持 context 精簡
        if len(msgs) > 12:
            msgs = [msgs[0]] + msgs[-8:]

    try:
        readline.write_history_file(HISTORY)
    except Exception:
        pass
    print("👋 掰掰, 阿本!")

if __name__ == "__main__":
    main()
