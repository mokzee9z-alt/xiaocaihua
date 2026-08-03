#!/usr/bin/env bash
# GhostGPT 部署腳本 (Linux/macOS)
# 輕量 AI 駭客終端 - 用 Hermes API 或 OpenAI-compatible API
set -euo pipefail

echo "👻 GhostGPT 部署中..."
echo "=========================================="

# 檢查 Python
PY="$(command -v python3 || command -v python || true)"
if [ -z "$PY" ]; then echo "❌ 需要 Python 3"; exit 1; fi

# 建立專案目錄
mkdir -p ~/ghostgpt && cd ~/ghostgpt

# 下載主程式
curl -fsSL -o ghostgpt.py "https://raw.githubusercontent.com/mokzee9z-alt/xiaocaihua/main/tools/AI-tools/ghostgpt.py" || {
  echo "⚠️ 下載失敗, 改用內建簡單版..."
  cat > ghostgpt.py << 'PYEOF'
#!/usr/bin/env python3
"""GhostGPT - 簡單版 AI 駭客終端"""
import sys, os, urllib.request, json, readline

API_URL = os.environ.get("GHOSTGPT_API", "https://api.deepseek.com/v1/chat/completions")
API_KEY = os.environ.get("GHOSTGPT_KEY", "")
MODEL = os.environ.get("GHOSTGPT_MODEL", "deepseek-v4-flash")

def chat(messages):
    req = urllib.request.Request(API_URL, data=json.dumps({
        "model": MODEL, "messages": messages, "stream": False
    }).encode(), headers={
        "Content-Type": "application/json",
        "Authorization": f"Bearer {API_KEY}"
    })
    with urllib.request.urlopen(req, timeout=120) as r:
        d = json.loads(r.read())
        return d["choices"][0]["message"]["content"]

def main():
    if not API_KEY:
        print("⚠️  請先設定 GHOSTGPT_KEY 環境變數")
        sys.exit(1)
    msgs = [{"role": "system", "content": "你是 GhostGPT, 一個專業的滲透測試 AI 助手。只在授權範圍內行動。"}]
    print("👻 GhostGPT 就緒 (輸入 exit 離開)")
    while True:
        try:
            q = input("ghost> ")
        except (EOFError, KeyboardInterrupt):
            print(); break
        if q.strip().lower() in ("exit", "quit"): break
        msgs.append({"role": "user", "content": q})
        try:
            print(chat(msgs))
            msgs.append({"role": "assistant", "content": "..."})
        except Exception as e:
            print(f"❌ {e}")

if __name__ == "__main__":
    main()
PYEOF
}

echo "=========================================="
echo "✅ GhostGPT 就位!"
echo "啟動:"
echo "  export GHOSTGPT_KEY=你的API金鑰"
echo "  python3 ~/ghostgpt/ghostgpt.py"
