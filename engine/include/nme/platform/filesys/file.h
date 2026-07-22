#ifndef NME_PLATFORM_FILE_SYS_FILE_H_
#define NME_PLATFORM_FILE_SYS_FILE_H_

#include "nme/platform/types.h"

namespace nme::fs {

enum class FileError : u8 {
    None        = 0,
    NotFound,
    AccessDenied,
    AlreadyExists,
    InvalidPath,
    IsDirectory,
    Io,
    OutOfMemory,
    Unknown
};

enum class FileMode : u8 {
    Read,       // open existing, read-only
    Write,      // create or truncate, write-only
    Append,     // create if needed, seek to end
    ReadWrite   // open existing, read + write
};

enum class SeekOrigin : u8 {
    Begin,
    Current,
    End
};

struct File {
    union {
        void* hdl;  // Win32 HANDLE
        i64   fd;   // POSIX file descriptor
    } os;
    bool is_open;
};

}  // namespace nme::fs

#endif  // NME_PLATFORM_FILE_SYS_FILE_H_
