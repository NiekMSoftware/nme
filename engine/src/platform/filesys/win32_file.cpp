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

Result<File, FileError> file_open(const char* path, const FileMode mode) {
    DWORD access = 0, disp = OPEN_EXISTING;
    constexpr DWORD share = FILE_SHARE_READ;
    switch (mode) {
        case FileMode::Read: {
            access = GENERIC_READ;
            disp   = OPEN_EXISTING;
            break;
        }
        case FileMode::Write: {
            access = GENERIC_WRITE;
            disp   = CREATE_ALWAYS;
            break;
        }
        case FileMode::Append: {
            access = FILE_APPEND_DATA;
            disp   = OPEN_ALWAYS;
            break;
        }
        case FileMode::ReadWrite: {
            access = GENERIC_READ | GENERIC_WRITE;
            disp   = OPEN_EXISTING;
        }
    }

    const HANDLE h = ::CreateFileA(path, access, share, nullptr, disp,
                FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE)
        return result_err<File, FileError>(from_last_error(::GetLastError()));

    if (mode == FileMode::Append)
        ::SetFilePointer(h, 0, nullptr, FILE_END);

    File f{};
    f.os.hdl  = h;
    f.is_open = true;
    return result_ok<File, FileError>(f);
}

void file_close(File* f) {
    if (f->is_open) {
        ::CloseHandle(handle(f));
        f->is_open = false;
        f->os.hdl  = nullptr;
    }
}

FileError file_flush(File* f) {
    if (!f || !f->is_open) return FileError::Io;
    return ::FlushFileBuffers(handle(f)) ? FileError::None : from_last_error(::GetLastError());
}

Result<usize, FileError> file_read(File* f, void* dst, const usize bytes) {
    if (!f || !f->is_open) return result_err<usize, FileError>(FileError::Io);
    DWORD got = 0;
    if (!::ReadFile(handle(f), dst, static_cast<DWORD>(bytes), &got, nullptr))
        return result_err<usize, FileError>(from_last_error(::GetLastError()));
    return result_ok<usize, FileError>(static_cast<usize>(got));
}

Result<usize, FileError> file_write(File* f, const void* src, const usize bytes) {
    if (!f || !f->is_open) return result_err<usize, FileError>(FileError::Io);
    DWORD put = 0;
    if (!::WriteFile(handle(f), src, static_cast<DWORD>(bytes), &put, nullptr))
        return result_err<usize, FileError>(from_last_error(::GetLastError()));
    return result_ok<usize, FileError>(static_cast<usize>(put));
}

Result<usize, FileError> file_seek(File* f, const i64 offset, SeekOrigin origin) {
    if (!f || !f->is_open) return result_err<usize, FileError>(FileError::Io);
    DWORD method = FILE_BEGIN;
    switch (origin) {
        case SeekOrigin::Begin: {
            method = FILE_BEGIN;
            break;
        }
        case SeekOrigin::Current: {
            method = FILE_CURRENT;
            break;
        }
        case SeekOrigin::End: {
            method = FILE_END;
            break;
        }
    }
    LARGE_INTEGER dist; dist.QuadPart = offset;
    LARGE_INTEGER now;
    if (!::SetFilePointerEx(handle(f), dist, &now, method))
        return result_err<usize, FileError>(from_last_error(::GetLastError()));
    return result_ok<usize, FileError>(static_cast<usize>(now.QuadPart));
}

Result<usize, FileError> file_tell(const File* f) {
    if (!f || !f->is_open) return result_err<usize, FileError>(FileError::Io);
    LARGE_INTEGER zero; zero.QuadPart = 0;
    LARGE_INTEGER now;
    if (!::SetFilePointerEx(handle(f), zero, &now, FILE_CURRENT))
        return result_err<usize, FileError>(from_last_error(::GetLastError()));
    return result_ok<usize, FileError>(static_cast<usize>(now.QuadPart));
}

Result<usize, FileError> file_size(const File* f) {
    if (!f || !f->is_open) return result_err<usize, FileError>(FileError::Io);
    LARGE_INTEGER sz;
    if (!::GetFileSizeEx(handle(f), &sz))
        return result_err<usize, FileError>(from_last_error(::GetLastError()));
    return result_ok<usize, FileError>(static_cast<usize>(sz.QuadPart));
}

bool file_exists(const char* path) {
    return ::GetFileAttributesA(path) != INVALID_FILE_ATTRIBUTES;
}

bool file_remove(const char* path) {
    return ::DeleteFileA(path) != 0;
}

usize file_size_of(const char* path) {
    WIN32_FILE_ATTRIBUTE_DATA d;
    if (!::GetFileAttributesExA(path, GetFileExInfoStandard, &d)) return 0;
    LARGE_INTEGER sz;
    sz.HighPart = static_cast<LONG>(d.nFileSizeHigh);
    sz.LowPart = d.nFileSizeLow;
    return static_cast<usize>(sz.QuadPart);
}

}  // namespace nme::fs