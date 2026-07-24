#ifndef NME_RESOURCE_RESOURCE_MANAGER_H_
#define NME_RESOURCE_RESOURCE_MANAGER_H_

#include "nme/core/string/string_id.h"  // GID
#include "nme/platform/collections/dynamic_array.h"
#include "nme/platform/collections/hash_map.h"
#include "nme/platform/memory/allocator.h"
#include "nme/platform/types.h"
#include "nme/resource/resource_loader.h"  // resource loader interface
#include "resource_handle.h"

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
    Ready,
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
void resource_manager_init    (ResourceManager* m, const Allocator& alloc);
void resource_manager_shutdown(ResourceManager* m);

// --- setup ---
void resource_register_loader(ResourceManager* m, ResourceLoader loader);

void resource_mount  (ResourceManager* m, Package* pkg);
void resource_unmount(ResourceManager* m, Package* pkg);

/**
 * @brief Load-or-dump by path: resolves the ID across mounts (type comes from TOC),
 * else reads the loose file at 'path' using 'type'. Re-requesting a live id just
 * bumps the refcount and returns the @b same handle (no reload).
 */
Result<ResourceHandle, ResourceError>
resource_acquire(ResourceManager* m, const char* path, u16 type);

/**
 * @brief Load-or-dump by GID alone, packed content only (type reads from TOC).
 * Use this to resolve stored cross-references, where you hold an id but no path.
 */
Result<ResourceHandle, ResourceError>
resource_acquire_id(ResourceManager* m, StringId id);

/**
 * @brief Registry lookup only: resolves already-live GID to its handle,
 * or @c kInvalidResourceHandle if not loaded. Never triggers I/O.
 */
ResourceHandle resource_lookup(ResourceManager* m, StringId id);

/** @brief Decrement; at zero the asset unloads exactly once and the slot invalidates. */
void resource_release(ResourceManager* m, ResourceHandle h);

// --- access ---
void*         resource_get(ResourceManager* m, ResourceHandle h);   // null if stale/not ready
ResourceState resource_state(ResourceManager* m, ResourceHandle h);

}  // namespace nme::res

#endif  // NME_RESOURCE_RESOURCE_MANAGER_H_