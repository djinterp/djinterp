/******************************************************************************
* djinterp [restd]                                            invoke_result.hpp
*
* invoke_result trait + INVOKE dispatcher machinery:
*   Yields `type` as the return type of INVOKE(F, Args...) when the call is
* well-formed, and has no `type` member otherwise (SFINAE-friendly). This
* file also hosts the internal::invoker dispatcher used by the rest of the
* invocation-family traits.
*
*   INVOKE PROTOCOL:
*   Per [func.require], INVOKE(f, t1, ..., tN) selects one of five forms:
*     1. (t1.*f)(args...)        -- f is a pmf, t1 is an object/derived ref
*     2. ((*t1).*f)(args...)     -- f is a pmf, t1 is pointer-like
*     3. t1.*f                   -- f is a pmd, t1 is an object/derived ref
*     4. (*t1).*f                -- f is a pmd, t1 is pointer-like
*     5. f(args...)              -- everything else (function pointer,
*                                   functor, lambda, function reference)
*
*   reference_wrapper handling (form 1.5, "t1.get().*f") is omitted here
* because restd::reference_wrapper has not yet been ported. When it lands,
* a sixth do_invoke overload should be added with an enable_if constraint
* on is_reference_wrapper<decay<T1>>.
*
*   IMPLEMENTATION TECHNIQUE:
*   internal::invoker holds the five forms as static declaration-only
* template members. They are never called -- they exist only to be probed
* via decltype (for the result type) and noexcept (for noexceptness).
* enable_if constraints on each overload ensure exactly one form is
* selected for any well-formed INVOKE expression. Each form's noexcept
* specifier mirrors its underlying expression, so noexcept(do_invoke(...))
* correctly reports the noexceptness of the target expression rather than
* that of the wrapper itself.
*
*   member_class<T> defaults to `void` for non-member-pointer T so that
* the downstream is_base_of / is_same checks evaluate to false rather
* than ill-forming when F is not a member pointer. This keeps the
* enable_if chains SFINAE-friendly without per-overload guarding.
*
*   PORTABILITY:
*   Available on C++11 and later. Standardized as invoke_result in C++17;
* restd backports to C++11+ since the implementation only needs C++11
* features (decltype, declval, variadic templates, rvalue references,
* trailing return types, enable_if-in-return-position).
*
*   The noexcept(noexcept(<expr>)) specifiers on the do_invoke overloads
* assume that substitution failure inside a noexcept-specifier is
* SFINAE-eligible. This is formally guaranteed since CWG 1330 (resolved
* into C++17 as P0012R1: noexcept becomes part of the function type) and
* is honored in practice by GCC 4.8+, Clang 3.3+, and recent MSVC even on
* C++11/14 mode. libstdc++ relies on the same pattern. If a non-conforming
* older compiler hard-errors during dispatcher instantiation, the fix is
* to move the noexcept probe out of do_invoke and into a per-form helper
* trait that is only instantiated after enable_if SFINAE has matched.
*
*   DEPENDENCIES:
*   restd::declval, is_member_pointer, is_member_function_pointer,
* is_member_object_pointer, is_base_of, is_same, decay, enable_if,
* void_t, integral_constant.
*
*
* path:      /inc/djinterp/re_std/type_traits/invoke_result.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                     created: 2026.04.29
******************************************************************************/

#ifndef DJINTERP_RESTD_TYPE_TRAITS_INVOKE_RESULT_
#define DJINTERP_RESTD_TYPE_TRAITS_INVOKE_RESULT_ 1

// djinterp
#include "../../core/djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// restd
#include "./true_type.hpp"
#include "./false_type.hpp"
#include "./integral_constant.hpp"
#include "./enable_if.hpp"
#include "./void_t.hpp"
#include "./is_same.hpp"
#include "./is_base_of.hpp"
#include "./is_member_pointer.hpp"
#include "./is_member_function_pointer.hpp"
#include "./is_member_object_pointer.hpp"
#include "./decay.hpp"
#include "../utility/declval.hpp"


NS_RESTD


    NS_INTERNAL

        // member_class
        //   trait: extracts the class type from a pointer-to-member type.
        //          Yields `void` for non-member-pointer types so that the
        //          downstream is_base_of / is_same checks cleanly evaluate
        //          to false rather than ill-forming.
        template<typename _T>
        struct member_class
        {
            typedef void type;
        };

        // member_class<_M _C::*>
        //   trait: specialization matching any pointer-to-member of class
        //          _C (whether data or function member).
        template<typename _M,
                 typename _C>
        struct member_class<_M _C::*>
        {
            typedef _C type;
        };

        // is_base_or_same
        //   trait: true_type if _Sub is _Base or derived from _Base;
        //          false_type otherwise. Wrapper to keep the enable_if
        //          expressions in the dispatcher readable.
        template<typename _Base,
                 typename _Sub>
        struct is_base_or_same
            : integral_constant<
                  bool,
                  (    is_same<_Base, _Sub>::value
                    || is_base_of<_Base, _Sub>::value ) >
        {};

        // invoker
        //   class: holds the five INVOKE-form overloads as static
        //          declaration-only template members. The overloads are
        //          never called; they exist purely to be used as decltype
        //          and noexcept operands to derive the result type and
        //          noexceptness of an INVOKE expression. enable_if
        //          constraints ensure exactly one form is selected for
        //          any well-formed INVOKE call. The noexcept specifier
        //          on each overload mirrors its underlying expression so
        //          that noexcept(do_invoke(...)) reports the noexceptness
        //          of the target, not of the wrapper.
        struct invoker
        {

            // form 1: (t1.*f)(args...)
            //   pmf, t1 is an object reference (or derived).
            template<typename _Pmf,
                     typename _T1,
                     typename... _Args>
            static auto do_invoke(_Pmf f, _T1&& t1, _Args&&... args)
                noexcept( noexcept(
                    ( static_cast<_T1&&>(t1) .* f )
                    ( static_cast<_Args&&>(args)... ) ) )
                -> typename enable_if<
                       (    is_member_function_pointer<_Pmf>::value
                         && is_base_or_same<
                                typename member_class<_Pmf>::type,
                                typename decay<_T1>::type >::value ),
                       decltype(
                           ( static_cast<_T1&&>(t1) .* f )
                           ( static_cast<_Args&&>(args)... ) ) >::type;

            // form 2: ((*t1).*f)(args...)
            //   pmf, t1 is pointer-like (smart pointer, raw pointer).
            template<typename _Pmf,
                     typename _T1,
                     typename... _Args>
            static auto do_invoke(_Pmf f, _T1&& t1, _Args&&... args)
                noexcept( noexcept(
                    ( ( *static_cast<_T1&&>(t1) ) .* f )
                    ( static_cast<_Args&&>(args)... ) ) )
                -> typename enable_if<
                       (    is_member_function_pointer<_Pmf>::value
                         && !is_base_or_same<
                                typename member_class<_Pmf>::type,
                                typename decay<_T1>::type >::value ),
                       decltype(
                           ( ( *static_cast<_T1&&>(t1) ) .* f )
                           ( static_cast<_Args&&>(args)... ) ) >::type;

            // form 3: t1.*f
            //   pmd, t1 is an object reference (or derived).
            template<typename _Pmd,
                     typename _T1>
            static auto do_invoke(_Pmd f, _T1&& t1)
                noexcept( noexcept(
                    static_cast<_T1&&>(t1) .* f ) )
                -> typename enable_if<
                       (    is_member_object_pointer<_Pmd>::value
                         && is_base_or_same<
                                typename member_class<_Pmd>::type,
                                typename decay<_T1>::type >::value ),
                       decltype( static_cast<_T1&&>(t1) .* f ) >::type;

            // form 4: (*t1).*f
            //   pmd, t1 is pointer-like.
            template<typename _Pmd,
                     typename _T1>
            static auto do_invoke(_Pmd f, _T1&& t1)
                noexcept( noexcept(
                    ( *static_cast<_T1&&>(t1) ) .* f ) )
                -> typename enable_if<
                       (    is_member_object_pointer<_Pmd>::value
                         && !is_base_or_same<
                                typename member_class<_Pmd>::type,
                                typename decay<_T1>::type >::value ),
                       decltype( ( *static_cast<_T1&&>(t1) ) .* f )
                       >::type;

            // form 5: f(args...)
            //   plain call -- function pointer, function reference,
            //   functor, lambda. F is forwarded to preserve cv/ref.
            template<typename _F,
                     typename... _Args>
            static auto do_invoke(_F&& f, _Args&&... args)
                noexcept( noexcept(
                    static_cast<_F&&>(f)
                    ( static_cast<_Args&&>(args)... ) ) )
                -> typename enable_if<
                       !is_member_pointer<typename decay<_F>::type>::value,
                       decltype(
                           static_cast<_F&&>(f)
                           ( static_cast<_Args&&>(args)... ) ) >::type;

        };

        // invoke_result_impl
        //   trait: SFINAE-friendly result-type computation. Primary template
        //          has no `type` member; the partial specialization defines
        //          `type` only when the INVOKE expression is well-formed.
        //          The leading `_Void` parameter is the void_t hook that
        //          drives the SFINAE selection.
        template<typename _Void,
                 typename _F,
                 typename... _Args>
        struct invoke_result_impl
        {};

        // invoke_result_impl<void, _F, _Args...>
        //   trait: specialization; selected when the INVOKE expression
        //          is well-formed (void_t collapses to void).
        template<typename _F,
                 typename... _Args>
        struct invoke_result_impl<
            restd::void_t<decltype(
                invoker::do_invoke( restd::declval<_F>(),
                                    restd::declval<_Args>()... ) )>,
            _F, _Args...>
        {
            typedef decltype(
                invoker::do_invoke( restd::declval<_F>(),
                                    restd::declval<_Args>()... ) ) type;
        };

    NS_END  // internal


    // invoke_result
    //   trait: yields `type` as the return type of INVOKE(F, Args...)
    //          when well-formed; has no `type` member otherwise.
    template<typename _F,
             typename... _Args>
    struct invoke_result
        : internal::invoke_result_impl<void, _F, _Args...>
    {};


    // invoke_result_t (C++14+)
    #if D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES
        template<typename _F,
                 typename... _Args>
        using invoke_result_t = typename invoke_result<_F, _Args...>::type;
    #endif


NS_END  // restd


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // DJINTERP_RESTD_TYPE_TRAITS_INVOKE_RESULT_
