#include <nme/core/jobs/job_system.h>
#include <nme/core/result/result.h>
#include <nme/core/string/string_id.h>
#include <nme/core/subsystem/kernel.h>
#include <nme/core/subsystem/subsystem.h>
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
    [[nodiscard]] nme::Error startup() override {
        if (!nme::platform::global_timer().startup())
            return nme::Error::NotInitialized;
        return nme::Error::None;
    }

    void shutdown() override {
        nme::platform::global_timer().shutdown();
    }

    [[nodiscard]] const char* name() const override { return "Timer"; }
};

// Borrowed
TimerSubsystem* g_timer;
nme::Renderer*  g_renderer;
nme::JobSystem* g_jobs;

nme::Error engine_startup(nme::Kernel& kernel) {
    g_timer = kernel.add<TimerSubsystem>();

    // TODO: Add more subsystems
    g_renderer = kernel.add<nme::Renderer>();
    g_jobs     = kernel.add<nme::JobSystem>();

    return kernel.startup();
}

void engine_run() {
    std::puts("entering main loop");

    const nme::StringId kStages[] = {
        NME_SID("input.poll"),
        NME_SID("world.update"),
        NME_SID("renderer.submit"),
    };
    for (const nme::StringId stage : kStages) {
#if NME_DEBUG
        std::printf("    stage %016llx   (%s)\n",   // TEMP: proves debug recovery
            static_cast<unsigned long long>(stage.value),
            nme::sid_to_str(stage));
#else
        std::printf("    stage %016llx\n",          // release: only the hash exists
            static_cast<unsigned long long>(stage.value));
#endif
    }

    using nme::platform::Timer;
    const Timer &clock = nme::platform::global_timer();

    bool running = true;
    nme::u64 frame = 0;
    constexpr nme::u64 kMaxFrames = 3;  // TEMP: no quit signal yet

    nme::u64 prev = Timer::now();

    while (running) {
        const nme::u64 curr = Timer::now();
        const nme::f64 dt = clock.to_seconds(curr - prev);
        prev = curr;

        // input.poll();
        // world.update(dt);
        // renderer.submit();

        std::printf("  frame %llu  dt = %.6f s\n",   // TEMP: proves the clock ticks
                    static_cast<unsigned long long>(frame), dt);

        if (++frame >= kMaxFrames) {
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

    nme::Kernel kernel;

    if (const nme::Error e = engine_startup(kernel); NME_FAILED(e)) {
        std::fprintf(stderr, "fatal: engine startup failed: (%s)\n",
            nme::error_toString(e));
        return EXIT_FAILURE;
    }
    NME_DEFER(kernel.shutdown());
    engine_run();
    return EXIT_SUCCESS;
}
