#include "nme/core/config/config_file.h"
#include "nme/core/string/string_id.h"

#include <fstream>      // TODO: swap for platform/filesys
#include <string>
#include <string_view>
#include <cstdio>

namespace {

std::string_view trim(std::string_view s) {
    const auto ws = " \t\r\n";
    const size_t a = s.find_first_not_of(ws);
    if (a == std::string_view::npos) return {};
    const size_t b = s.find_last_not_of(ws);
    return s.substr(a, b - a + 1);                  // inclusive length
}

}  // anonymous namespace

namespace nme {

Result<u32, ConfigError> config_load_ini(CVarTable* t, const char* path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return result_err<u32, ConfigError>(ConfigError::FileNotFound);
    const std::string text((std::istreambuf_iterator<char>(in)), {});

    std::string_view src = text;
    char section[kCVarStringMax] = {0};
    size_t pos = 0;
    u32 applied = 0;

    while (pos < src.size()) {
        size_t eol = src.find('\n', pos);
        if (eol == std::string_view::npos) eol = src.size();
        std::string_view line = trim(src.substr(pos, eol - pos));
        pos = eol + 1;

        if (line.empty() || line[0]==';' || line[0]=='#') continue;  // ignore comments

        if (line.front()=='[' && line.back()==']') {
            std::string_view s = line.substr(1, line.size() - 2);
            std::snprintf(section, sizeof(section), "%.*s", static_cast<int>(s.size()), s.data());
            continue;
        }

        const size_t eq = line.find('=');
        if (eq == std::string_view::npos) continue;

        std::string_view key = trim(line.substr(0, eq));
        std::string_view val = trim(line.substr(eq + 1));

        const size_t c = val.find_first_of(";#");       // inline comment start
        if (c != std::string_view::npos) val = trim(val.substr(0, c));

        char full[kCVarStringMax], v[kCVarStringMax];
        if (section[0]) std::snprintf(full, sizeof(full), "%s.%.*s", section, static_cast<int>(key.size()), key.data());
        else            std::snprintf(full, sizeof(full), "%.*s", static_cast<int>(key.size()), key.data());
        std::snprintf(v, sizeof(v), "%.*s", static_cast<int>(val.size()), val.data());

        if (cvar_set_from_str(t, make_sid(full), v)) ++applied;
    }

    return result_ok<u32, ConfigError>(applied);
}

}  // namespace nme
