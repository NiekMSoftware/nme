#include <doctest/doctest.h>

#include <random>
#include <type_traits>

#include <nme/core/math/vec.h>

#ifdef NME_TEST_WITH_GLM
#include <glm/glm.hpp>
#endif

using namespace nme;
using namespace nme::math;

TEST_CASE("union views all alias the same bytes") {
    SUBCASE("vec2") {
        Vector2f v{{1.0f, 2.0f}};
        CHECK(v.x == 1.0f);
        CHECK(v.y == 2.0f);
        CHECK(v.data[0] == 1.0f);
        CHECK(v.data[1] == 2.0f);
        v.data[0] = 9.0f;              // write through the array...
        CHECK(v.x == 9.0f);           // ...read it back through .x
    }
    SUBCASE("vec3: xyz / rgb / xy / data overlap") {
        Vector3f v{{1.0f, 2.0f, 3.0f}};
        CHECK(v.x == 1.0f); CHECK(v.r == 1.0f); CHECK(v.data[0] == 1.0f);
        CHECK(v.y == 2.0f); CHECK(v.g == 2.0f);
        CHECK(v.z == 3.0f); CHECK(v.b == 3.0f);
        CHECK(v.xy.x == 1.0f);
        CHECK(v.xy.y == 2.0f);
        v.g = 5.0f;
        CHECK(v.y == 5.0f);
        CHECK(v.data[1] == 5.0f);
    }
    SUBCASE("vec4: xyzw / rgba / xyz / rgb / xy / data overlap") {
        Vector4f v{{1.0f, 2.0f, 3.0f, 4.0f}};
        CHECK(v.w == 4.0f); CHECK(v.a == 4.0f); CHECK(v.data[3] == 4.0f);
        CHECK(v.xyz.z == 3.0f);
        CHECK(v.rgb.b == 3.0f);
        CHECK(v.xy.x == 1.0f);
        v.data[2] = 7.0f;
        CHECK(v.z == 7.0f);
        CHECK(v.xyz.z == 7.0f);
    }
}

// Every promised typedef must expose named components (these used to fall back
// to the array-only primary template and had no .x/.y/.z).
TEST_CASE("all typedefs expose named components") {
    Vector2d d2{{1.0, 2.0}};             CHECK(d2.y == 2.0);
    Vector3d d3{{1.0, 2.0, 3.0}};        CHECK(d3.z == 3.0);
    Vector4d d4{{1.0, 2.0, 3.0, 4.0}};   CHECK(d4.w == 4.0);
    Vector2i i2{{1, 2}};                 CHECK(i2.y == 2);
    Vector3i i3{{1, 2, 3}};              CHECK(i3.z == 3);
    Vector4i i4{{1, 2, 3, 4}};           CHECK(i4.w == 4);
}

TEST_CASE("arithmetic operators are exact on integers") {
    constexpr Vector3i a{{1, 2, 3}};
    constexpr Vector3i b{{10, 20, 30}};

    CHECK((a + b)  == Vector3i{{11, 22, 33}});
    CHECK((b - a)  == Vector3i{{9, 18, 27}});
    CHECK((-a)     == Vector3i{{-1, -2, -3}});
    CHECK((a * b)  == Vector3i{{10, 40, 90}});      // component-wise (Hadamard)
    CHECK((b / a)  == Vector3i{{10, 10, 10}});
    CHECK((a * 3)  == Vector3i{{3, 6, 9}});
    CHECK((3 * a)  == Vector3i{{3, 6, 9}});
    CHECK((b / 10) == Vector3i{{1, 2, 3}});

    Vector3i c = a;
    c += b; CHECK(c == Vector3i{{11, 22, 33}});
    c -= b; CHECK(c == a);
    c *= 2; CHECK(c == Vector3i{{2, 4, 6}});
    c /= 2; CHECK(c == a);

    CHECK(a == a);
    CHECK(a != b);
    CHECK_FALSE(a == b);
}

TEST_CASE("dot and cross") {
    CHECK(dot(Vector3f{{1, 2, 3}}, Vector3f{{4, 5, 6}}) == doctest::Approx(32.0f));
    CHECK(dot(Vector3f{{1, 0, 0}}, Vector3f{{0, 1, 0}}) == doctest::Approx(0.0f));

    // right-handed basis: x cross y == z
    Vector3f z = cross(Vector3f{{1, 0, 0}}, Vector3f{{0, 1, 0}});
    CHECK(z.x == doctest::Approx(0.0f));
    CHECK(z.y == doctest::Approx(0.0f));
    CHECK(z.z == doctest::Approx(1.0f));

    // anti-commutative: y cross x == -z
    CHECK(cross(Vector3f{{0, 1, 0}}, Vector3f{{1, 0, 0}}).z == doctest::Approx(-1.0f));

    // integer cross works too
    CHECK(cross(Vector3i{{1, 0, 0}}, Vector3i{{0, 1, 0}}) == Vector3i{{0, 0, 1}});
}

TEST_CASE("length, distance, normalize, reflect") {
    CHECK(length(Vector3f{{3, 4, 0}}) == doctest::Approx(5.0f));
    CHECK(length_squared(Vector3f{{3, 4, 0}}) == doctest::Approx(25.0f));
    CHECK(distance(Vector2f{{0, 0}}, Vector2f{{3, 4}}) == doctest::Approx(5.0f));

    Vector3f n = normalize(Vector3f{{0, 3, 0}});
    CHECK(n.x == doctest::Approx(0.0f));
    CHECK(n.y == doctest::Approx(1.0f));
    CHECK(n.z == doctest::Approx(0.0f));
    CHECK(length(n) == doctest::Approx(1.0f));

    // a downward ray off a floor (normal +y) flips only the y component
    Vector2f r = reflect(Vector2f{{1, -1}}, Vector2f{{0, 1}});
    CHECK(r.x == doctest::Approx(1.0f));
    CHECK(r.y == doctest::Approx(1.0f));
}

TEST_CASE("operators evaluate at compile time") {
    constexpr Vector3f a{{1, 2, 3}};
    constexpr Vector3f b{{4, 5, 6}};
    static_assert(dot(a, b) == 32.0f);
    static_assert(length_squared(a) == 14.0f);
    static_assert((a + b).data[2] == 9.0f);
    static_assert((a * 2.0f).data[1] == 4.0f);
    static_assert(cross(Vector3f{{1, 0, 0}}, Vector3f{{0, 1, 0}}).data[2] == 1.0f);
    static_assert(reflect(Vector2f{{1, -1}}, Vector2f{{0, 1}}).data[1] == 1.0f);
    static_assert(Vector3i{{1, 2, 3}} == Vector3i{{1, 2, 3}});
    CHECK(true);  // so doctest records the case as having run
}

TEST_CASE("generic fallback works for any (T, N)") {
    Vector<std::int16_t, 5> v{};
    for (u32 i = 0; i < 5; ++i) v.data[i] = static_cast<std::int16_t>(i + 1);
    CHECK(dot(v, v) == 1 + 4 + 9 + 16 + 25);
}

// length / normalize / etc. must be *rejected* for integer vectors (the SFINAE
// floating-point guard), not silently truncate through integer division.
namespace {
template <typename V, typename = void>
struct has_normalize : std::false_type {};
template <typename V>
struct has_normalize<V, std::void_t<decltype(normalize(std::declval<V>()))>>
    : std::true_type {};
}  // namespace
static_assert(has_normalize<Vector3f>::value,  "normalize should exist for f32 vectors");
static_assert(has_normalize<Vector3d>::value,  "normalize should exist for f64 vectors");
static_assert(!has_normalize<Vector3i>::value, "normalize should NOT exist for integer vectors");

#ifdef NME_TEST_WITH_GLM
namespace {

constexpr int   kTrials = 256;
constexpr float kEps    = 1e-5f;

doctest::Approx approx(float v) { return doctest::Approx(v).epsilon(kEps); }

glm::vec3 to_glm(const Vector3f& v) { return {v.x, v.y, v.z}; }
glm::vec4 to_glm(const Vector4f& v) { return {v.x, v.y, v.z, v.w}; }

void check_eq(const Vector3f& a, const glm::vec3& g) {
    CHECK(a.x == approx(g.x));
    CHECK(a.y == approx(g.y));
    CHECK(a.z == approx(g.z));
}
void check_eq(const Vector4f& a, const glm::vec4& g) {
    CHECK(a.x == approx(g.x));
    CHECK(a.y == approx(g.y));
    CHECK(a.z == approx(g.z));
    CHECK(a.w == approx(g.w));
}

Vector3f rand3(std::mt19937& rng) {
    std::uniform_real_distribution<float> d(-10.0f, 10.0f);
    return Vector3f{{d(rng), d(rng), d(rng)}};
}
Vector4f rand4(std::mt19937& rng) {
    std::uniform_real_distribution<float> d(-10.0f, 10.0f);
    return Vector4f{{d(rng), d(rng), d(rng), d(rng)}};
}
// Biased away from the origin so length/normalize stay well-conditioned.
Vector3f rand_nonzero3(std::mt19937& rng) {
    Vector3f v = rand3(rng);
    v.x += 20.0f;
    return v;
}

}  // namespace

TEST_CASE("GLM parity - vec3 arithmetic") {
    std::mt19937 rng(0xA11CE);
    std::uniform_real_distribution<float> ds(-10.0f, 10.0f);
    for (int t = 0; t < kTrials; ++t) {
        CAPTURE(t);
        Vector3f a = rand3(rng), b = rand3(rng);
        glm::vec3 ga = to_glm(a), gb = to_glm(b);
        float s = ds(rng);

        check_eq(a + b, ga + gb);
        check_eq(a - b, ga - gb);
        check_eq(-a,    -ga);
        check_eq(a * b, ga * gb);     // component-wise in both
        check_eq(a * s, ga * s);
        check_eq(s * a, s * ga);
    }
}

TEST_CASE("GLM parity - vec3 geometric") {
    std::mt19937 rng(0xB0B);
    for (int t = 0; t < kTrials; ++t) {
        CAPTURE(t);
        Vector3f a = rand3(rng), b = rand3(rng);
        glm::vec3 ga = to_glm(a), gb = to_glm(b);

        CHECK(dot(a, b)      == approx(glm::dot(ga, gb)));
        CHECK(length(a)      == approx(glm::length(ga)));
        CHECK(distance(a, b) == approx(glm::distance(ga, gb)));
        check_eq(cross(a, b), glm::cross(ga, gb));

        Vector3f u = rand_nonzero3(rng);
        check_eq(normalize(u), glm::normalize(to_glm(u)));

        Vector3f n = normalize(rand_nonzero3(rng));   // unit normal for reflect
        check_eq(reflect(a, n), glm::reflect(ga, to_glm(n)));
    }
}

TEST_CASE("GLM parity - vec4") {
    std::mt19937 rng(0xF00D);
    std::uniform_real_distribution<float> ds(-10.0f, 10.0f);
    for (int t = 0; t < kTrials; ++t) {
        CAPTURE(t);
        Vector4f a = rand4(rng), b = rand4(rng);
        glm::vec4 ga = to_glm(a), gb = to_glm(b);
        float s = ds(rng);

        check_eq(a + b, ga + gb);
        check_eq(a - b, ga - gb);
        check_eq(a * b, ga * gb);
        check_eq(a * s, ga * s);
        CHECK(dot(a, b) == approx(glm::dot(ga, gb)));
        CHECK(length(a) == approx(glm::length(ga)));
    }
}
#endif  // NME_TEST_WITH_GLM
