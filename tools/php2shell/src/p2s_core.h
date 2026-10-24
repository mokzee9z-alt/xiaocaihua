/*
 * p2s_core.h — PHP2Shell (CVE-2026-109618 / CVE-2026-109619) 排查核心
 *
 * 判定依據（NVD 2026-10-23，versionEndExcluding）：
 *   5.4.0 ~ <8.2.35  受影響
 *   8.3.0 ~ <8.3.36  受影響
 *   8.4.0 ~ <8.4.27  受影響
 *   8.5.0 ~ <8.5.12  受影響
 *   修補版 = 8.2.35 / 8.3.36 / 8.4.27 / 8.5.12
 *   <=8.1.x、8.0.x、7.x、5.x 全部受影響且官方已 EOL，只能升級
 */
#ifndef P2S_CORE_H
#define P2S_CORE_H

#define P2S_MAX_LINES   2000
#define P2S_LINE_MAX    512
#define P2S_MAX_HOSTS   256

/* 判定結果 */
#define P2S_SAFE      0   /* 沒有 PHP 或版本已修補 */
#define P2S_VULN      1   /* 確定受影響 */
#define P2S_UNKNOWN   2   /* 有 PHP 但看不到版本 */
#define P2S_ERROR     3

typedef struct {
    char target[128];      /* 來源：本機 / IP / URL */
    char kind[24];         /* local / lan / url */
    int  has_php;
    char version[32];
    char detail[256];
    int  verdict;          /* P2S_* */
    char advice[160];
    int  port;
    char server[128];      /* Server header */
    char powered[96];      /* X-Powered-By */
} p2s_finding;

typedef struct {
    p2s_finding items[P2S_MAX_HOSTS];
    int count;
    char logline[P2S_MAX_LINES][P2S_LINE_MAX];
    int  logcount;
    void (*on_log)(const char *utf8);   /* GUI 傳入；NULL=只收集 */
    void (*on_finding)(const p2s_finding *f);
} p2s_ctx;

/* 版本解析與判定：回傳 P2S_SAFE / P2S_VULN / P2S_UNKNOWN */
int  p2s_parse_version(const char *s, int *maj, int *min, int *pat);
int  p2s_verdict_for(const char *ver);
const char *p2s_verdict_text(int v);
const char *p2s_fix_for(const char *ver);

/* 掃描動作 */
void p2s_ctx_init(p2s_ctx *c);
void p2s_net_init(void);
void p2s_net_cleanup(void);
void p2s_log(p2s_ctx *c, const char *fmt, ...);
void p2s_add_finding(p2s_ctx *c, const p2s_finding *f);

int  p2s_scan_local(p2s_ctx *c);                 /* 本機 PHP 安裝 + 本機 HTTP */
int  p2s_scan_lan(p2s_ctx *c, int timeout_ms);   /* 區網 /24 的 80/443 */
int  p2s_scan_url(p2s_ctx *c, const char *url);  /* 單一網址 */

/* 報告 */
int  p2s_write_report(p2s_ctx *c, const char *path_utf8);
int  p2s_summary(p2s_ctx *c, char *out, int outlen);

#endif /* P2S_CORE_H */
