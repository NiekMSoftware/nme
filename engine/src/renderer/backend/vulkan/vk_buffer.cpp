#include <cstring>  // std::memcpy

#include "nme/platform/gfx/gfx_types.h"
#include "vk_common.h"

namespace nme::gdi {

namespace {

VkBufferUsageFlags to_vk_buffer_usage(const BufferUsage u) noexcept {
    VkBufferUsageFlags f = 0;
    if (any(u & BufferUsage::Vertex))  f |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    if (any(u & BufferUsage::Index))   f |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    if (any(u & BufferUsage::Uniform)) f |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    if (any(u & BufferUsage::Storage)) f |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    if (any(u & BufferUsage::CopySrc)) f |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    if (any(u & BufferUsage::CopyDst)) f |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    return f;
}

}  // anonymous namespace

GfxResult<Buffer> create_buffer(const Device d, const BufferDesc* desc) {
    auto* vd = vk::device_from(d);
    if (!vd || !desc || desc->size == 0) return result_err<Buffer, GfxError>(GfxError::InvalidArgs);

    VkBufferCreateInfo bci { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    bci.size = desc->size;
    bci.usage = to_vk_buffer_usage(desc->usage);
    bci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo aci {};
    aci.usage = VMA_MEMORY_USAGE_AUTO;      // VMA picks the memory type
    switch (desc->access) {
        case MemoryAccess::GpuOnly: break;  // device-local; upload needs staging (TODO)
        case MemoryAccess::CpuToGpu: aci.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT; break;
        case MemoryAccess::GpuToCpu: aci.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;           break;
    }

    VkBuffer      buffer = VK_NULL_HANDLE;
    VmaAllocation alloc  = nullptr;
    if (const VkResult r = vmaCreateBuffer(vd->allocator, &bci, &aci, &buffer, &alloc, nullptr); r != VK_SUCCESS) {
        return result_err<Buffer, GfxError>(vk::to_error(r));
    }

    // Park it in the pool
    u32 index = 0;
    if (!dynamic_array_empty(&vd->buffer_free)) {
        vd->buffer_free[dynamic_array_size(&vd->buffer_free) - 1];
        dynamic_array_pop(&vd->buffer_free);
    } else {
        index = static_cast<u32>(dynamic_array_size(&vd->buffers));
        NME_ASSERT(index <= vk::kHandleIndexMask);
        vk::BufferResource fresh{};
        fresh.generation = 1;
        dynamic_array_push(&vd->buffers, fresh);
    }

    vk::BufferResource* slot = &vd->buffers[index];
    slot->buffer     = buffer;
    slot->allocation = alloc;
    slot->size       = desc->size;
    slot->in_use     = true;
    // slot->generation: 1 for a fresh slot, or the value bumped at free time for a reused one.

    const u32 id = (static_cast<u32>(slot->generation) << vk::kHandleIndexBits) | index;
    return result_ok<Buffer, GfxError>(Buffer{id});
}

void destroy_buffer(const Device d, const Buffer h) {
    auto* vd = vk::device_from(d);
    if (!vd) return;

    vk::BufferResource* r = vk::resolve_buffer(vd, h);
    if (!r) return;

    vmaDestroyBuffer(vd->allocator, r->buffer, r->allocation);
    r->buffer     = VK_NULL_HANDLE;
    r->allocation = nullptr;
    r->in_use     = false;
    if (++r->generation == 0) r->generation = 1;    // invalidate outstanding handles; skip gen 0

    dynamic_array_push(&vd->buffer_free, h.id & vk::kHandleIndexMask);
}

GfxError write_buffer(Device d, Buffer dst, const void* data, const u64 size, const u64 offset) {
    auto* vd = vk::device_from(d);
    if (!vd || !data) return GfxError::InvalidArgs;

    vk::BufferResource* r = vk::resolve_buffer(vd, dst);
    if (!r || offset + size > r->size) return GfxError::InvalidArgs;

    // Mappable path only. A device-local (GpuOnly) buffer isn't host-visible, so
    // uploading it needs a staging buffer + copy on the transfer/command path --
    // deferred until that exists.
    VkMemoryPropertyFlags mem = 0;
    vmaGetAllocationMemoryProperties(vd->allocator, r->allocation, &mem);
    if (!(mem & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT))
        return GfxError::InvalidArgs;   // TODO: staging upload for device-local buffers

    void* mapped = nullptr;
    if (const VkResult res = vmaMapMemory(vd->allocator, r->allocation, &mapped); res != VK_SUCCESS)
        return vk::to_error(res);

    std::memcpy(static_cast<u8*>(mapped) + offset, data, size);
    vmaFlushAllocation(vd->allocator, r->allocation, offset, size);  // no-op when HOST_COHERENT
    vmaUnmapMemory(vd->allocator, r->allocation);

    return GfxError::None;
}

}  // namespace nme::gdi