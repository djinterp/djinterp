/*******************************************************************************
* djinterp [re_std]                                               shared_ptr.hpp
*
* shared-ownership smart pointer:
*   shared_ptr<T> holds a strong reference to a managed object. The
* object is destroyed when the LAST shared_ptr to it is destroyed or
* reset. Ref counting is via an internal control block, atomic where
* the compiler supports it (see internal/sp_control_block.hpp).
*
* layout:
*   m_ptr    raw pointer to the managed object (T*)
*   m_ctrl   pointer to the control block
*   Both are independent: the aliasing constructor lets m_ptr point to
*   a sub-object while m_ctrl manages a containing object's lifetime.
*
* known limitations / deferrals (this phase):
*   1. Pointer casts (static/dynamic/const/reinterpret) ship in 4b.
*   2. owner_less ships in 4b.
*   3. get_deleter ships in 4b.
*   4. Atomic shared_ptr ops (atomic_load/store/cas) NOT planned —
*      use std::atomic<shared_ptr<T>> in C++20+ if needed; re_std has
*      no <atomic> equivalent.
*   5. shared_ptr(p, d, alloc) ctor (deleter + allocator together) NOT
*      yet shipped. shared_ptr(p), shared_ptr(p, d), and the
*      allocate_shared path cover the common cases.
*   6. operator<=> deferred (needs <compare>).
*   7. Comparison operators use raw < rather than less<CT>; same
*      caveat as unique_ptr.
*
*
* path:      /inc/re_std/memory/shared_ptr.hpp
* link(s):   TBA
* author(s): re_std contributors                             created: 2026.05.02
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_MEMORY_SHARED_PTR_HPP
#define RE_STD_MEMORY_SHARED_PTR_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_IS_CPP11_OR_HIGHER

    // std
    #include <cstddef>

    #if RE_STD_HAS_RTTI
        // std
        #include <typeinfo>
    #endif

    #include "re_std/memory/sp_control_block.hpp"
    #include "re_std/memory/allocator_traits.hpp"
    #include "re_std/memory/default_delete.hpp"
    #include "re_std/memory/unique_ptr.hpp"
    #include "re_std/type_traits/enable_if.hpp"
    #include "re_std/type_traits/is_array.hpp"
    #include "re_std/type_traits/is_convertible.hpp"
    #include "re_std/utility/move.hpp"


namespace re_std
{

// Forward declarations.
template<typename T> class weak_ptr;
template<typename T> class enable_shared_from_this;


// =============================================================================
// internal: enable_shared_from_this hook
// =============================================================================

// Catch-all: called when Y is not derived from any
// enable_shared_from_this<U>. Lives in re_std:: (not re_std::internal::)
// so that unqualified `sp_esft_link(...)` calls in shared_ptr's
// internals find it via ordinary unqualified lookup. The inline
// friend in enable_shared_from_this is also in re_std:: (members of
// the enclosing namespace), and ADL adds it as a candidate when an
// argument's associated class set includes enable_shared_from_this<U>.
// Overload resolution then picks the more-specific friend over the
// variadic catch-all.
inline void sp_esft_link(...) RE_STD_NOEXCEPT
{
}


// =============================================================================
// shared_ptr<T>
// =============================================================================

template<typename T>
class shared_ptr
{
public:
    typedef T element_type;

    // weak_ptr / shared_ptr / esft can poke at m_ptr / m_ctrl directly.
    template<typename U> friend class shared_ptr;
    template<typename U> friend class weak_ptr;

private:
    element_type*                    m_ptr;
    internal::sp_control_block_base* m_ctrl;

    // Tag-dispatched private ctor: take ownership of (ptr, cb) without
    // bumping refcount. Used by make_shared / allocate_shared which
    // construct the cb with refcount = 1 already.
    struct from_cb_tag_t {};

public:
    // Public-but-underscored factory used by make_shared /
    // allocate_shared. Takes ownership of (ptr, cb) without bumping
    // refcount. Not for user code.
    template<typename U>
    static shared_ptr _sp_internal_from_cb(
        U* _p,
        internal::sp_control_block_base* _cb) RE_STD_NOEXCEPT
    {
        shared_ptr _r;
        _r.m_ptr  = _p;
        _r.m_ctrl = _cb;
        sp_esft_link(_cb, _p, _p);
        return _r;
    }

    // Public-but-underscored deleter accessor used by re_std::get_deleter.
    // Returns a void* to the deleter when the control block holds one
    // of the requested type, else null. Public because get_deleter is
    // a function template and friending it across the boundary is
    // syntactically painful for the SFINAE form.
    #if RE_STD_HAS_RTTI
        void* _sp_internal_get_deleter(const std::type_info& _ti) const
            RE_STD_NOEXCEPT
        {
            return m_ctrl ? m_ctrl->get_deleter(_ti) : 0;
        }
    #endif

public:
    // NB: this specifier was `private:` and never reopened, which made
    // every constructor, the destructor, get(), operator*, reset() and
    // use_count() on the PRIMARY shared_ptr<T> inaccessible -- the class
    // parsed fine and was simply unusable. The array specialisations
    // below get this right (they reopen with `public:` after their member
    // block), which is what the primary was meant to mirror.
    // -------------------------------------------------------------------------
    // construction
    // -------------------------------------------------------------------------

    RE_STD_CONSTEXPR shared_ptr() RE_STD_NOEXCEPT
        : m_ptr(0)
        , m_ctrl(0)
    {
    }

    RE_STD_CONSTEXPR shared_ptr(std::nullptr_t) RE_STD_NOEXCEPT
        : m_ptr(0)
        , m_ctrl(0)
    {
    }

    // From raw pointer with default deleter.
    template
    <
        typename Y,
        typename = typename enable_if
        <
            is_convertible<Y*, element_type*>::value
        >::type
    >
    explicit shared_ptr(Y* _p)
        : m_ptr(_p)
        , m_ctrl(_p
            ? new internal::sp_cb_pointer<Y, default_delete<Y> >
                  (_p, default_delete<Y>())
            : 0)
    {
        sp_esft_link(m_ctrl, _p, _p);
    }

    // From raw pointer with custom deleter.
    template
    <
        typename Y,
        typename D,
        typename = typename enable_if
        <
            is_convertible<Y*, element_type*>::value
        >::type
    >
    shared_ptr(Y* _p, D _d)
        : m_ptr(_p)
        , m_ctrl(new internal::sp_cb_pointer<Y, D>(_p, re_std::move(_d)))
    {
        sp_esft_link(m_ctrl, _p, _p);
    }

    // From nullptr with custom deleter.
    template<typename D>
    shared_ptr(std::nullptr_t, D _d)
        : m_ptr(0)
        , m_ctrl(new internal::sp_cb_pointer<element_type, D>
                     (0, re_std::move(_d)))
    {
    }

    // From raw pointer + custom deleter + allocator. Allocator-aware
    // pointer cb; the cb itself is allocated via A (rebound), and
    // a copy of A is held inside it for later self-deallocation.
    //
    // Standard semantics ([util.smartptr.shared.const]/9-10): if
    // construction throws, _d(_p) is invoked and the exception
    // propagates. We deallocate the cb storage and re-raise.
    template
    <
        typename Y,
        typename D,
        typename A,
        typename = typename enable_if
        <
            is_convertible<Y*, element_type*>::value
        >::type
    >
    shared_ptr(Y* _p, D _d, A _a)
        : m_ptr(0)
        , m_ctrl(0)
    {
        typedef internal::sp_cb_pointer_alloc<Y, D, A>      cb_t;
        typedef typename allocator_traits<A>
            ::template rebind_alloc<cb_t>                      alloc_cb_t;
        typedef allocator_traits<alloc_cb_t>                   cb_traits;

        alloc_cb_t _a_cb(_a);
        cb_t* _cb = cb_traits::allocate(_a_cb, 1);

        #if RE_STD_HAS_EXCEPTIONS
            try
            {
                cb_traits::construct(_a_cb, _cb,
                                     _p, re_std::move(_d), _a);
            }
            catch (...)
            {
                _d(_p);
                cb_traits::deallocate(_a_cb, _cb, 1);
                throw;
            }
        #else
            cb_traits::construct(_a_cb, _cb,
                                 _p, re_std::move(_d), _a);
        #endif

        m_ptr  = _p;
        m_ctrl = _cb;
        sp_esft_link(m_ctrl, _p, _p);
    }

    // From nullptr + custom deleter + allocator.
    template<typename D, typename A>
    shared_ptr(std::nullptr_t, D _d, A _a)
        : m_ptr(0)
        , m_ctrl(0)
    {
        typedef internal::sp_cb_pointer_alloc<element_type, D, A>  cb_t;
        typedef typename allocator_traits<A>
            ::template rebind_alloc<cb_t>                            alloc_cb_t;
        typedef allocator_traits<alloc_cb_t>                         cb_traits;

        alloc_cb_t _a_cb(_a);
        cb_t* _cb = cb_traits::allocate(_a_cb, 1);

        #if RE_STD_HAS_EXCEPTIONS
            try
            {
                cb_traits::construct(_a_cb, _cb,
                                     static_cast<element_type*>(0),
                                     re_std::move(_d), _a);
            }
            catch (...)
            {
                _d(static_cast<element_type*>(0));
                cb_traits::deallocate(_a_cb, _cb, 1);
                throw;
            }
        #else
            cb_traits::construct(_a_cb, _cb,
                                 static_cast<element_type*>(0),
                                 re_std::move(_d), _a);
        #endif

        m_ctrl = _cb;
    }

    // Aliasing constructor.
    template<typename U>
    shared_ptr(const shared_ptr<U>& _r,
               element_type* _p) RE_STD_NOEXCEPT
        : m_ptr(_p)
        , m_ctrl(_r.m_ctrl)
    {
        if (m_ctrl)
        {
            m_ctrl->add_ref();
        }
    }

    // Aliasing constructor (rvalue, C++17+ but harmless to ship now).
    template<typename U>
    shared_ptr(shared_ptr<U>&& _r,
               element_type* _p) RE_STD_NOEXCEPT
        : m_ptr(_p)
        , m_ctrl(_r.m_ctrl)
    {
        _r.m_ptr = 0;
        _r.m_ctrl = 0;
    }

    // Copy ctor (same type).
    shared_ptr(const shared_ptr& _r) RE_STD_NOEXCEPT
        : m_ptr(_r.m_ptr)
        , m_ctrl(_r.m_ctrl)
    {
        if (m_ctrl)
        {
            m_ctrl->add_ref();
        }
    }

    // Converting copy ctor.
    template
    <
        typename Y,
        typename = typename enable_if
        <
            is_convertible<Y*, element_type*>::value
        >::type
    >
    shared_ptr(const shared_ptr<Y>& _r) RE_STD_NOEXCEPT
        : m_ptr(_r.m_ptr)
        , m_ctrl(_r.m_ctrl)
    {
        if (m_ctrl)
        {
            m_ctrl->add_ref();
        }
    }

    // Move ctor (same type).
    shared_ptr(shared_ptr&& _r) RE_STD_NOEXCEPT
        : m_ptr(_r.m_ptr)
        , m_ctrl(_r.m_ctrl)
    {
        _r.m_ptr = 0;
        _r.m_ctrl = 0;
    }

    // Converting move ctor.
    template
    <
        typename Y,
        typename = typename enable_if
        <
            is_convertible<Y*, element_type*>::value
        >::type
    >
    shared_ptr(shared_ptr<Y>&& _r) RE_STD_NOEXCEPT
        : m_ptr(_r.m_ptr)
        , m_ctrl(_r.m_ctrl)
    {
        _r.m_ptr = 0;
        _r.m_ctrl = 0;
    }

    // From weak_ptr (declared here, defined in weak_ptr.hpp).
    template<typename Y>
    explicit shared_ptr(const weak_ptr<Y>& _w);

    // From unique_ptr.
    template
    <
        typename Y,
        typename D,
        typename = typename enable_if
        <
            is_convertible<Y*, element_type*>::value
            && !is_array<Y>::value
        >::type
    >
    shared_ptr(unique_ptr<Y, D>&& _u)
        : m_ptr(_u.get())
        , m_ctrl(_u.get()
            ? new internal::sp_cb_pointer<Y, D>
                  (_u.get(), re_std::move(_u.get_deleter()))
            : 0)
    {
        if (_u.get())
        {
            sp_esft_link(m_ctrl, _u.get(), _u.get());
        }
        _u.release();
    }

    // -------------------------------------------------------------------------
    // destruction
    // -------------------------------------------------------------------------

    ~shared_ptr()
    {
        if (m_ctrl)
        {
            m_ctrl->release();
        }
    }

    // -------------------------------------------------------------------------
    // assignment
    // -------------------------------------------------------------------------

    shared_ptr& operator=(const shared_ptr& _r) RE_STD_NOEXCEPT
    {
        shared_ptr(_r).swap(*this);
        return *this;
    }

    template<typename Y>
    typename enable_if
    <
        is_convertible<Y*, element_type*>::value,
        shared_ptr&
    >::type
    operator=(const shared_ptr<Y>& _r) RE_STD_NOEXCEPT
    {
        shared_ptr(_r).swap(*this);
        return *this;
    }

    shared_ptr& operator=(shared_ptr&& _r) RE_STD_NOEXCEPT
    {
        shared_ptr(re_std::move(_r)).swap(*this);
        return *this;
    }

    template<typename Y>
    typename enable_if
    <
        is_convertible<Y*, element_type*>::value,
        shared_ptr&
    >::type
    operator=(shared_ptr<Y>&& _r) RE_STD_NOEXCEPT
    {
        shared_ptr(re_std::move(_r)).swap(*this);
        return *this;
    }

    template<typename Y, typename D>
    typename enable_if
    <
        is_convertible<Y*, element_type*>::value && !is_array<Y>::value,
        shared_ptr&
    >::type
    operator=(unique_ptr<Y, D>&& _u)
    {
        shared_ptr(re_std::move(_u)).swap(*this);
        return *this;
    }

    // -------------------------------------------------------------------------
    // modifiers
    // -------------------------------------------------------------------------

    void reset() RE_STD_NOEXCEPT
    {
        shared_ptr().swap(*this);
    }

    template<typename Y>
    typename enable_if
    <
        is_convertible<Y*, element_type*>::value,
        void
    >::type
    reset(Y* _p)
    {
        shared_ptr(_p).swap(*this);
    }

    template<typename Y, typename D>
    typename enable_if
    <
        is_convertible<Y*, element_type*>::value,
        void
    >::type
    reset(Y* _p, D _d)
    {
        shared_ptr(_p, re_std::move(_d)).swap(*this);
    }

    void swap(shared_ptr& _r) RE_STD_NOEXCEPT
    {
        element_type* _tp = m_ptr;
        m_ptr = _r.m_ptr;
        _r.m_ptr = _tp;

        internal::sp_control_block_base* _tc = m_ctrl;
        m_ctrl = _r.m_ctrl;
        _r.m_ctrl = _tc;
    }

    // -------------------------------------------------------------------------
    // observers
    // -------------------------------------------------------------------------

    element_type* get() const RE_STD_NOEXCEPT
    {
        return m_ptr;
    }

    element_type& operator*() const RE_STD_NOEXCEPT
    {
        return *m_ptr;
    }

    element_type* operator->() const RE_STD_NOEXCEPT
    {
        return m_ptr;
    }

    long use_count() const RE_STD_NOEXCEPT
    {
        return m_ctrl ? static_cast<long>(m_ctrl->use_count()) : 0;
    }

    // unique() is deprecated in std C++17, removed C++20. re_std retains.
    bool unique() const RE_STD_NOEXCEPT
    {
        return use_count() == 1;
    }

    explicit operator bool() const RE_STD_NOEXCEPT
    {
        return m_ptr != 0;
    }

    // owner_before: ordering by control-block address.
    template<typename Y>
    bool owner_before(const shared_ptr<Y>& _r) const RE_STD_NOEXCEPT
    {
        return m_ctrl < _r.m_ctrl;
    }

    template<typename Y>
    bool owner_before(const weak_ptr<Y>& _r) const RE_STD_NOEXCEPT;
};


// =============================================================================
// comparison operators
// =============================================================================

template<typename T1, typename T2>
inline bool operator==(const shared_ptr<T1>& _a, const shared_ptr<T2>& _b)
    RE_STD_NOEXCEPT
{
    return _a.get() == _b.get();
}

template<typename T1, typename T2>
inline bool operator!=(const shared_ptr<T1>& _a, const shared_ptr<T2>& _b)
    RE_STD_NOEXCEPT
{
    return _a.get() != _b.get();
}

template<typename T1, typename T2>
inline bool operator<(const shared_ptr<T1>& _a, const shared_ptr<T2>& _b)
    RE_STD_NOEXCEPT
{
    return _a.get() < _b.get();
}

template<typename T1, typename T2>
inline bool operator<=(const shared_ptr<T1>& _a, const shared_ptr<T2>& _b)
    RE_STD_NOEXCEPT
{
    return !(_b < _a);
}

template<typename T1, typename T2>
inline bool operator>(const shared_ptr<T1>& _a, const shared_ptr<T2>& _b)
    RE_STD_NOEXCEPT
{
    return _b < _a;
}

template<typename T1, typename T2>
inline bool operator>=(const shared_ptr<T1>& _a, const shared_ptr<T2>& _b)
    RE_STD_NOEXCEPT
{
    return !(_a < _b);
}


// nullptr comparisons.
template<typename T>
inline bool operator==(const shared_ptr<T>& _a, std::nullptr_t) RE_STD_NOEXCEPT
{
    return !_a;
}

template<typename T>
inline bool operator==(std::nullptr_t, const shared_ptr<T>& _a) RE_STD_NOEXCEPT
{
    return !_a;
}

template<typename T>
inline bool operator!=(const shared_ptr<T>& _a, std::nullptr_t) RE_STD_NOEXCEPT
{
    return static_cast<bool>(_a);
}

template<typename T>
inline bool operator!=(std::nullptr_t, const shared_ptr<T>& _a) RE_STD_NOEXCEPT
{
    return static_cast<bool>(_a);
}


// =============================================================================
// shared_ptr<T[]>  -  array specialization
// =============================================================================
//
// Differs from the primary template in:
//   - element_type is T (the element), not the array itself
//   - m_ptr is element_type*, not T(*)[]
//   - operator[] replaces operator* / operator->
//   - conversion from shared_ptr<Y[]> requires Y(*)[] -> T(*)[]
//     (qualification only, no covariance — same rule as unique_ptr<T[]>)
//   - the default deleter is default_delete<T[]>, which uses delete[]
//
// shared_ptr<T[N]> (bounded array) is NOT YET implemented; it follows the
// same template structure with N as a non-type parameter. Will ship in
// a follow-up.

template<typename T>
class shared_ptr<T[]>
{
public:
    typedef T  element_type;

    template<typename U> friend class shared_ptr;
    template<typename U> friend class weak_ptr;

private:
    element_type*                    m_ptr;
    internal::sp_control_block_base* m_ctrl;

public:
    // Public-but-underscored factory mirroring the primary template's.
    template<typename U>
    static shared_ptr _sp_internal_from_cb(
        U* _p,
        internal::sp_control_block_base* _cb) RE_STD_NOEXCEPT
    {
        shared_ptr _r;
        _r.m_ptr  = _p;
        _r.m_ctrl = _cb;
        // No esft for arrays — shared_from_this is not meaningful for
        // array element types.
        return _r;
    }

    // ---- ctors ----

    RE_STD_CONSTEXPR shared_ptr() RE_STD_NOEXCEPT
        : m_ptr(0)
        , m_ctrl(0)
    {
    }

    RE_STD_CONSTEXPR shared_ptr(std::nullptr_t) RE_STD_NOEXCEPT
        : m_ptr(0)
        , m_ctrl(0)
    {
    }

    // From raw pointer with default deleter (default_delete<T[]>,
    // which calls delete[]). Conversion is qualification-only on
    // Y(*)[] -> element_type(*)[].
    template
    <
        typename Y,
        typename = typename enable_if
        <
            is_convertible<Y(*)[], element_type(*)[]>::value
        >::type
    >
    explicit shared_ptr(Y* _p)
        : m_ptr(_p)
        , m_ctrl(_p
            ? new internal::sp_cb_pointer<Y, default_delete<Y[]> >
                  (_p, default_delete<Y[]>())
            : 0)
    {
    }

    // From raw pointer with custom deleter.
    template
    <
        typename Y,
        typename D,
        typename = typename enable_if
        <
            is_convertible<Y(*)[], element_type(*)[]>::value
        >::type
    >
    shared_ptr(Y* _p, D _d)
        : m_ptr(_p)
        , m_ctrl(new internal::sp_cb_pointer<Y, D>(_p, re_std::move(_d)))
    {
    }

    // Aliasing ctor.
    template<typename U>
    shared_ptr(const shared_ptr<U>& _r,
               element_type* _p) RE_STD_NOEXCEPT
        : m_ptr(_p)
        , m_ctrl(_r.m_ctrl)
    {
        if (m_ctrl)
        {
            m_ctrl->add_ref();
        }
    }

    template<typename U>
    shared_ptr(shared_ptr<U>&& _r,
               element_type* _p) RE_STD_NOEXCEPT
        : m_ptr(_p)
        , m_ctrl(_r.m_ctrl)
    {
        _r.m_ptr = 0;
        _r.m_ctrl = 0;
    }

    // Copy / move ctors.
    shared_ptr(const shared_ptr& _r) RE_STD_NOEXCEPT
        : m_ptr(_r.m_ptr)
        , m_ctrl(_r.m_ctrl)
    {
        if (m_ctrl)
        {
            m_ctrl->add_ref();
        }
    }

    shared_ptr(shared_ptr&& _r) RE_STD_NOEXCEPT
        : m_ptr(_r.m_ptr)
        , m_ctrl(_r.m_ctrl)
    {
        _r.m_ptr = 0;
        _r.m_ctrl = 0;
    }

    // Converting copy/move from another shared_ptr<Y[]>.
    template
    <
        typename Y,
        typename = typename enable_if
        <
            is_convertible<Y(*)[], element_type(*)[]>::value
        >::type
    >
    shared_ptr(const shared_ptr<Y[]>& _r) RE_STD_NOEXCEPT
        : m_ptr(_r.m_ptr)
        , m_ctrl(_r.m_ctrl)
    {
        if (m_ctrl)
        {
            m_ctrl->add_ref();
        }
    }

    template
    <
        typename Y,
        typename = typename enable_if
        <
            is_convertible<Y(*)[], element_type(*)[]>::value
        >::type
    >
    shared_ptr(shared_ptr<Y[]>&& _r) RE_STD_NOEXCEPT
        : m_ptr(_r.m_ptr)
        , m_ctrl(_r.m_ctrl)
    {
        _r.m_ptr = 0;
        _r.m_ctrl = 0;
    }

    // ---- dtor ----

    ~shared_ptr()
    {
        if (m_ctrl)
        {
            m_ctrl->release();
        }
    }

    // ---- assignment ----

    shared_ptr& operator=(const shared_ptr& _r) RE_STD_NOEXCEPT
    {
        shared_ptr(_r).swap(*this);
        return *this;
    }

    shared_ptr& operator=(shared_ptr&& _r) RE_STD_NOEXCEPT
    {
        shared_ptr(re_std::move(_r)).swap(*this);
        return *this;
    }

    // ---- modifiers ----

    void reset() RE_STD_NOEXCEPT
    {
        shared_ptr().swap(*this);
    }

    void swap(shared_ptr& _r) RE_STD_NOEXCEPT
    {
        element_type* _tp = m_ptr;
        m_ptr = _r.m_ptr;
        _r.m_ptr = _tp;

        internal::sp_control_block_base* _tc = m_ctrl;
        m_ctrl = _r.m_ctrl;
        _r.m_ctrl = _tc;
    }

    // ---- observers ----

    element_type* get() const RE_STD_NOEXCEPT
    {
        return m_ptr;
    }

    element_type& operator[](std::ptrdiff_t _i) const RE_STD_NOEXCEPT
    {
        return m_ptr[_i];
    }

    long use_count() const RE_STD_NOEXCEPT
    {
        return m_ctrl ? static_cast<long>(m_ctrl->use_count()) : 0;
    }

    bool unique() const RE_STD_NOEXCEPT
    {
        return use_count() == 1;
    }

    explicit operator bool() const RE_STD_NOEXCEPT
    {
        return m_ptr != 0;
    }

    template<typename Y>
    bool owner_before(const shared_ptr<Y>& _r) const RE_STD_NOEXCEPT
    {
        return m_ctrl < _r.m_ctrl;
    }
};


// =============================================================================
// shared_ptr<T[N]>  -  bounded-array specialization
// =============================================================================
//
// Same shape as shared_ptr<T[]>, but the type encodes N. Conversions:
//   - shared_ptr<T[N]> -> shared_ptr<T[]>   IS provided (decay)
//   - shared_ptr<T[N]> <-> shared_ptr<U[N]> with qualification-only
//     convertibility is NOT YET provided. Add when needed.

template<typename T, std::size_t N>
class shared_ptr<T[N]>
{
public:
    typedef T  element_type;

    template<typename U> friend class shared_ptr;
    template<typename U> friend class weak_ptr;

private:
    element_type*                    m_ptr;
    internal::sp_control_block_base* m_ctrl;

public:
    template<typename U>
    static shared_ptr _sp_internal_from_cb(
        U* _p,
        internal::sp_control_block_base* _cb) RE_STD_NOEXCEPT
    {
        shared_ptr _r;
        _r.m_ptr  = _p;
        _r.m_ctrl = _cb;
        return _r;
    }

    RE_STD_CONSTEXPR shared_ptr() RE_STD_NOEXCEPT
        : m_ptr(0), m_ctrl(0) {}

    RE_STD_CONSTEXPR shared_ptr(std::nullptr_t) RE_STD_NOEXCEPT
        : m_ptr(0), m_ctrl(0) {}

    shared_ptr(const shared_ptr& _r) RE_STD_NOEXCEPT
        : m_ptr(_r.m_ptr), m_ctrl(_r.m_ctrl)
    {
        if (m_ctrl) m_ctrl->add_ref();
    }

    shared_ptr(shared_ptr&& _r) RE_STD_NOEXCEPT
        : m_ptr(_r.m_ptr), m_ctrl(_r.m_ctrl)
    {
        _r.m_ptr = 0;
        _r.m_ctrl = 0;
    }

    ~shared_ptr()
    {
        if (m_ctrl) m_ctrl->release();
    }

    shared_ptr& operator=(const shared_ptr& _r) RE_STD_NOEXCEPT
    {
        shared_ptr(_r).swap(*this);
        return *this;
    }

    shared_ptr& operator=(shared_ptr&& _r) RE_STD_NOEXCEPT
    {
        shared_ptr(re_std::move(_r)).swap(*this);
        return *this;
    }

    void reset() RE_STD_NOEXCEPT { shared_ptr().swap(*this); }

    void swap(shared_ptr& _r) RE_STD_NOEXCEPT
    {
        element_type* _tp = m_ptr; m_ptr = _r.m_ptr; _r.m_ptr = _tp;
        internal::sp_control_block_base* _tc = m_ctrl;
        m_ctrl = _r.m_ctrl; _r.m_ctrl = _tc;
    }

    element_type* get() const RE_STD_NOEXCEPT { return m_ptr; }

    element_type& operator[](std::ptrdiff_t _i) const RE_STD_NOEXCEPT
    {
        return m_ptr[_i];
    }

    long use_count() const RE_STD_NOEXCEPT
    {
        return m_ctrl ? static_cast<long>(m_ctrl->use_count()) : 0;
    }

    bool unique() const RE_STD_NOEXCEPT { return use_count() == 1; }

    explicit operator bool() const RE_STD_NOEXCEPT { return m_ptr != 0; }

    template<typename Y>
    bool owner_before(const shared_ptr<Y>& _r) const RE_STD_NOEXCEPT
    {
        return m_ctrl < _r.m_ctrl;
    }

    // Decay conversion: shared_ptr<T[N]> -> shared_ptr<T[]>.
    operator shared_ptr<T[]>() const RE_STD_NOEXCEPT
    {
        if (m_ctrl) m_ctrl->add_ref();
        return shared_ptr<T[]>::_sp_internal_from_cb(m_ptr, m_ctrl);
    }
};


}  // re_std
#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER

#endif  // RE_STD_MEMORY_SHARED_PTR_HPP
