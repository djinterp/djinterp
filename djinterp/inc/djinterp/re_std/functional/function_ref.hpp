/******************************************************************************
* re_std [functional]                                           function_ref.hpp
*
*   function_ref - a NON-OWNING reference to a callable.
*
*   THE POINT IS THAT IT DOES NOT OWN.
*   move_only_function and copyable_function store their target; function_ref
* stores a pointer to one that lives elsewhere.  That makes it two words,
* trivially copyable, and free to construct - the right type for a PARAMETER
* that accepts any callable without templating the function on it and without
* allocating.  It is the wrong type for a member, a return value, or anything
* outliving the call: the referenced callable's lifetime is the caller's
* problem, exactly as with a raw reference.
*
*   THE DANGLING TRAP IS REAL AND WORTH STATING.
*   `function_ref<int(int)> f = [](int x){ return x; };` binds to a temporary
* lambda that dies at the end of the full-expression, leaving f dangling.  That
* is inherent to a non-owning reference type and is why std restricts the
* constructor rather than making it convenient.  Bind to a named callable.
*
*   NO REF-QUALIFIER FORMS.  P0792 specifies only `R(Args...) cv noexcept(b)`,
* giving four specialisations rather than the twelve the owning wrappers need.
* A reference has no value category of its own to propagate.
*
*   TRIVIALLY COPYABLE BY CONSTRUCTION - two raw pointers, no user-provided
* special members - so it passes in registers and copies for free.
*
*   STD IS C++26; re_std IS C++11 (noexcept forms C++17) - a fifteen-year
* back-port.  Nothing here needs more than variadic templates.
*
*
* path:      /inc/djinterp/re_std/functional/function_ref.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.08.13
******************************************************************************/

#ifndef RESTD_FUNCTIONAL_FUNCTION_REF_
#define RESTD_FUNCTIONAL_FUNCTION_REF_ 1

// re_std
#include "../../djinterp.hpp"

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

#include "../type_traits/type_traits.hpp"
#include "../utility/utility.hpp"
#include "../memory/addressof.hpp"
#include "./invoke.hpp"

NS_DJINTERP
NS_RESTD

// function_ref
//   class: primary template, deliberately undefined.
template<typename _Signature>
class function_ref;

// function_ref<_Result(_Args...)>
//   class: non-owning reference to a callable invoked as a non-const lvalue.
template<typename _Result, typename... _Args>
class function_ref<_Result(_Args...)>
{
    typedef _Result (*_Thunk)(void*, _Args&&...);

    void*  m_target;
    _Thunk m_thunk;

    template<typename _Func>
    static _Result call(void* target, _Args&&... args)
    {
        return static_cast<_Result>(re_std::invoke(
            *static_cast<_Func*>(target), static_cast<_Args&&>(args)...));
    }

public:
    template<typename _Func,
             typename enable_if<
                 !is_same<typename decay<_Func>::type, function_ref>::value,
                 int>::type = 0>
    function_ref(_Func& func) D_NOEXCEPT
        : m_target(static_cast<void*>(re_std::addressof(func))),
          m_thunk(&call<_Func>)
    {}

    _Result operator()(_Args... args) const
    {
        return m_thunk(m_target, static_cast<_Args&&>(args)...);
    }
};

// function_ref<_Result(_Args...) const>
//   class: invokes the referenced callable as const.
template<typename _Result, typename... _Args>
class function_ref<_Result(_Args...) const>
{
    typedef _Result (*_Thunk)(const void*, _Args&&...);

    const void* m_target;
    _Thunk      m_thunk;

    template<typename _Func>
    static _Result call(const void* target, _Args&&... args)
    {
        return static_cast<_Result>(re_std::invoke(
            *static_cast<const _Func*>(target),
            static_cast<_Args&&>(args)...));
    }

public:
    template<typename _Func,
             typename enable_if<
                 !is_same<typename decay<_Func>::type, function_ref>::value,
                 int>::type = 0>
    function_ref(const _Func& func) D_NOEXCEPT
        : m_target(static_cast<const void*>(re_std::addressof(func))),
          m_thunk(&call<_Func>)
    {}

    _Result operator()(_Args... args) const
    {
        return m_thunk(m_target, static_cast<_Args&&>(args)...);
    }
};

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

//   `R(Args...) noexcept` is a distinct TYPE only from C++17.

template<typename _Result, typename... _Args>
class function_ref<_Result(_Args...) noexcept>
{
    typedef _Result (*_Thunk)(void*, _Args&&...);
    void*  m_target;
    _Thunk m_thunk;

    template<typename _Func>
    static _Result call(void* target, _Args&&... args) D_NOEXCEPT
    {
        return static_cast<_Result>(re_std::invoke(
            *static_cast<_Func*>(target), static_cast<_Args&&>(args)...));
    }

public:
    template<typename _Func,
             typename enable_if<
                 !is_same<typename decay<_Func>::type, function_ref>::value,
                 int>::type = 0>
    function_ref(_Func& func) D_NOEXCEPT
        : m_target(static_cast<void*>(re_std::addressof(func))),
          m_thunk(&call<_Func>)
    {}

    _Result operator()(_Args... args) const D_NOEXCEPT
    { return m_thunk(m_target, static_cast<_Args&&>(args)...); }
};

template<typename _Result, typename... _Args>
class function_ref<_Result(_Args...) const noexcept>
{
    typedef _Result (*_Thunk)(const void*, _Args&&...);
    const void* m_target;
    _Thunk      m_thunk;

    template<typename _Func>
    static _Result call(const void* target, _Args&&... args) D_NOEXCEPT
    {
        return static_cast<_Result>(re_std::invoke(
            *static_cast<const _Func*>(target),
            static_cast<_Args&&>(args)...));
    }

public:
    template<typename _Func,
             typename enable_if<
                 !is_same<typename decay<_Func>::type, function_ref>::value,
                 int>::type = 0>
    function_ref(const _Func& func) D_NOEXCEPT
        : m_target(static_cast<const void*>(re_std::addressof(func))),
          m_thunk(&call<_Func>)
    {}

    _Result operator()(_Args... args) const D_NOEXCEPT
    { return m_thunk(m_target, static_cast<_Args&&>(args)...); }
};

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER

NS_END  // re_std
NS_END  // djinterp

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // RESTD_FUNCTIONAL_FUNCTION_REF_
