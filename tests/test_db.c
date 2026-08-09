#include "test_harness.h"

#include "db.h"
#include "map.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

/*
 * Characterisation: block persistence via db_insert_block / db_load_blocks.
 * Deletes are stored as insert-or-replace with w=0 (same as main.c break path).
 * close() drains the async worker and commits, matching session end.
 */

static char g_tmpdir[256];
static char g_oldcwd[4096];

static void enter_temp_dir(void) {
    char tmpl[] = "/tmp/craft_db_test_XXXXXX";
    char *dir = mkdtemp(tmpl);
    ASSERT(dir != NULL);
    strncpy(g_tmpdir, dir, sizeof(g_tmpdir) - 1);
    g_tmpdir[sizeof(g_tmpdir) - 1] = '\0';
    ASSERT(getcwd(g_oldcwd, sizeof(g_oldcwd)) != NULL);
    ASSERT(chdir(g_tmpdir) == 0);
}

static void leave_temp_dir(void) {
    ASSERT(chdir(g_oldcwd) == 0);
}

static void open_db(char *path) {
    db_enable();
    ASSERT_INT_EQ(db_init(path), 0);
    ASSERT_INT_EQ(get_db_enabled(), 1);
}

static void close_db(void) {
    db_close();
    db_disable();
    ASSERT_INT_EQ(get_db_enabled(), 0);
}

static void test_block_insert_read_delete_roundtrip(void) {
    char path[] = "roundtrip.db";
    const int p = 1;
    const int q = -2;
    const int x = 10;
    const int y = 20;
    const int z = 30;
    const int w = 7; /* arbitrary non-zero block id */
    Map map;

    enter_temp_dir();

    /* insert → close (worker drain + commit) → reopen → load */
    open_db(path);
    db_insert_block(p, q, x, y, z, w);
    close_db();

    open_db(path);
    map_alloc(&map, 0, 0, 0, 0xff);
    db_load_blocks(&map, p, q);
    ASSERT_INT_EQ(map_get(&map, x, y, z), w);
    /* other chunk empty */
    {
        Map other;
        map_alloc(&other, 0, 0, 0, 0xff);
        db_load_blocks(&other, p + 1, q);
        ASSERT_INT_EQ(map_get(&other, x, y, z), 0);
        map_free(&other);
    }
    map_free(&map);

    /* delete = insert w=0 (game break path), then persist */
    db_insert_block(p, q, x, y, z, 0);
    close_db();

    open_db(path);
    map_alloc(&map, 0, 0, 0, 0xff);
    db_load_blocks(&map, p, q);
    ASSERT_INT_EQ(map_get(&map, x, y, z), 0);
    map_free(&map);
    close_db();

    leave_temp_dir();
}

static void test_block_insert_or_replace(void) {
    char path[] = "replace.db";
    const int p = 0;
    const int q = 0;
    const int x = 1;
    const int y = 2;
    const int z = 3;
    Map map;

    enter_temp_dir();

    open_db(path);
    db_insert_block(p, q, x, y, z, 1);
    db_insert_block(p, q, x, y, z, 4);
    close_db();

    open_db(path);
    map_alloc(&map, 0, 0, 0, 0xff);
    db_load_blocks(&map, p, q);
    ASSERT_INT_EQ(map_get(&map, x, y, z), 4);
    map_free(&map);
    close_db();

    leave_temp_dir();
}

void test_db_suite(void) {
    test_block_insert_read_delete_roundtrip();
    test_block_insert_or_replace();
}
