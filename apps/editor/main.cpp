#include "project.h"

#include "nme/platform/filesys/dir.h"
#include "nme/platform/filesys/path.h"
#include "nme/platform/types.h"

#include <cstdio>
#include <cstdlib>

namespace {

constexpr nme::usize kMaxProjects = 64;

bool read_index(nme::usize* out) {
    char line[64];
    if (!std::fgets(line, sizeof(line), stdin)) return false;
    char* end = nullptr;
    const long v = std::strtol(line, &end, 10);
    if (end == line || v < 0) return false;
    *out = static_cast<nme::usize>(v);
    return true;
}

}  // namespace

int main(const int argc, char** argv) {
    using namespace nme;

    const char* project_dir = argc > 1 ? argv[1] : "projects";

    if (!fs::dir_exists(project_dir))
        fs::dir_create(project_dir);

    editor::ProjectInfo projects[kMaxProjects];
    usize n = editor::project_scan(project_dir, projects, kMaxProjects);

    if (n == 0) {
        std::printf("No projects in '%s'. Creating a sample...\n", project_dir);
        fs::Path sample{};
        if (fs::path_join(&sample, project_dir, "Sample")
            && editor::project_create(sample.data)) {
            n = editor::project_scan(project_dir, projects, kMaxProjects);
        }
    }

    if (n == 0) {
        std::printf("No projects available and none could be created.\n");
        return 1;
    }

    std::printf("\nProjects in '%s':\n", project_dir);
    for (usize i = 0; i < n; ++i) {
        std::printf("  [%zu] %.*s\n", i, static_cast<int>(projects[i].name.len),
                    projects[i].name.data);
    }
    std::printf("\nSelect a project [0-%zu]: ", n - 1);
    std::fflush(stdout);

    usize choice = 0;
    if (!read_index(&choice) || choice >= n) {
        std::printf("No valid selection.\n");
        return 1;
    }

    editor::ContentRoot root{};
    if (!editor::project_open(&root, projects[choice].root.data)) {
        std::printf("Failed to open project.\n");
        return 1;
    }

    std::printf("\nOpened content root: %s\n", root.root.data);

    // From here the real editor takes over: mount `root`, walk its asset tree,
    // bring up the UI. For now, project discovery + open works end-to-end.
    return 0;
}