#ifndef NME_PLATFORM_GFX_GFX_H_
#define NME_PLATFORM_GFX_GFX_H_

#include "nme/platform/gfx/gfx_types.h"
#include "nme/platform/memory/allocator.h"

namespace nme::gfx {

// ---
// Surface  (Graphics Wrappers)
// The OS windowing seam. On desktop create_surface opens an OS window; on other
// platforms it wraps the native layer. This is the ONLY graphics code in the
// platform layer -- device/swapchain/commands all live in nme::gdi (renderer).
//
// The seam upward: the GDI's create_swapchain takes a Surface and calls
// surface_native() to get the handle it turns into a VkSurfaceKHR. That native
// handle is the single thing that crosses from platform up into the renderer.
// ---

GfxResult<Surface> create_surface(const WindowDesc* desc, const Allocator& alloc);
void               destroy_surface(Surface s);

bool         poll_event(Surface s, Event* out);   // false when queue drained
bool         surface_should_close(Surface s);
Extent2D     surface_size(Surface s);
NativeHandle surface_native(Surface s);           // consumed by nme::gdi::create_swapchain

}  // namespace nme::gfx

#endif  // NME_PLATFORM_GFX_GFX_H_