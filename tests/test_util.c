#include "test_harness.h"
#include "util.h"

static void test_util_macros(void) {
    ASSERT_INT_EQ(ABS(-3), 3);
    ASSERT_INT_EQ(ABS(4), 4);
    ASSERT_INT_EQ(MIN(2, 9), 2);
    ASSERT_INT_EQ(MAX(2, 9), 9);
    ASSERT_INT_EQ(SIGN(-5), -1);
    ASSERT_INT_EQ(SIGN(0), 0);
    ASSERT_INT_EQ(SIGN(7), 1);
    ASSERT_FLOAT_EQ(RADIANS(180.0), PI, 1e-9);
    ASSERT_FLOAT_EQ(DEGREES(PI), 180.0, 1e-9);
}

static void test_char_width(void) {
    ASSERT_INT_EQ(char_width(' '), 4);
    ASSERT_INT_EQ(char_width('A'), 6);
    ASSERT_INT_EQ(char_width('i'), 2);
    ASSERT_INT_EQ(char_width('W'), 10);
    ASSERT_INT_EQ(char_width('\0'), 0);
}

static void test_string_width(void) {
    ASSERT_INT_EQ(string_width(""), 0);
    ASSERT_INT_EQ(string_width("A"), char_width('A'));
    ASSERT_INT_EQ(string_width("Hi"), char_width('H') + char_width('i'));
    ASSERT_INT_EQ(string_width("WW"), 20);
}

static void test_tokenize_basic(void) {
    char buf[] = "a,b,,c";
    char *key = NULL;
    char *tok;

    tok = tokenize(buf, ",", &key);
    ASSERT_STR_EQ(tok, "a");
    tok = tokenize(NULL, ",", &key);
    ASSERT_STR_EQ(tok, "b");
    tok = tokenize(NULL, ",", &key);
    ASSERT_STR_EQ(tok, "c");
    tok = tokenize(NULL, ",", &key);
    ASSERT(tok == NULL);
}

static void test_wrap_single_line(void) {
    char out[128];
    int lines = wrap("hi", 100, out, (int)sizeof(out));
    ASSERT_INT_EQ(lines, 1);
    ASSERT_STR_EQ(out, "hi\n");
}

static void test_wrap_splits_on_width(void) {
    char out[128];
    /* "hello" width + "world" width exceed a tight max_width */
    int hello_w = string_width("hello");
    int lines = wrap("hello world", hello_w, out, (int)sizeof(out));
    ASSERT(lines >= 2);
    ASSERT(strstr(out, "hello") != NULL);
    ASSERT(strstr(out, "world") != NULL);
    ASSERT(strchr(out, '\n') != NULL);
}

void test_util_suite(void) {
    test_util_macros();
    test_char_width();
    test_string_width();
    test_tokenize_basic();
    test_wrap_single_line();
    test_wrap_splits_on_width();
}
