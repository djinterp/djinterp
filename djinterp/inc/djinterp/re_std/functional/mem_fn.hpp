/***********************************************************************
* re_std                                                      mem_fn.hpp
*
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
* author(s): re_std                                      date: 2026.05.07
***********************************************************************/

#ifndef DJINTERP_RE_STD_FUNCTIONAL_MEM_FN_
#define DJINTERP_RE_STD_FUNCTIONAL_MEM_FN_ 1

#include "djinterp.hpp"

#if (D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES &&  \
     D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES)

#include "re_std/utility/forward.hpp"
#include "re_std/functional/invoke.hpp"

namespace re_std
{

NS_INTERNAL

    // mem_fn_wrapper
    //   class: callable returned by mem_fn. Holds the member pointer
    // and delegates its operator() to re_std::invoke.
    template<typename _MemberPtr>
    class mem_fn_wrapper
    {
    private:
        // declared BEFORE operator(): a trailing return type is not a
        // complete-class context, so a member declared after it is not
        // yet visible there.
        _MemberPtr m_pm;

    public:
        D_CONSTEXPR mem_fn_wrapper(
            _MemberPtr _pm
        ) noexcept
            : m_pm(_pm)
        {}

        template<typename... _Args>
        D_CONSTEXPR auto
        operator()(
            _Args&&... _args
        ) const -> decltype(re_std::invoke(m_pm,
                                          re_std::forward<_Args>(_args)...))
        {
            return re_std::invoke(m_pm, re_std::forward<_Args>(_args)...);
        }

    };

NS_END  // internal

// mem_fn
//   function: factory producing a callable wrapper around a pointer-
// to-member.
template<typename _M,
         typename _T>
D_CONSTEXPR internal::mem_fn_wrapper<_M _T::*>
mem_fn(
    _M _T::* _pm
) noexcept
{
    return internal::mem_fn_wrapper<_M _T::*>(_pm);
}

} // namespace re_std

#endif // variadic templates + rvalue references

#endif  // DJINTERP_RE_STD_FUNCTIONAL_MEM_FN_
