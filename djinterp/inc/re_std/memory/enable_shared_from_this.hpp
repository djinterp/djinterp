/*******************************************************************************
* djinterp [re_std]                                  enable_shared_from_this.hpp
*
* CRTP base for types that need to obtain a shared_ptr to themselves:
*
*     class Foo : public re_std::enable_shared_from_this<Foo>
*     {
*         re_std::shared_ptr<Foo> bar()
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
* takes an enable_shared_from_this<T>* among its arguments. When
* shared_ptr's ctor calls sp_esft_link(cb, ptr, ptr), ADL on ptr's
* type finds this friend ONLY if the pointee derives from
* enable_shared_from_this<U> for some U. Otherwise, only the variadic
* catch-all in re_std::internal:: matches, and the call is a no-op.
*
* This is the same idiom libstdc++ uses for std::enable_shared_from_this.
* It correctly handles cases where T derives from enable_shared_from_this<U>
* via several inheritance hops.
*
*
* path:      /inc/re_std/memory/enable_shared_from_this.hpp
* link(s):   TBA
* author(s): re_std contributors                             created: 2026.05.02
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_MEMORY_ENABLE_SHARED_FROM_THIS_HPP
#define RE_STD_MEMORY_ENABLE_SHARED_FROM_THIS_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_IS_CPP11_OR_HIGHER

    #include "re_std/memory/shared_ptr.hpp"
    #include "re_std/memory/weak_ptr.hpp"


namespace re_std
{

// =============================================================================
// enable_shared_from_this<T>
// =============================================================================

template<typename T>
class enable_shared_from_this
{
private:
    mutable weak_ptr<T> m_weak_this;

    // Inline friend, found by ADL when an argument's associated
    // classes include enable_shared_from_this<T> (i.e. the pointee
    // derives from it). Templated on Y so the deducer captures the
    // most-derived static type at the call site.
    template<typename Y>
    friend void sp_esft_link
    (
        internal::sp_control_block_base*    _cb,
        const enable_shared_from_this*      _esft,
        const Y*                           _p
    ) RE_STD_NOEXCEPT
    {
        if (_esft && _esft->m_weak_this.expired())
        {
            _esft->m_weak_this._sp_internal_assign(
                const_cast<Y*>(_p), _cb);
        }
    }

protected:
    RE_STD_CONSTEXPR enable_shared_from_this() RE_STD_NOEXCEPT
        : m_weak_this()
    {
    }

    enable_shared_from_this(const enable_shared_from_this&) RE_STD_NOEXCEPT
        : m_weak_this()
    {
        // Copy ctor: do NOT copy the weak_this. Each enable_shared_from_this
        // instance starts fresh; the new shared_ptr (if any) re-installs.
    }

    enable_shared_from_this& operator=(const enable_shared_from_this&) RE_STD_NOEXCEPT
    {
        // Same logic: do not propagate weak_this.
        return *this;
    }

    ~enable_shared_from_this()
    {
    }

public:
    shared_ptr<T> shared_from_this()
    {
        return shared_ptr<T>(m_weak_this);
    }

    shared_ptr<const T> shared_from_this() const
    {
        return shared_ptr<const T>(m_weak_this);
    }

    weak_ptr<T> weak_from_this() RE_STD_NOEXCEPT
    {
        return m_weak_this;
    }

    weak_ptr<const T> weak_from_this() const RE_STD_NOEXCEPT
    {
        return m_weak_this;
    }
};


}  // re_std
#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER

#endif  // RE_STD_MEMORY_ENABLE_SHARED_FROM_THIS_HPP
