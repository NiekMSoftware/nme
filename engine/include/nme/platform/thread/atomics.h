#pragma once

#include <atomic>
#include <type_traits>

namespace nme {

// The engine's memory-ordering vocabulary. 1:1 with std::memory_order, minus
// `consume`, it's discouraged and every real compiler strengthens it to
// acquire anyway, so exposing it would only invite misuse.
enum class MemoryOrder {
    Relaxed,   // no ordering, only atomicity (counters, stats)
    Acquire,   // this load sees prior releases (the consumer half)
    Release,   // publishes prior writes        (the producer half)
    AcqRel,    // both, for read-modify-write   (CAS on a shared node)
    SeqCst,    // single global order           (the safe, slow default)
};

namespace detail {
constexpr std::memory_order to_std(const MemoryOrder o) noexcept {
    switch (o) {
        case MemoryOrder::Relaxed: return std::memory_order_relaxed;
        case MemoryOrder::Acquire: return std::memory_order_acquire;
        case MemoryOrder::Release: return std::memory_order_release;
        case MemoryOrder::AcqRel:  return std::memory_order_acq_rel;
        case MemoryOrder::SeqCst:  return std::memory_order_seq_cst;
    }
    return std::memory_order_seq_cst;
}

constexpr std::memory_order fail_order(const MemoryOrder o) {
    switch (o) {
        case MemoryOrder::Release: return std::memory_order_relaxed;
        case MemoryOrder::AcqRel: return std::memory_order_acquire;
        default: to_std(o);
    }
    return to_std(o);
}

}  // namespace detail

template<typename T>
class Atomic {
    static_assert(std::is_trivially_copyable_v<T>,
        "Atomic<T> requires trivially copyable T");

private:
    std::atomic<T> m_value;

public:
    Atomic() noexcept : m_value(T{}) {}     // explicit zero-init
    constexpr explicit Atomic(T desired) noexcept : m_value(desired) {}

    Atomic(const Atomic& other) = delete;
    Atomic(Atomic&& other) noexcept = delete;
    Atomic& operator=(const Atomic& other) = delete;
    Atomic& operator=(Atomic&& other) noexcept = delete;

    // Every op names its order. Defaults are the *safe* choice (SeqCst).
    T load(const MemoryOrder o = MemoryOrder::SeqCst) const noexcept
    {
        return m_value.load(detail::to_std(o));
    }
    void store(T v, const MemoryOrder o = MemoryOrder::SeqCst) noexcept
    {
        m_value.store(v, detail::to_std(o));
    }
    T exchange(T v, const MemoryOrder o = MemoryOrder::SeqCst) noexcept
    {
        return m_value.exchange(v, detail::to_std(o));
    }

    // CAS: `expected` is updated to the actual value on failure (std semantics).
    // Weak may fail spuriously -> use in a loop; Strong for one-shot checks.
    bool compareExchangeWeak(T& expected, T desired,
        const MemoryOrder o = MemoryOrder::SeqCst) noexcept
    {
        return m_value.compare_exchange_weak(expected, desired,
            detail::to_std(o), detail::fail_order(o));
    }
    bool compareExchangeStrong(T& expected, T desired,
        const MemoryOrder o = MemoryOrder::SeqCst) noexcept
    {
        return m_value.compare_exchange_strong(expected, desired,
            detail::to_std(o), detail::fail_order(o));
    }

    // Arithmetic/bitwise
    T fetchAdd(T v, const MemoryOrder o = MemoryOrder::SeqCst) noexcept { return m_value.fetch_add(v, detail::to_std(o)); }
    T fetchSub(T v, const MemoryOrder o = MemoryOrder::SeqCst) noexcept { return m_value.fetch_sub(v, detail::to_std(o)); }
    T fetchAnd(T v, const MemoryOrder o = MemoryOrder::SeqCst) noexcept { return m_value.fetch_and(v, detail::to_std(o)); }
    T fetchOr (T v, const MemoryOrder o = MemoryOrder::SeqCst) noexcept { return m_value.fetch_or(v, detail::to_std(o)); }
    T fetchXor(T v, const MemoryOrder o = MemoryOrder::SeqCst) noexcept { return m_value.fetch_xor(v, detail::to_std(o)); }

    [[nodiscard]] bool isLockFree() const noexcept { return m_value.is_lock_free(); }
    static constexpr bool kAlwaysLockFree = std::atomic<T>::is_always_lock_free;
};

inline void atomicThreadFence(const MemoryOrder o) noexcept { std::atomic_thread_fence(detail::to_std(o)); }
inline void atomicSignalFence(const MemoryOrder o) noexcept { std::atomic_signal_fence(detail::to_std(o)); }

}  // namespace nme
