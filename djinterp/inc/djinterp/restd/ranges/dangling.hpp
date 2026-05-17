/******************************************************************************
* djinterp [restd]                                                dangling.hpp
*
* dangling tag header:
*   Provides the placeholder type returned in lieu of an iterator or
* subrange when a range-based algorithm is called on an rvalue range
* that is not a borrowed_range. Holding the result of such a call gives
* a dangling object instead of an iterator into a destroyed range.
*
*   PORTABILITY:
*   Standalone empty class. Available unconditionally on C++98+.
*   The C++20 standard adds constexpr default constructors; since
* dangling is an aggregate with no members both default and copy
* construction are implicitly constexpr on C++11+.
*
*
* path:      /inc/djinterp/restd/ranges/dangling.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RESTD_RANGES_DANGLING_
#define DJINTERP_RESTD_RANGES_DANGLING_ 1

#include "../../core/djinterp.hpp"


NS_RESTD


// ===========================================================================
// I.   DANGLING
// ===========================================================================

// dangling
//   class: placeholder returned by range algorithms in lieu of an
// iterator (or subrange) when the source range is an rvalue and is
// not a borrowed_range. Carries no state; its sole purpose is to
// prevent users from inadvertently holding an iterator into a range
// that has already been destroyed.
// note: declared in the top-level restd namespace (matching std::ranges::
// dangling). The C++20 surface for using-decls in restd::ranges:: is
// re-exported by the umbrella header.
class dangling
{
public:
    // default ctor
    //   function: trivial. implicitly constexpr on C++11+.
    dangling()
    D_NOEXCEPT
    {}

    // value ctors
    //   function: accept and discard any argument list. Matches the
    // C++20 ctor requirement that dangling be constructible from any
    // sequence of arguments (used when algorithms instantiate the
    // dangling type with their argument pack).
    template<typename _Type>
    dangling(_Type const&)
    D_NOEXCEPT
    {}

#if D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES
    template<typename _T1,
             typename _T2,
             typename... _Rest>
    dangling(_T1 const&, _T2 const&, _Rest const&...)
    D_NOEXCEPT
    {}
#endif
};


NS_END  // restd


#endif  // DJINTERP_RESTD_RANGES_DANGLING_
