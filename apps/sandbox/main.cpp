#include <nme/core/result/result.h>
#include <nme/core/util/scope_guard.h>
#include <nme/platform/platform.h>
#include <nme/platform/timer/timer.h>
#include <nme/platform/types.h>
#include <nme/version.h>

#include <cstdio>
#include <cstdlib>

namespace {

// Bring engine subsystems online in dependency order. Each stage returns an
// Error once it exists (memory -> logging -> jobs -> resources -> ...);
// propagate the first failure. Trivially succeeds until those land (Ch. 6+).
nme::Error engine_startup() {
    // startup() caches the platform tick frequency; a false return means we
    // couldn't query it, so the engine can't measure time -- bail out.
    if (!nme::platform::global_timer().startup()) {
        return nme::Error::NotInitialized;
    }
    return nme::Error::None;
}

// Tear down in reverse order of startup. Runs on every exit path (see main).
void engine_shutdown() {
    // Later subsystems tear down above this line; the clock came up first, so
    // it goes down last.
    nme::platform::global_timer().shutdown();
}

// The main loop. Becomes the real game loop in Ch. 8, driven by a hi-res timer;
// for now it ticks a fixed number of frames so the skeleton runs end to end.
void engine_run() {
    std::puts("entering main loop");

    using nme::platform::Timer;
    const Timer& clock = nme::platform::global_timer();

    bool running = true;
    nme::u64 frame = 0;
    constexpr nme::u64 kMaxFrames = 3;   // TEMP: no quit signal yet (HID = Ch. 9)

    nme::u64 prev = Timer::now();        // seed so the first frame's dt is -0

    while (running) {
        const nme::u64 curr = Timer::now();
        const nme::f64 dt = clock.to_seconds(curr - prev);
        prev = curr;

        // input.poll();                  // Ch. 9  HID
        // world.update(dt);              // gameplay
        // renderer.submit();             // Vol II renderer

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

    if (const nme::Error err = engine_startup(); NME_FAILED(err)) {
        std::fprintf(stderr, "fatal: engine startup failed (%s)\n",
                     nme::error_toString(err));
        return EXIT_FAILURE;
    }
    NME_DEFER( engine_shutdown() );  // guaranteed teardown on every path below

    engine_run();

    return EXIT_SUCCESS;
}
