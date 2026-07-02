#pragma once

//==============================================================================
// Platform -- Hi-Res Timer
//   Gregory Ch. 8.5 Measuring and Dealing with Time
//------------------------------------------------------------------------------
// Timer interface to further implement with remaining timers.
//==============================================================================

#include "nme/platform/types.h"

namespace nme::platform {

class Timer {
private:
    u64 m_frequency = 0ULL;     // 0 until startup

public:
    Timer() = default;
    Timer(const Timer&)            = delete;  // single owner (the engine)
    Timer& operator=(const Timer&) = delete;

    bool startup() noexcept;                  // caches tick frequency
    void shutdown() noexcept;

    [[nodiscard]] static u64 now() noexcept;        // current tick (monotonic)
    [[nodiscard]] u64 frequency() const noexcept;   // ticks per second (cached)

    // Convert tick to delta seconds. Keep this off the per-frame hot path.
    [[nodiscard]] f64 to_seconds(u64 ticks) const noexcept;
};

// The engine's clock. A single Timer, brought up in engine startup and read by
// core/debug ScopedTimer (and its NME_PROFILE_SCOPE macro) without threading a
// reference through every call site. Thread-safe init (C++11 local static).
[[nodiscard]] Timer& global_timer() noexcept;

}  // namespace nme::platform
