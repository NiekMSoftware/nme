#include <cassert>
#include <cstdio>

#include "nme/core/jobs/job_system.h"
#include "nme/platform/thread/atomics.h"
#include "nme/platform/types.h"

int main() {
    using namespace nme;

    constexpr u32 kJobCount = 10'000;

    JobSystem js;
    js.startup();
    printf("workers: %u\n", js.workerCount());

    Atomic<u32> ran{0};
    JobCounter counter;

    js.runN(kJobCount, [&ran](u32 /*i*/) {
        ran.fetchAdd(1, MemoryOrder::Relaxed);
    }, counter);

    js.waitForCounter(counter);

    const u32 total = ran.load(MemoryOrder::Acquire);
    printf("ran: %u / %u\n", total, kJobCount);
    assert(total == kJobCount && "every job must run exactly once");
    assert(counter.done() && "counter must reach zero");

    static u32 slots[kJobCount] = {};
    JobCounter counter2;
    js.runN(kJobCount, [](const u32 i) { slots[i] = i * 2 + 1; }, counter2 );
    js.waitForCounter(counter2);

    for (u32 i = 0; i < kJobCount; i++) {
        assert(slots[i] == i * 2 + 1 && "each slot must be written by its job");
    }

    js.shutdown();
    printf("PASS\n");
    return 0;
}