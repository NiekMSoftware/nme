#include "project.h"

#include <cstdio>

#include "nme/platform/filesys/dir.h"
#include "nme/platform/filesys/file.h"
#include "nme/platform/filesys/path.h"
#include "nme/platform/types.h"

namespace nme::editor {

namespace {

// Placeholder contents for a freshly created marker
constexpr const char kMarkerStub[] = "# nme project\nversion = 1\n";

const char* file_error_name(fs::FileError e) {
    switch (e) {
        case fs::FileError::NotFound:      return "NotFound";
        case fs::FileError::AccessDenied:  return "AccessDenied";
        case fs::FileError::InvalidPath:   return "InvalidPath";
        case fs::FileError::IsDirectory:   return "IsDirectory";
        case fs::FileError::AlreadyExists: return "AlreadyExists";
        case fs::FileError::Io:            return "Io";
        case fs::FileError::OutOfMemory:   return "OutOfMemory";
        default:                           return "Unknown";
    }
}

bool write_marker(const char* dir) {
    fs::Path marker{};
    if (fs::path_join(&marker, dir, kProjectMarker)) return false;

    auto opened = fs::file_open(marker.data, fs::FileMode::Write);
    if (result_is_err(&opened)) {
        std::fprintf(stderr, "file_open('%s', Write) -> %s\n",
                     marker.data, (file_error_name(result_error(&opened))));
        return false;
    }

    fs::File f = result_value(&opened);
    const usize n = sizeof(kMarkerStub) - 1;
    auto wrote = fs::file_write(&f, kMarkerStub, n);
    fs::file_close(&f);

    if (result_is_err(&wrote) || result_value(&wrote) != n) {
        std::fprintf(stderr, "file_write -> short/failed (%zu/%zu)\n",
                     result_is_ok(&wrote) ? result_value(&wrote) : usize{0}, n);
        return false;
    }

    return true;
}

}  // namespace

bool project_is_valid(const char* dir) {
    if (!dir || !*dir) return false;
    fs::Path marker{};
    if (!fs::path_join(&marker, dir, kProjectMarker)) return false;
    return fs::file_exists(marker.data);
}

usize project_scan(const char* projects_dir, ProjectInfo* out, usize cap) {
    if (!projects_dir || !out || cap <= 0) return 0;

    auto opened = fs::dir_open(projects_dir);
    if (result_is_err(&opened)) return 0;

    fs::Dir d = result_value(&opened);
    usize   n = 0;

    fs::DirEntry e{};
    while (n < cap && fs::dir_next(&d, &e)) {
        if (e.kind != fs::EntryKind::Directory) continue;

        fs::Path child{};
        if (!fs::path_join(&child, projects_dir, e.name.data)) continue;
        if (!project_is_valid(child.data)) continue;

        ProjectInfo& info = out[n];
        info.root = child;
        info.name = fs::path_filename(info.root.data);  // points into info.root
        ++n;
    }

    fs::dir_close(&d);
    return n;
}

bool project_open(ContentRoot* cr, const char* dir) {
    if (!cr) return false;
    cr->is_valid = false;

    if (!project_is_valid(dir)) return false;
    if (!fs::path_normalize(&cr->root, dir)) return false;

    cr->is_valid = true;
    return true;
}

bool project_create(const char* dir) {
    if (!dir || !*dir) return false;
    if (!fs::dir_exists(dir)) return false;

    fs::Path marker{};
    if (!fs::path_join(&marker, dir, kProjectMarker)) {
        return false;
    }

    auto opened = fs::file_open(marker.data, fs::FileMode::Write);
    if (result_is_err(&opened)) {
        return false;
    }

    fs::File f = result_value(&opened);
    const usize n = sizeof(kMarkerStub) - 1;
    auto wrote = fs::file_write(&f, kMarkerStub, n);
    fs::file_close(&f);
    std::fprintf(stderr, "file_write -> %s\n", result_is_ok(&wrote) ? "true" : "false");

    return result_is_ok(&wrote) && result_value(&wrote) == n;
}

}  // namespace editor