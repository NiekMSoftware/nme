#ifndef NME_CORE_CONFIG_FILE_H_
#define NME_CORE_CONFIG_FILE_H_

#include "nme/core/config/cvar.h"
#include "nme/platform/error/result.h"

namespace nme {

enum class ConfigError : u8 {
    FileNotFound
};

Result<u32, ConfigError> config_load_ini(CVarTable* t, const char* path);

}  // namespace nme

#endif  // NME_CORE_CONFIG_FILE_H_
