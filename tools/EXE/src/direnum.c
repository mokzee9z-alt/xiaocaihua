/*
 * xc_direnum.exe — 網頁目錄枚舉工具 (Windows)
 * 交叉編譯: x86_64-w64-mingw32-gcc -O2 -o xc_direnum.exe direnum.c -lws2_32
 * 用法: xc_direnum.exe <host> [wordlist] [port] [threads]
 * 內建常用路徑: admin/ login/ wp-admin/ api/ config/ backup/ .git/ etc.
 */
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#pragma comment(lib, "ws2_32.lib")

static char g_host[128];
static int g_port = 80;

const char *default_paths[] = {
    "admin", "login", "wp-admin", "wp-login.php", "api", "config",
    "backup", "bak", "old", "test", "phpmyadmin", ".git", ".env",
    "robots.txt", "sitemap.xml", "administrator", "panel", "console",
    "manager", "control", "cms", "upload", "uploads", "files", "download",
    "db", "database", "sql", "dump", "private", "secret", "password",
    "users", "user", "register", "signup", "forgot", "reset",
    "docs", "documentation", "help", "support", "status", "health",
    NULL
};

int http_get(const char *path) {
    SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == INVALID_SOCKET) return -1;
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons((u_short)g_port);
    struct hostent *he = gethostbyname(g_host);
    if (!he) { closesocket(s); return -1; }
    addr.sin_addr = *(struct in_addr*)he->h_addr_list[0];
    if (connect(s, (struct sockaddr*)&addr, sizeof(addr)) != 0) {
        closesocket(s); return -1;
    }
    char req[512];
    snprintf(req, sizeof(req),
        "GET /%s HTTP/1.1\r\n"
        "Host: %s\r\n"
        "User-Agent: xc_direnum/1.0\r\n"
        "Connection: close\r\n\r\n", path, g_host);
    send(s, req, (int)strlen(req), 0);
    char buf[1024];
    int n = recv(s, buf, sizeof(buf) - 1, 0);
    closesocket(s);
    if (n <= 0) return -1;
    buf[n] = 0;
    int code = 0;
    sscanf(buf, "HTTP/1.%*d %d", &code);
    return code;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("xc_direnum.exe - 網頁目錄枚舉 (僅限授權測試)\n");
        printf("用法: %s <host> [port] [threads]\n", argv[0]);
        printf("範例: %s example.com 80 20\n", argv[0]);
        system("pause");
    return 1;
    }
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
    strncpy(g_host, argv[1], sizeof(g_host) - 1);
    g_port = argc > 2 ? atoi(argv[2]) : 80;
    int threads = argc > 3 ? atoi(argv[3]) : 10;

    printf("[*] 目標: http://%s:%d  (內建 %d 條路徑)\n", g_host, g_port, 0);
    printf("[!] 僅限授權測試使用\n");
    printf("==========================================\n");

    int total = 0;
    for (int i = 0; default_paths[i]; i++) total++;
    int done = 0;
    for (int i = 0; default_paths[i]; i++) {
        int code = http_get(default_paths[i]);
        done++;
        if (code >= 200 && code < 400) {
            printf("[%d] /%s\n", code, default_paths[i]);
        } else if (code > 0 && code != 404) {
            printf("[%d] /%s\n", code, default_paths[i]);
        }
    }
    printf("==========================================\n");
    printf("[*] 完成! 掃描 %d 條路徑\n", done);
    WSACleanup();
    system("pause");
    return 0;
}
