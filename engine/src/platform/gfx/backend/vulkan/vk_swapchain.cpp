#include "vk_common.h"

#include "nme/platform/gfx/gfx.h"

namespace nme::gfx {

Swapchain create_swapchain(Device, Surface, const SwapchainDesc*, GfxError* out_err) {
    if (out_err) *out_err = GfxError::None;
    return {};
}

void     destroy_swapchain(Swapchain) {}                                      // TODO: views, swapchain, surface, sync
Texture  swapchain_acquire(Swapchain) { return {}; }                       // TODO: vkAcquireNextImageKHR
void     swapchain_present(Swapchain) {}                                      // TODO: vkQueuePresentKHR
GfxError swapchain_resize(Swapchain, Extent2D) { return GfxError::Unknown; }  // TODO: recreate
Format   swapchain_format(Swapchain) { return Format::BGRA8Srgb; }            // TODO
Extent2D swapchain_extent(Swapchain) { return {}; }                        // TODO

}  // namespace nme::gfx