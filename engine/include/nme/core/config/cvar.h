//
// Created by niek on 7/20/2026.
//

#ifndef NME_CORE_CONFIG_CVAR_H_
#define NME_CORE_CONFIG_CVAR_H_

#include "nme/core/string/string_id.h"
#include "nme/platform/platform.h"
#include "nme/platform/types.h"

namespace nme {

enum class CVarType : u8 {
    Bool,
    Int,
    Float,
    String,
};

inline constexpr u32 kMaxCVars      = 256;
inline constexpr u32 kCVarStringMax = 64;

struct CVar {
    StringId name;     // hashed key
    CVarType type;
    union {
        bool asBool;
        i32  asInt;
        f32  asFloat;
        char asString[kCVarStringMax];
    };
#if NME_DEBUG
    const char* debugName;
#endif
};

struct CVarTable {
    CVar vars[kMaxCVars];
    u32  count;
};

void  cvar_table_init(CVarTable* t);
CVar* cvar_table_find(CVarTable* t, StringId name);

CVar* cvar_reg_bool  (CVarTable* t, StringId name, bool        def, const char* dbg);
CVar* cvar_reg_int   (CVarTable* t, StringId name, i32         def, const char* dbg);
CVar* cvar_reg_float (CVarTable* t, StringId name, f32         def, const char* dbg);
CVar* cvar_reg_string(CVarTable* t, StringId name, const char* def, const char* dbg);

bool        cvar_get_bool  (CVarTable* t, StringId name, bool        fb);
i32         cvar_get_int   (CVarTable* t, StringId name, i32         fb);
f32         cvar_get_float (CVarTable* t, StringId name, f32         fb);
const char* cvar_get_string(CVarTable* t, StringId name, const char* fb);

}  // namespace nme



#endif  // NME_CORE_CONFIG_CVAR_H_
