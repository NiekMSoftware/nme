#pragma once

// Clip-space convention flags. Distinct bits so each axis holds exactly one
// choice and can be tested with == (or & if ever combined into one word).

#define NME_DEPTH_NEG_ONE_TO_ONE (1 << 0)   // -1 <= z_ndc <= 1 (OpenGL)
#define NME_DEPTH_ZERO_TO_ONE    (1 << 1)   //  0 <= z_ndc <= 1 (D3D/Vulkan/Metal)
#define NME_LEFT_HANDED          (1 << 2)   // +Z into the screen
#define NME_RIGHT_HANDED         (1 << 3)   // +Z out of the screen

#if !defined(NME_DEPTH_RANGE)
    #define NME_DEPTH_RANGE NME_DEPTH_ZERO_TO_ONE
#endif
#if !defined(NME_HANDEDNESS)
    #define NME_HANDEDNESS NME_LEFT_HANDED
#endif

#if NME_HANDEDNESS != NME_LEFT_HANDED && NME_HANDEDNESS != NME_RIGHT_HANDED
    #error "NME_HANDEDNESS must be NME_LEFT_HANDED or NME_RIGHT_HANDED"
#endif
#if NME_DEPTH_RANGE != NME_DEPTH_ZERO_TO_ONE && NME_DEPTH_RANGE != NME_DEPTH_NEG_ONE_TO_ONE
    #error "NME_DEPTH_RANGE must be NME_DEPTH_ZERO_TO_ONE or NME_DEPTH_NEG_ONE_TO_ONE"
#endif

namespace nme::platform {

inline constexpr bool is_left_handed          = (NME_HANDEDNESS == NME_LEFT_HANDED);
inline constexpr bool is_right_handed         = (NME_HANDEDNESS == NME_RIGHT_HANDED);
inline constexpr bool is_depth_zero_to_one    = (NME_DEPTH_RANGE == NME_DEPTH_ZERO_TO_ONE);
inline constexpr bool is_depth_neg_one_to_one = (NME_DEPTH_RANGE == NME_DEPTH_NEG_ONE_TO_ONE);

}  // namespace nme::platform

// EOF