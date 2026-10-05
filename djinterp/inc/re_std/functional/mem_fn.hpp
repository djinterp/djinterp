/*******************************************************************************
* djinterp [re_std]                                                   mem_fn.hpp
*
* mem_fn class header:
* function: wraps a pointer-to-member into a uniform callable object.
*   The returned callable accepts the object (or pointer/reference_-
* wrapper to one) plus any further call args and forwards to
* `re_std::invoke`. Works for both pointer-to-member-function and
* pointer-to-member-data; the dispatching is handed off to invoke.
*
*   Min standard: C++11. Constexpr from C++20 in std (P1065); re_std
* makes both the factory and the wrapper's call operator constexpr from
* C++11 onward (single-return bodies).
*
*
* path:      /inc/re_std/functional/mem_fn.hpp
* link(s):   TBA
* author(s): re_std                                          created: 2026.05.07
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_FUNCTIONAL_MEM_FN_HPP
#define RE_STD_FUNCTIONAL_MEM_FN_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration

#if (RE_STD_LANG_HAS_VARIADIC_TEMPLATES &&  \
     RE_STD_LANG_HAS_RVALUE_REFERENCES)

#include "re_std/utility/forward.hpp"
#include "re_std/functional/invoke.hpp"

namespace re_std
{

namespace internal
{

    // mem_fn_wrapper
    //   class: callable returned by mem_fn. Holds the member pointer
    // and delegates its operator() to re_std::invoke.
    template<typename MemberPtr>
    class mem_fn_wrapper
    {
    private:
        // declared BEFORE operator(): a trailing return type is not a
        // complete-class context, so a member declared after it is not
        // yet visible there.
        MemberPtr m_pm;

    public:
        RE_STD_CONSTEXPR mem_fn_wrapper(
            MemberPtr _pm
        ) noexcept
            : m_pm(_pm)
        {}

        template<typename... Args>
        RE_STD_CONSTEXPR auto
        operator()(
            Args&&... _args
        ) const -> decltype(re_std::invoke(m_pm,
                                          re_std::forward<Args>(_args)...))
        {
            return re_std::invoke(m_pm, re_std::forward<Args>(_args)...);
        }

    };

}  // internal

// mem_fn
//   function: factory producing a callable wrapper around a pointer-
// to-member.
template<typename M,
         typename T>
RE_STD_CONSTEXPR internal::mem_fn_wrapper<M T::*>
mem_fn(
    M T::* _pm
) noexcept
{
    return internal::mem_fn_wrapper<M T::*>(_pm);
}

}  // re_std
#endif // variadic templates + rvalue references

#endif  // RE_STD_FUNCTIONAL_MEM_FN_HPP
