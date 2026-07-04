#ifndef NME_SPSC_RING_H_
#define NME_SPSC_RING_H_

#include <type_traits>

#include "nme/platform/platform.h"          // NME_CACHE_LINE
#include "nme/platform/types.h"             // usize
#include "nme/platform/thread/atomics.h"    // Atomic, MemoryOrder

namespace nme {

/** @brief Single-Producer Single-Consumer ring buffer. */
template <typename T, usize Capacity>
class SPSCRing {
private:
    // compile-time assertions
    // capacity is limited to a power of two size
    static_assert(std::is_trivially_copyable_v<T>,
        "SPSCRing<T> hands elements over by a trivial copy, put a non-trivial "
        "payload behind a pointer/handle");
    static_assert(Capacity >= 2, "capacity must be at least 2");
    static_assert((Capacity & (Capacity - 1)) == 0, "capacity must be a power of two");

    static constexpr usize kMask = Capacity - 1;
    static constexpr usize kCacheLine = NME_CACHE_LINE_SIZE;

    // --- producer line ---
    alignas(kCacheLine) Atomic<usize> m_write{0};   // next index to write
    usize m_readCache = 0;                                  // producer's stale view of m_read

    // --- consumer line ---
    alignas(kCacheLine) Atomic<usize> m_read{0};    // next index to read
    usize m_writeCache = 0;                                 // consumer's stale view of m_write

    // --- payload ---
    alignas(kCacheLine) T m_slots[Capacity];

public:
    SPSCRing() = default;
    ~SPSCRing() = default;

    // a live queue isn't copyable
    SPSCRing(const SPSCRing& other)            = delete;
    SPSCRing(SPSCRing&& other)                 = delete;
    // it is neither moveable
    SPSCRing& operator=(const SPSCRing& other) = delete;
    SPSCRing& operator=(SPSCRing&& other)      = delete;

    /** @brief Returns false, without touching the ring, if it is full. */
    bool tryPush(const T& item) noexcept {
        // nothing to synchronize to read with, therefore relaxed
        const usize w = m_write.load(MemoryOrder::Relaxed);

        // if the cache already shows room, there is definitely room,
        // and we skip the contended load
        if (w - m_readCache >= Capacity) {
            // cache full, refresh
            m_readCache = m_read.load(MemoryOrder::Acquire);
            if (w - m_readCache >= Capacity) {
                return false;   // genuinely full
            }
        }

        m_slots[w & kMask] = item;                       // (1) write the payload
        m_write.store(w + 1, MemoryOrder::Release); // (2) publish
        return true;
    }

    /** @brief Returns false, leaving `out` untouched, if it is empty. */
    bool tryPop(T& out) noexcept {
        // nothing to sync with, therefore relaxed
        const usize r = m_read.load(MemoryOrder::Relaxed);

        // empty test against cached write index
        if (r == m_writeCache) {
            // observe newer value
            m_writeCache = m_write.load(MemoryOrder::Acquire);
            if (r == m_writeCache) {
                return false;   // genuinely empty
            }
        }

        out = m_slots[r & kMask];                        // (3) read the payload
        m_read.store(r + 1, MemoryOrder::Release);  // (4) release the slot
        return true;
    }

    [[nodiscard]] usize sizeApprox() const noexcept {
        const usize r = m_read.load(MemoryOrder::Acquire);
        const usize w = m_write.load(MemoryOrder::Acquire);
        return w -r;
    }

    [[nodiscard]] bool emptyApprox() const noexcept { return sizeApprox() == 0; }
    [[nodiscard]] bool fullApprox()  const noexcept { return sizeApprox() >= Capacity; }

    [[nodiscard]] static constexpr usize capacity() noexcept { return Capacity; }
    static constexpr usize kCapacity = Capacity;
};

}  // namespace nme

#endif  // NME_SPSC_RING_H_
