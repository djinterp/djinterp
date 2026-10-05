/*******************************************************************************
* djinterp [re_std]                                        reference_wrapper.hpp
*
* reference_wrapper class header:
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
* author(s): re_std                                          created: 2026.05.07
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_FUNCTIONAL_REFERENCE_WRAPPER_HPP
#define RE_STD_FUNCTIONAL_REFERENCE_WRAPPER_HPP 1

// FLOOR, FOR NOW: below C++11 this header is empty rather than an error
// (README rule 5; re_std omits rather than degrades). The owner's ruling:
// compile at every level first; port to C++98 only where something needs it.
#include "../config.hpp"  // RE_STD_* configuration
#if RE_STD_LANG_IS_CPP11_OR_HIGHER

// re_std
#include "re_std/type_traits/type_traits.hpp"

#if RE_STD_LANG_HAS_RVALUE_REFERENCES

#include "re_std/utility/forward.hpp"
#include "re_std/functional/is_reference_wrapper.hpp"
                                    // is_reference_wrapper (+ the fwd decl)
#include "re_std/functional/invoke.hpp"            // re_std::invoke -- operator() needs it
                                    // DECLARED, not merely defined later

namespace re_std
{

// reference_wrapper
//   class: value-typed wrapper around a reference. Constructible from
// an lvalue; deleted from an rvalue. Implicitly converts back to the
// underlying reference and is itself callable when the wrapped object
// is callable.
template<typename Type>
class reference_wrapper
{
public:
    typedef Type type;

    // ctor from lvalue
    RE_STD_CONSTEXPR reference_wrapper(Type& _v) noexcept
        : m_ptr(&_v)
    {}

    // explicitly delete the rvalue ctor: storing a pointer to a
    // soon-to-die temporary is never useful.
    reference_wrapper(Type&&) = delete;

    // copy ctor / assign — defaulted via implicit rules; the
    // C++98 fallback is also a trivial pointer copy.
    RE_STD_CONSTEXPR reference_wrapper(const reference_wrapper& _o) noexcept
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
    RE_STD_CONSTEXPR operator Type&() const noexcept
    {
        return *m_ptr;
    }

    RE_STD_CONSTEXPR Type&
    get() const noexcept
    {
        return *m_ptr;
    }

    // call forwarder (delegates to re_std::invoke).
#if RE_STD_LANG_HAS_VARIADIC_TEMPLATES
    template<typename... Args>
    RE_STD_CONSTEXPR auto
    operator()(
        Args&&... _args
    ) const -> decltype(re_std::invoke(get(), re_std::forward<Args>(_args)...))
    {
        return re_std::invoke(get(), re_std::forward<Args>(_args)...);
    }
#endif

private:
    Type* m_ptr;
};

// is_reference_wrapper now lives in is_reference_wrapper.hpp (included
// above) so that invoke.hpp can use it without this class definition.

// C++17 deduction guide
#if RE_STD_LANG_IS_CPP17_OR_HIGHER
template<typename Type>
reference_wrapper(Type&) -> reference_wrapper<Type>;
#endif

}  // re_std
#endif // RE_STD_LANG_HAS_RVALUE_REFERENCES

#endif  // floor, for now


#endif  // RE_STD_FUNCTIONAL_REFERENCE_WRAPPER_HPP
