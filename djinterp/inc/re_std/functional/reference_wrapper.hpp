/***********************************************************************
* re_std                                           reference_wrapper.hpp
*
* class: copyable, assignable wrapper around a reference.
*   Stores a pointer internally and exposes the wrapped reference via an
* implicit conversion and the `get()` accessor. Modelling a value type
* lets reference_wrapper be stored in containers and forwarded by value
* without losing reference semantics.
*
*   The `operator()` overload makes a reference_wrapper to a callable
* itself callable; it forwards to `re_std::invoke`. Because of that, this
* header has a one-way include cycle with `invoke.hpp`: invoke.hpp uses
* `is_reference_wrapper` to detect the rw-arg dispatch case, and
* reference_wrapper.hpp includes invoke.hpp at the bottom of the file
* so the operator()'s dependent-name lookup resolves.
*
*   Min standard: C++11 (the deleted rvalue ctor needs rvalue refs).
*
*
* path:      /inc/re_std/functional/reference_wrapper.hpp
* link(s):   TBA
* author(s): re_std                                      date: 2026.05.07
***********************************************************************/

#ifndef DJINTERP_RE_STD_FUNCTIONAL_REFERENCE_WRAPPER_
#define DJINTERP_RE_STD_FUNCTIONAL_REFERENCE_WRAPPER_ 1

#include "djinterp.hpp"
#include "re_std/type_traits/type_traits.hpp"

#if D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES

#include "re_std/utility/forward.hpp"
#include "re_std/functional/is_reference_wrapper.hpp"
                                    // is_reference_wrapper (+ the fwd decl)
#include "re_std/functional/invoke.hpp"   // re_std::invoke -- operator() needs it
                                    // DECLARED, not merely defined later

namespace re_std
{

// reference_wrapper
//   class: value-typed wrapper around a reference. Constructible from
// an lvalue; deleted from an rvalue. Implicitly converts back to the
// underlying reference and is itself callable when the wrapped object
// is callable.
template<typename _Type>
class reference_wrapper
{
public:
    typedef _Type type;

    // ctor from lvalue
    D_CONSTEXPR reference_wrapper(_Type& _v) noexcept
        : m_ptr(&_v)
    {}

    // explicitly delete the rvalue ctor: storing a pointer to a
    // soon-to-die temporary is never useful.
    reference_wrapper(_Type&&) = delete;

    // copy ctor / assign — defaulted via implicit rules; the
    // C++98 fallback is also a trivial pointer copy.
    D_CONSTEXPR reference_wrapper(const reference_wrapper& _o) noexcept
        : m_ptr(_o.m_ptr)
    {}

    reference_wrapper&
    operator=(
        const reference_wrapper& _o
    ) noexcept
    {
        m_ptr = _o.m_ptr;
        return *this;
    }

    // accessors
    D_CONSTEXPR operator _Type&() const noexcept
    {
        return *m_ptr;
    }

    D_CONSTEXPR _Type&
    get() const noexcept
    {
        return *m_ptr;
    }

    // call forwarder (delegates to re_std::invoke).
#if D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES
    template<typename... _Args>
    D_CONSTEXPR auto
    operator()(
        _Args&&... _args
    ) const -> decltype(re_std::invoke(get(), re_std::forward<_Args>(_args)...))
    {
        return re_std::invoke(get(), re_std::forward<_Args>(_args)...);
    }
#endif

private:
    _Type* m_ptr;
};

// is_reference_wrapper now lives in is_reference_wrapper.hpp (included
// above) so that invoke.hpp can use it without this class definition.

// C++17 deduction guide
#if D_ENV_LANG_IS_CPP17_OR_HIGHER
template<typename _Type>
reference_wrapper(_Type&) -> reference_wrapper<_Type>;
#endif

} // namespace re_std


#endif // D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES

#endif  // DJINTERP_RE_STD_FUNCTIONAL_REFERENCE_WRAPPER_
