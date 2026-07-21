#include "vk_common.h"
#include "nme/renderer/gdi/gdi.h"

namespace nme::gdi {

GfxResult<Swapchain> create_swapchain(Device, Surface, const SwapchainDesc*) {
    return result_ok<Swapchain, GfxError>({});   // TODO: real swapchain; None-but-null placeholder
}

void               destroy_swapchain(Swapchain) {}                                      // TODO: views, swapchain, surface, sync
GfxResult<Texture> swapchain_acquire(Swapchain) { return result_err<Texture, GfxError>(GfxError::Unknown); }  // TODO: vkAcquireNextImageKHR
void               swapchain_present(Swapchain) {}                                      // TODO: vkQueuePresentKHR
GfxError           swapchain_resize(Swapchain, Extent2D) { return GfxError::Unknown; }  // TODO: recreate
Format             swapchain_format(Swapchain) { return Format::BGRA8Srgb; }            // TODO
Extent2D           swapchain_extent(Swapchain) { return {}; }                           // TODO

}  // namespace nme::gdi