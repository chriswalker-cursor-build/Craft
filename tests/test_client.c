#include "test_harness.h"
#include "client.h"

static void test_enable_flag(void) {
    client_disable();
    ASSERT_INT_EQ(get_client_enabled(), 0);
    client_enable();
    ASSERT_INT_EQ(get_client_enabled(), 1);
    client_disable();
    ASSERT_INT_EQ(get_client_enabled(), 0);
}

static void test_fmt_version(void) {
    char buf[64];
    int n = client_fmt_version(buf, sizeof(buf), CLIENT_PROTOCOL_VERSION);
    ASSERT(n > 0);
    ASSERT_STR_EQ(buf, "V,1\n");
}

static void test_fmt_login(void) {
    char buf[128];
    int n = client_fmt_login(buf, sizeof(buf), "alice", "tok123");
    ASSERT(n > 0);
    ASSERT_STR_EQ(buf, "A,alice,tok123\n");
}

static void test_fmt_position(void) {
    char buf[128];
    int n = client_fmt_position(buf, sizeof(buf), 1.0f, 2.0f, 3.0f, 0.25f, -0.5f);
    ASSERT(n > 0);
    ASSERT_STR_EQ(buf, "P,1.00,2.00,3.00,0.25,-0.50\n");
}

static void test_fmt_chunk(void) {
    char buf[64];
    int n = client_fmt_chunk(buf, sizeof(buf), -1, 2, 99);
    ASSERT(n > 0);
    ASSERT_STR_EQ(buf, "C,-1,2,99\n");
}

static void test_fmt_block_place_and_break(void) {
    char buf[64];
    ASSERT(client_fmt_block(buf, sizeof(buf), 3, 4, 5, 1) > 0);
    ASSERT_STR_EQ(buf, "B,3,4,5,1\n");
    ASSERT(client_fmt_block(buf, sizeof(buf), 3, 4, 5, 0) > 0);
    ASSERT_STR_EQ(buf, "B,3,4,5,0\n");
}

static void test_fmt_light(void) {
    char buf[64];
    ASSERT(client_fmt_light(buf, sizeof(buf), 1, 2, 3, 15) > 0);
    ASSERT_STR_EQ(buf, "L,1,2,3,15\n");
}

static void test_fmt_sign(void) {
    char buf[128];
    ASSERT(client_fmt_sign(buf, sizeof(buf), 8, 9, 10, 2, "hello") > 0);
    ASSERT_STR_EQ(buf, "S,8,9,10,2,hello\n");
}

static void test_fmt_talk(void) {
    char buf[128];
    ASSERT(client_fmt_talk(buf, sizeof(buf), "hi there") > 0);
    ASSERT_STR_EQ(buf, "T,hi there\n");
}

static void test_fmt_rejects_tiny_buffer(void) {
    char buf[4];
    ASSERT_INT_EQ(client_fmt_version(buf, sizeof(buf), 1), -1);
    ASSERT_INT_EQ(client_fmt_block(buf, sizeof(buf), 1, 2, 3, 4), -1);
}

static void test_disabled_send_helpers_are_nops(void) {
    /* No socket: disabled client must not attempt I/O / exit. */
    client_disable();
    client_version(CLIENT_PROTOCOL_VERSION);
    client_login("x", "y");
    client_position(1, 2, 3, 0, 0);
    client_chunk(0, 0, 0);
    client_block(0, 0, 0, 1);
    client_light(0, 0, 0, 1);
    client_sign(0, 0, 0, 0, "x");
    client_talk("hello");
    ASSERT(client_recv() == NULL);
    ASSERT_INT_EQ(get_client_enabled(), 0);
}

void test_client_suite(void) {
    test_enable_flag();
    test_fmt_version();
    test_fmt_login();
    test_fmt_position();
    test_fmt_chunk();
    test_fmt_block_place_and_break();
    test_fmt_light();
    test_fmt_sign();
    test_fmt_talk();
    test_fmt_rejects_tiny_buffer();
    test_disabled_send_helpers_are_nops();
}
