#include "vk_common.h"

#include "nme/platform/gfx/gfx.h"

#include <cstring>  // std::strcmp
#include <vector>   // TODO: swap for dynamic arr container once that's set

namespace nme::gfx {

namespace {

vk::VulkanDevice* g_vk = nullptr;   // the one active device

// Pick the first physical device exposing a graphics-capable queue family.
// TODO: score devices (discrete > integrated) and require present support.
bool pick_physical(VkInstance instance, VkPhysicalDevice* out_phys, u32* out_family) {
    u32 count = 0;
    vkEnumeratePhysicalDevices(instance, &count, nullptr);
    if (count == 0) return false;

    std::vector<VkPhysicalDevice> devices(count);
    vkEnumeratePhysicalDevices(instance, &count, devices.data());

    for (VkPhysicalDevice phys : devices) {
        u32 qcount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(phys, &qcount, nullptr);
        std::vector<VkQueueFamilyProperties> qprops(qcount);
        vkGetPhysicalDeviceQueueFamilyProperties(phys, &qcount, qprops.data());

        for (u32 i = 0; i < qcount; ++i) {
            if (qprops[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                *out_phys = phys;
                *out_family = i;
                return true;
            }
        }
    }
    return false;
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

}  // namespace nme::gfx