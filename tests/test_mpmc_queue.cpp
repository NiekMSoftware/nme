#include <doctest/doctest.h>

#include <nme/platform/thread/thread.h>
#include <nme/platform/thread/mutex.h>
#include <nme/platform/thread/atomics.h>
#include <nme/platform/types.h>

#include <cstdio>
#include <chrono>
#include <memory>
#include <queue>
#include <vector>

namespace {

template<typename Lockable>
class ScopedLock {
    Lockable& m_lock;
public:
    explicit ScopedLock(Lockable& lock) : m_lock(lock) { m_lock.lock(); }

    ~ScopedLock() { m_lock.unlock(); }
    ScopedLock(const ScopedLock&) = delete;
    ScopedLock& operator=(const ScopedLock&) = delete;
};

template<typename T>
class MutexQueue {
    nme::Mutex      m_mutex;
    std::queue<T>   m_items;

public:
    void push(const T& v) {
        ScopedLock<nme::Mutex> guard(m_mutex);
        m_items.push(v);
    }
    bool tryPop(T& out) {
        ScopedLock<nme::Mutex> guard(m_mutex);
        if (m_items.empty()) {
            return false;
        }
        out = m_items.front();
        m_items.pop();
        return true;
    }
    bool empty() {
        ScopedLock<nme::Mutex> guard(m_mutex);
        return m_items.empty();
    }
};

}  // namespace

TEST_SUITE("mpmc") {
    TEST_CASE("MutexQueue loses no items under contention") {
        using nme::Atomic;
        using nme::MemoryOrder;
        using nme::Thread;
        using nme::ThreadConfig;

        u32 producers = 1;
        u32 consumers = 1;
        SUBCASE("SPSC (1 producer, 1 consumer)") { producers = 1; consumers = 1; }
        SUBCASE("MPSC (4 producer, 1 consumer)") { producers = 4; consumers = 1; }
        SUBCASE("SPMC (1 producer, 1 consumer)") { producers = 1; consumers = 4; }
        SUBCASE("MPMC (1 producer, 1 consumer)") { producers = 4; consumers = 4; }

        CAPTURE(producers);     // printed alongside any failure
        CAPTURE(consumers);

        // Scale down ~10x when running under ThreadSanitizer.
        constexpr u32 kItemsPerProducer = 250'000;
        const u64 total = static_cast<u64>(producers) * kItemsPerProducer;

        MutexQueue<u32> queue;

        auto seen = std::make_unique<Atomic<u32>[]>(total); // Atomic() zero-inits

        Atomic<u64> consumed{0};
        Atomic<u32> live_producers{producers};

        const auto t_start = std::chrono::high_resolution_clock::now();

        std::vector<Thread> threads;
        threads.reserve(producers + consumers);     // reserve => no move-on-growth

        // Producers: disjoin value ranges cover [0, total] exactly once.
        for (u32 p = 0; p < producers; ++p) {
            char name[16];
            std::snprintf(name, sizeof(name), "nme.prod.%u", p);
            ThreadConfig cfg;
            cfg.name = name;    // consumed synchronously during construction

            const u32 begin = p * kItemsPerProducer;
            const u32 end   = begin + kItemsPerProducer;

            threads.emplace_back(
                [&queue, &live_producers, begin, end] {
                    for (u32 v = begin; v < end; ++v) {
                        queue.push(v);
                    }
                    // Release: a consumer that sees the count hit zero also sees
                    // this producer's pushes.
                    live_producers.fetchSub(1, MemoryOrder::Release);
                },
                cfg
            );
        }

        // Consumers: pop until producers are done and the queue is truly empty.
        for (u32 c = 0; c < consumers; ++c) {
            char name[16];
            std::snprintf(name, sizeof(name), "nme.cons.%u", c);
            ThreadConfig cfg;
            cfg.name = name;

            threads.emplace_back(
                [&queue, &seen, &consumed, &live_producers] {
                    u32 v;
                    for (;;) {
                        if (queue.tryPop(v)) {
                            seen[v].fetchAdd(1, MemoryOrder::Relaxed);
                            consumed.fetchAdd(1, MemoryOrder::Relaxed);
                            continue;
                        }
                        if (live_producers.load(MemoryOrder::Acquire) != 0) {
                            nme::yieldCurrentThread();  // producers still live; back off
                            continue;
                        }
                        if (queue.tryPop(v)) {
                            seen[v].fetchAdd(1, MemoryOrder::Relaxed);
                            consumed.fetchAdd(1, MemoryOrder::Relaxed);
                            continue;
                        }
                        break;
                    }
                },
                cfg
            );
        }

        for (Thread& t : threads) {
            t.join();   // all assertions below run here, on the main thread
        }

        const auto t_end = std::chrono::high_resolution_clock::now();
        const f64 elapsed = std::chrono::duration<f64>(t_end - t_start).count();

        u64 lost = 0;
        u64 duplicated = 0;
        for (u64 i = 0; i < total; ++i) {
            const u32 n = seen[i].load(MemoryOrder::Relaxed);
            if (n == 0) {
                ++lost;
            } else if (n > 1) {
                ++duplicated;
            }
        }

        MESSAGE("items=" << total << "  "
            << (elapsed > 0.0f ? (static_cast<f64>(total) / elapsed) / 1e6 : 0.0)
            << " M items/s");

        CHECK(consumed.load(MemoryOrder::Relaxed) == total);
        CHECK(lost == 0);
        CHECK(duplicated == 0);
        CHECK(queue.empty());
    }
}