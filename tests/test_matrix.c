#include "test_harness.h"
#include "matrix.h"

static int mat_eq(const float *a, const float *b, float eps) {
    for (int i = 0; i < 16; i++) {
        if (fabsf(a[i] - b[i]) > eps) {
            return 0;
        }
    }
    return 1;
}

static void test_normalize(void) {
    float x = 3.0f, y = 0.0f, z = 4.0f;
    normalize(&x, &y, &z);
    ASSERT_FLOAT_EQ(x, 0.6f, 1e-6f);
    ASSERT_FLOAT_EQ(y, 0.0f, 1e-6f);
    ASSERT_FLOAT_EQ(z, 0.8f, 1e-6f);
    ASSERT_FLOAT_EQ(x * x + y * y + z * z, 1.0f, 1e-6f);
}

static void test_mat_identity(void) {
    float m[16];
    float expected[16] = {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    };
    memset(m, 0xcd, sizeof(m));
    mat_identity(m);
    ASSERT(mat_eq(m, expected, 0.0f));
}

static void test_mat_translate(void) {
    float m[16];
    mat_translate(m, 2.0f, -3.0f, 5.0f);
    ASSERT_FLOAT_EQ(m[0], 1.0f, 0.0f);
    ASSERT_FLOAT_EQ(m[5], 1.0f, 0.0f);
    ASSERT_FLOAT_EQ(m[10], 1.0f, 0.0f);
    ASSERT_FLOAT_EQ(m[15], 1.0f, 0.0f);
    ASSERT_FLOAT_EQ(m[12], 2.0f, 0.0f);
    ASSERT_FLOAT_EQ(m[13], -3.0f, 0.0f);
    ASSERT_FLOAT_EQ(m[14], 5.0f, 0.0f);
    ASSERT_FLOAT_EQ(m[1], 0.0f, 0.0f);
    ASSERT_FLOAT_EQ(m[4], 0.0f, 0.0f);
}

static void test_mat_multiply_identity(void) {
    float a[16], b[16], out[16];
    mat_identity(a);
    mat_translate(b, 1.0f, 2.0f, 3.0f);
    mat_multiply(out, a, b);
    ASSERT(mat_eq(out, b, 1e-6f));
    mat_multiply(out, b, a);
    ASSERT(mat_eq(out, b, 1e-6f));
}

static void test_mat_vec_multiply_identity(void) {
    float m[16];
    float v[4] = {1.0f, 2.0f, 3.0f, 1.0f};
    float out[4];
    mat_identity(m);
    mat_vec_multiply(out, m, v);
    ASSERT_FLOAT_EQ(out[0], 1.0f, 1e-6f);
    ASSERT_FLOAT_EQ(out[1], 2.0f, 1e-6f);
    ASSERT_FLOAT_EQ(out[2], 3.0f, 1e-6f);
    ASSERT_FLOAT_EQ(out[3], 1.0f, 1e-6f);
}

static void test_mat_vec_multiply_translate(void) {
    float m[16];
    float v[4] = {1.0f, 2.0f, 3.0f, 1.0f};
    float out[4];
    mat_translate(m, 10.0f, 20.0f, 30.0f);
    mat_vec_multiply(out, m, v);
    ASSERT_FLOAT_EQ(out[0], 11.0f, 1e-6f);
    ASSERT_FLOAT_EQ(out[1], 22.0f, 1e-6f);
    ASSERT_FLOAT_EQ(out[2], 33.0f, 1e-6f);
    ASSERT_FLOAT_EQ(out[3], 1.0f, 1e-6f);
}

static void test_mat_ortho_bounds(void) {
    float m[16];
    mat_ortho(m, 0.0f, 200.0f, 0.0f, 100.0f, -1.0f, 1.0f);
    ASSERT_FLOAT_EQ(m[0], 2.0f / 200.0f, 1e-6f);
    ASSERT_FLOAT_EQ(m[5], 2.0f / 100.0f, 1e-6f);
    ASSERT_FLOAT_EQ(m[10], -2.0f / 2.0f, 1e-6f);
    ASSERT_FLOAT_EQ(m[12], -1.0f, 1e-6f);
    ASSERT_FLOAT_EQ(m[13], -1.0f, 1e-6f);
    ASSERT_FLOAT_EQ(m[15], 1.0f, 0.0f);
}

static void test_mat_frustum_perspective_shape(void) {
    float m[16];
    mat_frustum(m, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 100.0f);
    ASSERT_FLOAT_EQ(m[0], 1.0f, 1e-6f);
    ASSERT_FLOAT_EQ(m[5], 1.0f, 1e-6f);
    ASSERT_FLOAT_EQ(m[11], -1.0f, 0.0f);
    ASSERT_FLOAT_EQ(m[15], 0.0f, 0.0f);
}

static void test_mat_rotate_axis_y_quarter_turn(void) {
    float m[16];
    float v[4] = {1.0f, 0.0f, 0.0f, 1.0f};
    float out[4];
    /* -pi/2 about Y: +X -> +Z */
    mat_rotate(m, 0.0f, 1.0f, 0.0f, -3.14159265359f / 2.0f);
    mat_vec_multiply(out, m, v);
    ASSERT_FLOAT_EQ(out[0], 0.0f, 1e-5f);
    ASSERT_FLOAT_EQ(out[1], 0.0f, 1e-5f);
    ASSERT_FLOAT_EQ(out[2], 1.0f, 1e-5f);
}

void test_matrix_suite(void) {
    test_normalize();
    test_mat_identity();
    test_mat_translate();
    test_mat_multiply_identity();
    test_mat_vec_multiply_identity();
    test_mat_vec_multiply_translate();
    test_mat_ortho_bounds();
    test_mat_frustum_perspective_shape();
    test_mat_rotate_axis_y_quarter_turn();
}
