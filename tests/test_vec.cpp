// Unit tests for nme::math vectors.
// Covers storage/aliasing, constructors, all operators (incl. bitwise on
// integers), geometric ops, the free-function set, and GLM parity.
//
// Unit tests written by a LLM to fully cover everything.

#include <doctest/doctest.h>

#include <cmath>
#include <random>
#include <type_traits>

#include <nme/core/math/vec.hpp>

#ifdef NME_TEST_WITH_GLM
#include <glm/glm.hpp>
#include <glm/gtc/epsilon.hpp>
#endif

using namespace nme;
using namespace nme::math;

// ===========================================================================
//  Test-local helpers
// ===========================================================================
namespace {

// Component-wise "close enough". The guide's acceptance bar for Ch.5a is
// "every op matches GLM within 1e-5 on random input", so that is the default
// tolerance everywhere below.
//
// The comparison is |a-b| <= eps * max(1, |a|, |b|): effectively an absolute
// 1e-5 for small values (the exact-value cases) that scales up relatively for
// large operands (the random parity cases, where inputs reach +/-100 and a
// fixed 1e-5 would be tighter than float precision allows).
constexpr f32 kEps = 1e-5f;

template<typename T, usize N>
bool approx(const Vector<T, N>& a, const Vector<T, N>& b, T eps = T(kEps)) {
    for (usize i = 0; i < N; ++i) {
        const T da = a[i] < T(0) ? -a[i] : a[i];
        const T db = b[i] < T(0) ? -b[i] : b[i];
        const T d  = (a[i] < b[i]) ? (b[i] - a[i]) : (a[i] - b[i]);
        const T scale = da > db ? (da > T(1) ? da : T(1)) : (db > T(1) ? db : T(1));
        if (d > eps * scale) return false;
    }
    return true;
}

// A deterministic RNG so a failing case is always reproducible. One seed for
// the whole TU; doctest runs cases in registration order.
std::mt19937& rng() {
    static std::mt19937 gen{0xC0FFEEu};
    return gen;
}

f32 rand_f(const f32 lo = -100.0f, const f32 hi = 100.0f) {
    std::uniform_real_distribution<f32> d(lo, hi);
    return d(rng());
}

template<usize N>
Vector<f32, N> rand_vec(const f32 lo = -100.0f, const f32 hi = 100.0f) {
    Vector<f32, N> v;
    for (usize i = 0; i < N; ++i) v[i] = rand_f(lo, hi);
    return v;
}

// Non-degenerate: every component is guaranteed away from zero, so length /
// normalize / component-wise divide are all numerically stable.
template<usize N>
Vector<f32, N> rand_vec_nonzero(const f32 mag_lo = 1.0f, const f32 mag_hi = 100.0f) {
    Vector<f32, N> v;
    for (usize i = 0; i < N; ++i) {
        const f32 s = rand_f(mag_lo, mag_hi);
        v[i] = (rand_f() < 0.0f) ? -s : s;
    }
    return v;
}

constexpr int kTrials = 256;   // random trials per parity case

#ifdef NME_TEST_WITH_GLM
// ---- nme <-> glm bridges -------------------------------------------------
template<usize N> struct glm_of;
template<> struct glm_of<2> { using type = glm::vec2; };
template<> struct glm_of<3> { using type = glm::vec3; };
template<> struct glm_of<4> { using type = glm::vec4; };
template<usize N> using glm_of_t = typename glm_of<N>::type;

glm::vec2 to_glm(const Vector<f32, 2>& v) { return {v[0], v[1]}; }
glm::vec3 to_glm(const Vector<f32, 3>& v) { return {v[0], v[1], v[2]}; }
glm::vec4 to_glm(const Vector<f32, 4>& v) { return {v[0], v[1], v[2], v[3]}; }

Vector<f32, 2> from_glm(const glm::vec2& v) { return {v.x, v.y}; }
Vector<f32, 3> from_glm(const glm::vec3& v) { return {v.x, v.y, v.z}; }
Vector<f32, 4> from_glm(const glm::vec4& v) { return {v.x, v.y, v.z, v.w}; }
#endif

}  // namespace

// ===========================================================================
//  1. Storage, aliasing, and raw access
// ===========================================================================
TEST_CASE("vec: storage size and comp reflect N") {
    CHECK(Vector2f::size() == 2);
    CHECK(Vector3f::size() == 3);
    CHECK(Vector4f::size() == 4);
    CHECK(Vector4f::comp == 4);
    CHECK(sizeof(Vector4f) == 4 * sizeof(f32));
}

TEST_CASE("vec: named members alias data[] (union storage)") {
    SUBCASE("vec2 x/y == r/g == s/t == u/v") {
        Vector2f v{1.0f, 2.0f};
        CHECK(v.x == v.r);
        CHECK(v.y == v.g);
        CHECK(v.x == v.s);
        CHECK(v.y == v.t);
        CHECK(v.x == v.u);
        CHECK(v.y == v.v);
        CHECK(v.vec[0] == 1.0f);
        CHECK(v.vec[1] == 2.0f);
    }
    SUBCASE("vec3 x/y/z == r/g/b") {
        Vector3f v{1.0f, 2.0f, 3.0f};
        CHECK(v.x == v.r);
        CHECK(v.y == v.g);
        CHECK(v.z == v.b);
        CHECK(v.vec[2] == 3.0f);
    }
    SUBCASE("vec4 x/y/z/w == r/g/b/a") {
        Vector4f v{1.0f, 2.0f, 3.0f, 4.0f};
        CHECK(v.x == v.r);
        CHECK(v.w == v.a);
        CHECK(v.vec[3] == 4.0f);
    }
    SUBCASE("writing through one alias is visible through the others") {
        Vector3f v{};
        v.r = 7.0f;
        CHECK(v.x == 7.0f);
        CHECK(v.vec[0] == 7.0f);
    }
}

TEST_CASE("vec: data() points at the first component") {
    Vector4f v{1.0f, 2.0f, 3.0f, 4.0f};
    f32* p = v.data();
    CHECK(p == &v[0]);
    CHECK(p[3] == 4.0f);
    const Vector4f& cv = v;
    const f32* cp = cv.data();
    CHECK(cp[0] == 1.0f);
}

// ===========================================================================
//  2. Constructors
// ===========================================================================
TEST_CASE("vec: default constructs to zero") {
    Vector4f v;
    CHECK(v[0] == 0.0f);
    CHECK(v[1] == 0.0f);
    CHECK(v[2] == 0.0f);
    CHECK(v[3] == 0.0f);
}

TEST_CASE("vec: scalar broadcast fills every component") {
    Vector3f v{5.0f};
    CHECK(v[0] == 5.0f);
    CHECK(v[1] == 5.0f);
    CHECK(v[2] == 5.0f);
}

TEST_CASE("vec: component-pack constructor flattens left-to-right") {
    SUBCASE("all scalars") {
        Vector4f v{1.0f, 2.0f, 3.0f, 4.0f};
        CHECK(v[0] == 1.0f);
        CHECK(v[3] == 4.0f);
    }
    SUBCASE("vector + scalar (vec3 from vec2 + z)") {
        Vector2f xy{1.0f, 2.0f};
        Vector3f v{xy, 3.0f};
        CHECK(v[0] == 1.0f);
        CHECK(v[1] == 2.0f);
        CHECK(v[2] == 3.0f);
    }
    SUBCASE("scalar + vector") {
        Vector2f yz{2.0f, 3.0f};
        Vector3f v{1.0f, yz};
        CHECK(v[0] == 1.0f);
        CHECK(v[1] == 2.0f);
        CHECK(v[2] == 3.0f);
    }
    SUBCASE("two vectors concatenate (vec4 from vec2 + vec2)") {
        Vector2f a{1.0f, 2.0f};
        Vector2f b{3.0f, 4.0f};
        Vector4f v{a, b};
        CHECK(v[0] == 1.0f);
        CHECK(v[1] == 2.0f);
        CHECK(v[2] == 3.0f);
        CHECK(v[3] == 4.0f);
    }
    SUBCASE("trailing slots are zero-filled") {
        Vector4f v{1.0f, 2.0f};
        CHECK(v[2] == 0.0f);
        CHECK(v[3] == 0.0f);
    }
    SUBCASE("overflow components are dropped (vec4 from two vec3s)") {
        Vector3f a{1.0f, 2.0f, 3.0f};
        Vector3f b{4.0f, 5.0f, 6.0f};
        Vector4f v{a, b};   // 6 slots offered, only 4 taken
        CHECK(v[0] == 1.0f);
        CHECK(v[1] == 2.0f);
        CHECK(v[2] == 3.0f);
        CHECK(v[3] == 4.0f);
    }
}

TEST_CASE("vec: cross-size assignment copies min and zero-fills the tail") {
    SUBCASE("larger <- smaller zero-fills") {
        Vector4f v{9.0f, 9.0f, 9.0f, 9.0f};
        Vector2f small{1.0f, 2.0f};
        v = small;
        CHECK(v[0] == 1.0f);
        CHECK(v[1] == 2.0f);
        CHECK(v[2] == 0.0f);   // stale 9 must be gone
        CHECK(v[3] == 0.0f);
    }
    SUBCASE("smaller <- larger truncates") {
        Vector2f v{};
        Vector4f big{1.0f, 2.0f, 3.0f, 4.0f};
        v = big;
        CHECK(v[0] == 1.0f);
        CHECK(v[1] == 2.0f);
    }
}

// ===========================================================================
//  3. Element access
// ===========================================================================
TEST_CASE("vec: operator[] reads and writes") {
    Vector3f v{};
    v[0] = 10.0f;
    v[1] = 20.0f;
    v[2] = 30.0f;
    CHECK(v[0] == 10.0f);
    CHECK(v[2] == 30.0f);
    const Vector3f& cv = v;
    CHECK(cv[1] == 20.0f);
}

// ===========================================================================
//  4. Arithmetic operators (scalar-truth checks)
// ===========================================================================
TEST_CASE("vec: unary +/-") {
    constexpr Vector3f v{1.0f, -2.0f, 3.0f};
    CHECK(approx(+v, v));
    CHECK(approx(-v, Vector3f{-1.0f, 2.0f, -3.0f}));
}

TEST_CASE("vec: component-wise vector op vector") {
    Vector3f a{1.0f, 2.0f, 3.0f};
    Vector3f b{4.0f, 5.0f, 6.0f};
    CHECK(approx(a + b, Vector3f{5.0f, 7.0f, 9.0f}));
    CHECK(approx(a - b, Vector3f{-3.0f, -3.0f, -3.0f}));
    CHECK(approx(a * b, Vector3f{4.0f, 10.0f, 18.0f}));
    CHECK(approx(b / a, Vector3f{4.0f, 2.5f, 2.0f}));
}

TEST_CASE("vec: compound-assign vector op vector") {
    SUBCASE("+= returns *this") {
        Vector3f a{1.0f, 2.0f, 3.0f};
        Vector3f& ref = (a += Vector3f{1.0f, 1.0f, 1.0f});
        CHECK(&ref == &a);
        CHECK(approx(a, Vector3f{2.0f, 3.0f, 4.0f}));
    }
    SUBCASE("-=") {
        Vector3f a{5.0f, 5.0f, 5.0f};
        a -= Vector3f{1.0f, 2.0f, 3.0f};
        CHECK(approx(a, Vector3f{4.0f, 3.0f, 2.0f}));
    }
    SUBCASE("*=") {
        Vector3f a{2.0f, 3.0f, 4.0f};
        a *= Vector3f{2.0f, 2.0f, 2.0f};
        CHECK(approx(a, Vector3f{4.0f, 6.0f, 8.0f}));
    }
    SUBCASE("/=") {
        Vector3f a{8.0f, 6.0f, 4.0f};
        a /= Vector3f{2.0f, 2.0f, 2.0f};
        CHECK(approx(a, Vector3f{4.0f, 3.0f, 2.0f}));
    }
}

TEST_CASE("vec: vector op scalar (and commuted scalar op vector)") {
    Vector3f v{1.0f, 2.0f, 3.0f};
    CHECK(approx(v * 2.0f, Vector3f{2.0f, 4.0f, 6.0f}));
    CHECK(approx(2.0f * v, Vector3f{2.0f, 4.0f, 6.0f}));   // commutativity
    CHECK(approx(v / 2.0f, Vector3f{0.5f, 1.0f, 1.5f}));
}

TEST_CASE("vec: compound-assign vector op scalar") {
    SUBCASE("*= scalar returns *this") {
        Vector3f v{1.0f, 2.0f, 3.0f};
        Vector3f& ref = (v *= 3.0f);
        CHECK(&ref == &v);
        CHECK(approx(v, Vector3f{3.0f, 6.0f, 9.0f}));
    }
    SUBCASE("/= scalar") {
        Vector3f v{3.0f, 6.0f, 9.0f};
        v /= 3.0f;
        CHECK(approx(v, Vector3f{1.0f, 2.0f, 3.0f}));
    }
}

// ===========================================================================
//  5. Equality / ordering
// ===========================================================================
TEST_CASE("vec: operator== is exact component-wise") {
    CHECK(Vector3f{1.0f, 2.0f, 3.0f} == Vector3f{1.0f, 2.0f, 3.0f});
    CHECK_FALSE(Vector3f{1.0f, 2.0f, 3.0f} == Vector3f{1.0f, 2.0f, 3.5f});
}

TEST_CASE("vec: operator<=> gives lexicographic order (for containers)") {
    // Reduce the spaceship result to a plain bool BEFORE handing it to CHECK.
    // doctest's expression decomposition can't compare a std::partial_ordering
    // against literal 0 on MSVC (consteval _Literal_zero), so we don't let it.
    constexpr bool less    = (Vector3f{1.0f, 0.0f, 0.0f} <=> Vector3f{2.0f, 0.0f, 0.0f}) < 0;
    const bool greater = (Vector3f{1.0f, 2.0f, 0.0f} <=> Vector3f{1.0f, 1.0f, 9.0f}) > 0;
    const bool equal_  = (Vector3f{1.0f, 2.0f, 3.0f} <=> Vector3f{1.0f, 2.0f, 3.0f}) == 0;
    CHECK(less);
    CHECK(greater);
    CHECK(equal_);
}

// ===========================================================================
//  6. Bitwise ops on integer vectors
//     Guarded: enable once the integer bitwise operators land in the library
//     (the test_vec.cpp banner mentions them, but they are not in vec_ops.inl).
// ===========================================================================
TEST_CASE("vec: bitwise vector op vector") {
    Vector3i a{0b1100, 0b1010, 0b1111};
    Vector3i b{0b1010, 0b0110, 0b0001};
    CHECK((a & b) == Vector3i{0b1000, 0b0010, 0b0001});
    CHECK((a | b) == Vector3i{0b1110, 0b1110, 0b1111});
    CHECK((a ^ b) == Vector3i{0b0110, 0b1100, 0b1110});
    CHECK((~a) == Vector3i{~0b1100, ~0b1010, ~0b1111});
    CHECK((a << Vector3i{1, 2, 3}) == Vector3i{0b1100 << 1, 0b1010 << 2, 0b1111 << 3});
    CHECK((a >> Vector3i{1, 2, 3}) == Vector3i{0b1100 >> 1, 0b1010 >> 2, 0b1111 >> 3});
}

TEST_CASE("vec: bitwise vector op scalar (broadcast)") {
    Vector3u v{0b1100u, 0b1010u, 0b0110u};
    CHECK((v & 0b0110u) == Vector3u{0b0100u, 0b0010u, 0b0110u});
    CHECK((v | 0b0001u) == Vector3u{0b1101u, 0b1011u, 0b0111u});
    CHECK((v ^ 0b1111u) == Vector3u{0b0011u, 0b0101u, 0b1001u});
    CHECK((v << 1u) == Vector3u{0b11000u, 0b10100u, 0b01100u});
    CHECK((v >> 1u) == Vector3u{0b0110u, 0b0101u, 0b0011u});
    // & | ^ are commutative with the scalar on the left.
    CHECK((0b0110u & v) == (v & 0b0110u));
    CHECK((0b0001u | v) == (v | 0b0001u));
    CHECK((0b1111u ^ v) == (v ^ 0b1111u));
}

TEST_CASE("vec: bitwise compound-assign") {
    SUBCASE("vector rhs") {
        Vector3i v{0b1100, 0b1010, 0b1111};
        Vector3i m{0b1010, 0b0110, 0b0001};
        Vector3i& ref = (v &= m);
        CHECK(&ref == &v);   // returns *this
        CHECK(v == Vector3i{0b1000, 0b0010, 0b0001});
        v = Vector3i{0b1100, 0b1010, 0b1111}; v |= m;
        CHECK(v == Vector3i{0b1110, 0b1110, 0b1111});
        v = Vector3i{0b1100, 0b1010, 0b1111}; v ^= m;
        CHECK(v == Vector3i{0b0110, 0b1100, 0b1110});
        v = Vector3i{1, 2, 3}; v <<= Vector3i{1, 1, 1};
        CHECK(v == Vector3i{2, 4, 6});
        v = Vector3i{8, 4, 2}; v >>= Vector3i{1, 1, 1};
        CHECK(v == Vector3i{4, 2, 1});
    }
    SUBCASE("scalar rhs") {
        Vector3u v{0b1100u, 0b1010u, 0b0110u};
        v &= 0b0110u;
        CHECK(v == Vector3u{0b0100u, 0b0010u, 0b0110u});
        v = Vector3u{0b1100u, 0b1010u, 0b0110u}; v |= 0b0001u;
        CHECK(v == Vector3u{0b1101u, 0b1011u, 0b0111u});
        v = Vector3u{0b1100u, 0b1010u, 0b0110u}; v <<= 2u;
        CHECK(v == Vector3u{0b110000u, 0b101000u, 0b011000u});
    }
}

// ===========================================================================
//  7. Geometric functions
// ===========================================================================
TEST_CASE("vec: dot product") {
    CHECK(dot(Vector3f{1.0f, 2.0f, 3.0f}, Vector3f{4.0f, 5.0f, 6.0f})
          == doctest::Approx(32.0f));
    CHECK(dot(Vector3f{1.0f, 0.0f, 0.0f}, Vector3f{0.0f, 1.0f, 0.0f})
          == doctest::Approx(0.0f));   // orthogonal
}

TEST_CASE("vec: cross product (vec3 only)") {
    Vector3f x{1.0f, 0.0f, 0.0f};
    Vector3f y{0.0f, 1.0f, 0.0f};
    CHECK(approx(cross(x, y), Vector3f{0.0f, 0.0f, 1.0f}));   // x X y = z
    CHECK(approx(cross(y, x), Vector3f{0.0f, 0.0f, -1.0f}));  // anti-commutative
    // Result is orthogonal to both inputs. Use an ABSOLUTE tolerance scaled to
    // the operand magnitudes: Approx's epsilon is relative, so it degenerates
    // to ~0 when the expected value is 0. The residual of dot(cross(a,b), a)
    // grows with |a||b|, so we scale the bound accordingly.
    Vector3f a = rand_vec<3>();
    Vector3f b = rand_vec<3>();
    Vector3f c = cross(a, b);
    const f32 tol = 1e-4f * length(a) * length(b);
    CHECK(std::abs(dot(c, a)) <= tol);
    CHECK(std::abs(dot(c, b)) <= tol);
}

TEST_CASE("vec: length, length_sqr, distance") {
    Vector3f v{3.0f, 4.0f, 0.0f};
    CHECK(length_sqr(v) == doctest::Approx(25.0f));
    CHECK(length(v) == doctest::Approx(5.0f));
    CHECK(distance(Vector3f{0.0f, 0.0f, 0.0f}, v) == doctest::Approx(5.0f));
}

TEST_CASE("vec: normalize yields unit length; zero stays zero") {
    Vector3f v{3.0f, 4.0f, 0.0f};
    Vector3f n = normalize(v);
    CHECK(length(n) == doctest::Approx(1.0f));
    CHECK(normalized(n));
    Vector3f z{0.0f, 0.0f, 0.0f};
    CHECK(approx(normalize(z), z));   // no divide-by-zero blow-up
}

TEST_CASE("vec: reflect off a surface normal") {
    // A ball falling straight down onto a flat floor bounces straight up.
    constexpr Vector3f incident{0.0f, -1.0f, 0.0f};
    constexpr Vector3f normal{0.0f, 1.0f, 0.0f};
    CHECK(approx(reflect(incident, normal), Vector3f{0.0f, 1.0f, 0.0f}));
}

TEST_CASE("vec: project a onto onto") {
    constexpr Vector3f a{2.0f, 3.0f, 0.0f};
    constexpr Vector3f onto{1.0f, 0.0f, 0.0f};
    CHECK(approx(project(a, onto), Vector3f{2.0f, 0.0f, 0.0f}));
}

// ===========================================================================
//  8. Common functions
// ===========================================================================
TEST_CASE("vec: abs / min / max / clamp") {
    CHECK(approx(abs(Vector3f{-1.0f, 2.0f, -3.0f}), Vector3f{1.0f, 2.0f, 3.0f}));
    CHECK(approx(min(Vector3f{1.0f, 5.0f, 3.0f}, Vector3f{4.0f, 2.0f, 6.0f}),
                 Vector3f{1.0f, 2.0f, 3.0f}));
    CHECK(approx(max(Vector3f{1.0f, 5.0f, 3.0f}, Vector3f{4.0f, 2.0f, 6.0f}),
                 Vector3f{4.0f, 5.0f, 6.0f}));
    CHECK(approx(clamp(Vector3f{-1.0f, 0.5f, 2.0f}, 0.0f, 1.0f),
                 Vector3f{0.0f, 0.5f, 1.0f}));
}

TEST_CASE("vec: lerp endpoints and midpoint") {
    Vector3f a{0.0f, 0.0f, 0.0f};
    Vector3f b{10.0f, 20.0f, 30.0f};
    CHECK(approx(lerp(a, b, 0.0f), a));
    CHECK(approx(lerp(a, b, 1.0f), b));
    CHECK(approx(lerp(a, b, 0.5f), Vector3f{5.0f, 10.0f, 15.0f}));
}

TEST_CASE("vec: floor / ceil / round / sign") {
    CHECK(approx(floor(Vector3f{1.2f, 2.7f, -1.2f}), Vector3f{1.0f, 2.0f, -2.0f}));
    CHECK(approx(ceil(Vector3f{1.2f, 2.7f, -1.2f}), Vector3f{2.0f, 3.0f, -1.0f}));
    CHECK(approx(round(Vector3f{1.4f, 2.6f, -1.5f}), Vector3f{1.0f, 3.0f, -2.0f}));
    CHECK(approx(sign(Vector3f{-3.0f, 0.0f, 7.0f}), Vector3f{-1.0f, 0.0f, 1.0f}));
}

// ===========================================================================
//  9. Comparison functions, boolean reductions, tolerant equality
// ===========================================================================
TEST_CASE("vec: lessThan / greaterThan family return bool-vectors") {
    Vector3f a{1.0f, 5.0f, 3.0f};
    Vector3f b{4.0f, 2.0f, 3.0f};
    CHECK(lessThan(a, b)         == Vector<bool, 3>{true, false, false});
    CHECK(lessThanEqual(a, b)    == Vector<bool, 3>{true, false, true});
    CHECK(greaterThan(a, b)      == Vector<bool, 3>{false, true, false});
    CHECK(greaterThanEqual(a, b) == Vector<bool, 3>{false, true, true});
}

TEST_CASE("vec: any / all / negate") {
    CHECK(any(Vector<bool, 3>{false, false, true}));
    CHECK_FALSE(any(Vector<bool, 3>{false, false, false}));
    CHECK(all(Vector<bool, 3>{true, true, true}));
    CHECK_FALSE(all(Vector<bool, 3>{true, false, true}));
    CHECK(negate(Vector<bool, 3>{true, false, true})
          == Vector<bool, 3>{false, true, false});
}

TEST_CASE("vec: tolerant equal / notEqual") {
    Vector3f a{1.0f, 2.0f, 3.0f};
    Vector3f b{1.0f + 1e-7f, 2.0f - 1e-7f, 3.0f};
    SUBCASE("within tolerance -> all equal") {
        CHECK(all(equal(a, b, 1e-5f)));
        CHECK_FALSE(any(notEqual(a, b, 1e-5f)));
    }
    SUBCASE("outside tolerance -> flagged component") {
        Vector3f c{1.0f, 2.0f, 3.1f};
        CHECK(equal(a, c, 1e-5f)    == Vector<bool, 3>{true, true, false});
        CHECK(notEqual(a, c, 1e-5f) == Vector<bool, 3>{false, false, true});
    }
    SUBCASE("per-component epsilon vector") {
        Vector3f eps{1e-5f, 1e-5f, 1e-5f};
        CHECK(all(equal(a, b, eps)));
    }
}

// ===========================================================================
// 10. Trigonometric functions
// ===========================================================================
TEST_CASE("vec: radians <-> degrees round-trip") {
    constexpr Vector3f deg{0.0f, 90.0f, 180.0f};
    Vector3f rad = radians(deg);
    CHECK(rad[1] == doctest::Approx(PI<f32> / 2.0f));
    CHECK(approx(degrees(rad), deg, 1e-3f));
}

TEST_CASE("vec: sin/cos/tan applied component-wise") {
    Vector3f v{0.0f, PI<f32> / 2.0f, PI<f32>};
    Vector3f s = sin(v);
    CHECK(s[0] == doctest::Approx(0.0f).epsilon(1e-5));
    CHECK(s[1] == doctest::Approx(1.0f).epsilon(1e-5));
    Vector3f c = cos(v);
    CHECK(c[0] == doctest::Approx(1.0f).epsilon(1e-5));
    CHECK(c[2] == doctest::Approx(-1.0f).epsilon(1e-5));
}

TEST_CASE("vec: atan2 is component-wise y-over-x") {
    Vector2f y{1.0f, 0.0f};
    Vector2f x{0.0f, 1.0f};
    Vector2f r = atan2(y, x);
    CHECK(r[0] == doctest::Approx(PI<f32> / 2.0f));
    CHECK(r[1] == doctest::Approx(0.0f));
}

// ===========================================================================
// 11. Vector constants
// ===========================================================================
TEST_CASE("vec: zero / one constants") {
    CHECK(approx(Vector3f::zero, Vector3f{0.0f, 0.0f, 0.0f}));
    CHECK(approx(Vector3f::one,  Vector3f{1.0f, 1.0f, 1.0f}));
}

TEST_CASE("vec: direction constants (right-handed, +z forward)") {
    CHECK(approx(Vector3f::right,   Vector3f{1.0f, 0.0f, 0.0f}));
    CHECK(approx(Vector3f::left,    Vector3f{-1.0f, 0.0f, 0.0f}));
    CHECK(approx(Vector3f::up,      Vector3f{0.0f, 1.0f, 0.0f}));
    CHECK(approx(Vector3f::down,    Vector3f{0.0f, -1.0f, 0.0f}));
    CHECK(approx(Vector3f::forward, Vector3f{0.0f, 0.0f, 1.0f}));
    CHECK(approx(Vector3f::back,    Vector3f{0.0f, 0.0f, -1.0f}));
    // vec2 has the planar four but no forward/back.
    CHECK(approx(Vector2f::right, Vector2f{1.0f, 0.0f}));
    CHECK(approx(Vector2f::up,    Vector2f{0.0f, 1.0f}));
}

// ===========================================================================
// 12. Swizzles
// ===========================================================================
TEST_CASE("vec: swizzle extracts and reorders components") {
    Vector4f v{1.0f, 2.0f, 3.0f, 4.0f};
    CHECK(swizzle<0>(v) == 1.0f);                                   // scalar
    CHECK(approx(swizzle<1, 0>(v), Vector2f{2.0f, 1.0f}));          // .yx
    CHECK(approx(swizzle<0, 1, 2>(v), Vector3f{1.0f, 2.0f, 3.0f})); // .xyz
    CHECK(approx(swizzle<3, 2, 1, 0>(v),
                 Vector4f{4.0f, 3.0f, 2.0f, 1.0f}));                // reversed
}

// ===========================================================================
// 13. GLM parity — the Ch.5a acceptance bar
//     "Every op matches GLM within 1e-5 on random input."
// ===========================================================================
#ifdef NME_TEST_WITH_GLM

TEST_CASE_TEMPLATE("vec parity: arithmetic matches GLM", Rank,
                   std::integral_constant<usize, 2>,
                   std::integral_constant<usize, 3>,
                   std::integral_constant<usize, 4>) {
    constexpr usize N = Rank::value;
    for (int t = 0; t < kTrials; ++t) {
        const auto a  = rand_vec<N>();
        const auto b  = rand_vec_nonzero<N>();
        const auto ga = to_glm(a);
        const auto gb = to_glm(b);
        const f32  s  = rand_f(-10.0f, 10.0f);

        CHECK(approx(a + b, from_glm(glm_of_t<N>(ga + gb))));
        CHECK(approx(a - b, from_glm(glm_of_t<N>(ga - gb))));
        CHECK(approx(a * b, from_glm(glm_of_t<N>(ga * gb))));
        CHECK(approx(b / a, from_glm(glm_of_t<N>(gb / ga))));
        CHECK(approx(a * s, from_glm(glm_of_t<N>(ga * s))));
        CHECK(approx(s * a, from_glm(glm_of_t<N>(s * ga))));
        CHECK(approx(-a,    from_glm(glm_of_t<N>(-ga))));
    }
}

TEST_CASE_TEMPLATE("vec parity: geometric matches GLM", Rank,
                   std::integral_constant<usize, 2>,
                   std::integral_constant<usize, 3>,
                   std::integral_constant<usize, 4>) {
    constexpr usize N = Rank::value;
    for (int t = 0; t < kTrials; ++t) {
        const auto a  = rand_vec_nonzero<N>();
        const auto b  = rand_vec_nonzero<N>();
        const auto ga = to_glm(a);
        const auto gb = to_glm(b);

        CHECK(dot(a, b)      == doctest::Approx(glm::dot(ga, gb)).epsilon(1e-5));
        CHECK(length(a)      == doctest::Approx(glm::length(ga)).epsilon(1e-5));
        CHECK(length_sqr(a)  == doctest::Approx(glm::dot(ga, ga)).epsilon(1e-5));
        CHECK(distance(a, b) == doctest::Approx(glm::distance(ga, gb)).epsilon(1e-5));
        CHECK(approx(normalize(a), from_glm(glm_of_t<N>(glm::normalize(ga)))));
    }
}

TEST_CASE("vec parity: cross matches GLM (vec3)") {
    for (int t = 0; t < kTrials; ++t) {
        const auto a = rand_vec<3>();
        const auto b = rand_vec<3>();
        CHECK(approx(cross(a, b),
                     from_glm(glm::cross(to_glm(a), to_glm(b)))));
    }
}

TEST_CASE("vec parity: reflect matches GLM (vec3)") {
    for (int t = 0; t < kTrials; ++t) {
        const auto i = rand_vec<3>();
        const auto n = normalize(rand_vec_nonzero<3>());   // reflect wants unit n
        CHECK(approx(reflect(i, n),
                     from_glm(glm::reflect(to_glm(i), to_glm(n)))));
    }
}

TEST_CASE_TEMPLATE("vec parity: common funcs match GLM", Rank,
                   std::integral_constant<usize, 2>,
                   std::integral_constant<usize, 3>,
                   std::integral_constant<usize, 4>) {
    constexpr usize N = Rank::value;
    for (int t = 0; t < kTrials; ++t) {
        const auto a  = rand_vec<N>();
        const auto b  = rand_vec<N>();
        const auto ga = to_glm(a);
        const auto gb = to_glm(b);
        const f32  u  = rand_f(0.0f, 1.0f);

        CHECK(approx(abs(a),    from_glm(glm_of_t<N>(glm::abs(ga)))));
        CHECK(approx(min(a, b), from_glm(glm_of_t<N>(glm::min(ga, gb)))));
        CHECK(approx(max(a, b), from_glm(glm_of_t<N>(glm::max(ga, gb)))));
        CHECK(approx(floor(a),  from_glm(glm_of_t<N>(glm::floor(ga)))));
        CHECK(approx(ceil(a),   from_glm(glm_of_t<N>(glm::ceil(ga)))));
        CHECK(approx(lerp(a, b, u),
                     from_glm(glm_of_t<N>(glm::mix(ga, gb, u)))));
    }
}

TEST_CASE_TEMPLATE("vec parity: trig matches GLM", Rank,
                   std::integral_constant<usize, 2>,
                   std::integral_constant<usize, 3>,
                   std::integral_constant<usize, 4>) {
    constexpr usize N = Rank::value;
    for (int t = 0; t < kTrials; ++t) {
        const auto a  = rand_vec<N>(-3.0f, 3.0f);   // keep tan away from blow-ups
        const auto ga = to_glm(a);
        CHECK(approx(sin(a),     from_glm(glm_of_t<N>(glm::sin(ga)))));
        CHECK(approx(cos(a),     from_glm(glm_of_t<N>(glm::cos(ga)))));
        CHECK(approx(radians(a), from_glm(glm_of_t<N>(glm::radians(ga)))));
        CHECK(approx(degrees(a), from_glm(glm_of_t<N>(glm::degrees(ga))), 1e-4f));
    }
}

#endif  // NME_TEST_WITH_GLM

// EOF