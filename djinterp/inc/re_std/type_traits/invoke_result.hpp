/*******************************************************************************
* djinterp [re_std]                                            invoke_result.hpp
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
*   REFERENCE_WRAPPER FORMS (completed 2026-08-25):
*   The standard splits forms 1 and 3 three ways, not two -- the middle
* case is a reference_wrapper argument, which is neither the object
* itself nor a dereferenceable pointer:
*     1b. (t1.get().*f)(args...)  -- f is a pmf, t1 is a reference_wrapper
*     3b. t1.get().*f             -- f is a pmd, t1 is a reference_wrapper
* Without these, invoke_result<Pmf, reference_wrapper<C>> selected the
* pointer-like form, whose `*t1` is ill-formed for a reference_wrapper,
* so the trait reported the call as non-invocable. The pointer-like
* forms are now additionally constrained on !is_reference_wrapper so
* the three-way split is unambiguous.
*
*   This mirrors bullets 2 and 5 of functional/invoke.hpp; the two must
* agree, or invoke_result would disagree with what invoke actually does.
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
* re_std backports to C++11+ since the implementation only needs C++11
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
*   re_std::declval, is_member_pointer, is_member_function_pointer,
* is_member_object_pointer, is_base_of, is_same, decay, enable_if,
* void_t, integral_constant.
*
*
* path:      /inc/re_std/type_traits/invoke_result.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.29
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_TYPE_TRAITS_INVOKE_RESULT_HPP
#define RE_STD_TYPE_TRAITS_INVOKE_RESULT_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_IS_CPP11_OR_HIGHER

// re_std
#include "./true_type.hpp"
#include "./false_type.hpp"
#include "./integral_constant.hpp"
#include "./enable_if.hpp"
#include "./void_t.hpp"
#include "./is_same.hpp"
#include "./is_base_of.hpp"
// forms 1b / 3b: reference_wrapper is neither the object nor a pointer.
// is_reference_wrapper lives in its own header precisely so traits like
// this one can dispatch on it without pulling in reference_wrapper's
// definition -- see functional/is_reference_wrapper.hpp.
#include "../functional/is_reference_wrapper.hpp"
#include "./is_member_pointer.hpp"
#include "./is_member_function_pointer.hpp"
#include "./is_member_object_pointer.hpp"
#include "./decay.hpp"
#include "../utility/declval.hpp"


namespace re_std
{


    namespace internal
    {

        // member_class
        //   trait: extracts the class type from a pointer-to-member type.
        //          Yields `void` for non-member-pointer types so that the
        //          downstream is_base_of / is_same checks cleanly evaluate
        //          to false rather than ill-forming.
        template<typename T>
        struct member_class
        {
            typedef void type;
        };

        // member_class<M C::*>
        //   trait: specialization matching any pointer-to-member of class
        //          C (whether data or function member).
        template<typename M,
                 typename C>
        struct member_class<M C::*>
        {
            typedef C type;
        };

        // is_base_or_same
        //   trait: true_type if Sub is Base or derived from Base;
        //          false_type otherwise. Wrapper to keep the enable_if
        //          expressions in the dispatcher readable.
        template<typename Base,
                 typename Sub>
        struct is_base_or_same
            : integral_constant<
                  bool,
                  (    is_same<Base, Sub>::value
                    || is_base_of<Base, Sub>::value ) >
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
            template<typename Pmf,
                     typename T1,
                     typename... Args>
            static auto do_invoke(Pmf f, T1&& t1, Args&&... args)
                noexcept( noexcept(
                    ( static_cast<T1&&>(t1) .* f )
                    ( static_cast<Args&&>(args)... ) ) )
                -> typename enable_if<
                       (    is_member_function_pointer<Pmf>::value
                         && is_base_or_same<
                                typename member_class<Pmf>::type,
                                typename decay<T1>::type >::value ),
                       decltype(
                           ( static_cast<T1&&>(t1) .* f )
                           ( static_cast<Args&&>(args)... ) ) >::type;

            // form 1b: (t1.get().*f)(args...)
            //   pmf, t1 is a reference_wrapper. Constrained ONLY on
            //   is_reference_wrapper -- the wrapped type does not have to
            //   relate to the pmf's class, because a reference_wrapper of
            //   an unrelated type simply makes .get().*f ill-formed and
            //   SFINAEs this overload away on its own.
            template<typename Pmf,
                     typename T1,
                     typename... Args>
            static auto do_invoke(Pmf f, T1&& t1, Args&&... args)
                noexcept( noexcept(
                    ( t1.get() .* f )
                    ( static_cast<Args&&>(args)... ) ) )
                -> typename enable_if<
                       (    is_member_function_pointer<Pmf>::value
                         && is_reference_wrapper<
                                typename decay<T1>::type >::value ),
                       decltype(
                           ( t1.get() .* f )
                           ( static_cast<Args&&>(args)... ) ) >::type;

            // form 2: ((*t1).*f)(args...)
            //   pmf, t1 is pointer-like (smart pointer, raw pointer).
            template<typename Pmf,
                     typename T1,
                     typename... Args>
            static auto do_invoke(Pmf f, T1&& t1, Args&&... args)
                noexcept( noexcept(
                    ( ( *static_cast<T1&&>(t1) ) .* f )
                    ( static_cast<Args&&>(args)... ) ) )
                -> typename enable_if<
                       (    is_member_function_pointer<Pmf>::value
                         && !is_base_or_same<
                                typename member_class<Pmf>::type,
                                typename decay<T1>::type >::value
                         && !is_reference_wrapper<
                                typename decay<T1>::type >::value ),
                       decltype(
                           ( ( *static_cast<T1&&>(t1) ) .* f )
                           ( static_cast<Args&&>(args)... ) ) >::type;

            // form 3: t1.*f
            //   pmd, t1 is an object reference (or derived).
            template<typename Pmd,
                     typename T1>
            static auto do_invoke(Pmd f, T1&& t1)
                noexcept( noexcept(
                    static_cast<T1&&>(t1) .* f ) )
                -> typename enable_if<
                       (    is_member_object_pointer<Pmd>::value
                         && is_base_or_same<
                                typename member_class<Pmd>::type,
                                typename decay<T1>::type >::value ),
                       decltype( static_cast<T1&&>(t1) .* f ) >::type;

            // form 3b: t1.get().*f
            //   pmd, t1 is a reference_wrapper.
            template<typename Pmd,
                     typename T1>
            static auto do_invoke(Pmd f, T1&& t1)
                noexcept( noexcept( t1.get() .* f ) )
                -> typename enable_if<
                       (    is_member_object_pointer<Pmd>::value
                         && is_reference_wrapper<
                                typename decay<T1>::type >::value ),
                       decltype( t1.get() .* f ) >::type;

            // form 4: (*t1).*f
            //   pmd, t1 is pointer-like.
            template<typename Pmd,
                     typename T1>
            static auto do_invoke(Pmd f, T1&& t1)
                noexcept( noexcept(
                    ( *static_cast<T1&&>(t1) ) .* f ) )
                -> typename enable_if<
                       (    is_member_object_pointer<Pmd>::value
                         && !is_base_or_same<
                                typename member_class<Pmd>::type,
                                typename decay<T1>::type >::value
                         && !is_reference_wrapper<
                                typename decay<T1>::type >::value ),
                       decltype( ( *static_cast<T1&&>(t1) ) .* f )
                       >::type;

            // form 5: f(args...)
            //   plain call -- function pointer, function reference,
            //   functor, lambda. F is forwarded to preserve cv/ref.
            template<typename F,
                     typename... Args>
            static auto do_invoke(F&& f, Args&&... args)
                noexcept( noexcept(
                    static_cast<F&&>(f)
                    ( static_cast<Args&&>(args)... ) ) )
                -> typename enable_if<
                       !is_member_pointer<typename decay<F>::type>::value,
                       decltype(
                           static_cast<F&&>(f)
                           ( static_cast<Args&&>(args)... ) ) >::type;

        };

        // invoke_result_impl
        //   trait: SFINAE-friendly result-type computation. Primary template
        //          has no `type` member; the partial specialization defines
        //          `type` only when the INVOKE expression is well-formed.
        //          The leading `Void` parameter is the void_t hook that
        //          drives the SFINAE selection.
        template<typename Void,
                 typename F,
                 typename... Args>
        struct invoke_result_impl
        {};

        // invoke_result_impl<void, F, Args...>
        //   trait: specialization; selected when the INVOKE expression
        //          is well-formed (void_t collapses to void).
        template<typename F,
                 typename... Args>
        struct invoke_result_impl<
            re_std::void_t<decltype(
                invoker::do_invoke( re_std::declval<F>(),
                                    re_std::declval<Args>()... ) )>,
            F, Args...>
        {
            typedef decltype(
                invoker::do_invoke( re_std::declval<F>(),
                                    re_std::declval<Args>()... ) ) type;
        };

    }  // internal


    // invoke_result
    //   trait: yields `type` as the return type of INVOKE(F, Args...)
    //          when well-formed; has no `type` member otherwise.
    template<typename F,
             typename... Args>
    struct invoke_result
        : internal::invoke_result_impl<void, F, Args...>
    {};


    // invoke_result_t (C++14+)
    #if RE_STD_LANG_HAS_ALIAS_TEMPLATES
        template<typename F,
                 typename... Args>
        using invoke_result_t = typename invoke_result<F, Args...>::type;
    #endif


}  // re_std


#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER

#endif  // RE_STD_TYPE_TRAITS_INVOKE_RESULT_HPP
