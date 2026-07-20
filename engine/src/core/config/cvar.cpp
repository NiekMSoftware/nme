#include "nme/core/config/cvar.h"

namespace nme {

void cvar_table_init(CVarTable* t) {}
CVar* cvar_table_find(CVarTable* t, StringId name) {}

CVar* cvar_reg_bool(CVarTable* t, StringId name, bool def, const char* dbg) {}
CVar* cvar_reg_int(CVarTable* t, StringId name, i32 def, const char* dbg) {}
CVar* cvar_reg_float(CVarTable* t, StringId name, f32 def, const char* dbg) {}
CVar* cvar_reg_string(CVarTable* t, StringId name, const char* def, const char* dbg) {}

bool cvar_get_bool(CVarTable* t, StringId name, bool fb) {}
i32 cvar_get_int(CVarTable* t, StringId name, i32 fb) {}
f32 cvar_get_float(CVarTable* t, StringId name, f32 fb) {}
const char* cvar_get_string(CVarTable* t, StringId name, const char* fb) {}

}  // namespace nme
