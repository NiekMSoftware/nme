#ifndef NME_PLATFORM_FILESYS_FILE_H_
#define NME_PLATFORM_FILESYS_FILE_H_

#include "nme/platform/error/result.h"
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

// --- lifetime ---
Result<File, FileError> file_open (const char* path, FileMode mode);
void                    file_close(File* f);
FileError               file_flush(File* f);

// --- transfer ---
Result<usize, FileError> file_read (File* f, void* dst, usize bytes);
Result<usize, FileError> file_write(File* f, const void* src, usize bytes);

// --- positioning ---
Result<usize, FileError> file_seek(File* f, i64 offset, SeekOrigin origin);
Result<usize, FileError> file_tell(const File* f);
Result<usize, FileError> file_size(const File* f);

// --- path-based queries ---
bool  file_exists (const char* path);
usize file_size_of(const char* path);
bool  file_remove (const char* path);

}  // namespace nme::fs

#endif  // NME_PLATFORM_FILESYS_FILE_H_
