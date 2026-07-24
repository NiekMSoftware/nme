#ifndef NME_RESOURCE_RESOURCE_LOADER_H_
#define NME_RESOURCE_RESOURCE_LOADER_H_

#include "nme/platform/filesys/read_file.h"

namespace nme::res {

// Represents a loader for managing resources in memory, providing mechanisms to
// load and unload resources using custom logic defined by function pointers.
struct ResourceLoader {
    // Function pointer used for loading resources from a FileBlob into memory.
    void* (*load)(const fs::FileBlob* blob, const Allocator* a);
    // Function pointer to unload a resource from memory.
    void* (*unload)(void* asset, const Allocator* a);
};

}

#endif  // NME_RESOURCE_RESOURCE_LOADER_H_
