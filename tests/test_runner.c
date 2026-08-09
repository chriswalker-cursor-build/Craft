#include "test_harness.h"

void test_matrix_suite(void);
void test_util_suite(void);

int main(void) {
    printf("Craft pure-helper tests\n");
    run_suite("matrix", test_matrix_suite);
    run_suite("util", test_util_suite);

    printf("\n%d assertions, %d failed\n", g_tests_run, g_tests_failed);
    return g_tests_failed == 0 ? 0 : 1;
}
