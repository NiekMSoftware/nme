#ifndef NME_RENDERER_GDI_GDI_TYPES_H_
#define NME_RENDERER_GDI_GDI_TYPES_H_

#include "nme/platform/gfx/gfx_types.h"   // Handle, GfxError, Extent2D (down into platform)
#include "nme/platform/types.h"

// Graphics Device Interface.
// The backend-agnostic GPU abstraction. Lives in the renderer, one layer above
// core; depends downward on core (Allocator) and platform (Surface, the shared
// handle/error/extent primitives). Never the reverse.

namespace nme::gdi {

// Reuse the platform-layer graphics primitives rather than redefine them.
using gfx::Handle;
using gfx::GfxError;
using gfx::GfxResult;      // template<typename T> = Result<T, GfxError>
using gfx::Extent2D;
using gfx::Surface;        // created by platform; consumed by create_swapchain

// GPU objects and resources are all just handles.
using Device      = Handle<struct DeviceTag>;
using Swapchain   = Handle<struct SwapchainTag>;
using CommandList = Handle<struct CommandListTag>;
using Buffer      = Handle<struct BufferTag>;
using Texture     = Handle<struct TextureTag>;      // also serves as a render target
using Shader      = Handle<struct ShaderTag>;
using Pipeline    = Handle<struct PipelineTag>;

enum class Backend : u8 { Auto, Vulkan, D3D12, D3D11, Metal, OpenGL, OpenGLES, Null };
inline const char* backend_str(const Backend b) {
    switch (b) {
        case Backend::Auto:     return "Auto";
        case Backend::Vulkan:   return "Vulkan";
        case Backend::D3D12:    return "D3D12";
        case Backend::D3D11:    return "D3D11";
        case Backend::Metal:    return "Metal";
        case Backend::OpenGL:   return "OpenGL";
        case Backend::OpenGLES: return "OpenGLES";
        case Backend::Null:     return "Null";
    }
    return "Unknown";
}

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

#define NME_GDI_FLAGS(E) \
    constexpr  E operator|(E a, E b)    noexcept { return static_cast<E>(u32(a) | u32(b)); } \
    constexpr  E operator&(E a, E b)    noexcept { return static_cast<E>(u32(a) & u32(b)); } \
    constexpr  E& operator|=(E& a, E b) noexcept { return a = a | b; }                       \
    constexpr bool any(E a) noexcept { return u32(a) != 0; }

enum class BufferUsage : u32 {
    None = 0,
    Vertex = 1 << 0,
    Index  = 1 << 1,
    Uniform = 1 << 2,
    Storage = 1 << 3,
    CopySrc = 1 << 4,
    CopyDst = 1 << 5,
};
NME_GDI_FLAGS(BufferUsage);

enum class TextureUsage : u32 {
    None        = 0,
    Sampled     = 1 << 0,
    ColorTarget = 1 << 1,
    DepthTarget = 1 << 2,
    CopySrc     = 1 << 3,
    CopyDst     = 1 << 4,
};
NME_GDI_FLAGS(TextureUsage)

struct Rect2D   { i32 x, y; u32 width, height; };
struct Color    { float r, g, b, a; };
struct Viewport { float x, y, width, height, min_depth, max_depth; };

// --- Descriptors ---

struct DeviceDesc {
    Backend backend;    // auto = platform default
    bool    debug;      // debug layers; off in ship builds
};

struct SwapchainDesc {
    Extent2D    extent;
    Format      format;
    PresentMode present_mode;
    u32         image_count;    // 2 = double, 3 = triple buffer
};

struct BufferDesc {
    u64          size;
    BufferUsage  usage;
    MemoryAccess access;
    const char*  debug_name;
};

struct TextureDesc {
    Extent2D     extent;
    Format       format;
    TextureUsage usage;
    u32          mip_levels;
    const char*  debug_name;
};

struct ShaderDesc {
    ShaderStage stage;
    const void* bytecode;       // SPIR-V / DXIL / metallib, etc.
    usize       size;
    const char* entry;
};

struct PipelineDesc {
    Shader            vertex_shader;
    Shader            fragment_shader;
    PrimitiveTopology topology;
    Format            color_format;
    Format            depth_format;
    const char*       debug_name;
};

// --- Render Pass ---

struct ColorAttachment {
    Texture target;
    LoadOp  load;
    StoreOp store;
    Color   clear;
};

struct DepthAttachment {
    Texture target;
    LoadOp  load;
    StoreOp store;
    float   clear_depth;
};

struct RenderPassDesc {
    ColorAttachment color;
    DepthAttachment depth;
};

}  // namespace nme::gdi

#endif  // NME_RENDERER_GDI_GDI_TYPES_H_