#pragma once

#include "nme/core/subsystem/subsystem_error.h"

namespace nme {

class Subsystem {
public:
    virtual ~Subsystem() = default;

    [[nodiscard]] virtual SubsystemError startup() = 0;
    virtual void                         shutdown() = 0;

    [[nodiscard]] virtual const char* name() const = 0;
};

}  // namespace nme