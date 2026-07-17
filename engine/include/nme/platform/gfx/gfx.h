#pragma once
//==============================================================================
// Core -- Graphics wrappers
//   Gregory sec. 1.5 -- Graphics Wrappers
//------------------------------------------------------------------------------
// Responsibility documented; implementation added in the chapter/volume noted.
// TODO: thin cross-API graphics primitives for higher layers.
//==============================================================================

#include "nme/platform/gfx/gfx_types.h"

namespace nme::gfx {

// ---
// Surface
// On desktop create_window makes an OS window; on other platforms
// the impl wraps the native layer.
// ---

Surface createSurface(const WindowDesc* desc, GfxError* out_err = nullptr);
void    destroySurface(Surface s);

bool         poll_event(Surface s, Event* out); // false when queue drained
bool         surface_should_close(Surface s);
Extent2D     surface_size(Surface s);
NativeHandle surface_native(Surface s);         // consumed by create_swapchain

// ---
// Device
// Enumerates + initializes the adapter and owns every resource below.
// ---

Device  create_device(const DeviceDesc* desc, GfxError* out_err = nullptr);
void    destroy_device(Device d);
Backend device_backend(Device d);
void    device_wait_idle(Device d);

}  // namespace nme
