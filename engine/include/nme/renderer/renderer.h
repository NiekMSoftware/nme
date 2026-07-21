#pragma once

#include "nme/core/subsystem/subsystem.h"
#include "nme/core/subsystem/subsystem_error.h"
#include "nme/platform/gfx/gfx.h"          // gfx::Surface, gfx::valid
#include "nme/renderer/gdi/gdi.h"          // gdi::Device / Swapchain and the GDI calls

//==============================================================================
// Low-Level Renderer
//------------------------------------------------------------------------------
// For now this owns the Graphics Device Interface: it brings up the GDI device
// and a swapchain over the window's surface, and tears them down in reverse.
// TODO: materials/shaders, cameras, per-frame submission.
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
    // The window's surface must already be valid (start this AFTER the window
    // subsystem). alloc backs the GDI's host-side allocations.
    Renderer(const gfx::Surface surface, const Allocator& alloc)
        : alloc_(alloc), surface_(surface) {}

    [[nodiscard]] SubsystemError startup() override;
    void shutdown() override;

    [[nodiscard]] const char* name() const override { return "Renderer"; }

    [[nodiscard]] gdi::Device    device()    const { return device_; }
    [[nodiscard]] gdi::Swapchain swapchain() const { return swapchain_; }

private:
    RendererError init();   // real work speaks RendererError; startup() maps it

    Allocator      alloc_;
    gfx::Surface   surface_{};
    gdi::Device    device_{};
    gdi::Swapchain swapchain_{};
};

}  // namespace nme