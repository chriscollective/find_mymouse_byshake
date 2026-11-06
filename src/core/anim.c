#include "anim.h"

#include "arrow.h"

#include <math.h>

#define GROW_TAU_MS   55.0f
#define SHRINK_TAU_MS 120.0f
#define SHADOW_FADE_SPAN   1.5f    /* full shadow from scale 2.5 upward */
#define SYSTEM_SHADOW_BASE 0.45f   /* roughly the Windows pointer shadow */
#define RIM_FADE_SPAN      1.5f    /* full classic rim from scale 2.5 upward */
#define BASE_DPI           96.0f
#define HANDOFF_RIM_GONE   1.05f   /* rim fully faded below this scale */
#define SHRINK_END_SPEED   0.4f    /* landing speed, relative to average */

static float fade(float scale, float span)
{
    float t = (scale - 1.0f) / span;
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    return t;
}

float anim_step(float cur, float target, float dt_ms)
{
    if (!(dt_ms > 0.0f)) return cur;
    float tau = target > cur ? GROW_TAU_MS : SHRINK_TAU_MS;
    float k = 1.0f - expf(-dt_ms / tau);
    return cur + (target - cur) * k;
}

float anim_shrink_scale(float from, float elapsed_ms)
{
    if (!(elapsed_ms > 0.0f)) return from;
    if (elapsed_ms >= ANIM_SHRINK_MS) return 1.0f;
    float t = elapsed_ms / ANIM_SHRINK_MS;
    /* blend of linear and ease-out-quad: start speed 2-a, end speed a */
    const float a = SHRINK_END_SPEED;
    float p = a * t + (1.0f - a) * t * (2.0f - t);
    return 1.0f + (from - 1.0f) * (1.0f - p);
}

float anim_shadow_strength(float scale, int system_shadow)
{
    float t = fade(scale, SHADOW_FADE_SPAN);
    float base = system_shadow ? SYSTEM_SHADOW_BASE : 0.0f;
    return base + (1.0f - base) * t;
}

int anim_hide_real_cursor(float scale, float target)
{
    return target > 1.0f || scale > ANIM_HANDOFF_SCALE;
}

float anim_handoff_opacity(float scale)
{
    return fade(scale - (ANIM_FADE_END - 1.0f), ANIM_HANDOFF_SCALE - ANIM_FADE_END);
}

float anim_handoff_rim(float scale)
{
    return fade(scale - (HANDOFF_RIM_GONE - 1.0f), ANIM_HANDOFF_SCALE - HANDOFF_RIM_GONE);
}

float anim_rim_units(float scale, unsigned dpi)
{
    float d = dpi == 0 ? BASE_DPI : (float)dpi;
    float thin = BASE_DPI / d;          /* 1 physical px expressed in units */
    if (thin > ARROW_RIM_UNITS) thin = ARROW_RIM_UNITS;
    float t = fade(scale, RIM_FADE_SPAN);
    return thin + (ARROW_RIM_UNITS - thin) * t;
}
