#ifndef NME_APPS_EDITOR_PROJECT_H_
#define NME_APPS_EDITOR_PROJECT_H_

#include "nme/platform/filesys/path.h"
#include "nme/platform/types.h"

namespace nme::editor {

inline constexpr const char* kProjectMarker = "project.nme";

struct ProjectInfo {
    fs::Path    root;
    fs::StrView name;
};

struct ContentRoot {
    fs::Path root;
    bool     is_valid;
};

bool project_is_valid(const char* dir);
usize project_scan(const char* projects_dir, ProjectInfo* out, usize cap);
bool project_open(ContentRoot* cr, const char* dir);
bool project_create(const char* dir);

}  // namespace nme::editor

#endif  // NME_APPS_EDITOR_PROJECT_H_