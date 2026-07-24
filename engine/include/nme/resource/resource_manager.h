#ifndef NME_RESOURCE_RESOURCE_MANAGER_H_
#define NME_RESOURCE_RESOURCE_MANAGER_H_
#include "nme/core/string/string_id.h"
#include "nme/platform/types.h"

namespace nme::res {

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

}  // namespace nme::res

#endif  // NME_RESOURCE_RESOURCE_MANAGER_H_