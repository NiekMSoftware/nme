#include <nme/core/config/cvar.h>
#include <nme/core/config/config_file.h>
#include <nme/core/jobs/job_system.h>
#include <nme/core/memory/heap_alloc.h>
#include <nme/core/string/string_id.h>
#include <nme/core/subsystem/kernel.h>
#include <nme/core/subsystem/subsystem.h>
#include <nme/core/subsystem/subsystem_error.h>
#include <nme/core/util/scope_guard.h>
#include <nme/platform/filesys/filesys.h>
#include <nme/platform/gfx/gfx.h>
#include <nme/platform/platform.h>
#include <nme/platform/timer/timer.h>
#include <nme/platform/types.h>
#include <nme/renderer/renderer.h>
#include <nme/version.h>

#include <cstdio>
#include <cstdlib>

namespace {

class TimerSubsystem final : public nme::Subsystem {
public:
    [[nodiscard]] nme::SubsystemError startup() override {
        if (!nme::platform::global_timer().startup())
            return nme::subsystem_error(nme::SubsystemError::Category::NotInitialized, "hi-res timer failed to start");
        return nme::subsystem_ok();
    }

    void shutdown() override {
        nme::platform::global_timer().shutdown();
    }

    [[nodiscard]] const char* name() const override { return "Timer"; }
};

// Config subsystem. The file system (platform/filesys) locates, verifies and
// reads the ini; core/config only parses it. That keeps the dependency edge
// pointing down: core/config -> platform/filesys, never the reverse.
//
// Config subsystem configured by a local LLM.
class ConfigSubsystem final : public nme::Subsystem {
public:
    [[nodiscard]] nme::SubsystemError startup() override {
        nme::cvar_table_init(&table_);

        // programmer defaults - the ini overrides these (Gregory's model)
        nme::cvar_reg_float(&table_, NME_SID("physics.gravity"),   -9.81f, "physics.gravity");
        nme::cvar_reg_int  (&table_, NME_SID("window.width"),       1280,  "window.width");
        nme::cvar_reg_int  (&table_, NME_SID("window.height"),       720,  "window.height");
        nme::cvar_reg_bool (&table_, NME_SID("debug.showProfiler"), false, "debug.showProfiler");

        // Resolve the path through the file system so separators are normalized
        // and a bad path is rejected here rather than deep in the parser.
        nme::fs::Path cfg{};
        if (!nme::fs::path_join(&cfg, "config", "engine.ini")) {
            std::puts("  config: invalid path, using defaults");
            return nme::subsystem_ok();
        }

        // Cheap existence check -> clean defaults branch, no error to unpack.
        if (!nme::fs::file_exists(cfg.data)) {
            std::printf("  config: %s not found, using defaults\n", cfg.data);
            return nme::subsystem_ok();
        }

        // Parse values (core/config owns the ini grammar).
        auto r = nme::config_load_ini(&table_, cfg.data);
        if (nme::result_is_err(&r)) {
            std::puts("  config: parse failed, using defaults");
            return nme::subsystem_ok();
        }
        std::printf("  config: applied %u override(s) from %s\n",
                    static_cast<unsigned>(nme::result_value(&r)), cfg.data);

        echo_source(cfg.data);   // debug aid: dump the exact bytes we loaded
        return nme::subsystem_ok();
    }

    void shutdown() override {}

    [[nodiscard]] const char* name() const override { return "Config"; }

    nme::CVarTable* table() { return &table_; }

    void set_allocator(const nme::Allocator& a) { alloc_ = a; }

private:
    // Reads the whole config file through the engine heap and prints it, then
    // hands the bytes straight back. This is the allocator-injected read path
    // (read_file.h) in action; scoped to debug so release builds don't pay for it.
    void echo_source([[maybe_unused]] const char* path) const {
#if NME_DEBUG
        auto rb = nme::fs::file_read_entire(path, &alloc_);
        if (nme::result_is_err(&rb)) return;

        nme::fs::FileBlob blob = nme::result_value(&rb);
        std::printf("  config: source %s (%zu bytes)\n", path, blob.size);
        std::printf("--- %.*s---\n", static_cast<int>(blob.size),
                    reinterpret_cast<const char*>(blob.data));
        nme::fs::file_blob_free(&blob, &alloc_);
#endif
    }

    nme::CVarTable table_{};
    nme::Allocator alloc_{};
};

ConfigSubsystem* g_config;

// Owns the OS window surface (platform/gfx). Wraps a platform resource for
// lifecycle, exactly like TimerSubsystem. MUST start after Config (reads CVars).
class WindowSubsystem final : public nme::Subsystem {
public:
    [[nodiscard]] nme::SubsystemError startup() override {
        nme::CVarTable* cfg = g_config->table();

        nme::gfx::WindowDesc desc{};
        desc.title     = "nme";     // temp title
        desc.extent    = { static_cast<nme::u32>(nme::cvar_get_int(cfg, NME_SID("window.width"),  1280)),
                           static_cast<nme::u32>(nme::cvar_get_int(cfg, NME_SID("window.height"),  720)) };
        desc.resizable = true;

        auto rs = nme::gfx::create_surface(&desc, alloc_);
        if (nme::result_is_err(&rs))
            return nme::subsystem_error(nme::SubsystemError::Category::NotInitialized,
                "failed to create window surface");
        surface_ = nme::result_value(&rs);

        std::printf("  window: %ux%u\n", desc.extent.width, desc.extent.height);
        return nme::subsystem_ok();
    }

    void shutdown() override {
        nme::gfx::destroy_surface(surface_);
    }

    [[nodiscard]] const char* name() const override { return "Window"; }

    [[nodiscard]] nme::gfx::Surface surface() const { return surface_; }

    void set_allocator(const nme::Allocator& a) { alloc_ = a; }

private:
    nme::gfx::Surface surface_{};   // id 0 == null until startup()
    nme::Allocator alloc_{};
};

// Borrowed
TimerSubsystem*  g_timer;
WindowSubsystem* g_window;
nme::Renderer*   g_renderer;
nme::JobSystem*  g_jobs;

nme::SubsystemError engine_startup(nme::Kernel& kernel, const nme::Allocator& alloc) {
    g_config   = kernel.add<ConfigSubsystem>();
    g_config->set_allocator(alloc);               // config reads via filesys now
    g_timer    = kernel.add<TimerSubsystem>();
    g_window   = kernel.add<WindowSubsystem>();   // after Config, before Renderer
    g_window->set_allocator(alloc);

    // TODO: Add more subsystems
    g_renderer = kernel.add<nme::Renderer>(g_window->surface(), alloc);
    g_jobs     = kernel.add<nme::JobSystem>();

    if (const nme::SubsystemError e = kernel.startup(); subsystem_failed(e))
        return e;

    // set full title
    char title[192];   // room for scene/project once they exist
    std::snprintf(title, sizeof title, "%s - %s <%s>",
                  NME_PLATFORM_NAME,
                  nme::engine_version(),
                  g_renderer->backend_name());
    nme::gfx::surface_set_title(g_window->surface(), title);
    return nme::subsystem_ok();
}

void engine_run() {
    std::puts("entering main loop");

    nme::CVarTable* cfg = g_config->table();

    const bool showProfiler = nme::cvar_get_bool(cfg, NME_SID("debug.showProfiler"), false);
    std::printf("  config: showProfiler=%d\n", showProfiler ? 1 : 0);

    const nme::StringId kStages[] = {
        NME_SID("input.poll"),
        NME_SID("world.update"),
        NME_SID("renderer.submit"),
    };
    for (const nme::StringId stage : kStages) {
#if NME_DEBUG
        std::printf("    stage %016llx   (%s)\n",
            static_cast<unsigned long long>(stage.value), nme::sid_to_str(stage));
#else
        std::printf("    stage %016llx\n", static_cast<unsigned long long>(stage.value));
#endif
    }

    using nme::platform::Timer;
    const Timer&            clock  = nme::platform::global_timer();
    const nme::gfx::Surface window = g_window->surface();

    nme::u64 prev      = Timer::now();
    nme::f64 fps_accum = 0.0;
    nme::u32 fps_count = 0;

    // Runs until the user closes the window (WM_CLOSE -> surface_should_close()).
    while (!nme::gfx::surface_should_close(window)) {

        // input.poll(): drain OS events. Key/mouse feed the HID layer later (Ch.9).
        nme::gfx::Event ev{};
        while (nme::gfx::poll_event(window, &ev)) {
            if (ev.type == nme::gfx::EventType::Resize) {
                std::printf("  resize -> %ux%u\n", ev.resize.width, ev.resize.height);
                // TODO(renderer): swapchain_resize(sc, ev.resize) once Vol II lands.
            }
        }

        const nme::u64 curr = Timer::now();
        const nme::f64 dt   = clock.to_seconds(curr - prev);
        prev = curr;

        // world.update(dt);  renderer.submit();

        ++fps_count;
        fps_accum += dt;
        if (fps_accum >= 1.0) {                    // report once per second, not per frame
            std::printf("  %.1f fps  (%.3f ms/frame)\n",
                        static_cast<nme::f64>(fps_count) / fps_accum,
                        1000.0 * fps_accum / static_cast<nme::f64>(fps_count));
            fps_accum = 0.0;
            fps_count = 0;
        }
    }
}

}  // namespace

int main() {
    std::printf("%s  [%s | %s | %s]\n\n", nme::engine_version(),
                    NME_PLATFORM_NAME, NME_COMPILER_NAME, NME_DEBUG ? "debug" : "release");

    nme::HeapAllocator g_heap{};
    nme::heap_alloc_init(&g_heap);

    nme::Kernel kernel(nme::heap_as_allocator(&g_heap));

    if (const nme::SubsystemError e = engine_startup(kernel, nme::heap_as_allocator(&g_heap)); subsystem_failed(e)) {
        std::fprintf(stderr, "fatal: engine startup failed: %s (%s)\n",
            e.detail, nme::subsystem_category_to_str(e.category));
        return EXIT_FAILURE;
    }
    NME_DEFER(kernel.shutdown());
    engine_run();
    return EXIT_SUCCESS;
}