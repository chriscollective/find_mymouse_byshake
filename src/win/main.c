/* Entry point: hidden window that listens to raw mouse input, runs the
 * shake detector and animates the enlarged cursor. */
#include "app.h"

#include "anim.h"
#include "config.h"
#include "frameclock.h"
#include "overlay.h"
#include "syscursor.h"
#include "throttle.h"
#include "tray.h"

#include <string.h>
#include <wchar.h>

#define TIMER_ANIM        1
#define TIMER_TRAY_RETRY  2
#define TIMER_CURSOR_RETRY 3
#define CURSOR_RETRY_MS   500
#define EXIT_RESTORE_TRIES   5
#define EXIT_RESTORE_WAIT_MS 50
#define FRAME_MS          15
#define WATCHDOG_MS       50       /* backstop if vsync ticks stall */
#define TRAY_RETRY_MS     3000
#define HOLD_MS           300      /* stay big this long after the last shake */
#define MOVE_MIN_US       4000     /* throttle for following the cursor */
#define MUTEX_NAME        L"Local\\find_mymouse_byshake.single"

static App g_app;

/* Thresholds are in physical pixels, so scale them for the DPI of the
 * monitor the cursor is on (mixed-DPI setups differ per monitor). */
static void apply_detector_config(App *a, UINT dpi)
{
    ShakeConfig c = settings_shake_config(a->sens, dpi);
    shake_init(&a->det, &c);
    a->det_dpi = dpi;
}

static void save_settings(const App *a)
{
    Config c = config_load();
    c.enabled = a->enabled;
    c.sensitivity = (int)a->sens;
    c.size = (int)a->size;
    c.color = (int)a->color;
    if (config_save(&c) != 0) {
        tray_balloon(a, L"設定未儲存", L"無法寫入設定，下次啟動時會恢復成原本的設定。");
    }
}

static int cursor_visible(void)
{
    CURSORINFO ci;
    ci.cbSize = sizeof ci;
    if (!GetCursorInfo(&ci)) return 1;
    return (ci.flags & CURSOR_SHOWING) != 0;
}

/* Millisecond clock with sub-ms precision (GetTickCount steps ~16 ms,
 * which made the shrink animation advance unevenly). */
static double precise_ms(void)
{
    static LARGE_INTEGER freq;
    LARGE_INTEGER now;
    if (freq.QuadPart == 0) QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&now);
    return (double)now.QuadPart * 1000.0 / (double)freq.QuadPart;
}

/* Hide the real cursor, at most one failed attempt per effect. */
static void try_hide(App *a)
{
    if (!a->hide_failed && syscursor_hide() != 0) a->hide_failed = 1;
}

/* Returns 1 once the real cursor is visible again. Failed restores are
 * retried at most every CURSOR_RETRY_MS, not on every frame. The back-off
 * only applies after a real failure (a bare timestamp compare would
 * misfire once GetTickCount passes 2^31, i.e. after ~24.8 days uptime). */
static int try_restore(App *a, DWORD now)
{
    if (!syscursor_is_hidden()) {
        a->restore_backoff = 0;
        return 1;
    }
    if (a->restore_backoff && now - a->restore_failed_at < CURSOR_RETRY_MS) return 0;
    if (syscursor_restore() == 0) {
        a->restore_backoff = 0;
        return 1;
    }
    a->restore_backoff = 1;
    a->restore_failed_at = now;
    return 0;
}

/* Frame ticks synced to the display. A slow timer always runs beside
 * them: ticks are time-based, so extra ones are harmless, and if vsync
 * ticks ever stall (display off, DwmFlush stuck) the effect still ends
 * on time and the real cursor still comes back. */
static void start_frames(App *a)
{
    int vsync = frameclock_start(a->hwnd, WM_APP_FRAME) == 0;
    SetTimer(a->hwnd, TIMER_ANIM, vsync ? WATCHDOG_MS : FRAME_MS, NULL);
}

static void stop_frames(App *a)
{
    frameclock_stop();
    KillTimer(a->hwnd, TIMER_ANIM);
}

static void start_effect(App *a)
{
    DWORD now = GetTickCount();
    a->last_shake = now;
    a->target = settings_max_scale(a->size);
    if (a->animating) return;
    a->animating = 1;
    a->hide_failed = 0;
    a->scale = 1.0f;
    a->last_frame_ms = precise_ms();
    start_frames(a);
    POINT p;
    if (GetCursorPos(&p)) overlay_show(a->scale, a->color, 0, p);
    try_hide(a);        /* after the overlay is up: no frame without a cursor */
}

/* Restoring the cursors failed (rare): keep retrying until it works. */
static void restore_cursor_or_retry(App *a)
{
    if (syscursor_restore() != 0) {
        SetTimer(a->hwnd, TIMER_CURSOR_RETRY, CURSOR_RETRY_MS, NULL);
    }
}

/* On exit no timer will fire again: retry synchronously a few times. If
 * it still fails, the kept flag makes the next start restore them. */
static void restore_cursor_before_exit(void)
{
    for (int i = 0; i < EXIT_RESTORE_TRIES && syscursor_restore() != 0; i++) {
        Sleep(EXIT_RESTORE_WAIT_MS);
    }
}

static void stop_effect(App *a)
{
    stop_frames(a);
    a->animating = 0;
    a->scale = 1.0f;
    a->target = 1.0f;
    restore_cursor_or_retry(a);  /* before the overlay goes: never cursorless */
    overlay_hide();
}

/* Follow the cursor between animation frames, at most every MOVE_MIN_US. */
static void follow_cursor(POINT p)
{
    static LARGE_INTEGER freq, last;
    LARGE_INTEGER now;
    if (freq.QuadPart == 0) QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&now);
    if (!throttle_due(now.QuadPart, last.QuadPart, freq.QuadPart, MOVE_MIN_US)) return;
    last = now;
    overlay_move(p);
}

/* Growing: exponential ease toward the target (re-shakes blend in).
 * Shrinking: a timed curve that lands exactly on 1.0 -- no slow crawl. */
static void advance_scale(App *a, DWORD now, double t)
{
    if (a->target > 1.0f && now - a->last_shake > HOLD_MS) {
        a->target = 1.0f;
        a->shrink_from = a->scale;
        a->shrink_t0 = t;
    }
    if (a->target > 1.0f) {
        a->scale = anim_step(a->scale, a->target, (float)(t - a->last_frame_ms));
    } else {
        a->scale = anim_shrink_scale(a->shrink_from, (float)(t - a->shrink_t0));
    }
    a->last_frame_ms = t;
}

static void on_anim_tick(App *a)
{
    DWORD now = GetTickCount();
    double t = precise_ms();
    advance_scale(a, now, t);
    int landed = a->target <= 1.0f && t - a->shrink_t0 >= ANIM_SHRINK_MS;
    /* Shrinking tail: the real cursor comes back on top and the arrow fades
     * out beneath it -- but only once the cursor really is back; until
     * then the arrow stays opaque so there is never a cursorless frame. */
    int handoff = !anim_hide_real_cursor(a->scale, a->target) && try_restore(a, now);
    if (handoff && landed) {
        stop_effect(a);
        return;
    }
    POINT p;
    if (GetCursorPos(&p)) overlay_show(a->scale, a->color, handoff, p);
    if (!handoff) try_hide(a);   /* after the opaque arrow is up (re-shake) */
}

static void on_mouse_input(App *a)
{
    if (!a->enabled) return;
    POINT p;
    if (!GetCursorPos(&p)) return;   /* e.g. secure desktop */
    if (a->animating) follow_cursor(p);
    UINT dpi = dpi_at(p);
    if (dpi != a->det_dpi) apply_detector_config(a, dpi);
    if (!shake_feed(&a->det, p.x, p.y, GetTickCount())) return;
    /* games and video players hide the cursor: don't draw over them */
    if (a->animating || cursor_visible()) start_effect(a);
}

static void on_command(App *a, int id)
{
    if (id == IDM_ENABLE) {
        a->enabled = !a->enabled;
        if (!a->enabled) stop_effect(a);
    } else if (id >= IDM_SENS_LOW && id <= IDM_SENS_HIGH) {
        a->sens = settings_clamp_sensitivity(id - IDM_SENS_LOW);
        apply_detector_config(a, a->det_dpi);
    } else if (id >= IDM_SIZE_SMALL && id <= IDM_SIZE_LARGE) {
        a->size = settings_clamp_size(id - IDM_SIZE_SMALL);
    } else if (id == IDM_COLOR_BLACK || id == IDM_COLOR_WHITE) {
        a->color = settings_clamp_color(id - IDM_COLOR_BLACK);
    } else if (id == IDM_AUTOSTART) {
        if (autostart_set(!autostart_enabled()) != 0) {
            tray_balloon(a, L"設定失敗", L"無法變更開機自動啟動，請稍後再試。");
        }
        return;
    } else if (id == IDM_TEST) {
        start_effect(a);
        return;
    } else if (id == IDM_EXIT) {
        DestroyWindow(a->hwnd);
        return;
    } else {
        return;
    }
    save_settings(a);
}

static void add_tray_or_retry(App *a)
{
    if (tray_add(a) != 0) SetTimer(a->hwnd, TIMER_TRAY_RETRY, TRAY_RETRY_MS, NULL);
}

static void on_timer(App *a, WPARAM id)
{
    if (id == TIMER_ANIM) {
        on_anim_tick(a);
    } else if (id == TIMER_TRAY_RETRY && tray_add(a) == 0) {
        KillTimer(a->hwnd, TIMER_TRAY_RETRY);
    } else if (id == TIMER_CURSOR_RETRY) {
        /* a new effect started meanwhile: it owns the hidden cursor now */
        if (a->animating || syscursor_restore() == 0) KillTimer(a->hwnd, TIMER_CURSOR_RETRY);
    }
}

static LRESULT CALLBACK wnd_proc(HWND h, UINT m, WPARAM w, LPARAM l)
{
    App *a = &g_app;
    if (a->msg_taskbar_created != 0 && m == a->msg_taskbar_created) {
        add_tray_or_retry(a);        /* Explorer restarted */
        return 0;
    }
    switch (m) {
    case WM_INPUT:
        on_mouse_input(a);
        break;                       /* DefWindowProc must still run */
    case WM_APP_FRAME:
        frameclock_ack();
        if (a->animating) on_anim_tick(a);
        return 0;
    case WM_TIMER:
        on_timer(a, w);
        return 0;
    case WM_APP_TRAY:
        if (l == WM_LBUTTONUP || l == WM_RBUTTONUP) tray_show_menu(a);
        return 0;
    case WM_COMMAND:
        on_command(a, LOWORD(w));
        return 0;
    case WM_APP_PING:
        tray_balloon(a, L"已經在執行中", L"右鍵點系統匣的游標圖示，可以調整設定或結束。");
        return 0;
    case WM_ENDSESSION:
        if (w) stop_effect(a);       /* logoff / shutdown: give the cursor back */
        return 0;
    case WM_DESTROY:
        stop_effect(a);
        restore_cursor_before_exit();
        tray_remove(a);
        overlay_shutdown();
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(h, m, w, l);
}

static int create_main_window(App *a)
{
    WNDCLASSEXW wc;
    memset(&wc, 0, sizeof wc);
    wc.cbSize = sizeof wc;
    wc.lpfnWndProc = wnd_proc;
    wc.hInstance = a->inst;
    wc.lpszClassName = WND_CLASS;
    if (!RegisterClassExW(&wc)) return -1;
    /* top-level (not message-only) so it receives TaskbarCreated */
    a->hwnd = CreateWindowExW(0, WND_CLASS, APP_TITLE, WS_OVERLAPPED, 0, 0, 0, 0,
                              NULL, NULL, a->inst, NULL);
    return a->hwnd ? 0 : -1;
}

static int register_raw_mouse(HWND hwnd)
{
    RAWINPUTDEVICE rid;
    rid.usUsagePage = 0x01;          /* generic desktop */
    rid.usUsage = 0x02;              /* mouse */
    rid.dwFlags = RIDEV_INPUTSINK;   /* receive input in the background */
    rid.hwndTarget = hwnd;
    return RegisterRawInputDevices(&rid, 1, sizeof rid) ? 0 : -1;
}

/* If we run elevated, UIPI would drop these messages coming from the
 * normal-integrity Explorer / a non-elevated second instance. */
static void allow_messages_from_explorer(const App *a)
{
    const UINT msgs[] = { a->msg_taskbar_created, WM_APP_PING, WM_CLOSE };
    for (size_t i = 0; i < sizeof msgs / sizeof msgs[0]; i++) {
        if (msgs[i] != 0) ChangeWindowMessageFilterEx(a->hwnd, msgs[i], MSGFLT_ALLOW, NULL);
    }
}

static void first_run_or_refresh(App *a, Config *c)
{
    if (c->welcomed) {
        autostart_refresh_path();
        return;
    }
    int autostart_ok = autostart_set(1) == 0;
    c->welcomed = 1;
    c->sensitivity = (int)a->sens;
    c->size = (int)a->size;
    c->color = (int)a->color;
    int saved = config_save(c) == 0;
    tray_balloon(a, L"已在背景執行",
                 autostart_ok && saved
                     ? L"快速左右搖晃滑鼠，游標就會放大。\n右鍵點系統匣的游標圖示可以調整設定。"
                     : L"快速左右搖晃滑鼠，游標就會放大。\n（注意：開機自動啟動或設定沒有寫入成功）");
}

static int init_app(HINSTANCE inst)
{
    App *a = &g_app;
    syscursor_install_crash_guard();
    syscursor_recover();             /* last run died with the cursor hidden */
    Config c = config_load();
    a->inst = inst;
    a->enabled = c.enabled;
    a->sens = settings_clamp_sensitivity(c.sensitivity);
    a->size = settings_clamp_size(c.size);
    a->color = settings_clamp_color(c.color);
    a->scale = a->target = 1.0f;
    apply_detector_config(a, GetDpiForSystem());
    a->msg_taskbar_created = RegisterWindowMessageW(L"TaskbarCreated");
    if (create_main_window(a) != 0 || overlay_init(inst) != 0) return -1;
    if (register_raw_mouse(a->hwnd) != 0) return -1;
    allow_messages_from_explorer(a);
    add_tray_or_retry(a);
    first_run_or_refresh(a, &c);
    return 0;
}

/* Another instance is running: ask it to quit, or to say hello. */
static void notify_existing(int quit)
{
    HWND other = FindWindowW(WND_CLASS, NULL);
    if (other) PostMessageW(other, quit ? WM_CLOSE : WM_APP_PING, 0, 0);
}

int WINAPI wWinMain(HINSTANCE inst, HINSTANCE prev, PWSTR cmd, int show)
{
    (void)prev;
    (void)show;
    int quit = cmd && wcsstr(cmd, L"--quit") != NULL;
    HANDLE mutex = CreateMutexW(NULL, TRUE, MUTEX_NAME);
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        if (mutex) CloseHandle(mutex);
        notify_existing(quit);
        return 0;
    }
    int rc = 0;
    if (!quit) {
        if (init_app(inst) != 0) {
            MessageBoxW(NULL, L"啟動失敗，請重新開機後再試一次。", APP_TITLE, MB_ICONERROR);
            rc = 1;
        } else {
            MSG msg;
            while (GetMessageW(&msg, NULL, 0, 0) > 0) {
                TranslateMessage(&msg);
                DispatchMessageW(&msg);
            }
        }
    } else {
        syscursor_recover();   /* --quit with nothing running: still tidy up */
    }
    if (mutex) {
        ReleaseMutex(mutex);
        CloseHandle(mutex);
    }
    return rc;
}
