#include "test_util.h"
#include "throttle.h"

#define F_10MHZ 10000000LL
#define F_3GHZ  3000000000LL

static void first_call_is_due(void)
{
    CHECK(throttle_due(123456789LL, 0, F_10MHZ, 4000) == 1);
}

static void too_soon_is_not_due(void)
{
    long long last = 50000000LL;
    CHECK(throttle_due(last + 39999, last, F_10MHZ, 4000) == 0);  /* 3.9999 ms */
    CHECK(throttle_due(last + 40000, last, F_10MHZ, 4000) == 1);  /* 4 ms */
}

static void long_uptime_does_not_overflow(void)
{
    /* 11 days at 10 MHz and 1 hour at 3 GHz: (delta * 1e6) would overflow */
    long long eleven_days = 11LL * 24 * 3600 * F_10MHZ;
    CHECK(throttle_due(eleven_days, 0, F_10MHZ, 4000) == 1);
    CHECK(throttle_due(eleven_days, 1, F_10MHZ, 4000) == 1);
    long long hour_3ghz = 3600LL * F_3GHZ;
    CHECK(throttle_due(hour_3ghz, 5, F_3GHZ, 4000) == 1);
    CHECK(throttle_due(hour_3ghz + 1000, hour_3ghz, F_3GHZ, 4000) == 0);
}

static void backwards_counter_is_due(void)
{
    CHECK(throttle_due(1000, 5000, F_10MHZ, 4000) == 1);
}

static void bad_freq_is_due(void)
{
    CHECK(throttle_due(1000, 900, 0, 4000) == 1);
}

void run_throttle_tests(void)
{
    printf("[throttle]\n");
    RUN(first_call_is_due);
    RUN(too_soon_is_not_due);
    RUN(long_uptime_does_not_overflow);
    RUN(backwards_counter_is_due);
    RUN(bad_freq_is_due);
}
