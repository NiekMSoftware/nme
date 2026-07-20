#ifndef NME_SUBSYSTEM_ERROR_H
#define NME_SUBSYSTEM_ERROR_H
#include "nme/platform/types.h"

namespace nme {

struct SubsystemError {
    enum class Category {
        None = 0,            // success
        Unknown,             // unclassified failure
        OutOfMemory,
        InvalidConfig,       // bad/missing configuration
        DeviceUnavailable,   // required hardware/device not usable
        AlreadyInitialized,  // startup() called while already up
        NotInitialized,      // a required dependency wasn't up
        DependencyFailed,    // a subsystem this one needs failed to start
    };

    Category    category = Category::None;  // the Kernel branches on this
    u32         code     = 0;               // subsystem-private detail; opaque here
    const char* detail   = "";              // static string for logs; never null
};

// --- construction ---

[[nodiscard]] constexpr SubsystemError subsystem_ok() noexcept { return {}; }

[[nodiscard]] constexpr SubsystemError subsystem_error(
        const SubsystemError::Category category,
        const char* const detail,
        const u32 code = 0) noexcept {
    return { category, code, detail };
}
// --- queries ---

[[nodiscard]] constexpr bool subsystem_failed(const SubsystemError e) noexcept {
    return e.category != SubsystemError::Category::None;
}

// Stable label for the coarse category; never null. For the specific reason,
// log e.detail instead.
[[nodiscard]] constexpr const char* subsystem_category_to_str(const SubsystemError::Category c) noexcept {
    switch (c) {
        case SubsystemError::Category::None:               return "None";
        case SubsystemError::Category::Unknown:            return "Unknown";
        case SubsystemError::Category::OutOfMemory:        return "OutOfMemory";
        case SubsystemError::Category::InvalidConfig:      return "InvalidConfig";
        case SubsystemError::Category::DeviceUnavailable:  return "DeviceUnavailable";
        case SubsystemError::Category::AlreadyInitialized: return "AlreadyInitialized";
        case SubsystemError::Category::NotInitialized:     return "NotInitialized";
        case SubsystemError::Category::DependencyFailed:   return "DependencyFailed";
    }
    return "Category(?)";
}

}  // namespace nme

#endif  // NME_SUBSYSTEM_ERROR_H
