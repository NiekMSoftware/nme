#include "nme/platform/filesys/path.h"

#include <cstring>

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
    if (!out || !in) return false;

    char  buf[kMaxPath];
    usize n = 0;
    auto  put = [&](const char c) -> bool {
        if (n >= kMaxPath - 1) return false;
        buf[n++] = c;
        return true;
    };

    const char* p = in;

    // preserve an optional drive
    const usize d = drive_len(p);
    for (usize i = 0; i < d; ++i)
        if (!put(*p++)) return false;

    const bool absolute = is_sep(*p);
    if (absolute) {
        if (!put('/'))   return false;
        while (is_sep(*p)) ++p;
    }
    const usize root = n;

    // walk components, remembering each segment's start so ".." can pop.
    usize seg[kMaxPath / 2];
    usize segc = 0;

    while (*p) {
        const char* start = p;
        while (*p && !is_sep(*p)) ++p;
        const auto len = static_cast<usize>(p - start);

        const bool dot    = len == 1 && start[0] == '.';
        const bool dotdot = len == 2 && start[0] == '.' && start[1] == '.';

        if (len == 0 || dot) {
            // "" (duplicate separator) -> nothing to add.
        } else if (dotdot) {
            if (segc > 0) {
                const usize s = seg[--segc];    // pop last segment...
                n = (s > root) ? s - 1 : s;     // ...and it's leading '/'
            } else if (!absolute) {
                if (n > root) if (!put('/')) return false;
                if (!put('.') || !put('.')) return false;
            }
        } else {
            if (n > root) if (!put('/')) return false;
            seg[segc++] = n;
            for (const char* q = start; q < p; ++q)
                if (!put(*q)) return false;
        }

        while (is_sep(*p)) ++p;
    }

    if (n == 0) { if (put('.')) return false; }  // empty -> '.'
    buf[n] = '\0';

    for (usize i = 0; i <= n; ++i) out->data[i] = buf[i];
    out->len = n;
    return true;
}

bool path_join(Path* out, const char* a, const char* b) {
    if (!out || !a || !b) return false;
    if (path_is_absolute(b) || !a[0]) return path_normalize(out, b);
    if (!b[0]) return path_normalize(out, a);

    char tmp[kMaxPath * 2];
    usize n = 0;
    auto  push = [&](const char* s) -> bool {
        while (*s) {
            if (n >= sizeof(tmp) - 2)
                return false;
            tmp[n++] = *s++;
        }
        return true;
    };
    if (!push(a)) return false;
    if (n && !is_sep(tmp[n - 1])) tmp[n++] = '/';
    if (!push(b)) return false;
    tmp[n] = '\0';
    return path_normalize(out, tmp);
}

StrView path_filename(const char* path) {
    if (!path) return empty();
    const char* data = path;

    for (const char* p = path; *p; ++p)
        if (is_sep(*p))
            data = p + 1;

    usize len = 0;
    while (data[len]) ++len;
    return StrView{ data, len };
}

StrView path_extension(const char* path) {
    const auto [data, len] = path_filename(path);
    for (usize i = len; i-- > 0;) {
        if (data[i] == '.') {
            return (i == 0)
                    ? empty()
                    : StrView{ data + i + 1, len - i - 1 };
        }
    }
    return empty();
}

StrView path_stem(const char* path) {
    if (!path) return empty();
    const StrView name = path_filename(path);
    if (name.data == path) return empty();      // no seperator at all
    const usize len = static_cast<usize>(name.data - path) - 1;     // drop trailing '/'
    if (len == 0) return StrView { path, 1 };   // parent of "/x" is "/"
    return StrView{ path, len };
}

}  // namespace nme::fs