/*
 * p2s_cli.c — 命令列版（開發測試用；交付給阿本的是 GUI 版）
 * 用法：
 *   p2s_cli local              只掃本機
 *   p2s_cli lan                掃區網
 *   p2s_cli url http://x/      掃單一網址
 *   p2s_cli all                本機 + 區網
 *   p2s_cli ver 8.4.26         只測版本判定
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "p2s_core.h"

static void cli_log(const char *s)
{
    printf("%s\n", s);
    fflush(stdout);
}

int main(int argc, char **argv)
{
    p2s_ctx ctx;
    const char *mode = argc > 1 ? argv[1] : "all";
    char sum[256];

    p2s_ctx_init(&ctx);
    ctx.on_log = cli_log;
    p2s_net_init();

    if (strcmp(mode, "ver") == 0) {
        const char *v = argc > 2 ? argv[2] : "";
        printf("%s -> %s\n", v, p2s_verdict_text(p2s_verdict_for(v)));
        return 0;
    }

    printf("PHP2Shell 排查 (CVE-2026-109618 / CVE-2026-109619)\n");
    printf("判定基準: PHP <8.2.35 / <8.3.36 / <8.4.27 / <8.5.12 皆受影響\n");
    printf("------------------------------------------------------------\n");

    if (strcmp(mode, "local") == 0 || strcmp(mode, "all") == 0)
        p2s_scan_local(&ctx);

    if (strcmp(mode, "lan") == 0 || strcmp(mode, "all") == 0)
        p2s_scan_lan(&ctx, 350);

    if (strcmp(mode, "url") == 0) {
        if (argc > 2) p2s_scan_url(&ctx, argv[2]);
        else printf("用法: p2s_cli url http://example.com/\n");
    }

    printf("------------------------------------------------------------\n");
    p2s_summary(&ctx, sum, (int)sizeof(sum));
    printf("%s\n", sum);

    if (p2s_write_report(&ctx, "php2shell_report.txt") == 0)
        printf("報告已寫入 php2shell_report.txt\n");

    return 0;
}
