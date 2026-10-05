/*******************************************************************************
* djinterp [re_std]                                        is_swappable_with.hpp
*
* is_swappable_with trait:
*   true_type if swap(declval<T>(), declval<U>()) and the reverse call are
* both well-formed expressions in unevaluated context, false_type otherwise.
*
*   LOOKUP:
*   The unqualified swap() call participates in argument-dependent lookup, and
* re_std::swap is brought into the detection namespace via using-declaration so
* that user types relying on the generic re_std swap fallback are correctly
* reported as swappable. This mirrors the "using std::swap; swap(t, u);" rule
* from [swappable.requirements] in the C++ standard.
*
*   PORTABILITY:
*   Available on C++11 and later. C++98/03 omits the trait entirely because
* the underlying SFINAE probe requires decltype, declval, and rvalue
* references. Returning false_type unconditionally on C++98/03 would be
* incorrect (built-in types are swappable), so omission is the only sound
* fallback.
*
*   DEPENDENCIES:
*   re_std::swap from /inc/re_std/utility/swap.hpp and re_std::declval
* from /inc/re_std/utility/declval.hpp must be declared before this
* header is included. If the utility headers are not yet present in the
* tree, this file will fail to compile at the using-declaration on the line
* `using re_std::swap;`.
*
*
* path:      /inc/re_std/type_traits/is_swappable_with.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.29
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_TYPE_TRAITS_IS_SWAPPABLE_WITH_HPP
#define RE_STD_TYPE_TRAITS_IS_SWAPPABLE_WITH_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_IS_CPP11_OR_HIGHER

// re_std
#include "./true_type.hpp"
#include "./false_type.hpp"
#include "./void_t.hpp"
#include "../utility/declval.hpp"
#include "../utility/swap.hpp"


namespace re_std
{


    namespace internal
    {

        // swappable_lookup
        //   namespace: dedicated lookup context that brings re_std::swap into
        //              scope. Inside this namespace, an unqualified call to
        //              swap(t, u) finds re_std::swap (via the using-declaration)
        //              and any ADL-found overload from the namespaces of T
        //              or U, exactly matching the standard's lookup rules
        //              for [swappable].
        namespace swappable_lookup
        {

            using re_std::swap;

            // is_swappable_with_helper
            //   trait: primary; default to false_type when the swap
            //          expressions are not well-formed.
            template<typename T,
                     typename U,
                     typename = void>
            struct is_swappable_with_helper
                : false_type
            {};

            // is_swappable_with_helper<T, U, void>
            //   trait: specialization; selected when both swap(t, u) and
            //          swap(u, t) are well-formed expressions. Uses void_t
            //          on the decltypes of both directional calls so that
            //          asymmetric swap overloads (one direction only) are
            //          correctly rejected.
            template<typename T,
                     typename U>
            struct is_swappable_with_helper<
                T,
                U,
                re_std::void_t<
                    decltype( swap( re_std::declval<T>(),
                                    re_std::declval<U>() ) ),
                    decltype( swap( re_std::declval<U>(),
                                    re_std::declval<T>() ) )
                    > >
                : true_type
            {};

        }  // namespace swappable_lookup

    }  // internal


    // is_swappable_with
    //   trait: true_type if swap(declval<T>(), declval<U>()) and the
    //          reverse call are both well-formed; false_type otherwise.
    //
    //   note: T and U are passed as-is (no reference-stripping or
    //         cv-stripping). To test "swappability of objects of type T",
    //         see is_swappable<T>, which forwards add_lvalue_reference<T>::type
    //         on both sides.
    template<typename T,
             typename U>
    struct is_swappable_with
        : internal::swappable_lookup::is_swappable_with_helper<T, U>
    {};


    // is_swappable_with_v (C++14+)
    #if RE_STD_LANG_HAS_VARIABLE_TEMPLATES
        template<typename T,
                 typename U>
        RE_STD_CONSTEXPR bool is_swappable_with_v = is_swappable_with<T, U>::value;
    #endif


}  // re_std


#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER

#endif  // RE_STD_TYPE_TRAITS_IS_SWAPPABLE_WITH_HPP
