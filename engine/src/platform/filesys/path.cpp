#include "nme/platform/filesys/path.h"

namespace nme::fs {

namespace {

bool is_sep  (const char c) { return c == '/' || c == '\\'; }
bool is_alpha(const char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }

usize drive_len(const char* p) {
    return (is_alpha(p[0]) && p[1] == ':') ? usize{2} : usize{0};
}

StrView empty() { return StrView{nullptr, 0}; }

}  // anonymous namespace

bool path_is_absolute(const char* path) {
    if (!path || !path[0]) return false;
    if (is_sep(path[0])) return true;       // "/x" or "\\host\share"
    const usize d = drive_len(path);
    return d && is_sep(path[d]);            // "C:\x"
}

bool path_normalize(Path* out, const char* in) {

}

bool path_join(Path* out, const char* a, const char* b) {}

StrView path_filename(const char* path) {}

StrView path_extension(const char* path) {}

StrView path_stem(const char* path) {}

bool path_is_absolute(const char* path) {}

}  // namespace nme::fs