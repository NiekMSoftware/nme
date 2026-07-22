#include "nme/platform/filesys/file.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace nme::fs {

namespace {
FileError from_last_error(const DWORD e) {
    switch (e) {
        case ERROR_FILE_NOT_FOUND:
        case ERROR_PATH_NOT_FOUND:      return FileError::NotFound;
        case ERROR_ACCESS_DENIED:       return FileError::AccessDenied;
        case ERROR_FILE_EXISTS:
        case ERROR_ALREADY_EXISTS:      return FileError::AlreadyExists;
        case ERROR_INVALID_NAME:        return FileError::InvalidPath;
        case ERROR_NOT_ENOUGH_MEMORY:
        case ERROR_OUTOFMEMORY:         return FileError::OutOfMemory;
        default:                        return FileError::Io;
    }
}

HANDLE handle(const File* f) { return static_cast<HANDLE>(f->os.hdl); }
}  // anonymous namespace



}  // namespace nme::fs