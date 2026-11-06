#include "throttle.h"

#define US_PER_SECOND 1000000LL

int throttle_due(long long now, long long last, long long freq, long long min_us)
{
    if (last == 0 || freq <= 0 || now < last) return 1;
    /* compare against a precomputed tick budget instead of multiplying
     * the (possibly huge) delta: min_us * freq stays far below 2^63 */
    long long need = min_us * freq / US_PER_SECOND;
    return now - last >= need;
}
