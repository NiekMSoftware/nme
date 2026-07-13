#pragma once

#include "nme/core/result/error.h"

namespace nme {

class Subsystem {
public:
    virtual ~Subsystem() = default;

    [[nodiscard]] virtual Error startup() = 0;
    virtual void shutdown() = 0;

    [[nodiscard]] virtual const char* name() const = 0;
};

}  // namespace nme