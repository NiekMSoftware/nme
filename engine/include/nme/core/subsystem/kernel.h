#pragma once

#include <memory>   // unique_ptr
#include <vector>   // TODO: replace with custom container

#include "nme/core/assert/assert.h"
#include "nme/core/result/error.h"

namespace nme {

class Subsystem;

class Kernel {
    std::vector<std::unique_ptr<Subsystem>> m_subsystems;   ///< Owned subsystems, in start-up order.
    usize m_started = 0;    ///< Count currently up (a prefix of m_subsystems).

public:
    Kernel() = default;
    ~Kernel() { shutdown(); }

    Kernel(const Kernel&)            = delete;
    Kernel& operator=(const Kernel&) = delete;

    template<class T, class... Args>
    T* add(Args&&... args);

    [[nodiscard]] Error startup();

    void shutdown();
};

template <class T, class... Args>
T* Kernel::add(Args&&... args) {
    NME_ASSERT(m_started == 0);
    auto sys = std::make_unique<T>(std::forward<Args>(args)...);
    T* borrowed = sys.get();
    m_subsystems.push_back(std::move(sys));
    return borrowed;
}

}  // namespace nme