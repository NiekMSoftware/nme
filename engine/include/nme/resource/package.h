#ifndef NME_RESOURCE_PACKAGE_H_
#define NME_RESOURCE_PACKAGE_H_

#include "nme/core/string/string_id.h"
#include "nme/platform/collections/dynamic_array.h"
#include "nme/platform/collections/hash_map.h"
#include "nme/platform/filesys/file.h"
#include "nme/platform/filesys/read_file.h"
#include "nme/platform/types.h"

namespace nme::res {

inline constexpr u32 kPackMagic = 0x4b41504e;   // 'NPAK', little-endian
inline constexpr u32 kPackVersion = 1;

enum class PackSection : u8 {
    Immediate = 0,      // read full on load (small assets, headers)
    Bulk,               // large, aligned, later mmap/stream-able (pixels, audio)
    Temp,               // needed during load-init only
    Debug               // omitted from ship builds
};

enum class PackCompression : u8 {
    None = 0,
    LZ4,
};

struct PackEntry {
    StringId        id;
    u64             offset;         // relative to the section's base offset
    u64             stored;         // bytes on disk (compressed)
    u64             raw;            // bytes once decompressed (== stored if none)
    u16             type;           // resource loader tag
    PackSection     section;
    PackCompression compression;
    u8              _pad[4];
};

// On-disk header, written by the packer.
struct PackHeader {
    u32 magic;
    u32 version;
    u32 flags;              // bit0: big-endian payload
    u32 entry_count;
    u64 toc_offset;         // PackEntry[entry_count]
    u64 immediate_base;     // start of the immediate region
    u64 bulk_base;          // start of the bulk region
};

// A mounted archive: header + TOC held in RAM, file kept open for payload reads.
struct Package {
    fs::File                m_file;
    DynamicArray<PackEntry> m_toc;      // rows, in packer order
    HashMap<u32>            m_index;    // id -> row in m_toc
    u64                     m_immediateBase;
    u64                     m_bulkBase;
    Allocator               m_alloc;
    bool                    bOpen;
};

Result<Package, fs::FileError> package_mount  (const char* path, const Allocator& a);
void                           package_unmount(Package* p);

const PackEntry* package_find(const Package* p, StringId id);

Result<fs::FileBlob, fs::FileError>
package_read_entry(Package* p, const PackEntry* e, const Allocator* a);

}  // namespace nme::res

#endif  // NME_RESOURCE_PACKAGE_H_
