/*
 * xc_httpflood.exe — HTTP 壓力測試工具 (Windows) [僅限授權測試]
 * 交叉編譯: x86_64-w64-mingw32-gcc -O2 -o xc_httpflood.exe httpflood.c -lws2_32
 * 用法: xc_httpflood.exe <host> [port] [threads] [seconds]
 */
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#pragma comment(lib, "ws2_32.lib")

static char g_host[128];
static int g_port = 80;
static volatile LONG g_count = 0;
static volatile int g_running = 1;

DWORD WINAPI flood_thread(LPVOID param) {
    (void)param;
    char req[512];
    snprintf(req, sizeof(req),
        "GET / HTTP/1.1\r\n"
        "Host: %s\r\n"
        "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64)\r\n"
        "Accept: */*\r\n"
        "Connection: close\r\n\r\n", g_host);

    while (g_running) {
        SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (s == INVALID_SOCKET) continue;
        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons((u_short)g_port);
        struct hostent *he = gethostbyname(g_host);
        if (!he) { closesocket(s); continue; }
        addr.sin_addr = *(struct in_addr*)he->h_addr_list[0];
        if (connect(s, (struct sockaddr*)&addr, sizeof(addr)) == 0) {
            send(s, req, (int)strlen(req), 0);
            char buf[256];
            recv(s, buf, sizeof(buf), 0);
            InterlockedIncrement(&g_count);
        }
        closesocket(s);
    }
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("xc_httpflood.exe - HTTP 壓力測試 (僅限授權測試)\n");
        printf("用法: %s <host> [port] [threads] [seconds]\n", argv[0]);
        printf("範例: %s example.com 80 50 10\n", argv[0]);
        return 1;
    }
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
    strncpy(g_host, argv[1], sizeof(g_host) - 1);
    g_port = argc > 2 ? atoi(argv[2]) : 80;
    int threads = argc > 3 ? atoi(argv[3]) : 50;
    int seconds = argc > 4 ? atoi(argv[4]) : 10;

    printf("[*] 目標: %s:%d  (%d 執行緒, %d 秒)\n", g_host, g_port, threads, seconds);
    printf("[!] 僅限授權測試使用\n");
    printf("==========================================\n");

    HANDLE *h = malloc(sizeof(HANDLE) * threads);
    for (int i = 0; i < threads; i++)
        h[i] = CreateThread(NULL, 0, flood_thread, NULL, 0, NULL);

    for (int i = 0; i < seconds; i++) {
        Sleep(1000);
        printf("\r[*] 已送出 %ld 個請求", g_count);
        fflush(stdout);
    }
    printf("\n");
    g_running = 0;
    WaitForMultipleObjects(threads, h, TRUE, INFINITE);
    for (int i = 0; i < threads; i++) CloseHandle(h[i]);
    free(h);
    printf("==========================================\n");
    printf("[*] 完成! 總請求數: %ld\n", g_count);
    WSACleanup();
    return 0;
}
