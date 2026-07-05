#ifndef NME_WS_DEQUE_H_
#define NME_WS_DEQUE_H_

#include <type_traits>

#include "nme/platform/platform.h"
#include "nme/platform/thread/atomics.h"
#include "nme/platform/types.h"

namespace nme {

/**
 * @brief Chase-lev work-stealing deque.
 *
 * Fully lock-free concurrent deque, owner and thieves touch opposite ends.
 * Any number of thieves steal from the top, only the last-remaining element
 * makes pop and steal race, and that race is resolved by a CAS on top.
 */
template<typename T, usize Capacity>
class WSDeque {
private:
    static_assert(std::is_trivially_copyable_v<T>,
        "WSDeque<T> moves T by trivial copy; put non-trivial payloads "
        "behind a pointer/handle");
    static_assert(Capacity >= 2, "capacity must be at least 2");
    static_assert((Capacity & (Capacity - 1)) == 0, "capacity must be a power of 2");

    static constexpr usize kMask = Capacity - 1;
    static constexpr usize kCacheLine = NME_CACHE_LINE_SIZE;

    alignas(kCacheLine) Atomic<usize> m_bottom{0};
    alignas(kCacheLine) Atomic<usize> m_top{0};
    alignas(kCacheLine) T             m_slots[Capacity];

public:
    WSDeque() = default;
    ~WSDeque() = default;

    WSDeque(const WSDeque&)            = delete;
    WSDeque(WSDeque&&)                 = delete;
    WSDeque& operator=(const WSDeque&) = delete;
    WSDeque& operator=(WSDeque&&)      = delete;

    bool push(const T& item) noexcept {
        const usize b = m_bottom.load(MemoryOrder::Relaxed);
        const usize t = m_top.load(MemoryOrder::Acquire);
        if (b - t >= Capacity) {
            return false;   // full, caller's granularity is wrong; never silently grow
        }
        m_slots[b & kMask] = item;
        m_bottom.store(b + 1, MemoryOrder::Release);
        return true;
    }

    bool pop(T& out) noexcept {
        const usize bPrev = m_bottom.load(MemoryOrder::Relaxed);
        const usize tPrev = m_top.load(MemoryOrder::Relaxed);
        if (bPrev == tPrev) {
            return false;   // definitely empty
        }

        const usize b = bPrev - 1;
        m_bottom.store(b, MemoryOrder::Relaxed);

        // order the bottom-store above against the top-load below.
        atomicThreadFence(MemoryOrder::SeqCst);

        usize t = m_top.load(MemoryOrder::Relaxed);
        if (t > b) {                                             // became empty (thief took it)
            m_bottom.store(b + 1, MemoryOrder::Relaxed);    // restore
            return false;
        }

        out = m_slots[b & kMask];   // provisional read
        if (t == b) {
            // last element: a thief may be trying to take it too. Whoever CASes
            // top first wins it.
            if (!m_top.compareExchangeStrong(t, t + 1, MemoryOrder::SeqCst)) {
                m_bottom.store(b + 1, MemoryOrder::Relaxed);
                return false;
            }
            m_bottom.store(b + 1, MemoryOrder::Relaxed);
            return true;
        }
        // t < b: more than one element, uncontended it's ours.
        return true;
    }

    enum class StealResult { Success, Empty, Abort };

    StealResult steal(T& out) noexcept {
        usize t = m_top.load(MemoryOrder::Acquire);
        atomicThreadFence(MemoryOrder::SeqCst);
        const usize b = m_bottom.load(MemoryOrder::Acquire);

        if (t >= b) {
            return StealResult::Empty;  // genuinely empty
        }

        out = m_slots[t & kMask];
        // claim slot t. if someone advanced top (another thief, or owner's pop of the last element),
        // our read is void -> Abort
        if (!m_top.compareExchangeStrong(t, t + 1, MemoryOrder::SeqCst)) {
            return StealResult::Abort;
        }
        return StealResult::Success;
    }

    [[nodiscard]] usize sizeApprox() const noexcept {
        const usize b = m_bottom.load(MemoryOrder::Relaxed);
        const usize t = m_top.load(MemoryOrder::Relaxed);
        return (b > t) ? (b - t) : 0;
    }

    [[nodiscard]] usize emptyApprox() const noexcept { return sizeApprox() == 0; }
    [[nodiscard]] static constexpr usize capacity() noexcept { return Capacity; }
};

}  // namespace nme

#endif  // NME_WS_DEQUE_H_
