#include "test_util.h"
#include "shake.h"

#define STEP_MS 8u

static ShakeConfig cfg(void)
{
    ShakeConfig c = { 70, 12, 280, 1100, 4 };
    return c;
}

/* Move linearly from (x0,y0) to (x1,y1) over dur ms, sampling every STEP_MS.
 * Advances *t. Returns 1 if the detector fired at any sample. */
static int move(ShakeDetector *d, int x0, int y0, int x1, int y1,
                unsigned *t, unsigned dur)
{
    int fired = 0;
    unsigned steps = dur / STEP_MS;
    if (steps == 0) steps = 1;
    for (unsigned i = 1; i <= steps; i++) {
        int x = x0 + (int)((long)(x1 - x0) * (long)i / (long)steps);
        int y = y0 + (int)((long)(y1 - y0) * (long)i / (long)steps);
        *t += STEP_MS;
        fired |= shake_feed(d, x, y, *t);
    }
    return fired;
}

/* Back-and-forth along one axis: `strokes` single-direction strokes. */
static int shake_axis(ShakeDetector *d, int vertical, int amp, int strokes,
                      unsigned stroke_ms, unsigned pause_ms, unsigned *t)
{
    int fired = 0, pos = 0;
    for (int i = 0; i < strokes; i++) {
        int next = (i % 2 == 0) ? amp : 0;
        if (vertical) fired |= move(d, 500, 500 + pos, 500, 500 + next, t, stroke_ms);
        else          fired |= move(d, 500 + pos, 500, 500 + next, 500, t, stroke_ms);
        pos = next;
        *t += pause_ms;
    }
    return fired;
}

static void fresh(ShakeDetector *d, unsigned *t, unsigned start)
{
    ShakeConfig c = cfg();
    shake_init(d, &c);
    *t = start;
    shake_feed(d, 500, 500, *t);
}

static void fast_horizontal_shake_fires(void)
{
    ShakeDetector d; unsigned t;
    fresh(&d, &t, 1000);
    CHECK(shake_axis(&d, 0, 200, 6, 100, 0, &t) == 1);
}

static void fast_vertical_shake_fires(void)
{
    ShakeDetector d; unsigned t;
    fresh(&d, &t, 1000);
    CHECK(shake_axis(&d, 1, 200, 6, 100, 0, &t) == 1);
}

static void single_long_swipe_does_not_fire(void)
{
    ShakeDetector d; unsigned t;
    fresh(&d, &t, 1000);
    CHECK(move(&d, 500, 500, 1500, 500, &t, 250) == 0);
}

static void one_back_and_forth_does_not_fire(void)
{
    ShakeDetector d; unsigned t;
    fresh(&d, &t, 1000);
    CHECK(shake_axis(&d, 0, 200, 2, 100, 0, &t) == 0);
}

static void slow_back_and_forth_does_not_fire(void)
{
    ShakeDetector d; unsigned t;
    fresh(&d, &t, 1000);
    CHECK(shake_axis(&d, 0, 300, 8, 600, 0, &t) == 0);
}

static void tiny_jitter_does_not_fire(void)
{
    ShakeDetector d; unsigned t;
    fresh(&d, &t, 1000);
    CHECK(shake_axis(&d, 0, 20, 12, 40, 0, &t) == 0);
}

static void pauses_between_strokes_break_the_chain(void)
{
    ShakeDetector d; unsigned t;
    fresh(&d, &t, 1000);
    CHECK(shake_axis(&d, 0, 200, 8, 100, 400, &t) == 0);
}

static void tick_wraparound_still_fires(void)
{
    ShakeDetector d; unsigned t;
    fresh(&d, &t, 0xFFFFFF00u);
    CHECK(shake_axis(&d, 0, 200, 6, 100, 0, &t) == 1);
}

static void stops_firing_after_shake_ends(void)
{
    ShakeDetector d; unsigned t;
    fresh(&d, &t, 1000);
    CHECK(shake_axis(&d, 0, 200, 6, 100, 0, &t) == 1);
    t += 200; /* short rest, below the reset gap */
    /* calm, slow movement afterwards must not keep the shake alive */
    CHECK(move(&d, 500, 500, 560, 520, &t, 900) == 0);
}

static void more_strokes_needed_means_harder(void)
{
    ShakeConfig c = cfg();
    c.strokes_needed = 8;
    ShakeDetector d; unsigned t = 1000;
    shake_init(&d, &c);
    shake_feed(&d, 500, 500, t);
    CHECK(shake_axis(&d, 0, 200, 5, 100, 0, &t) == 0);
}

void run_shake_tests(void)
{
    printf("[shake]\n");
    RUN(fast_horizontal_shake_fires);
    RUN(fast_vertical_shake_fires);
    RUN(single_long_swipe_does_not_fire);
    RUN(one_back_and_forth_does_not_fire);
    RUN(slow_back_and_forth_does_not_fire);
    RUN(tiny_jitter_does_not_fire);
    RUN(pauses_between_strokes_break_the_chain);
    RUN(tick_wraparound_still_fires);
    RUN(stops_firing_after_shake_ends);
    RUN(more_strokes_needed_means_harder);
}
