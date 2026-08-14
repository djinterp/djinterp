/***********************************************************************
* restd                                                allocator_traits.hpp
*
* uniform allocator interface:
*   allocator_traits<_Alloc> normalises an allocator type so that
* container code can speak one vocabulary regardless of which optional
* members the allocator chose to define. Every member type and every
* static function in this trait has a fallback for when the underlying
* allocator omits the corresponding member.
*
* member-type fallbacks:
*   pointer                  _A::pointer            else value_type*
*   const_pointer            _A::const_pointer      else
*                              pointer_traits<pointer>::rebind<const value_type>
*   void_pointer             _A::void_pointer       else
*                              pointer_traits<pointer>::rebind<void>
*   const_void_pointer       _A::const_void_pointer else
*                              pointer_traits<pointer>::rebind<const void>
*   difference_type          _A::difference_type    else
*                              pointer_traits<pointer>::difference_type
*   size_type                _A::size_type          else
*                              make_unsigned<difference_type>
*   propagate_on_container_*  _A::p_o_c_*            else false_type
*   is_always_equal          _A::is_always_equal    else is_empty<_A>
*   rebind_alloc<_U>         _A::rebind<_U>::other  else head-substitution
*
* static-function fallbacks:
*   allocate(a, n)           a.allocate(n)
*   allocate(a, n, hint)     a.allocate(n, hint) if defined
*                              else a.allocate(n)
*   deallocate(a, p, n)      a.deallocate(p, n)
*   construct(a, p, args...) a.construct(p, args...) if defined
*                              else ::new ((void*)p) U(args...)
*                              (or restd::construct_at on C++20+)
*   destroy(a, p)            a.destroy(p) if defined
*                              else restd::destroy_at(p)
*   max_size(a)              a.max_size() if defined
*                              else (size_type)-1 / sizeof(value_type)
*   select_on_container_copy_construction(a)
*                            a.select_on_container_copy_construction()
*                            if defined, else returns a.
*
* C++11+ floor:
*   The detection idiom uses void_t + decltype(declval<_A>().X(...)).
*   None of those is available pre-C++11, so the entire header is
*   gated. Code that needs allocator_traits on C++98 must do allocator
*   member calls directly.
*
* not yet implemented:
*   allocate_at_least  (C++23). Trivial wrapper around allocate; will
*                      ship alongside the rest of the C++23 surface.
*
*
* path:      /inc/restd/memory/allocator_traits.hpp
* link(s):   TBA
* author(s): restd contributors                          date: 2026.05.01
***********************************************************************/

#ifndef RESTD_MEMORY_ALLOCATOR_TRAITS_
#define RESTD_MEMORY_ALLOCATOR_TRAITS_ 1

#include "djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

    #include <cstddef>  // size_t
    #include "restd/memory/pointer_traits.hpp"
    #include "restd/memory/destroy_at.hpp"
    #include "restd/type_traits/integral_constant.hpp"
    #include "restd/type_traits/void_t.hpp"
    #include "restd/type_traits/enable_if.hpp"
    #include "restd/type_traits/is_empty.hpp"
    #include "restd/type_traits/make_unsigned.hpp"
    #include "restd/utility/declval.hpp"
    #include "restd/utility/forward.hpp"

    #if D_ENV_LANG_IS_CPP20_OR_HIGHER
        #include "restd/memory/construct_at.hpp"
    #elif D_ENV_CPP98_HAS_NEW
        #include <new>  // placement new
    #endif


namespace restd
{

// =============================================================================
// internal: member-type detection
// =============================================================================

namespace internal
{

    // ---------- pointer ----------
    //   A::pointer if defined, else A::value_type*.

    template<typename _A, typename = void>
    struct alloc_pointer
    {
        typedef typename _A::value_type* type;
    };

    template<typename _A>
    struct alloc_pointer
    <
        _A,
        typename void_t<typename _A::pointer>::type
    >
    {
        typedef typename _A::pointer type;
    };

    // ---------- const_pointer ----------
    //   A::const_pointer if defined, else
    //   pointer_traits<pointer>::rebind<const value_type>.

    template<typename _A, typename _Ptr, typename = void>
    struct alloc_const_pointer
    {
        typedef typename pointer_traits<_Ptr>
            ::template rebind<const typename _A::value_type> type;
    };

    template<typename _A, typename _Ptr>
    struct alloc_const_pointer
    <
        _A,
        _Ptr,
        typename void_t<typename _A::const_pointer>::type
    >
    {
        typedef typename _A::const_pointer type;
    };

    // ---------- void_pointer ----------

    template<typename _A, typename _Ptr, typename = void>
    struct alloc_void_pointer
    {
        typedef typename pointer_traits<_Ptr>::template rebind<void> type;
    };

    template<typename _A, typename _Ptr>
    struct alloc_void_pointer
    <
        _A,
        _Ptr,
        typename void_t<typename _A::void_pointer>::type
    >
    {
        typedef typename _A::void_pointer type;
    };

    // ---------- const_void_pointer ----------

    template<typename _A, typename _Ptr, typename = void>
    struct alloc_const_void_pointer
    {
        typedef typename pointer_traits<_Ptr>::template rebind<const void> type;
    };

    template<typename _A, typename _Ptr>
    struct alloc_const_void_pointer
    <
        _A,
        _Ptr,
        typename void_t<typename _A::const_void_pointer>::type
    >
    {
        typedef typename _A::const_void_pointer type;
    };

    // ---------- difference_type ----------

    template<typename _A, typename _Ptr, typename = void>
    struct alloc_difference_type
    {
        typedef typename pointer_traits<_Ptr>::difference_type type;
    };

    template<typename _A, typename _Ptr>
    struct alloc_difference_type
    <
        _A,
        _Ptr,
        typename void_t<typename _A::difference_type>::type
    >
    {
        typedef typename _A::difference_type type;
    };

    // ---------- size_type ----------
    //   A::size_type if defined, else make_unsigned<difference_type>.

    template<typename _A, typename _Diff, typename = void>
    struct alloc_size_type
    {
        typedef typename make_unsigned<_Diff>::type type;
    };

    template<typename _A, typename _Diff>
    struct alloc_size_type
    <
        _A,
        _Diff,
        typename void_t<typename _A::size_type>::type
    >
    {
        typedef typename _A::size_type type;
    };

    // ---------- propagate_on_container_copy_assignment ----------

    template<typename _A, typename = void>
    struct alloc_pocca
    {
        typedef false_type type;
    };

    template<typename _A>
    struct alloc_pocca
    <
        _A,
        typename void_t
        <
            typename _A::propagate_on_container_copy_assignment
        >::type
    >
    {
        typedef typename _A::propagate_on_container_copy_assignment type;
    };

    // ---------- propagate_on_container_move_assignment ----------

    template<typename _A, typename = void>
    struct alloc_pocma
    {
        typedef false_type type;
    };

    template<typename _A>
    struct alloc_pocma
    <
        _A,
        typename void_t
        <
            typename _A::propagate_on_container_move_assignment
        >::type
    >
    {
        typedef typename _A::propagate_on_container_move_assignment type;
    };

    // ---------- propagate_on_container_swap ----------

    template<typename _A, typename = void>
    struct alloc_pocs
    {
        typedef false_type type;
    };

    template<typename _A>
    struct alloc_pocs
    <
        _A,
        typename void_t<typename _A::propagate_on_container_swap>::type
    >
    {
        typedef typename _A::propagate_on_container_swap type;
    };

    // ---------- is_always_equal ----------
    //   A::is_always_equal if defined, else is_empty<A>. The is_empty
    //   fallback was added by C++17 and matches the standard's
    //   default rule.

    template<typename _A, typename = void>
    struct alloc_is_always_equal
    {
        typedef typename is_empty<_A>::type type;
    };

    template<typename _A>
    struct alloc_is_always_equal
    <
        _A,
        typename void_t<typename _A::is_always_equal>::type
    >
    {
        typedef typename _A::is_always_equal type;
    };

    // ---------- rebind_alloc ----------
    //   _A::rebind<_U>::other if defined, else replace _A's first
    //   template argument with _U.

    template<typename _A, typename _U, typename = void>
    struct alloc_rebind_substituted
    {
        // Primary: ill-formed if _A is not a class template specialisation.
        // The pattern below catches the common case.
    };

    template
    <
        template<typename, typename...> class _Tmpl,
        typename _Head,
        typename... _Tail,
        typename _U
    >
    struct alloc_rebind_substituted<_Tmpl<_Head, _Tail...>, _U>
    {
        typedef _Tmpl<_U, _Tail...> type;
    };

    template<typename _A, typename _U, typename = void>
    struct alloc_rebind
        : alloc_rebind_substituted<_A, _U>
    {
    };

    template<typename _A, typename _U>
    struct alloc_rebind
    <
        _A,
        _U,
        typename void_t
        <
            typename _A::template rebind<_U>::other
        >::type
    >
    {
        typedef typename _A::template rebind<_U>::other type;
    };

}  // namespace internal


// =============================================================================
// internal: static-function detection
// =============================================================================

namespace internal
{

    // ---------- has a.allocate(n, hint) ----------

    template
    <
        typename _A,
        typename _Size,
        typename _CVPtr,
        typename = void
    >
    struct has_allocate_hint
        : false_type
    {
    };

    template<typename _A, typename _Size, typename _CVPtr>
    struct has_allocate_hint
    <
        _A, _Size, _CVPtr,
        typename void_t
        <
            decltype
            (
                restd::declval<_A&>().allocate
                (
                    restd::declval<_Size>(),
                    restd::declval<_CVPtr>()
                )
            )
        >::type
    >
        : true_type
    {
    };

    // ---------- has a.construct(p, args...) ----------
    //
    //   We use the "auto test(int) -> decltype(...)" trick rather than
    //   void_t because parameter packs sit awkwardly inside the
    //   default-template-arg substitution.

    template<typename _A, typename _P, typename... _Args>
    struct has_member_construct
    {
    private:
        template<typename _A1, typename _P1, typename... _A1rgs>
        static auto try_call(int)
            -> decltype
               (
                   (void)restd::declval<_A1&>().construct
                   (
                       restd::declval<_P1>(),
                       restd::declval<_A1rgs>()...
                   ),
                   true_type()
               );

        template<typename, typename, typename...>
        static false_type try_call(...);

    public:
        typedef decltype(try_call<_A, _P, _Args...>(0)) type;
        static const bool value = type::value;
    };

    // ---------- has a.destroy(p) ----------

    template<typename _A, typename _P, typename = void>
    struct has_member_destroy
        : false_type
    {
    };

    template<typename _A, typename _P>
    struct has_member_destroy
    <
        _A, _P,
        typename void_t
        <
            decltype(restd::declval<_A&>().destroy(restd::declval<_P>()))
        >::type
    >
        : true_type
    {
    };

    // ---------- has a.max_size() ----------

    template<typename _A, typename = void>
    struct has_member_max_size
        : false_type
    {
    };

    template<typename _A>
    struct has_member_max_size
    <
        _A,
        typename void_t
        <
            decltype(restd::declval<const _A&>().max_size())
        >::type
    >
        : true_type
    {
    };

    // ---------- has a.select_on_container_copy_construction() ----------

    template<typename _A, typename = void>
    struct has_member_socc
        : false_type
    {
    };

    template<typename _A>
    struct has_member_socc
    <
        _A,
        typename void_t
        <
            decltype
            (
                restd::declval<const _A&>()
                    .select_on_container_copy_construction()
            )
        >::type
    >
        : true_type
    {
    };

}  // namespace internal


// =============================================================================
// internal: dispatchers for max_size and select_on_container_copy_construction
//
// These live outside the class because they need full template-argument
// freedom (size_type and value_type are not in scope at namespace level
// otherwise) and they should not be part of the public allocator_traits
// surface.
// =============================================================================

namespace internal
{

    // ---------- max_size dispatch ----------

    template<typename _SizeType, typename _ValueType, typename _A>
    #if D_ENV_LANG_IS_CPP20_OR_HIGHER
        constexpr
    #endif
    typename enable_if
    <
        has_member_max_size<_A>::value,
        _SizeType
    >::type
    alloc_max_size_dispatch(const _A& _a, int)
    {
        return static_cast<_SizeType>(_a.max_size());
    }

    template<typename _SizeType, typename _ValueType, typename _A>
    #if D_ENV_LANG_IS_CPP20_OR_HIGHER
        constexpr
    #endif
    typename enable_if
    <
        !has_member_max_size<_A>::value,
        _SizeType
    >::type
    alloc_max_size_dispatch(const _A&, ...)
    {
        // Fallback formula matches the C++17 wording. size_type is
        // required to be unsigned, so (size_type)-1 is its max.
        return static_cast<_SizeType>(-1) / sizeof(_ValueType);
    }

    // ---------- select_on_container_copy_construction dispatch ----------

    template<typename _A>
    #if D_ENV_LANG_IS_CPP20_OR_HIGHER
        constexpr
    #endif
    typename enable_if
    <
        has_member_socc<_A>::value,
        _A
    >::type
    alloc_socc_dispatch(const _A& _a, int)
    {
        return _a.select_on_container_copy_construction();
    }

    template<typename _A>
    #if D_ENV_LANG_IS_CPP20_OR_HIGHER
        constexpr
    #endif
    typename enable_if
    <
        !has_member_socc<_A>::value,
        _A
    >::type
    alloc_socc_dispatch(const _A& _a, ...)
    {
        return _a;
    }

}  // namespace internal

// NOTE: this block must precede allocator_traits: the member functions below
// name internal::alloc_*_dispatch through a qualified-id whose nested-name-
// specifier does not depend on a template parameter, so it is looked up at
// template DEFINITION time, not at instantiation.

// =============================================================================
// allocator_traits
// =============================================================================

// allocator_traits<_Alloc>
//   class: uniform allocator interface. All members and all static
//          functions have detection-with-fallback semantics.
template<typename _Alloc>
struct allocator_traits
{
    // -------------------------------------------------------------------------
    // member types
    // -------------------------------------------------------------------------

    typedef _Alloc                              allocator_type;
    typedef typename _Alloc::value_type         value_type;

    typedef typename internal::alloc_pointer<_Alloc>::type
        pointer;

    typedef typename internal::alloc_const_pointer<_Alloc, pointer>::type
        const_pointer;

    typedef typename internal::alloc_void_pointer<_Alloc, pointer>::type
        void_pointer;

    typedef typename internal::alloc_const_void_pointer<_Alloc, pointer>::type
        const_void_pointer;

    typedef typename internal::alloc_difference_type<_Alloc, pointer>::type
        difference_type;

    typedef typename internal::alloc_size_type<_Alloc, difference_type>::type
        size_type;

    typedef typename internal::alloc_pocca<_Alloc>::type
        propagate_on_container_copy_assignment;

    typedef typename internal::alloc_pocma<_Alloc>::type
        propagate_on_container_move_assignment;

    typedef typename internal::alloc_pocs<_Alloc>::type
        propagate_on_container_swap;

    typedef typename internal::alloc_is_always_equal<_Alloc>::type
        is_always_equal;

    // rebind_alloc / rebind_traits  (require alias templates)
    #if D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES
        template<typename _U>
        using rebind_alloc =
            typename internal::alloc_rebind<_Alloc, _U>::type;

        template<typename _U>
        using rebind_traits = allocator_traits<rebind_alloc<_U> >;
    #endif

    // -------------------------------------------------------------------------
    // allocate
    // -------------------------------------------------------------------------

    static
    #if D_ENV_LANG_IS_CPP20_OR_HIGHER
        constexpr
    #endif
    pointer allocate(_Alloc& _a, size_type _n)
    {
        return _a.allocate(_n);
    }

    // allocate(a, n, hint) — try a.allocate(n, hint), else a.allocate(n).

    template<typename _A>
    static
    #if D_ENV_LANG_IS_CPP20_OR_HIGHER
        constexpr
    #endif
    typename enable_if
    <
        internal::has_allocate_hint<_A, size_type, const_void_pointer>::value,
        pointer
    >::type
    allocate(_A& _a, size_type _n, const_void_pointer _hint)
    {
        return _a.allocate(_n, _hint);
    }

    template<typename _A>
    static
    #if D_ENV_LANG_IS_CPP20_OR_HIGHER
        constexpr
    #endif
    typename enable_if
    <
        !internal::has_allocate_hint<_A, size_type, const_void_pointer>::value,
        pointer
    >::type
    allocate(_A& _a, size_type _n, const_void_pointer)
    {
        return _a.allocate(_n);
    }

    // -------------------------------------------------------------------------
    // deallocate
    // -------------------------------------------------------------------------

    static
    #if D_ENV_LANG_IS_CPP20_OR_HIGHER
        constexpr
    #endif
    void deallocate(_Alloc& _a, pointer _p, size_type _n)
    {
        _a.deallocate(_p, _n);
    }

    // -------------------------------------------------------------------------
    // construct
    // -------------------------------------------------------------------------

    // Two overloads, dispatched on whether _Alloc has a member construct.
    //
    // The fallback is `::new((void*)p) U(args...)` on C++11..C++17 and
    // `restd::construct_at(p, args...)` on C++20+. The C++20 standard
    // mandated the construct_at form so that constexpr-allocator code
    // can trace through allocator_traits without hitting a non-
    // constexpr placement-new expression.

    template<typename _U, typename... _Args>
    static
    #if D_ENV_LANG_IS_CPP20_OR_HIGHER
        constexpr
    #endif
    typename enable_if
    <
        internal::has_member_construct<_Alloc, _U*, _Args...>::value,
        void
    >::type
    construct(_Alloc& _a, _U* _p, _Args&&... _args)
    {
        _a.construct(_p, restd::forward<_Args>(_args)...);
    }

    template<typename _U, typename... _Args>
    static
    #if D_ENV_LANG_IS_CPP20_OR_HIGHER
        constexpr
    #endif
    typename enable_if
    <
        !internal::has_member_construct<_Alloc, _U*, _Args...>::value,
        void
    >::type
    construct(_Alloc&, _U* _p, _Args&&... _args)
    {
        #if D_ENV_LANG_IS_CPP20_OR_HIGHER
            restd::construct_at(_p, restd::forward<_Args>(_args)...);
        #else
            ::new (static_cast<void*>(_p))
                _U(restd::forward<_Args>(_args)...);
        #endif
    }

    // -------------------------------------------------------------------------
    // destroy
    // -------------------------------------------------------------------------

    template<typename _U>
    static
    #if D_ENV_LANG_IS_CPP20_OR_HIGHER
        constexpr
    #endif
    typename enable_if
    <
        internal::has_member_destroy<_Alloc, _U*>::value,
        void
    >::type
    destroy(_Alloc& _a, _U* _p)
    {
        _a.destroy(_p);
    }

    template<typename _U>
    static
    #if D_ENV_LANG_IS_CPP20_OR_HIGHER
        constexpr
    #endif
    typename enable_if
    <
        !internal::has_member_destroy<_Alloc, _U*>::value,
        void
    >::type
    destroy(_Alloc&, _U* _p)
    {
        restd::destroy_at(_p);
    }

    // -------------------------------------------------------------------------
    // max_size  /  select_on_container_copy_construction
    //
    // Both have detect-or-fallback semantics. The dispatchers live in
    // internal:: (below) so they don't leak through the public surface
    // of allocator_traits.
    // -------------------------------------------------------------------------

    static
    #if D_ENV_LANG_IS_CPP20_OR_HIGHER
        constexpr
    #endif
    size_type max_size(const _Alloc& _a) D_NOEXCEPT
    {
        return internal::alloc_max_size_dispatch<size_type, value_type>
                   (_a, 0);
    }

    static
    #if D_ENV_LANG_IS_CPP20_OR_HIGHER
        constexpr
    #endif
    _Alloc select_on_container_copy_construction(const _Alloc& _a)
    {
        return internal::alloc_socc_dispatch(_a, 0);
    }
};




}  // namespace restd

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // RESTD_MEMORY_ALLOCATOR_TRAITS_
