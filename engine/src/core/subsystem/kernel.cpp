#include "nme/core/subsystem/kernel.h"

#include "nme/core/subsystem/subsystem.h"

namespace nme {

Error Kernel::startup() {
    if (m_started > 0)
        return Error::AlreadyInitialized;

    const usize count = dynamic_array_size(&m_subsystems);
    for (m_started = 0; m_started < count; ++m_started) {
        if (const Error e = m_subsystems[m_started]->startup(); NME_FAILED(e)) {
            while (m_started > 0)
                m_subsystems[--m_started]->shutdown();
            return e;
        }
    }
    return Error::None;
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