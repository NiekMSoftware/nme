#pragma once

#include <type_traits>
#include <utility>

#include "nme/platform/types.h"

namespace nme
{
/**
 * @brief Runs a cleanup action when it leaves scope, unless dismissed.
 *
 * @code
 * auto* h = pool.acquire();
 * ScopeGuard rollback { [&] { pool.release(h); } };  // undo an early return
 * // ...
 * rollback.dismiss();                                // commit: keep
 * @endcode
 */
template<class Fn>
class [[nodiscard]] ScopeGuard {
private:
    b32 m_active = true;
    Fn m_fn;

public:
    explicit ScopeGuard(Fn fn) noexcept(std::is_nothrow_move_constructible_v<Fn>)
	    : m_fn(std::move(fn)) { }

    ~ScopeGuard() { if (m_active) m_fn(); }

    // Move transfer
    ScopeGuard(ScopeGuard &&other) noexcept(std::is_nothrow_move_constructible_v<Fn>)
	    : m_active(other.m_active), m_fn(std::move(other.m_fn)) {
	    other.m_active = false;
    }

    // Delete copy, as it would run twice
    ScopeGuard(const ScopeGuard&)            = delete;
    ScopeGuard& operator=(const ScopeGuard&) = delete;
    ScopeGuard& operator=(ScopeGuard&&)      = delete;

    void dismiss() noexcept { m_active = false; }
};

}  // namespace nme

// Go-style scope-exit: NME_DEFER( release(handle); );
#define NME_CONCAT_IMPL(a, b) a##b
#define NME_CONCAT(a, b)      NME_CONCAT_IMPL(a, b)
#define NME_DEFER(...) \
    ::nme::ScopeGuard NME_CONCAT(nme_defer_, __LINE__) { [&]() { __VA_ARGS__; } }