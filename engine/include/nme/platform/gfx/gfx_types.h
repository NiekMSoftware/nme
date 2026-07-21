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

#define NME_GFX_FLAGS(E) \
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
NME_GFX_FLAGS(BufferUsage);

enum class TextureUsage : u32 {
    None        = 0,
    Sampled     = 1 << 0,
    ColorTarget = 1 << 1,
    DepthTarget = 1 << 2,
    CopySrc     = 1 << 3,
    CopyDst     = 1 << 4,
};
NME_GFX_FLAGS(TextureUsage)

struct Extent2D { u32 width, height; };
struct Rect2D   { i32 x, y; u32 width, height; };
struct Color    { float r, g, b, a; };
struct Viewport { float x, y, width, height, min_depth, max_depth; };

enum class EventType : u8 { None, Close, Resize, KeyDown, KeyUp, MouseMove, MouseButton, Focus };

struct Event {
    EventType type;
    union {
        Extent2D                                    resize;
        struct { i32 x, y; u32 button; bool down; } mouse;
        struct { u32 code; bool down; }             key;
        struct { bool gained; }                     focus;
    };
};

// --- Descriptors ---

struct WindowDesc {
    const char* title;
    Extent2D    extent;
    bool        resizable;
};

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

}  // nme::gfx

#endif  // NME_PLATFORM_GFX_TYPES_H_
