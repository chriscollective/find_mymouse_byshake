#include "test_util.h"

int g_failures = 0;
int g_checks = 0;

int main(void)
{
    run_shake_tests();
    run_arrow_tests();
    run_anim_settings_tests();
    run_throttle_tests();
    printf("\n%d checks, %d failures\n", g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}
