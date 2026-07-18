#ifndef NME_VK_COMMON_H_
#define NME_VK_COMMON_H_

#include <vulkan/vulkan.h>

#include "nme/platform/gfx/gfx_types.h"
#include "nme/platform/types.h"

namespace nme::gfx::vk {

struct VulkanDevice {
    VkInstance               instance        = VK_NULL_HANDLE;
    VkPhysicalDevice         physical        = VK_NULL_HANDLE;
    VkDevice                 device          = VK_NULL_HANDLE;
    u32                      graphics_family = 0;
    VkQueue                  graphics_queue  = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT debug           = VK_NULL_HANDLE;
    bool                     validation      = false;
};

// Single active device. Multi-device would promote this to a small registry keyed by the handle id.
// Defined in vk_backend.cpp; used by vk_swapchain.cpp to reach the live device.
VulkanDevice* device_from(Device d) noexcept;

// VkResult -> engine error, translated once at the backend boundary.
GfxError vk_error(VkResult r) noexcept;

}  // namespace nme::gfx::vk

#endif  // NME_VK_COMMON_H_
