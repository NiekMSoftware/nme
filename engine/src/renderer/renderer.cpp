#include "nme/renderer/renderer.h"

namespace nme {

namespace {

// GDI failures are broader than RendererError; fold them onto the closest cause.
RendererError renderer_error_from(const gfx::GfxError e) {
    switch (e) {
        case gfx::GfxError::None:               return RendererError::None;
        case gfx::GfxError::BackendUnavailable: return RendererError::NoDevice;
        case gfx::GfxError::DeviceLost:
        case gfx::GfxError::SurfaceLost:        return RendererError::DeviceLost;
        default:                                return RendererError::NoDevice;  // OOM/Unknown: coarse for now
    }
}

}  // anonymous namespace

SubsystemError Renderer::startup() {
    return to_subsystem_error(init());
}

void Renderer::shutdown() {
    if (gfx::valid(device_)) gdi::device_wait_idle(device_);   // drain any in-flight work first

    if (gfx::valid(swapchain_)) { gdi::destroy_swapchain(swapchain_); swapchain_ = {}; }
    if (gfx::valid(device_))    { gdi::destroy_device(device_);       device_    = {}; }
}

RendererError Renderer::init() {
    // --- device ---
    gdi::DeviceDesc dd{};
    dd.backend = gdi::Backend::Vulkan;
    dd.debug   = true;                     // validation layers on; gate on build config for ship

    auto rd = gdi::create_device(&dd, alloc_);
    if (result_is_err(&rd)) return renderer_error_from(result_error(&rd));
    device_ = result_value(&rd);

    // --- swapchain over the window surface ---
    gdi::SwapchainDesc sd{};
    sd.extent       = gfx::surface_size(surface_);
    sd.format       = gdi::Format::BGRA8Srgb;   // typical desktop back buffer
    sd.present_mode = gdi::PresentMode::Fifo;   // always supported (vsync)
    sd.image_count  = 3;                        // triple buffer

    auto rsc = gdi::create_swapchain(device_, surface_, &sd);
    if (result_is_err(&rsc)) {
        gdi::destroy_device(device_);           // unwind: don't leak the device we just made
        device_ = {};
        return renderer_error_from(result_error(&rsc));
    }
    swapchain_ = result_value(&rsc);

    return RendererError::None;
}

}  // namespace nme