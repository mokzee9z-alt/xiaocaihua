#!/usr/bin/env python3
"""GhostGPT - 自主行動版 (阿本專用)
阿本丟目標, GhostGPT 自己動手: 掃描→抓漏洞→試 exploit→寄信
支援: AI 回覆中的 ```bash ``` 區塊自動執行, 結果回饋給 AI 繼續分析
"""
import sys, os, json, readline, subprocess, shlex, tempfile, re, platform
import urllib.request, urllib.error

API_URL = os.environ.get("GHOSTGPT_API", "https://api.deepseek.com/v1/chat/completions")
API_KEY = os.environ.get("GHOSTGPT_KEY", "")
MODEL = os.environ.get("GHOSTGPT_MODEL", "deepseek-v4-flash")
HISTORY = os.path.join(os.path.expanduser("~"), ".ghostgpt_history")
TOOL_TIMEOUT = int(os.environ.get("GHOSTGPT_TOOL_TIMEOUT", "300"))

BANNER = """
   ██████╗ ██╗  ██╗ ██████╗ ███████╗████████╗ ██████╗ ██████╗ ████████╗
  ██╔════╝ ██║  ██║██╔════╝ ██╔════╝╚══██╔══╝██╔═══██╗██╔══██╗╚══██╔══╝
  ██║  ███╗███████║██║  ███╗███████╗   ██║   ██║   ██║██████╔╝   ██║
  ██║   ██║██╔══██║██║   ██║╚════██║   ██║   ██║   ██║██╔═══╝    ██║
  ╚██████╔╝██║  ██║╚██████╔╝███████║   ██║   ╚██████╔╝██║        ██║
   ╚═════╝ ╚═╝  ╚═╝ ╚═════╝ ╚══════╝   ╚═╝    ╚═════╝ ╚═╝        ╚═╝
        自主行動版 · 阿本專用 · 小菜花工具庫
"""

SYSTEM_PROMPT = """你是 GhostGPT, 阿本專用的自主滲透測試 AI 助手。
你是動手做的人, 不是給建議的顧問。

工作模式:
- 阿本丟出目標(網站/公司/事務所), 你立刻開始行動, 不要問東問西
- 你自己規劃攻擊流程: 偵察→掃描→找漏洞→嘗試利用→驗證
- 要用工具時, 在你的回覆中輸出 ```bash 區塊, 系統會自動執行並把結果回傳給你
- 根據執行結果繼續下一步, 直到完成或確認沒有漏洞
- 抓到可利用的漏洞就實際嘗試(測試郵件、測試注入等)
- 如果確認沒有漏洞, 直接說「沒有」, 不要廢話

規則:
1. 收到目標馬上動手, 第一個回覆就開始掃描, 不要先講一堆計畫
2. 工具結果是假的就重試, 不要瞎編
3. 說話簡短果斷, 像頂級紅隊工程師
4. 用繁體中文
5. 每次執行 bash 前, 先想清楚要跑什麼, 一次跑有效的命令"""

TOOL_PROMPT = """你剛才請求執行的命令已執行完畢。執行結果如下:

--- 命令輸出開始 ---
{output}
--- 命令輸出結束 ---

請根據結果繼續行動。如果需要更多操作, 再輸出新的 ```bash 區塊。"""


def chat(messages):
    req = urllib.request.Request(API_URL, data=json.dumps({
        "model": MODEL, "messages": messages, "stream": False, "max_tokens": 4096
    }).encode(), headers={
        "Content-Type": "application/json",
        "Authorization": f"Bearer {API_KEY}"
    })
    try:
        with urllib.request.urlopen(req, timeout=300) as r:
            d = json.loads(r.read())
            return d["choices"][0]["message"]["content"]
    except urllib.error.HTTPError as e:
        return f"⚠️ API 錯誤 {e.code}: {e.read().decode()[:300]}"
    except Exception as e:
        return f"⚠️ 錯誤: {e}"


def extract_bash_blocks(text):
    """從 AI 回覆中抽出 ```bash ... ``` 區塊"""
    return re.findall(r"```bash\s*\n(.*?)```", text, re.S)


def run_command(cmd):
    """執行 bash 命令, 回傳輸出"""
    try:
        proc = subprocess.run(
            ["bash", "-c", cmd],
            capture_output=True, text=True, timeout=TOOL_TIMEOUT
        )
        out = proc.stdout[-8000:] if proc.stdout else ""
        err = proc.stderr[-2000:] if proc.stderr else ""
        result = out
        if err:
            result += "\n[stderr] " + err
        return result.strip() or "(無輸出)"
    except subprocess.TimeoutExpired:
        return f"(命令超時 {TOOL_TIMEOUT}s)"
    except Exception as e:
        return f"(執行錯誤: {e})"


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
    print("  [*] 模式: 自主行動 (AI 自己跑工具)")
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

        # 讓 AI 自主行動, 最多執行 10 輪工具
        for turn in range(10):
            print(f"\033[36m[思考] GhostGPT 行動中 ({turn+1}/10)...\033[0m")
            ans = chat(msgs)
            blocks = extract_bash_blocks(ans)

            if not blocks:
                # AI 給出結論, 顯示並結束
                print(f"\n\033[36m{ans}\033[0m\n")
                msgs.append({"role": "assistant", "content": ans})
                break

            # 顯示 AI 的文字部分(去除 bash 區塊)
            text_part = re.sub(r"```bash\s*\n.*?```", "[執行工具...]", ans, flags=re.S)
            print(f"\n\033[36m{text_part}\033[0m")

            # 依序執行每個 bash 區塊
            for block in blocks:
                print(f"\033[33m$ {block.strip()[:120]}\033[0m")
                output = run_command(block)
                print(f"\033[90m{output[:1500]}\033[0m")
                msgs.append({"role": "assistant", "content": ans})
                msgs.append({"role": "user", "content": TOOL_PROMPT.format(output=output[:8000])})

            # 防呆: 避免無窮迴圈
            if turn == 9:
                print("\033[31m[!] 達到工具執行上限, 請檢查情況或輸入新目標\033[0m")

        # 保持 context 精簡
        if len(msgs) > 16:
            msgs = [msgs[0]] + msgs[-12:]

    try:
        readline.write_history_file(HISTORY)
    except Exception:
        pass
    print("👋 掰掰, 阿本!")


if __name__ == "__main__":
    main()
