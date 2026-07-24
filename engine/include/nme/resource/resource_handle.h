//
// Created by niek on 7/24/2026.
//

#ifndef NME_RESOURCE_RESOURCE_HANDLE_H_
#define NME_RESOURCE_RESOURCE_HANDLE_H_

#include "nme/platform/types.h"

namespace nme::res {

/**
 * @brief Opaque handle to a loaded resource, an index into the manager's slot table
 * plus a generation counter.
 */
struct ResourceHandle {
    u32 index;
    u32 generation;
};

inline constexpr ResourceHandle kInvalidResourceHandle = {0, 0};

inline bool resource_handle_valid(ResourceHandle h) { return h.index != 0; }

inline bool operator==(const ResourceHandle& lhs, const ResourceHandle& rhs) {
    return lhs.index == rhs.index && lhs.generation == rhs.generation;
}
inline bool operator!=(const ResourceHandle& lhs, const ResourceHandle& rhs) {
    return !(lhs == rhs);
}

}  // namespace nme::res

#endif  // NME_RESOURCE_RESOURCE_HANDLE_H_
