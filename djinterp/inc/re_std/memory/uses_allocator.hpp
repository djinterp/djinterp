/*******************************************************************************
* djinterp [re_std]                                           uses_allocator.hpp
*
* trait detecting allocator-aware types:
*   uses_allocator<T, Alloc>::value is true iff T defines a nested
* type T::allocator_type and Alloc is convertible to that type. When
* either condition fails, the trait is false_type.
*
* the trait drives uses-allocator construction in pair, tuple,
* optional, etc. -  if uses_allocator<T,A>::value is true, the
* container constructs T as `T(allocator_arg, alloc, args...)`;
* otherwise it constructs T as `T(args...)`.
*
* C++11+ floor:
*   The detection requires void_t-style SFINAE on a nested type, plus
* is_convertible. Both are C++11+ in re_std. On C++98/03 the header is
* empty; code that needs uses_allocator must itself be gated.
*
*
* path:      /inc/re_std/memory/uses_allocator.hpp
* link(s):   TBA
* author(s): re_std contributors                             created: 2026.05.01
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_MEMORY_USES_ALLOCATOR_HPP
#define RE_STD_MEMORY_USES_ALLOCATOR_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_IS_CPP11_OR_HIGHER

    #include "re_std/type_traits/integral_constant.hpp"
    #include "re_std/type_traits/is_convertible.hpp"
    #include "re_std/type_traits/void_t.hpp"


namespace re_std
{

// =============================================================================
// internal: detect T::allocator_type
// =============================================================================

namespace internal
{

    // has_allocator_type<T>
    //   trait: true_type if T::allocator_type is a valid nested type.
    template<typename T, typename = void>
    struct has_allocator_type
        : false_type
    {
    };

    template<typename T>
    struct has_allocator_type
    <
        T,
        void_t<typename T::allocator_type>
    >
        : true_type
    {
    };

    // uses_allocator_helper<T, Alloc>
    //   trait: the default answer, dispatched on has_allocator_type so that
    // T::allocator_type is named only when it exists.
    template
    <
        typename T,
        typename Alloc,
        bool = has_allocator_type<T>::value
    >
    struct uses_allocator_helper
        : false_type
    {
    };

    template<typename T, typename Alloc>
    struct uses_allocator_helper<T, Alloc, true>
        : integral_constant
          <
              bool,
              is_convertible<Alloc, typename T::allocator_type>::value
          >
    {
    };

}  // internal
// =============================================================================
// uses_allocator
// =============================================================================

// uses_allocator<T, Alloc>
//   trait: true iff T::allocator_type is defined and Alloc is
//          convertible to it. Exactly two parameters, as std's: it is a
//          customisation point, and a third (defaulted) parameter would turn
//          every user's uses_allocator<X, A> partial specialisation into
//          one whose implicit third argument depends on its parameters,
//          which is ill-formed. The dispatch lives in an internal helper.
template<typename T, typename Alloc>
struct uses_allocator
    : internal::uses_allocator_helper<T, Alloc>
{
};


// uses_allocator_v
#if RE_STD_LANG_HAS_VARIABLE_TEMPLATES
    template<typename T, typename Alloc>
    RE_STD_CONSTEXPR bool uses_allocator_v = uses_allocator<T, Alloc>::value;
#endif


}  // re_std
#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER

#endif  // RE_STD_MEMORY_USES_ALLOCATOR_HPP
