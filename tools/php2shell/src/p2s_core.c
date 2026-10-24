/*
 * p2s_core.c — PHP2Shell 排查核心
 * Windows(_WIN32) 與 Linux 皆可編譯；Linux 版用來在 VPS 實測邏輯。
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <time.h>

#include "p2s_core.h"

#ifdef _WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #include <windows.h>
  #include <iphlpapi.h>
  #include <winhttp.h>
  #pragma comment(lib, "ws2_32.lib")
  #pragma comment(lib, "iphlpapi.lib")
  #pragma comment(lib, "winhttp.lib")
  #define P2S_SOCK SOCKET
  #define P2S_BAD  INVALID_SOCKET
  #define p2s_closesock(s) closesocket(s)
  #define P2S_POPEN _popen
  #define P2S_PCLOSE _pclose
  typedef HANDLE p2s_tid_t;
#else
  #include <sys/socket.h>
  #include <sys/types.h>
  #include <netinet/in.h>
  #include <netinet/tcp.h>
  #include <arpa/inet.h>
  #include <unistd.h>
  #include <fcntl.h>
  #include <errno.h>
  #include <ifaddrs.h>
  #include <netdb.h>
  #include <net/if.h>
  #include <sys/time.h>
  #include <pthread.h>
  #define P2S_SOCK int
  #define P2S_BAD  (-1)
  #define p2s_closesock(s) close(s)
  #define P2S_POPEN popen
  #define P2S_PCLOSE pclose
  typedef pthread_t p2s_tid_t;
#endif

/* ================= 小工具（不依賴 strcasestr） ================= */

static const char *p2s_stristr(const char *hay, const char *needle)
{
    size_t nl;
    if (!hay || !needle) return NULL;
    nl = strlen(needle);
    if (nl == 0) return hay;
    for (; *hay; hay++) {
        size_t i = 0;
        while (i < nl && hay[i] &&
               tolower((unsigned char)hay[i]) == tolower((unsigned char)needle[i]))
            i++;
        if (i == nl) return hay;
    }
    return NULL;
}

static int p2s_strnicmp(const char *a, const char *b, size_t n)
{
    size_t i;
    for (i = 0; i < n; i++) {
        int ca = tolower((unsigned char)a[i]);
        int cb = tolower((unsigned char)b[i]);
        if (ca != cb) return ca - cb;
        if (!ca) return 0;
    }
    return 0;
}

/* ================= 版本判定 ================= */

int p2s_parse_version(const char *s, int *maj, int *min, int *pat)
{
    int a = -1, b = -1, c = -1;
    const char *p = s;
    if (!s) return 0;
    while (*p && !isdigit((unsigned char)*p)) p++;
    if (!*p) return 0;
    if (sscanf(p, "%d.%d.%d", &a, &b, &c) < 2) return 0;
    if (c < 0) c = 0;
    if (a < 0 || b < 0) return 0;
    if (maj) *maj = a;
    if (min) *min = b;
    if (pat) *pat = c;
    return 1;
}

int p2s_verdict_for(const char *ver)
{
    int a = 0, b = 0, c = 0;
    if (!p2s_parse_version(ver, &a, &b, &c)) return P2S_UNKNOWN;
    if (a < 5) return P2S_UNKNOWN;
    if (a == 5 || a == 7 || (a == 8 && b == 0) || (a == 8 && b == 1))
        return P2S_VULN;                       /* 5.4~8.1 全中且已 EOL */
    if (a == 8 && b == 2) return (c < 35) ? P2S_VULN : P2S_SAFE;
    if (a == 8 && b == 3) return (c < 36) ? P2S_VULN : P2S_SAFE;
    if (a == 8 && b == 4) return (c < 27) ? P2S_VULN : P2S_SAFE;
    if (a == 8 && b == 5) return (c < 12) ? P2S_VULN : P2S_SAFE;
    if (a == 8 && b > 5)  return P2S_SAFE;
    return P2S_UNKNOWN;
}

const char *p2s_verdict_text(int v)
{
    switch (v) {
    case P2S_SAFE:    return "SAFE";
    case P2S_VULN:    return "VULNERABLE";
    case P2S_UNKNOWN: return "UNKNOWN";
    default:          return "ERROR";
    }
}

const char *p2s_fix_for(const char *ver)
{
    int a = 0, b = 0, c = 0;
    if (!p2s_parse_version(ver, &a, &b, &c)) return "patch or upgrade PHP";
    if (a == 8 && b == 2) return "upgrade to PHP 8.2.35+";
    if (a == 8 && b == 3) return "upgrade to PHP 8.3.36+";
    if (a == 8 && b == 4) return "upgrade to PHP 8.4.27+";
    if (a == 8 && b == 5) return "upgrade to PHP 8.5.12+";
    if (a <= 8) return "EOL version - upgrade to 8.4.27 / 8.5.12";
    return "patch or upgrade PHP";
}

/* ================= 輸出 ================= */

void p2s_ctx_init(p2s_ctx *c) { memset(c, 0, sizeof(*c)); }

void p2s_net_init(void)
{
#ifdef _WIN32
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
#endif
}

void p2s_net_cleanup(void)
{
#ifdef _WIN32
    WSACleanup();
#endif
}

void p2s_log(p2s_ctx *c, const char *fmt, ...)
{
    char buf[P2S_LINE_MAX];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    if (c->logcount < P2S_MAX_LINES) {
        strncpy(c->logline[c->logcount], buf, P2S_LINE_MAX - 1);
        c->logline[c->logcount][P2S_LINE_MAX - 1] = 0;
        c->logcount++;
    }
    if (c->on_log) c->on_log(buf);
}

void p2s_add_finding(p2s_ctx *c, const p2s_finding *f)
{
    if (c->count >= P2S_MAX_HOSTS) return;
    c->items[c->count] = *f;
    c->count++;
    if (c->on_finding) c->on_finding(f);
}

/* ================= HTTP 探測 ================= */

typedef struct {
    int  open;
    int  got_reply;
    char server[128];
    char powered[96];
    char cookie[96];
    char phpver[32];
    int  phpsessid;
} p2s_http;

/* 網域 → IPv4。回傳 1 成功 */
static int p2s_resolve_host(const char *host, char *ipout, int iplen)
{
    struct addrinfo hints, *res = NULL;
    /* 已經是 IP 就直接用 */
    {
        struct in_addr tmp;
        if (inet_pton(AF_INET, host, &tmp) == 1) {
            snprintf(ipout, iplen, "%s", host);
            return 1;
        }
    }
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    if (getaddrinfo(host, NULL, &hints, &res) != 0 || !res) return 0;
    {
        struct sockaddr_in *sin = (struct sockaddr_in *)res->ai_addr;
        char buf[64];
#ifdef _WIN32
        InetNtopA(AF_INET, &sin->sin_addr, buf, (DWORD)sizeof(buf));
#else
        inet_ntop(AF_INET, &sin->sin_addr, buf, sizeof(buf));
#endif
        snprintf(ipout, iplen, "%s", buf);
    }
    freeaddrinfo(res);
    return 1;
}

static int p2s_connect(const char *ip, int port, int timeout_ms)
{
    P2S_SOCK s;
    struct sockaddr_in sa;
    struct timeval tv;
    int one = 1;

#ifdef _WIN32
    u_long nb = 1;
#else
    int fl;
#endif

    s = socket(AF_INET, SOCK_STREAM, 0);
    if (s == P2S_BAD) return -1;
    setsockopt(s, IPPROTO_TCP, TCP_NODELAY, (const char *)&one, sizeof(one));

    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_port = htons((unsigned short)port);
    if (inet_pton(AF_INET, ip, &sa.sin_addr) != 1) { p2s_closesock(s); return -1; }

#ifdef _WIN32
    ioctlsocket(s, FIONBIO, &nb);
#else
    fl = fcntl(s, F_GETFL, 0);
    fcntl(s, F_SETFL, fl | O_NONBLOCK);
#endif

    if (connect(s, (struct sockaddr *)&sa, sizeof(sa)) != 0) {
        fd_set wfds;
        struct timeval to;
        int rc, soerr = 0;
        socklen_t sl = sizeof(soerr);
        FD_ZERO(&wfds);
        FD_SET(s, &wfds);
        to.tv_sec = timeout_ms / 1000;
        to.tv_usec = (timeout_ms % 1000) * 1000;
        rc = select((int)s + 1, NULL, &wfds, NULL, &to);
        if (rc <= 0) { p2s_closesock(s); return -1; }
        getsockopt(s, SOL_SOCKET, SO_ERROR, (char *)&soerr, &sl);
        if (soerr != 0) { p2s_closesock(s); return -1; }
    }

#ifdef _WIN32
    nb = 0;
    ioctlsocket(s, FIONBIO, &nb);
#else
    fcntl(s, F_SETFL, fl);
#endif

    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv, sizeof(tv));
    setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, (const char *)&tv, sizeof(tv));
    return (int)s;
}

static void p2s_hdr(const char *buf, const char *name, char *out, int outlen)
{
    const char *p = buf;
    size_t nlen = strlen(name);
    out[0] = 0;
    while (p && *p) {
        const char *eol = strstr(p, "\r\n");
        size_t linelen = eol ? (size_t)(eol - p) : strlen(p);
        if (linelen > nlen && p[nlen] == ':' && p2s_strnicmp(p, name, nlen) == 0) {
            const char *v = p + nlen + 1;
            size_t vlen;
            while (*v == ' ' || *v == '\t') v++;
            vlen = linelen - (size_t)(v - p);
            if (vlen >= (size_t)outlen) vlen = (size_t)outlen - 1;
            memcpy(out, v, vlen);
            out[vlen] = 0;
            while (vlen > 0 && (out[vlen-1] == '\r' || out[vlen-1] == ' '))
                out[--vlen] = 0;
            return;
        }
        if (!eol) break;
        p = eol + 2;
    }
}

static int p2s_grab_phpver(const char *hay, char *out, int outlen)
{
    const char *marks[2];
    int k;
    out[0] = 0;
    if (!hay) return 0;
    marks[0] = "PHP/";
    marks[1] = "PHP ";
    for (k = 0; k < 2; k++) {
        const char *p = hay;
        while ((p = p2s_stristr(p, marks[k])) != NULL) {
            const char *d = p + strlen(marks[k]);
            if (isdigit((unsigned char)*d)) {
                int i = 0;
                while (d[i] && (isdigit((unsigned char)d[i]) || d[i] == '.') && i < outlen - 1) {
                    out[i] = d[i];
                    i++;
                }
                out[i] = 0;
                if (i > 0) return 1;
            }
            p = d;
        }
    }
    return 0;
}

static void p2s_http_probe(const char *host, int port, int timeout_ms, p2s_http *r)
{
    char req[512];
    char buf[8192];
    char ip[64];
    int s, n, total = 0;

    memset(r, 0, sizeof(*r));
    if (!p2s_resolve_host(host, ip, (int)sizeof(ip))) return;

    s = p2s_connect(ip, port, timeout_ms);
    if (s < 0) return;
    r->open = 1;

#ifdef _WIN32
    snprintf(req, sizeof(req),
        "GET / HTTP/1.0\r\nHost: %s\r\n"
        "User-Agent: Mozilla/5.0 (compatible; PHP2Shell-Check/1.0)\r\n"
        "Accept: */*\r\nConnection: close\r\n\r\n", host);
#else
    snprintf(req, sizeof(req),
        "GET / HTTP/1.0\r\nHost: %s\r\n"
        "User-Agent: Mozilla/5.0 (compatible; PHP2Shell-Check/1.0)\r\n"
        "Accept: */*\r\nConnection: close\r\n\r\n", host);
#endif
    if (send(s, req, (int)strlen(req), 0) <= 0) { p2s_closesock(s); return; }

    memset(buf, 0, sizeof(buf));
    while (total < (int)sizeof(buf) - 1) {
        n = (int)recv(s, buf + total, (int)(sizeof(buf) - 1 - total), 0);
        if (n <= 0) break;
        total += n;
    }
    p2s_closesock(s);
    if (total <= 0) return;

    buf[total] = 0;
    r->got_reply = 1;
    p2s_hdr(buf, "Server", r->server, (int)sizeof(r->server));
    p2s_hdr(buf, "X-Powered-By", r->powered, (int)sizeof(r->powered));
    p2s_hdr(buf, "Set-Cookie", r->cookie, (int)sizeof(r->cookie));
    r->phpsessid = (p2s_stristr(r->cookie, "PHPSESSID") != NULL);

    if (!p2s_grab_phpver(r->server, r->phpver, (int)sizeof(r->phpver)))
        p2s_grab_phpver(r->powered, r->phpver, (int)sizeof(r->phpver));
}

/* ============ Windows：用 WinHTTP 支援 https（系統內建，不需 OpenSSL） ============ */
#ifdef _WIN32
static void p2s_http_probe_tls(const char *host, int port, int timeout_ms, p2s_http *r)
{
    HINTERNET hs = NULL, hc = NULL, hr = NULL;
    wchar_t whost[256];
    DWORD sz = 0;
    wchar_t *whdr = NULL;
    char *mb = NULL;
    int mblen;

    memset(r, 0, sizeof(*r));
    MultiByteToWideChar(CP_ACP, 0, host, -1, whost, 256);

    hs = WinHttpOpen(L"Mozilla/5.0 (compatible; PHP2Shell-Check/1.0)",
                     WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                     WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hs) return;
    WinHttpSetTimeouts(hs, timeout_ms, timeout_ms, timeout_ms, timeout_ms);

    hc = WinHttpConnect(hs, whost, (INTERNET_PORT)port, 0);
    if (!hc) goto done;

    hr = WinHttpOpenRequest(hc, L"GET", L"/", NULL,
                            WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
                            WINHTTP_FLAG_SECURE);
    if (!hr) goto done;

    r->open = 1;
    if (!WinHttpSendRequest(hr, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                            WINHTTP_NO_REQUEST_DATA, 0, 0, 0)) goto done;
    if (!WinHttpReceiveResponse(hr, NULL)) goto done;
    r->got_reply = 1;

    sz = 0;
    WinHttpQueryHeaders(hr, WINHTTP_QUERY_RAW_HEADERS_CRLF,
                        WINHTTP_HEADER_NAME_BY_INDEX, NULL, &sz,
                        WINHTTP_NO_HEADER_INDEX);
    if (sz == 0) goto done;

    whdr = (wchar_t *)calloc(1, sz + sizeof(wchar_t));
    if (!whdr) goto done;
    if (!WinHttpQueryHeaders(hr, WINHTTP_QUERY_RAW_HEADERS_CRLF,
                             WINHTTP_HEADER_NAME_BY_INDEX, whdr, &sz,
                             WINHTTP_NO_HEADER_INDEX))
        goto done;

    mblen = WideCharToMultiByte(CP_UTF8, 0, whdr, -1, NULL, 0, NULL, NULL);
    if (mblen > 0) {
        mb = (char *)calloc(1, (size_t)mblen);
        if (mb) {
            WideCharToMultiByte(CP_UTF8, 0, whdr, -1, mb, mblen, NULL, NULL);
            p2s_hdr(mb, "Server", r->server, (int)sizeof(r->server));
            p2s_hdr(mb, "X-Powered-By", r->powered, (int)sizeof(r->powered));
            p2s_hdr(mb, "Set-Cookie", r->cookie, (int)sizeof(r->cookie));
            r->phpsessid = (p2s_stristr(mb, "PHPSESSID") != NULL);
            if (!p2s_grab_phpver(r->server, r->phpver, (int)sizeof(r->phpver)))
                p2s_grab_phpver(r->powered, r->phpver, (int)sizeof(r->phpver));
        }
    }

done:
    if (whdr) free(whdr);
    if (mb) free(mb);
    if (hr) WinHttpCloseHandle(hr);
    if (hc) WinHttpCloseHandle(hc);
    if (hs) WinHttpCloseHandle(hs);
}
#endif /* _WIN32 */

/* 把 probe 結果轉成一筆 finding */
static void p2s_fill_from_http(p2s_finding *f, const p2s_http *h)
{
    int has_php = 0;
    if (p2s_stristr(h->powered, "PHP") || p2s_stristr(h->server, "PHP") ||
        h->phpver[0] || h->phpsessid)
        has_php = 1;

    f->has_php = has_php;
    snprintf(f->server, sizeof(f->server), "%s", h->server);
    snprintf(f->powered, sizeof(f->powered), "%s", h->powered);

    if (!has_php) {
        f->verdict = P2S_SAFE;
        f->version[0] = 0;
        snprintf(f->detail, sizeof(f->detail), "no PHP fingerprint on this port");
        snprintf(f->advice, sizeof(f->advice), "nothing to do for this port");
        return;
    }

    if (h->phpver[0]) {
        snprintf(f->version, sizeof(f->version), "%s", h->phpver);
        f->verdict = p2s_verdict_for(h->phpver);
        snprintf(f->detail, sizeof(f->detail), "PHP %s detected via HTTP header", h->phpver);
        snprintf(f->advice, sizeof(f->advice), "%s", p2s_fix_for(h->phpver));
    } else {
        f->version[0] = 0;
        f->verdict = P2S_UNKNOWN;
        snprintf(f->detail, sizeof(f->detail),
                 "PHP detected (cookie/header) but version hidden");
        snprintf(f->advice, sizeof(f->advice),
                 "hide version -> must check php -v on the host itself");
    }
}

/* ================= 本機掃描 ================= */

static int p2s_run_phpv(const char *phpbin, char *out, int outlen)
{
    char cmd[512];
    FILE *fp;
    char line[256];
    out[0] = 0;
    snprintf(cmd, sizeof(cmd), "\"%s\" -v 2>&1", phpbin);
    fp = P2S_POPEN(cmd, "r");
    if (!fp) {
        snprintf(cmd, sizeof(cmd), "%s -v 2>&1", phpbin);
        fp = P2S_POPEN(cmd, "r");
        if (!fp) return 0;
    }
    while (fgets(line, sizeof(line), fp)) {
        if (p2s_stristr(line, "PHP ")) {
            char ver[32];
            if (p2s_grab_phpver(line, ver, (int)sizeof(ver))) {
                snprintf(out, outlen, "%s", ver);
                P2S_PCLOSE(fp);
                return 1;
            }
        }
        if (strlen(out) + strlen(line) < (size_t)outlen - 1)
            strncat(out, line, (size_t)outlen - strlen(out) - 1);
    }
    P2S_PCLOSE(fp);
    return 0;
}

int p2s_scan_local(p2s_ctx *c)
{
    static const char *cands[] = {
#ifdef _WIN32
        "php", "php.exe",
        "C:\\php\\php.exe",
        "C:\\xampp\\php\\php.exe",
        "C:\\wamp64\\bin\\php\\php.exe",
        "C:\\wamp\\bin\\php\\php.exe",
        "C:\\laragon\\bin\\php\\php.exe",
        "C:\\Program Files\\PHP\\php.exe",
        "C:\\Program Files (x86)\\PHP\\php.exe",
        "C:\\ProgramData\\php\\php.exe",
#else
        "php", "/usr/bin/php", "/usr/local/bin/php",
#endif
        NULL
    };
    char ver[32];
    int i, found = 0;

    p2s_log(c, "[local] 檢查本機 PHP 執行檔...");

    for (i = 0; cands[i]; i++) {
        p2s_finding f;
        if (!p2s_run_phpv(cands[i], ver, (int)sizeof(ver))) continue;

        memset(&f, 0, sizeof(f));
        snprintf(f.target, sizeof(f.target), "%s", "THIS-PC");
        snprintf(f.kind, sizeof(f.kind), "%s", "local");
        snprintf(f.version, sizeof(f.version), "%s", ver);
        f.has_php = 1;
        f.verdict = p2s_verdict_for(ver);
        snprintf(f.detail, sizeof(f.detail), "php -v @ %s", cands[i]);
        snprintf(f.advice, sizeof(f.advice), "%s", p2s_fix_for(ver));
        p2s_add_finding(c, &f);
        p2s_log(c, "[local] 找到 PHP %s (%s) -> %s", ver, cands[i], p2s_verdict_text(f.verdict));
        found = 1;
        break;
    }

    if (!found) {
        p2s_finding f;
        memset(&f, 0, sizeof(f));
        snprintf(f.target, sizeof(f.target), "%s", "THIS-PC");
        snprintf(f.kind, sizeof(f.kind), "%s", "local");
        f.verdict = P2S_SAFE;
        snprintf(f.detail, sizeof(f.detail), "no php.exe found on this machine");
        snprintf(f.advice, sizeof(f.advice), "no local PHP install detected");
        p2s_add_finding(c, &f);
        p2s_log(c, "[local] 本機沒有找到 PHP 執行檔");
    }

    /* 本機自己的 HTTP 服務（可能跑 PHP 的） */
    {
        static const int ports[] = {80, 443, 8080, 8000, 8888, 3000, 5000};
        int p;
        p2s_log(c, "[local] 檢查本機 Web 服務埠...");
        for (p = 0; p < (int)(sizeof(ports)/sizeof(ports[0])); p++) {
            p2s_http h;
            p2s_finding f;
            p2s_http_probe("127.0.0.1", ports[p], 400, &h);
            if (!h.open) continue;

            memset(&f, 0, sizeof(f));
            snprintf(f.target, sizeof(f.target), "%s", "127.0.0.1");
            snprintf(f.kind, sizeof(f.kind), "%s", "local");
            f.port = ports[p];
            p2s_fill_from_http(&f, &h);
            p2s_add_finding(c, &f);
            p2s_log(c, "[local] 127.0.0.1:%d 開著 | PHP=%s %s -> %s",
                    ports[p], h.phpver[0] ? h.phpver : (f.has_php ? "yes" : "no"),
                    f.server, p2s_verdict_text(f.verdict));
        }
    }
    return 0;
}

/* ================= 網卡列舉 ================= */

typedef struct {
    char ip[64];
    char mask[64];
    char name[64];
} p2s_nic;

static int p2s_skip_nic(const char *name, const char *desc)
{
    static const char *bad[] = {
        "loopback", "vmware", "virtualbox", "vbox", "zerotier", "wsl",
        "hyper-v", "vethernet", "bluetooth", "tap", "tun", "docker",
        "vpn", "wireguard", "tailscale", "hamachi", "npcap", "loopback",
        NULL
    };
    int i;
    char combo[256];
    snprintf(combo, sizeof(combo), "%s %s", name ? name : "", desc ? desc : "");
    for (i = 0; bad[i]; i++)
        if (p2s_stristr(combo, bad[i])) return 1;
    return 0;
}

static int p2s_list_nics(p2s_nic *out, int maxn)
{
    int n = 0;
#ifdef _WIN32
    IP_ADAPTER_INFO *ai = NULL, *p;
    ULONG sz = sizeof(IP_ADAPTER_INFO) * 32;
    ai = (IP_ADAPTER_INFO *)malloc(sz);
    if (!ai) return 0;
    if (GetAdaptersInfo(ai, &sz) == ERROR_SUCCESS) {
        for (p = ai; p && n < maxn; p = p->Next) {
            if (p->Type == MIB_IF_TYPE_LOOPBACK) continue;
            if (!p->IpAddressList.IpAddress.String[0]) continue;
            if (strcmp(p->IpAddressList.IpAddress.String, "0.0.0.0") == 0) continue;
            if (p2s_skip_nic(p->Description, p->AdapterName)) continue;
            snprintf(out[n].ip, sizeof(out[n].ip), "%s", p->IpAddressList.IpAddress.String);
            snprintf(out[n].mask, sizeof(out[n].mask), "%s", p->IpAddressList.IpMask.String);
            snprintf(out[n].name, sizeof(out[n].name), "%s", p->Description);
            n++;
        }
    }
    free(ai);
#else
    struct ifaddrs *ifa = NULL, *p;
    if (getifaddrs(&ifa) == 0) {
        for (p = ifa; p && n < maxn; p = p->ifa_next) {
            struct sockaddr_in *sin;
            if (!p->ifa_addr || p->ifa_addr->sa_family != AF_INET) continue;
            if (!(p->ifa_flags & IFF_UP)) continue;
            if (p->ifa_flags & IFF_LOOPBACK) continue;
            if (p2s_skip_nic(p->ifa_name, "")) continue;
            sin = (struct sockaddr_in *)p->ifa_addr;
            snprintf(out[n].ip, sizeof(out[n].ip), "%s", inet_ntoa(sin->sin_addr));
            sin = (struct sockaddr_in *)p->ifa_netmask;
            if (sin) snprintf(out[n].mask, sizeof(out[n].mask), "%s", inet_ntoa(sin->sin_addr));
            snprintf(out[n].name, sizeof(out[n].name), "%s", p->ifa_name);
            n++;
        }
        freeifaddrs(ifa);
    }
#endif
    return n;
}

/* ================= 區網掃描（多執行緒） ================= */

typedef struct {
    p2s_ctx *ctx;
    char hosts[P2S_MAX_HOSTS * 2][32];
    int  nhosts;
    int  next;
    int  timeout_ms;
    int  mutex_ready;
#ifdef _WIN32
    CRITICAL_SECTION cs;
#else
    pthread_mutex_t mu;
#endif
} p2s_lanjob;

static void p2s_lan_lock(p2s_lanjob *j)
{
#ifdef _WIN32
    EnterCriticalSection(&j->cs);
#else
    pthread_mutex_lock(&j->mu);
#endif
}

static void p2s_lan_unlock(p2s_lanjob *j)
{
#ifdef _WIN32
    LeaveCriticalSection(&j->cs);
#else
    pthread_mutex_unlock(&j->mu);
#endif
}

typedef struct {
    p2s_lanjob *job;
} p2s_lanarg;

static void p2s_lan_worker(void *arg)
{
    p2s_lanjob *j = ((p2s_lanarg *)arg)->job;
    for (;;) {
        char host[32];
        int idx, port = 0;
        p2s_http h;
        p2s_finding f;

        p2s_lan_lock(j);
        idx = j->next++;
        p2s_lan_unlock(j);
        if (idx >= j->nhosts) break;

        snprintf(host, sizeof(host), "%s", j->hosts[idx]);

        p2s_http_probe(host, 80, j->timeout_ms, &h);
        port = 80;
        if (!h.got_reply) {
            p2s_http_probe(host, 443, j->timeout_ms, &h);
            port = 443;
        }
        if (!h.open) continue;

        memset(&f, 0, sizeof(f));
        snprintf(f.target, sizeof(f.target), "%s", host);
        snprintf(f.kind, sizeof(f.kind), "%s", "lan");
        f.port = port;
        if (h.got_reply) {
            p2s_fill_from_http(&f, &h);
        } else {
            f.has_php = 0;
            f.verdict = P2S_UNKNOWN;
            snprintf(f.detail, sizeof(f.detail), "port %d open but no HTTP reply (TLS/other)", port);
            snprintf(f.advice, sizeof(f.advice), "identify manually");
        }
        p2s_add_finding(j->ctx, &f);
        p2s_log(j->ctx, "[lan] %s:%d | PHP=%s -> %s", host, port,
                h.phpver[0] ? h.phpver : (f.has_php ? "yes" : "no"),
                p2s_verdict_text(f.verdict));
    }
}

#ifdef _WIN32
static DWORD WINAPI p2s_win_worker(LPVOID p) { p2s_lan_worker(p); return 0; }
#else
static void *p2s_posix_worker(void *p) { p2s_lan_worker(p); return NULL; }
#endif

int p2s_scan_lan(p2s_ctx *c, int timeout_ms)
{
    p2s_nic nics[16];
    int nn, i, k, threads = 12, started = 0;
    p2s_lanjob job;
    p2s_lanarg *args;
    p2s_tid_t *tids;

    nn = p2s_list_nics(nics, 16);
    if (nn == 0) {
        p2s_log(c, "[lan] 找不到可用的實體網卡（已排除虛擬網卡）");
        return 0;
    }
    for (i = 0; i < nn; i++)
        p2s_log(c, "[lan] 網卡 %s -> %s / %s", nics[i].name, nics[i].ip, nics[i].mask);

    memset(&job, 0, sizeof(job));
    job.ctx = c;
    job.timeout_ms = timeout_ms > 0 ? timeout_ms : 350;

    /* 每個網卡掃 /24：只掃常見的私有段前綴 */
    for (i = 0; i < nn; i++) {
        char base[32];
        int b1, b2, b3, b4;
        if (sscanf(nics[i].ip, "%d.%d.%d.%d", &b1, &b2, &b3, &b4) != 4) continue;
        snprintf(base, sizeof(base), "%d.%d.%d.", b1, b2, b3);
        for (k = 1; k <= 254; k++) {
            if (job.nhosts >= (int)(sizeof(job.hosts)/sizeof(job.hosts[0]))) break;
            if (k == b4) continue;
            snprintf(job.hosts[job.nhosts], 32, "%s%d", base, k);
            job.nhosts++;
        }
    }
    if (job.nhosts == 0) { p2s_log(c, "[lan] 沒有可掃的網段"); return 0; }

    p2s_log(c, "[lan] 開始掃 %d 個位址的 80/443...", job.nhosts);

#ifdef _WIN32
    InitializeCriticalSection(&job.cs);
#else
    pthread_mutex_init(&job.mu, NULL);
#endif

    args = (p2s_lanarg *)calloc((size_t)threads, sizeof(p2s_lanarg));
    tids = (p2s_tid_t *)calloc((size_t)threads, sizeof(p2s_tid_t));
    if (!args || !tids) { free(args); free(tids); return -1; }

    for (i = 0; i < threads; i++) {
        args[i].job = &job;
#ifdef _WIN32
        tids[i] = CreateThread(NULL, 0, p2s_win_worker, &args[i], 0, NULL);
        if (tids[i]) started++;
#else
        if (pthread_create(&tids[i], NULL, p2s_posix_worker, &args[i]) == 0) started++;
#endif
    }
    for (i = 0; i < started; i++) {
#ifdef _WIN32
        WaitForSingleObject(tids[i], INFINITE);
        CloseHandle(tids[i]);
#else
        pthread_join(tids[i], NULL);
#endif
    }

#ifdef _WIN32
    DeleteCriticalSection(&job.cs);
#else
    pthread_mutex_destroy(&job.mu);
#endif
    free(args);
    free(tids);
    p2s_log(c, "[lan] 區網掃描完成");
    return 0;
}

/* ================= 單一網址 ================= */

int p2s_scan_url(p2s_ctx *c, const char *url)
{
    char host[256];
    const char *p = url;
    int port = 80;
    int is_tls = 0;
    p2s_http h;
    p2s_finding f;

    if (p2s_stristr(p, "https://")) { port = 443; is_tls = 1; p += 8; }
    else if (p2s_stristr(p, "http://")) { p += 7; }
    else if (p2s_stristr(p, "//")) { p += 2; }

    {
        int i = 0;
        while (*p && *p != '/' && *p != ':' && i < (int)sizeof(host) - 1) host[i++] = *p++;
        host[i] = 0;
        if (*p == ':') {                 /* 自訂埠 */
            int q = atoi(p + 1);
            if (q > 0 && q < 65536) { port = q; is_tls = (q == 443); }
        }
    }
    if (!host[0]) { p2s_log(c, "[url] 網址解析失敗：%s", url); return -1; }

    p2s_log(c, "[url] 檢查 %s (%s:%d)", url, host, port);

#ifdef _WIN32
    if (is_tls) p2s_http_probe_tls(host, port, 8000, &h);
    else        p2s_http_probe(host, port, 6000, &h);
    /* https 連不上時，退回明文探測（有些站同時開 80） */
    if (!h.open && !is_tls) p2s_http_probe(host, port, 4000, &h);
#else
    if (is_tls) {
        memset(&h, 0, sizeof(h));
        p2s_log(c, "[url] 注意：Linux 測試版不支援 TLS，https 站請用 Windows 版檢查");
    } else {
        p2s_http_probe(host, port, 6000, &h);
    }
#endif

    memset(&f, 0, sizeof(f));
    snprintf(f.target, sizeof(f.target), "%.120s", host);
    snprintf(f.kind, sizeof(f.kind), "%s", "url");
    f.port = port;

    if (!h.open) {
        f.verdict = P2S_ERROR;
        snprintf(f.detail, sizeof(f.detail), "connection failed");
        snprintf(f.advice, sizeof(f.advice), "check host/port or firewall");
        p2s_add_finding(c, &f);
        p2s_log(c, "[url] 連不上 %s:%d", host, port);
        return 0;
    }
    if (!h.got_reply) {
        f.verdict = P2S_UNKNOWN;
        snprintf(f.detail, sizeof(f.detail), "port open but no plaintext HTTP reply (HTTPS?)");
        snprintf(f.advice, sizeof(f.advice), "try the https:// URL instead");
        p2s_add_finding(c, &f);
        p2s_log(c, "[url] %s:%d 沒有明文 HTTP 回應，可能要走 https", host, port);
        return 0;
    }
    p2s_fill_from_http(&f, &h);
    p2s_add_finding(c, &f);
    p2s_log(c, "[url] %s | Server=%s | X-Powered-By=%s | PHP=%s -> %s",
            host, h.server, h.powered,
            h.phpver[0] ? h.phpver : (f.has_php ? "yes" : "no"),
            p2s_verdict_text(f.verdict));
    return 0;
}

/* ================= 報告 ================= */

int p2s_summary(p2s_ctx *c, char *out, int outlen)
{
    int i, vuln = 0, safe = 0, unk = 0, err = 0;
    for (i = 0; i < c->count; i++) {
        switch (c->items[i].verdict) {
        case P2S_VULN:    vuln++; break;
        case P2S_SAFE:    safe++; break;
        case P2S_UNKNOWN: unk++;  break;
        default:          err++;  break;
        }
    }
    snprintf(out, outlen,
             "檢查 %d 筆 | 受影響 %d | 已修補/無PHP %d | 無法判斷 %d | 錯誤 %d",
             c->count, vuln, safe, unk, err);
    return vuln;
}

int p2s_write_report(p2s_ctx *c, const char *path)
{
    FILE *fp = fopen(path, "wb");
    int i;
    char sum[256];
    time_t now = time(NULL);
    struct tm *lt;

    if (!fp) return -1;
    fprintf(fp, "\xEF\xBB\xBF");   /* UTF-8 BOM：Windows 記事本不亂碼 */

    lt = localtime(&now);
    fprintf(fp, "PHP2Shell (CVE-2026-109618 / CVE-2026-109619) 排查報告\r\n");
    fprintf(fp, "產出時間：%04d-%02d-%02d %02d:%02d:%02d\r\n",
            lt->tm_year + 1900, lt->tm_mon + 1, lt->tm_mday,
            lt->tm_hour, lt->tm_min, lt->tm_sec);
    fprintf(fp, "判定基準：PHP < 8.2.35、< 8.3.36、< 8.4.27、< 8.5.12 皆受影響\r\n");
    p2s_summary(c, sum, (int)sizeof(sum));
    fprintf(fp, "總計：%s\r\n", sum);
    fprintf(fp, "%s\r\n", "------------------------------------------------------------");

    for (i = 0; i < c->count; i++) {
        const p2s_finding *f = &c->items[i];
        fprintf(fp, "[%d] %s (%s%s%d)\r\n", i + 1, f->target, f->kind,
                f->port ? ":" : "", f->port);
        fprintf(fp, "    判定：%s\r\n", p2s_verdict_text(f->verdict));
        if (f->version[0]) fprintf(fp, "    PHP 版本：%s\r\n", f->version);
        if (f->server[0])  fprintf(fp, "    Server：%s\r\n", f->server);
        if (f->powered[0]) fprintf(fp, "    X-Powered-By：%s\r\n", f->powered);
        fprintf(fp, "    說明：%s\r\n", f->detail);
        fprintf(fp, "    建議：%s\r\n\r\n", f->advice);
    }

    fprintf(fp, "%s\r\n", "------------------------------------------------------------");
    fprintf(fp, "修補版本對照：\r\n");
    fprintf(fp, "  PHP 8.5.x -> 8.5.12 以上\r\n");
    fprintf(fp, "  PHP 8.4.x -> 8.4.27 以上\r\n");
    fprintf(fp, "  PHP 8.3.x -> 8.3.36 以上\r\n");
    fprintf(fp, "  PHP 8.2.x -> 8.2.35 以上\r\n");
    fprintf(fp, "  PHP 8.1 及更舊（含 7.x / 5.x）-> 已停止支援，必須升級主版本\r\n");
    fprintf(fp, "\r\n此報告為被動指紋檢查（讀 HTTP 標頭），未進行任何攻擊行為。\r\n");
    fclose(fp);
    return 0;
}
