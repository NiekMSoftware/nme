// Unit tests for nme::SPSCRing.
//
// This TU only contains test cases; the doctest runner/main lives in
// test_main.cpp (the one with DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN). Link the two
// together. All CHECK/REQUIRE calls happen on the main thread only - doctest's
// assertion macros are not meant to be fired from worker threads, so the
// concurrency test funnels its results back through plain variables that are
// read after join().
//
//
// Test written by a LLM to cover everything fully

#include <doctest/doctest.h>

#include "nme/platform/thread/spsc_ring.h"
#include "nme/platform/thread/thread.h"

#include <cstdint>
#include <type_traits>

using nme::SPSCRing;
using nme::Thread;
using nme::yieldCurrentThread;

TEST_CASE("SPSCRing is empty on construction") {
    SPSCRing<int, 4> ring;

    CHECK(ring.emptyApprox());
    CHECK_FALSE(ring.fullApprox());
    CHECK(ring.sizeApprox() == 0);
    CHECK(ring.capacity() == 4);

    int out = -1;
    CHECK_FALSE(ring.tryPop(out));   // pop on empty fails...
    CHECK(out == -1);                // ...and leaves the target untouched
}

TEST_CASE("SPSCRing uses every slot — no sacrificed slot") {
    SPSCRing<int, 4> ring;

    REQUIRE(ring.tryPush(1));
    REQUIRE(ring.tryPush(2));
    REQUIRE(ring.tryPush(3));
    REQUIRE(ring.tryPush(4));        // 4th push into a capacity-4 ring must succeed

    CHECK(ring.sizeApprox() == 4);
    CHECK(ring.fullApprox());
    CHECK_FALSE(ring.tryPush(5));    // now genuinely full
    CHECK(ring.sizeApprox() == 4);   // the failed push changed nothing
}

TEST_CASE("SPSCRing preserves FIFO order") {
    SPSCRing<int, 8> ring;
    for (int i = 0; i < 5; ++i) REQUIRE(ring.tryPush(i * 10));

    int out = 0;
    for (int i = 0; i < 5; ++i) {
        REQUIRE(ring.tryPop(out));
        CHECK(out == i * 10);
    }
    CHECK(ring.emptyApprox());
}

TEST_CASE("SPSCRing sizeApprox tracks single-threaded push/pop exactly") {
    SPSCRing<int, 8> ring;
    int out = 0;
    CHECK(ring.sizeApprox() == 0);
    ring.tryPush(1); CHECK(ring.sizeApprox() == 1);
    ring.tryPush(2); CHECK(ring.sizeApprox() == 2);
    ring.tryPop(out); CHECK(ring.sizeApprox() == 1);
    ring.tryPop(out); CHECK(ring.sizeApprox() == 0);
}

TEST_CASE("SPSCRing: drain to empty, then pop reports empty (regression)") {
    // The exact case an earlier empty-check bug got wrong: after draining, a
    // further pop must report empty rather than hand back a stale slot.
    SPSCRing<int, 4> ring;
    REQUIRE(ring.tryPush(42));

    int out = -1;
    REQUIRE(ring.tryPop(out));
    CHECK(out == 42);
    CHECK(ring.emptyApprox());

    out = -1;
    CHECK_FALSE(ring.tryPop(out));   // empty again
    CHECK(out == -1);
}

TEST_CASE("SPSCRing survives many index wraps") {
    // Capacity 4 over 100k cycles => ~25k wraps of the free-running indices.
    // Exercises the mask / wrap arithmetic that a plain modulo scheme wouldn't.
    SPSCRing<int, 4> ring;
    int expect = 0, out = 0;
    for (int i = 0; i < 100000; ++i) {
        REQUIRE(ring.tryPush(i));
        REQUIRE(ring.tryPop(out));
        CHECK(out == expect++);
    }
    CHECK(ring.emptyApprox());
}

TEST_CASE("SPSCRing works at the minimum capacity of 2") {
    SPSCRing<int, 2> ring;
    REQUIRE(ring.tryPush(10));
    REQUIRE(ring.tryPush(20));
    CHECK_FALSE(ring.tryPush(30));   // full at 2

    int out = 0;
    REQUIRE(ring.tryPop(out)); CHECK(out == 10);
    REQUIRE(ring.tryPop(out)); CHECK(out == 20);
    CHECK_FALSE(ring.tryPop(out));
}

TEST_CASE("SPSCRing carries a trivially-copyable struct payload") {
    struct Msg { std::uint32_t id; float value; };
    static_assert(std::is_trivially_copyable_v<Msg>, "Msg must be trivially copyable");

    SPSCRing<Msg, 4> ring;
    REQUIRE(ring.tryPush(Msg{1, 1.5f}));
    REQUIRE(ring.tryPush(Msg{2, 2.5f}));

    Msg out{};
    REQUIRE(ring.tryPop(out));
    CHECK(out.id == 1);
    CHECK(out.value == doctest::Approx(1.5f));
    REQUIRE(ring.tryPop(out));
    CHECK(out.id == 2);
    CHECK(out.value == doctest::Approx(2.5f));
}

TEST_CASE("SPSCRing carries pointer payloads (the Job* case)") {
    SPSCRing<int*, 4> ring;
    int a = 0, b = 0;
    REQUIRE(ring.tryPush(&a));
    REQUIRE(ring.tryPush(&b));

    int* out = nullptr;
    REQUIRE(ring.tryPop(out)); CHECK(out == &a);
    REQUIRE(ring.tryPop(out)); CHECK(out == &b);
}

TEST_CASE("SPSCRing: one producer + one consumer transfer everything, in order"
          " * doctest::timeout(30.0)") {
    // The real contract. Compile this TU with -fsanitize=thread to also prove
    // the acquire/release edges are race-free; without TSan it still proves
    // nothing is lost, duplicated, or reordered.
    constexpr std::uint64_t N = 1'000'000;
    SPSCRing<std::uint64_t, 1024> ring;

    // Written only by the consumer thread; read on main after join() (which is
    // itself a happens-before edge), so no atomics are needed here.
    bool orderOk = true;
    std::uint64_t checksum = 0;

    Thread producer([&] {
        for (std::uint64_t i = 0; i < N; ++i)
            while (!ring.tryPush(i)) yieldCurrentThread();
    });

    Thread consumer([&] {
        std::uint64_t next = 0;
        std::uint64_t v = 0;
        while (next < N) {
            if (ring.tryPop(v)) {
                if (v != next) orderOk = false;   // must arrive strictly in order
                checksum += v;
                ++next;
            } else {
                yieldCurrentThread();
            }
        }
    });

    producer.join();
    consumer.join();

    CHECK(orderOk);
    CHECK(checksum == N * (N - 1) / 2);   // sum of 0..N-1 => none lost or duplicated
}