/***********************************************************************
* re_std                                                     identity.hpp
*
* function object: perfect-forwarding passthrough.
*   Yields its argument unchanged. Used as the default projection in
* <ranges> and as a building block for other adaptors. Standard surface
* is C++20; re_std back-ports it to C++11+ since the body needs only
* perfect forwarding. The transparent-functor `is_transparent` typedef is
* provided so it composes with set/map's heterogeneous-lookup machinery.
*
*
* path:      /inc/djinterp/re_std/functional/identity.hpp
* link(s):   TBA
* author(s): re_std                                      date: 2026.05.07
***********************************************************************/

#ifndef DJINTERP_RE_STD_FUNCTIONAL_IDENTITY_
#define DJINTERP_RE_STD_FUNCTIONAL_IDENTITY_ 1

#include "djinterp.hpp"

#if D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES
    #include "re_std/utility/forward.hpp"
#endif

namespace re_std
{

#if D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES

// identity
//   class: passthrough callable. operator() forwards its argument.
struct identity
{
    typedef int is_transparent;

    template<typename _Type>
    D_CONSTEXPR _Type&&
    operator()(
        _Type&& _v
    ) const
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
        noexcept
#endif
    {
        return re_std::forward<_Type>(_v);
    }
};

#endif // D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES

} // namespace re_std

#endif  // DJINTERP_RE_STD_FUNCTIONAL_IDENTITY_
