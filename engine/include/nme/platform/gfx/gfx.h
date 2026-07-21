#ifndef NME_PLATFORM_GFX_GFX_H_
#define NME_PLATFORM_GFX_GFX_H_

#include "nme/platform/gfx/gfx_types.h"

namespace nme::gfx {

// ---
// Surface
// On desktop create_window makes an OS window; on other platforms
// the impl wraps the native layer.
// ---

Surface create_surface(const WindowDesc* desc, GfxError* out_err = nullptr);
void    destroy_surface(Surface s);

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

// ---
// Frame + Commands
// Per frame: begin_frame -> acquire_command_list -> cmd_* -> submit -> end_frame.
// ---

void        begin_frame(Device d);
CommandList acquire_command_list(Device d);     // transient; owned by device
void        submit(Device d, CommandList cl);
void        end_frame(Device d);

// Recording
void cmd_begin(CommandList cl);
void cmd_end(CommandList cl);

void cmd_begin_render_pass(CommandList cl, const RenderPassDesc* pass);
void cmd_end_render_pass(CommandList cl);

// Render state
void cmd_set_viewport(CommandList cl, const Viewport* vp);
void cmd_set_scissor(CommandList cl, const Rect2D* scissor);
void cmd_bind_pipeline(CommandList cl, Pipeline p);
void cmd_push_constants(CommandList cl, const void* data, u32 size);
void cmd_bind_texture(CommandList cl, u32 slot, Texture t);

// Geometry submission
void cmd_bind_vertex_buffer(CommandList cl, u32 slot, Buffer b, u64 offset = 0);
void cmd_bind_index_buffer(CommandList cl, Buffer b, IndexType type, u64 offset = 0);
void cmd_draw(CommandList cl, u32 vertex_count, u32 first_vertex = 0);
void cmd_draw_indexed(CommandList cl, u32 index_count, u32 first_index = 0, i32 vertex_offset = 0);

}  // namespace nme::gfx

#endif  // NME_PLATFORM_GFX_GFX_H_
