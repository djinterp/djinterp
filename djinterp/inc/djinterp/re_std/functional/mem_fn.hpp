/***********************************************************************
* restd                                                       mem_fn.hpp
*
* function: wraps a pointer-to-member into a uniform callable object.
*   The returned callable accepts the object (or pointer/reference_-
* wrapper to one) plus any further call args and forwards to
* `restd::invoke`. Works for both pointer-to-member-function and
* pointer-to-member-data; the dispatching is handed off to invoke.
*
*   Min standard: C++11. Constexpr from C++20 in std (P1065); restd
* makes both the factory and the wrapper's call operator constexpr from
* C++11 onward (single-return bodies).
*
*
* path:      /inc/restd/functional/mem_fn.hpp
* link(s):   TBA
* author(s): restd                                       date: 2026.05.07
***********************************************************************/

#ifndef RESTD_FUNCTIONAL_MEM_FN_
#define RESTD_FUNCTIONAL_MEM_FN_ 1

#include "djinterp.hpp"

#if (D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES &&  \
     D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES)

#include "restd/utility/forward.hpp"
#include "restd/functional/invoke.hpp"

namespace restd
{

NS_INTERNAL

    // mem_fn_wrapper
    //   class: callable returned by mem_fn. Holds the member pointer
    // and delegates its operator() to restd::invoke.
    template<typename _MemberPtr>
    class mem_fn_wrapper
    {
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
        ) const -> decltype(restd::invoke(m_pm,
                                          restd::forward<_Args>(_args)...))
        {
            return restd::invoke(m_pm, restd::forward<_Args>(_args)...);
        }

    private:
        _MemberPtr m_pm;
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

} // namespace restd

#endif // variadic templates + rvalue references

#endif // RESTD_FUNCTIONAL_MEM_FN_
