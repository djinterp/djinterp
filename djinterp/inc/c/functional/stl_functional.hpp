/******************************************************************************
* djinterp [core]                                            stl_functional.hpp
*
* Backported STL functional utilities for cross-version compatibility.
*   This header provides implementations of standard library functional
* utilities that exist in newer C++ standards but not older ones. Native
* implementations are used when available.
*
* BACKPORTED UTILITIES:
*   C++17:
*     - invoke, invoke_result, invoke_result_t
*     - is_invocable, is_invocable_r, is_nothrow_invocable (+ _v helpers)
*     - not_fn
*   C++20:
*     - identity
*     - bind_front (requires C++14 minimum)
*   Pre-C++14:
*     - Transparent comparators (less<>, greater<>, equal_to<>)
*
* path:      \inc\stl_functional.hpp
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.01.15
******************************************************************************/

#ifndef DJINTERP_STL_FUNCTIONAL_
#define DJINTERP_STL_FUNCTIONAL_ 1

#include <cstddef>
#include <functional>
#include <tuple>
#include <type_traits>
#include <utility>
#include ".\env.h"
#include ".\cpp_features.h"
#include ".\djinterp.h"


///////////////////////////////////////////////////////////////////////////////
///             I.    CONFIGURATION AND FEATURE DETECTION                   ///
///////////////////////////////////////////////////////////////////////////////

// D_STL_FUNCTIONAL_HAS_INVOKE
//   constant: 1 if std::invoke is available (C++17).
#ifndef D_STL_FUNCTIONAL_HAS_INVOKE
    #if D_ENV_CPP_FEATURE_STL_INVOKE
        #define D_STL_FUNCTIONAL_HAS_INVOKE 1
    #elif D_ENV_LANG_IS_CPP17_OR_HIGHER
        #define D_STL_FUNCTIONAL_HAS_INVOKE 1
    #else
        #define D_STL_FUNCTIONAL_HAS_INVOKE 0
    #endif
#endif

// D_STL_FUNCTIONAL_HAS_NOT_FN
//   constant: 1 if std::not_fn is available (C++17).
#ifndef D_STL_FUNCTIONAL_HAS_NOT_FN
    #if D_ENV_CPP_FEATURE_STL_NOT_FN
        #define D_STL_FUNCTIONAL_HAS_NOT_FN 1
    #elif D_ENV_LANG_IS_CPP17_OR_HIGHER
        #define D_STL_FUNCTIONAL_HAS_NOT_FN 1
    #else
        #define D_STL_FUNCTIONAL_HAS_NOT_FN 0
    #endif
#endif

// D_STL_FUNCTIONAL_HAS_BIND_FRONT
//   constant: 1 if std::bind_front is available (C++20).
#ifndef D_STL_FUNCTIONAL_HAS_BIND_FRONT
    #if D_ENV_CPP_FEATURE_STL_BIND_FRONT
        #define D_STL_FUNCTIONAL_HAS_BIND_FRONT 1
    #elif D_ENV_LANG_IS_CPP20_OR_HIGHER
        #define D_STL_FUNCTIONAL_HAS_BIND_FRONT 1
    #else
        #define D_STL_FUNCTIONAL_HAS_BIND_FRONT 0
    #endif
#endif

// D_STL_FUNCTIONAL_HAS_BIND_BACK
//   constant: 1 if std::bind_back is available (C++23).
#ifndef D_STL_FUNCTIONAL_HAS_BIND_BACK
    #if D_ENV_CPP_FEATURE_STL_BIND_BACK
        #define D_STL_FUNCTIONAL_HAS_BIND_BACK 1
    #elif D_ENV_LANG_IS_CPP23_OR_HIGHER
        #define D_STL_FUNCTIONAL_HAS_BIND_BACK 1
    #else
        #define D_STL_FUNCTIONAL_HAS_BIND_BACK 0
    #endif
#endif

// D_STL_FUNCTIONAL_HAS_IDENTITY
//   constant: 1 if std::identity is available (C++20).
#ifndef D_STL_FUNCTIONAL_HAS_IDENTITY
    #if D_ENV_CPP_FEATURE_STL_RANGES
        #define D_STL_FUNCTIONAL_HAS_IDENTITY 1
    #elif D_ENV_LANG_IS_CPP20_OR_HIGHER
        #define D_STL_FUNCTIONAL_HAS_IDENTITY 1
    #else
        #define D_STL_FUNCTIONAL_HAS_IDENTITY 0
    #endif
#endif

// D_STL_FUNCTIONAL_HAS_MOVE_ONLY_FUNCTION
//   constant: 1 if std::move_only_function is available (C++23).
#ifndef D_STL_FUNCTIONAL_HAS_MOVE_ONLY_FUNCTION
    #if D_ENV_CPP_FEATURE_STL_MOVE_ONLY_FUNCTION
        #define D_STL_FUNCTIONAL_HAS_MOVE_ONLY_FUNCTION 1
    #elif D_ENV_LANG_IS_CPP23_OR_HIGHER
        #define D_STL_FUNCTIONAL_HAS_MOVE_ONLY_FUNCTION 1
    #else
        #define D_STL_FUNCTIONAL_HAS_MOVE_ONLY_FUNCTION 0
    #endif
#endif

// D_STL_FUNCTIONAL_HAS_TRANSPARENT_OPS
//   constant: 1 if transparent operator functors are available (C++14).
#ifndef D_STL_FUNCTIONAL_HAS_TRANSPARENT_OPS
    #if D_ENV_LANG_IS_CPP14_OR_HIGHER
        #define D_STL_FUNCTIONAL_HAS_TRANSPARENT_OPS 1
    #else
        #define D_STL_FUNCTIONAL_HAS_TRANSPARENT_OPS 0
    #endif
#endif

// D_STL_FUNCTIONAL_CONSTEXPR
//   macro: expands to constexpr if C++14 or higher (relaxed constexpr).
#if D_ENV_LANG_IS_CPP14_OR_HIGHER
    #define D_STL_FUNCTIONAL_CONSTEXPR constexpr
#else
    #define D_STL_FUNCTIONAL_CONSTEXPR
#endif

// D_STL_FUNCTIONAL_CONSTEXPR_17
//   macro: expands to constexpr if C++17 or higher.
#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    #define D_STL_FUNCTIONAL_CONSTEXPR_17 constexpr
#else
    #define D_STL_FUNCTIONAL_CONSTEXPR_17
#endif

// D_STL_FUNCTIONAL_CONSTEXPR_20
//   macro: expands to constexpr if C++20 or higher.
#if D_ENV_LANG_IS_CPP20_OR_HIGHER
    #define D_STL_FUNCTIONAL_CONSTEXPR_20 constexpr
#else
    #define D_STL_FUNCTIONAL_CONSTEXPR_20
#endif

// D_STL_FUNCTIONAL_NODISCARD
//   macro: expands to [[nodiscard]] if C++17 or higher.
#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    #define D_STL_FUNCTIONAL_NODISCARD [[nodiscard]]
#else
    #define D_STL_FUNCTIONAL_NODISCARD
#endif


NS_DJINTERP
NS_STL

///////////////////////////////////////////////////////////////////////////////
///             II.   INVOKE BACKPORT (PRE-C++17)                           ///
///////////////////////////////////////////////////////////////////////////////

#if !D_STL_FUNCTIONAL_HAS_INVOKE

NS_INTERNAL

    // invoke_impl
    //   helper: implementation detail for invoke with member function pointers.
    template<typename _Base,
             typename _Type,
             typename _Derived,
             typename... _Args>
    auto invoke_impl(
        _Type _Base::* _pmf,
        _Derived&&     _ref,
        _Args&&...     _args)
        -> typename std::enable_if<
            std::is_function<_Type>::value &&
            std::is_base_of<_Base, typename std::decay<_Derived>::type>::value,
            decltype((std::forward<_Derived>(_ref).*_pmf)(
                std::forward<_Args>(_args)...))>::type
    {
        return (std::forward<_Derived>(_ref).*_pmf)(
            std::forward<_Args>(_args)...);
    }

    // invoke_impl
    //   helper: implementation for dereferenced pointer to member function.
    template<typename _Base,
             typename _Type,
             typename _RefWrapper,
             typename... _Args>
    auto invoke_impl(
        _Type _Base::* _pmf,
        _RefWrapper&&  _ref,
        _Args&&...     _args)
        -> typename std::enable_if<
            std::is_function<_Type>::value &&
            !std::is_base_of<_Base, typename std::decay<_RefWrapper>::type>::value,
            decltype(((*std::forward<_RefWrapper>(_ref)).*_pmf)(
                std::forward<_Args>(_args)...))>::type
    {
        return ((*std::forward<_RefWrapper>(_ref)).*_pmf)(
            std::forward<_Args>(_args)...);
    }

    // invoke_impl
    //   helper: implementation for member data pointer with direct access.
    template<typename _Base,
             typename _Type,
             typename _Derived>
    auto invoke_impl(
        _Type _Base::* _pmd,
        _Derived&&     _ref)
        -> typename std::enable_if<
            !std::is_function<_Type>::value &&
            std::is_base_of<_Base, typename std::decay<_Derived>::type>::value,
            decltype(std::forward<_Derived>(_ref).*_pmd)>::type
    {
        return std::forward<_Derived>(_ref).*_pmd;
    }

    // invoke_impl
    //   helper: implementation for member data pointer with indirect access.
    template<typename _Base,
             typename _Type,
             typename _Ptr>
    auto invoke_impl(
        _Type _Base::* _pmd,
        _Ptr&&         _ptr)
        -> typename std::enable_if<
            !std::is_function<_Type>::value &&
            !std::is_base_of<_Base, typename std::decay<_Ptr>::type>::value,
            decltype((*std::forward<_Ptr>(_ptr)).*_pmd)>::type
    {
        return (*std::forward<_Ptr>(_ptr)).*_pmd;
    }

    // invoke_impl
    //   helper: implementation for regular callable objects.
    template<typename _Fn,
             typename... _Args>
    auto invoke_impl(_Fn&&      _fn,
                     _Args&&... _args)
        -> typename std::enable_if<
            !std::is_member_pointer<typename std::decay<_Fn>::type>::value,
            decltype(std::forward<_Fn>(_fn)(
                std::forward<_Args>(_args)...))>::type
    {
        return std::forward<_Fn>(_fn)(std::forward<_Args>(_args)...);
    }

NS_END  // internal

// invoke
//   function: invokes a callable object with the given arguments.
// Backport of std::invoke for pre-C++17 compilers.
template<typename _Fn,
         typename... _Args>
auto invoke(_Fn&&      _fn,
            _Args&&... _args)
    -> decltype(internal::invoke_impl(std::forward<_Fn>(_fn),
                                      std::forward<_Args>(_args)...))
{
    return internal::invoke_impl(std::forward<_Fn>(_fn),
                                 std::forward<_Args>(_args)...);
}

#else

// use standard invoke when available
using std::invoke;

#endif  // !D_STL_FUNCTIONAL_HAS_INVOKE


///////////////////////////////////////////////////////////////////////////////
///             III.  INVOKE_RESULT BACKPORT (PRE-C++17)                    ///
///////////////////////////////////////////////////////////////////////////////

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// use standard invoke_result when available
template<typename _Fn,
         typename... _Args>
using invoke_result = std::invoke_result<_Fn, _Args...>;

template<typename _Fn,
         typename... _Args>
using invoke_result_t = std::invoke_result_t<_Fn, _Args...>;

#else

NS_INTERNAL

    // invoke_result_helper
    //   helper: SFINAE-based invoke result detection.
    template<typename _AlwaysVoid,
             typename _Fn,
             typename... _Args>
    struct invoke_result_helper
    {};

    template<typename _Fn,
             typename... _Args>
    struct invoke_result_helper<
        decltype(void(invoke(std::declval<_Fn>(), std::declval<_Args>()...))),
        _Fn,
        _Args...>
    {
        using type = decltype(invoke(std::declval<_Fn>(),
                                     std::declval<_Args>()...));
    };

NS_END  // internal

// invoke_result
//   type trait: determines the result type of invoking a callable.
// Backport of std::invoke_result for pre-C++17 compilers.
template<typename _Fn,
         typename... _Args>
struct invoke_result : internal::invoke_result_helper<void, _Fn, _Args...>
{};

template<typename _Fn,
         typename... _Args>
using invoke_result_t = typename invoke_result<_Fn, _Args...>::type;

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER


///////////////////////////////////////////////////////////////////////////////
///             IV.   IS_INVOCABLE BACKPORT (PRE-C++17)                     ///
///////////////////////////////////////////////////////////////////////////////

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// use standard type traits when available
template<typename _Fn,
         typename... _Args>
using is_invocable = std::is_invocable<_Fn, _Args...>;

template<typename _Fn,
         typename... _Args>
inline constexpr bool is_invocable_v = std::is_invocable_v<_Fn, _Args...>;

template<typename _Ret,
         typename _Fn,
         typename... _Args>
using is_invocable_r = std::is_invocable_r<_Ret, _Fn, _Args...>;

template<typename _Ret,
         typename _Fn,
         typename... _Args>
inline constexpr bool is_invocable_r_v = std::is_invocable_r_v<_Ret, _Fn, _Args...>;

template<typename _Fn,
         typename... _Args>
using is_nothrow_invocable = std::is_nothrow_invocable<_Fn, _Args...>;

template<typename _Fn,
         typename... _Args>
inline constexpr bool is_nothrow_invocable_v = 
    std::is_nothrow_invocable_v<_Fn, _Args...>;

template<typename _Ret,
         typename _Fn,
         typename... _Args>
using is_nothrow_invocable_r = std::is_nothrow_invocable_r<_Ret, _Fn, _Args...>;

template<typename _Ret,
         typename _Fn,
         typename... _Args>
inline constexpr bool is_nothrow_invocable_r_v = 
    std::is_nothrow_invocable_r_v<_Ret, _Fn, _Args...>;

#else

NS_INTERNAL

    // is_invocable_impl
    //   helper: primary template (not invocable).
    template<typename _Fn,
             typename _ArgsTuple,
             typename = void>
    struct is_invocable_impl : std::false_type
    {};

    // is_invocable_impl
    //   helper: specialization for invocable case.
    template<typename _Fn,
             typename... _Args>
    struct is_invocable_impl<_Fn, std::tuple<_Args...>,
        decltype(void(invoke(std::declval<_Fn>(),
                             std::declval<_Args>()...)))>
        : std::true_type
    {};

NS_END  // internal

// is_invocable
//   type trait: determines if a callable can be invoked with given arguments.
// Backport of std::is_invocable for pre-C++17 compilers.
template<typename _Fn,
         typename... _Args>
struct is_invocable 
    : internal::is_invocable_impl<_Fn, std::tuple<_Args...>>
{};

#if D_ENV_LANG_IS_CPP14_OR_HIGHER
template<typename _Fn,
         typename... _Args>
inline constexpr bool is_invocable_v = is_invocable<_Fn, _Args...>::value;
#endif

// is_invocable_r
//   type trait: determines if result of invocation is convertible to _Ret.
template<typename _Ret,
         typename _Fn,
         typename... _Args>
struct is_invocable_r
{
private:
    template<typename,
             typename = void>
    struct check : std::false_type
    {};

    template<typename _Dummy>
    struct check<_Dummy,
        typename std::enable_if<
            is_invocable<_Fn, _Args...>::value &&
            (std::is_void<_Ret>::value ||
             std::is_convertible<invoke_result_t<_Fn, _Args...>, _Ret>::value)
        >::type>
        : std::true_type
    {};

public:
    static constexpr bool value = check<void>::value;
};

#if D_ENV_LANG_IS_CPP14_OR_HIGHER
template<typename _Ret,
         typename _Fn,
         typename... _Args>
inline constexpr bool is_invocable_r_v = is_invocable_r<_Ret, _Fn, _Args...>::value;
#endif

// is_nothrow_invocable
//   type trait: determines if a callable can be invoked without throwing.
template<typename _Fn,
         typename... _Args>
struct is_nothrow_invocable
{
private:
    template<typename,
             typename = void>
    struct check : std::false_type
    {};

    template<typename _Dummy>
    struct check<_Dummy,
        typename std::enable_if<
            is_invocable<_Fn, _Args...>::value &&
            noexcept(invoke(std::declval<_Fn>(),
                            std::declval<_Args>()...))
        >::type>
        : std::true_type
    {};

public:
    static constexpr bool value = check<void>::value;
};

#if D_ENV_LANG_IS_CPP14_OR_HIGHER
template<typename _Fn,
         typename... _Args>
inline constexpr bool is_nothrow_invocable_v = 
    is_nothrow_invocable<_Fn, _Args...>::value;
#endif

// is_nothrow_invocable_r
//   type trait: determines if invocation is nothrow and result converts to _Ret.
template<typename _Ret,
         typename _Fn,
         typename... _Args>
struct is_nothrow_invocable_r
{
private:
    template<typename,
             typename = void>
    struct check : std::false_type
    {};

    template<typename _Dummy>
    struct check<_Dummy,
        typename std::enable_if<
            is_nothrow_invocable<_Fn, _Args...>::value &&
            (std::is_void<_Ret>::value ||
             std::is_convertible<invoke_result_t<_Fn, _Args...>, _Ret>::value)
        >::type>
        : std::true_type
    {};

public:
    static constexpr bool value = check<void>::value;
};

#if D_ENV_LANG_IS_CPP14_OR_HIGHER
template<typename _Ret,
         typename _Fn,
         typename... _Args>
inline constexpr bool is_nothrow_invocable_r_v = 
    is_nothrow_invocable_r<_Ret, _Fn, _Args...>::value;
#endif

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER


///////////////////////////////////////////////////////////////////////////////
///             V.    IDENTITY BACKPORT (PRE-C++20)                         ///
///////////////////////////////////////////////////////////////////////////////

#if !D_STL_FUNCTIONAL_HAS_IDENTITY

// identity
//   functor: returns its argument unchanged.
// Backport of std::identity for pre-C++20 compilers.
struct identity
{
    template<typename _Type>
    D_STL_FUNCTIONAL_NODISCARD
    D_STL_FUNCTIONAL_CONSTEXPR
    _Type&& operator()(_Type&& _t) const noexcept
    {
        return std::forward<_Type>(_t);
    }

    using is_transparent = void;
};

#else

using std::identity;

#endif  // !D_STL_FUNCTIONAL_HAS_IDENTITY


///////////////////////////////////////////////////////////////////////////////
///             VI.   NOT_FN BACKPORT (PRE-C++17)                           ///
///////////////////////////////////////////////////////////////////////////////

#if !D_STL_FUNCTIONAL_HAS_NOT_FN

NS_INTERNAL

    // not_fn_impl
    //   helper: implementation class for not_fn.
    template<typename _Fn>
    class not_fn_impl
    {
    private:
        _Fn m_fn;

    public:
        template<typename _FnFwd>
        explicit D_STL_FUNCTIONAL_CONSTEXPR
        not_fn_impl(_FnFwd&& _fn)
            : m_fn(std::forward<_FnFwd>(_fn))
        {}

        not_fn_impl(const not_fn_impl&)            = default;
        not_fn_impl(not_fn_impl&&)                 = default;
        not_fn_impl& operator=(const not_fn_impl&) = default;
        not_fn_impl& operator=(not_fn_impl&&)      = default;

        // call operators for all cv/ref combinations
        template<typename... _Args>
        D_STL_FUNCTIONAL_CONSTEXPR
        auto operator()(_Args&&... _args) &
            -> decltype(!invoke(std::declval<_Fn&>(),
                               std::forward<_Args>(_args)...))
        {
            return !invoke(m_fn, std::forward<_Args>(_args)...);
        }

        template<typename... _Args>
        D_STL_FUNCTIONAL_CONSTEXPR
        auto operator()(_Args&&... _args) const&
            -> decltype(!invoke(std::declval<const _Fn&>(),
                               std::forward<_Args>(_args)...))
        {
            return !invoke(m_fn, std::forward<_Args>(_args)...);
        }

        template<typename... _Args>
        D_STL_FUNCTIONAL_CONSTEXPR
        auto operator()(_Args&&... _args) &&
            -> decltype(!invoke(std::declval<_Fn>(),
                               std::forward<_Args>(_args)...))
        {
            return !invoke(std::move(m_fn), std::forward<_Args>(_args)...);
        }

        template<typename... _Args>
        D_STL_FUNCTIONAL_CONSTEXPR
        auto operator()(_Args&&... _args) const&&
            -> decltype(!invoke(std::declval<const _Fn>(),
                               std::forward<_Args>(_args)...))
        {
            return !invoke(std::move(m_fn), std::forward<_Args>(_args)...);
        }
    };

NS_END  // internal

// not_fn
//   function: creates a call wrapper that negates the result of its callable.
// Backport of std::not_fn for pre-C++17 compilers.
template<typename _Fn>
D_STL_FUNCTIONAL_CONSTEXPR
internal::not_fn_impl<typename std::decay<_Fn>::type>
not_fn(_Fn&& _fn)
{
    return internal::not_fn_impl<typename std::decay<_Fn>::type>(
        std::forward<_Fn>(_fn));
}

#else

using std::not_fn;

#endif  // !D_STL_FUNCTIONAL_HAS_NOT_FN


///////////////////////////////////////////////////////////////////////////////
///             VII.  BIND_FRONT BACKPORT (PRE-C++20)                       ///
///////////////////////////////////////////////////////////////////////////////

#if !D_STL_FUNCTIONAL_HAS_BIND_FRONT

// bind_front requires C++14 for index_sequence
#if D_ENV_LANG_IS_CPP14_OR_HIGHER

NS_INTERNAL

    // bind_front_impl
    //   helper: implementation class for bind_front.
    template<typename _Fn,
             typename... _BoundArgs>
    class bind_front_impl
    {
    private:
        using bound_tuple_type = std::tuple<_BoundArgs...>;

        _Fn              m_fn;
        bound_tuple_type m_bound_args;

        template<std::size_t... _Indices,
                 typename...    _CallArgs>
        D_STL_FUNCTIONAL_CONSTEXPR
        auto call_impl(std::index_sequence<_Indices...>,
                       _CallArgs&&... _call_args) &
            -> invoke_result_t<_Fn&,
                               _BoundArgs&...,
                               _CallArgs...>
        {
            return invoke(m_fn,
                          std::get<_Indices>(m_bound_args)...,
                          std::forward<_CallArgs>(_call_args)...);
        }

        template<std::size_t... _Indices,
                 typename...    _CallArgs>
        D_STL_FUNCTIONAL_CONSTEXPR
        auto call_impl(std::index_sequence<_Indices...>,
                       _CallArgs&&... _call_args) const&
            -> invoke_result_t<const _Fn&,
                               const _BoundArgs&...,
                               _CallArgs...>
        {
            return invoke(m_fn,
                          std::get<_Indices>(m_bound_args)...,
                          std::forward<_CallArgs>(_call_args)...);
        }

        template<std::size_t... _Indices,
                 typename...    _CallArgs>
        D_STL_FUNCTIONAL_CONSTEXPR
        auto call_impl(std::index_sequence<_Indices...>,
                       _CallArgs&&... _call_args) &&
            -> invoke_result_t<_Fn,
                               _BoundArgs...,
                               _CallArgs...>
        {
            return invoke(std::move(m_fn),
                          std::get<_Indices>(std::move(m_bound_args))...,
                          std::forward<_CallArgs>(_call_args)...);
        }

        template<std::size_t... _Indices,
                 typename...    _CallArgs>
        D_STL_FUNCTIONAL_CONSTEXPR
        auto call_impl(std::index_sequence<_Indices...>,
                       _CallArgs&&... _call_args) const&&
            -> invoke_result_t<const _Fn,
                               const _BoundArgs...,
                               _CallArgs...>
        {
            return invoke(std::move(m_fn),
                          std::get<_Indices>(std::move(m_bound_args))...,
                          std::forward<_CallArgs>(_call_args)...);
        }

    public:
        template<typename _FnFwd,
                 typename... _BoundArgsFwd>
        explicit D_STL_FUNCTIONAL_CONSTEXPR
        bind_front_impl(_FnFwd&&           _fn,
                        _BoundArgsFwd&&... _bound_args)
            : m_fn(std::forward<_FnFwd>(_fn))
            , m_bound_args(std::forward<_BoundArgsFwd>(_bound_args)...)
        {}

        bind_front_impl(const bind_front_impl&)            = default;
        bind_front_impl(bind_front_impl&&)                 = default;
        bind_front_impl& operator=(const bind_front_impl&) = default;
        bind_front_impl& operator=(bind_front_impl&&)      = default;

        template<typename... _CallArgs>
        D_STL_FUNCTIONAL_CONSTEXPR
        auto operator()(_CallArgs&&... _call_args) &
            -> decltype(call_impl(std::index_sequence_for<_BoundArgs...>{},
                                  std::forward<_CallArgs>(_call_args)...))
        {
            return call_impl(std::index_sequence_for<_BoundArgs...>{},
                            std::forward<_CallArgs>(_call_args)...);
        }

        template<typename... _CallArgs>
        D_STL_FUNCTIONAL_CONSTEXPR
        auto operator()(_CallArgs&&... _call_args) const&
            -> decltype(call_impl(std::index_sequence_for<_BoundArgs...>{},
                                  std::forward<_CallArgs>(_call_args)...))
        {
            return call_impl(std::index_sequence_for<_BoundArgs...>{},
                            std::forward<_CallArgs>(_call_args)...);
        }

        template<typename... _CallArgs>
        D_STL_FUNCTIONAL_CONSTEXPR
        auto operator()(_CallArgs&&... _call_args) &&
            -> decltype(std::move(*this).call_impl(
                std::index_sequence_for<_BoundArgs...>{},
                std::forward<_CallArgs>(_call_args)...))
        {
            return std::move(*this).call_impl(
                std::index_sequence_for<_BoundArgs...>{},
                std::forward<_CallArgs>(_call_args)...);
        }

        template<typename... _CallArgs>
        D_STL_FUNCTIONAL_CONSTEXPR
        auto operator()(_CallArgs&&... _call_args) const&&
            -> decltype(std::move(*this).call_impl(
                std::index_sequence_for<_BoundArgs...>{},
                std::forward<_CallArgs>(_call_args)...))
        {
            return std::move(*this).call_impl(
                std::index_sequence_for<_BoundArgs...>{},
                std::forward<_CallArgs>(_call_args)...);
        }
    };

NS_END  // internal

// bind_front
//   function: creates a call wrapper with bound leading arguments.
// Backport of std::bind_front for pre-C++20 compilers.
template<typename _Fn,
         typename... _BoundArgs>
D_STL_FUNCTIONAL_CONSTEXPR
internal::bind_front_impl<typename std::decay<_Fn>::type,
                          typename std::decay<_BoundArgs>::type...>
bind_front(_Fn&&          _fn,
           _BoundArgs&&... _bound_args)
{
    return internal::bind_front_impl<
        typename std::decay<_Fn>::type,
        typename std::decay<_BoundArgs>::type...>(
            std::forward<_Fn>(_fn),
            std::forward<_BoundArgs>(_bound_args)...);
}

#endif  // D_ENV_LANG_IS_CPP14_OR_HIGHER

#else

using std::bind_front;

#endif  // !D_STL_FUNCTIONAL_HAS_BIND_FRONT


///////////////////////////////////////////////////////////////////////////////
///             VIII. BIND_BACK BACKPORT (PRE-C++23)                        ///
///////////////////////////////////////////////////////////////////////////////

#if !D_STL_FUNCTIONAL_HAS_BIND_BACK

// bind_back requires C++14 for index_sequence
#if D_ENV_LANG_IS_CPP14_OR_HIGHER

NS_INTERNAL

    // bind_back_impl
    //   helper: implementation class for bind_back.
    template<typename _Fn,
             typename... _BoundArgs>
    class bind_back_impl
    {
    private:
        using bound_tuple_type = std::tuple<_BoundArgs...>;

        _Fn              m_fn;
        bound_tuple_type m_bound_args;

        template<std::size_t... _Indices,
                 typename...    _CallArgs>
        D_STL_FUNCTIONAL_CONSTEXPR
        auto call_impl(std::index_sequence<_Indices...>,
                       _CallArgs&&... _call_args) &
            -> invoke_result_t<_Fn&,
                               _CallArgs...,
                               _BoundArgs&...>
        {
            return invoke(m_fn,
                          std::forward<_CallArgs>(_call_args)...,
                          std::get<_Indices>(m_bound_args)...);
        }

        template<std::size_t... _Indices,
                 typename...    _CallArgs>
        D_STL_FUNCTIONAL_CONSTEXPR
        auto call_impl(std::index_sequence<_Indices...>,
                       _CallArgs&&... _call_args) const&
            -> invoke_result_t<const _Fn&,
                               _CallArgs...,
                               const _BoundArgs&...>
        {
            return invoke(m_fn,
                          std::forward<_CallArgs>(_call_args)...,
                          std::get<_Indices>(m_bound_args)...);
        }

        template<std::size_t... _Indices,
                 typename...    _CallArgs>
        D_STL_FUNCTIONAL_CONSTEXPR
        auto call_impl(std::index_sequence<_Indices...>,
                       _CallArgs&&... _call_args) &&
            -> invoke_result_t<_Fn,
                               _CallArgs...,
                               _BoundArgs...>
        {
            return invoke(std::move(m_fn),
                          std::forward<_CallArgs>(_call_args)...,
                          std::get<_Indices>(std::move(m_bound_args))...);
        }

        template<std::size_t... _Indices,
                 typename...    _CallArgs>
        D_STL_FUNCTIONAL_CONSTEXPR
        auto call_impl(std::index_sequence<_Indices...>,
                       _CallArgs&&... _call_args) const&&
            -> invoke_result_t<const _Fn,
                               _CallArgs...,
                               const _BoundArgs...>
        {
            return invoke(std::move(m_fn),
                          std::forward<_CallArgs>(_call_args)...,
                          std::get<_Indices>(std::move(m_bound_args))...);
        }

    public:
        template<typename _FnFwd,
                 typename... _BoundArgsFwd>
        explicit D_STL_FUNCTIONAL_CONSTEXPR
        bind_back_impl(_FnFwd&&           _fn,
                       _BoundArgsFwd&&... _bound_args)
            : m_fn(std::forward<_FnFwd>(_fn))
            , m_bound_args(std::forward<_BoundArgsFwd>(_bound_args)...)
        {}

        bind_back_impl(const bind_back_impl&)            = default;
        bind_back_impl(bind_back_impl&&)                 = default;
        bind_back_impl& operator=(const bind_back_impl&) = default;
        bind_back_impl& operator=(bind_back_impl&&)      = default;

        template<typename... _CallArgs>
        D_STL_FUNCTIONAL_CONSTEXPR
        auto operator()(_CallArgs&&... _call_args) &
            -> decltype(call_impl(std::index_sequence_for<_BoundArgs...>{},
                                  std::forward<_CallArgs>(_call_args)...))
        {
            return call_impl(std::index_sequence_for<_BoundArgs...>{},
                            std::forward<_CallArgs>(_call_args)...);
        }

        template<typename... _CallArgs>
        D_STL_FUNCTIONAL_CONSTEXPR
        auto operator()(_CallArgs&&... _call_args) const&
            -> decltype(call_impl(std::index_sequence_for<_BoundArgs...>{},
                                  std::forward<_CallArgs>(_call_args)...))
        {
            return call_impl(std::index_sequence_for<_BoundArgs...>{},
                            std::forward<_CallArgs>(_call_args)...);
        }

        template<typename... _CallArgs>
        D_STL_FUNCTIONAL_CONSTEXPR
        auto operator()(_CallArgs&&... _call_args) &&
            -> decltype(std::move(*this).call_impl(
                std::index_sequence_for<_BoundArgs...>{},
                std::forward<_CallArgs>(_call_args)...))
        {
            return std::move(*this).call_impl(
                std::index_sequence_for<_BoundArgs...>{},
                std::forward<_CallArgs>(_call_args)...);
        }

        template<typename... _CallArgs>
        D_STL_FUNCTIONAL_CONSTEXPR
        auto operator()(_CallArgs&&... _call_args) const&&
            -> decltype(std::move(*this).call_impl(
                std::index_sequence_for<_BoundArgs...>{},
                std::forward<_CallArgs>(_call_args)...))
        {
            return std::move(*this).call_impl(
                std::index_sequence_for<_BoundArgs...>{},
                std::forward<_CallArgs>(_call_args)...);
        }
    };

NS_END  // internal

// bind_back
//   function: creates a call wrapper with bound trailing arguments.
// Backport of std::bind_back for pre-C++23 compilers.
template<typename _Fn,
         typename... _BoundArgs>
D_STL_FUNCTIONAL_CONSTEXPR
internal::bind_back_impl<typename std::decay<_Fn>::type,
                         typename std::decay<_BoundArgs>::type...>
bind_back(_Fn&&          _fn,
          _BoundArgs&&... _bound_args)
{
    return internal::bind_back_impl<
        typename std::decay<_Fn>::type,
        typename std::decay<_BoundArgs>::type...>(
            std::forward<_Fn>(_fn),
            std::forward<_BoundArgs>(_bound_args)...);
}

#endif  // D_ENV_LANG_IS_CPP14_OR_HIGHER

#else

using std::bind_back;

#endif  // !D_STL_FUNCTIONAL_HAS_BIND_BACK


///////////////////////////////////////////////////////////////////////////////
///             IX.   TRANSPARENT COMPARATORS BACKPORT (PRE-C++14)          ///
///////////////////////////////////////////////////////////////////////////////

#if !D_STL_FUNCTIONAL_HAS_TRANSPARENT_OPS

// less_transparent
//   functor: transparent less-than comparison.
// Backport of std::less<void> for pre-C++14 compilers.
struct less_transparent
{
    template<typename _Lhs,
             typename _Rhs>
    D_STL_FUNCTIONAL_CONSTEXPR
    auto operator()(const _Lhs& _lhs, const _Rhs& _rhs) const
        -> decltype(_lhs < _rhs)
    {
        return _lhs < _rhs;
    }

    using is_transparent = void;
};

// greater_transparent
//   functor: transparent greater-than comparison.
// Backport of std::greater<void> for pre-C++14 compilers.
struct greater_transparent
{
    template<typename _Lhs,
             typename _Rhs>
    D_STL_FUNCTIONAL_CONSTEXPR
    auto operator()(const _Lhs& _lhs, const _Rhs& _rhs) const
        -> decltype(_lhs > _rhs)
    {
        return _lhs > _rhs;
    }

    using is_transparent = void;
};

// less_equal_transparent
//   functor: transparent less-than-or-equal comparison.
// Backport of std::less_equal<void> for pre-C++14 compilers.
struct less_equal_transparent
{
    template<typename _Lhs,
             typename _Rhs>
    D_STL_FUNCTIONAL_CONSTEXPR
    auto operator()(const _Lhs& _lhs, const _Rhs& _rhs) const
        -> decltype(_lhs <= _rhs)
    {
        return _lhs <= _rhs;
    }

    using is_transparent = void;
};

// greater_equal_transparent
//   functor: transparent greater-than-or-equal comparison.
// Backport of std::greater_equal<void> for pre-C++14 compilers.
struct greater_equal_transparent
{
    template<typename _Lhs,
             typename _Rhs>
    D_STL_FUNCTIONAL_CONSTEXPR
    auto operator()(const _Lhs& _lhs, const _Rhs& _rhs) const
        -> decltype(_lhs >= _rhs)
    {
        return _lhs >= _rhs;
    }

    using is_transparent = void;
};

// equal_to_transparent
//   functor: transparent equality comparison.
// Backport of std::equal_to<void> for pre-C++14 compilers.
struct equal_to_transparent
{
    template<typename _Lhs,
             typename _Rhs>
    D_STL_FUNCTIONAL_CONSTEXPR
    auto operator()(const _Lhs& _lhs, const _Rhs& _rhs) const
        -> decltype(_lhs == _rhs)
    {
        return _lhs == _rhs;
    }

    using is_transparent = void;
};

// not_equal_to_transparent
//   functor: transparent inequality comparison.
// Backport of std::not_equal_to<void> for pre-C++14 compilers.
struct not_equal_to_transparent
{
    template<typename _Lhs,
             typename _Rhs>
    D_STL_FUNCTIONAL_CONSTEXPR
    auto operator()(const _Lhs& _lhs, const _Rhs& _rhs) const
        -> decltype(_lhs != _rhs)
    {
        return _lhs != _rhs;
    }

    using is_transparent = void;
};

// hash_transparent
//   functor: transparent hash computation.
struct hash_transparent
{
    template<typename _Type>
    D_STL_FUNCTIONAL_CONSTEXPR
    std::size_t operator()(const _Type& _val) const
    {
        return std::hash<typename std::decay<_Type>::type>{}(_val);
    }

    using is_transparent = void;
};

// type aliases for consistency with std naming
using less          = less_transparent;
using greater       = greater_transparent;
using less_equal    = less_equal_transparent;
using greater_equal = greater_equal_transparent;
using equal_to      = equal_to_transparent;
using not_equal_to  = not_equal_to_transparent;

#else

// use standard transparent comparators
using less          = std::less<>;
using greater       = std::greater<>;
using less_equal    = std::less_equal<>;
using greater_equal = std::greater_equal<>;
using equal_to      = std::equal_to<>;
using not_equal_to  = std::not_equal_to<>;

#endif  // !D_STL_FUNCTIONAL_HAS_TRANSPARENT_OPS


NS_END  // stl
NS_END  // djinterp


#endif  // DJINTERP_STL_FUNCTIONAL_
