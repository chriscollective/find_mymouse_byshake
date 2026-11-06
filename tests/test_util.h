#ifndef TEST_UTIL_H
#define TEST_UTIL_H

#include <stdio.h>

extern int g_failures;
extern int g_checks;

#define CHECK(cond)                                                        \
    do {                                                                   \
        g_checks++;                                                        \
        if (!(cond)) {                                                     \
            g_failures++;                                                  \
            fprintf(stderr, "  FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
        }                                                                  \
    } while (0)

#define RUN(fn)                     \
    do {                            \
        printf("- %s\n", #fn);      \
        fn();                       \
    } while (0)

void run_shake_tests(void);
void run_arrow_tests(void);
void run_anim_settings_tests(void);
void run_throttle_tests(void);

#endif
