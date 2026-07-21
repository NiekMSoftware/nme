#include <cstring>  // std::strcmp

#include "nme/platform/collections/dynamic_array.h"
#include "nme/renderer/gdi/gdi.h"
#include "vk_common.h"

namespace nme::gdi {

namespace {

vk::VulkanDevice* g_vk = nullptr;   // the one active device

// Returns true and writes the first graphics-capable queue family index.
bool find_graphics_family(VkPhysicalDevice phys, u32* out_family, const Allocator& alloc) {
    u32 qcount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(phys, &qcount, nullptr);

    DynamicArray<VkQueueFamilyProperties> qprops{};
    dynamic_array_init(&qprops, alloc);
    dynamic_array_reserve(&qprops, qcount);
    vkGetPhysicalDeviceQueueFamilyProperties(phys, &qcount, dynamic_array_data(&qprops));

    const VkQueueFamilyProperties* qp = dynamic_array_data(&qprops);
    bool found = false;
    for (u32 i = 0; i < qcount; ++i) {
        if (qp[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            *out_family = i;
            found       = true;
            return true;
        }
    }

    dynamic_array_destroy(&qprops);
    return found;
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
bool pick_physical(VkInstance instance, VkPhysicalDevice* out_phys, u32* out_family, const Allocator& alloc) {
    u32 count = 0;
    vkEnumeratePhysicalDevices(instance, &count, nullptr);
    if (count == 0) return false;

    DynamicArray<VkPhysicalDevice> devices{};
    dynamic_array_init(&devices, alloc);
    dynamic_array_reserve(&devices, count);
    vkEnumeratePhysicalDevices(instance, &count, dynamic_array_data(&devices));

    const VkPhysicalDevice* devs = dynamic_array_data(&devices);

    VkPhysicalDevice best        = VK_NULL_HANDLE;
    u32              best_family = 0;
    i32              best_score  = -1;

    for (u32 i = 0; i < count; ++i) {
        u32 family = 0;
        if (!find_graphics_family(devs[i], &family, alloc)) continue;   // no graphics queue -> unusable

        if (const i32 score = score_device(devs[i]); score > best_score) {
            best        = devs[i];
            best_family = family;
            best_score  = score;
        }
    }

    dynamic_array_destroy(&devices);

    if (best == VK_NULL_HANDLE) return false;

    *out_phys = best;
    *out_family = best_family;

    return true;
}

constexpr const char* kValidationLayer = "VK_LAYER_KHRONOS_validation";

bool validation_available(const Allocator& alloc) {
    u32 count = 0;
    vkEnumerateInstanceLayerProperties(&count, nullptr);

    DynamicArray<VkLayerProperties> layers{};
    dynamic_array_init(&layers, alloc);
    dynamic_array_reserve(&layers, count);
    vkEnumerateInstanceLayerProperties(&count, dynamic_array_data(&layers));

    const VkLayerProperties* ls = dynamic_array_data(&layers);
    bool found = false;
    for (u32 i = 0; i < count; ++i) {
        if (std::strcmp(ls[i].layerName, kValidationLayer) == 0) {
            found = true;
            return true;
        }
    }

    dynamic_array_destroy(&layers);
    return found;
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

GfxResult<Device> create_device(const DeviceDesc* desc, const Allocator& alloc) {
    const auto fail = [](const GfxError e) { return result_err<Device, GfxError>(e); };

    auto* vd = new vk::VulkanDevice{};
    vd->validation = desc && desc->debug && validation_available(alloc);

    // --- instance ---
    VkApplicationInfo app { VK_STRUCTURE_TYPE_APPLICATION_INFO };
    app.pApplicationName = "NME";
    app.apiVersion       = VK_API_VERSION_1_3;

    const char* extensions[4];
    u32 ext_count = 0;
    extensions[ext_count++] = "VK_KHR_surface";
#if defined(_WIN32)
    extensions[ext_count++] = "VK_KHR_win32_surface";
#elif defined (__APPLE__)
    extensions[ext_count++] = "VK_EXT_metal_surface";   // VK_MVK_metal_surface is deprecated
    // TODO(apple): also add VK_KHR_portability_enumeration + set the create flag for MoltenVK.
#elif defined (__linux__)
    extensions[ext_count++] = "VK_KHR_xcb_surface";
#endif
    if (vd->validation) extensions[ext_count++] = "VK_EXT_debug_utils";

    VkInstanceCreateInfo ici { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
    ici.pApplicationInfo = &app;
    ici.enabledExtensionCount = ext_count;
    ici.ppEnabledExtensionNames = extensions;
    if (vd->validation) {
        ici.enabledLayerCount = 1;
        ici.ppEnabledLayerNames = &kValidationLayer;
    }

    if (const VkResult r = vkCreateInstance(&ici, nullptr, &vd->instance); r != VK_SUCCESS) {
        delete vd;
        return fail(vk::to_error(r));
    }

    // TODO: if vd->validation, create a VkDebugUtilsMessengerEXT here so layer
    //       messages route through nme's logger.

    // --- physical device ---
    if (!pick_physical(vd->instance, &vd->physical, &vd->graphics_family, alloc)) {
        vkDestroyInstance(vd->instance, nullptr);
        delete vd;
        return fail(GfxError::BackendUnavailable);
    }

    // --- logical device + graphics queue ---
    constexpr f32 priority = 1.0f;
    VkDeviceQueueCreateInfo qci { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
    qci.queueFamilyIndex = vd->graphics_family;
    qci.queueCount       = 1;
    qci.pQueuePriorities = &priority;

    const char* dev_ext[] = { "VK_KHR_swapchain" };
    VkDeviceCreateInfo dci { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };   // was DEVICE_QUEUE_CREATE_INFO (wrong sType)
    dci.queueCreateInfoCount    = 1;
    dci.pQueueCreateInfos       = &qci;
    dci.enabledExtensionCount   = 1;
    dci.ppEnabledExtensionNames = dev_ext;

    if (const VkResult r = vkCreateDevice(vd->physical, &dci, nullptr, &vd->device); r != VK_SUCCESS) {
        vkDestroyInstance(vd->instance, nullptr);
        delete vd;
        return fail(vk::to_error(r));
    }
    vkGetDeviceQueue(vd->device, vd->graphics_family, 0, &vd->graphics_queue);

    g_vk = vd;
    return result_ok<Device, GfxError>(Device{1});
}

void destroy_device(Device) {
    if (!g_vk) return;
    if (g_vk->device)   vkDestroyDevice(g_vk->device, nullptr);

    // TODO: destroy debug messenger before the instance.

    if (g_vk->instance) vkDestroyInstance(g_vk->instance, nullptr);
    delete g_vk;
    g_vk = nullptr;
}

Backend device_backend(Device) { return Backend::Vulkan; }

void device_wait_idle(Device) {
    if (g_vk && g_vk->device)
        vkDeviceWaitIdle(g_vk->device);
}

// --- resources ---
// TODO: Each needs a VkBuffer/VkImage/VkShaderModule/VkPipeline created here and an
// id -> object pool entry so the handle can resolve back. Stubbed for now.

GfxResult<Buffer>   create_buffer(Device, const BufferDesc*)     { return result_err<Buffer, GfxError>(GfxError::Unknown); }
GfxResult<Texture>  create_texture(Device, const TextureDesc*)  { return result_err<Texture, GfxError>(GfxError::Unknown); }
GfxResult<Shader>   create_shader(Device, const ShaderDesc*)    { return result_err<Shader, GfxError>(GfxError::Unknown); }
GfxResult<Pipeline> create_pipeline(Device, const PipelineDesc*){ return result_err<Pipeline, GfxError>(GfxError::Unknown); }

void destroy_buffer(Device, Buffer)     {}   // TODO
void destroy_texture(Device, Texture)   {}   // TODO
void destroy_shader(Device, Shader)     {}   // TODO
void destroy_pipeline(Device, Pipeline) {}   // TODO

GfxError write_buffer(Device, Buffer, const void*, u64, u64) { return GfxError::Unknown; }  // TODO

// --- frame + commands ---
// TODO: Needs a VkCommandPool + per-frame VkCommandBuffers, and frame-in-flight
// fences. cmd_* record into the active VkCommandBuffer.

void        begin_frame(Device) {}                        // TODO
CommandList acquire_command_list(Device) { return {}; }   // TODO
void        submit(Device, CommandList) {}                // TODO: vkQueueSubmit
void        end_frame(Device) {}                          // TODO

void cmd_begin(CommandList) {}                                          // TODO: vkBeginCommandBuffer
void cmd_end(CommandList) {}                                            // TODO: vkEndCommandBuffer
void cmd_begin_render_pass(CommandList, const RenderPassDesc*) {}       // TODO: vkCmdBeginRendering
void cmd_end_render_pass(CommandList) {}                                // TODO: vkCmdEndRendering
void cmd_set_viewport(CommandList, const Viewport*) {}                  // TODO: vkCmdSetViewport
void cmd_set_scissor(CommandList, const Rect2D*) {}                     // TODO: vkCmdSetScissor
void cmd_bind_pipeline(CommandList, Pipeline) {}                        // TODO: vkCmdBindPipeline
void cmd_push_constants(CommandList, const void*, u32) {}               // TODO: vkCmdPushConstants
void cmd_bind_texture(CommandList, u32, Texture) {}                     // TODO: descriptor set bind
void cmd_bind_vertex_buffer(CommandList, u32, Buffer, u64) {}           // TODO: vkCmdBindVertexBuffers
void cmd_bind_index_buffer(CommandList, Buffer, IndexType, u64) {}      // TODO: vkCmdBindIndexBuffer
void cmd_draw(CommandList, u32, u32) {}                                 // TODO: vkCmdDraw
void cmd_draw_indexed(CommandList, u32, u32, i32) {}                    // TODO: vkCmdDrawIndexed

}  // namespace nme::gdi