#ifndef NME_PLATFORM_FILESYS_DIR_H_
#define NME_PLATFORM_FILESYS_DIR_H_

#include "nme/platform/error/result.h"
#include "nme/platform/filesys/file.h"   // FileError
#include "nme/platform/filesys/path.h"   // StrView, kMaxPath
#include "nme/platform/types.h"

namespace nme::fs {

enum class EntryKind : u8 {
    File,
    Directory,
    Other
};

struct DirEntry {
    StrView   name;
    EntryKind kind;
};

struct Dir {
    void* handle;             // Win32 HANDLE, or POSIX DIR*
    char  scratch[kMaxPath];  // staging for the current entry's leaf name
    bool  is_open;
    bool  primed;             // Win32: FindFirstFile already staged entry 0
};

// --- iteration ---

Result<Dir, FileError> dir_open (const char* path);
bool                   dir_next (Dir* d, DirEntry* out);
void                   dir_close(Dir* d);

// --- path-based queries ---
bool dir_exists(const char* path);
bool dir_create(const char* path);  // one level only; parent must already exist

}  // namespace nme::fs

#endif  // NME_PLATFORM_FILESYS_DIR_H_