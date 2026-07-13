#pragma once

#include "nme/core/subsystem/subsystem.h"

//==============================================================================
// Low-Level Renderer (Volume II)
//   Gregory sec. 1.5 -- Low-Level Renderer
//------------------------------------------------------------------------------
// Responsibility documented; implementation added in the chapter/volume noted.
// TODO: graphics device interface, materials/shaders, cameras, submission.
//==============================================================================

namespace nme {

class Renderer final : public Subsystem {
public:
    [[nodiscard]] Error startup() override;
    void shutdown() override;

    [[nodiscard]] const char* name() const override { return "Renderer"; }
};

}  // namespace nme
