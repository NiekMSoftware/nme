#include <chrono>  // high resolution clock (might be replaced in the future)
#include "nme/platform/timer/timer.h"

namespace nme::platform {

using Clock = std::chrono::high_resolution_clock;
static_assert(Clock::is_steady, "nme::platform::Timer requires a monotonic clock");


bool Timer::startup() noexcept {
    // steady_clock's tick rate is a compile-time ratio (period); ticks/sec is
    // den/num. A nanosecond clock (the common case) yields 1'000'000'000.
    m_frequency = static_cast<u64>(Clock::period::den) /
                  static_cast<u64>(Clock::period::num);
    return m_frequency != 0;
}

void Timer::shutdown() noexcept {
    m_frequency = 0;
}

u64 Timer::now() noexcept {
    return static_cast<u64>(Clock::now().time_since_epoch().count());
}

u64 Timer::frequency() const noexcept {
    return m_frequency;
}

f64 Timer::to_seconds(const u64 ticks) const noexcept {
    return (m_frequency == 0)
            ? 0.0
            : static_cast<f64>(ticks) / static_cast<f64>(m_frequency);
}

Timer& global_timer() noexcept {
    static Timer instance;      // one clock; engine calls .startup()/.shutdown()
    return instance;
}

}  // namespace nme::platform
