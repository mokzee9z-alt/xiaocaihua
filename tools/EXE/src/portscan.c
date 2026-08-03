/*
 * xc_portscan.exe — 快速 TCP 埠掃描器 (Windows)
 * 交叉編譯: x86_64-w64-mingw32-gcc -O2 -o xc_portscan.exe portscan.c -lws2_32
 * 用法: xc_portscan.exe <host> [start-port] [end-port] [threads]
 */
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#pragma comment(lib, "ws2_32.lib")

static char g_target_ip[64] = "127.0.0.1";

DWORD WINAPI scan_thread(LPVOID param) {
    int port = (int)(INT_PTR)param;
    SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == INVALID_SOCKET) return 0;
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons((u_short)port);
    inet_pton(AF_INET, g_target_ip, &addr.sin_addr);
    u_long mode = 1;
    ioctlsocket(s, FIONBIO, &mode);
    int ret = connect(s, (struct sockaddr*)&addr, sizeof(addr));
    if (ret == SOCKET_ERROR && WSAGetLastError() == WSAEWOULDBLOCK) {
        fd_set wset;
        FD_ZERO(&wset);
        FD_SET(s, &wset);
        struct timeval tv = {3, 0};
        ret = select(0, NULL, &wset, NULL, &tv);
        if (ret > 0) {
            printf("[OPEN] port %d\n", port);
            fflush(stdout);
        }
    }
    closesocket(s);
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("xc_portscan.exe - 快速 TCP 掃描器\n");
        printf("用法: %s <host> [start] [end] [threads]\n", argv[0]);
        printf("範例: %s 192.168.1.1 1 1024 200\n", argv[0]);
        return 1;
    }
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    struct hostent *he = gethostbyname(argv[1]);
    if (!he) {
        printf("[!] 無法解析主機: %s\n", argv[1]);
        return 1;
    }
    strcpy(g_target_ip, inet_ntoa(*(struct in_addr*)he->h_addr_list[0]));
    printf("[*] 目標: %s (%s)\n", argv[1], g_target_ip);

    int start = argc > 2 ? atoi(argv[2]) : 1;
    int end = argc > 3 ? atoi(argv[3]) : 1024;
    int threads = argc > 4 ? atoi(argv[4]) : 100;
    if (end < start) { int t = start; start = end; end = t; }

    printf("[*] 掃描 %s 的 port %d-%d (%d 執行緒)...\n", g_target_ip, start, end, threads);
    printf("==========================================\n");

    HANDLE *h = malloc(sizeof(HANDLE) * threads);
    int idx = 0;
    for (int p = start; p <= end; p++) {
        h[idx] = CreateThread(NULL, 0, scan_thread, (LPVOID)(INT_PTR)p, 0, NULL);
        idx++;
        if (idx >= threads) {
            WaitForMultipleObjects(idx, h, TRUE, INFINITE);
            for (int i = 0; i < idx; i++) CloseHandle(h[i]);
            idx = 0;
        }
    }
    if (idx > 0) {
        WaitForMultipleObjects(idx, h, TRUE, INFINITE);
        for (int i = 0; i < idx; i++) CloseHandle(h[i]);
    }
    free(h);
    printf("==========================================\n");
    printf("[*] 掃描完成。\n");
    WSACleanup();
    return 0;
}
