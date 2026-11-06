/* Arrow cursor rendered from a signed distance field: white or black
 * fill with a contrasting rim and a soft drop shadow. Anti-aliasing
 * comes for free from the distance. */
#include "arrow.h"

#include <math.h>
#include <stdlib.h>

#define SHADOW_DX      0.6f
#define SHADOW_DY      0.9f
#define SHADOW_BLUR    1.6f
#define SHADOW_ALPHA   0.35f
#define MAX_PPU        64.0f

/* Classic arrow outline, tip at (0, 0), clockwise in screen space. */
static const float PTS[][2] = {
    { 0.0f,  0.0f }, { 0.0f, 17.0f }, { 3.8f, 13.0f }, { 6.3f, 19.0f },
    { 9.8f, 18.2f }, { 7.3f, 12.2f }, { 12.2f, 12.2f },
};
#define NPTS ((int)(sizeof PTS / sizeof PTS[0]))

static float clampf(float v, float lo, float hi)
{
    return v < lo ? lo : (v > hi ? hi : v);
}

static float seg_dist2(float px, float py, const float *a, const float *b)
{
    float ex = b[0] - a[0], ey = b[1] - a[1];
    float wx = px - a[0], wy = py - a[1];
    float t = clampf((wx * ex + wy * ey) / (ex * ex + ey * ey), 0.0f, 1.0f);
    float dx = wx - ex * t, dy = wy - ey * t;
    return dx * dx + dy * dy;
}

float arrow_sdf(float x, float y)
{
    float best = 1e30f;
    int inside = 0;
    for (int i = 0, j = NPTS - 1; i < NPTS; j = i++) {
        const float *a = PTS[j], *b = PTS[i];
        float d2 = seg_dist2(x, y, a, b);
        if (d2 < best) best = d2;
        if ((a[1] > y) != (b[1] > y) &&
            x < a[0] + (y - a[1]) * (b[0] - a[0]) / (b[1] - a[1])) {
            inside = !inside;
        }
    }
    float d = sqrtf(best);
    return inside ? d : -d;
}

/* One premultiplied 0xAARRGGBB pixel at arrow-space point (u, v). */
static unsigned shade(float u, float v, float ppu, float rim_px, const ArrowStyle *st)
{
    float sd = arrow_sdf(u, v) * ppu;
    float cov = clampf(0.5f + sd, 0.0f, 1.0f);
    float ink = clampf(0.5f + sd - rim_px, 0.0f, 1.0f);   /* 1 inside the rim */
    float sa = 0.0f;
    if (st->shadow > 0.0f && cov < 1.0f) {
        float ss = arrow_sdf(u - SHADOW_DX, v - SHADOW_DY);
        sa = SHADOW_ALPHA * st->shadow * clampf(1.0f + ss / SHADOW_BLUR, 0.0f, 1.0f);
    }
    float a = cov + sa * (1.0f - cov);
    float fill = st->color == ARROW_WHITE ? 1.0f : 0.0f;
    float rim = fill + (1.0f - 2.0f * fill) * st->rim_strength;  /* toward contrast */
    float white = (ink * fill + (1.0f - ink) * rim) * cov;
    unsigned A = (unsigned)(a * 255.0f + 0.5f);
    unsigned C = (unsigned)(white * 255.0f + 0.5f);
    return (A << 24) | (C << 16) | (C << 8) | C;
}

static int style_ok(const ArrowStyle *st)
{
    return st && st->rim_units > 0.0f && st->rim_units <= ARROW_MAX_RIM_UNITS &&
           st->rim_strength >= 0.0f && st->rim_strength <= 1.0f &&
           st->shadow >= 0.0f && st->shadow <= 1.0f &&
           (st->color == ARROW_BLACK || st->color == ARROW_WHITE);
}

int arrow_render(float px_per_unit, const ArrowStyle *style, ArrowImage *out)
{
    if (!out || !(px_per_unit > 0.0f) || px_per_unit > MAX_PPU) return -1;
    if (!style_ok(style)) return -1;     /* comparisons are false for NaN */
    const float ppu = px_per_unit;
    const float rim_px = style->rim_units * ppu;
    float pad_units = style->shadow > 0.0f ? SHADOW_DY + SHADOW_BLUR : 0.0f;
    int pad = (int)ceilf(pad_units * ppu) + 1;
    int w = (int)ceilf(ARROW_UNITS_W * ppu) + 2 * pad;
    int h = (int)ceilf(ARROW_UNITS_H * ppu) + 2 * pad;
    unsigned *pixels = malloc((size_t)w * (size_t)h * sizeof *pixels);
    if (!pixels) return -1;
    for (int y = 0; y < h; y++) {
        float v = ((float)(y - pad) + 0.5f) / ppu;
        for (int x = 0; x < w; x++) {
            float u = ((float)(x - pad) + 0.5f) / ppu;
            pixels[y * w + x] = shade(u, v, ppu, rim_px, style);
        }
    }
    out->width = w;
    out->height = h;
    out->hot_x = pad;
    out->hot_y = pad;
    out->pixels = pixels;
    return 0;
}

void arrow_free(ArrowImage *img)
{
    if (!img) return;
    free(img->pixels);
    img->pixels = NULL;
    img->width = img->height = 0;
}
