/***********************************************************************
* restd                                            reference_wrapper.hpp
*
* class: copyable, assignable wrapper around a reference.
*   Stores a pointer internally and exposes the wrapped reference via an
* implicit conversion and the `get()` accessor. Modelling a value type
* lets reference_wrapper be stored in containers and forwarded by value
* without losing reference semantics.
*
*   The `operator()` overload makes a reference_wrapper to a callable
* itself callable; it forwards to `restd::invoke`. Because of that, this
* header has a one-way include cycle with `invoke.hpp`: invoke.hpp uses
* `is_reference_wrapper` to detect the rw-arg dispatch case, and
* reference_wrapper.hpp includes invoke.hpp at the bottom of the file
* so the operator()'s dependent-name lookup resolves.
*
*   Min standard: C++11 (the deleted rvalue ctor needs rvalue refs).
*
*
* path:      /inc/restd/functional/reference_wrapper.hpp
* link(s):   TBA
* author(s): restd                                       date: 2026.05.07
***********************************************************************/

#ifndef RESTD_FUNCTIONAL_REFERENCE_WRAPPER_
#define RESTD_FUNCTIONAL_REFERENCE_WRAPPER_ 1

#include "djinterp.hpp"
#include "restd/type_traits/type_traits.hpp"

#if D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES

#include "restd/utility/forward.hpp"

namespace restd
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

    // call forwarder (delegates to restd::invoke).
#if D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES
    template<typename... _Args>
    D_CONSTEXPR auto
    operator()(
        _Args&&... _args
    ) const -> decltype(restd::invoke(get(), restd::forward<_Args>(_args)...))
    {
        return restd::invoke(get(), restd::forward<_Args>(_args)...);
    }
#endif

private:
    _Type* m_ptr;
};

NS_INTERNAL

    // is_reference_wrapper_helper
    //   trait: primary -- false for arbitrary types.
    template<typename _Type>
    struct is_reference_wrapper_helper : false_type
    {};

    // is_reference_wrapper_helper<reference_wrapper<U>>
    //   trait: specialization -- true for reference_wrapper.
    template<typename _U>
    struct is_reference_wrapper_helper< reference_wrapper<_U> > : true_type
    {};

NS_END  // internal

// is_reference_wrapper
//   trait: detects reference_wrapper. Cv-stripped.
template<typename _Type>
struct is_reference_wrapper
    : internal::is_reference_wrapper_helper<typename remove_cv<_Type>::type>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

template<typename _Type>
D_CONSTEXPR bool is_reference_wrapper_v
    = is_reference_wrapper<_Type>::value;

#endif

// C++17 deduction guide
#if D_ENV_LANG_IS_CPP17_OR_HIGHER
template<typename _Type>
reference_wrapper(_Type&) -> reference_wrapper<_Type>;
#endif

} // namespace restd

// invoke.hpp uses is_reference_wrapper above; reference_wrapper's
// operator() uses restd::invoke. invoke.hpp also includes this header
// at its top so it has the full definition; the include guards make
// the cycle harmless -- the no-op skip inside whichever file is
// re-entered means each definition is reached exactly once.
#include "restd/functional/invoke.hpp"

#endif // D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES

#endif // RESTD_FUNCTIONAL_REFERENCE_WRAPPER_
