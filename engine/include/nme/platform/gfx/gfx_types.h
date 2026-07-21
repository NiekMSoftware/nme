#ifndef NME_PLATFORM_GFX_GFX_TYPES_H_
#define NME_PLATFORM_GFX_GFX_TYPES_H_

#include "nme/platform/error/result.h"   // Result<T, E>
#include "nme/platform/types.h"

// Graphics Wrappers (Platform Independence Layer, Gregory Fig. 1.16).
// The OS-facing half of graphics: the native window/surface and the shared
// primitives the GDI (nme::gdi, in the renderer) builds on. Nothing here knows
// about a GPU device -- that all lives one layer up.

namespace nme::gfx {

// Generic typed handle; id 0 is the null handle.
// The GDI reuses this template for its own resource handles (down-dependency).
template<typename Tag>
struct Handle { u32 id; };

template<typename Tag> constexpr bool valid(Handle<Tag> h) { return h.id != 0; }
template<typename Tag> constexpr bool operator==(Handle<Tag> a, Handle<Tag> b) { return a.id == b.id; }
template<typename Tag> constexpr bool operator!=(Handle<Tag> a, Handle<Tag> b) { return a.id != b.id; }

// The one OS object owned by this layer.
using Surface      = Handle<struct SurfaceTag>;     // window / native layer
using NativeHandle = void*;                         // HWND / NSWindow* / ANativeWindow*, backend-specific

// Shared graphics error. Surface creation reports the windowing subset
// (Unknown / OutOfMemory / SurfaceLost); the GDI reuses the same enum for
// device + swapchain failures so callers see one error type across the seam.
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

// Every fallible graphics call (surface creation + the whole GDI) reports
// value-or-error through this. Result lives in nme::, so the unqualified name
// resolves via the enclosing namespace.
template<typename T>
using GfxResult = Result<T, GfxError>;

struct Extent2D { u32 width, height; };

// --- Window events ---

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

struct WindowDesc {
    const char* title;
    Extent2D    extent;
    bool        resizable;
};

}  // namespace nme::gfx

#endif  // NME_PLATFORM_GFX_GFX_TYPES_H_