#include "nme/platform/filesys/dir.h"

#include "nme/platform/error/result.h"
#include "nme/platform/filesys/file.h"   // FileError
#include "nme/platform/filesys/path.h"   // StrView, kMaxPath
#include "nme/platform/types.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <cstring>

namespace nme::fs {

namespace {

// UTF-8 -> UTF-16. `cap` is the wide-char capacity of `out` (incl. the null).
bool utf8_to_wide(const char* in, wchar_t* out, const int cap) {
    if (!in || !out || cap <= 0) return false;
    return MultiByteToWideChar(CP_UTF8, 0, in, -1, out, cap) > 0;
}

// UTF-16 -> UTF-8. `cap` is the byte capacity of `out` (incl. the null).
bool wide_to_utf8(const wchar_t* in, char* out, const int cap) {
    if (!in || !out || cap <= 0) return false;
    return WideCharToMultiByte(CP_UTF8, 0, in, -1, out, cap, nullptr, nullptr) > 0;
}

EntryKind kind_from_attrs(const DWORD attrs) {
    return (attrs & FILE_ATTRIBUTE_DIRECTORY) != 0 ? EntryKind::Directory
                                                   : EntryKind::File;
}

bool is_dot_or_dotdot(const char* s) {
    return s[0] == '.' && (s[1] == '\0' || (s[1] == '.' && s[2] == '\0'));
}

FileError error_from_last(const DWORD e) {
    switch (e) {
        case ERROR_FILE_NOT_FOUND:
        case ERROR_PATH_NOT_FOUND:     return FileError::NotFound;
        case ERROR_ACCESS_DENIED:      return FileError::AccessDenied;
        case ERROR_ALREADY_EXISTS:     return FileError::AlreadyExists;
        case ERROR_NOT_ENOUGH_MEMORY:
        case ERROR_OUTOFMEMORY:        return FileError::OutOfMemory;
        default:                       return FileError::Io;
    }
}

}  // namespace

Result<Dir, FileError> dir_open(const char* path) {
    Dir d{};
    d.handle  = INVALID_HANDLE_VALUE;
    d.is_open = false;
    d.primed  = false;

    if (!path || !*path) {
        return result_err<Dir, FileError>(FileError::InvalidPath);
    }

    wchar_t wpath[kMaxPath];
    if (!utf8_to_wide(path, wpath, static_cast<int>(kMaxPath))) {
        return result_err<Dir, FileError>(FileError::InvalidPath);
    }

    const DWORD attrs = GetFileAttributesW(wpath);
    if (attrs == INVALID_FILE_ATTRIBUTES) {
        return result_err<Dir, FileError>(error_from_last(GetLastError()));
    }
    if ((attrs & FILE_ATTRIBUTE_DIRECTORY) == 0) {
        return result_err<Dir, FileError>(FileError::InvalidPath);  // it's a file, not a dir
    }

    // Stash the directory path in scratch; the first dir_next builds the search
    // pattern from it, after which scratch is reused to stage entry names.
    const usize len = std::strlen(path);
    if (len >= kMaxPath) {
        return result_err<Dir, FileError>(FileError::InvalidPath);
    }
    std::memcpy(d.scratch, path, len + 1);

    d.is_open = true;
    return result_ok<Dir, FileError>(d);
}

bool dir_next(Dir* d, DirEntry* out) {
    if (!d || !out || !d->is_open) return false;

    for (;;) {
        WIN32_FIND_DATAW fd{};

        if (!d->primed) {
            // First call: open the search handle from the path held in scratch.
            wchar_t pattern[kMaxPath + 2];
            if (!utf8_to_wide(d->scratch, pattern, static_cast<int>(kMaxPath))) {
                d->is_open = false;
                return false;
            }
            usize wl = 0;
            while (pattern[wl] != L'\0') ++wl;
            if (wl > 0 && pattern[wl - 1] != L'\\' && pattern[wl - 1] != L'/') {
                pattern[wl++] = L'\\';
            }
            pattern[wl++] = L'*';
            pattern[wl]   = L'\0';

            const HANDLE h = FindFirstFileW(pattern, &fd);
            if (h == INVALID_HANDLE_VALUE) {
                d->is_open = false;
                return false;  // empty directory, or it vanished under us
            }
            d->handle = h;
            d->primed = true;  // "search handle is open"; fd holds entry 0
        } else {
            if (!FindNextFileW(static_cast<HANDLE>(d->handle), &fd)) {
                return false;  // ERROR_NO_MORE_FILES (or a read error)
            }
        }

        if (!wide_to_utf8(fd.cFileName, d->scratch, static_cast<int>(kMaxPath))) {
            continue;  // unrepresentable name (shouldn't happen for UTF-8)
        }
        if (is_dot_or_dotdot(d->scratch)) {
            continue;
        }

        out->name = StrView{d->scratch, std::strlen(d->scratch)};
        out->kind = kind_from_attrs(fd.dwFileAttributes);
        return true;
    }
}

void dir_close(Dir* d) {
    if (!d) return;
    if (d->is_open && d->primed &&
        static_cast<HANDLE>(d->handle) != INVALID_HANDLE_VALUE) {
        FindClose(static_cast<HANDLE>(d->handle));
    }
    d->handle  = INVALID_HANDLE_VALUE;
    d->is_open = false;
    d->primed  = false;
}

bool dir_exists(const char* path) {
    if (!path || !*path) return false;
    wchar_t wpath[kMaxPath];
    if (!utf8_to_wide(path, wpath, static_cast<int>(kMaxPath))) return false;
    const DWORD attrs = GetFileAttributesW(wpath);
    return attrs != INVALID_FILE_ATTRIBUTES &&
           (attrs & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

bool dir_create(const char* path) {
    if (!path || !*path) return false;
    wchar_t wpath[kMaxPath];
    if (!utf8_to_wide(path, wpath, static_cast<int>(kMaxPath))) return false;
    if (CreateDirectoryW(wpath, nullptr)) return true;
    // Idempotent: treat "already there as a directory" as success.
    if (GetLastError() == ERROR_ALREADY_EXISTS) return dir_exists(path);
    return false;
}

}  // namespace nme::fs