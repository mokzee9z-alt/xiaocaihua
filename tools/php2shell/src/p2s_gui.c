/*
 * p2s_gui.c — PHP2Shell 排查工具（Windows GUI，Unicode）
 * 編譯：
 *   x86_64-w64-mingw32-gcc -O2 -mwindows -municode -o php2shell_check.exe \
 *     p2s_core.c p2s_gui.c -lws2_32 -liphlpapi -lwinhttp -lcomctl32
 */
#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "p2s_core.h"

#define WM_APP_LOG      (WM_APP + 1)
#define WM_APP_DONE     (WM_APP + 2)

#define IDC_BTN_SCAN    1001
#define IDC_BTN_URL     1002
#define IDC_EDIT_URL    1003
#define IDC_EDIT_LOG    1004
#define IDC_BTN_SAVE    1005
#define IDC_BTN_OPEN    1006
#define IDC_STATIC_HDR  1007

static HWND g_hwnd, g_log, g_url, g_btnScan, g_btnUrl, g_btnSave, g_btnOpen;
static HANDLE g_thread = NULL;
static p2s_ctx g_ctx;
static wchar_t g_reportPath[MAX_PATH];
static volatile LONG g_running = 0;

/* ---------- UTF-8 ↔ UTF-16 ---------- */
static wchar_t *p2s_u8_to_w(const char *s)
{
    int n = MultiByteToWideChar(CP_UTF8, 0, s, -1, NULL, 0);
    wchar_t *w;
    if (n <= 0) return NULL;
    w = (wchar_t *)calloc((size_t)n, sizeof(wchar_t));
    if (!w) return NULL;
    MultiByteToWideChar(CP_UTF8, 0, s, -1, w, n);
    return w;
}

/* ---------- 附加一行到結果區 ---------- */
static void append_line(const wchar_t *w)
{
    int len = GetWindowTextLengthW(g_log);
    SendMessageW(g_log, EM_SETSEL, (WPARAM)len, (LPARAM)len);
    SendMessageW(g_log, EM_REPLACESEL, FALSE, (LPARAM)w);
    SendMessageW(g_log, EM_REPLACESEL, FALSE, (LPARAM)L"\r\n");
    SendMessageW(g_log, EM_SCROLLCARET, 0, 0);
}

static void append_utf8(const char *s)
{
    wchar_t *w = p2s_u8_to_w(s);
    if (w) {
        append_line(w);
        free(w);
    }
}

/* ---------- 背景執行緒的回呼（只 PostMessage，不碰 UI） ---------- */
static void gui_on_log(const char *utf8)
{
    char *dup = _strdup(utf8);
    if (dup) PostMessageW(g_hwnd, WM_APP_LOG, 0, (LPARAM)dup);
}

static void gui_on_finding(const p2s_finding *f)
{
    char line[512];
    const char *mark;
    switch (f->verdict) {
    case P2S_VULN:    mark = "[!] 受影響 "; break;
    case P2S_SAFE:    mark = "[OK] 安全  "; break;
    case P2S_UNKNOWN: mark = "[?] 待確認"; break;
    default:          mark = "[x] 錯誤  "; break;
    }
    snprintf(line, sizeof(line), "  %s %s%s%d%s%s", mark, f->target,
             f->port ? ":" : "", f->port,
             f->version[0] ? "  PHP " : "",
             f->version[0] ? f->version : "");
    gui_on_log(line);
}

/* ---------- 掃描工作 ---------- */
static DWORD WINAPI scan_worker(LPVOID param)
{
    int mode = (int)(INT_PTR)param;

    p2s_ctx_init(&g_ctx);
    g_ctx.on_log = gui_on_log;
    g_ctx.on_finding = gui_on_finding;

    if (mode == 1) {
        gui_on_log("==== 開始全面掃描（本機 + 區網）====");
        p2s_scan_local(&g_ctx);
        p2s_scan_lan(&g_ctx, 350);
    } else if (mode == 2) {
        wchar_t wurl[512];
        char url[768];
        int n;
        GetWindowTextW(g_url, wurl, 512);
        n = WideCharToMultiByte(CP_UTF8, 0, wurl, -1, url, (int)sizeof(url), NULL, NULL);
        if (n <= 0 || url[0] == 0) {
            gui_on_log("請先在上面輸入網址，例如 http://example.com/");
            PostMessageW(g_hwnd, WM_APP_DONE, 0, 0);
            return 0;
        }
        gui_on_log("==== 檢查單一網址 ====");
        p2s_scan_url(&g_ctx, url);
    }

    PostMessageW(g_hwnd, WM_APP_DONE, 0, 0);
    return 0;
}

static void start_scan(int mode)
{
    if (InterlockedCompareExchange(&g_running, 1, 0) != 0) {
        append_line(L"還在跑，等它跑完。");
        return;
    }
    EnableWindow(g_btnScan, FALSE);
    EnableWindow(g_btnUrl, FALSE);
    SetWindowTextW(g_log, L"");
    g_thread = CreateThread(NULL, 0, scan_worker, (LPVOID)(INT_PTR)mode, 0, NULL);
    if (!g_thread) {
        InterlockedExchange(&g_running, 0);
        EnableWindow(g_btnScan, TRUE);
        EnableWindow(g_btnUrl, TRUE);
        append_line(L"執行緒啟動失敗");
    }
}

/* ---------- 存報告 ---------- */
static void save_report(void)
{
    char path[1024];
    char sum[256];
    SYSTEMTIME st;
    int n;

    GetLocalTime(&st);
    snprintf(path, sizeof(path),
             "%ls\\php2shell_report_%04d%02d%02d_%02d%02d%02d.txt",
             g_reportPath, st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
    n = (int)strlen(path);

    if (p2s_write_report(&g_ctx, path) == 0) {
        wchar_t *w = p2s_u8_to_w(path);
        p2s_summary(&g_ctx, sum, (int)sizeof(sum));
        if (w) {
            wchar_t msg[1200];
            _snwprintf(msg, 1200, L"報告已存到：\r\n%s", w);
            MessageBoxW(g_hwnd, msg, L"已儲存", MB_OK | MB_ICONINFORMATION);
            free(w);
        }
        (void)n;
    } else {
        MessageBoxW(g_hwnd, L"報告寫入失敗（可能資料夾沒有寫入權限，換放桌面試試）",
                    L"錯誤", MB_OK | MB_ICONERROR);
    }
}

/* ---------- 開報告資料夾 ---------- */
static void open_folder(void)
{
    ShellExecuteW(NULL, L"open", g_reportPath, NULL, NULL, SW_SHOWNORMAL);
}

/* ---------- 視窗程序 ---------- */
static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg) {
    case WM_CREATE: {
        HFONT font = (HFONT)GetStockObject(DEFAULT_GUI_FONT);

        CreateWindowW(L"STATIC",
            L"PHP2Shell 排查工具  (CVE-2026-109618 / CVE-2026-109619)",
            WS_CHILD | WS_VISIBLE, 12, 10, 720, 20, hwnd, (HMENU)IDC_STATIC_HDR, NULL, NULL);

        CreateWindowW(L"STATIC",
            L"判定：PHP 8.5 < 8.5.12 / 8.4 < 8.4.27 / 8.3 < 8.3.36 / 8.2 < 8.2.35 皆受影響，8.1 及更舊全中",
            WS_CHILD | WS_VISIBLE, 12, 32, 760, 18, hwnd, NULL, NULL, NULL);

        g_btnScan = CreateWindowW(L"BUTTON", L"▶ 開始全面掃描（本機＋區網）",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            12, 58, 260, 38, hwnd, (HMENU)IDC_BTN_SCAN, NULL, NULL);

        g_url = CreateWindowW(L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
            286, 58, 300, 24, hwnd, (HMENU)IDC_EDIT_URL, NULL, NULL);
        SendMessageW(g_url, WM_SETTEXT, 0, (LPARAM)L" http://");

        g_btnUrl = CreateWindowW(L"BUTTON", L"檢查這個網址",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            592, 58, 150, 26, hwnd, (HMENU)IDC_BTN_URL, NULL, NULL);

        g_btnSave = CreateWindowW(L"BUTTON", L"💾 儲存報告",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            12, 104, 150, 30, hwnd, (HMENU)IDC_BTN_SAVE, NULL, NULL);

        g_btnOpen = CreateWindowW(L"BUTTON", L"📁 開啟報告資料夾",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            170, 104, 180, 30, hwnd, (HMENU)IDC_BTN_OPEN, NULL, NULL);

        g_log = CreateWindowW(L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL |
            ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL,
            12, 142, 730, 380, hwnd, (HMENU)IDC_EDIT_LOG, NULL, NULL);

        SendMessageW(g_btnScan, WM_SETFONT, (WPARAM)font, TRUE);
        SendMessageW(g_btnUrl, WM_SETFONT, (WPARAM)font, TRUE);
        SendMessageW(g_btnSave, WM_SETFONT, (WPARAM)font, TRUE);
        SendMessageW(g_btnOpen, WM_SETFONT, (WPARAM)font, TRUE);
        SendMessageW(g_log, WM_SETFONT, (WPARAM)font, TRUE);
        SendMessageW(g_url, WM_SETFONT, (WPARAM)font, TRUE);

        append_line(L"按左上角「開始全面掃描」，會自動做完：");
        append_line(L"  1. 檢查這台電腦有沒有裝 PHP、版本多少");
        append_line(L"  2. 檢查這台電腦自己的網站服務（80/443/8080…）");
        append_line(L"  3. 掃描同一個區網所有機器的網站，看誰的 PHP 有問題");
        append_line(L"  4. 也可以在上面輸入單一網址，只查那台");
        append_line(L"");
        append_line(L"全程只讀取網頁標頭（被動指紋），不會對任何機器做攻擊動作。");
        append_line(L"");
        return 0;
    }

    case WM_APP_LOG: {
        char *s = (char *)lp;
        if (s) {
            append_utf8(s);
            free(s);
        }
        return 0;
    }

    case WM_APP_DONE: {
        char sum[256];
        int vuln;
        vuln = p2s_summary(&g_ctx, sum, (int)sizeof(sum));
        append_line(L"");
        append_utf8(sum);
        if (vuln > 0) {
            wchar_t wmsg[512];
            append_line(L"");
            append_line(L"===== 有受影響的目標，建議立刻升級 PHP =====");
            _snwprintf(wmsg, 512,
                L"掃到 %d 個受影響的目標。\r\n報告已自動存檔，要現在看資料夾嗎？", vuln);
            if (MessageBoxW(hwnd, wmsg, L"發現受影響目標",
                            MB_YESNO | MB_ICONWARNING) == IDYES)
                open_folder();
        }
        if (g_thread) { CloseHandle(g_thread); g_thread = NULL; }
        InterlockedExchange(&g_running, 0);
        EnableWindow(g_btnScan, TRUE);
        EnableWindow(g_btnUrl, TRUE);
        return 0;
    }

    case WM_COMMAND:
        switch (LOWORD(wp)) {
        case IDC_BTN_SCAN: start_scan(1); return 0;
        case IDC_BTN_URL:  start_scan(2); return 0;
        case IDC_BTN_SAVE: save_report(); return 0;
        case IDC_BTN_OPEN: open_folder(); return 0;
        }
        return 0;

    case WM_CLOSE:
        if (InterlockedCompareExchange(&g_running, 0, 0) != 0) {
            if (MessageBoxW(hwnd, L"掃描還在進行，要中斷嗎？", L"確認",
                            MB_YESNO | MB_ICONQUESTION) != IDYES)
                return 0;
        }
        DestroyWindow(hwnd);
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE hPrev, LPWSTR cmd, int show)
{
    WNDCLASSW wc;
    MSG msg;
    DWORD pid;
    wchar_t exe[MAX_PATH];
    wchar_t *slash;

    (void)hPrev; (void)cmd; (void)show;

    p2s_net_init();
    p2s_ctx_init(&g_ctx);

    /* 報告存到 exe 同一層（隨身碟根目錄） */
    GetModuleFileNameW(NULL, exe, MAX_PATH);
    wcscpy(g_reportPath, exe);
    slash = wcsrchr(g_reportPath, L'\\');
    if (slash) *slash = 0;
    else wcscpy(g_reportPath, L".");

    /* 同一時間只跑一個 */
    CreateMutexW(NULL, TRUE, L"PHP2ShellCheck_Mutex");
    if (GetLastError() == ERROR_ALREADY_EXISTS && !IsDebuggerPresent()) {
        MessageBoxW(NULL, L"這個程式已經開著了。", L"PHP2Shell 排查", MB_OK | MB_ICONINFORMATION);
        return 0;
    }
    pid = GetCurrentProcessId();
    (void)pid;

    InitCommonControls();

    memset(&wc, 0, sizeof(wc));
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"PHP2ShellCheckWnd";
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    RegisterClassW(&wc);

    g_hwnd = CreateWindowW(L"PHP2ShellCheckWnd",
        L"PHP2Shell 排查工具 — 檢查 PHP 是否有 CVE-2026-109618 / 109619",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 780, 580,
        NULL, NULL, hInst, NULL);
    if (!g_hwnd) return 1;

    ShowWindow(g_hwnd, SW_SHOW);
    UpdateWindow(g_hwnd);

    while (GetMessageW(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    p2s_net_cleanup();
    return 0;
}
