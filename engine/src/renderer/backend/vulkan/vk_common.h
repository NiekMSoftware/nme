#ifndef NME_GDI_BACKEND_VULKAN_VK_COMMON_H_
#define NME_GDI_BACKEND_VULKAN_VK_COMMON_H_

#include <vk_mem_alloc.h>  // decl only, impl is in vk_vma.cpp
#include <vulkan/vulkan.h>

#include "nme/platform/collections/dynamic_array.h"
#include "nme/platform/gfx/gfx_types.h"
#include "nme/platform/memory/allocator.h"
#include "nme/renderer/gdi/gdi_types.h"

namespace nme::gdi::vk {

struct BufferResource {
    VkBuffer      buffer     = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;
    u64           size       = 0;
    u16           generation = 0;   // live slots carry >= 1, so an encoded id is never 0
    bool          in_use     = false;
};

struct VulkanDevice {
    VkInstance               instance        = VK_NULL_HANDLE;
    VkPhysicalDevice         physical        = VK_NULL_HANDLE;
    VkDevice                 device          = VK_NULL_HANDLE;
    u32                      graphics_family = 0;
    VkQueue                  graphics_queue  = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT debug           = VK_NULL_HANDLE;
    bool                     validation      = false;

    VmaAllocator             allocator       = nullptr;     // backs all resource memory
    Allocator                alloc{};                       // host allocator that owns this block

    DynamicArray<BufferResource> buffers{};                 // the buffered pool
    DynamicArray<u32>            buffer_free{};             // recycled slot indices
};

// --- handle encoding: id = (generation << kHandleIndexBits) | index ---

constexpr u32 kHandleIndexBits = 16;
constexpr u32 kHandleIndexMask = (1u << kHandleIndexBits) - 1;

// Buffer handle -> pooled resource, or nullptr for null / stale / freed handles.
inline BufferResource* resolve_buffer(VulkanDevice* vd, const Buffer h) {
    const u32 index = h.id & kHandleIndexMask;
    const u32 gen   = h.id >> kHandleIndexBits;
    if (index >= dynamic_array_size(&vd->buffers)) return nullptr;
    BufferResource* r = &vd->buffers[index];
    return (r->in_use && r->generation == gen) ? r : nullptr;
}

// Single active device. Multi-device would promote this to a small registry keyed by the handle id.
// Defined in vk_backend.cpp; used by vk_swapchain.cpp to reach the live device.
VulkanDevice* device_from(Device d) noexcept;

// VkResult -> engine error, translated once at the backend boundary.
GfxError to_error(VkResult r) noexcept;

}  // namespace nme::gdi::vk

#endif  // NME_GDI_BACKEND_VULKAN_VK_COMMON_H_
