#pragma once

#include <cstdio>

#include "nme/platform/timer/timer.h"
#include "nme/platform/types.h"

namespace nme {

class [[nodiscard]] ScopedTimer {
    platform::Timer& m_timer;
    const char*      m_label;
    u64              m_start;

public:
    ScopedTimer(platform::Timer& timer, const char* label) noexcept
        : m_timer(timer), m_label(label), m_start(timer.now()) { }

    ~ScopedTimer() {
        const u64 elapsed = m_timer.now() - m_start;
        const f64 ms      = m_timer.to_seconds(elapsed) * 1000.0;
        std::printf("[timer] %-20s %8.3f ms\n", m_label, ms);   // -> profiler later
    }

    ScopedTimer(const ScopedTimer&) = delete;
    ScopedTimer& operator=(const ScopedTimer&) = delete;
};

}  // namespace nme

#define NME_CONCAT_IMPL(a, b) a##b
#define NME_CONCAT(a, b)      NME_CONCAT_IMPL(a, b)
#define NME_PROFILE_SCOPE(label)                                 \
    ::nme::ScopedTimer NME_CONCAT(nme_scoped_timer_, __LINE__) { \
        ::nme::platform::global_timer(), label }
