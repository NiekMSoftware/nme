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


}  // nme::gfx

#endif  // NME_PLATFORM_GFX_TYPES_H_
