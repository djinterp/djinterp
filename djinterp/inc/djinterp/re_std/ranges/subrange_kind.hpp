/******************************************************************************
* djinterp [restd]                                            subrange_kind.hpp
*
* subrange_kind enum header:
*   Provides the two-value enumeration that distinguishes a sized
* subrange from an unsized one. subrange<I, S, K> stores a cached
* size when K == subrange_kind::sized; the cached field is omitted
* when K == subrange_kind::unsized, saving a word per subrange.
*
*   PORTABILITY:
*   - C++11+: scoped enumeration (enum class : unsigned char).
*   - C++98/03: struct-wrapper fallback. subrange_kind is a struct
*     holding a sole nested unscoped enum 'sized / unsized', and a
*     value typedef. Calls of the form 'subrange_kind::sized' compile
*     unchanged on both paths.
*
*
* path:      /inc/djinterp/re_std/ranges/subrange_kind.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RESTD_RANGES_SUBRANGE_KIND_
#define DJINTERP_RESTD_RANGES_SUBRANGE_KIND_ 1

#include "../../core/djinterp.hpp"


NS_RESTD


// ===========================================================================
// I.   SUBRANGE_KIND
// ===========================================================================

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// subrange_kind
//   enum: classifies a subrange as size-tracking (cached integer
// length) or not. The non-sized form omits the cached length and is
// the default when iter/sentinel are not sized_sentinel_for.
enum class subrange_kind : unsigned char
{
    unsized = 0,
    sized   = 1
};

#else

// subrange_kind
//   struct: C++98/03 stand-in for the scoped enum. Provides the same
// member names so that 'subrange_kind::sized' compiles on both paths.
// note: this is a struct not a namespace so it can appear as a
// non-type template parameter via its nested 'value' typedef.
struct subrange_kind
{
    enum value
    {
        unsized = 0,
        sized   = 1
    };
};

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


NS_END  // restd


#endif  // DJINTERP_RESTD_RANGES_SUBRANGE_KIND_
