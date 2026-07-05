#include <doctest/doctest.h>

#include <atomic>
#include <thread>
#include <vector>

#include "nme/core/jobs/ws_deque.h"

using nme::WSDeque;
using Steal = WSDeque<int, 1024>::StealResult;

// -----------------------------------------------------------------------------
// Deterministic, single-threaded behaviour (owner side only).
// -----------------------------------------------------------------------------
TEST_CASE("single-threaded owner semantics") {
    WSDeque<int, 1024> d;
    int v = -1;

    SUBCASE("empty pop fails and does not underflow") {
        // Regression: bottom is unsigned; popping an empty deque must not
        // wrap bottom to SIZE_MAX. Pop several times while empty.
        for (int i = 0; i < 4; ++i) {
            REQUIRE_FALSE(d.pop(v));
        }
        REQUIRE(d.emptyApprox());
    }

    SUBCASE("owner pop is LIFO") {
        for (int i = 0; i < 500; ++i) REQUIRE(d.push(i));
        CHECK(d.sizeApprox() == 500);
        for (int i = 499; i >= 0; --i) {
            REQUIRE(d.pop(v));
            CHECK(v == i);
        }
        CHECK_FALSE(d.pop(v));
        CHECK(d.emptyApprox());
    }

    SUBCASE("push then pop then push reuses slots (wrap-around)") {
        // Cycle well past Capacity so the ring index wraps many times.
        for (int cycle = 0; cycle < 10; ++cycle) {
            for (int i = 0; i < 700; ++i) REQUIRE(d.push(i));
            for (int i = 699; i >= 0; --i) {
                REQUIRE(d.pop(v));
                CHECK(v == i);
            }
        }
    }

    SUBCASE("push fails when full, never silently drops") {
        // Fill to capacity, next push must report false.
        for (nme::usize i = 0; i < d.capacity(); ++i) REQUIRE(d.push(static_cast<int>(i)));
        CHECK_FALSE(d.push(9999));            // full
        CHECK(d.sizeApprox() == d.capacity());
        REQUIRE(d.pop(v));                    // make room
        CHECK(d.push(9999));                  // now it fits
    }
}

// -----------------------------------------------------------------------------
// Empty deque: a steal reports Empty, never Success.
// -----------------------------------------------------------------------------
TEST_CASE("steal on empty reports Empty") {
    WSDeque<int, 1024> d;
    int v = -1;
    CHECK(d.steal(v) == Steal::Empty);

    REQUIRE(d.push(42));
    CHECK(d.steal(v) == Steal::Success);
    CHECK(v == 42);
    CHECK(d.steal(v) == Steal::Empty);       // drained again
}

// -----------------------------------------------------------------------------
// Concurrent invariant: across owner pop + all thieves' steals, every pushed
// item is taken EXACTLY ONCE. Non-deterministic by nature, so the case runs
// several rounds -- a broken deque (missing-fence double-take, or lost item)
// fails the invariant on at least one schedule with high probability.
//
// NOTE: TSan cannot model atomic_thread_fence, so a clean TSan run does NOT
// prove the ordering; and x86's strong model hides the pop() fence bug. This
// test validates the ALGORITHM. Run it on ARM to exercise weak ordering.
// -----------------------------------------------------------------------------
TEST_CASE("owner vs thieves, each item taken exactly once") {
    constexpr int   kItems    = 100000;
    constexpr int   kThieves  = 7;
    constexpr nme::usize kCapacity = 1 << 16;
    using Deque = WSDeque<int, kCapacity>;

    for (int round = 0; round < 3; ++round) {
        Deque d;
        std::vector<std::atomic<int>> taken(kItems);
        for (auto& c : taken) c.store(0);

        std::atomic<bool> go{false};
        std::atomic<bool> producerDone{false};
        std::atomic<int>  totalTaken{0};

        auto claim = [&](int item) {
            taken[item].fetch_add(1, std::memory_order_relaxed);
            totalTaken.fetch_add(1, std::memory_order_relaxed);
        };

        std::vector<std::thread> thieves;
        thieves.reserve(kThieves);
        for (int t = 0; t < kThieves; ++t) {
            thieves.emplace_back([&] {
                while (!go.load()) { /* spin to start together */ }
                int v;
                while (!producerDone.load() || !d.emptyApprox()) {
                    if (d.steal(v) == Deque::StealResult::Success) claim(v);
                }
            });
        }

        std::thread owner([&] {
            while (!go.load()) {}
            int v;
            for (int i = 0; i < kItems; ++i) {
                while (!d.push(i)) { if (d.pop(v)) claim(v); }  // full -> drain one
                if ((i & 3) == 0 && d.pop(v)) claim(v);         // owner takes some back
            }
            producerDone.store(true);
            while (d.pop(v)) claim(v);                          // drain remainder
        });

        go.store(true);
        owner.join();
        for (auto& th : thieves) th.join();

        int dupes = 0, missing = 0;
        for (int i = 0; i < kItems; ++i) {
            const int c = taken[i].load();
            if (c > 1) ++dupes;
            if (c == 0) ++missing;
        }

        CAPTURE(round);
        CAPTURE(dupes);
        CAPTURE(missing);
        REQUIRE(dupes == 0);                 // double-take == missing-fence bug
        REQUIRE(missing == 0);               // lost work
        REQUIRE(totalTaken.load() == kItems);
    }
}