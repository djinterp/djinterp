/*******************************************************************************
* djinterp [re_std]                                                    decay.hpp
*
* decay trait header:
*   Applies the type transformations that occur when an lvalue is passed
* by value: array-to-pointer, function-to-pointer, and removal of
* references and cv-qualifiers. Per [meta.trans.other]:
*   1. let U be remove_reference<Type>::type;
*   2. if is_array<U>: yield remove_extent<U>::type*;
*   3. else if is_function<U>: yield add_pointer<U>::type;
*   4. else: yield remove_cv<U>::type.
*
*     decay<int>::type             -> int
*     decay<int&>::type            -> int
*     decay<const int&>::type      -> int
*     decay<int[5]>::type          -> int*
*     decay<int(&)[5]>::type       -> int*
*     decay<void(int)>::type       -> void(*)(int)
*     decay<void(&)(int)>::type    -> void(*)(int)
*
*
* path:      /inc/re_std/type_traits/decay.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.28
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_TYPE_TRAITS_DECAY_HPP
#define RE_STD_TYPE_TRAITS_DECAY_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration
#include "./conditional.hpp"
#include "./is_array.hpp"
#include "./is_function.hpp"
#include "./remove_cv.hpp"
#include "./remove_extent.hpp"
#include "./remove_reference.hpp"
#include "./add_pointer.hpp"


namespace re_std
{


// =============================================================================
// I.   DECAY
// =============================================================================

namespace internal
{

    // decay_array_or_function
    //   trait: handles the array / function / value-type cases on an
    // already-unreferenced type.
    template<typename U,
             bool     IsArray,
             bool     IsFunction>
    struct decay_select
    {
        // value type: strip cv.
        typedef typename remove_cv<U>::type type;
    };

    template<typename U>
    struct decay_select<U, true, false>
    {
        // array: pointer to element.
        typedef typename remove_extent<U>::type* type;
    };

    template<typename U>
    struct decay_select<U, false, true>
    {
        // function: add pointer.
        typedef typename add_pointer<U>::type type;
    };

}  // internal


// decay
//   trait: applies argument-type-decay rules to Type.
template<typename Type>
struct decay
{
private:
    typedef typename remove_reference<Type>::type U;

public:
    typedef typename internal::decay_select<
                U,
                is_array<U>::value,
                is_function<U>::value
            >::type type;
};


// =============================================================================
// II.  DECAY_T (C++11+ alias)
// =============================================================================

#if RE_STD_LANG_HAS_ALIAS_TEMPLATES

    // decay_t
    //   alias: convenience alias for decay<Type>::type.
    template<typename Type>
    using decay_t = typename decay<Type>::type;

#endif  // RE_STD_LANG_HAS_ALIAS_TEMPLATES


}  // re_std


#endif  // RE_STD_TYPE_TRAITS_DECAY_HPP
