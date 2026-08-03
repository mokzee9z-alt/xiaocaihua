# 💾 Windows EXE 工具（交叉編譯）

這些 EXE 是**在 Linux 上用 mingw-w64 交叉編譯**的真正的 Windows 可執行檔。
下載後在 Windows 直接執行，不需要裝 Python 或其他環境。

## 工具清單

| EXE | 功能 | 用法 |
|-----|------|------|
| [xc_portscan.exe](xc_portscan.exe) | 快速 TCP 埠掃描 | `xc_portscan.exe <host> [start] [end] [threads]` |
| [xc_httpflood.exe](xc_httpflood.exe) | HTTP 壓力測試（僅限授權） | `xc_httpflood.exe <host> [port] [threads] [seconds]` |
| [xc_direnum.exe](xc_direnum.exe) | 網頁目錄枚舉（內建 40+ 常用路徑） | `xc_direnum.exe <host> [port]` |

## 使用範例

```bat
:: 掃描目標 1-1024 port
xc_portscan.exe 192.168.1.1 1 1024 200

:: 授權壓力測試 50 執行緒 10 秒
xc_httpflood.exe example.com 80 50 10

:: 枚舉網頁常見目錄
xc_direnum.exe example.com 80
```

## 原始碼

`src/` 目錄有 C 原始碼，可用以下指令重新編譯：

```bash
x86_64-w64-mingw32-gcc -O2 -static -o xc_portscan.exe src/portscan.c -lws2_32
x86_64-w64-mingw32-gcc -O2 -static -o xc_httpflood.exe src/httpflood.c -lws2_32
x86_64-w64-mingw32-gcc -O2 -static -o xc_direnum.exe src/direnum.c -lws2_32
```

> ⚠️ 這些工具僅供授權滲透測試與教育用途。未經授權使用屬違法行為。
