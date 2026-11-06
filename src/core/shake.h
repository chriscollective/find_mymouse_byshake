/* Shake detector: turns a stream of cursor positions into "the user is
 * shaking the mouse" events. Pure C, no Windows dependency, unit-tested. */
#ifndef SHAKE_H
#define SHAKE_H

#define SHAKE_MAX_STROKES 16

typedef struct {
    int min_stroke_px;      /* a stroke must travel at least this far      */
    int reverse_px;         /* hysteresis before a direction change counts */
    unsigned max_stroke_ms; /* slower strokes (or pauses) break the chain  */
    unsigned window_ms;     /* all counted strokes must fit in this window */
    int strokes_needed;     /* strokes (one direction each) to trigger     */
} ShakeConfig;

typedef struct {
    int dir;                /* -1, 0 (not moving yet), +1 */
    int origin;             /* where the current stroke started */
    unsigned origin_t;
    int extreme;            /* furthest point of the current stroke */
    unsigned extreme_t;
    unsigned strokes[SHAKE_MAX_STROKES]; /* end times of finished strokes */
    int count;
} ShakeAxis;

typedef struct {
    ShakeConfig cfg;
    ShakeAxis x, y;
    unsigned last_t;
    int started;
} ShakeDetector;

void shake_init(ShakeDetector *d, const ShakeConfig *cfg);

/* Feed one cursor sample (screen px, millisecond tick that may wrap).
 * Returns 1 while a shake is in progress, 0 otherwise. */
int shake_feed(ShakeDetector *d, int x, int y, unsigned t_ms);

#endif
