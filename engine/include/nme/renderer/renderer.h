#pragma once

#include "nme/core/subsystem/subsystem.h"
#include "nme/core/subsystem/subsystem_error.h"

//==============================================================================
// Low-Level Renderer (Volume II)
//   Gregory sec. 1.5 -- Low-Level Renderer
//------------------------------------------------------------------------------
// Responsibility documented; implementation added in the chapter/volume noted.
// TODO: graphics device interface, materials/shaders, cameras, submission.
//==============================================================================

namespace nme {

// Rich and private to the renderer; mapped to SubsystemError at the boundary.
enum class RendererError {
    None = 0,
    NoDevice,             // no suitable graphics adapter
    DeviceLost,           // adapter vanished / reset during init
    ShaderCompileFailed,
};

[[nodiscard]] constexpr SubsystemError::Category renderer_error_category(const RendererError e) noexcept {
    switch (e) {
        case RendererError::None:                return SubsystemError::Category::None;
        case RendererError::NoDevice:
        case RendererError::DeviceLost:          return SubsystemError::Category::DeviceUnavailable;
        case RendererError::ShaderCompileFailed: return SubsystemError::Category::InvalidConfig;
    }
    return SubsystemError::Category::Unknown;
}

[[nodiscard]] constexpr const char* renderer_error_to_str(const RendererError e) noexcept {
    switch (e) {
        case RendererError::None:                return "None";
        case RendererError::NoDevice:            return "NoDevice";
        case RendererError::DeviceLost:          return "DeviceLost";
        case RendererError::ShaderCompileFailed: return "ShaderCompileFailed";
    }
    return "RendererError(?)";
}

// carrier: coarse category for the Kernel + raw enum as code + log string in detail
[[nodiscard]] constexpr SubsystemError to_subsystem_error(const RendererError e) noexcept {
    return subsystem_error(renderer_error_category(e), renderer_error_to_str(e), static_cast<u32>(e));
}

class Renderer final : public Subsystem {
public:
    [[nodiscard]] SubsystemError startup() override;
    void shutdown() override;

    [[nodiscard]] const char* name() const override { return "Renderer"; }

private:
    RendererError init();   // real work speaks RendererError; startup() maps it
};

}  // namespace nme