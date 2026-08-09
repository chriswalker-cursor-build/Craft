#include "test_harness.h"

int g_tests_run = 0;
int g_tests_failed = 0;

void test_matrix_suite(void);
void test_util_suite(void);
void test_db_suite(void);
void test_client_suite(void);

int main(void) {
    printf("Craft unit tests\n");
    run_suite("matrix", test_matrix_suite);
    run_suite("util", test_util_suite);
    run_suite("db", test_db_suite);
    run_suite("client", test_client_suite);

    printf("\n%d assertions, %d failed\n", g_tests_run, g_tests_failed);
    return g_tests_failed == 0 ? 0 : 1;
}
