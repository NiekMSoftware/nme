#include <cstring>  // std::strcmp
#include <vector>  // TODO: swap for dynamic arr container once that's set

#include "nme/platform/gfx/gfx.h"
#include "vk_common.h"

namespace nme::gfx {

namespace {

vk::VulkanDevice* g_vk = nullptr;   // the one active device

// Returns true and writes the first graphics-capable queue family index.
bool find_graphics_family(VkPhysicalDevice phys, u32* out_family) {
    u32 qcount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(phys, &qcount, nullptr);

    std::vector<VkQueueFamilyProperties> qprops(qcount);

    vkGetPhysicalDeviceQueueFamilyProperties(phys, &qcount, qprops.data());

    for (u32 i = 0; i < qcount; ++i) {
        if (qprops[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            *out_family = i;
            return true;
        }
    }
    return false;
}

// Higher is better
i32 score_device(VkPhysicalDevice phys) {
    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(phys, &props);

    i32 score = 0;
    if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) score += 1000;
    score += static_cast<i32>(props.limits.maxImageDimension2D);    // rough quality proxy
    return score;
}

// Pick the highest scoring device that actually has a graphics queue
// and report that queue's family.
bool pick_physical(VkInstance instance, VkPhysicalDevice* out_phys, u32* out_family) {
    u32 count = 0;
    vkEnumeratePhysicalDevices(instance, &count, nullptr);
    if (count == 0) return false;

    std::vector<VkPhysicalDevice> devices(count);
    vkEnumeratePhysicalDevices(instance, &count, devices.data());

    VkPhysicalDevice best        = VK_NULL_HANDLE;
    u32              best_family = 0;
    i32              best_score  = -1;

    for (VkPhysicalDevice device : devices) {
        u32 family = 0;
        if (!find_graphics_family(device, &family)) continue;   // no graphics queue -> unusable

        if (const i32 score = score_device(device); score > best_score) {
            best        = device;
            best_family = family;
            best_score  = score;
        }
    }

    if (best == VK_NULL_HANDLE) return false;

    *out_phys = best;
    *out_family = best_family;

    return true;
}

constexpr const char* kValidationLayer = "VK_LAYER_KHRONOS_validation";

bool validation_available() {
    u32 count = 0;
    vkEnumerateInstanceLayerProperties(&count, nullptr);

    std::vector<VkLayerProperties> layers(count);

    vkEnumerateInstanceLayerProperties(&count, layers.data());
    for (const auto& layer : layers) {
        if (std::strcmp(layer.layerName, kValidationLayer) == 0) return true;
    }
    return false;
}

}  // anonymous namespace

namespace vk {

VulkanDevice* device_from(Device) noexcept { return g_vk; }

GfxError to_error(const VkResult r) noexcept {
    switch (r) {
        case VK_SUCCESS: return GfxError::None;
        case VK_ERROR_OUT_OF_HOST_MEMORY:
        case VK_ERROR_OUT_OF_DEVICE_MEMORY:   return GfxError::OutOfMemory;
        case VK_ERROR_DEVICE_LOST:            return GfxError::DeviceLost;
        case VK_ERROR_SURFACE_LOST_KHR:       return GfxError::SurfaceLost;
        case VK_ERROR_OUT_OF_DATE_KHR:        return GfxError::SwapchainOutOfDate;
        case VK_ERROR_INITIALIZATION_FAILED:  return GfxError::BackendUnavailable;
        default:                              return GfxError::Unknown;
    }
}

}  // namespace vk

}  // namespace nme::gfx