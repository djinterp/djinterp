/***********************************************************************
* restd                                                      unique_ptr.hpp
*
* exclusive-ownership smart pointer:
*   unique_ptr<_T, _D>      single object
*   unique_ptr<_T[], _D>    array
*
* both specialisations are move-only (copy ctor and copy-assignment are
* deleted). on destruction, the deleter is invoked on the held pointer
* if it is non-null. Ownership transfers via move construction, move
* assignment, or release()+reset() pairs.
*
* deleter selection:
*   _D defaults to restd::default_delete<_T> for the single form and
*   restd::default_delete<_T[]> for the array form.
*
* pointer type detection:
*   pointer = _D::pointer if defined, else _T*. The detection idiom is
*   the same void_t-based approach used by allocator_traits.
*
* converting moves:
*   unique_ptr<_U, _E> -> unique_ptr<_T, _D> is enabled when:
*     - _U* is convertible to _T*
*     - _U is not an array
*     - either _D is a reference and _E is the same type, or
*       _D is non-reference and _E is convertible to _D
*   The array specialisation has stricter rules (qualification-conversion
*   only on the element type) per [unique.ptr.runtime.ctor].
*
* known limitations / deferrals:
*   1. NO EMPTY-BASE OPTIMISATION. The deleter is held by value as a
*      member, not as a private base. sizeof(unique_ptr<T>) is therefore
*      sizeof(T*) + sizeof(D) + alignment padding, not sizeof(T*) for
*      stateless deleters. Will be revisited when restd ships a
*      compressed_pair helper.
*
*   2. Comparison operators use raw operator< rather than less<CT>.
*      Std mandates less<common_type_t<P1,P2>> for total order on
*      pointers; raw < gives the same result on every flat-memory
*      architecture in production today. Will be fixed when restd
*      ships <functional>.
*
*   3. operator<=> deferred (needs <compare>).
*
*   4. hash<unique_ptr> deferred (needs restd::hash).
*
*   5. Reference deleters (_D = X&) work for the simple cases but the
*      full constructor-overload table for reference _D per
*      [unique.ptr.single.ctor] is not exhaustively implemented. Use
*      with caution.
*
*
* path:      /inc/restd/memory/unique_ptr.hpp
* link(s):   TBA
* author(s): restd contributors                          date: 2026.05.02
***********************************************************************/

#ifndef RESTD_MEMORY_UNIQUE_PTR_
#define RESTD_MEMORY_UNIQUE_PTR_ 1

#include "djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

    #include <cstddef>  // size_t, ptrdiff_t, nullptr_t

    #include "restd/memory/default_delete.hpp"
    #include "restd/type_traits/integral_constant.hpp"
    #include "restd/type_traits/enable_if.hpp"
    #include "restd/type_traits/is_array.hpp"
    #include "restd/type_traits/is_convertible.hpp"
    #include "restd/type_traits/is_reference.hpp"
    #include "restd/type_traits/is_same.hpp"
    #include "restd/type_traits/remove_reference.hpp"
    #include "restd/type_traits/add_lvalue_reference.hpp"
    #include "restd/type_traits/void_t.hpp"
    #include "restd/utility/move.hpp"
    #include "restd/utility/forward.hpp"


namespace restd
{

// =============================================================================
// internal: pointer-type detection
// =============================================================================

namespace internal
{

    // up_pointer<_D, _T>::type
    //   trait: _D::pointer if defined, else _T*. Used to compute
    //   unique_ptr's pointer typedef.

    template<typename _D, typename _T, typename = void>
    struct up_pointer
    {
        typedef _T* type;
    };

    template<typename _D, typename _T>
    struct up_pointer
    <
        _D,
        _T,
        typename void_t<typename remove_reference<_D>::type::pointer>::type
    >
    {
        typedef typename remove_reference<_D>::type::pointer type;
    };

    // up_safe_array_conversion<_From, _To>
    //   trait: true if a unique_ptr<_From[]> -> unique_ptr<_To[]> array
    //   conversion is allowed. Per [unique.ptr.runtime.ctor], this
    //   requires that _From(*)[] is convertible to _To(*)[] — i.e. only
    //   qualification conversions on the element type, never derived-
    //   to-base.

    template<typename _From, typename _To>
    struct up_safe_array_conversion
        : integral_constant<bool, is_convertible<_From(*)[], _To(*)[]>::value>
    {
    };

}  // namespace internal


// =============================================================================
// unique_ptr<_T, _D>  -  single-object specialisation
// =============================================================================

// unique_ptr<_T, _D>
//   class: exclusive-ownership smart pointer for a single _T.
template<typename _T, typename _D = default_delete<_T> >
class unique_ptr
{
public:
    // -------------------------------------------------------------------------
    // member types
    // -------------------------------------------------------------------------

    typedef typename internal::up_pointer<_D, _T>::type pointer;
    typedef _T                                          element_type;
    typedef _D                                          deleter_type;

private:
    pointer       m_ptr;
    deleter_type  m_del;

    // copy operations are deleted (unique ownership).
    unique_ptr(const unique_ptr&)            D_DELETE_FN;
    unique_ptr& operator=(const unique_ptr&) D_DELETE_FN;

public:
    // -------------------------------------------------------------------------
    // construction
    // -------------------------------------------------------------------------

    // Default ctor: empty pointer, default-constructed deleter.
    // Disabled when _D is a pointer or a reference, per the standard
    // ([unique.ptr.single.ctor]/1), since those cannot be value-init'd
    // into a usable state.
    D_CONSTEXPR unique_ptr() D_NOEXCEPT
        : m_ptr()
        , m_del()
    {
    }

    // Ctor from nullptr: same as default.
    D_CONSTEXPR unique_ptr(std::nullptr_t) D_NOEXCEPT
        : m_ptr()
        , m_del()
    {
    }

    // Ctor from raw pointer: takes ownership.
    explicit unique_ptr(pointer _p) D_NOEXCEPT
        : m_ptr(_p)
        , m_del()
    {
    }

    // Ctor from raw pointer + deleter (lvalue ref form).
    // Note: when _D is a reference type (_D = X&), this is the form that
    // binds the reference. Not exhaustively tested for reference _D —
    // see header documentation.
    unique_ptr
    (
        pointer                                        _p,
        typename add_lvalue_reference<const _D>::type  _d
    ) D_NOEXCEPT
        : m_ptr(_p)
        , m_del(_d)
    {
    }

    // Ctor from raw pointer + deleter (rvalue ref form).
    // For non-reference _D, takes a true rvalue.
    unique_ptr
    (
        pointer                                  _p,
        typename remove_reference<_D>::type&&    _d
    ) D_NOEXCEPT
        : m_ptr(_p)
        , m_del(restd::move(_d))
    {
    }

    // Move ctor.
    unique_ptr(unique_ptr&& _other) D_NOEXCEPT
        : m_ptr(_other.release())
        , m_del(restd::forward<_D>(_other.m_del))
    {
    }

    // Converting move ctor: unique_ptr<_U, _E> -> unique_ptr<_T, _D>.
    // Constraints (per [unique.ptr.single.ctor]/14):
    //   - unique_ptr<_U,_E>::pointer is convertible to pointer
    //   - _U is not an array type
    //   - _D is a reference => _E is the same type as _D
    //     OR _D is not a reference => _E is convertible to _D
    template
    <
        typename _U,
        typename _E,
        typename = typename enable_if
        <
            is_convertible
            <
                typename unique_ptr<_U, _E>::pointer,
                pointer
            >::value
            && !is_array<_U>::value
            && (
                (
                    is_reference<_D>::value
                    && is_same<_E, _D>::value
                )
                || (
                    !is_reference<_D>::value
                    && is_convertible<_E, _D>::value
                )
            )
        >::type
    >
    unique_ptr(unique_ptr<_U, _E>&& _other) D_NOEXCEPT
        : m_ptr(_other.release())
        , m_del(restd::forward<_E>(_other.get_deleter()))
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

    unique_ptr& operator=(unique_ptr&& _other) D_NOEXCEPT
    {
        reset(_other.release());
        m_del = restd::forward<_D>(_other.m_del);
        return *this;
    }

    template<typename _U, typename _E>
    typename enable_if
    <
        is_convertible
        <
            typename unique_ptr<_U, _E>::pointer,
            pointer
        >::value
        && !is_array<_U>::value,
        unique_ptr&
    >::type
    operator=(unique_ptr<_U, _E>&& _other) D_NOEXCEPT
    {
        reset(_other.release());
        m_del = restd::forward<_E>(_other.get_deleter());
        return *this;
    }

    unique_ptr& operator=(std::nullptr_t) D_NOEXCEPT
    {
        reset();
        return *this;
    }

    // -------------------------------------------------------------------------
    // observers
    // -------------------------------------------------------------------------

    typename add_lvalue_reference<_T>::type
    operator*() const
    {
        return *m_ptr;
    }

    pointer operator->() const D_NOEXCEPT
    {
        return m_ptr;
    }

    pointer get() const D_NOEXCEPT
    {
        return m_ptr;
    }

    deleter_type& get_deleter() D_NOEXCEPT
    {
        return m_del;
    }

    const deleter_type& get_deleter() const D_NOEXCEPT
    {
        return m_del;
    }

    explicit operator bool() const D_NOEXCEPT
    {
        return m_ptr != pointer();
    }

    // -------------------------------------------------------------------------
    // modifiers
    // -------------------------------------------------------------------------

    pointer release() D_NOEXCEPT
    {
        pointer _old = m_ptr;
        m_ptr = pointer();
        return _old;
    }

    void reset(pointer _p = pointer()) D_NOEXCEPT
    {
        pointer _old = m_ptr;
        m_ptr = _p;
        if (_old != pointer())
        {
            m_del(_old);
        }
    }

    void swap(unique_ptr& _other) D_NOEXCEPT
    {
        // Manual two-step swap for the pointer; for the deleter we use
        // restd::swap when it lands. For now this is correct for any
        // movable deleter.
        pointer _tmp_p = m_ptr;
        m_ptr = _other.m_ptr;
        _other.m_ptr = _tmp_p;

        // Deleter swap via move-construct + move-assign.
        deleter_type _tmp_d = restd::move(m_del);
        m_del = restd::move(_other.m_del);
        _other.m_del = restd::move(_tmp_d);
    }
};


// =============================================================================
// unique_ptr<_T[], _D>  -  array specialisation
// =============================================================================

// unique_ptr<_T[], _D>
//   class: exclusive-ownership smart pointer for a heap-allocated array
//   of _T. Differs from the single form in:
//     - operator[] replaces operator* and operator->
//     - converting ctors use the much stricter qualification-conversion
//       rule (no derived-to-base array conversions)
//     - reset() can take any pointer convertible-via-array to _T*, not
//       just exactly _T*
template<typename _T, typename _D>
class unique_ptr<_T[], _D>
{
public:
    typedef typename internal::up_pointer<_D, _T>::type pointer;
    typedef _T                                          element_type;
    typedef _D                                          deleter_type;

private:
    pointer       m_ptr;
    deleter_type  m_del;

    unique_ptr(const unique_ptr&)            D_DELETE_FN;
    unique_ptr& operator=(const unique_ptr&) D_DELETE_FN;

public:
    // ---- ctors ----

    D_CONSTEXPR unique_ptr() D_NOEXCEPT
        : m_ptr()
        , m_del()
    {
    }

    D_CONSTEXPR unique_ptr(std::nullptr_t) D_NOEXCEPT
        : m_ptr()
        , m_del()
    {
    }

    // Pointer-taking ctor. SFINAE-restricted to types that satisfy the
    // qualification-conversion rule for arrays. A raw _T* always
    // qualifies trivially.
    template
    <
        typename _U,
        typename = typename enable_if
        <
            is_same<_U, pointer>::value
            || (
                is_same<pointer, element_type*>::value
                && is_convertible<_U(*)[], element_type(*)[]>::value
            )
        >::type
    >
    explicit unique_ptr(_U _p) D_NOEXCEPT
        : m_ptr(_p)
        , m_del()
    {
    }

    template
    <
        typename _U,
        typename = typename enable_if
        <
            is_same<_U, pointer>::value
            || (
                is_same<pointer, element_type*>::value
                && is_convertible<_U(*)[], element_type(*)[]>::value
            )
        >::type
    >
    unique_ptr
    (
        _U                                            _p,
        typename add_lvalue_reference<const _D>::type _d
    ) D_NOEXCEPT
        : m_ptr(_p)
        , m_del(_d)
    {
    }

    template
    <
        typename _U,
        typename = typename enable_if
        <
            is_same<_U, pointer>::value
            || (
                is_same<pointer, element_type*>::value
                && is_convertible<_U(*)[], element_type(*)[]>::value
            )
        >::type
    >
    unique_ptr
    (
        _U                                       _p,
        typename remove_reference<_D>::type&&    _d
    ) D_NOEXCEPT
        : m_ptr(_p)
        , m_del(restd::move(_d))
    {
    }

    unique_ptr(unique_ptr&& _other) D_NOEXCEPT
        : m_ptr(_other.release())
        , m_del(restd::forward<_D>(_other.m_del))
    {
    }

    // Converting move ctor: stricter rules than the single form.
    template
    <
        typename _U,
        typename _E,
        typename = typename enable_if
        <
            is_array<_U>::value
            && is_same<pointer, element_type*>::value
            && is_same
               <
                   typename unique_ptr<_U, _E>::pointer,
                   typename unique_ptr<_U, _E>::element_type*
               >::value
            && is_convertible
               <
                   typename unique_ptr<_U, _E>::element_type(*)[],
                   element_type(*)[]
               >::value
            && (
                (
                    is_reference<_D>::value
                    && is_same<_E, _D>::value
                )
                || (
                    !is_reference<_D>::value
                    && is_convertible<_E, _D>::value
                )
            )
        >::type
    >
    unique_ptr(unique_ptr<_U, _E>&& _other) D_NOEXCEPT
        : m_ptr(_other.release())
        , m_del(restd::forward<_E>(_other.get_deleter()))
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

    unique_ptr& operator=(unique_ptr&& _other) D_NOEXCEPT
    {
        reset(_other.release());
        m_del = restd::forward<_D>(_other.m_del);
        return *this;
    }

    template<typename _U, typename _E>
    typename enable_if
    <
        is_array<_U>::value
        && is_same<pointer, element_type*>::value
        && is_convertible
           <
               typename unique_ptr<_U, _E>::element_type(*)[],
               element_type(*)[]
           >::value,
        unique_ptr&
    >::type
    operator=(unique_ptr<_U, _E>&& _other) D_NOEXCEPT
    {
        reset(_other.release());
        m_del = restd::forward<_E>(_other.get_deleter());
        return *this;
    }

    unique_ptr& operator=(std::nullptr_t) D_NOEXCEPT
    {
        reset();
        return *this;
    }

    // ---- observers ----

    typename add_lvalue_reference<_T>::type
    operator[](std::size_t _i) const
    {
        return m_ptr[_i];
    }

    pointer get() const D_NOEXCEPT
    {
        return m_ptr;
    }

    deleter_type& get_deleter() D_NOEXCEPT
    {
        return m_del;
    }

    const deleter_type& get_deleter() const D_NOEXCEPT
    {
        return m_del;
    }

    explicit operator bool() const D_NOEXCEPT
    {
        return m_ptr != pointer();
    }

    // ---- modifiers ----

    pointer release() D_NOEXCEPT
    {
        pointer _old = m_ptr;
        m_ptr = pointer();
        return _old;
    }

    // reset(nullptr) and reset() — explicit nullptr overload.
    void reset(std::nullptr_t = D_NULLPTR) D_NOEXCEPT
    {
        pointer _old = m_ptr;
        m_ptr = pointer();
        if (_old != pointer())
        {
            m_del(_old);
        }
    }

    // reset(pointer) — SFINAE-restricted like the ctors.
    template<typename _U>
    typename enable_if
    <
        is_same<_U, pointer>::value
        || (
            is_same<pointer, element_type*>::value
            && is_convertible<_U(*)[], element_type(*)[]>::value
        ),
        void
    >::type
    reset(_U _p) D_NOEXCEPT
    {
        pointer _old = m_ptr;
        m_ptr = _p;
        if (_old != pointer())
        {
            m_del(_old);
        }
    }

    void swap(unique_ptr& _other) D_NOEXCEPT
    {
        pointer _tmp_p = m_ptr;
        m_ptr = _other.m_ptr;
        _other.m_ptr = _tmp_p;

        deleter_type _tmp_d = restd::move(m_del);
        m_del = restd::move(_other.m_del);
        _other.m_del = restd::move(_tmp_d);
    }
};


// =============================================================================
// comparison operators  (unique_ptr <=> unique_ptr)
// =============================================================================
//
// Note: per [unique.ptr.special]/4-9, the relational operators are
// specified in terms of less<common_type_t<P1,P2>>. We use raw operator<
// here pending restd::less; this gives the same result on every flat-
// memory architecture in production today.

template<typename _T1, typename _D1, typename _T2, typename _D2>
inline bool operator==
(
    const unique_ptr<_T1, _D1>& _a,
    const unique_ptr<_T2, _D2>& _b
)
{
    return _a.get() == _b.get();
}

template<typename _T1, typename _D1, typename _T2, typename _D2>
inline bool operator!=
(
    const unique_ptr<_T1, _D1>& _a,
    const unique_ptr<_T2, _D2>& _b
)
{
    return _a.get() != _b.get();
}

template<typename _T1, typename _D1, typename _T2, typename _D2>
inline bool operator<
(
    const unique_ptr<_T1, _D1>& _a,
    const unique_ptr<_T2, _D2>& _b
)
{
    return _a.get() < _b.get();
}

template<typename _T1, typename _D1, typename _T2, typename _D2>
inline bool operator<=
(
    const unique_ptr<_T1, _D1>& _a,
    const unique_ptr<_T2, _D2>& _b
)
{
    return !(_b < _a);
}

template<typename _T1, typename _D1, typename _T2, typename _D2>
inline bool operator>
(
    const unique_ptr<_T1, _D1>& _a,
    const unique_ptr<_T2, _D2>& _b
)
{
    return _b < _a;
}

template<typename _T1, typename _D1, typename _T2, typename _D2>
inline bool operator>=
(
    const unique_ptr<_T1, _D1>& _a,
    const unique_ptr<_T2, _D2>& _b
)
{
    return !(_a < _b);
}


// =============================================================================
// comparison operators  (unique_ptr <=> nullptr)
// =============================================================================

template<typename _T, typename _D>
inline bool operator==
(
    const unique_ptr<_T, _D>& _a,
    std::nullptr_t
) D_NOEXCEPT
{
    return !_a;
}

template<typename _T, typename _D>
inline bool operator==
(
    std::nullptr_t,
    const unique_ptr<_T, _D>& _a
) D_NOEXCEPT
{
    return !_a;
}

template<typename _T, typename _D>
inline bool operator!=
(
    const unique_ptr<_T, _D>& _a,
    std::nullptr_t
) D_NOEXCEPT
{
    return static_cast<bool>(_a);
}

template<typename _T, typename _D>
inline bool operator!=
(
    std::nullptr_t,
    const unique_ptr<_T, _D>& _a
) D_NOEXCEPT
{
    return static_cast<bool>(_a);
}

template<typename _T, typename _D>
inline bool operator<
(
    const unique_ptr<_T, _D>& _a,
    std::nullptr_t
)
{
    return _a.get() < typename unique_ptr<_T, _D>::pointer();
}

template<typename _T, typename _D>
inline bool operator<
(
    std::nullptr_t,
    const unique_ptr<_T, _D>& _a
)
{
    return typename unique_ptr<_T, _D>::pointer() < _a.get();
}

template<typename _T, typename _D>
inline bool operator<=
(
    const unique_ptr<_T, _D>& _a,
    std::nullptr_t
)
{
    return !(D_NULLPTR < _a);
}

template<typename _T, typename _D>
inline bool operator<=
(
    std::nullptr_t,
    const unique_ptr<_T, _D>& _a
)
{
    return !(_a < D_NULLPTR);
}

template<typename _T, typename _D>
inline bool operator>
(
    const unique_ptr<_T, _D>& _a,
    std::nullptr_t
)
{
    return D_NULLPTR < _a;
}

template<typename _T, typename _D>
inline bool operator>
(
    std::nullptr_t,
    const unique_ptr<_T, _D>& _a
)
{
    return _a < D_NULLPTR;
}

template<typename _T, typename _D>
inline bool operator>=
(
    const unique_ptr<_T, _D>& _a,
    std::nullptr_t
)
{
    return !(_a < D_NULLPTR);
}

template<typename _T, typename _D>
inline bool operator>=
(
    std::nullptr_t,
    const unique_ptr<_T, _D>& _a
)
{
    return !(D_NULLPTR < _a);
}


}  // namespace restd

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // RESTD_MEMORY_UNIQUE_PTR_
