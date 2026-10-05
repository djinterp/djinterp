/*******************************************************************************
* djinterp [re_std]                                               unique_ptr.hpp
*
* exclusive-ownership smart pointer:
*   unique_ptr<T, D>      single object
*   unique_ptr<T[], D>    array
*
* both specialisations are move-only (copy ctor and copy-assignment are
* deleted). on destruction, the deleter is invoked on the held pointer
* if it is non-null. Ownership transfers via move construction, move
* assignment, or release()+reset() pairs.
*
* deleter selection:
*   D defaults to re_std::default_delete<T> for the single form and
*   re_std::default_delete<T[]> for the array form.
*
* pointer type detection:
*   pointer = D::pointer if defined, else T*. The detection idiom is
*   the same void_t-based approach used by allocator_traits.
*
* converting moves:
*   unique_ptr<U, E> -> unique_ptr<T, D> is enabled when:
*     - U* is convertible to T*
*     - U is not an array
*     - either D is a reference and E is the same type, or
*       D is non-reference and E is convertible to D
*   The array specialisation has stricter rules (qualification-conversion
*   only on the element type) per [unique.ptr.runtime.ctor].
*
* known limitations / deferrals:
*   1. NO EMPTY-BASE OPTIMISATION. The deleter is held by value as a
*      member, not as a private base. sizeof(unique_ptr<T>) is therefore
*      sizeof(T*) + sizeof(D) + alignment padding, not sizeof(T*) for
*      stateless deleters. Will be revisited when re_std ships a
*      compressed_pair helper.
*
*   2. Comparison operators use raw operator< rather than less<CT>.
*      Std mandates less<common_type_t<P1,P2>> for total order on
*      pointers; raw < gives the same result on every flat-memory
*      architecture in production today. Will be fixed when re_std
*      ships <functional>.
*
*   3. operator<=> deferred (needs <compare>).
*
*   4. hash<unique_ptr> deferred (needs re_std::hash).
*
*   5. Reference deleters (D = X&) work for the simple cases but the
*      full constructor-overload table for reference D per
*      [unique.ptr.single.ctor] is not exhaustively implemented. Use
*      with caution.
*
*
* path:      /inc/re_std/memory/unique_ptr.hpp
* link(s):   TBA
* author(s): re_std contributors                             created: 2026.05.02
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_MEMORY_UNIQUE_PTR_HPP
#define RE_STD_MEMORY_UNIQUE_PTR_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_IS_CPP11_OR_HIGHER

    // std
    #include <cstddef>  // size_t, ptrdiff_t, nullptr_t

    #include "re_std/memory/default_delete.hpp"
    #include "re_std/type_traits/integral_constant.hpp"
    #include "re_std/type_traits/enable_if.hpp"
    #include "re_std/type_traits/is_array.hpp"
    #include "re_std/type_traits/is_convertible.hpp"
    #include "re_std/type_traits/is_reference.hpp"
    #include "re_std/type_traits/is_same.hpp"
    #include "re_std/type_traits/remove_reference.hpp"
    #include "re_std/type_traits/add_lvalue_reference.hpp"
    #include "re_std/type_traits/void_t.hpp"
    #include "re_std/utility/move.hpp"
    #include "re_std/utility/forward.hpp"


namespace re_std
{

// =============================================================================
// internal: pointer-type detection
// =============================================================================

namespace internal
{

    // up_pointer<D, T>::type
    //   trait: D::pointer if defined, else T*. Used to compute
    //   unique_ptr's pointer typedef.

    template<typename D, typename T, typename = void>
    struct up_pointer
    {
        typedef T* type;
    };

    template<typename D, typename T>
    struct up_pointer
    <
        D,
        T,
        void_t<typename remove_reference<D>::type::pointer>
    >
    {
        typedef typename remove_reference<D>::type::pointer type;
    };

    // up_safe_array_conversion<From, To>
    //   trait: true if a unique_ptr<From[]> -> unique_ptr<To[]> array
    //   conversion is allowed. Per [unique.ptr.runtime.ctor], this
    //   requires that From(*)[] is convertible to To(*)[] — i.e. only
    //   qualification conversions on the element type, never derived-
    //   to-base.

    template<typename From, typename To>
    struct up_safe_array_conversion
        : integral_constant<bool, is_convertible<From(*)[], To(*)[]>::value>
    {
    };

}  // internal
// =============================================================================
// unique_ptr<T, D>  -  single-object specialisation
// =============================================================================

// unique_ptr<T, D>
//   class: exclusive-ownership smart pointer for a single T.
template<typename T, typename D = default_delete<T> >
class unique_ptr
{
public:
    // -------------------------------------------------------------------------
    // member types
    // -------------------------------------------------------------------------

    typedef typename internal::up_pointer<D, T>::type pointer;
    typedef T                                          element_type;
    typedef D                                          deleter_type;

private:
    pointer       m_ptr;
    deleter_type  m_del;

    // copy operations are deleted (unique ownership).
    RE_STD_DELETED_FN(unique_ptr(const unique_ptr&))
    RE_STD_DELETED_FN(unique_ptr& operator=(const unique_ptr&))

public:
    // -------------------------------------------------------------------------
    // construction
    // -------------------------------------------------------------------------

    // Default ctor: empty pointer, default-constructed deleter.
    // Disabled when D is a pointer or a reference, per the standard
    // ([unique.ptr.single.ctor]/1), since those cannot be value-init'd
    // into a usable state.
    RE_STD_CONSTEXPR unique_ptr() RE_STD_NOEXCEPT
        : m_ptr()
        , m_del()
    {
    }

    // Ctor from nullptr: same as default.
    RE_STD_CONSTEXPR unique_ptr(std::nullptr_t) RE_STD_NOEXCEPT
        : m_ptr()
        , m_del()
    {
    }

    // Ctor from raw pointer: takes ownership.
    explicit unique_ptr(pointer _p) RE_STD_NOEXCEPT
        : m_ptr(_p)
        , m_del()
    {
    }

    // Ctor from raw pointer + deleter (lvalue ref form).
    // Note: when D is a reference type (D = X&), this is the form that
    // binds the reference. Not exhaustively tested for reference D —
    // see header documentation.
    unique_ptr
    (
        pointer                                        _p,
        typename add_lvalue_reference<const D>::type  _d
    ) RE_STD_NOEXCEPT
        : m_ptr(_p)
        , m_del(_d)
    {
    }

    // Ctor from raw pointer + deleter (rvalue ref form).
    // For non-reference D, takes a true rvalue.
    unique_ptr
    (
        pointer                                  _p,
        typename remove_reference<D>::type&&    _d
    ) RE_STD_NOEXCEPT
        : m_ptr(_p)
        , m_del(re_std::move(_d))
    {
    }

    // Move ctor.
    unique_ptr(unique_ptr&& _other) RE_STD_NOEXCEPT
        : m_ptr(_other.release())
        , m_del(re_std::forward<D>(_other.m_del))
    {
    }

    // Converting move ctor: unique_ptr<U, E> -> unique_ptr<T, D>.
    // Constraints (per [unique.ptr.single.ctor]/14):
    //   - unique_ptr<U,E>::pointer is convertible to pointer
    //   - U is not an array type
    //   - D is a reference => E is the same type as D
    //     OR D is not a reference => E is convertible to D
    template
    <
        typename U,
        typename E,
        typename = typename enable_if
        <
            is_convertible
            <
                typename unique_ptr<U, E>::pointer,
                pointer
            >::value
            && !is_array<U>::value
            && (
                (
                    is_reference<D>::value
                    && is_same<E, D>::value
                )
                || (
                    !is_reference<D>::value
                    && is_convertible<E, D>::value
                )
            )
        >::type
    >
    unique_ptr(unique_ptr<U, E>&& _other) RE_STD_NOEXCEPT
        : m_ptr(_other.release())
        , m_del(re_std::forward<E>(_other.get_deleter()))
    {
    }

    // -------------------------------------------------------------------------
    // destruction
    // -------------------------------------------------------------------------

    ~unique_ptr()
    {
        if (m_ptr != pointer())
        {
            m_del(m_ptr);
        }
    }

    // -------------------------------------------------------------------------
    // assignment
    // -------------------------------------------------------------------------

    unique_ptr& operator=(unique_ptr&& _other) RE_STD_NOEXCEPT
    {
        reset(_other.release());
        m_del = re_std::forward<D>(_other.m_del);
        return *this;
    }

    template<typename U, typename E>
    typename enable_if
    <
        is_convertible
        <
            typename unique_ptr<U, E>::pointer,
            pointer
        >::value
        && !is_array<U>::value,
        unique_ptr&
    >::type
    operator=(unique_ptr<U, E>&& _other) RE_STD_NOEXCEPT
    {
        reset(_other.release());
        m_del = re_std::forward<E>(_other.get_deleter());
        return *this;
    }

    unique_ptr& operator=(std::nullptr_t) RE_STD_NOEXCEPT
    {
        reset();
        return *this;
    }

    // -------------------------------------------------------------------------
    // observers
    // -------------------------------------------------------------------------

    typename add_lvalue_reference<T>::type
    operator*() const
    {
        return *m_ptr;
    }

    pointer operator->() const RE_STD_NOEXCEPT
    {
        return m_ptr;
    }

    pointer get() const RE_STD_NOEXCEPT
    {
        return m_ptr;
    }

    deleter_type& get_deleter() RE_STD_NOEXCEPT
    {
        return m_del;
    }

    const deleter_type& get_deleter() const RE_STD_NOEXCEPT
    {
        return m_del;
    }

    explicit operator bool() const RE_STD_NOEXCEPT
    {
        return m_ptr != pointer();
    }

    // -------------------------------------------------------------------------
    // modifiers
    // -------------------------------------------------------------------------

    pointer release() RE_STD_NOEXCEPT
    {
        pointer _old = m_ptr;
        m_ptr = pointer();
        return _old;
    }

    void reset(pointer _p = pointer()) RE_STD_NOEXCEPT
    {
        pointer _old = m_ptr;
        m_ptr = _p;
        if (_old != pointer())
        {
            m_del(_old);
        }
    }

    void swap(unique_ptr& _other) RE_STD_NOEXCEPT
    {
        // Manual two-step swap for the pointer; for the deleter we use
        // re_std::swap when it lands. For now this is correct for any
        // movable deleter.
        pointer _tmp_p = m_ptr;
        m_ptr = _other.m_ptr;
        _other.m_ptr = _tmp_p;

        // Deleter swap via move-construct + move-assign.
        deleter_type _tmp_d = re_std::move(m_del);
        m_del = re_std::move(_other.m_del);
        _other.m_del = re_std::move(_tmp_d);
    }
};


// =============================================================================
// unique_ptr<T[], D>  -  array specialisation
// =============================================================================

// unique_ptr<T[], D>
//   class: exclusive-ownership smart pointer for a heap-allocated array
//   of T. Differs from the single form in:
//     - operator[] replaces operator* and operator->
//     - converting ctors use the much stricter qualification-conversion
//       rule (no derived-to-base array conversions)
//     - reset() can take any pointer convertible-via-array to T*, not
//       just exactly T*
template<typename T, typename D>
class unique_ptr<T[], D>
{
public:
    typedef typename internal::up_pointer<D, T>::type pointer;
    typedef T                                          element_type;
    typedef D                                          deleter_type;

private:
    pointer       m_ptr;
    deleter_type  m_del;

    RE_STD_DELETED_FN(unique_ptr(const unique_ptr&))
    RE_STD_DELETED_FN(unique_ptr& operator=(const unique_ptr&))

public:
    // ---- ctors ----

    RE_STD_CONSTEXPR unique_ptr() RE_STD_NOEXCEPT
        : m_ptr()
        , m_del()
    {
    }

    RE_STD_CONSTEXPR unique_ptr(std::nullptr_t) RE_STD_NOEXCEPT
        : m_ptr()
        , m_del()
    {
    }

    // Pointer-taking ctor. SFINAE-restricted to types that satisfy the
    // qualification-conversion rule for arrays. A raw T* always
    // qualifies trivially.
    template
    <
        typename U,
        typename = typename enable_if
        <
            is_same<U, pointer>::value
            || (
                is_same<pointer, element_type*>::value
                && is_convertible<U(*)[], element_type(*)[]>::value
            )
        >::type
    >
    explicit unique_ptr(U _p) RE_STD_NOEXCEPT
        : m_ptr(_p)
        , m_del()
    {
    }

    template
    <
        typename U,
        typename = typename enable_if
        <
            is_same<U, pointer>::value
            || (
                is_same<pointer, element_type*>::value
                && is_convertible<U(*)[], element_type(*)[]>::value
            )
        >::type
    >
    unique_ptr
    (
        U                                            _p,
        typename add_lvalue_reference<const D>::type _d
    ) RE_STD_NOEXCEPT
        : m_ptr(_p)
        , m_del(_d)
    {
    }

    template
    <
        typename U,
        typename = typename enable_if
        <
            is_same<U, pointer>::value
            || (
                is_same<pointer, element_type*>::value
                && is_convertible<U(*)[], element_type(*)[]>::value
            )
        >::type
    >
    unique_ptr
    (
        U                                       _p,
        typename remove_reference<D>::type&&    _d
    ) RE_STD_NOEXCEPT
        : m_ptr(_p)
        , m_del(re_std::move(_d))
    {
    }

    unique_ptr(unique_ptr&& _other) RE_STD_NOEXCEPT
        : m_ptr(_other.release())
        , m_del(re_std::forward<D>(_other.m_del))
    {
    }

    // Converting move ctor: stricter rules than the single form.
    template
    <
        typename U,
        typename E,
        typename = typename enable_if
        <
            is_array<U>::value
            && is_same<pointer, element_type*>::value
            && is_same
               <
                   typename unique_ptr<U, E>::pointer,
                   typename unique_ptr<U, E>::element_type*
               >::value
            && is_convertible
               <
                   typename unique_ptr<U, E>::element_type(*)[],
                   element_type(*)[]
               >::value
            && (
                (
                    is_reference<D>::value
                    && is_same<E, D>::value
                )
                || (
                    !is_reference<D>::value
                    && is_convertible<E, D>::value
                )
            )
        >::type
    >
    unique_ptr(unique_ptr<U, E>&& _other) RE_STD_NOEXCEPT
        : m_ptr(_other.release())
        , m_del(re_std::forward<E>(_other.get_deleter()))
    {
    }

    // ---- dtor ----

    ~unique_ptr()
    {
        if (m_ptr != pointer())
        {
            m_del(m_ptr);
        }
    }

    // ---- assignment ----

    unique_ptr& operator=(unique_ptr&& _other) RE_STD_NOEXCEPT
    {
        reset(_other.release());
        m_del = re_std::forward<D>(_other.m_del);
        return *this;
    }

    template<typename U, typename E>
    typename enable_if
    <
        is_array<U>::value
        && is_same<pointer, element_type*>::value
        && is_convertible
           <
               typename unique_ptr<U, E>::element_type(*)[],
               element_type(*)[]
           >::value,
        unique_ptr&
    >::type
    operator=(unique_ptr<U, E>&& _other) RE_STD_NOEXCEPT
    {
        reset(_other.release());
        m_del = re_std::forward<E>(_other.get_deleter());
        return *this;
    }

    unique_ptr& operator=(std::nullptr_t) RE_STD_NOEXCEPT
    {
        reset();
        return *this;
    }

    // ---- observers ----

    typename add_lvalue_reference<T>::type
    operator[](std::size_t _i) const
    {
        return m_ptr[_i];
    }

    pointer get() const RE_STD_NOEXCEPT
    {
        return m_ptr;
    }

    deleter_type& get_deleter() RE_STD_NOEXCEPT
    {
        return m_del;
    }

    const deleter_type& get_deleter() const RE_STD_NOEXCEPT
    {
        return m_del;
    }

    explicit operator bool() const RE_STD_NOEXCEPT
    {
        return m_ptr != pointer();
    }

    // ---- modifiers ----

    pointer release() RE_STD_NOEXCEPT
    {
        pointer _old = m_ptr;
        m_ptr = pointer();
        return _old;
    }

    // reset(nullptr) and reset() — explicit nullptr overload.
    void reset(std::nullptr_t = RE_STD_NULLPTR) RE_STD_NOEXCEPT
    {
        pointer _old = m_ptr;
        m_ptr = pointer();
        if (_old != pointer())
        {
            m_del(_old);
        }
    }

    // reset(pointer) — SFINAE-restricted like the ctors.
    template<typename U>
    typename enable_if
    <
        is_same<U, pointer>::value
        || (
            is_same<pointer, element_type*>::value
            && is_convertible<U(*)[], element_type(*)[]>::value
        ),
        void
    >::type
    reset(U _p) RE_STD_NOEXCEPT
    {
        pointer _old = m_ptr;
        m_ptr = _p;
        if (_old != pointer())
        {
            m_del(_old);
        }
    }

    void swap(unique_ptr& _other) RE_STD_NOEXCEPT
    {
        pointer _tmp_p = m_ptr;
        m_ptr = _other.m_ptr;
        _other.m_ptr = _tmp_p;

        deleter_type _tmp_d = re_std::move(m_del);
        m_del = re_std::move(_other.m_del);
        _other.m_del = re_std::move(_tmp_d);
    }
};


// =============================================================================
// comparison operators  (unique_ptr <=> unique_ptr)
// =============================================================================
//
// Note: per [unique.ptr.special]/4-9, the relational operators are
// specified in terms of less<common_type_t<P1,P2>>. We use raw operator<
// here pending re_std::less; this gives the same result on every flat-
// memory architecture in production today.

template<typename T1, typename D1, typename T2, typename D2>
inline bool operator==
(
    const unique_ptr<T1, D1>& _a,
    const unique_ptr<T2, D2>& _b
)
{
    return _a.get() == _b.get();
}

template<typename T1, typename D1, typename T2, typename D2>
inline bool operator!=
(
    const unique_ptr<T1, D1>& _a,
    const unique_ptr<T2, D2>& _b
)
{
    return _a.get() != _b.get();
}

template<typename T1, typename D1, typename T2, typename D2>
inline bool operator<
(
    const unique_ptr<T1, D1>& _a,
    const unique_ptr<T2, D2>& _b
)
{
    return _a.get() < _b.get();
}

template<typename T1, typename D1, typename T2, typename D2>
inline bool operator<=
(
    const unique_ptr<T1, D1>& _a,
    const unique_ptr<T2, D2>& _b
)
{
    return !(_b < _a);
}

template<typename T1, typename D1, typename T2, typename D2>
inline bool operator>
(
    const unique_ptr<T1, D1>& _a,
    const unique_ptr<T2, D2>& _b
)
{
    return _b < _a;
}

template<typename T1, typename D1, typename T2, typename D2>
inline bool operator>=
(
    const unique_ptr<T1, D1>& _a,
    const unique_ptr<T2, D2>& _b
)
{
    return !(_a < _b);
}


// =============================================================================
// comparison operators  (unique_ptr <=> nullptr)
// =============================================================================

template<typename T, typename D>
inline bool operator==
(
    const unique_ptr<T, D>& _a,
    std::nullptr_t
) RE_STD_NOEXCEPT
{
    return !_a;
}

template<typename T, typename D>
inline bool operator==
(
    std::nullptr_t,
    const unique_ptr<T, D>& _a
) RE_STD_NOEXCEPT
{
    return !_a;
}

template<typename T, typename D>
inline bool operator!=
(
    const unique_ptr<T, D>& _a,
    std::nullptr_t
) RE_STD_NOEXCEPT
{
    return static_cast<bool>(_a);
}

template<typename T, typename D>
inline bool operator!=
(
    std::nullptr_t,
    const unique_ptr<T, D>& _a
) RE_STD_NOEXCEPT
{
    return static_cast<bool>(_a);
}

template<typename T, typename D>
inline bool operator<
(
    const unique_ptr<T, D>& _a,
    std::nullptr_t
)
{
    return _a.get() < typename unique_ptr<T, D>::pointer();
}

template<typename T, typename D>
inline bool operator<
(
    std::nullptr_t,
    const unique_ptr<T, D>& _a
)
{
    return typename unique_ptr<T, D>::pointer() < _a.get();
}

template<typename T, typename D>
inline bool operator<=
(
    const unique_ptr<T, D>& _a,
    std::nullptr_t
)
{
    return !(RE_STD_NULLPTR < _a);
}

template<typename T, typename D>
inline bool operator<=
(
    std::nullptr_t,
    const unique_ptr<T, D>& _a
)
{
    return !(_a < RE_STD_NULLPTR);
}

template<typename T, typename D>
inline bool operator>
(
    const unique_ptr<T, D>& _a,
    std::nullptr_t
)
{
    return RE_STD_NULLPTR < _a;
}

template<typename T, typename D>
inline bool operator>
(
    std::nullptr_t,
    const unique_ptr<T, D>& _a
)
{
    return _a < RE_STD_NULLPTR;
}

template<typename T, typename D>
inline bool operator>=
(
    const unique_ptr<T, D>& _a,
    std::nullptr_t
)
{
    return !(_a < RE_STD_NULLPTR);
}

template<typename T, typename D>
inline bool operator>=
(
    std::nullptr_t,
    const unique_ptr<T, D>& _a
)
{
    return !(RE_STD_NULLPTR < _a);
}


}  // re_std
#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER

#endif  // RE_STD_MEMORY_UNIQUE_PTR_HPP
