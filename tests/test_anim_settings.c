#include "test_util.h"
#include "anim.h"
#include "settings.h"

static void anim_converges_without_overshoot(void)
{
    float s = 1.0f;
    int monotonic = 1;
    for (int i = 0; i < 60; i++) {
        float n = anim_step(s, 5.0f, 16.0f);
        if (n < s || n > 5.0f) monotonic = 0;
        s = n;
    }
    CHECK(monotonic);
    CHECK(s > 4.95f);
}

static void anim_shrinks_back(void)
{
    float s = 5.0f;
    for (int i = 0; i < 80; i++) s = anim_step(s, 1.0f, 16.0f);
    CHECK(s < 1.05f && s >= 1.0f);
}

static void anim_grows_faster_than_it_shrinks(void)
{
    float up = anim_step(1.0f, 5.0f, 50.0f) - 1.0f;
    float down = 5.0f - anim_step(5.0f, 1.0f, 50.0f);
    CHECK(up > down);
}

static void anim_zero_dt_is_noop(void)
{
    CHECK(anim_step(2.5f, 5.0f, 0.0f) == 2.5f);
    CHECK(anim_step(2.5f, 5.0f, -10.0f) == 2.5f);
}

static void shadow_fades_out_at_normal_size(void)
{
    CHECK(anim_shadow_strength(1.0f, 0) == 0.0f);   /* matches a shadowless cursor */
    CHECK(anim_shadow_strength(0.9f, 0) == 0.0f);
    CHECK(anim_shadow_strength(5.0f, 0) == 1.0f);
    CHECK(anim_shadow_strength(1.0f, 1) > 0.0f);    /* system pointer shadow on */
    CHECK(anim_shadow_strength(1.0f, 1) < 1.0f);
    int monotonic = 1;
    float prev = -1.0f;
    for (float s = 1.0f; s <= 3.0f; s += 0.01f) {
        float v = anim_shadow_strength(s, 0);
        if (v < prev) monotonic = 0;
        prev = v;
    }
    CHECK(monotonic);
}

static void rim_is_one_physical_pixel_at_normal_size(void)
{
    /* rim_px = rim_units * ppu, ppu = scale * dpi / 96 */
    const unsigned dpis[] = { 96, 120, 144, 192 };
    for (int i = 0; i < 4; i++) {
        float units = anim_rim_units(1.0f, dpis[i]);
        float px = units * 1.0f * (float)dpis[i] / 96.0f;
        CHECK(px > 0.99f && px < 1.01f);
    }
    CHECK(anim_rim_units(5.0f, 144) == 1.0f);       /* big: classic thick rim */
    CHECK(anim_rim_units(1.0f, 72) == 1.0f);        /* never thicker than 1 unit */
    CHECK(anim_rim_units(1.0f, 0) == 1.0f);         /* unknown dpi = 96 */
    int monotonic = 1;
    float prev = 0.0f;
    for (float s = 1.0f; s <= 3.0f; s += 0.01f) {
        float v = anim_rim_units(s, 144);
        if (v < prev) monotonic = 0;
        prev = v;
    }
    CHECK(monotonic);
}

static void real_cursor_hidden_only_while_big_or_growing(void)
{
    CHECK(anim_hide_real_cursor(1.0f, 5.0f) == 1);   /* just started growing */
    CHECK(anim_hide_real_cursor(4.0f, 5.0f) == 1);
    CHECK(anim_hide_real_cursor(4.0f, 1.0f) == 1);   /* shrinking, still big */
    CHECK(anim_hide_real_cursor(1.0f + (ANIM_HANDOFF_SCALE - 1.0f) * 0.5f, 1.0f) == 0);
                                                     /* tail: real cursor on top */
    CHECK(anim_hide_real_cursor(1.0f, 1.0f) == 0);
}

static void overlay_fades_out_under_real_cursor(void)
{
    CHECK(anim_handoff_opacity(3.0f) == 1.0f);
    CHECK(anim_handoff_opacity(1.0f) == 0.0f);
    float mid = anim_handoff_opacity(1.0f + (ANIM_HANDOFF_SCALE - 1.0f) * 0.5f);
    CHECK(mid > 0.0f && mid < 1.0f);
    int monotonic = 1;
    float prev = -1.0f;
    for (float x = 1.0f; x <= 2.0f; x += 0.01f) {
        float v = anim_handoff_opacity(x);
        if (v < prev) monotonic = 0;
        prev = v;
    }
    CHECK(monotonic);
}

static void rim_fades_first_after_handoff(void)
{
    CHECK(anim_handoff_rim(2.0f) == 1.0f);
    CHECK(anim_handoff_rim(ANIM_HANDOFF_SCALE) > 0.99f);
    CHECK(anim_handoff_rim(1.0f) == 0.0f);
    /* the rim is gone well before the arrow itself is (half way down) */
    float s = 1.0f + (ANIM_HANDOFF_SCALE - 1.0f) * 0.5f;
    CHECK(anim_handoff_rim(s) == 0.0f && anim_handoff_opacity(s) > 0.2f);
}

static void overlay_is_invisible_before_it_is_removed(void)
{
    /* the overlay must reach zero opacity above the hide threshold, so
     * removing it is never a visible jump (e.g. a white halo on dark) */
    CHECK(anim_handoff_opacity(ANIM_FADE_END) == 0.0f);
    CHECK(ANIM_FADE_END > 1.0f && ANIM_FADE_END < ANIM_HANDOFF_SCALE);
}

static void handoff_point_is_consistent(void)
{
    /* just above the hand-off scale: hidden and opaque; just below: shown */
    float h = ANIM_HANDOFF_SCALE;
    CHECK(anim_hide_real_cursor(h + 0.001f, 1.0f) == 1);
    CHECK(anim_hide_real_cursor(h - 0.001f, 1.0f) == 0);
    CHECK(anim_handoff_opacity(h) > 0.99f);
}

static void shrink_is_timed_and_lands_exactly(void)
{
    CHECK(anim_shrink_scale(5.0f, 0.0f) == 5.0f);
    CHECK(anim_shrink_scale(5.0f, ANIM_SHRINK_MS) == 1.0f);      /* exact landing */
    CHECK(anim_shrink_scale(5.0f, ANIM_SHRINK_MS + 50.0f) == 1.0f);
    CHECK(anim_shrink_scale(5.0f, -5.0f) == 5.0f);
    int monotonic = 1;
    float prev = 6.0f;
    for (float t = 0.0f; t <= ANIM_SHRINK_MS; t += 1.0f) {
        float s = anim_shrink_scale(5.0f, t);
        if (s > prev) monotonic = 0;
        prev = s;
    }
    CHECK(monotonic);
}

static void shrink_tail_is_short(void)
{
    /* the nearly-normal-size tail (below the hand-off scale) must be a
     * few frames, not a long crawl: that crawl is what felt "blunt" */
    const float from[] = { 3.0f, 5.0f, 7.0f };
    for (int i = 0; i < 3; i++) {
        float t_handoff = -1.0f;
        for (float t = 0.0f; t <= ANIM_SHRINK_MS; t += 0.5f) {
            if (anim_shrink_scale(from[i], t) < ANIM_HANDOFF_SCALE) { t_handoff = t; break; }
        }
        CHECK(t_handoff > 0.0f);
        CHECK(ANIM_SHRINK_MS - t_handoff < 70.0f);
    }
}

static void shrink_starts_promptly(void)
{
    /* first 60fps frame already moves visibly (no sluggish start) */
    float s = anim_shrink_scale(5.0f, 16.7f);
    CHECK(5.0f - s > 0.05f);
}

static void sensitivity_presets_are_ordered(void)
{
    ShakeConfig lo = settings_shake_config(SENS_LOW, 96);
    ShakeConfig md = settings_shake_config(SENS_MEDIUM, 96);
    ShakeConfig hi = settings_shake_config(SENS_HIGH, 96);
    CHECK(lo.min_stroke_px > md.min_stroke_px && md.min_stroke_px > hi.min_stroke_px);
    CHECK(lo.strokes_needed >= md.strokes_needed && md.strokes_needed >= hi.strokes_needed);
}

static void thresholds_scale_with_dpi(void)
{
    ShakeConfig a = settings_shake_config(SENS_MEDIUM, 96);
    ShakeConfig b = settings_shake_config(SENS_MEDIUM, 192);
    ShakeConfig z = settings_shake_config(SENS_MEDIUM, 0);
    CHECK(b.min_stroke_px == a.min_stroke_px * 2);
    CHECK(b.reverse_px == a.reverse_px * 2);
    CHECK(z.min_stroke_px == a.min_stroke_px);
    CHECK(b.window_ms == a.window_ms);
}

static void sizes_are_ordered(void)
{
    CHECK(settings_max_scale(SIZE_SMALL) < settings_max_scale(SIZE_MEDIUM));
    CHECK(settings_max_scale(SIZE_MEDIUM) < settings_max_scale(SIZE_LARGE));
    CHECK(settings_max_scale(SIZE_SMALL) > 1.5f);
}

static void clamps_reject_garbage(void)
{
    CHECK(settings_clamp_sensitivity(-1) == SENS_MEDIUM);
    CHECK(settings_clamp_sensitivity(99) == SENS_MEDIUM);
    CHECK(settings_clamp_sensitivity(2) == SENS_HIGH);
    CHECK(settings_clamp_size(7) == SIZE_MEDIUM);
    CHECK(settings_clamp_size(0) == SIZE_SMALL);
    CHECK(settings_clamp_color(-1) == ARROW_WHITE);
    CHECK(settings_clamp_color(5) == ARROW_WHITE);
    CHECK(settings_clamp_color(ARROW_BLACK) == ARROW_BLACK);
    CHECK(settings_clamp_color(ARROW_WHITE) == ARROW_WHITE);
}

void run_anim_settings_tests(void)
{
    printf("[anim/settings]\n");
    RUN(anim_converges_without_overshoot);
    RUN(anim_shrinks_back);
    RUN(anim_grows_faster_than_it_shrinks);
    RUN(anim_zero_dt_is_noop);
    RUN(shadow_fades_out_at_normal_size);
    RUN(rim_is_one_physical_pixel_at_normal_size);
    RUN(real_cursor_hidden_only_while_big_or_growing);
    RUN(overlay_fades_out_under_real_cursor);
    RUN(handoff_point_is_consistent);
    RUN(overlay_is_invisible_before_it_is_removed);
    RUN(rim_fades_first_after_handoff);
    RUN(shrink_is_timed_and_lands_exactly);
    RUN(shrink_tail_is_short);
    RUN(shrink_starts_promptly);
    RUN(sensitivity_presets_are_ordered);
    RUN(thresholds_scale_with_dpi);
    RUN(sizes_are_ordered);
    RUN(clamps_reject_garbage);
}
