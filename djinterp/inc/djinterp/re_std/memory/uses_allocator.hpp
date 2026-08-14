/***********************************************************************
* restd                                                 uses_allocator.hpp
*
* trait detecting allocator-aware types:
*   uses_allocator<_T, _Alloc>::value is true iff _T defines a nested
* type _T::allocator_type and _Alloc is convertible to that type. When
* either condition fails, the trait is false_type.
*
* the trait drives uses-allocator construction in pair, tuple,
* optional, etc. -  if uses_allocator<T,A>::value is true, the
* container constructs T as `T(allocator_arg, alloc, args...)`;
* otherwise it constructs T as `T(args...)`.
*
* C++11+ floor:
*   The detection requires void_t-style SFINAE on a nested type, plus
* is_convertible. Both are C++11+ in restd. On C++98/03 the header is
* empty; code that needs uses_allocator must itself be gated.
*
*
* path:      /inc/djinterp/re_std/memory/uses_allocator.hpp
* link(s):   TBA
* author(s): restd contributors                          date: 2026.05.01
***********************************************************************/

#ifndef RESTD_MEMORY_USES_ALLOCATOR_
#define RESTD_MEMORY_USES_ALLOCATOR_ 1

#include "djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

    #include "restd/type_traits/integral_constant.hpp"
    #include "restd/type_traits/is_convertible.hpp"
    #include "restd/type_traits/void_t.hpp"


namespace restd
{

// =============================================================================
// internal: detect _T::allocator_type
// =============================================================================

namespace internal
{

    // has_allocator_type<_T>
    //   trait: true_type if _T::allocator_type is a valid nested type.
    template<typename _T, typename = void>
    struct has_allocator_type
        : false_type
    {
    };

    template<typename _T>
    struct has_allocator_type
    <
        _T,
        typename void_t<typename _T::allocator_type>::type
    >
        : true_type
    {
    };

}  // namespace internal


// =============================================================================
// uses_allocator
// =============================================================================

// uses_allocator<_T, _Alloc>
//   trait: true iff _T::allocator_type is defined and _Alloc is
//          convertible to it. Three-parameter primary template uses a
//          bool dispatcher to select a fully-defined specialisation.
template
<
    typename _T,
    typename _Alloc,
    bool = internal::has_allocator_type<_T>::value
>
struct uses_allocator
    : false_type
{
};

template<typename _T, typename _Alloc>
struct uses_allocator<_T, _Alloc, true>
    : integral_constant
      <
          bool,
          is_convertible<_Alloc, typename _T::allocator_type>::value
      >
{
};


// uses_allocator_v
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _T, typename _Alloc>
    D_CONSTEXPR bool uses_allocator_v = uses_allocator<_T, _Alloc>::value;
#endif


}  // namespace restd

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // RESTD_MEMORY_USES_ALLOCATOR_
