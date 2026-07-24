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
#include <nme/resource/package.h>
#include <nme/resource/resource_manager.h>
#include <nme/version.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>

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
        std::printf("---\n%.*s ---\n\n", static_cast<int>(blob.size),
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

// Resource subsystem. Owns the resource manager and any mounted archives.
// Depends only downward -- platform/filesys for bytes, core/string for the GID
// (StringId). Asset-owning subsystems (renderer, audio) register their loaders
// against manager() and acquire through it; the manager stays asset-agnostic.
class ResourceSubsystem final : public nme::Subsystem {
public:
    [[nodiscard]] nme::SubsystemError startup() override {
        nme::res::resource_manager_init(&mgr_, alloc_);

        // Mount the base archive if present. When it isn't, loose files
        // (StreamingAssets style) still resolve via the manager's fallback.
        nme::fs::Path pak{};
        if (nme::fs::path_join(&pak, "assets", "base.nmepak") && nme::fs::file_exists(pak.data)) {
            auto r = nme::res::package_mount(pak.data, alloc_);
            if (nme::result_is_err(&r))
                return nme::subsystem_error(nme::SubsystemError::Category::NotInitialized,
                    "failed to mount base.nmepak");
            packs_[pack_count_] = nme::result_value(&r);
            nme::res::resource_mount(&mgr_, &packs_[pack_count_]);
            ++pack_count_;
            std::printf("  resources: mounted %s\n", pak.data);
        } else {
            std::puts("  resources: no base.nmepak, loose files only");
        }
        return nme::subsystem_ok();
    }

    void shutdown() override {
        nme::res::resource_manager_shutdown(&mgr_);                 // unload live assets first
        while (pack_count_ > 0) nme::res::package_unmount(&packs_[--pack_count_]);
    }

    [[nodiscard]] const char* name() const override { return "Resources"; }

    // Asset owners register loaders and acquire through this.
    nme::res::ResourceManager* manager() { return &mgr_; }

    void set_allocator(const nme::Allocator& a) { alloc_ = a; }

private:
    nme::res::ResourceManager mgr_{};
    nme::res::Package         packs_[4]{};   // room for base + patches/DLC
    nme::u32                  pack_count_ = 0;
    nme::Allocator            alloc_{};
};

ResourceSubsystem* g_resources;

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

    g_resources = kernel.add<ResourceSubsystem>();  // before Renderer: it registers loaders here
    g_resources->set_allocator(alloc);

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

    // --- resource manager smoke test (runs once at boot) ---------------------
    // Registers a trivial loader that keeps the raw bytes, then drives the
    // acquire -> bump -> release -> unload path so we can see it work.
    {
        enum : nme::u16 { kRawType = 0 };

        struct RawAsset { nme::usize size; nme::u8* bytes; };

        auto raw_load = [](const nme::fs::FileBlob* b, const nme::Allocator* a) -> void* {
            auto* r   = static_cast<RawAsset*>(nme::alloc(a, sizeof(RawAsset), alignof(RawAsset)));
            r->size   = b->size;
            r->bytes  = static_cast<nme::u8*>(nme::alloc(a, b->size ? b->size : 1, 16));
            std::memcpy(r->bytes, b->data, b->size);
            return r;
        };
        auto raw_unload = [](void* asset, const nme::Allocator* a) -> void* {
            auto* r = static_cast<RawAsset*>(asset);
            nme::free(a, r->bytes, r->size ? r->size : 1);
            nme::free(a, r, sizeof(RawAsset));
            return nullptr;
        };

        nme::res::ResourceManager* rm = g_resources->manager();
        nme::res::resource_register_loader(rm, kRawType, { raw_load, raw_unload });

        // A loose file is the safe target at boot (works with or without a pak).
        const char* kProbe = "config/engine.ini";
        std::printf("  resources: probing \"%s\"\n", kProbe);

        auto r1 = nme::res::resource_acquire(rm, kProbe, kRawType);
        if (nme::result_is_err(&r1)) {
            const nme::res::ResourceError e = nme::result_error(&r1);
            const char* names[] = { "None","FileError","NoLoader","NotFound","LoadFailed","OutOfMemory" };
            std::printf("  resources: acquire failed -> %s\n", names[static_cast<nme::u8>(e)]);
            if (e == nme::res::ResourceError::NoLoader)
                std::puts("  resources: (loader not registered on this manager -- "
                          "check resource_register_loader / init order)");
        } else {
            const nme::res::ResourceHandle h = nme::result_value(&r1);
            const auto* a1 = static_cast<const RawAsset*>(nme::res::resource_get(rm, h));
            std::printf("  resources: acquired handle{%u,%u}  %zu bytes\n",
                        h.index, h.generation, a1 ? a1->size : 0);

            // Re-acquire: same id -> same handle, refcount bumped, no reload.
            auto r2 = nme::res::resource_acquire(rm, kProbe, kRawType);
            const nme::res::ResourceHandle h2 = nme::result_value(&r2);
            std::printf("  resources: re-acquire same handle? %s\n", (h == h2) ? "yes" : "no");

            // Drop both refs: first keeps it live, second unloads it.
            nme::res::resource_release(rm, h2);
            std::printf("  resources: after 1 release, still ready? %s\n",
                        (nme::res::resource_state(rm, h) == nme::res::ResourceState::Ready) ? "yes" : "no");
            nme::res::resource_release(rm, h);
            std::printf("  resources: after 2 releases, get() -> %s (stale handle)\n",
                        nme::res::resource_get(rm, h) ? "non-null" : "null");
        }
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