#include "nme/resource/package.h"

namespace nme::res {

Result<Package, fs::FileError> package_mount(const char* path, Allocator a) {

}

void package_unmount(Package* p) {

}

const PackEntry* package_find(const Package* p, StringId id) {

}

Result<fs::FileBlob, fs::FileError>
package_read_entry(Package* p, const PackEntry* e, const Allocator* a) {

}

}  // namespace nme::res