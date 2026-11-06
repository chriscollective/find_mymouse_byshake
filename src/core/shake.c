/* Shake detection, one axis at a time.
 *
 * A "stroke" is a movement in one direction. A shake is N strokes in a
 * row, each long enough (min_stroke_px) and fast enough (max_stroke_ms),
 * all ending within window_ms. Direction changes only count after the
 * cursor comes back reverse_px from its furthest point, which filters
 * out hand jitter.
 *
 * The detector mutates its own state object on each sample; that is the
 * whole point of a state machine, so it is not treated as an input. */
#include "shake.h"

#include <stdlib.h>
#include <string.h>

static void axis_reset(ShakeAxis *a, int p, unsigned t)
{
    memset(a, 0, sizeof *a);
    a->origin = p;
    a->origin_t = t;
    a->extreme = p;
    a->extreme_t = t;
}

static void axis_prune(ShakeAxis *a, unsigned now, unsigned window_ms)
{
    int keep = 0;
    for (int i = 0; i < a->count; i++) {
        if (now - a->strokes[i] <= window_ms) a->strokes[keep++] = a->strokes[i];
    }
    a->count = keep;
}

static void axis_push(ShakeAxis *a, unsigned t)
{
    if (a->count == SHAKE_MAX_STROKES) {
        memmove(a->strokes, a->strokes + 1, (SHAKE_MAX_STROKES - 1) * sizeof a->strokes[0]);
        a->count--;
    }
    a->strokes[a->count++] = t;
}

static int stroke_ok(const ShakeAxis *a, const ShakeConfig *c)
{
    return abs(a->extreme - a->origin) >= c->min_stroke_px &&
           a->extreme_t - a->origin_t <= c->max_stroke_ms;
}

/* First movement after a reset: wait until we know the direction. */
static void axis_start(ShakeAxis *a, const ShakeConfig *c, int p, unsigned t)
{
    int d = p - a->origin;
    if (abs(d) >= c->reverse_px) {
        a->dir = d > 0 ? 1 : -1;
        a->extreme = p;
        a->extreme_t = t;
    } else if (t - a->origin_t > c->max_stroke_ms) {
        a->origin = p;   /* creeping slowly: keep the origin fresh */
        a->origin_t = t;
    }
}

static void axis_reverse(ShakeAxis *a, const ShakeConfig *c, int p, unsigned t)
{
    if (stroke_ok(a, c)) axis_push(a, a->extreme_t);
    else a->count = 0;
    a->origin = a->extreme;
    a->origin_t = a->extreme_t;
    a->dir = -a->dir;
    a->extreme = p;
    a->extreme_t = t;
}

static int axis_feed(ShakeAxis *a, const ShakeConfig *c, int p, unsigned t)
{
    if (a->dir == 0) {
        axis_start(a, c, p, t);
        return 0;
    }
    if ((p - a->extreme) * a->dir > 0) {
        a->extreme = p;
        a->extreme_t = t;
    } else if ((a->extreme - p) * a->dir >= c->reverse_px) {
        axis_reverse(a, c, p, t);
    }
    axis_prune(a, t, c->window_ms);
    /* Fire only while the stroke in progress itself qualifies and is still
     * live (its far end was reached recently): the shake triggers without
     * waiting for one more direction change, and hovering or creeping
     * after the shake cannot keep re-triggering it from old strokes. */
    int live = t - a->extreme_t <= c->max_stroke_ms / 2;
    return live && stroke_ok(a, c) && a->count + 1 >= c->strokes_needed;
}

void shake_init(ShakeDetector *d, const ShakeConfig *cfg)
{
    memset(d, 0, sizeof *d);
    d->cfg = *cfg;
}

int shake_feed(ShakeDetector *d, int x, int y, unsigned t_ms)
{
    if (!d->started || t_ms - d->last_t > d->cfg.max_stroke_ms) {
        axis_reset(&d->x, x, t_ms);
        axis_reset(&d->y, y, t_ms);
        d->started = 1;
        d->last_t = t_ms;
        return 0;
    }
    d->last_t = t_ms;
    int fx = axis_feed(&d->x, &d->cfg, x, t_ms);
    int fy = axis_feed(&d->y, &d->cfg, y, t_ms);
    return fx || fy;
}
