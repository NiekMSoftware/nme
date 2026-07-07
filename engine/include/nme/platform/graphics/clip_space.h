#pragma once

#define NME_LEFT_HANDED  1
#define NME_RIGHT_HANDED 2

#define NME_DEPTH_ZERO_TO_ONE    1
#define NME_DEPTH_NEG_ONE_TO_ONE 2

#if !defined(NME_HANDEDNESS)
    #define NME_HANDEDNESS   NME_LEFT_HANDED
#endif

#if !defined(NME_DEPTH_RANGE)
    #define NME_DEPTH_RANGE   NME_DEPTH_ZERO_TO_ONE
#endif

#if NME_HANDEDNESS != NME_LEFT_HANDED && NME_HANDEDNESS != NME_RIGHT_HANDED
    #error "NME_HANDEDNESS must be NME_LEFT_HANDED or NME_RIGHT_HANDED"
#endif

#if NME_DEPTH_RANGE != NME_DEPTH_ZERO_TO_ONE && NME_DEPTH_RANGE != NME_DEPTH_NEG_ONE_TO_ONE
    #error "NME_DEPTH_RANGE must be NME_DEPTH_ZERO_TO_ONE or NME_DEPTH_NEG_ONE_TO_ONE"
#endif

#if NME_HANDEDNESS == NME_LEFT_HANDED
    #define NME_IS_LEFT_HANDED  1
    #define NME_IS_RIGHT_HANDED 0
#else
    #define NME_IS_LEFT_HANDED  0
    #define NME_IS_RIGHT_HANDED 1
#endif

#if NME_DEPTH_RANGE == NME_DEPTH_ZERO_TO_ONE
    #define NME_IS_DEPTH_ZERO_TO_ONE    1
    #define NME_IS_DEPTH_NEG_ONE_TO_ONE 0
#else
    #define NME_IS_DEPTH_ZERO_TO_ONE    0
    #define NME_IS_DEPTH_NEG_ONE_TO_ONE 1
#endif

namespace nme::platform {

inline constexpr bool is_left_handed          = (NME_IS_LEFT_HANDED == 1);
inline constexpr bool is_right_handed         = (NME_IS_RIGHT_HANDED == 1);
inline constexpr bool is_depth_zero_to_one    = (NME_IS_DEPTH_ZERO_TO_ONE == 1);
inline constexpr bool is_depth_neg_one_to_one = (NME_IS_DEPTH_NEG_ONE_TO_ONE == 1);

}  // namespace nme::platform