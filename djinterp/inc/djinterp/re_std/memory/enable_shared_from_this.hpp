/***********************************************************************
* restd                                       enable_shared_from_this.hpp
*
* CRTP base for types that need to obtain a shared_ptr to themselves:
*
*     class Foo : public restd::enable_shared_from_this<Foo>
*     {
*         restd::shared_ptr<Foo> bar()
*         {
*             return shared_from_this();
*         }
*     };
*
* shared_from_this() must only be called when at least one shared_ptr
* to *this exists (i.e., a shared_ptr-managed Foo). Otherwise it
* throws bad_weak_ptr (or, when exceptions are unavailable, returns
* an empty shared_ptr).
*
* implementation note (the inline-friend ADL trick):
*   The class declares a templated inline friend `sp_esft_link` that
* takes an enable_shared_from_this<_T>* among its arguments. When
* shared_ptr's ctor calls sp_esft_link(cb, ptr, ptr), ADL on ptr's
* type finds this friend ONLY if the pointee derives from
* enable_shared_from_this<U> for some U. Otherwise, only the variadic
* catch-all in restd::internal:: matches, and the call is a no-op.
*
* This is the same idiom libstdc++ uses for std::enable_shared_from_this.
* It correctly handles cases where T derives from enable_shared_from_this<U>
* via several inheritance hops.
*
*
* path:      /inc/restd/memory/enable_shared_from_this.hpp
* link(s):   TBA
* author(s): restd contributors                          date: 2026.05.02
***********************************************************************/

#ifndef RESTD_MEMORY_ENABLE_SHARED_FROM_THIS_
#define RESTD_MEMORY_ENABLE_SHARED_FROM_THIS_ 1

#include "djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

    #include "restd/memory/shared_ptr.hpp"
    #include "restd/memory/weak_ptr.hpp"


namespace restd
{

// =============================================================================
// enable_shared_from_this<_T>
// =============================================================================

template<typename _T>
class enable_shared_from_this
{
private:
    mutable weak_ptr<_T> m_weak_this;

    // Inline friend, found by ADL when an argument's associated
    // classes include enable_shared_from_this<_T> (i.e. the pointee
    // derives from it). Templated on _Y so the deducer captures the
    // most-derived static type at the call site.
    template<typename _Y>
    friend void sp_esft_link
    (
        internal::sp_control_block_base*    _cb,
        const enable_shared_from_this*      _esft,
        const _Y*                           _p
    ) D_NOEXCEPT
    {
        if (_esft && _esft->m_weak_this.expired())
        {
            _esft->m_weak_this._sp_internal_assign(
                const_cast<_Y*>(_p), _cb);
        }
    }

protected:
    D_CONSTEXPR enable_shared_from_this() D_NOEXCEPT
        : m_weak_this()
    {
    }

    enable_shared_from_this(const enable_shared_from_this&) D_NOEXCEPT
        : m_weak_this()
    {
        // Copy ctor: do NOT copy the weak_this. Each enable_shared_from_this
        // instance starts fresh; the new shared_ptr (if any) re-installs.
    }

    enable_shared_from_this& operator=(const enable_shared_from_this&) D_NOEXCEPT
    {
        // Same logic: do not propagate weak_this.
        return *this;
    }

    ~enable_shared_from_this()
    {
    }

public:
    shared_ptr<_T> shared_from_this()
    {
        return shared_ptr<_T>(m_weak_this);
    }

    shared_ptr<const _T> shared_from_this() const
    {
        return shared_ptr<const _T>(m_weak_this);
    }

    weak_ptr<_T> weak_from_this() D_NOEXCEPT
    {
        return m_weak_this;
    }

    weak_ptr<const _T> weak_from_this() const D_NOEXCEPT
    {
        return m_weak_this;
    }
};


}  // namespace restd

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // RESTD_MEMORY_ENABLE_SHARED_FROM_THIS_
