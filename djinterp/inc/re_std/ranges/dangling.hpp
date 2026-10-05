/*******************************************************************************
* djinterp [re_std]                                                 dangling.hpp
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
* path:      /inc/re_std/ranges/dangling.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.13
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_RANGES_DANGLING_HPP
#define RE_STD_RANGES_DANGLING_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


namespace re_std
{


// ===========================================================================
// I.   DANGLING
// ===========================================================================

// dangling
//   class: placeholder returned by range algorithms in lieu of an
// iterator (or subrange) when the source range is an rvalue and is
// not a borrowed_range. Carries no state; its sole purpose is to
// prevent users from inadvertently holding an iterator into a range
// that has already been destroyed.
// note: declared in the top-level re_std namespace (matching std::ranges::
// dangling). The C++20 surface for using-decls in re_std::ranges:: is
// re-exported by the umbrella header.
class dangling
{
public:
    // default ctor
    //   function: trivial. implicitly constexpr on C++11+.
    dangling()
    RE_STD_NOEXCEPT
    {}

    // value ctors
    //   function: accept and discard any argument list. Matches the
    // C++20 ctor requirement that dangling be constructible from any
    // sequence of arguments (used when algorithms instantiate the
    // dangling type with their argument pack).
    template<typename Type>
    dangling(Type const&)
    RE_STD_NOEXCEPT
    {}

#if RE_STD_LANG_HAS_VARIADIC_TEMPLATES
    template<typename T1,
             typename T2,
             typename... Rest>
    dangling(T1 const&, T2 const&, Rest const&...)
    RE_STD_NOEXCEPT
    {}
#endif
};


}  // re_std


#endif  // RE_STD_RANGES_DANGLING_HPP
