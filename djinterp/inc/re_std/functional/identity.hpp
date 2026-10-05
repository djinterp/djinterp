/*******************************************************************************
* djinterp [re_std]                                                 identity.hpp
*
* identity class header:
* function object: perfect-forwarding passthrough.
*   Yields its argument unchanged. Used as the default projection in
* <ranges> and as a building block for other adaptors. Standard surface
* is C++20; re_std back-ports it to C++11+ since the body needs only
* perfect forwarding. The transparent-functor `is_transparent` typedef is
* provided so it composes with set/map's heterogeneous-lookup machinery.
*
*
* path:      /inc/re_std/functional/identity.hpp
* link(s):   TBA
* author(s): re_std                                          created: 2026.05.07
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_FUNCTIONAL_IDENTITY_HPP
#define RE_STD_FUNCTIONAL_IDENTITY_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration

#if RE_STD_LANG_HAS_RVALUE_REFERENCES
    #include "re_std/utility/forward.hpp"
#endif

namespace re_std
{

#if RE_STD_LANG_HAS_RVALUE_REFERENCES

// identity
//   class: passthrough callable. operator() forwards its argument.
struct identity
{
    typedef int is_transparent;

    template<typename Type>
    RE_STD_CONSTEXPR Type&&
    operator()(
        Type&& _v
    ) const
#if RE_STD_LANG_IS_CPP11_OR_HIGHER
        noexcept
#endif
    {
        return re_std::forward<Type>(_v);
    }
};

#endif // RE_STD_LANG_HAS_RVALUE_REFERENCES

}  // re_std
#endif  // RE_STD_FUNCTIONAL_IDENTITY_HPP
