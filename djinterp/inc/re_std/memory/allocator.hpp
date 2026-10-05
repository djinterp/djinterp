/*******************************************************************************
* djinterp [re_std]                                                allocator.hpp
*
* the canonical default allocator:
*   re_std::allocator<T> allocates raw memory via ::operator new and
* releases it via ::operator delete. It is stateless: any two instances
* compare equal, and rebinding to a different element type produces an
* allocator that is also equal to all others.
*
* surface (per C++ standard, with re_std's "retain even when deprecated"
* policy):
*   value_type, size_type, difference_type
*   pointer, const_pointer, reference, const_reference
*       (deprecated in std C++17, removed in std C++20; re_std retains)
*   propagate_on_container_move_assignment  (C++11+)
*   is_always_equal                         (C++17+, re_std back-ports to C++11+)
*   rebind<U>::other                       (deprecated in std C++17,
*                                            removed in std C++20; re_std
*                                            retains for back-compat)
*   ctors:
*     allocator() noexcept
*     allocator(const allocator&) noexcept
*     allocator(const allocator<U>&) noexcept   (converting)
*   members:
*     allocate(_n)                          throws bad_alloc on failure
*     deallocate(_p, _n) noexcept
*     address(_r) / address(_cr)            (retained from C++98)
*     max_size() const noexcept              (retained from C++98)
*     construct(_p, _args...)                (variadic on C++11+,
*                                             single-arg on C++98/03)
*     destroy(_p)                            calls _p->~U()
*   non-members:
*     operator==                              always true (stateless)
*     operator!=                              always false
*
* tier behaviour:
*   C++98/03      Single-arg construct(pointer, const_reference).
*                 No noexcept (uses RE_STD_NOEXCEPT shim).
*                 No is_always_equal / propagate_on_container_*.
*   C++11+        Variadic construct via perfect forwarding.
*                 propagate_on_container_move_assignment = true_type.
*                 is_always_equal = true_type.
*   C++20+        allocate/deallocate are constexpr (matches std).
*
* dependencies:
*   <new>                       gated on RE_STD_HAS_HEADER_NEW. allocate
*                               degrades to returning 0 on failure when
*                               <new> is unavailable, since it cannot
*                               throw bad_alloc.
*   re_std::addressof            for address().
*
*
* path:      /inc/re_std/memory/allocator.hpp
* link(s):   TBA
* author(s): re_std contributors                             created: 2026.05.01
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_MEMORY_ALLOCATOR_HPP
#define RE_STD_MEMORY_ALLOCATOR_HPP 1

// std
#include <cstddef>  // size_t, ptrdiff_t
// re_std
#include "../config.hpp"  // RE_STD_* configuration
#include "re_std/memory/addressof.hpp"

#if RE_STD_HAS_HEADER_NEW
    // std
    #include <new>  // ::operator new, ::operator delete, bad_alloc
#endif

#if ( (RE_STD_HAS_HEADER_NEW) &&                                                \
      (!RE_STD_HAS_EXCEPTIONS) )
    // std
    #include <cstdlib>  // std::abort, allocate's overflow without exceptions
#endif

#if RE_STD_LANG_IS_CPP11_OR_HIGHER
    #include "re_std/type_traits/integral_constant.hpp"
    #include "re_std/utility/forward.hpp"
#endif


namespace re_std
{

// =============================================================================
// allocator  -  primary template
// =============================================================================

// allocator<T>
//   class: stateless default allocator.
template<typename T>
class allocator
{
public:
    // -------------------------------------------------------------------------
    // member types
    // -------------------------------------------------------------------------
    typedef T              value_type;
    typedef std::size_t     size_type;
    typedef std::ptrdiff_t  difference_type;

    // Deprecated in std C++17, removed in std C++20. Re_std retains for
    // portability with code that names them directly.
    typedef T*             pointer;
    typedef const T*       const_pointer;
    typedef T&             reference;
    typedef const T&       const_reference;

    #if RE_STD_LANG_IS_CPP11_OR_HIGHER
        typedef true_type   propagate_on_container_move_assignment;
        typedef true_type   is_always_equal;
    #endif

    // C++98-style rebind nested struct. Retained on every tier so that
    // C++98 code (and re_std::allocator_traits's substitution fallback)
    // can name allocator<U>::rebind<V>::other portably.
    template<typename U>
    struct rebind
    {
        typedef allocator<U> other;
    };

    // -------------------------------------------------------------------------
    // construction / destruction
    // -------------------------------------------------------------------------

    // Default ctor.
    allocator() RE_STD_NOEXCEPT
    {
    }

    // Copy ctor.
    allocator(const allocator&) RE_STD_NOEXCEPT
    {
    }

    // Converting ctor: build allocator<T> from allocator<U>. Stateless
    // so the body is empty.
    template<typename U>
    allocator(const allocator<U>&) RE_STD_NOEXCEPT
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

    pointer       address(reference _r)       const RE_STD_NOEXCEPT
    {
        return re_std::addressof(_r);
    }

    const_pointer address(const_reference _r) const RE_STD_NOEXCEPT
    {
        return re_std::addressof(_r);
    }

    // -------------------------------------------------------------------------
    // allocate / deallocate
    // -------------------------------------------------------------------------

    // allocate
    //   function: obtain raw uninitialised storage for _n objects of T.
    //   Throws bad_alloc on failure (or returns 0 when <new> is
    //   unavailable, since bad_alloc is then unavailable too).
    //
    //   Constexpr from C++20+: matches std and supports constexpr-new
    //   contexts.
    #if RE_STD_LANG_IS_CPP20_OR_HIGHER
        constexpr pointer allocate(size_type _n)
    #else
        pointer allocate(size_type _n)
    #endif
    {
        // Overflow check: if _n * sizeof(T) would wrap size_type, ask
        // for too-few bytes and silently corrupt. Catch it here.
        if (_n > this->max_size())
        {
            #if ( (RE_STD_HAS_HEADER_NEW) &&                                    \
                  (RE_STD_HAS_EXCEPTIONS) )
                throw std::bad_alloc();
            #elif RE_STD_HAS_HEADER_NEW
                // exceptions off: std's allocators abort here, and so must
                // this one -- callers do not check for a null result
                std::abort();
            #else
                // No <new> means no bad_alloc. Return 0 and trust the
                // caller to check; this is the only signalling channel
                // available on this tier.
                return 0;
            #endif
        }

        #if RE_STD_HAS_HEADER_NEW
            return static_cast<pointer>(::operator new(_n * sizeof(T)));
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
    #if RE_STD_LANG_IS_CPP20_OR_HIGHER
        constexpr void deallocate(pointer _p, size_type _n) RE_STD_NOEXCEPT
    #else
        void deallocate(pointer _p, size_type _n) RE_STD_NOEXCEPT
    #endif
    {
        (void)_n;
        #if RE_STD_HAS_HEADER_NEW
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
    size_type max_size() const RE_STD_NOEXCEPT
    {
        return static_cast<size_type>(-1) / sizeof(T);
    }

    // -------------------------------------------------------------------------
    // construct / destroy
    //   (retained from C++98; deprecated in std C++17, removed in C++20)
    // -------------------------------------------------------------------------

    #if RE_STD_LANG_HAS_VARIADIC_TEMPLATES

        // C++11+ form: variadic, with perfect forwarding.
        template<typename U, typename... Args>
        void construct(U* _p, Args&&... _args)
        {
            ::new (static_cast<void*>(_p))
                U(re_std::forward<Args>(_args)...);
        }

        template<typename U>
        void destroy(U* _p)
        {
            _p->~U();
        }

    #else

        // C++98/03 form: single-arg construct, scalar destroy. The
        // signature matches std::allocator<T>::construct from C++98.
        void construct(pointer _p, const_reference _val)
        {
            ::new (static_cast<void*>(_p)) T(_val);
        }

        void destroy(pointer _p)
        {
            _p->~T();
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
//   Deprecated in std C++17, removed in std C++20. Re_std retains for
//   back-compat with code that names it directly.
template<>
class allocator<void>
{
public:
    typedef void        value_type;
    typedef void*       pointer;
    typedef const void* const_pointer;

    template<typename U>
    struct rebind
    {
        typedef allocator<U> other;
    };

    #if RE_STD_LANG_IS_CPP11_OR_HIGHER
        // The trait typedefs are well-defined for void too, and tuple /
        // optional generic-allocator code may name them.
        typedef true_type propagate_on_container_move_assignment;
        typedef true_type is_always_equal;
    #endif

    allocator() RE_STD_NOEXCEPT
    {
    }

    allocator(const allocator&) RE_STD_NOEXCEPT
    {
    }

    template<typename U>
    allocator(const allocator<U>&) RE_STD_NOEXCEPT
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

template<typename T1, typename T2>
RE_STD_CONSTEXPR_INLINE bool operator==
(
    const allocator<T1>&,
    const allocator<T2>&
) RE_STD_NOEXCEPT
{
    return true;
}

template<typename T1, typename T2>
RE_STD_CONSTEXPR_INLINE bool operator!=
(
    const allocator<T1>&,
    const allocator<T2>&
) RE_STD_NOEXCEPT
{
    return false;
}


}  // re_std
#endif  // RE_STD_MEMORY_ALLOCATOR_HPP
