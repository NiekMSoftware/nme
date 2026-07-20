#include <nme/core/config/cvar.h>
#include <nme/core/config/config_file.h>
#include <nme/core/jobs/job_system.h>
#include <nme/core/memory/heap_alloc.h>
#include <nme/core/string/string_id.h>
#include <nme/core/subsystem/kernel.h>
#include <nme/core/subsystem/subsystem.h>
#include <nme/core/subsystem/subsystem_error.h>
#include <nme/core/util/scope_guard.h>
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

class ConfigSubsystem final : public nme::Subsystem {
public:
    [[nodiscard]] nme::SubsystemError startup() override {
        nme::cvar_table_init(&table_);

        // programmer defaults — the ini overrides these (Gregory's model)
        nme::cvar_reg_float(&table_, NME_SID("physics.gravity"),   -9.81f, "physics.gravity");
        nme::cvar_reg_int  (&table_, NME_SID("window.width"),       1280,  "window.width");
        nme::cvar_reg_int  (&table_, NME_SID("window.height"),       720,  "window.height");
        nme::cvar_reg_bool (&table_, NME_SID("debug.showProfiler"), false,  "debug.showProfiler");
        nme::cvar_reg_int  (&table_, NME_SID("app.maxFrames"),         3,   "app.maxFrames");

        // non-const: result_is_err() takes a non-const pointer
        auto r = nme::config_load_ini(&table_, "config/engine.ini");
        if (nme::result_is_err(&r)) {
            std::puts("  config: engine.ini not found, using defaults");
        } else {
            std::printf("  config: applied %u override(s)\n",
                        static_cast<unsigned>(nme::result_value(&r)));
        }
        return nme::subsystem_ok();
    }

    void shutdown() override {}

    [[nodiscard]] const char* name() const override { return "Config"; }

    nme::CVarTable* table() { return &table_; }

private:
    nme::CVarTable table_{};
};

// Borrowed
ConfigSubsystem* g_config;
TimerSubsystem*  g_timer;
nme::Renderer*   g_renderer;
nme::JobSystem*  g_jobs;

nme::SubsystemError engine_startup(nme::Kernel& kernel) {
    g_config   = kernel.add<ConfigSubsystem>();
    g_timer    = kernel.add<TimerSubsystem>();

    // TODO: Add more subsystems
    g_renderer = kernel.add<nme::Renderer>();
    g_jobs     = kernel.add<nme::JobSystem>();

    return kernel.startup();
}

void engine_run() {
    std::puts("entering main loop");

    nme::CVarTable* cfg = g_config->table();

    const nme::i32 maxFrames    = nme::cvar_get_int (cfg, NME_SID("app.maxFrames"), 3);
    const bool     showProfiler = nme::cvar_get_bool(cfg, NME_SID("debug.showProfiler"), false);
    std::printf("  config: maxFrames=%d showProfiler=%d\n", maxFrames, showProfiler ? 1 : 0);

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
    const Timer& clock = nme::platform::global_timer();

    bool running = true;
    nme::u64 frame = 0;
    nme::u64 prev = Timer::now();

    while (running) {
        const nme::u64 curr = Timer::now();
        const nme::f64 dt = clock.to_seconds(curr - prev);
        prev = curr;

        // input.poll();  world.update(dt);  renderer.submit();

        std::printf("  frame %llu  dt = %.6f s\n",
                    static_cast<unsigned long long>(frame), dt);

        if (++frame >= static_cast<nme::u64>(maxFrames)) {
            running = false;
        }
    }

    std::printf("exited main loop after %llu frames\n",
                static_cast<unsigned long long>(frame));
}

}  // namespace

int main() {
    std::printf("%s  [%s | %s | %s]\n\n", nme::engine_version(),
                    NME_PLATFORM_NAME, NME_COMPILER_NAME, NME_DEBUG ? "debug" : "release");

    nme::HeapAllocator g_heap{};
    nme::heap_alloc_init(&g_heap);

    nme::Kernel kernel(nme::heap_as_allocator(&g_heap));

    if (const nme::SubsystemError e = engine_startup(kernel); subsystem_failed(e)) {
        std::fprintf(stderr, "fatal: engine startup failed: %s (%s)\n",
            e.detail, nme::subsystem_category_to_str(e.category));
        return EXIT_FAILURE;
    }
    NME_DEFER(kernel.shutdown());
    engine_run();
    return EXIT_SUCCESS;
}