/******************************************************************************
* djinterp [restd]                                                   decay.hpp
*
* decay trait header:
*   Applies the type transformations that occur when an lvalue is passed
* by value: array-to-pointer, function-to-pointer, and removal of
* references and cv-qualifiers. Per [meta.trans.other]:
*   1. let U be remove_reference<_Type>::type;
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
* path:      /inc/djinterp/re_std/type_traits/decay.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.28
******************************************************************************/

#ifndef DJINTERP_RESTD_TYPE_TRAITS_DECAY_
#define DJINTERP_RESTD_TYPE_TRAITS_DECAY_ 1

// djinterp
#include "../../core/djinterp.hpp"
#include "./conditional.hpp"
#include "./is_array.hpp"
#include "./is_function.hpp"
#include "./remove_cv.hpp"
#include "./remove_extent.hpp"
#include "./remove_reference.hpp"
#include "./add_pointer.hpp"


NS_RESTD


// =============================================================================
// I.   DECAY
// =============================================================================

NS_INTERNAL

    // decay_array_or_function
    //   helper: handles the array / function / value-type cases on an
    // already-unreferenced type.
    template<typename _U,
             bool     _IsArray,
             bool     _IsFunction>
    struct decay_select
    {
        // value type: strip cv.
        typedef typename remove_cv<_U>::type type;
    };

    template<typename _U>
    struct decay_select<_U, true, false>
    {
        // array: pointer to element.
        typedef typename remove_extent<_U>::type* type;
    };

    template<typename _U>
    struct decay_select<_U, false, true>
    {
        // function: add pointer.
        typedef typename add_pointer<_U>::type type;
    };

NS_END  // internal


// decay
//   trait: applies argument-type-decay rules to _Type.
template<typename _Type>
struct decay
{
private:
    typedef typename remove_reference<_Type>::type _U;

public:
    typedef typename internal::decay_select<
                _U,
                is_array<_U>::value,
                is_function<_U>::value
            >::type type;
};


// =============================================================================
// II.  DECAY_T (C++11+ alias)
// =============================================================================

#if D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES

    // decay_t
    //   alias: convenience alias for decay<_Type>::type.
    template<typename _Type>
    using decay_t = typename decay<_Type>::type;

#endif  // D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES


NS_END  // restd


#endif  // DJINTERP_RESTD_TYPE_TRAITS_DECAY_
