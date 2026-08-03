/*
 * xc_toolbox.exe — 小菜花網路工具箱 (Windows GUI)
 * 整合: 埠掃描 / 目錄枚舉 / HTTP 壓力測試
 * 交叉編譯: x86_64-w64-mingw32-gcc -O2 -mwindows -o xc_toolbox.exe toolbox.c -lws2_32 -lcomctl32
 */
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "comctl32.lib")

#define IDC_TARGET   101
#define IDC_PORTFROM 102
#define IDC_PORTTO   103
#define IDC_START    104
#define IDC_OUTPUT   105
#define IDC_MODE     106
#define IDC_HOST     107
#define IDC_THREADS  108

static HWND g_target, g_pfrom, g_pto, g_start, g_output, g_mode, g_threads;
static volatile int g_running = 0;

/* 執行緒安全輸出 */
static void append_text(const char *s) {
    int len = GetWindowTextLengthA(g_output);
    SendMessageA(g_output, EM_SETSEL, len, len);
    SendMessageA(g_output, EM_REPLACESEL, FALSE, (LPARAM)s);
}

static DWORD WINAPI portscan_worker(LPVOID p) {
    char host[128], out[512];
    strcpy(host, (char*)p);
    int from = (int)(INT_PTR)GetWindowLongPtrA(g_pfrom, GWLP_USERDATA);
    int to   = (int)(INT_PTR)GetWindowLongPtrA(g_pto, GWLP_USERDATA);
    int th   = (int)(INT_PTR)GetWindowLongPtrA(g_threads, GWLP_USERDATA);
    if (th < 1) th = 100; if (from < 1) from = 1; if (to < from) to = from;

    struct hostent *he = gethostbyname(host);
    if (!he) { append_text("[!] 無法解析主機\n"); g_running = 0; EnableWindow(g_start, TRUE); return 0; }
    char ip[64]; strcpy(ip, inet_ntoa(*(struct in_addr*)he->h_addr_list[0]));
    sprintf(out, "[*] 目標: %s (%s)\n[*] 掃描 port %d-%d (%d 執行緒)\n", host, ip, from, to, th);
    append_text(out);

    int total = to - from + 1, found = 0;
    for (int port = from; port <= to && g_running; port++) {
        SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (s == INVALID_SOCKET) continue;
        struct sockaddr_in addr;
        addr.sin_family = AF_INET; addr.sin_port = htons((u_short)port);
        inet_pton(AF_INET, ip, &addr.sin_addr);
        u_long mode = 1; ioctlsocket(s, FIONBIO, &mode);
        int ret = connect(s, (struct sockaddr*)&addr, sizeof(addr));
        if (ret == SOCKET_ERROR && WSAGetLastError() == WSAEWOULDBLOCK) {
            fd_set wset; FD_ZERO(&wset); FD_SET(s, &wset);
            struct timeval tv = {2, 0};
            if (select(0, NULL, &wset, NULL, &tv) > 0) {
                sprintf(out, "[OPEN] port %d\n", port);
                append_text(out); found++;
            }
        }
        closesocket(s);
    }
    sprintf(out, "[*] 完成! 找到 %d 個開放埠\n", found);
    append_text(out);
    g_running = 0; EnableWindow(g_start, TRUE);
    return 0;
}

static DWORD WINAPI direnum_worker(LPVOID p) {
    char host[128], out[512];
    strcpy(host, (char*)p);
    int port = (int)(INT_PTR)GetWindowLongPtrA(g_pfrom, GWLP_USERDATA);
    if (port < 1) port = 80;

    const char *paths[] = {
        "admin", "login", "wp-admin", "wp-login.php", "api", "config",
        "backup", "bak", "old", "test", "phpmyadmin", ".git", ".env",
        "robots.txt", "sitemap.xml", "administrator", "panel", "console",
        "manager", "upload", "uploads", "files", "db", "database", "sql",
        "private", "secret", "users", "register", "docs", "status", NULL
    };
    sprintf(out, "[*] 目標: http://%s:%d\n[*] 掃描 %d 條常用路徑...\n", host, port, 0);
    append_text(out);
    int cnt = 0;
    for (int i = 0; paths[i] && g_running; i++) {
        SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (s == INVALID_SOCKET) continue;
        struct sockaddr_in addr;
        addr.sin_family = AF_INET; addr.sin_port = htons((u_short)port);
        struct hostent *he = gethostbyname(host);
        if (!he) { closesocket(s); break; }
        addr.sin_addr = *(struct in_addr*)he->h_addr_list[0];
        if (connect(s, (struct sockaddr*)&addr, sizeof(addr)) != 0) { closesocket(s); continue; }
        char req[512];
        snprintf(req, sizeof(req), "GET /%s HTTP/1.1\r\nHost: %s\r\nUser-Agent: xc_toolbox/1.0\r\nConnection: close\r\n\r\n", paths[i], host);
        send(s, req, (int)strlen(req), 0);
        char buf[1024];
        int n = recv(s, buf, sizeof(buf)-1, 0);
        closesocket(s);
        if (n <= 0) continue;
        buf[n] = 0; int code = 0;
        sscanf(buf, "HTTP/1.%*d %d", &code);
        if (code >= 200 && code < 400) {
            sprintf(out, "[%d] /%s\n", code, paths[i]);
            append_text(out); cnt++;
        }
    }
    sprintf(out, "[*] 完成! 找到 %d 個可存取路徑\n", cnt);
    append_text(out);
    g_running = 0; EnableWindow(g_start, TRUE);
    return 0;
}

static DWORD WINAPI flood_worker(LPVOID p) {
    char host[128], out[512];
    strcpy(host, (char*)p);
    int port = (int)(INT_PTR)GetWindowLongPtrA(g_pfrom, GWLP_USERDATA);
    int th = (int)(INT_PTR)GetWindowLongPtrA(g_threads, GWLP_USERDATA);
    int secs = (int)(INT_PTR)GetWindowLongPtrA(g_pto, GWLP_USERDATA);
    if (port < 1) port = 80; if (th < 1) th = 20; if (secs < 1) secs = 10;

    sprintf(out, "[*] HTTP 壓力測試: %s:%d (%d 執行緒, %d 秒)\n[!] 僅限授權測試\n", host, port, th, secs);
    append_text(out);

    long count = 0;
    DWORD end = GetTickCount() + secs * 1000;
    while (GetTickCount() < end && g_running) {
        SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (s == INVALID_SOCKET) continue;
        struct sockaddr_in addr;
        addr.sin_family = AF_INET; addr.sin_port = htons((u_short)port);
        struct hostent *he = gethostbyname(host);
        if (!he) { closesocket(s); break; }
        addr.sin_addr = *(struct in_addr*)he->h_addr_list[0];
        if (connect(s, (struct sockaddr*)&addr, sizeof(addr)) == 0) {
            char req[512];
            snprintf(req, sizeof(req), "GET / HTTP/1.1\r\nHost: %s\r\nUser-Agent: Mozilla/5.0\r\nConnection: close\r\n\r\n", host);
            send(s, req, (int)strlen(req), 0);
            char buf[256]; recv(s, buf, sizeof(buf), 0);
            count++;
        }
        closesocket(s);
    }
    sprintf(out, "[*] 完成! 送出 %ld 個請求\n", count);
    append_text(out);
    g_running = 0; EnableWindow(g_start, TRUE);
    return 0;
}

static void start_job(HWND hwnd) {
    if (g_running) { MessageBoxA(hwnd, "正在執行中, 請稍候", "小菜花工具箱", MB_OK); return; }
    char host[128];
    GetWindowTextA(g_target, host, sizeof(host));
    if (strlen(host) == 0) { MessageBoxA(hwnd, "請輸入目標 IP 或網址", "小菜花工具箱", MB_OK); return; }
    char buf[32];
    GetWindowTextA(g_pfrom, buf, sizeof(buf));
    int from = atoi(buf);
    GetWindowTextA(g_pto, buf, sizeof(buf));
    int to = atoi(buf);
    GetWindowTextA(g_threads, buf, sizeof(buf));
    int th = atoi(buf);
    SetWindowLongPtrA(g_pfrom, GWLP_USERDATA, (LONG_PTR)from);
    SetWindowLongPtrA(g_pto, GWLP_USERDATA, (LONG_PTR)to);
    SetWindowLongPtrA(g_threads, GWLP_USERDATA, (LONG_PTR)th);

    int mode = (int)SendMessageA(g_mode, CB_GETCURSEL, 0, 0);
    char *h = malloc(128); strcpy(h, host);
    g_running = 1; EnableWindow(g_start, FALSE);
    if (mode == 0) CreateThread(NULL, 0, portscan_worker, h, 0, NULL);
    else if (mode == 1) CreateThread(NULL, 0, direnum_worker, h, 0, NULL);
    else CreateThread(NULL, 0, flood_worker, h, 0, NULL);
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    switch (msg) {
    case WM_CREATE: {
        HFONT f = CreateFontA(18, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, "Consolas");
        CreateWindowA("STATIC", "小菜花網路工具箱 v1.0", WS_CHILD|WS_VISIBLE, 12, 10, 400, 26, hwnd, NULL, NULL, NULL);
        CreateWindowA("STATIC", "功能:", WS_CHILD|WS_VISIBLE, 12, 44, 50, 20, hwnd, NULL, NULL, NULL);
        g_mode = CreateWindowA("COMBOBOX", "", WS_CHILD|WS_VISIBLE|CBS_DROPDOWNLIST, 70, 42, 260, 120, hwnd, (HMENU)IDC_MODE, NULL, NULL);
        SendMessageA(g_mode, CB_ADDSTRING, 0, (LPARAM)"埠掃描 - 檢查目標開了哪些連接埠");
        SendMessageA(g_mode, CB_ADDSTRING, 0, (LPARAM)"目錄枚舉 - 找網站隱藏管理頁面");
        SendMessageA(g_mode, CB_ADDSTRING, 0, (LPARAM)"HTTP 壓力測試 - 網站流量測試(授權用)");
        SendMessageA(g_mode, CB_SETCURSEL, 0, 0);

        CreateWindowA("STATIC", "目標 IP/網址:", WS_CHILD|WS_VISIBLE, 12, 78, 110, 20, hwnd, NULL, NULL, NULL);
        g_target = CreateWindowA("EDIT", "", WS_CHILD|WS_VISIBLE|WS_BORDER|ES_AUTOHSCROLL, 130, 76, 310, 24, hwnd, (HMENU)IDC_TARGET, NULL, NULL);

        CreateWindowA("STATIC", "參數1(起始port/port):", WS_CHILD|WS_VISIBLE, 12, 110, 150, 20, hwnd, NULL, NULL, NULL);
        g_pfrom = CreateWindowA("EDIT", "1", WS_CHILD|WS_VISIBLE|WS_BORDER, 170, 108, 60, 24, hwnd, (HMENU)IDC_PORTFROM, NULL, NULL);
        CreateWindowA("STATIC", "參數2(結束port/秒數):", WS_CHILD|WS_VISIBLE, 250, 110, 140, 20, hwnd, NULL, NULL, NULL);
        g_pto = CreateWindowA("EDIT", "1024", WS_CHILD|WS_VISIBLE|WS_BORDER, 395, 108, 50, 24, hwnd, (HMENU)IDC_PORTTO, NULL, NULL);

        CreateWindowA("STATIC", "執行緒:", WS_CHILD|WS_VISIBLE, 12, 142, 60, 20, hwnd, NULL, NULL, NULL);
        g_threads = CreateWindowA("EDIT", "100", WS_CHILD|WS_VISIBLE|WS_BORDER, 80, 140, 60, 24, hwnd, (HMENU)IDC_THREADS, NULL, NULL);

        g_start = CreateWindowA("BUTTON", "▶ 開始執行", WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON, 170, 138, 120, 30, hwnd, (HMENU)IDC_START, NULL, NULL);
        g_output = CreateWindowA("EDIT", "", WS_CHILD|WS_VISIBLE|WS_BORDER|ES_MULTILINE|ES_AUTOVSCROLL|WS_VSCROLL|ES_READONLY, 12, 178, 440, 200, hwnd, (HMENU)IDC_OUTPUT, NULL, NULL);
        SendMessageA(g_output, WM_SETFONT, (WPARAM)f, TRUE);
        SendMessageA(g_mode, WM_SETFONT, (WPARAM)f, TRUE);
        SendMessageA(g_target, WM_SETFONT, (WPARAM)f, TRUE);
        SendMessageA(g_pfrom, WM_SETFONT, (WPARAM)f, TRUE);
        SendMessageA(g_pto, WM_SETFONT, (WPARAM)f, TRUE);
        SendMessageA(g_threads, WM_SETFONT, (WPARAM)f, TRUE);
        SendMessageA(g_start, WM_SETFONT, (WPARAM)f, TRUE);
        append_text("歡迎使用小菜花工具箱!\n選功能 → 填目標 → 按開始執行\n結果會顯示在這裡, 不會消失\n");
        break;
    }
    case WM_COMMAND:
        if (LOWORD(w) == IDC_START) start_job(hwnd);
        break;
    case WM_DESTROY:
        g_running = 0;
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProcA(hwnd, msg, w, l);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR cmd, int show) {
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
    WNDCLASSA wc = {0};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    wc.lpszClassName = "XCToolbox";
    RegisterClassA(&wc);
    HWND hwnd = CreateWindowA("XCToolbox", "小菜花網路工具箱", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 480, 430, NULL, NULL, hInst, NULL);
    ShowWindow(hwnd, show);
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    WSACleanup();
    return 0;
}
