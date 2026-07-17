#ifndef NME_PLATFORM_GFX_TYPES_H_
#define NME_PLATFORM_GFX_TYPES_H_

#include "nme/platform/types.h"

namespace nme::gfx {

template<typename Tag>
struct Handle { u32 id; };

template<typename Tag> constexpr bool valid(Handle<Tag> h) { return h.id != 0; }
template<typename Tag> constexpr bool operator==(Handle<Tag> a, Handle<Tag> b) { return a.id == b.id; }
template<typename Tag> constexpr bool operator!=(Handle<Tag> a, Handle<Tag> b) { return a.id != b.id; }

// Objects and resources alike are all just handles.
using Device      = Handle<struct DeviceTag>;
using Surface     = Handle<struct SurfaceTag>;      // window / native layer
using Swapchain   = Handle<struct SwapchainTag>;
using CommandList = Handle<struct CommandListTag>;
using Buffer      = Handle<struct BufferTag>;
using Texture     = Handle<struct TextureTag>;      // also serves as a render target
using Shader      = Handle<struct ShaderTag>;
using Pipeline    = Handle<struct PipelineTag>;

using NativeHandle = void*;     // HWND / NSWindow* / ANativeWindow*, backend-specific

enum class GfxError : u8 {
    None = 0,
    Unknown,
    OutOfMemory,
    InvalidArgs,
    BackendUnavailable,
    DeviceLost,
    SurfaceLost,
    SwapchainOutOfDate
};

enum class Backend : u8 { Auto, Vulkan, D3D12, D3D11, Metal, OpenGL, OpenGLES, Null };

enum class Format : u8 {
    Undefined,
    RGBA8Unorm, RGBA8Srgb, BGRA8Unorm, BGRA8Srgb,   // desktop back-buffer formats
    RGBA16F,                                        // HDR targets
    R32F, D32F, D24UnormS8                          // depth / depth-stencil
};

enum class PrimitiveTopology : u8 { TriangleList, TriangleStrip, LineList, PointList };
enum class IndexType         : u8 { U16, U32 };
enum class ShaderStage       : u8 { Vertex, Fragment, Compute };
enum class PresentMode       : u8 { Fifo, Mailbox, Immediate };     // vsync / triple / off
enum class LoadOp            : u8 { Load, Clear, DontCare };
enum class StoreOp           : u8 { Store, DontCare };
enum class MemoryAccess      : u8 { GpuOnly, CpuToGpu, GpuToCpu };

}  // nme::gfx

#endif  // NME_PLATFORM_GFX_TYPES_H_
