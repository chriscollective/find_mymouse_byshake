/* Overflow-safe "has enough time passed?" check on raw counter ticks
 * (e.g. QueryPerformanceCounter values). */
#ifndef THROTTLE_H
#define THROTTLE_H

/* Returns 1 if at least min_us microseconds passed between `last` and
 * `now`, given `freq` ticks per second. last == 0 means "never": due.
 * A counter going backwards is treated as due. */
int throttle_due(long long now, long long last, long long freq, long long min_us);

#endif
