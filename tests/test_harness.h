#ifndef TEST_HARNESS_H
#define TEST_HARNESS_H

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern int g_tests_run;
extern int g_tests_failed;

#define ASSERT(cond) do { \
    g_tests_run++; \
    if (!(cond)) { \
        g_tests_failed++; \
        fprintf(stderr, "  FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
    } \
} while (0)

#define ASSERT_MSG(cond, msg) do { \
    g_tests_run++; \
    if (!(cond)) { \
        g_tests_failed++; \
        fprintf(stderr, "  FAIL %s:%d: %s (%s)\n", __FILE__, __LINE__, #cond, msg); \
    } \
} while (0)

#define ASSERT_FLOAT_EQ(a, b, eps) do { \
    g_tests_run++; \
    if (!(fabsf((float)(a) - (float)(b)) <= (float)(eps))) { \
        g_tests_failed++; \
        fprintf(stderr, "  FAIL %s:%d: |%g - %g| > %g\n", \
            __FILE__, __LINE__, (double)(a), (double)(b), (double)(eps)); \
    } \
} while (0)

#define ASSERT_INT_EQ(a, b) do { \
    g_tests_run++; \
    if ((int)(a) != (int)(b)) { \
        g_tests_failed++; \
        fprintf(stderr, "  FAIL %s:%d: %d != %d\n", \
            __FILE__, __LINE__, (int)(a), (int)(b)); \
    } \
} while (0)

#define ASSERT_STR_EQ(a, b) do { \
    g_tests_run++; \
    if (strcmp((a), (b)) != 0) { \
        g_tests_failed++; \
        fprintf(stderr, "  FAIL %s:%d: \"%s\" != \"%s\"\n", \
            __FILE__, __LINE__, (a), (b)); \
    } \
} while (0)

typedef void (*test_fn)(void);

static void run_suite(const char *name, test_fn fn) {
    int before_failed = g_tests_failed;
    int before_run = g_tests_run;
    printf("SUITE %s\n", name);
    fn();
    int failed = g_tests_failed - before_failed;
    int run = g_tests_run - before_run;
    if (failed == 0) {
        printf("  OK (%d assertions)\n", run);
    } else {
        printf("  %d failed / %d assertions\n", failed, run);
    }
}

#endif
