#include "overlay.h"

#include "anim.h"
#include "arrow.h"

#include <shellscalingapi.h>
#include <string.h>

#define OVERLAY_CLASS L"find_mymouse_byshake.overlay"
#define LEVELS        800      /* cached scale steps: 1.00 .. 8.99 */
#define LEVEL_STEP    0.01f    /* fine steps: no visible size / rim jumps */
#define BASE_DPI      96.0f

typedef struct {
    HBITMAP bmp;
    int w, h, hot_x, hot_y;
} Frame;

static HWND g_wnd;
static Frame g_frames[LEVELS];
static UINT g_dpi;
static ArrowColor g_color;   /* colour the cached frames were rendered in */
static int g_sys_shadow;     /* system pointer shadow on? read per effect */
static int g_handoff;        /* cached frames belong to the hand-off phase? */
static int g_visible;
static POINT g_hot;          /* hotspot offset of the frame on screen */

static LRESULT CALLBACK overlay_proc(HWND h, UINT m, WPARAM w, LPARAM l)
{
    if (m == WM_NCHITTEST) return HTTRANSPARENT;
    return DefWindowProcW(h, m, w, l);
}

int overlay_init(HINSTANCE inst)
{
    WNDCLASSEXW wc;
    memset(&wc, 0, sizeof wc);
    wc.cbSize = sizeof wc;
    wc.lpfnWndProc = overlay_proc;
    wc.hInstance = inst;
    wc.lpszClassName = OVERLAY_CLASS;
    if (!RegisterClassExW(&wc)) return -1;
    DWORD ex = WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST |
               WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE;
    g_wnd = CreateWindowExW(ex, OVERLAY_CLASS, L"", WS_POPUP, 0, 0, 1, 1,
                            NULL, NULL, inst, NULL);
    return g_wnd ? 0 : -1;
}

UINT dpi_at(POINT pt)
{
    HMONITOR mon = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);
    UINT dx = (UINT)BASE_DPI, dy = (UINT)BASE_DPI;
    if (FAILED(GetDpiForMonitor(mon, MDT_EFFECTIVE_DPI, &dx, &dy))) return (UINT)BASE_DPI;
    return dx;
}

static void clear_cache(void)
{
    for (int i = 0; i < LEVELS; i++) {
        if (g_frames[i].bmp) DeleteObject(g_frames[i].bmp);
    }
    memset(g_frames, 0, sizeof g_frames);
}

static int make_frame(Frame *f, float ppu, const ArrowStyle *style)
{
    ArrowImage img;
    if (arrow_render(ppu, style, &img) != 0) return -1;
    BITMAPINFO bi;
    memset(&bi, 0, sizeof bi);
    bi.bmiHeader.biSize = sizeof bi.bmiHeader;
    bi.bmiHeader.biWidth = img.width;
    bi.bmiHeader.biHeight = -img.height;   /* top-down rows */
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;
    void *bits = NULL;
    HBITMAP bmp = CreateDIBSection(NULL, &bi, DIB_RGB_COLORS, &bits, NULL, 0);
    if (bmp) {
        memcpy(bits, img.pixels, (size_t)img.width * (size_t)img.height * 4u);
        f->bmp = bmp;
        f->w = img.width;
        f->h = img.height;
        f->hot_x = img.hot_x;
        f->hot_y = img.hot_y;
    }
    arrow_free(&img);
    return bmp ? 0 : -1;
}

/* The look of a level depends on (dpi, colour, hand-off phase): any
 * change of those invalidates the cache. */
static const Frame *frame_for(float scale, ArrowColor color, int handoff, UINT dpi)
{
    if (dpi != g_dpi || color != g_color || handoff != g_handoff) {
        clear_cache();
        g_dpi = dpi;
        g_color = color;
        g_handoff = handoff;
    }
    /* hand-off: round down, so the rim is never stronger than it should be */
    float lvf = (scale - 1.0f) / LEVEL_STEP;
    int lv = (int)(handoff ? lvf : lvf + 0.5f);
    if (lv < 0) lv = 0;
    if (lv >= LEVELS) lv = LEVELS - 1;
    Frame *f = &g_frames[lv];
    float scale_q = 1.0f + (float)lv * LEVEL_STEP;
    float ppu = scale_q * (float)dpi / BASE_DPI;
    ArrowStyle style;
    style.rim_units = anim_rim_units(scale_q, dpi);
    style.rim_strength = handoff ? anim_handoff_rim(scale_q) : 1.0f;
    style.shadow = anim_shadow_strength(scale_q, g_sys_shadow);
    style.color = color;
    if (!f->bmp && make_frame(f, ppu, &style) != 0) return NULL;
    return f;
}

static int system_pointer_shadow(void)
{
    BOOL on = FALSE;
    if (!SystemParametersInfoW(SPI_GETCURSORSHADOW, 0, &on, 0)) return 0;
    return on != FALSE;
}

static BYTE opacity_byte(float opacity)
{
    if (!(opacity > 0.0f)) return 0;
    if (opacity >= 1.0f) return 255;
    return (BYTE)(opacity * 255.0f + 0.5f);
}

void overlay_show(float scale, ArrowColor color, int handoff, POINT pt)
{
    if (!g_wnd) return;
    if (!g_visible) g_sys_shadow = system_pointer_shadow();  /* cache is empty here */
    const Frame *f = frame_for(scale, color, handoff, dpi_at(pt));
    if (!f) return;
    float opacity = handoff ? anim_handoff_opacity(scale) : 1.0f;
    HDC screen = GetDC(NULL);
    HDC mem = CreateCompatibleDC(screen);
    HGDIOBJ old = SelectObject(mem, f->bmp);
    POINT dst = { pt.x - f->hot_x, pt.y - f->hot_y };
    POINT src = { 0, 0 };
    SIZE size = { f->w, f->h };
    BLENDFUNCTION bf = { AC_SRC_OVER, 0, opacity_byte(opacity), AC_SRC_ALPHA };
    UpdateLayeredWindow(g_wnd, screen, &dst, &size, mem, &src, 0, &bf, ULW_ALPHA);
    SelectObject(mem, old);
    DeleteDC(mem);
    ReleaseDC(NULL, screen);
    g_hot.x = f->hot_x;
    g_hot.y = f->hot_y;
    if (!g_visible) {
        ShowWindow(g_wnd, SW_SHOWNOACTIVATE);
        g_visible = 1;
    }
    SetWindowPos(g_wnd, HWND_TOPMOST, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
}

void overlay_move(POINT pt)
{
    if (!g_wnd || !g_visible) return;
    SetWindowPos(g_wnd, NULL, pt.x - g_hot.x, pt.y - g_hot.y, 0, 0,
                 SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
}

void overlay_hide(void)
{
    if (g_wnd && g_visible) ShowWindow(g_wnd, SW_HIDE);
    g_visible = 0;
    clear_cache();
}

void overlay_shutdown(void)
{
    overlay_hide();
    if (g_wnd) DestroyWindow(g_wnd);
    g_wnd = NULL;
}
