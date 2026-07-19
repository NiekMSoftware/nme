#ifndef NME_CORE_STRING_STRING_ID_H
#define NME_CORE_STRING_STRING_ID_H

#include "nme/platform/platform.h"
#include "nme/platform/types.h"

namespace nme {

// --- FNV-1a 64 bit ---

inline constexpr u64 kFnvOffsetBasis64 = 14695981039346656037ull;
inline constexpr u64 kFnvPrime64 = 1099511628211ull;

constexpr u64 fnv1a_64(const char* str, const u64 hash = kFnvOffsetBasis64) noexcept {
    return (*str == '\0')
            ? hash
            : fnv1a_64(str + 1,
                        (hash ^ static_cast<u64>(static_cast<unsigned char>(*str)))
                       * kFnvPrime64);
}

// --- String ID ---
// A hashed string, compare it like an int. It carries no characters at runtime.

struct StringId {
    u64 value = 0;

    StringId() = default;
    explicit constexpr StringId(const u64 v) noexcept : value(v) {}

    // not explicit on purpose, lets StringID be used directly as a 'switch' condition
    // and a 'case' label.
    constexpr operator u64() const noexcept { return value; }
};

constexpr bool operator==(const StringId a, const StringId b) noexcept { return a.value == b.value; }
constexpr bool operator!=(const StringId a, const StringId b) noexcept { return a.value != b.value; }

constexpr StringId make_sid(const char* str) noexcept { return StringId{fnv1a_64(str)}; }

// Compile time construction: StringId id = "player.health"_sid;
// Usable as a 'case' label because it is a converted constant expression.
constexpr StringId operator""_sid(const char* str, usize /*len*/) noexcept { return make_sid(str); }

#if NME_DEBUG

// Runtime hash + register the source string, so it can be recovered later.
StringId intern(const char* str);

// Recover the original string for an id, or nullptr if it was never registered.
const char* sid_to_str(StringId id);

// Register a compile-time id's source text without re-hashing at runtime.
StringId register_sid(StringId id, const char* str);

// Compile-time id AND (in debug) registers its text for reverse lookup.
#define NME_SID(str) ::nme::register_sid(::nme::make_sid(str), (str))

#else   // retail: no strings, no table

#define NME_SID(str) (::nme::make_sid(str))

#endif

}  // namespace nme

#endif  // NME_CORE_STRING_STRING_ID_H
