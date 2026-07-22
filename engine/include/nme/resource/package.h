#ifndef NME_RESOURCE_PACKAGE_H_
#define NME_RESOURCE_PACKAGE_H_
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

struct PackEntry { };

struct PackHeader { };

struct Package { };

}  // namespace nme::res

#endif  // NME_RESOURCE_PACKAGE_H_
