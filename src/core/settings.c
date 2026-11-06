#include "settings.h"

#define BASE_DPI       96u
#define REVERSE_PX     12
#define MAX_STROKE_MS  280u
#define WINDOW_MS      1100u

typedef struct { int min_stroke_px; int strokes_needed; } SensPreset;

static const SensPreset SENS_PRESETS[] = {
    [SENS_LOW]    = { 100, 5 },
    [SENS_MEDIUM] = {  70, 4 },
    [SENS_HIGH]   = {  45, 3 },
};

static const float SIZE_SCALES[] = {
    [SIZE_SMALL]  = 3.0f,
    [SIZE_MEDIUM] = 5.0f,
    [SIZE_LARGE]  = 7.0f,
};

static int scale_px(int px, unsigned dpi)
{
    return (int)(((long)px * (long)dpi + BASE_DPI / 2) / (long)BASE_DPI);
}

ShakeConfig settings_shake_config(Sensitivity s, unsigned dpi)
{
    if (dpi == 0) dpi = BASE_DPI;
    SensPreset p = SENS_PRESETS[settings_clamp_sensitivity((int)s)];
    ShakeConfig c;
    c.min_stroke_px = scale_px(p.min_stroke_px, dpi);
    c.reverse_px = scale_px(REVERSE_PX, dpi);
    c.max_stroke_ms = MAX_STROKE_MS;
    c.window_ms = WINDOW_MS;
    c.strokes_needed = p.strokes_needed;
    return c;
}

float settings_max_scale(CursorSize z)
{
    return SIZE_SCALES[settings_clamp_size((int)z)];
}

Sensitivity settings_clamp_sensitivity(int v)
{
    return (v >= SENS_LOW && v <= SENS_HIGH) ? (Sensitivity)v : SENS_MEDIUM;
}

CursorSize settings_clamp_size(int v)
{
    return (v >= SIZE_SMALL && v <= SIZE_LARGE) ? (CursorSize)v : SIZE_MEDIUM;
}

ArrowColor settings_clamp_color(int v)
{
    return v == ARROW_BLACK ? ARROW_BLACK : ARROW_WHITE;
}
