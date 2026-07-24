#ifndef NME_RESOURCE_RESOURCE_MANAGER_H_
#define NME_RESOURCE_RESOURCE_MANAGER_H_

#include "nme/core/string/string_id.h"      // GID
#include "nme/platform/collections/dynamic_array.h"
#include "nme/platform/collections/hash_map.h"
#include "nme/platform/memory/allocator.h"
#include "nme/platform/types.h"
#include "nme/resource/resource_loader.h"   // resource loader interface

namespace nme::res {

struct Package;

inline constexpr usize kMaxResourceTypes = 64;

enum class ResourceError : u8 {
    None = 0,
    FileError,
    NoLoader,
    NotFound,
    LoadFailed,
    OutOfMemory
};

enum class ResourceState : u8 {
    Unloaded = 0,
    Loading,        // TODO: add async load path
    Loaded,
    Failed
};

/**
 * @brief Represents a slot for managing a specific resource's data and state within the resource
 * management system.
 */
struct ResourceSlot {
    StringId      id;
    void*         asset;
    u32           ref_count;
    u32           generation;
    ResourceState state;
    u8            _pad[5];
    u16           type;
};

struct ResourceManager {
    Allocator                  m_alloc;
    DynamicArray<ResourceSlot> m_slots;
    HashMap<u32>               m_index;
    DynamicArray<u32>          m_free;
    DynamicArray<Package*>     m_mounts;
    ResourceLoader             m_loaders[kMaxResourceTypes];
    bool                       m_loaderSet[kMaxResourceTypes];
};

}  // namespace nme::res

#endif  // NME_RESOURCE_RESOURCE_MANAGER_H_