#ifndef NME_PLATFORM_FS_READ_FILE_H_
#define NME_PLATFORM_FS_READ_FILE_H_

#include "nme/platform/error/result.h"
#include "nme/platform/filesys/file.h"
#include "nme/platform/types.h"

// forward declare allocator
namespace nme {
struct Allocator;
}  // namespace nme

namespace nme::fs {

// Bytes owned by the Allocator passed to file_read_entire; free via file_blob_free.
struct FileBlob {
    u8*   data;
    usize size;
};

Result<usize, FileError> file_read_into(const char* path, void* dst, usize cap);

Result<FileBlob, FileError> file_read_entire(const char* path, const Allocator* allocator);
void                        file_blob_free(FileBlob* blob, const Allocator* allocator);

}  // namespace nme::fs

#endif  // NME_PLATFORM_FS_READ_FILE_H_
