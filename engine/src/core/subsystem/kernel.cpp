#include "nme/core/subsystem/kernel.h"

#include "nme/core/subsystem/subsystem.h"

namespace nme {

SubsystemError Kernel::startup() {
    if (m_started > 0)
        return subsystem_error(SubsystemError::Category::AlreadyInitialized,
                               "kernel: startup() called while already started");

    const usize count = dynamic_array_size(&m_subsystems);
    for (m_started = 0; m_started < count; ++m_started) {
        if (const auto err = m_subsystems[m_started]->startup(); subsystem_failed(err)) {
            while (m_started > 0)
                m_subsystems[--m_started]->shutdown();
            return err;   // carries the failing subsystem's category + detail up
        }
    }
    return subsystem_ok();
}

void Kernel::shutdown() {
    // reverse shutdown
    while (m_started > 0)
        m_subsystems[--m_started]->shutdown();

    // destroy + free every subsystem the array owns
    for (usize i = dynamic_array_size(&m_subsystems); i-- > 0;) {
        Subsystem* s = m_subsystems[i];
        s->~Subsystem();                // virtual dtor runs the concrete one
        free(&m_alloc, s, 0);     // header of heap alloc carries size; therefore, we pass 0
    }
    dynamic_array_clear(&m_subsystems);
}

}  // namespace nme