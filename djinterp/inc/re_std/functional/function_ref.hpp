/*******************************************************************************
* djinterp [re_std]                                             function_ref.hpp
*
* function_ref class header:
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
* path:      /inc/re_std/functional/function_ref.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.08.13
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_FUNCTIONAL_FUNCTION_REF_HPP
#define RE_STD_FUNCTIONAL_FUNCTION_REF_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration

#if RE_STD_LANG_IS_CPP11_OR_HIGHER

#include "../type_traits/type_traits.hpp"
#include "../utility/utility.hpp"
#include "../memory/addressof.hpp"
#include "./invoke.hpp"

namespace re_std
{

// function_ref
//   class: primary template, deliberately undefined.
template<typename Signature>
class function_ref;

// function_ref<Result(Args...)>
//   class: non-owning reference to a callable invoked as a non-const lvalue.
template<typename Result, typename... Args>
class function_ref<Result(Args...)>
{
    typedef Result (*_Thunk)(void*, Args&&...);

    void*  m_target;
    _Thunk m_thunk;

    template<typename Func>
    static Result call(void* target, Args&&... args)
    {
        return static_cast<Result>(re_std::invoke(
            *static_cast<Func*>(target), static_cast<Args&&>(args)...));
    }

public:
    template<typename Func,
             typename enable_if<
                 !is_same<typename decay<Func>::type, function_ref>::value,
                 int>::type = 0>
    function_ref(Func& func) RE_STD_NOEXCEPT
        : m_target(static_cast<void*>(re_std::addressof(func))),
          m_thunk(&call<Func>)
    {}

    Result operator()(Args... args) const
    {
        return m_thunk(m_target, static_cast<Args&&>(args)...);
    }
};

// function_ref<Result(Args...) const>
//   class: invokes the referenced callable as const.
template<typename Result, typename... Args>
class function_ref<Result(Args...) const>
{
    typedef Result (*_Thunk)(const void*, Args&&...);

    const void* m_target;
    _Thunk      m_thunk;

    template<typename Func>
    static Result call(const void* target, Args&&... args)
    {
        return static_cast<Result>(re_std::invoke(
            *static_cast<const Func*>(target),
            static_cast<Args&&>(args)...));
    }

public:
    template<typename Func,
             typename enable_if<
                 !is_same<typename decay<Func>::type, function_ref>::value,
                 int>::type = 0>
    function_ref(const Func& func) RE_STD_NOEXCEPT
        : m_target(static_cast<const void*>(re_std::addressof(func))),
          m_thunk(&call<Func>)
    {}

    Result operator()(Args... args) const
    {
        return m_thunk(m_target, static_cast<Args&&>(args)...);
    }
};

#if RE_STD_LANG_IS_CPP17_OR_HIGHER

//   `R(Args...) noexcept` is a distinct TYPE only from C++17.

template<typename Result, typename... Args>
class function_ref<Result(Args...) noexcept>
{
    typedef Result (*_Thunk)(void*, Args&&...);
    void*  m_target;
    _Thunk m_thunk;

    template<typename Func>
    static Result call(void* target, Args&&... args) RE_STD_NOEXCEPT
    {
        return static_cast<Result>(re_std::invoke(
            *static_cast<Func*>(target), static_cast<Args&&>(args)...));
    }

public:
    template<typename Func,
             typename enable_if<
                 !is_same<typename decay<Func>::type, function_ref>::value,
                 int>::type = 0>
    function_ref(Func& func) RE_STD_NOEXCEPT
        : m_target(static_cast<void*>(re_std::addressof(func))),
          m_thunk(&call<Func>)
    {}

    Result operator()(Args... args) const RE_STD_NOEXCEPT
    { return m_thunk(m_target, static_cast<Args&&>(args)...); }
};

template<typename Result, typename... Args>
class function_ref<Result(Args...) const noexcept>
{
    typedef Result (*_Thunk)(const void*, Args&&...);
    const void* m_target;
    _Thunk      m_thunk;

    template<typename Func>
    static Result call(const void* target, Args&&... args) RE_STD_NOEXCEPT
    {
        return static_cast<Result>(re_std::invoke(
            *static_cast<const Func*>(target),
            static_cast<Args&&>(args)...));
    }

public:
    template<typename Func,
             typename enable_if<
                 !is_same<typename decay<Func>::type, function_ref>::value,
                 int>::type = 0>
    function_ref(const Func& func) RE_STD_NOEXCEPT
        : m_target(static_cast<const void*>(re_std::addressof(func))),
          m_thunk(&call<Func>)
    {}

    Result operator()(Args... args) const RE_STD_NOEXCEPT
    { return m_thunk(m_target, static_cast<Args&&>(args)...); }
};

#endif  // RE_STD_LANG_IS_CPP17_OR_HIGHER

}  // re_std
#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER

#endif  // RE_STD_FUNCTIONAL_FUNCTION_REF_HPP
