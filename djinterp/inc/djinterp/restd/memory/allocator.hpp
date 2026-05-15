/***********************************************************************
* restd                                                       allocator.hpp
*
* the canonical default allocator:
*   restd::allocator<_T> allocates raw memory via ::operator new and
* releases it via ::operator delete. It is stateless: any two instances
* compare equal, and rebinding to a different element type produces an
* allocator that is also equal to all others.
*
* surface (per C++ standard, with restd's "retain even when deprecated"
* policy):
*   value_type, size_type, difference_type
*   pointer, const_pointer, reference, const_reference
*       (deprecated in std C++17, removed in std C++20; restd retains)
*   propagate_on_container_move_assignment  (C++11+)
*   is_always_equal                         (C++17+, restd back-ports to C++11+)
*   rebind<_U>::other                       (deprecated in std C++17,
*                                            removed in std C++20; restd
*                                            retains for back-compat)
*   ctors:
*     allocator() noexcept
*     allocator(const allocator&) noexcept
*     allocator(const allocator<_U>&) noexcept   (converting)
*   members:
*     allocate(_n)                          throws bad_alloc on failure
*     deallocate(_p, _n) noexcept
*     address(_r) / address(_cr)            (retained from C++98)
*     max_size() const noexcept              (retained from C++98)
*     construct(_p, _args...)                (variadic on C++11+,
*                                             single-arg on C++98/03)
*     destroy(_p)                            calls _p->~_U()
*   non-members:
*     operator==                              always true (stateless)
*     operator!=                              always false
*
* tier behaviour:
*   C++98/03      Single-arg construct(pointer, const_reference).
*                 No noexcept (uses D_NOEXCEPT shim).
*                 No is_always_equal / propagate_on_container_*.
*   C++11+        Variadic construct via perfect forwarding.
*                 propagate_on_container_move_assignment = true_type.
*                 is_always_equal = true_type.
*   C++20+        allocate/deallocate are constexpr (matches std).
*
* dependencies:
*   <new>                       gated on D_ENV_CPP98_HAS_NEW. allocate
*                               degrades to returning 0 on failure when
*                               <new> is unavailable, since it cannot
*                               throw bad_alloc.
*   restd::addressof            for address().
*
*
* path:      /inc/restd/memory/allocator.hpp
* link(s):   TBA
* author(s): restd contributors                          date: 2026.05.01
***********************************************************************/

#ifndef RESTD_MEMORY_ALLOCATOR_
#define RESTD_MEMORY_ALLOCATOR_ 1

#include "djinterp.hpp"
#include <cstddef>  // size_t, ptrdiff_t

#include "restd/memory/addressof.hpp"

#if D_ENV_CPP98_HAS_NEW
    #include <new>  // ::operator new, ::operator delete, bad_alloc
#endif

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    #include "restd/type_traits/integral_constant.hpp"
    #include "restd/utility/forward.hpp"
#endif


namespace restd
{

// =============================================================================
// allocator  -  primary template
// =============================================================================

// allocator<_T>
//   class: stateless default allocator.
template<typename _T>
class allocator
{
public:
    // -------------------------------------------------------------------------
    // member types
    // -------------------------------------------------------------------------
    typedef _T              value_type;
    typedef std::size_t     size_type;
    typedef std::ptrdiff_t  difference_type;

    // Deprecated in std C++17, removed in std C++20. Restd retains for
    // portability with code that names them directly.
    typedef _T*             pointer;
    typedef const _T*       const_pointer;
    typedef _T&             reference;
    typedef const _T&       const_reference;

    #if D_ENV_LANG_IS_CPP11_OR_HIGHER
        typedef true_type   propagate_on_container_move_assignment;
        typedef true_type   is_always_equal;
    #endif

    // C++98-style rebind nested struct. Retained on every tier so that
    // C++98 code (and restd::allocator_traits's substitution fallback)
    // can name allocator<U>::rebind<V>::other portably.
    template<typename _U>
    struct rebind
    {
        typedef allocator<_U> other;
    };

    // -------------------------------------------------------------------------
    // construction / destruction
    // -------------------------------------------------------------------------

    // Default ctor.
    allocator() D_NOEXCEPT
    {
    }

    // Copy ctor.
    allocator(const allocator&) D_NOEXCEPT
    {
    }

    // Converting ctor: build allocator<_T> from allocator<_U>. Stateless
    // so the body is empty.
    template<typename _U>
    allocator(const allocator<_U>&) D_NOEXCEPT
    {
    }

    // Dtor.
    ~allocator()
    {
    }

    // -------------------------------------------------------------------------
    // address  (retained from C++98; deprecated in std C++17,
    //           removed in std C++20)
    // -------------------------------------------------------------------------

    pointer       address(reference _r)       const D_NOEXCEPT
    {
        return restd::addressof(_r);
    }

    const_pointer address(const_reference _r) const D_NOEXCEPT
    {
        return restd::addressof(_r);
    }

    // -------------------------------------------------------------------------
    // allocate / deallocate
    // -------------------------------------------------------------------------

    // allocate
    //   function: obtain raw uninitialised storage for _n objects of _T.
    //   Throws bad_alloc on failure (or returns 0 when <new> is
    //   unavailable, since bad_alloc is then unavailable too).
    //
    //   Constexpr from C++20+: matches std and supports constexpr-new
    //   contexts.
    #if D_ENV_LANG_IS_CPP20_OR_HIGHER
        constexpr pointer allocate(size_type _n)
    #else
        pointer allocate(size_type _n)
    #endif
    {
        // Overflow check: if _n * sizeof(_T) would wrap size_type, ask
        // for too-few bytes and silently corrupt. Catch it here.
        if (_n > this->max_size())
        {
            #if D_ENV_CPP98_HAS_NEW
                throw std::bad_alloc();
            #else
                // No <new> means no bad_alloc. Return 0 and trust the
                // caller to check; this is the only signalling channel
                // available on this tier.
                return 0;
            #endif
        }

        #if D_ENV_CPP98_HAS_NEW
            return static_cast<pointer>(::operator new(_n * sizeof(_T)));
        #else
            // Without <new>, ::operator new is still callable on most
            // hosted impls (it's a builtin), but we can't be sure. Fall
            // back to returning 0; no portable allocation primitive.
            (void)_n;
            return 0;
        #endif
    }

    // deallocate
    //   function: release storage previously obtained from allocate().
    //   _n should match the original allocate(_n); ignored on this
    //   implementation, present for sized-deallocation interop.
    #if D_ENV_LANG_IS_CPP20_OR_HIGHER
        constexpr void deallocate(pointer _p, size_type _n) D_NOEXCEPT
    #else
        void deallocate(pointer _p, size_type _n) D_NOEXCEPT
    #endif
    {
        (void)_n;
        #if D_ENV_CPP98_HAS_NEW
            ::operator delete(_p);
        #else
            (void)_p;
        #endif
    }

    // -------------------------------------------------------------------------
    // max_size
    //   (retained from C++98; deprecated in std C++17, removed in C++20)
    // -------------------------------------------------------------------------

    // max_size
    //   function: largest _n for which allocate(_n) is well-formed.
    //             size_type is required by the standard to be unsigned,
    //             so static_cast<size_type>(-1) is its max value.
    size_type max_size() const D_NOEXCEPT
    {
        return static_cast<size_type>(-1) / sizeof(_T);
    }

    // -------------------------------------------------------------------------
    // construct / destroy
    //   (retained from C++98; deprecated in std C++17, removed in C++20)
    // -------------------------------------------------------------------------

    #if D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES

        // C++11+ form: variadic, with perfect forwarding.
        template<typename _U, typename... _Args>
        void construct(_U* _p, _Args&&... _args)
        {
            ::new (static_cast<void*>(_p))
                _U(restd::forward<_Args>(_args)...);
        }

        template<typename _U>
        void destroy(_U* _p)
        {
            _p->~_U();
        }

    #else

        // C++98/03 form: single-arg construct, scalar destroy. The
        // signature matches std::allocator<T>::construct from C++98.
        void construct(pointer _p, const_reference _val)
        {
            ::new (static_cast<void*>(_p)) _T(_val);
        }

        void destroy(pointer _p)
        {
            _p->~_T();
        }

    #endif
};


// =============================================================================
// allocator<void>  -  void specialisation
// =============================================================================

// allocator<void>
//   class: void specialisation. Cannot allocate or deallocate, but
//   provides the rebind machinery so that code generic in the element
//   type can name allocator<void>::rebind<T>::other.
//
//   Deprecated in std C++17, removed in std C++20. Restd retains for
//   back-compat with code that names it directly.
template<>
class allocator<void>
{
public:
    typedef void        value_type;
    typedef void*       pointer;
    typedef const void* const_pointer;

    template<typename _U>
    struct rebind
    {
        typedef allocator<_U> other;
    };

    #if D_ENV_LANG_IS_CPP11_OR_HIGHER
        // The trait typedefs are well-defined for void too, and tuple /
        // optional generic-allocator code may name them.
        typedef true_type propagate_on_container_move_assignment;
        typedef true_type is_always_equal;
    #endif

    allocator() D_NOEXCEPT
    {
    }

    allocator(const allocator&) D_NOEXCEPT
    {
    }

    template<typename _U>
    allocator(const allocator<_U>&) D_NOEXCEPT
    {
    }
};


// =============================================================================
// operator==  /  operator!=
// =============================================================================

// All instances of allocator<T> compare equal. Cross-element-type
// comparison is allowed and also returns true (per the standard,
// stateless allocators that share the same value-type pattern compare
// equal regardless of T).

template<typename _T1, typename _T2>
D_CONSTEXPR_INLINE bool operator==
(
    const allocator<_T1>&,
    const allocator<_T2>&
) D_NOEXCEPT
{
    return true;
}

template<typename _T1, typename _T2>
D_CONSTEXPR_INLINE bool operator!=
(
    const allocator<_T1>&,
    const allocator<_T2>&
) D_NOEXCEPT
{
    return false;
}


}  // namespace restd

#endif  // RESTD_MEMORY_ALLOCATOR_
