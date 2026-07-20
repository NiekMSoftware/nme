#include "nme/core/config/cvar.h"
#include "nme/platform/debug/assert.h"

#include <cstdio>
#include <cstdlib>

namespace nme {

void cvar_table_init(CVarTable* t) { t->count = 0; }
CVar* cvar_table_find(CVarTable* t, const StringId name) {
    for (u32 i = 0; i < t->count; ++i)
        if (t->vars[i].name == name) return &t->vars[i];
    return nullptr;
}

CVar* cvar_reg_bool(CVarTable* t, const StringId name, const bool def, const char* dbg) {
    CVar* c = cvar_table_find(t, name);
    if (!c) {
        NME_ASSERT(t->count < kMaxCVars);
        c = &t->vars[t->count++];
    }
    c->name = name;
    c->type = CVarType::Bool;
    c->asBool = def;

#if NME_DEBUG
    c->debugName = dbg;
#else
    (void)dbg;
#endif

    return c;
}

CVar* cvar_reg_float(CVarTable* t, const StringId name, const f32 def, const char* dbg) {
    CVar* c = cvar_table_find(t, name);
    if (!c) {
        NME_ASSERT(t->count < kMaxCVars);
        c = &t->vars[t->count++];
    }
    c->name = name;
    c->type = CVarType::Float;
    c->asFloat = def;

#if NME_DEBUG
    c->debugName = dbg;
#else
    (void)dbg;
#endif

    return c;
}

CVar* cvar_reg_int(CVarTable* t, const StringId name, const i32 def, const char* dbg) {
    CVar* c = cvar_table_find(t, name);
    if (!c) {
        NME_ASSERT(t->count < kMaxCVars);
        c = &t->vars[t->count++];
    }
    c->name = name;
    c->type = CVarType::Int;
    c->asInt = def;

#if NME_DEBUG
    c->debugName = dbg;
#else
    (void)dbg;
#endif

    return c;
}

CVar* cvar_reg_string(CVarTable* t, const StringId name, const char* def, const char* dbg) {
    CVar* c = cvar_table_find(t, name);
    if (!c) {
        NME_ASSERT(t->count < kMaxCVars);
        c = &t->vars[t->count++];
    }
    c->name = name;
    c->type = CVarType::String;
    std::snprintf(c->asString, kCVarStringMax, "%s", def);  // copy + always null-terminates
#if NME_DEBUG
    c->debugName = dbg;
#else
    (void)dbg;
#endif

    return c;
}

bool cvar_get_bool(CVarTable* t, const StringId name, const bool fb) {
    const CVar* c = cvar_table_find(t, name);
    if ((c && c->type == CVarType::Bool))
        return c->asBool;
    return fb;
}

i32 cvar_get_int(CVarTable* t, const StringId name, const i32 fb) {
    const CVar* c = cvar_table_find(t, name);
    if ((c && c->type == CVarType::Int))
        return c->asInt;
    return fb;
}
f32 cvar_get_float(CVarTable* t, const StringId name, const f32 fb) {
    const CVar* c = cvar_table_find(t, name);
    if ((c && c->type == CVarType::Float))
        return c->asFloat;
    return fb;
}
const char* cvar_get_string(CVarTable* t, const StringId name, const char* fb) {
    const CVar* c = cvar_table_find(t, name);
    if ((c && c->type == CVarType::String))
        return c->asString;
    return fb;
}

bool cvar_set_from_str(CVarTable* t, const StringId name, const char* v) {
    CVar* c = cvar_table_find(t, name);
    if (!c) return false;
    switch (c->type) {
        case CVarType::Bool: {
            c->asBool = (v[0]=='1'||v[0]=='t'||v[0]=='T');
            return true;
        }
        case CVarType::Int: {
            c->asInt = static_cast<i32>(std::strtol(v, nullptr, 10));
            return true;
        }
        case CVarType::Float: {
            c->asFloat = static_cast<f32>(std::strtof(v, nullptr));
            return true;
        }
        case CVarType::String: {
            std::snprintf(c->asString, sizeof(c->asString), "%s", v);
            return true;
        }
    }
    return false;
}

}  // namespace nme
