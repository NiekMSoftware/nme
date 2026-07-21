#ifndef NME_RENDERER_GDI_GDI_H_
#define NME_RENDERER_GDI_GDI_H_

#include "nme/platform/gfx/gfx.h"          // Surface + surface_native (down into platform)
#include "nme/platform/memory/allocator.h"
#include "nme/renderer/gdi/gdi_types.h"

namespace nme::gdi {

// ---
// Device
// Enumerates + initializes the adapter and owns every resource below.
// ---

GfxResult<Device> create_device(const DeviceDesc* desc, const Allocator& alloc);
void              destroy_device(Device d);
Backend           device_backend(Device d);
void              device_wait_idle(Device d);

// ---
// Resources
// Data the app feeds the GPU. Return a null handle on failure.
// ---

GfxResult<Buffer>   create_buffer(Device d, const BufferDesc* desc);
GfxResult<Texture>  create_texture(Device d, const TextureDesc* desc);
GfxResult<Shader>   create_shader(Device d, const ShaderDesc* desc);
GfxResult<Pipeline> create_pipeline(Device d, const PipelineDesc* desc);

void destroy_buffer(Device d, Buffer b);
void destroy_texture(Device d, Texture t);
void destroy_shader(Device d, Shader s);
void destroy_pipeline(Device d, Pipeline p);

// CPU -> VRAM upload. Pure status: GfxError::None on success (no value to hand back).
GfxError write_buffer(Device d, Buffer dst, const void* data, u64 size, u64 offset = 0);

// ---
// Swapchain
// Bridges the platform window and the GPU: takes a platform Surface, builds the
// presentable back buffers on top of it.
// ---

GfxResult<Swapchain> create_swapchain(Device d, Surface s, const SwapchainDesc* desc);
void                 destroy_swapchain(Swapchain s);

// Current back buffer as a render target. Fails with SwapchainOutOfDate when the
// window was resized -- the caller recreates via swapchain_resize and retries.
GfxResult<Texture> swapchain_acquire(Swapchain sc);
void               swapchain_present(Swapchain sc);
GfxError           swapchain_resize(Swapchain sc, Extent2D extent);   // pure status
Format             swapchain_format(Swapchain sc);
Extent2D           swapchain_extent(Swapchain sc);

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

}  // namespace nme::gdi

#endif  // NME_RENDERER_GDI_GDI_H_