//
// Created by niek on 7/24/2026.
//

#include "nme/resource/resource_manager.h"

namespace nme::res {

void resource_manager_init(ResourceManager* m, const Allocator& alloc) {
    m->m_alloc = alloc;
    dynamic_array_init(&m->m_slots, alloc);
    dynamic_array_init(&m->m_free, alloc);
    dynamic_array_init(&m->m_mounts, alloc);
    hash_map_init(&m->m_index, alloc, 256);
    for (usize i = 0; i < kMaxResourceTypes; ++i) m->m_loaderSet[i] = false;
}

void resource_manager_shutdown(ResourceManager* m) {
    // unload anything still live so no asset outlives the manager, then drop the tables.
    // mounts are owned by the caller, we just forget them here
    for (usize i = 0; i < dynamic_array_size(&m->m_slots); ++i) {
        const ResourceSlot* s = &m->m_slots[i];
        if (s->state == ResourceState::Ready && s->asset) {
            if (const ResourceLoader& L = m->m_loaders[s->type]; L.unload)
                L.unload(s->asset, &m->m_alloc);
        }
    }
    hash_map_destroy(&m->m_index);
    dynamic_array_destroy(&m->m_slots);
    dynamic_array_destroy(&m->m_free);
    dynamic_array_destroy(&m->m_mounts);
}

void resource_register_loader(ResourceManager* m, const u16 type, const ResourceLoader loader) {
    NME_ASSERT(type < kMaxResourceTypes);
    m->m_loaders[type]   = loader;
    m->m_loaderSet[type] = true;
}

void resource_mount(ResourceManager* m, Package* pkg) {
    dynamic_array_push(&m->m_mounts, pkg);  // appended -> highest priority
}

void resource_unmount(ResourceManager* m, const Package* pkg) {
    for (usize i = 0; i < dynamic_array_size(&m->m_mounts); ++i) {
        if (m->m_mounts[i] == pkg) {
            // erase-swap would reorder priority, shift instead to preserve it.
            for (usize j = i + i; j < dynamic_array_size(&m->m_mounts); ++j)
                m->m_mounts[j - 1] = m->m_mounts[j];
            dynamic_array_pop(&m->m_mounts);
            return;
        }
    }
}

}  // namespace nme::res