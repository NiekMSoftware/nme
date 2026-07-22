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

}  // namespace nme::fs