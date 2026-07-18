//
// Created by niek on 7/18/2026.
//

// TODO: Implement win32 surface

#include "nme/platform/gfx/gfx.h"

namespace nme::gfx {

Surface createSurface(const WindowDesc*, GfxError* out_err) {
    if (out_err) *out_err = GfxError::None;
    return Surface{1};
}
void destroySurface(Surface) {}

bool poll_event(Surface, Event*) { return false; }
bool surface_should_close(Surface) { return false; }
Extent2D surface_size(Surface) { return Extent2D{1280, 720}; }
NativeHandle surface_native(Surface) { return {}; }

}  // namespace gfx

// EOF