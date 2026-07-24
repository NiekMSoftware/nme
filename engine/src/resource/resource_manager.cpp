//
// Created by niek on 7/24/2026.
//

#include "nme/resource/resource_manager.h"

#include "nme/resource/package.h"

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
            for (usize j = i + 1; j < dynamic_array_size(&m->m_mounts); ++j)
                m->m_mounts[j - 1] = m->m_mounts[j];
            dynamic_array_pop(&m->m_mounts);
            return;
        }
    }
}

// --- internal helpers ---
namespace {

ResourceSlot* find_live(ResourceManager* m, const StringId id, u32* out_index) {
    const u32* row = hash_map_find(&m->m_index, id);
    if (!row) return nullptr;
    *out_index = *row;
    ResourceSlot* s = &m->m_slots[*row];
    NME_ASSERT(s->ref_count > 0 && s->id == id);
    return s;
}

ResourceSlot* resolve(ResourceManager* m, const ResourceHandle h) {
    if (!resource_handle_valid(h))                  return nullptr;
    if (h.index >= dynamic_array_size(&m->m_slots)) return nullptr;
    ResourceSlot* s = &m->m_slots[h.index];
    return (s->generation == h.generation) ? s : nullptr;   // stale otherwise
}

u32 bump_generation(u32 g) { ++g; return g == 0 ? 1 : g; }  // skip invalid sentinel

Result<fs::FileBlob, ResourceError>
resolve_bytes(ResourceManager* m, const StringId id, const char* path, u16* io_type) {
    for (usize i = dynamic_array_size(&m->m_mounts); i-- > 0;) {
        Package* pkg = m->m_mounts[i];
        if (const PackEntry* e = package_find(pkg, id)) {
            *io_type = e->type;
            auto br = package_read_entry(pkg, e, &m->m_alloc);
            if (result_is_ok(&br))
                return result_ok<fs::FileBlob, ResourceError>(result_value(&br));
            return result_err<fs::FileBlob, ResourceError>(ResourceError::FileError);
        }
    }

    if (path) {
        auto br = fs::file_read_entire(path, &m->m_alloc);
        if (result_is_ok(&br))
            return result_ok<fs::FileBlob, ResourceError>(result_value(&br));
        return result_err<fs::FileBlob, ResourceError>(ResourceError::FileError);
    }
    return result_err<fs::FileBlob, ResourceError>(ResourceError::NotFound);
}

// Shared load path once we have an id + byte source + resolved type.
Result<ResourceHandle, ResourceError>
load_new(ResourceManager* m, const StringId id, const char* path, u16 type) {
    if (!m->m_loaderSet[type])
        return result_err<ResourceHandle, ResourceError>(ResourceError::NoLoader);

    auto bytes = resolve_bytes(m, id, path, &type);
    if (result_is_err(&bytes)) return result_err<ResourceHandle, ResourceError>(result_error(&bytes));

    // A package hit may have changed 'type'; make sure that loader exists too.
    if (!m->m_loaderSet[type]) {
        fs::FileBlob b = result_value(&bytes);
        fs::file_blob_free(&b, &m->m_alloc);
        return result_err<ResourceHandle, ResourceError>(ResourceError::NoLoader);
    }

    fs::FileBlob blob = result_value(&bytes);
    void* asset = m->m_loaders[type].load(&blob, &m->m_alloc);
    fs::file_blob_free(&blob, &m->m_alloc);     // raw bytes are transient
    if (!asset) return result_err<ResourceHandle, ResourceError>(ResourceError::LoadFailed);

    u32 slot;
    if (!dynamic_array_empty(&m->m_free)) {
        slot = m->m_free[dynamic_array_size(&m->m_free) - 1];
        dynamic_array_pop(&m->m_free);
    } else {
        slot = static_cast<u32>(dynamic_array_size(&m->m_slots));
        constexpr ResourceSlot fresh{};
        dynamic_array_push(&m->m_slots, fresh);
    }

    ResourceSlot* s = &m->m_slots[slot];
    s->id         = id;
    s->asset      = asset;
    s->ref_count  = 1;
    s->type       = type;
    s->state      = ResourceState::Ready;
    s->generation = bump_generation(s->generation);
    hash_map_insert(&m->m_index, id, slot);     // now findable by id
    return result_ok<ResourceHandle, ResourceError>(ResourceHandle{slot, s->generation});
}

}  // namespace

// --- api ---

ResourceHandle resource_lookup(ResourceManager* m, const StringId id) {
    u32 idx;
    const ResourceSlot* s = find_live(m, id, &idx);
    if (s != nullptr) return ResourceHandle{idx, s->generation};
    return kInvalidResourceHandle;
}

Result<ResourceHandle, ResourceError>
resource_acquire(ResourceManager* m, const char* path, const u16 type) {
    const StringId id = NME_SID(path);  // register path for debug reverse-lookup
    u32 idx;
    if (ResourceSlot* hit = find_live(m, id, &idx)) {
        ++hit->ref_count;
        return result_ok<ResourceHandle, ResourceError>(ResourceHandle{idx, hit->generation});
    }
    return load_new(m, id, path, type);
}

Result<ResourceHandle, ResourceError> resource_acquire_id(ResourceManager* m, const StringId id) {
    u32 idx;
    if (ResourceSlot* hit = find_live(m, id, &idx)) {
        ++hit->ref_count;
        return result_ok<ResourceHandle, ResourceError>(ResourceHandle{idx, hit->generation});
    }
    return load_new(m, id, nullptr, 0);     // packed-only; type from TOC
}

void resource_release(ResourceManager* m, const ResourceHandle h) {
    ResourceSlot* s = resolve(m, h);
    if (!s) return;
    NME_ASSERT(s->ref_count > 0);
    if (--s->ref_count > 0)
        return;

    const ResourceLoader& L = m->m_loaders[s->type];
    if (L.unload && s->asset) L.unload(s->asset, &m->m_alloc);
    hash_map_remove(&m->m_index, s->id);            // drop from registry
    s->asset = nullptr;
    s->state = ResourceState::Unloaded;
    s->generation = bump_generation(s->generation);
    dynamic_array_push(&m->m_free, h.index);
}

void* resource_get(ResourceManager* m, const ResourceHandle h) {
    ResourceSlot* s = resolve(m, h);
    return (s && s->state == ResourceState::Ready) ? s->asset : nullptr;
}

ResourceState resource_state(ResourceManager* m, const ResourceHandle h) {
    const ResourceSlot* s = resolve(m, h);
    return s ? s->state : ResourceState::Unloaded;
}

}  // namespace nme::res