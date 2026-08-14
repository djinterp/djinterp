/***********************************************************************
* restd                                                        weak_ptr.hpp
*
* non-owning observer of a shared_ptr-managed object:
*   weak_ptr<_T> tracks an object without keeping it alive. Use lock()
* to obtain a shared_ptr if the object still exists, or expired() to
* check.
*
* must be included for shared_ptr's weak_ptr-using API to be complete:
*   shared_ptr.hpp forward-declares weak_ptr, but its ctor from
* weak_ptr (shared_ptr<T>(const weak_ptr<U>&)) and owner_before(weak)
* are defined here. Code that uses those needs to #include this file
* (the restd/memory umbrella does it transitively).
*
*
* path:      /inc/djinterp/re_std/memory/weak_ptr.hpp
* link(s):   TBA
* author(s): restd contributors                          date: 2026.05.02
***********************************************************************/

#ifndef RESTD_MEMORY_WEAK_PTR_
#define RESTD_MEMORY_WEAK_PTR_ 1

#include "djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

    #include "restd/memory/shared_ptr.hpp"
    #include "restd/memory/bad_weak_ptr.hpp"
    #include "restd/type_traits/enable_if.hpp"
    #include "restd/type_traits/is_convertible.hpp"


namespace restd
{

// =============================================================================
// weak_ptr<_T>
// =============================================================================

template<typename _T>
class weak_ptr
{
public:
    typedef _T element_type;

    template<typename _U> friend class weak_ptr;
    template<typename _U> friend class shared_ptr;
    template<typename _U> friend class enable_shared_from_this;

private:
    element_type*                    m_ptr;
    internal::sp_control_block_base* m_ctrl;

public:
    // ---- intentionally public, implementation-detail interface ----
    //
    // Used by enable_shared_from_this's inline-friend hook. The name
    // _sp_internal_* signals "do not call from user code." We keep it
    // public because the inline friend in enable_shared_from_this is
    // a free function (not a member of any class), so the standard
    // "friend class" route does not grant it access.

    void _sp_internal_assign(element_type* _p,
                             internal::sp_control_block_base* _cb) D_NOEXCEPT
    {
        // Bumps the weak refcount on the new cb. Assumes weak_ptr was
        // previously empty (called only from esft init while shared_ptr
        // is being constructed).
        m_ptr = _p;
        m_ctrl = _cb;
        if (m_ctrl)
        {
            m_ctrl->weak_add_ref();
        }
    }

    // -------------------------------------------------------------------------
    // construction
    // -------------------------------------------------------------------------

    D_CONSTEXPR weak_ptr() D_NOEXCEPT
        : m_ptr(0)
        , m_ctrl(0)
    {
    }

    weak_ptr(const weak_ptr& _r) D_NOEXCEPT
        : m_ptr(_r.m_ptr)
        , m_ctrl(_r.m_ctrl)
    {
        if (m_ctrl)
        {
            m_ctrl->weak_add_ref();
        }
    }

    template
    <
        typename _Y,
        typename = typename enable_if
        <
            is_convertible<_Y*, element_type*>::value
        >::type
    >
    weak_ptr(const weak_ptr<_Y>& _r) D_NOEXCEPT
        : m_ptr(_r.m_ptr)
        , m_ctrl(_r.m_ctrl)
    {
        if (m_ctrl)
        {
            m_ctrl->weak_add_ref();
        }
    }

    // From shared_ptr.
    template
    <
        typename _Y,
        typename = typename enable_if
        <
            is_convertible<_Y*, element_type*>::value
        >::type
    >
    weak_ptr(const shared_ptr<_Y>& _r) D_NOEXCEPT
        : m_ptr(_r.m_ptr)
        , m_ctrl(_r.m_ctrl)
    {
        if (m_ctrl)
        {
            m_ctrl->weak_add_ref();
        }
    }

    weak_ptr(weak_ptr&& _r) D_NOEXCEPT
        : m_ptr(_r.m_ptr)
        , m_ctrl(_r.m_ctrl)
    {
        _r.m_ptr = 0;
        _r.m_ctrl = 0;
    }

    template
    <
        typename _Y,
        typename = typename enable_if
        <
            is_convertible<_Y*, element_type*>::value
        >::type
    >
    weak_ptr(weak_ptr<_Y>&& _r) D_NOEXCEPT
        : m_ptr(_r.m_ptr)
        , m_ctrl(_r.m_ctrl)
    {
        _r.m_ptr = 0;
        _r.m_ctrl = 0;
    }

    // -------------------------------------------------------------------------
    // destruction
    // -------------------------------------------------------------------------

    ~weak_ptr()
    {
        if (m_ctrl)
        {
            m_ctrl->weak_release();
        }
    }

    // -------------------------------------------------------------------------
    // assignment
    // -------------------------------------------------------------------------

    weak_ptr& operator=(const weak_ptr& _r) D_NOEXCEPT
    {
        weak_ptr(_r).swap(*this);
        return *this;
    }

    template<typename _Y>
    typename enable_if
    <
        is_convertible<_Y*, element_type*>::value,
        weak_ptr&
    >::type
    operator=(const weak_ptr<_Y>& _r) D_NOEXCEPT
    {
        weak_ptr(_r).swap(*this);
        return *this;
    }

    template<typename _Y>
    typename enable_if
    <
        is_convertible<_Y*, element_type*>::value,
        weak_ptr&
    >::type
    operator=(const shared_ptr<_Y>& _r) D_NOEXCEPT
    {
        weak_ptr(_r).swap(*this);
        return *this;
    }

    weak_ptr& operator=(weak_ptr&& _r) D_NOEXCEPT
    {
        weak_ptr(restd::move(_r)).swap(*this);
        return *this;
    }

    template<typename _Y>
    typename enable_if
    <
        is_convertible<_Y*, element_type*>::value,
        weak_ptr&
    >::type
    operator=(weak_ptr<_Y>&& _r) D_NOEXCEPT
    {
        weak_ptr(restd::move(_r)).swap(*this);
        return *this;
    }

    // -------------------------------------------------------------------------
    // modifiers
    // -------------------------------------------------------------------------

    void reset() D_NOEXCEPT
    {
        weak_ptr().swap(*this);
    }

    void swap(weak_ptr& _r) D_NOEXCEPT
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

    long use_count() const D_NOEXCEPT
    {
        return m_ctrl ? static_cast<long>(m_ctrl->use_count()) : 0;
    }

    bool expired() const D_NOEXCEPT
    {
        return use_count() == 0;
    }

    // lock(): atomic check + bump. Returns empty if expired.
    shared_ptr<_T> lock() const D_NOEXCEPT
    {
        if (m_ctrl && m_ctrl->add_ref_if_nonzero())
        {
            // Bypass the public ctor (which would also bump). Use the
            // private from_cb_tag_t ctor by friending. Since
            // shared_ptr's ctor that bumps is the copy ctor, and we
            // want to NOT bump (we already did), we use the trick of
            // constructing via a moved-from-empty shared_ptr.
            shared_ptr<_T> _sp;
            _sp.m_ptr = m_ptr;
            _sp.m_ctrl = m_ctrl;
            return _sp;
        }
        return shared_ptr<_T>();
    }

    template<typename _Y>
    bool owner_before(const weak_ptr<_Y>& _r) const D_NOEXCEPT
    {
        return m_ctrl < _r.m_ctrl;
    }

    template<typename _Y>
    bool owner_before(const shared_ptr<_Y>& _r) const D_NOEXCEPT
    {
        return m_ctrl < _r.m_ctrl;
    }
};


// =============================================================================
// shared_ptr's weak_ptr-using ctor body  (declared in shared_ptr.hpp)
// =============================================================================

template<typename _T>
template<typename _Y>
shared_ptr<_T>::shared_ptr(const weak_ptr<_Y>& _w)
    : m_ptr(0)
    , m_ctrl(0)
{
    if (_w.m_ctrl && _w.m_ctrl->add_ref_if_nonzero())
    {
        m_ptr = _w.m_ptr;
        m_ctrl = _w.m_ctrl;
    }
    else
    {
        #if D_ENV_CPP98_HAS_EXCEPTION
            throw bad_weak_ptr();
        #else
            // No exception support — leave empty. Caller is responsible.
        #endif
    }
}


// shared_ptr::owner_before(weak_ptr) body.
template<typename _T>
template<typename _Y>
bool shared_ptr<_T>::owner_before(const weak_ptr<_Y>& _r) const D_NOEXCEPT
{
    return m_ctrl < _r.m_ctrl;
}


// =============================================================================
// weak_ptr<_T[]>  -  array specialization
// =============================================================================
//
// A minimal specialization that mirrors weak_ptr<_T> but uses
// element_type* (= _T*) for storage and returns shared_ptr<_T[]> from
// lock(). weak_ptr never dereferences, so there are no operator*/->/[]
// changes to make.
//
// Conversion rules use the array qualification-only rule, matching
// shared_ptr<_T[]>.

template<typename _T>
class weak_ptr<_T[]>
{
public:
    typedef _T  element_type;

    template<typename _U> friend class weak_ptr;
    template<typename _U> friend class shared_ptr;

private:
    element_type*                    m_ptr;
    internal::sp_control_block_base* m_ctrl;

public:
    void _sp_internal_assign(element_type* _p,
                             internal::sp_control_block_base* _cb) D_NOEXCEPT
    {
        m_ptr = _p;
        m_ctrl = _cb;
        if (m_ctrl)
        {
            m_ctrl->weak_add_ref();
        }
    }

    D_CONSTEXPR weak_ptr() D_NOEXCEPT
        : m_ptr(0)
        , m_ctrl(0)
    {
    }

    weak_ptr(const weak_ptr& _r) D_NOEXCEPT
        : m_ptr(_r.m_ptr)
        , m_ctrl(_r.m_ctrl)
    {
        if (m_ctrl)
        {
            m_ctrl->weak_add_ref();
        }
    }

    weak_ptr(const shared_ptr<_T[]>& _r) D_NOEXCEPT
        : m_ptr(_r.get())
        , m_ctrl(_r.m_ctrl)
    {
        if (m_ctrl)
        {
            m_ctrl->weak_add_ref();
        }
    }

    weak_ptr(weak_ptr&& _r) D_NOEXCEPT
        : m_ptr(_r.m_ptr)
        , m_ctrl(_r.m_ctrl)
    {
        _r.m_ptr = 0;
        _r.m_ctrl = 0;
    }

    ~weak_ptr()
    {
        if (m_ctrl)
        {
            m_ctrl->weak_release();
        }
    }

    weak_ptr& operator=(const weak_ptr& _r) D_NOEXCEPT
    {
        weak_ptr(_r).swap(*this);
        return *this;
    }

    weak_ptr& operator=(const shared_ptr<_T[]>& _r) D_NOEXCEPT
    {
        weak_ptr(_r).swap(*this);
        return *this;
    }

    weak_ptr& operator=(weak_ptr&& _r) D_NOEXCEPT
    {
        weak_ptr(restd::move(_r)).swap(*this);
        return *this;
    }

    void reset() D_NOEXCEPT
    {
        weak_ptr().swap(*this);
    }

    void swap(weak_ptr& _r) D_NOEXCEPT
    {
        element_type* _tp = m_ptr;
        m_ptr = _r.m_ptr;
        _r.m_ptr = _tp;

        internal::sp_control_block_base* _tc = m_ctrl;
        m_ctrl = _r.m_ctrl;
        _r.m_ctrl = _tc;
    }

    long use_count() const D_NOEXCEPT
    {
        return m_ctrl ? static_cast<long>(m_ctrl->use_count()) : 0;
    }

    bool expired() const D_NOEXCEPT
    {
        return use_count() == 0;
    }

    shared_ptr<_T[]> lock() const D_NOEXCEPT
    {
        if (m_ctrl && m_ctrl->add_ref_if_nonzero())
        {
            return shared_ptr<_T[]>::_sp_internal_from_cb(m_ptr, m_ctrl);
        }
        return shared_ptr<_T[]>();
    }
};


// =============================================================================
// weak_ptr<_T[_N]>  -  bounded-array specialization
// =============================================================================

template<typename _T, std::size_t _N>
class weak_ptr<_T[_N]>
{
public:
    typedef _T  element_type;

    template<typename _U> friend class weak_ptr;
    template<typename _U> friend class shared_ptr;

private:
    element_type*                    m_ptr;
    internal::sp_control_block_base* m_ctrl;

public:
    D_CONSTEXPR weak_ptr() D_NOEXCEPT
        : m_ptr(0), m_ctrl(0) {}

    weak_ptr(const weak_ptr& _r) D_NOEXCEPT
        : m_ptr(_r.m_ptr), m_ctrl(_r.m_ctrl)
    {
        if (m_ctrl) m_ctrl->weak_add_ref();
    }

    weak_ptr(const shared_ptr<_T[_N]>& _r) D_NOEXCEPT
        : m_ptr(_r.get()), m_ctrl(_r.m_ctrl)
    {
        if (m_ctrl) m_ctrl->weak_add_ref();
    }

    weak_ptr(weak_ptr&& _r) D_NOEXCEPT
        : m_ptr(_r.m_ptr), m_ctrl(_r.m_ctrl)
    {
        _r.m_ptr = 0;
        _r.m_ctrl = 0;
    }

    ~weak_ptr()
    {
        if (m_ctrl) m_ctrl->weak_release();
    }

    weak_ptr& operator=(const weak_ptr& _r) D_NOEXCEPT
    {
        weak_ptr(_r).swap(*this);
        return *this;
    }

    weak_ptr& operator=(const shared_ptr<_T[_N]>& _r) D_NOEXCEPT
    {
        weak_ptr(_r).swap(*this);
        return *this;
    }

    weak_ptr& operator=(weak_ptr&& _r) D_NOEXCEPT
    {
        weak_ptr(restd::move(_r)).swap(*this);
        return *this;
    }

    void reset() D_NOEXCEPT { weak_ptr().swap(*this); }

    void swap(weak_ptr& _r) D_NOEXCEPT
    {
        element_type* _tp = m_ptr; m_ptr = _r.m_ptr; _r.m_ptr = _tp;
        internal::sp_control_block_base* _tc = m_ctrl;
        m_ctrl = _r.m_ctrl; _r.m_ctrl = _tc;
    }

    long use_count() const D_NOEXCEPT
    {
        return m_ctrl ? static_cast<long>(m_ctrl->use_count()) : 0;
    }

    bool expired() const D_NOEXCEPT { return use_count() == 0; }

    shared_ptr<_T[_N]> lock() const D_NOEXCEPT
    {
        if (m_ctrl && m_ctrl->add_ref_if_nonzero())
        {
            return shared_ptr<_T[_N]>::_sp_internal_from_cb(m_ptr, m_ctrl);
        }
        return shared_ptr<_T[_N]>();
    }
};


}  // namespace restd

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // RESTD_MEMORY_WEAK_PTR_
