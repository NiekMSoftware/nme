// Unit tests for nme::math vectors.
// Covers storage/aliasing, constructors, all operators (incl. bitwise on
// integers), geometric ops, the free-function set, and GLM parity.
//
// Unit tests written by a LLM to fully cover everything.

#include <doctest/doctest.h>

#include <random>
#include <type_traits>

#include <nme/core/math/vec.hpp>

#ifdef NME_TEST_WITH_GLM
#include <glm/glm.hpp>
#endif

using namespace nme;
using namespace nme::math;

// ===========================================================================
// Storage / layout
// ===========================================================================

TEST_CASE("vec: union views all alias the same bytes") {
    Vector4f v(1, 2, 3, 4);
    CHECK(v.x == 1.0f); CHECK(v.r == 1.0f); CHECK(v.data[0] == 1.0f);
    CHECK(v.w == 4.0f); CHECK(v.a == 4.0f); CHECK(v.data[3] == 4.0f);
    CHECK(v.xy.x == 1.0f);
    CHECK(v.xyz.z == 3.0f);
    CHECK(v.rgb.b == 3.0f);
    v.data[2] = 7.0f;
    CHECK(v.z == 7.0f);
    CHECK(v.xyz.z == 7.0f);
}

// ===========================================================================
// Constructors
// ===========================================================================

TEST_CASE("vec: constructors") {
    SUBCASE("per-component") {
        Vector3f v(1, 2, 3);
        CHECK(v.data[0] == 1.0f); CHECK(v.data[1] == 2.0f); CHECK(v.data[2] == 3.0f);
    }
    SUBCASE("splat") {
        Vector4f v(5.0f);
        CHECK(v.x == 5.0f); CHECK(v.y == 5.0f); CHECK(v.z == 5.0f); CHECK(v.w == 5.0f);
    }
    SUBCASE("composition (smaller + scalars)") {
        CHECK(Vector3f(Vector2f(1, 2), 3)              == Vector3f(1, 2, 3));
        CHECK(Vector4f(Vector3f(1, 2, 3), 4)           == Vector4f(1, 2, 3, 4));
        CHECK(Vector4f(Vector2f(1, 2), 3, 4)           == Vector4f(1, 2, 3, 4));
        CHECK(Vector4f(Vector2f(1, 2), Vector2f(3, 4)) == Vector4f(1, 2, 3, 4));
    }
    SUBCASE("truncation via sub-vector views") {
        Vector4f v(1, 2, 3, 4);
        CHECK(Vector3f(v.xyz) == Vector3f(1, 2, 3));
        CHECK(Vector2f(v.xy)  == Vector2f(1, 2));
    }
    SUBCASE("cross-type conversion (explicit)") {
        Vector3i vi(1, 2, 3);
        Vector3f vf(vi);
        CHECK(vf == Vector3f(1, 2, 3));
        Vector3i back(Vector3f(1.9f, 2.1f, 3.5f));   // truncates toward zero
        CHECK(back == Vector3i(1, 2, 3));
    }
    SUBCASE("value-init zeroes") {
        Vector3f z{};
        CHECK(z.x == 0.0f); CHECK(z.y == 0.0f); CHECK(z.z == 0.0f);
    }
}

TEST_CASE("vec: all typedefs expose named components") {
    CHECK(Vector2d(1, 2).y == 2.0);
    CHECK(Vector3d(1, 2, 3).z == 3.0);
    CHECK(Vector4d(1, 2, 3, 4).w == 4.0);
    CHECK(Vector2i(1, 2).y == 2);
    CHECK(Vector3i(1, 2, 3).z == 3);
    CHECK(Vector4i(1, 2, 3, 4).w == 4);
}

// ===========================================================================
// Arithmetic (exact, integers)
// ===========================================================================

TEST_CASE("vec: arithmetic operators are exact on integers") {
    constexpr Vector3i a(1, 2, 3);
    constexpr Vector3i b(10, 20, 30);

    CHECK((a + b) == Vector3i(11, 22, 33));
    CHECK((b - a) == Vector3i(9, 18, 27));
    CHECK((-a)    == Vector3i(-1, -2, -3));
    CHECK((a * b) == Vector3i(10, 40, 90));
    CHECK((b / a) == Vector3i(10, 10, 10));
    CHECK((a * 3) == Vector3i(3, 6, 9));
    CHECK((3 * a) == Vector3i(3, 6, 9));

    Vector3i c = a;
    c += b; CHECK(c == Vector3i(11, 22, 33));
    c -= b; CHECK(c == a);
    c *= 2; CHECK(c == Vector3i(2, 4, 6));

    CHECK(a == a);
    CHECK(a != b);
}

// ===========================================================================
// Bitwise (integers only, component-wise)
// ===========================================================================

TEST_CASE("vec: bitwise operators on integer vectors") {
    constexpr Vector3i a(0b1100, 0b1010, 0xFF);
    constexpr Vector3i b(0b1010, 0b0110, 0x0F);

    CHECK((a & b)  == Vector3i(0b1000, 0b0010, 0x0F));
    CHECK((a | b)  == Vector3i(0b1110, 0b1110, 0xFF));
    CHECK((a ^ b)  == Vector3i(0b0110, 0b1100, 0xF0));
    CHECK((a << 1) == Vector3i(0b11000, 0b10100, 0x1FE));
    CHECK((a >> 2) == Vector3i(0b11, 0b10, 0x3F));
    CHECK((~Vector3i(0, -1, 5)) == Vector3i(~0, ~(-1), ~5));

    Vector3i c = a;
    c &= b;  CHECK(c == (a & b));
    c = a; c |= 0x1;  CHECK(c == (a | 0x1));
    c = a; c <<= 1;   CHECK(c == (a << 1));
}

// Bitwise must be rejected for float vectors and bool masks (SFINAE).
namespace {
template <typename V, typename = void> struct has_bitand : std::false_type {};
template <typename V>
struct has_bitand<V, std::void_t<decltype(std::declval<V>() & std::declval<V>())>>
    : std::true_type {};
}  // namespace
static_assert(has_bitand<Vector3i>::value,          "bitwise & should exist for integer vectors");
static_assert(!has_bitand<Vector3f>::value,         "bitwise & should NOT exist for float vectors");
static_assert(!has_bitand<Vector<bool, 3>>::value,  "bitwise & should NOT exist for bool masks");

// ===========================================================================
// Geometric
// ===========================================================================

TEST_CASE("vec: dot and cross") {
    CHECK(dot(Vector3f(1, 2, 3), Vector3f(4, 5, 6)) == doctest::Approx(32.0f));
    Vector3f z = cross(Vector3f(1, 0, 0), Vector3f(0, 1, 0));
    CHECK(z.z == doctest::Approx(1.0f));
    CHECK(cross(Vector3i(1, 0, 0), Vector3i(0, 1, 0)) == Vector3i(0, 0, 1));
}

TEST_CASE("vec: length, distance, normalize, reflect") {
    CHECK(length(Vector3f(3, 4, 0)) == doctest::Approx(5.0f));
    CHECK(distance(Vector2f(0, 0), Vector2f(3, 4)) == doctest::Approx(5.0f));
    CHECK(length(normalize(Vector3f(0, 3, 0))) == doctest::Approx(1.0f));
    Vector2f r = reflect(Vector2f(1, -1), Vector2f(0, 1));
    CHECK(r.x == doctest::Approx(1.0f));
    CHECK(r.y == doctest::Approx(1.0f));
}

// ===========================================================================
// Compile-time
// ===========================================================================

TEST_CASE("vec: operators evaluate at compile time") {
    constexpr Vector3f a(1, 2, 3);
    constexpr Vector3f b(4, 5, 6);
    static_assert(dot(a, b) == 32.0f);
    static_assert((a + b).data[2] == 9.0f);
    static_assert(cross(Vector3f(1, 0, 0), Vector3f(0, 1, 0)).data[2] == 1.0f);
    static_assert(reflect(Vector2f(1, -1), Vector2f(0, 1)).data[1] == 1.0f);
    static_assert((Vector3i(0b110, 0, 0) & Vector3i(0b010, 0, 0)).data[0] == 0b010);
    static_assert(Vector4f(2.0f).data[3] == 2.0f);            // splat, constexpr
    CHECK(true);
}

TEST_CASE("vec: generic fallback works for any (T, N)") {
    Vector<std::int16_t, 5> v{};
    for (u32 i = 0; i < 5; ++i) v.data[i] = static_cast<std::int16_t>(i + 1);
    CHECK(dot(v, v) == 1 + 4 + 9 + 16 + 25);
}

// normalize must be rejected for integer vectors.
namespace {
template <typename V, typename = void> struct has_normalize : std::false_type {};
template <typename V>
struct has_normalize<V, std::void_t<decltype(normalize(std::declval<V>()))>>
    : std::true_type {};
}  // namespace
static_assert(has_normalize<Vector3f>::value);
static_assert(!has_normalize<Vector3i>::value);

// ===========================================================================
// Free functions
// ===========================================================================

TEST_CASE("vec: free-function spot checks") {
    CHECK(min(Vector3f(1, 5, 3), Vector3f(2, 4, 3)) == Vector3f(1, 4, 3));
    CHECK(max(Vector3f(1, 5, 3), Vector3f(2, 4, 3)) == Vector3f(2, 5, 3));
    CHECK(clamp(Vector3f(-1, 0.5f, 2), 0.0f, 1.0f)  == Vector3f(0, 0.5f, 1));
    CHECK(abs(Vector3i(-1, 2, -3)) == Vector3i(1, 2, 3));
    CHECK(is_near(lerp(Vector2f(0, 0), Vector2f(10, 20), 0.5f), Vector2f(5, 10)));
    CHECK(min_component(Vector3f(3, 1, 2)) == doctest::Approx(1.0f));
    CHECK(component_sum(Vector4i(1, 2, 3, 4)) == 10);

    auto gt = greater_than(Vector3f(1, 9, 1), Vector3f(5, 5, 5));   // {F,T,F}
    CHECK(any(gt));
    CHECK_FALSE(all(gt));
    CHECK(select(gt, Vector3f(7, 7, 7), Vector3f(0, 0, 0)) == Vector3f(0, 7, 0));
}

// ===========================================================================
// GLM parity — the guide's 5a acceptance bar.
// ===========================================================================
#ifdef NME_TEST_WITH_GLM
namespace {

constexpr int   kTrials = 256;
constexpr float kEps    = 1e-5f;
doctest::Approx approx(float v) { return doctest::Approx(v).epsilon(kEps); }

glm::vec3 to_glm(const Vector3f& v) { return {v.x, v.y, v.z}; }
glm::vec4 to_glm(const Vector4f& v) { return {v.x, v.y, v.z, v.w}; }

void check_eq(const Vector3f& a, const glm::vec3& g) {
    CHECK(a.x == approx(g.x)); CHECK(a.y == approx(g.y)); CHECK(a.z == approx(g.z));
}
void check_eq(const Vector4f& a, const glm::vec4& g) {
    CHECK(a.x == approx(g.x)); CHECK(a.y == approx(g.y));
    CHECK(a.z == approx(g.z)); CHECK(a.w == approx(g.w));
}

Vector3f rand3(std::mt19937& rng) {
    std::uniform_real_distribution<float> d(-10.0f, 10.0f);
    return Vector3f(d(rng), d(rng), d(rng));
}
Vector3f rand_nonzero3(std::mt19937& rng) { Vector3f v = rand3(rng); v.x += 20.0f; return v; }

}  // namespace

TEST_CASE("vec: GLM parity - vec3 arithmetic + geometric") {
    std::mt19937 rng(0xA11CE);
    std::uniform_real_distribution<float> ds(-10.0f, 10.0f);
    for (int t = 0; t < kTrials; ++t) {
        CAPTURE(t);
        Vector3f a = rand3(rng), b = rand3(rng);
        glm::vec3 ga = to_glm(a), gb = to_glm(b);
        float s = ds(rng);

        check_eq(a + b, ga + gb);
        check_eq(a - b, ga - gb);
        check_eq(a * b, ga * gb);
        check_eq(a * s, ga * s);
        check_eq(min(a, b), glm::min(ga, gb));
        check_eq(max(a, b), glm::max(ga, gb));
        check_eq(abs(a),    glm::abs(ga));
        check_eq(floor(a),  glm::floor(ga));
        check_eq(lerp(a, b, 0.3f), glm::mix(ga, gb, 0.3f));

        CHECK(dot(a, b)      == approx(glm::dot(ga, gb)));
        CHECK(length(a)      == approx(glm::length(ga)));
        CHECK(distance(a, b) == approx(glm::distance(ga, gb)));
        check_eq(cross(a, b), glm::cross(ga, gb));

        Vector3f u = rand_nonzero3(rng);
        check_eq(normalize(u), glm::normalize(to_glm(u)));
        Vector3f n = normalize(rand_nonzero3(rng));
        check_eq(reflect(a, n), glm::reflect(ga, to_glm(n)));
    }
}

TEST_CASE("vec: GLM parity - vec4") {
    std::mt19937 rng(0xF00D);
    std::uniform_real_distribution<float> ds(-10.0f, 10.0f);
    for (int t = 0; t < kTrials; ++t) {
        CAPTURE(t);
        Vector4f a(ds(rng), ds(rng), ds(rng), ds(rng));
        Vector4f b(ds(rng), ds(rng), ds(rng), ds(rng));
        glm::vec4 ga = to_glm(a), gb = to_glm(b);
        check_eq(a + b, ga + gb);
        check_eq(a * b, ga * gb);
        CHECK(dot(a, b) == approx(glm::dot(ga, gb)));
        CHECK(length(a) == approx(glm::length(ga)));
    }
}

TEST_CASE("vec: GLM parity - integer bitwise (ivec3)") {
    std::mt19937 rng(0x5EED);
    std::uniform_int_distribution<int> di(-1000, 1000);
    std::uniform_int_distribution<int> sh(0, 5);
    auto eq = [](const Vector3i& v, const glm::ivec3& g) {
        return v.x == g.x && v.y == g.y && v.z == g.z;
    };
    for (int t = 0; t < kTrials; ++t) {
        CAPTURE(t);
        Vector3i a(di(rng), di(rng), di(rng));
        Vector3i b(di(rng), di(rng), di(rng));
        glm::ivec3 ga(a.x, a.y, a.z), gb(b.x, b.y, b.z);
        int s = sh(rng);
        CHECK(eq(a & b, ga & gb));
        CHECK(eq(a | b, ga | gb));
        CHECK(eq(a ^ b, ga ^ gb));
        CHECK(eq(a << s, ga << s));
        CHECK(eq(a >> s, ga >> s));
        CHECK(eq(~a, ~ga));
    }
}

#endif  // NME_TEST_WITH_GLM
