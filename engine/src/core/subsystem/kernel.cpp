#include "nme/core/subsystem/kernel.h"

#include "nme/core/subsystem/subsystem.h"

namespace nme {

Error Kernel::startup() {
    if (m_started > 0)
        return Error::AlreadyInitialized;

    for (m_started = 0; m_started < m_subsystems.size(); ++m_started) {
        const Error e = m_subsystems[m_started]->startup();
        if (NME_FAILED(e)) {
            // This subsystem cleaned up after itself; unwind the ones already up in reverse.
            while (m_started > 0)
                m_subsystems[--m_started]->shutdown();
            return e;
        }
    }

    return Error::None;
}

void Kernel::shutdown() {
    while (m_started > 0)
        m_subsystems[--m_started]->shutdown();
    m_subsystems.clear();
}

}  // namespace nme