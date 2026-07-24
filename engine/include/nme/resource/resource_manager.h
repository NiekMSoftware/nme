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
    DynamicArray<ResourceSlot> m_slots;     // index -> record
    HashMap<u32>               m_index;     // live id -> slot index (the registry)
    DynamicArray<u32>          m_free;      // reusable slot indices
    DynamicArray<Package*>     m_mounts;    // search order: back = highest priority
    ResourceLoader             m_loaders[kMaxResourceTypes];
    bool                       m_loaderSet[kMaxResourceTypes];
};

// --- lifetime ---
void resource_manager_init    (ResourceManager* m, Allocator alloc);
void resource_manager_shutdown(ResourceManager* m);

// --- setup ---
void resource_register_loader(ResourceManager* m, ResourceLoader loader);

void resource_mount  (ResourceManager* m, Package* pkg);
void resource_unmount(ResourceManager* m, Package* pkg);

}  // namespace nme::res

#endif  // NME_RESOURCE_RESOURCE_MANAGER_H_