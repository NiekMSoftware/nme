#include "nme/core/string/string_id.h"

// The entire table is debug only

#if NME_DEBUG

#include <cstring>
#include <unordered_map>    // TODO: replace with a proper hash map/table from the collections

#include "nme/core/assert/assert.h"

namespace nme {

namespace {

std::unordered_map<u64, const char*>& table() {
    static std::unordered_map<u64, const char*> table;
    return table;
}

void reg(StringId id, const char* str) {
    auto& t = table();
    if (const auto it = t.find(id.value); it != t.end()) {
        NME_ASSERT(std::strcmp(it->second, str) == 0);
        return;
    }

    // Copy
    const usize size = std::strlen(str) + 1;
    auto copy = new char[size];
    std::memcpy(copy, str, size);
    t.emplace(id.value, copy);
}

}  // anonymous namespace

StringId intern(const char* str) {
    const StringId id{fnv1a_64(str)};
    reg(id, str);
    return id;
}

StringId register_sid(const StringId id, const char* str) {
    reg(id, str);
    return id;
}

const char* sid_to_str(const StringId id) {
    const auto& t = table();
    const auto it = t.find(id.value);
    return (it == t.end()) ? nullptr : it->second;
}

}  // namespace nme

#endif  // NME_DEBUG
