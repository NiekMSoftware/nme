#include <doctest/doctest.h>

#include <nme/core/math/matrix.hpp>

#include <array>

using namespace nme;
using namespace nme::math;

// ============================================================================
// Conventions
// ----------------------------------------------------------------------------
// nme::math is ROW-major and computes result = M * v (v a column vector), so
//   nme(i, j) == mat[i * M + j]         (i = row, j = col)
// GLM is COLUMN-major (result = M * v), so it indexes glm[col][row] == glm[j][i].
// Every element-wise GLM comparison therefore checks nme(i, j) == glm[j][i].
// ============================================================================

namespace {

constexpr double EPS = 1e-5;

template<typename T, usize N, usize M>
void check_mat_approx(const Matrix<T, N, M>& a, const Matrix<T, N, M>& b, double eps = EPS) {
    for (usize i = 0; i < N; ++i)
        for (usize j = 0; j < M; ++j)
            CHECK(a(i, j) == doctest::Approx(static_cast<double>(b(i, j))).epsilon(eps));
}

template<typename T, usize N>
Matrix<T, N, N> diagonal(const std::array<T, N>& d) {
    Matrix<T, N, N> m{};
    for (usize i = 0; i < N; ++i) m(i, i) = d[i];
    return m;
}

template<typename T, usize N>
Matrix<T, N, N> identity_ref() {
    Matrix<T, N, N> m{};
    for (usize i = 0; i < N; ++i) m(i, i) = T(1);
    return m;
}

}  // namespace

#ifdef NME_TEST_WITH_GLM
    #include <glm/glm.hpp>
    #include <glm/gtc/matrix_transform.hpp>
    #include <glm/ext/matrix_clip_space.hpp>

namespace {
void check_glm(const Matrix4f& a, const glm::mat4& g, float eps = 1e-4f) {
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            CHECK(a(usize(i), usize(j)) == doctest::Approx(g[j][i]).epsilon(eps));
}
void check_glm(const Matrix3f& a, const glm::mat3& g, float eps = 1e-4f) {
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            CHECK(a(usize(i), usize(j)) == doctest::Approx(g[j][i]).epsilon(eps));
}
}  // namespace
#endif

// ============================================================================
// Traits, introspection, raw access
// ============================================================================

TEST_CASE("traits and static introspection") {
    static_assert(std::is_same_v<Matrix4f::value_type, f32>);
    static_assert(std::is_same_v<Matrix4f::size_type, usize>);
    static_assert(std::is_same_v<Matrix4f::type, Matrix<f32, 4, 4>>);

    static_assert(Matrix4f::row_count == 4 && Matrix4f::col_count == 4);
    static_assert(Matrix2x3f::row_count == 2 && Matrix2x3f::col_count == 3);

    CHECK(Matrix4f::rows() == 4);
    CHECK(Matrix4f::cols() == 4);
    CHECK(Matrix4f::size() == 16);
    CHECK(Matrix2x3f::rows() == 2);
    CHECK(Matrix2x3f::cols() == 3);
    CHECK(Matrix2x3f::size() == 6);
}

TEST_CASE("default construction is all zeros") {
    Matrix4f m;
    for (usize i = 0; i < 16; ++i) CHECK(m.data()[i] == doctest::Approx(0.f));

    Matrix2x3f r;
    for (usize i = 0; i < 6; ++i) CHECK(r.data()[i] == doctest::Approx(0.f));
}

TEST_CASE("single-scalar constructor broadcasts to every element") {
    // The single-scalar ctor fills every element with s (a full broadcast),
    // which is what clamp(m, lo, hi) with scalar bounds relies on.
    Matrix4f m(2.f);
    for (usize i = 0; i < 4; ++i)
        for (usize j = 0; j < 4; ++j)
            CHECK(m(i, j) == doctest::Approx(2.f));
}

TEST_CASE("component-pack constructor") {
    SUBCASE("scalars fill row-major and zero-fill the tail") {
        Matrix4f m(
            1.f,  2.f,  3.f,  4.f,
            5.f,  6.f,  7.f,  8.f,
            9.f, 10.f, 11.f, 12.f);   // last row omitted
        CHECK(m(0, 0) == doctest::Approx(1.f));
        CHECK(m(1, 0) == doctest::Approx(5.f));
        CHECK(m(2, 3) == doctest::Approx(12.f));
        CHECK(m(3, 0) == doctest::Approx(0.f));
        CHECK(m(3, 3) == doctest::Approx(0.f));
    }
    SUBCASE("vectors are flattened into the element stream") {
        Matrix2f m(Vector<f32, 2>(1.f, 2.f), Vector<f32, 2>(3.f, 4.f));
        CHECK(m(0, 0) == doctest::Approx(1.f));
        CHECK(m(0, 1) == doctest::Approx(2.f));
        CHECK(m(1, 0) == doctest::Approx(3.f));
        CHECK(m(1, 1) == doctest::Approx(4.f));
    }
    SUBCASE("overflowing arguments are dropped") {
        Matrix2f m(1.f, 2.f, 3.f, 4.f, 5.f, 6.f);  // extra 5,6 ignored
        CHECK(m(1, 1) == doctest::Approx(4.f));
    }
}

TEST_CASE("converting constructor and assignment") {
    SUBCASE("element-type conversion") {
        Matrix2f f(1.f, 2.f, 3.f, 4.f);
        Matrix2d d(f);
        CHECK(d(0, 0) == doctest::Approx(1.0));
        CHECK(d(1, 1) == doctest::Approx(4.0));
    }
    SUBCASE("shrink copies the overlapping top-left block") {
        Matrix4f big(
            1.f,  2.f,  3.f,  4.f,
            5.f,  6.f,  7.f,  8.f,
            9.f, 10.f, 11.f, 12.f,
           13.f, 14.f, 15.f, 16.f);
        Matrix2f small(big);
        CHECK(small(0, 0) == doctest::Approx(1.f));
        CHECK(small(0, 1) == doctest::Approx(2.f));
        CHECK(small(1, 0) == doctest::Approx(5.f));
        CHECK(small(1, 1) == doctest::Approx(6.f));
    }
    SUBCASE("grow embeds and zero-fills") {
        Matrix2f small(1.f, 2.f, 3.f, 4.f);
        Matrix4f big(small);
        CHECK(big(0, 0) == doctest::Approx(1.f));
        CHECK(big(1, 1) == doctest::Approx(4.f));
        CHECK(big(0, 2) == doctest::Approx(0.f));
        CHECK(big(3, 3) == doctest::Approx(0.f));
    }
    SUBCASE("assignment operator behaves like the converting ctor") {
        Matrix2f small(1.f, 2.f, 3.f, 4.f);
        Matrix4f big;
        big = small;
        CHECK(big(0, 0) == doctest::Approx(1.f));
        CHECK(big(1, 1) == doctest::Approx(4.f));
        CHECK(big(2, 2) == doctest::Approx(0.f));
    }
}

TEST_CASE("element, row, and named access all alias the same storage") {
    Matrix4f m(
        1.f,  2.f,  3.f,  4.f,
        5.f,  6.f,  7.f,  8.f,
        9.f, 10.f, 11.f, 12.f,
       13.f, 14.f, 15.f, 16.f);

    SUBCASE("data() is contiguous row-major") {
        for (usize i = 0; i < 4; ++i)
            for (usize j = 0; j < 4; ++j)
                CHECK(m.data()[i * 4 + j] == doctest::Approx(m(i, j)));
    }
    SUBCASE("operator[] returns the row and matches operator()") {
        for (usize i = 0; i < 4; ++i)
            for (usize j = 0; j < 4; ++j)
                CHECK(m[i][j] == doctest::Approx(m(i, j)));
    }
    SUBCASE("named accessors X/Y/Z/W alias the rows") {
        CHECK(m.X[0] == doctest::Approx(m(0, 0)));
        CHECK(m.Y[1] == doctest::Approx(m(1, 1)));
        CHECK(m.Z[2] == doctest::Approx(m(2, 2)));
        CHECK(m.W[3] == doctest::Approx(m(3, 3)));
    }
    SUBCASE("writes through operator() are visible through data()") {
        m(2, 1) = 99.f;
        CHECK(m.data()[2 * 4 + 1] == doctest::Approx(99.f));
    }
}

// ============================================================================
// Arithmetic operators
// ============================================================================

TEST_CASE("unary plus and minus") {
    Matrix2f m(1.f, -2.f, 3.f, -4.f);
    check_mat_approx(+m, m);
    Matrix2f neg = -m;
    CHECK(neg(0, 0) == doctest::Approx(-1.f));
    CHECK(neg(0, 1) == doctest::Approx(2.f));
    CHECK(neg(1, 1) == doctest::Approx(4.f));
}

TEST_CASE("matrix addition and subtraction") {
    Matrix2f a(1.f, 2.f, 3.f, 4.f);
    Matrix2f b(5.f, 6.f, 7.f, 8.f);

    check_mat_approx(a + b, Matrix2f(6.f, 8.f, 10.f, 12.f));
    check_mat_approx(b - a, Matrix2f(4.f, 4.f, 4.f, 4.f));

    Matrix2f c = a;
    c += b;
    check_mat_approx(c, a + b);
    c -= b;
    check_mat_approx(c, a);
}

TEST_CASE("component-wise product and quotient") {
    Matrix2f a(1.f, 2.f, 3.f, 4.f);
    Matrix2f b(2.f, 2.f, 2.f, 2.f);
    check_mat_approx(comp_mul(a, b), Matrix2f(2.f, 4.f, 6.f, 8.f));
    check_mat_approx(comp_div(a, b), Matrix2f(0.5f, 1.f, 1.5f, 2.f));
}

TEST_CASE("scalar broadcast operators") {
    Matrix2f m(1.f, 2.f, 3.f, 4.f);

    check_mat_approx(m * 2.f, Matrix2f(2.f, 4.f, 6.f, 8.f));
    check_mat_approx(2.f * m, Matrix2f(2.f, 4.f, 6.f, 8.f));   // commutative
    check_mat_approx(m / 2.f, Matrix2f(0.5f, 1.f, 1.5f, 2.f));

    Matrix2f a = m;
    a *= 3.f;
    check_mat_approx(a, m * 3.f);
    a /= 3.f;
    check_mat_approx(a, m);
}

TEST_CASE("square matrix product") {
    SUBCASE("multiplying by identity is a no-op") {
        Matrix3f m(1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 10.f);
        check_mat_approx(m * identity_ref<f32, 3>(), m);
        check_mat_approx(identity_ref<f32, 3>() * m, m);
    }
    SUBCASE("known 2x2 product") {
        Matrix2f a(1.f, 2.f, 3.f, 4.f);
        Matrix2f b(5.f, 6.f, 7.f, 8.f);
        // [1 2;3 4] * [5 6;7 8] = [19 22; 43 50]
        check_mat_approx(a * b, Matrix2f(19.f, 22.f, 43.f, 50.f));
    }
    SUBCASE("associativity") {
        Matrix2f a(1.f, 2.f, 3.f, 4.f);
        Matrix2f b(2.f, 0.f, 1.f, 2.f);
        Matrix2f c(1.f, 1.f, 0.f, 1.f);
        check_mat_approx((a * b) * c, a * (b * c));
    }
}

TEST_CASE("non-square matrix product") {
    // (2x3) * (3x2) -> (2x2)
    Matrix2x3f a(1.f, 2.f, 3.f,
                 4.f, 5.f, 6.f);
    Matrix3x2f b(7.f,  8.f,
                 9.f, 10.f,
                11.f, 12.f);
    // row0: 1*7+2*9+3*11=58 ; 1*8+2*10+3*12=64
    // row1: 4*7+5*9+6*11=139; 4*8+5*10+6*12=154
    Matrix2f r = a * b;
    check_mat_approx(r, Matrix2f(58.f, 64.f, 139.f, 154.f));
}

TEST_CASE("matrix * vector (column vector)") {
    Matrix3f m(1.f, 2.f, 3.f,
               4.f, 5.f, 6.f,
               7.f, 8.f, 9.f);
    Vector<f32, 3> v(1.f, 0.f, -1.f);
    Vector<f32, 3> r = m * v;
    CHECK(r[0] == doctest::Approx(1.f - 3.f));   // -2
    CHECK(r[1] == doctest::Approx(4.f - 6.f));   // -2
    CHECK(r[2] == doctest::Approx(7.f - 9.f));   // -2
}

TEST_CASE("vector * matrix (row vector)") {
    Matrix3f m(1.f, 2.f, 3.f,
               4.f, 5.f, 6.f,
               7.f, 8.f, 9.f);
    Vector<f32, 3> v(1.f, 0.f, -1.f);
    Vector<f32, 3> r = v * m;   // result[j] = sum_i v[i] * m(i, j)
    CHECK(r[0] == doctest::Approx(1.f - 7.f));   // -6
    CHECK(r[1] == doctest::Approx(2.f - 8.f));   // -6
    CHECK(r[2] == doctest::Approx(3.f - 9.f));   // -6
}

TEST_CASE("in-place matrix compose (*=)") {
    Matrix2f a(1.f, 2.f, 3.f, 4.f);
    Matrix2f b(5.f, 6.f, 7.f, 8.f);
    Matrix2f expected = a * b;
    a *= b;
    check_mat_approx(a, expected);
}

TEST_CASE("equality and ordering") {
    Matrix2i a(1, 2, 3, 4);
    Matrix2i b(1, 2, 3, 4);
    Matrix2i c(1, 2, 3, 5);   // greater in the last element

    CHECK(a == b);
    CHECK(a != c);
    CHECK(a < c);
    CHECK(c > a);
    CHECK(a <= b);
    CHECK(a >= b);
    CHECK(a <= c);
    CHECK_FALSE(c <= a);
}

// ============================================================================
// Algorithms
// ============================================================================

TEST_CASE("transpose") {
    SUBCASE("square") {
        Matrix3f m(1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f);
        Matrix3f t = transpose(m);
        for (usize i = 0; i < 3; ++i)
            for (usize j = 0; j < 3; ++j)
                CHECK(t(i, j) == doctest::Approx(m(j, i)));
    }
    SUBCASE("non-square flips dimensions") {
        Matrix2x3f m(1.f, 2.f, 3.f, 4.f, 5.f, 6.f);
        Matrix3x2f t = transpose(m);
        CHECK(t.rows() == 3);
        CHECK(t.cols() == 2);
        for (usize i = 0; i < 2; ++i)
            for (usize j = 0; j < 3; ++j)
                CHECK(t(j, i) == doctest::Approx(m(i, j)));
    }
    SUBCASE("double transpose is identity") {
        Matrix2x3f m(1.f, 2.f, 3.f, 4.f, 5.f, 6.f);
        check_mat_approx(transpose(transpose(m)), m);
    }
}

TEST_CASE("determinant") {
    CHECK(determinant(Matrix<f32, 1, 1>(5.f)) == doctest::Approx(5.f));
    CHECK(determinant(Matrix2f(1.f, 2.f, 3.f, 4.f)) == doctest::Approx(-2.f));
    CHECK(determinant(Matrix3f(6.f, 1.f, 1.f,
                               4.f, -2.f, 5.f,
                               2.f, 8.f, 7.f)) == doctest::Approx(-306.f));
    CHECK(determinant(identity_ref<f32, 4>()) == doctest::Approx(1.f));
    CHECK(determinant(diagonal<f32, 4>({2.f, 3.f, 4.f, 5.f})) == doctest::Approx(120.f));
}

TEST_CASE("determinant generic path (5x5, triangular)") {
    // Upper-triangular: determinant is the product of the diagonal.
    Matrix<f32, 5, 5> m{};
    const std::array<f32, 5> diag{2.f, 3.f, 1.f, 4.f, 5.f};
    for (usize i = 0; i < 5; ++i)
        for (usize j = i; j < 5; ++j)
            m(i, j) = (i == j) ? diag[i] : 1.f;   // arbitrary above-diagonal fill
    CHECK(determinant(m) == doctest::Approx(2.f * 3.f * 1.f * 4.f * 5.f));  // 120
}

TEST_CASE("inverse satisfies M * inverse(M) == I") {
    SUBCASE("1x1") {
        Matrix<f32, 1, 1> m(4.f);
        CHECK(inverse(m)(0, 0) == doctest::Approx(0.25f));
    }
    SUBCASE("2x2") {
        Matrix2f m(4.f, 7.f, 2.f, 6.f);
        check_mat_approx(m * inverse(m), identity_ref<f32, 2>());
    }
    SUBCASE("3x3") {
        Matrix3f m(3.f, 0.f, 2.f,
                   2.f, 0.f, -2.f,
                   0.f, 1.f, 1.f);
        check_mat_approx(m * inverse(m), identity_ref<f32, 3>(), 1e-4);
    }
    SUBCASE("4x4 from a composed transform") {
        Matrix4f m = translate(Vector<f32, 3>(1.f, 2.f, 3.f))
                   * rotate_z(0.7f)
                   * scale(Vector<f32, 4>(2.f, 3.f, 4.f, 0.f));
        check_mat_approx(m * inverse(m), identity_ref<f32, 4>(), 1e-4);
    }
    SUBCASE("inverse of identity is identity") {
        check_mat_approx(inverse(identity_ref<f32, 4>()), identity_ref<f32, 4>());
    }
}

// ============================================================================
// Reductions / element-wise helpers (matrix_common.hpp)
// ============================================================================

TEST_CASE("trace, sum, product") {
    Matrix3f m(1.f, 9.f, 9.f,
               9.f, 2.f, 9.f,
               9.f, 9.f, 3.f);
    CHECK(trace(m) == doctest::Approx(6.f));

    Matrix2f s(1.f, 2.f, 3.f, 4.f);
    CHECK(sum(s) == doctest::Approx(10.f));
    CHECK(product(s) == doctest::Approx(24.f));
}

TEST_CASE("frobenius norm") {
    Matrix2f m(1.f, 2.f, 2.f, 4.f);   // sum of squares = 1+4+4+16 = 25
    CHECK(norm_euclidian_sqrt(m) == doctest::Approx(25.f));
    CHECK(norm_euclidian(m) == doctest::Approx(5.f));
}

TEST_CASE("abs, min, max, sign") {
    Matrix2f m(-1.f, 2.f, -3.f, 4.f);
    check_mat_approx(abs(m), Matrix2f(1.f, 2.f, 3.f, 4.f));
    check_mat_approx(sign(m), Matrix2f(-1.f, 1.f, -1.f, 1.f));

    Matrix2f a(1.f, 5.f, 3.f, 8.f);
    Matrix2f b(4.f, 2.f, 6.f, 7.f);
    check_mat_approx(min(a, b), Matrix2f(1.f, 2.f, 3.f, 7.f));
    check_mat_approx(max(a, b), Matrix2f(4.f, 5.f, 6.f, 8.f));
}

TEST_CASE("clamp with matrix bounds") {
    Matrix2f m(-5.f, 0.5f, 3.f, 10.f);
    Matrix2f lo(0.f, 0.f, 0.f, 0.f);
    Matrix2f hi(1.f, 1.f, 1.f, 1.f);
    check_mat_approx(clamp(m, lo, hi), Matrix2f(0.f, 0.5f, 1.f, 1.f));
}

TEST_CASE("clamp with scalar bounds") {
    // Builds Matrix(lo)/Matrix(hi) via the broadcasting single-scalar ctor.
    Matrix2f m(-5.f, 0.5f, 3.f, 10.f);
    check_mat_approx(clamp(m, 0.f, 1.f), Matrix2f(0.f, 0.5f, 1.f, 1.f));
}

TEST_CASE("lerp") {
    Matrix2f a(0.f, 0.f, 0.f, 0.f);
    Matrix2f b(10.f, 20.f, 30.f, 40.f);
    check_mat_approx(lerp(a, b, 0.f), a);
    check_mat_approx(lerp(a, b, 1.f), b);
    check_mat_approx(lerp(a, b, 0.5f), Matrix2f(5.f, 10.f, 15.f, 20.f));
}

TEST_CASE("floor, ceil, round") {
    Matrix2f m(1.2f, 1.8f, -1.2f, -1.8f);
    check_mat_approx(floor(m), Matrix2f(1.f, 1.f, -2.f, -2.f));
    check_mat_approx(ceil(m), Matrix2f(2.f, 2.f, -1.f, -1.f));
    check_mat_approx(round(m), Matrix2f(1.f, 2.f, -1.f, -2.f));
}

// ============================================================================
// Constants
// ============================================================================

TEST_CASE("zero and identity constants") {
    for (usize i = 0; i < 4; ++i)
        for (usize j = 0; j < 4; ++j) {
            CHECK(Matrix4f::zero(i, j) == doctest::Approx(0.f));
            CHECK(Matrix4f::identity(i, j) == doctest::Approx(i == j ? 1.f : 0.f));
        }
}

// ============================================================================
// Integer matrices (no floating helpers, exact arithmetic)
// ============================================================================

TEST_CASE("integer matrices") {
    Matrix2i a(1, 2, 3, 4);
    Matrix2i b(5, 6, 7, 8);

    CHECK((a + b) == Matrix2i(6, 8, 10, 12));
    CHECK((a * b) == Matrix2i(19, 22, 43, 50));
    CHECK(comp_mul(a, b) == Matrix2i(5, 12, 21, 32));
    CHECK(determinant(a) == -2);
    CHECK(trace(a) == 5);

    Matrix2u u(1u, 2u, 3u, 4u);
    CHECK((u + u) == Matrix2u(2u, 4u, 6u, 8u));   // unsigned: no unary minus
}

// ============================================================================
// Transforms
// ============================================================================

TEST_CASE("translate moves points but not directions") {
    const Matrix4f T = translate(Vector<f32, 3>(5.f, 6.f, 7.f));

    Vector<f32, 4> point(1.f, 2.f, 3.f, 1.f);
    Vector<f32, 4> movedP = T * point;
    CHECK(movedP[0] == doctest::Approx(6.f));
    CHECK(movedP[1] == doctest::Approx(8.f));
    CHECK(movedP[2] == doctest::Approx(10.f));
    CHECK(movedP[3] == doctest::Approx(1.f));

    Vector<f32, 4> dir(1.f, 2.f, 3.f, 0.f);
    Vector<f32, 4> movedD = T * dir;
    CHECK(movedD[0] == doctest::Approx(1.f));
    CHECK(movedD[1] == doctest::Approx(2.f));
    CHECK(movedD[2] == doctest::Approx(3.f));
    CHECK(movedD[3] == doctest::Approx(0.f));
}

TEST_CASE("scale") {
    SUBCASE("4x4") {
        Matrix4f S = scale(Vector<f32, 4>(2.f, 3.f, 4.f, 0.f));  // w ignored
        CHECK(S(0, 0) == doctest::Approx(2.f));
        CHECK(S(1, 1) == doctest::Approx(3.f));
        CHECK(S(2, 2) == doctest::Approx(4.f));
        CHECK(S(3, 3) == doctest::Approx(1.f));
    }
    SUBCASE("2D scale as 3x3") {
        Matrix3f S = scale(Vector<f32, 2>(2.f, 3.f));
        CHECK(S(0, 0) == doctest::Approx(2.f));
        CHECK(S(1, 1) == doctest::Approx(3.f));
        CHECK(S(2, 2) == doctest::Approx(1.f));
    }
}

TEST_CASE("rotations") {
    const f32 a = 0.7f;
    const f32 c = std::cos(a), s = std::sin(a);

    SUBCASE("rotate_z rotates +X toward +Y") {
        Matrix4f R = rotate_z(a);
        Vector<f32, 4> r = R * Vector<f32, 4>(1.f, 0.f, 0.f, 0.f);
        CHECK(r[0] == doctest::Approx(c));
        CHECK(r[1] == doctest::Approx(s));
        CHECK(r[2] == doctest::Approx(0.f));
    }
    SUBCASE("rotate_y rotates +Z toward +X") {
        Matrix4f R = rotate_y(a);
        Vector<f32, 4> r = R * Vector<f32, 4>(0.f, 0.f, 1.f, 0.f);
        CHECK(r[0] == doctest::Approx(s));
        CHECK(r[1] == doctest::Approx(0.f));
        CHECK(r[2] == doctest::Approx(c));
    }
    SUBCASE("axis-angle about Z equals rotate_z") {
        check_mat_approx(rotate_axis_angle(Vector<f32, 3>(0.f, 0.f, 1.f), a), rotate_z(a));
    }
    SUBCASE("axis-angle about Y equals rotate_y") {
        check_mat_approx(rotate_axis_angle(Vector<f32, 3>(0.f, 1.f, 0.f), a), rotate_y(a));
    }
}

// ============================================================================
// GLM parity
// ============================================================================

#ifdef NME_TEST_WITH_GLM

TEST_CASE("basic transforms match GLM") {
    check_glm(translate(Vector<f32, 3>(1.5f, -2.f, 3.25f)),
              glm::translate(glm::mat4(1.f), glm::vec3(1.5f, -2.f, 3.25f)));

    check_glm(scale(Vector<f32, 4>(2.f, 3.f, 0.5f, 0.f)),
              glm::scale(glm::mat4(1.f), glm::vec3(2.f, 3.f, 0.5f)));

    check_glm(rotate_y(0.6f), glm::rotate(glm::mat4(1.f), 0.6f, glm::vec3(0.f, 1.f, 0.f)));
    check_glm(rotate_z(0.6f), glm::rotate(glm::mat4(1.f), 0.6f, glm::vec3(0.f, 0.f, 1.f)));

    glm::vec3 axis = glm::normalize(glm::vec3(0.3f, 0.7f, 0.5f));
    check_glm(rotate_axis_angle(Vector<f32, 3>(axis.x, axis.y, axis.z), 0.9f),
              glm::rotate(glm::mat4(1.f), 0.9f, axis));
}

TEST_CASE("composed transform T*R*S matches GLM") {
    Matrix4f C = translate(Vector<f32, 3>(1.f, 2.f, 3.f))
               * rotate_z(0.7f)
               * scale(Vector<f32, 4>(2.f, 3.f, 4.f, 0.f));

    glm::mat4 gC = glm::translate(glm::mat4(1.f), glm::vec3(1.f, 2.f, 3.f))
                 * glm::rotate(glm::mat4(1.f), 0.7f, glm::vec3(0.f, 0.f, 1.f))
                 * glm::scale(glm::mat4(1.f), glm::vec3(2.f, 3.f, 4.f));
    check_glm(C, gC);
}

TEST_CASE("perspective variants match GLM") {
    const f32 fov = 1.0f, asp = 16.f / 9.f, n = 0.1f, f = 100.f;
    check_glm(perspective_lh_01(fov, asp, n, f), glm::perspectiveLH_ZO(fov, asp, n, f));
    check_glm(perspective_lh_11(fov, asp, n, f), glm::perspectiveLH_NO(fov, asp, n, f));
    check_glm(perspective_rh_01(fov, asp, n, f), glm::perspectiveRH_ZO(fov, asp, n, f));
    check_glm(perspective_rh_11(fov, asp, n, f), glm::perspectiveRH_NO(fov, asp, n, f));
}

TEST_CASE("orthographic variants match GLM") {
    const f32 l = -2.f, r = 3.f, b = -1.f, t = 1.5f, n = 0.1f, f = 50.f;
    check_glm(orthographic_lh_01(l, r, b, t, n, f), glm::orthoLH_ZO(l, r, b, t, n, f));
    check_glm(orthographic_lh_11(l, r, b, t, n, f), glm::orthoLH_NO(l, r, b, t, n, f));
    check_glm(orthographic_rh_01(l, r, b, t, n, f), glm::orthoRH_ZO(l, r, b, t, n, f));
    check_glm(orthographic_rh_11(l, r, b, t, n, f), glm::orthoRH_NO(l, r, b, t, n, f));
}

TEST_CASE("view matrices match GLM") {
    glm::vec3 eye(4.f, 3.f, 5.f), target(0.f, 0.f, 0.f), up(0.f, 1.f, 0.f);
    Vector<f32, 3> e(eye.x, eye.y, eye.z), tg(target.x, target.y, target.z), u(up.x, up.y, up.z);

    check_glm(look_at_lh(e, tg, u), glm::lookAtLH(eye, target, up));
    check_glm(look_at_rh(e, tg, u), glm::lookAtRH(eye, target, up));
}

TEST_CASE("look_to matches GLM (equivalent to look_at at eye + dir)") {
    glm::vec3 eye(4.f, 3.f, 5.f), dir(1.f, 0.5f, -2.f), up(0.f, 1.f, 0.f);
    Vector<f32, 3> e(eye.x, eye.y, eye.z), d(dir.x, dir.y, dir.z), u(up.x, up.y, up.z);

    // look_to(eye, dir) == look_at(eye, eye + dir); dir need not be normalized.
    check_glm(look_to_lh(e, d, u), glm::lookAtLH(eye, eye + dir, up));
    check_glm(look_to_rh(e, d, u), glm::lookAtRH(eye, eye + dir, up));

    // look_to and look_at agree when fed matching arguments.
    check_mat_approx(look_to_lh(e, d, u), look_at_lh(e, e + d, u));
    check_mat_approx(look_to_rh(e, d, u), look_at_rh(e, e + d, u));
}

#endif  // NME_TEST_WITH_GLM

// ============================================================================
// Config dispatchers (compile against whatever clip-space macros are set)
// ============================================================================

TEST_CASE("perspective/orthographic/look dispatchers wire to the configured variant") {
    const f32 fov = 1.0f, asp = 1.5f, n = 0.1f, f = 100.f;

#if NME_HANDEDNESS == NME_LEFT_HANDED && NME_DEPTH_RANGE == NME_DEPTH_ZERO_TO_ONE
    check_mat_approx(perspective(fov, asp, n, f), perspective_lh_01(fov, asp, n, f));
#elif NME_HANDEDNESS == NME_LEFT_HANDED && NME_DEPTH_RANGE == NME_DEPTH_NEG_ONE_TO_ONE
    check_mat_approx(perspective(fov, asp, n, f), perspective_lh_11(fov, asp, n, f));
#elif NME_HANDEDNESS == NME_RIGHT_HANDED && NME_DEPTH_RANGE == NME_DEPTH_ZERO_TO_ONE
    check_mat_approx(perspective(fov, asp, n, f), perspective_rh_01(fov, asp, n, f));
#else
    check_mat_approx(perspective(fov, asp, n, f), perspective_rh_11(fov, asp, n, f));
#endif

    const f32 l = -1.f, r = 1.f, b = -1.f, t = 1.f;
#if NME_HANDEDNESS == NME_LEFT_HANDED && NME_DEPTH_RANGE == NME_DEPTH_ZERO_TO_ONE
    check_mat_approx(orthographic(l, r, b, t, n, f), orthographic_lh_01(l, r, b, t, n, f));
#elif NME_HANDEDNESS == NME_LEFT_HANDED && NME_DEPTH_RANGE == NME_DEPTH_NEG_ONE_TO_ONE
    check_mat_approx(orthographic(l, r, b, t, n, f), orthographic_lh_11(l, r, b, t, n, f));
#elif NME_HANDEDNESS == NME_RIGHT_HANDED && NME_DEPTH_RANGE == NME_DEPTH_ZERO_TO_ONE
    check_mat_approx(orthographic(l, r, b, t, n, f), orthographic_rh_01(l, r, b, t, n, f));
#else
    check_mat_approx(orthographic(l, r, b, t, n, f), orthographic_rh_11(l, r, b, t, n, f));
#endif

    Vector<f32, 3> eye(4.f, 3.f, 5.f), target(0.f, 0.f, 0.f), up(0.f, 1.f, 0.f);
#if NME_HANDEDNESS == NME_LEFT_HANDED
    check_mat_approx(look_at(eye, target, up), look_at_lh(eye, target, up));
    check_mat_approx(look_to(eye, target - eye, up), look_to_lh(eye, target - eye, up));
#else
    check_mat_approx(look_at(eye, target, up), look_at_rh(eye, target, up));
    check_mat_approx(look_to(eye, target - eye, up), look_to_rh(eye, target - eye, up));
#endif
}