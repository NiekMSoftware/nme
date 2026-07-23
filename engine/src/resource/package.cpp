#include "nme/resource/package.h"

namespace nme::res {

inline constexpr usize kBulkAlign = 16;

namespace {
usize next_pow2_atleast(const usize n) {
    usize c = 8;
    while (c < n) c <<= 1;
    return c;
}
}

Result<Package, fs::FileError> package_mount(const char* path, Allocator a) {
    auto file_r = fs::file_open(path, fs::FileMode::Read);
    if (result_is_err(&file_r)) return result_err<Package, fs::FileError>(result_error(&file_r));

    Package p{};
    p.m_file = result_value(&file_r);
    p.m_alloc = a;
    dynamic_array_init(&p.m_toc, a);

    PackHeader h{};
    auto hr = fs::file_read(&p.m_file, &h, sizeof(h));
    if (result_is_err(&hr) || result_value(&hr) != sizeof(h)) {
        fs::file_close(&p.m_file);      // wrong format / endian
        return result_err<Package, fs::FileError>(fs::FileError::InvalidPath);
    }

    // pull the whole TOC into RAM in one read
    dynamic_array_reserve(&p.m_toc, h.entry_count);
    if (h.entry_count) {
        fs::file_seek(&p.m_file, static_cast<i64>(h.toc_offset), fs::SeekOrigin::Begin);
        const usize bytes = h.entry_count * sizeof(PackEntry);
        auto tr = fs::file_read(&p.m_file, dynamic_array_data(&p.m_toc), bytes);
        if (result_is_err(&tr) || result_value(&tr) != bytes) {
            fs::file_close(&p.m_file);
            return result_err<Package, fs::FileError>(fs::FileError::Io);
        }
        p.m_toc.m_size = h.entry_count;
    }

    // build the id
    hash_map_init(&p.m_index, a, next_pow2_atleast(h.entry_count * 10 / 7 + 1));
    for (usize i = 0; i < h.entry_count; ++i) {
        NME_ASSERT(p.m_toc[i].id.value != 0 && "packer emitted a null resource id");
        hash_map_insert(&p.m_index, p.m_toc[i].id, static_cast<u32>(i));
    }

    p.m_immediateBase = h.immediate_base;
    p.m_bulkBase      = h.bulk_base;
    p.bOpen           = true;
    return result_ok<Package, fs::FileError>(p);
}

void package_unmount(Package* p) {
    if (!p || !p->bOpen) return;
    hash_map_destroy(&p->m_index);
    dynamic_array_destroy(&p->m_toc);
    fs::file_close(&p->m_file);
    p->bOpen = false;
}

const PackEntry* package_find(const Package* p, const StringId id) {
    const u32* row = hash_map_find(&p->m_index, id);  // O(1) lookup
    return (row) ? &p->m_toc[*row] : nullptr;
}

Result<fs::FileBlob, fs::FileError>
package_read_entry(Package* p, const PackEntry* e, const Allocator* a) {
    NME_ASSERT(p && p->bOpen);
    NME_ASSERT(e->compression == PackCompression::None
        && "wire a codec branch into package_read_entry before packing compressed");

    const u64 base = (e->section == PackSection::Bulk) ? p->m_bulkBase : p->m_immediateBase;
    const usize align = (e->section == PackSection::Bulk) ? kBulkAlign : alignof(u64);

    u8* dst = static_cast<u8*>(alloc(a, e->raw, align));
    if (!dst) return result_err<fs::FileBlob, fs::FileError>(fs::FileError::OutOfMemory);

    fs::file_seek(&p->m_file, static_cast<i64>(base + e->offset), fs::SeekOrigin::Begin);
    auto rr = fs::file_read(&p->m_file, dst, e->stored);    // stored == raw when None
    if (result_is_err(&rr) || result_value(&rr) != e->stored) {
        free(a, dst, e->raw);
        return result_err<fs::FileBlob, fs::FileError>(fs::FileError::Io);
    }
    return result_ok<fs::FileBlob, fs::FileError>(fs::FileBlob{ dst, e->raw });
}

}  // namespace nme::res