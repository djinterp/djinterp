/***********************************************************************
* restd                                                      identity.hpp
*
* function object: perfect-forwarding passthrough.
*   Yields its argument unchanged. Used as the default projection in
* <ranges> and as a building block for other adaptors. Standard surface
* is C++20; restd back-ports it to C++11+ since the body needs only
* perfect forwarding. The transparent-functor `is_transparent` typedef is
* provided so it composes with set/map's heterogeneous-lookup machinery.
*
*
* path:      /inc/restd/functional/identity.hpp
* link(s):   TBA
* author(s): restd                                       date: 2026.05.07
***********************************************************************/

#ifndef RESTD_FUNCTIONAL_IDENTITY_
#define RESTD_FUNCTIONAL_IDENTITY_ 1

#include "djinterp.hpp"

#if D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES
    #include "restd/utility/forward.hpp"
#endif

namespace restd
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
        return restd::forward<_Type>(_v);
    }
};

#endif // D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES

} // namespace restd

#endif // RESTD_FUNCTIONAL_IDENTITY_
