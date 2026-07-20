//
// Created by niek on 7/20/2026.
//

#ifndef NME_PLATFORM_ERROR_RESULT_H_
#define NME_PLATFORM_ERROR_RESULT_H_
#include <type_traits>

namespace nme {

template<typename T, typename E>
struct Result {
    union {
        T value;
        E error;
    };
    bool bOk;

    static_assert(std::is_trivially_copyable_v<T>, "Result<T, E>: T must be trivially copyable");
    static_assert(std::is_trivially_copyable_v<E>, "Result<T, E>: E must be trivially copyable");
};

// --- construction ---

template<typename T, typename E>
inline Result<T, E> result_ok(const T value) {
    Result<T, E> r{};
    r.value = value;
    r.bOk = true;
    return r;
}
template<typename T, typename E>
inline Result<T, E> result_err(const E error) {
    Result<T, E> r{};
    r.error = error;
    r.bOk = false;
    return r;
}

// --- queries ---

template<typename T, typename E>
inline bool result_is_ok(Result<T, E>* r)  { return r->bOk; }
template<typename T, typename E>
inline bool result_is_err(Result<T, E>* r) { return !r->bOk; }

// --- access ---

template<typename T, typename E>
inline T result_value(const Result<T, E>* r) {
    NME_ASSERT(r->bOk);
    return r->value;
}
template<typename T, typename E>
inline E result_error(const Result<T, E>* r) {
    NME_ASSERT(!r->bOk);
    return r->error;
}
template<typename T, typename E>
inline T result_value_or(const Result<T, E>* r, const T fallback) {
    return r->bOk ? r->value : fallback;    // short-circuits: never reads value when inactive
}

}  // namespace nme

#endif  // NME_PLATFORM_ERROR_RESULT_H_
