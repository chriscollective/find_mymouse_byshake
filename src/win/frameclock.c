#include "frameclock.h"

#include <dwmapi.h>
#include <stdlib.h>

#define FALLBACK_SLEEP_MS 16    /* if DwmFlush fails: roughly 60 Hz */
#define SPIN_US           1000  /* DwmFlush back in <1 ms: not a real frame
                                   (display off, nothing to compose) */
#define SPIN_SLEEP_MS     4     /* ...so back off instead of spinning */
#define STOP_WAIT_MS      200   /* DwmFlush returns within one frame */

/* Freed by frameclock_stop() once the worker has exited. If the worker
 * does not exit in time (DwmFlush stuck), ctx is deliberately leaked:
 * the still-running thread keeps using it until it finally returns. */
typedef struct {
    HANDLE stop;
    HWND target;
    UINT msg;
} ClockCtx;

static HANDLE g_thread;
static ClockCtx *g_ctx;
static volatile LONG g_pending;   /* a frame message is queued, unhandled */

/* Microseconds since `since` (QPC; GetTickCount's ~16 ms steps would
 * misread real 144 Hz frames as instant returns). */
static long long elapsed_us(LARGE_INTEGER since, LARGE_INTEGER freq)
{
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    return (now.QuadPart - since.QuadPart) * 1000000LL / freq.QuadPart;
}

static DWORD WINAPI clock_main(LPVOID param)
{
    ClockCtx *ctx = param;
    LARGE_INTEGER freq, before;
    QueryPerformanceFrequency(&freq);
    while (WaitForSingleObject(ctx->stop, 0) == WAIT_TIMEOUT) {
        QueryPerformanceCounter(&before);
        if (FAILED(DwmFlush())) Sleep(FALLBACK_SLEEP_MS);
        else if (elapsed_us(before, freq) < SPIN_US) Sleep(SPIN_SLEEP_MS);
        /* at most one queued frame: a busy UI thread is never flooded */
        if (InterlockedCompareExchange(&g_pending, 1, 0) == 0) {
            if (!PostMessageW(ctx->target, ctx->msg, 0, 0)) InterlockedExchange(&g_pending, 0);
        }
    }
    return 0;   /* ctx is released by frameclock_stop() */
}

int frameclock_start(HWND target, UINT msg)
{
    if (g_thread) return 0;
    ClockCtx *ctx = malloc(sizeof *ctx);
    if (!ctx) return -1;
    ctx->target = target;
    ctx->msg = msg;
    ctx->stop = CreateEventW(NULL, TRUE, FALSE, NULL);
    if (!ctx->stop) {
        free(ctx);
        return -1;
    }
    InterlockedExchange(&g_pending, 0);
    g_thread = CreateThread(NULL, 0, clock_main, ctx, 0, NULL);
    if (!g_thread) {
        CloseHandle(ctx->stop);
        free(ctx);
        return -1;
    }
    g_ctx = ctx;
    return 0;
}

void frameclock_stop(void)
{
    if (!g_thread) return;
    SetEvent(g_ctx->stop);
    if (WaitForSingleObject(g_thread, STOP_WAIT_MS) == WAIT_OBJECT_0) {
        CloseHandle(g_ctx->stop);
        free(g_ctx);
    }                            /* else: leak ctx, the late thread owns it */
    CloseHandle(g_thread);       /* a late thread still exits on its own */
    g_thread = NULL;
    g_ctx = NULL;
}

void frameclock_ack(void)
{
    InterlockedExchange(&g_pending, 0);
}
