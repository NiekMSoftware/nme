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

// ---
// Resources
// Data the apps feeds the GPU. Return a null handle on failure.
// ---

Buffer   create_buffer(Device d, const BufferDesc* desc, GfxError* out_err = nullptr);
Texture  create_texture(Device d, const TextureDesc* desc, GfxError* out_err = nullptr);
Shader   create_shader(Device d, const ShaderDesc* desc, GfxError* out_err = nullptr);
Pipeline create_pipeline(Device d, const PipelineDesc* desc, GfxError* out_err = nullptr);

void destroy_buffer(Device d, Buffer b);
void destroy_texture(Device d, Texture t);
void destroy_shader(Device d, Shader s);
void destroy_pipeline(Device d, Pipeline p);

// CPU -> VRAM upload. None on success.
GfxError write_buffer(Device d, Buffer dst, const void* data, u64 size, u64 offset = 0);

// ---
// Swapchain
// ---

Swapchain create_swapchain(Device d, Surface s, const SwapchainDesc* desc, GfxError* out_err = nullptr);
void      destroy_swapchain(Swapchain s);

// Current back buffer as a render target.
Texture  swapchain_acquire(Swapchain sc);
void     swapchain_present(Swapchain sc);
GfxError swapchain_resize(Swapchain sc, Extent2D extent);
Format   swapchain_format(Swapchain sc);
Extent2D swapchain_extent(Swapchain sc);

}  // namespace nme
