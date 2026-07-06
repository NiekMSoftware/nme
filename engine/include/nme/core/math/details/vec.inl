using namespace nme;
using namespace nme::math;

// =========================================================================
//                               Static Members
// =========================================================================

template<typename T, usize N>
const Vector<T, N> Vector<T, N>::zero { 0 };
template<typename T, usize N>
const Vector<T, N> Vector<T, N>::one { 1 };

template<typename T, usize N>
const Vector<T, N> Vector<T, N>::x { T(1), T(0) };
template<typename T, usize N>
const Vector<T, N> Vector<T, N>::y { T(0), T(1) };
template<typename T, usize N>
const Vector<T, N> Vector<T, N>::z { T(0), T(0), T(1) };
template<typename T, usize N>
const Vector<T, N> Vector<T, N>::w { T(0), T(0), T(0), T(1) };

template <typename T, usize N>
constexpr usize Vector<T, N>::size() noexcept { return N; }
template <typename T, usize N>
constexpr T* Vector<T, N>::data() noexcept { return &base::data[0]; }
template<typename T, usize N>
constexpr const T* Vector<T, N>::data() const noexcept { return &base::data[0]; }

// =========================================================================
//                              Constructors
// =========================================================================

template <typename T, usize N>
constexpr Vector<T, N>::Vector() noexcept = default;

template <typename T, usize N>
template <convertible_to<T> X>
constexpr Vector<T, N>::Vector(X s) noexcept {
    for (i32 i = 0; i < N; ++i)
        base::data[i] = static_cast<T>(s);
}

template <typename T, usize N>
template <convertible_to<T> X, convertible_to<T> Y>
constexpr Vector<T, N>::Vector(X x_, Y y_) noexcept {
    base::data[0] = static_cast<T>(x_);

    if constexpr (N > 1)
        base::data[1] = static_cast<T>(y_);
}

template <typename T, usize N>
template <convertible_to<T> X, convertible_to<T> Y, convertible_to<T> Z>
constexpr Vector<T, N>::Vector(X x_, Y y_, Z z_) noexcept {
    base::data[0] = static_cast<T>(x_);

    if constexpr (N > 1)
        base::data[1] = static_cast<T>(y_);
    if constexpr (N > 2)
        base::data[2] = static_cast<T>(z_);
}

template <typename T, usize N>
template <convertible_to<T> X, convertible_to<T> Y, convertible_to<T> Z, convertible_to<T> W>
constexpr Vector<T, N>::Vector(X x_, Y y_, Z z_, W w_) noexcept {
    base::data[0] = static_cast<T>(x_);

    if constexpr (N > 1)
        base::data[1] = static_cast<T>(y_);
    if constexpr (N > 2)
        base::data[2] = static_cast<T>(z_);
    if constexpr (N > 3)
        base::data[3] = static_cast<T>(w_);
}

template <typename T, usize N>
template <convertible_to<T> U, usize M, convertible_to<T>... Args>
constexpr Vector<T, N>::Vector(const Vector<U, M>& copy, const Args&... args) noexcept {
    constexpr usize c = std::min(N, M);

    i32 i = 0;
    for (; i < c; ++i)
        base::data[i] = static_cast<T>(copy.data[i]);

    // unfold any additional args
    (((i < N) ? base::data[i++] = static_cast<T>(args) : false), ...);
}

// EOF