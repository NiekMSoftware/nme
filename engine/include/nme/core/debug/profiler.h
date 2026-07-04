#pragma once
//==============================================================================
// Core Systems -- Profiling / instrumentation front-end
//   (Gregory sec. 1.5 "Profiling / Stats Gathering"; Ch. 10.8; Ch. 2)
//------------------------------------------------------------------------------
// With -DNME_ENABLE_TRACY=ON these route to Tracy; otherwise they compile to
// nothing. Ch. 10 grows this into an in-engine hierarchical profiler.
//   void update() { NME_PROFILE_ZONE(); ... }   // once per frame: NME_PROFILE_FRAME_MARK();
//==============================================================================
#if defined(NME_ENABLE_TRACY) && NME_ENABLE_TRACY
    #include <tracy/Tracy.hpp>
    #define NME_PROFILE_ZONE()           ZoneScoped
    #define NME_PROFILE_ZONE_NAMED(name) ZoneScopedN(name)
    #define NME_PROFILE_FRAME_MARK()     FrameMark
    #define NME_PROFILE_THREAD_NAME(name) tracy::SetThreadName(name)
#else
    #define NME_PROFILE_ZONE()           (void)0
    #define NME_PROFILE_ZONE_NAMED(name) (void)0
    #define NME_PROFILE_FRAME_MARK()     (void)0
#endif
