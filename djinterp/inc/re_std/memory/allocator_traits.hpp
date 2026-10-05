/*******************************************************************************
* djinterp [re_std]                                         allocator_traits.hpp
*
* uniform allocator interface:
*   allocator_traits<Alloc> normalises an allocator type so that
* container code can speak one vocabulary regardless of which optional
* members the allocator chose to define. Every member type and every
* static function in this trait has a fallback for when the underlying
* allocator omits the corresponding member.
*
* member-type fallbacks:
*   pointer                  A::pointer            else value_type*
*   const_pointer            A::const_pointer      else
*                              pointer_traits<pointer>::rebind<const value_type>
*   void_pointer             A::void_pointer       else
*                              pointer_traits<pointer>::rebind<void>
*   const_void_pointer       A::const_void_pointer else
*                              pointer_traits<pointer>::rebind<const void>
*   difference_type          A::difference_type    else
*                              pointer_traits<pointer>::difference_type
*   size_type                A::size_type          else
*                              make_unsigned<difference_type>
*   propagate_on_container_*  A::p_o_c_*            else false_type
*   is_always_equal          A::is_always_equal    else is_empty<A>
*   rebind_alloc<U>         A::rebind<U>::other  else head-substitution
*
* static-function fallbacks:
*   allocate(a, n)           a.allocate(n)
*   allocate(a, n, hint)     a.allocate(n, hint) if defined
*                              else a.allocate(n)
*   deallocate(a, p, n)      a.deallocate(p, n)
*   construct(a, p, args...) a.construct(p, args...) if defined
*                              else ::new ((void*)p) U(args...)
*                              (or re_std::construct_at on C++20+)
*   destroy(a, p)            a.destroy(p) if defined
*                              else re_std::destroy_at(p)
*   max_size(a)              a.max_size() if defined
*                              else (size_type)-1 / sizeof(value_type)
*   select_on_container_copy_construction(a)
*                            a.select_on_container_copy_construction()
*                            if defined, else returns a.
*
* C++11+ floor:
*   The detection idiom uses void_t + decltype(declval<A>().X(...)).
*   None of those is available pre-C++11, so the entire header is
*   gated. Code that needs allocator_traits on C++98 must do allocator
*   member calls directly.
*
* not yet implemented:
*   allocate_at_least  (C++23). Trivial wrapper around allocate; will
*                      ship alongside the rest of the C++23 surface.
*
*
* path:      /inc/re_std/memory/allocator_traits.hpp
* link(s):   TBA
* author(s): re_std contributors                             created: 2026.05.01
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_MEMORY_ALLOCATOR_TRAITS_HPP
#define RE_STD_MEMORY_ALLOCATOR_TRAITS_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_IS_CPP11_OR_HIGHER

    #include <cstddef>  // size_t
    #include "re_std/memory/pointer_traits.hpp"
    #include "re_std/memory/destroy_at.hpp"
    #include "re_std/type_traits/integral_constant.hpp"
    #include "re_std/type_traits/void_t.hpp"
    #include "re_std/type_traits/enable_if.hpp"
    #include "re_std/type_traits/is_empty.hpp"
    #include "re_std/type_traits/make_unsigned.hpp"
    #include "re_std/utility/declval.hpp"
    #include "re_std/utility/forward.hpp"

    #if RE_STD_LANG_IS_CPP20_OR_HIGHER
        #include "re_std/memory/construct_at.hpp"
    #elif RE_STD_HAS_HEADER_NEW
        // std
        #include <new>  // placement new
    #endif


namespace re_std
{

// =============================================================================
// internal: member-type detection
// =============================================================================

namespace internal
{

    // ---------- pointer ----------
    //   A::pointer if defined, else A::value_type*.

    template<typename A, typename = void>
    struct alloc_pointer
    {
        typedef typename A::value_type* type;
    };

    template<typename A>
    struct alloc_pointer
    <
        A,
        void_t<typename A::pointer>
    >
    {
        typedef typename A::pointer type;
    };

    // ---------- const_pointer ----------
    //   A::const_pointer if defined, else
    //   pointer_traits<pointer>::rebind<const value_type>.

    template<typename A, typename Ptr, typename = void>
    struct alloc_const_pointer
    {
        typedef typename pointer_traits<Ptr>
            ::template rebind<const typename A::value_type> type;
    };

    template<typename A, typename Ptr>
    struct alloc_const_pointer
    <
        A,
        Ptr,
        void_t<typename A::const_pointer>
    >
    {
        typedef typename A::const_pointer type;
    };

    // ---------- void_pointer ----------

    template<typename A, typename Ptr, typename = void>
    struct alloc_void_pointer
    {
        typedef typename pointer_traits<Ptr>::template rebind<void> type;
    };

    template<typename A, typename Ptr>
    struct alloc_void_pointer
    <
        A,
        Ptr,
        void_t<typename A::void_pointer>
    >
    {
        typedef typename A::void_pointer type;
    };

    // ---------- const_void_pointer ----------

    template<typename A, typename Ptr, typename = void>
    struct alloc_const_void_pointer
    {
        typedef typename pointer_traits<Ptr>::template rebind<const void> type;
    };

    template<typename A, typename Ptr>
    struct alloc_const_void_pointer
    <
        A,
        Ptr,
        void_t<typename A::const_void_pointer>
    >
    {
        typedef typename A::const_void_pointer type;
    };

    // ---------- difference_type ----------

    template<typename A, typename Ptr, typename = void>
    struct alloc_difference_type
    {
        typedef typename pointer_traits<Ptr>::difference_type type;
    };

    template<typename A, typename Ptr>
    struct alloc_difference_type
    <
        A,
        Ptr,
        void_t<typename A::difference_type>
    >
    {
        typedef typename A::difference_type type;
    };

    // ---------- size_type ----------
    //   A::size_type if defined, else make_unsigned<difference_type>.

    template<typename A, typename Diff, typename = void>
    struct alloc_size_type
    {
        typedef typename make_unsigned<Diff>::type type;
    };

    template<typename A, typename Diff>
    struct alloc_size_type
    <
        A,
        Diff,
        void_t<typename A::size_type>
    >
    {
        typedef typename A::size_type type;
    };

    // ---------- propagate_on_container_copy_assignment ----------

    template<typename A, typename = void>
    struct alloc_pocca
    {
        typedef false_type type;
    };

    template<typename A>
    struct alloc_pocca
    <
        A,
        void_t
        <
            typename A::propagate_on_container_copy_assignment
        >
    >
    {
        typedef typename A::propagate_on_container_copy_assignment type;
    };

    // ---------- propagate_on_container_move_assignment ----------

    template<typename A, typename = void>
    struct alloc_pocma
    {
        typedef false_type type;
    };

    template<typename A>
    struct alloc_pocma
    <
        A,
        void_t
        <
            typename A::propagate_on_container_move_assignment
        >
    >
    {
        typedef typename A::propagate_on_container_move_assignment type;
    };

    // ---------- propagate_on_container_swap ----------

    template<typename A, typename = void>
    struct alloc_pocs
    {
        typedef false_type type;
    };

    template<typename A>
    struct alloc_pocs
    <
        A,
        void_t<typename A::propagate_on_container_swap>
    >
    {
        typedef typename A::propagate_on_container_swap type;
    };

    // ---------- is_always_equal ----------
    //   A::is_always_equal if defined, else is_empty<A>. The is_empty
    //   fallback was added by C++17 and matches the standard's
    //   default rule.

    template<typename A, typename = void>
    struct alloc_is_always_equal
    {
        typedef typename is_empty<A>::type type;
    };

    template<typename A>
    struct alloc_is_always_equal
    <
        A,
        void_t<typename A::is_always_equal>
    >
    {
        typedef typename A::is_always_equal type;
    };

    // ---------- rebind_alloc ----------
    //   A::rebind<U>::other if defined, else replace A's first
    //   template argument with U.

    template<typename A, typename U, typename = void>
    struct alloc_rebind_substituted
    {
        // Primary: ill-formed if A is not a class template specialisation.
        // The pattern below catches the common case.
    };

    template
    <
        template<typename, typename...> class Tmpl,
        typename Head,
        typename... Tail,
        typename U
    >
    struct alloc_rebind_substituted<Tmpl<Head, Tail...>, U>
    {
        typedef Tmpl<U, Tail...> type;
    };

    template<typename A, typename U, typename = void>
    struct alloc_rebind
        : alloc_rebind_substituted<A, U>
    {
    };

    template<typename A, typename U>
    struct alloc_rebind
    <
        A,
        U,
        void_t
        <
            typename A::template rebind<U>::other
        >
    >
    {
        typedef typename A::template rebind<U>::other type;
    };

}  // internal
// =============================================================================
// internal: static-function detection
// =============================================================================

namespace internal
{

    // ---------- has a.allocate(n, hint) ----------

    template
    <
        typename A,
        typename Size,
        typename CVPtr,
        typename = void
    >
    struct has_allocate_hint
        : false_type
    {
    };

    template<typename A, typename Size, typename CVPtr>
    struct has_allocate_hint
    <
        A, Size, CVPtr,
        void_t
        <
            decltype
            (
                re_std::declval<A&>().allocate
                (
                    re_std::declval<Size>(),
                    re_std::declval<CVPtr>()
                )
            )
        >
    >
        : true_type
    {
    };

    // ---------- has a.construct(p, args...) ----------
    //
    //   We use the "auto test(int) -> decltype(...)" trick rather than
    //   void_t because parameter packs sit awkwardly inside the
    //   default-template-arg substitution.

    template<typename A, typename P, typename... Args>
    struct has_member_construct
    {
    private:
        template<typename A1, typename P1, typename... A1rgs>
        static auto try_call(int)
            -> decltype
               (
                   (void)re_std::declval<A1&>().construct
                   (
                       re_std::declval<P1>(),
                       re_std::declval<A1rgs>()...
                   ),
                   true_type()
               );

        template<typename, typename, typename...>
        static false_type try_call(...);

    public:
        typedef decltype(try_call<A, P, Args...>(0)) type;
        static const bool value = type::value;
    };

    // ---------- has a.destroy(p) ----------

    template<typename A, typename P, typename = void>
    struct has_member_destroy
        : false_type
    {
    };

    template<typename A, typename P>
    struct has_member_destroy
    <
        A, P,
        void_t
        <
            decltype(re_std::declval<A&>().destroy(re_std::declval<P>()))
        >
    >
        : true_type
    {
    };

    // ---------- has a.max_size() ----------

    template<typename A, typename = void>
    struct has_member_max_size
        : false_type
    {
    };

    template<typename A>
    struct has_member_max_size
    <
        A,
        void_t
        <
            decltype(re_std::declval<const A&>().max_size())
        >
    >
        : true_type
    {
    };

    // ---------- has a.select_on_container_copy_construction() ----------

    template<typename A, typename = void>
    struct has_member_socc
        : false_type
    {
    };

    template<typename A>
    struct has_member_socc
    <
        A,
        void_t
        <
            decltype
            (
                re_std::declval<const A&>()
                    .select_on_container_copy_construction()
            )
        >
    >
        : true_type
    {
    };

}  // internal
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

    template<typename SizeType, typename ValueType, typename A>
    #if RE_STD_LANG_IS_CPP20_OR_HIGHER
        constexpr
    #endif
    typename enable_if
    <
        has_member_max_size<A>::value,
        SizeType
    >::type
    alloc_max_size_dispatch(const A& _a, int)
    {
        return static_cast<SizeType>(_a.max_size());
    }

    template<typename SizeType, typename ValueType, typename A>
    #if RE_STD_LANG_IS_CPP20_OR_HIGHER
        constexpr
    #endif
    typename enable_if
    <
        !has_member_max_size<A>::value,
        SizeType
    >::type
    alloc_max_size_dispatch(const A&, ...)
    {
        // Fallback formula matches the C++17 wording. size_type is
        // required to be unsigned, so (size_type)-1 is its max.
        return static_cast<SizeType>(-1) / sizeof(ValueType);
    }

    // ---------- select_on_container_copy_construction dispatch ----------

    template<typename A>
    #if RE_STD_LANG_IS_CPP20_OR_HIGHER
        constexpr
    #endif
    typename enable_if
    <
        has_member_socc<A>::value,
        A
    >::type
    alloc_socc_dispatch(const A& _a, int)
    {
        return _a.select_on_container_copy_construction();
    }

    template<typename A>
    #if RE_STD_LANG_IS_CPP20_OR_HIGHER
        constexpr
    #endif
    typename enable_if
    <
        !has_member_socc<A>::value,
        A
    >::type
    alloc_socc_dispatch(const A& _a, ...)
    {
        return _a;
    }

}  // internal
// NOTE: this block must precede allocator_traits: the member functions below
// name internal::alloc_*_dispatch through a qualified-id whose nested-name-
// specifier does not depend on a template parameter, so it is looked up at
// template DEFINITION time, not at instantiation.

// =============================================================================
// allocator_traits
// =============================================================================

// allocator_traits<Alloc>
//   class: uniform allocator interface. All members and all static
//          functions have detection-with-fallback semantics.
template<typename Alloc>
struct allocator_traits
{
    // -------------------------------------------------------------------------
    // member types
    // -------------------------------------------------------------------------

    typedef Alloc                              allocator_type;
    typedef typename Alloc::value_type         value_type;

    typedef typename internal::alloc_pointer<Alloc>::type
        pointer;

    typedef typename internal::alloc_const_pointer<Alloc, pointer>::type
        const_pointer;

    typedef typename internal::alloc_void_pointer<Alloc, pointer>::type
        void_pointer;

    typedef typename internal::alloc_const_void_pointer<Alloc, pointer>::type
        const_void_pointer;

    typedef typename internal::alloc_difference_type<Alloc, pointer>::type
        difference_type;

    typedef typename internal::alloc_size_type<Alloc, difference_type>::type
        size_type;

    typedef typename internal::alloc_pocca<Alloc>::type
        propagate_on_container_copy_assignment;

    typedef typename internal::alloc_pocma<Alloc>::type
        propagate_on_container_move_assignment;

    typedef typename internal::alloc_pocs<Alloc>::type
        propagate_on_container_swap;

    typedef typename internal::alloc_is_always_equal<Alloc>::type
        is_always_equal;

    // rebind_alloc / rebind_traits  (require alias templates)
    #if RE_STD_LANG_HAS_ALIAS_TEMPLATES
        template<typename U>
        using rebind_alloc =
            typename internal::alloc_rebind<Alloc, U>::type;

        template<typename U>
        using rebind_traits = allocator_traits<rebind_alloc<U> >;
    #endif

    // -------------------------------------------------------------------------
    // allocate
    // -------------------------------------------------------------------------

    static
    #if RE_STD_LANG_IS_CPP20_OR_HIGHER
        constexpr
    #endif
    pointer allocate(Alloc& _a, size_type _n)
    {
        return _a.allocate(_n);
    }

    // allocate(a, n, hint) — try a.allocate(n, hint), else a.allocate(n).

    template<typename A>
    static
    #if RE_STD_LANG_IS_CPP20_OR_HIGHER
        constexpr
    #endif
    typename enable_if
    <
        internal::has_allocate_hint<A, size_type, const_void_pointer>::value,
        pointer
    >::type
    allocate(A& _a, size_type _n, const_void_pointer _hint)
    {
        return _a.allocate(_n, _hint);
    }

    template<typename A>
    static
    #if RE_STD_LANG_IS_CPP20_OR_HIGHER
        constexpr
    #endif
    typename enable_if
    <
        !internal::has_allocate_hint<A, size_type, const_void_pointer>::value,
        pointer
    >::type
    allocate(A& _a, size_type _n, const_void_pointer)
    {
        return _a.allocate(_n);
    }

    // -------------------------------------------------------------------------
    // deallocate
    // -------------------------------------------------------------------------

    static
    #if RE_STD_LANG_IS_CPP20_OR_HIGHER
        constexpr
    #endif
    void deallocate(Alloc& _a, pointer _p, size_type _n)
    {
        _a.deallocate(_p, _n);
    }

    // -------------------------------------------------------------------------
    // construct
    // -------------------------------------------------------------------------

    // Two overloads, dispatched on whether Alloc has a member construct.
    //
    // The fallback is `::new((void*)p) U(args...)` on C++11..C++17 and
    // `re_std::construct_at(p, args...)` on C++20+. The C++20 standard
    // mandated the construct_at form so that constexpr-allocator code
    // can trace through allocator_traits without hitting a non-
    // constexpr placement-new expression.

    template<typename U, typename... Args>
    static
    #if RE_STD_LANG_IS_CPP20_OR_HIGHER
        constexpr
    #endif
    typename enable_if
    <
        internal::has_member_construct<Alloc, U*, Args...>::value,
        void
    >::type
    construct(Alloc& _a, U* _p, Args&&... _args)
    {
        _a.construct(_p, re_std::forward<Args>(_args)...);
    }

    template<typename U, typename... Args>
    static
    #if RE_STD_LANG_IS_CPP20_OR_HIGHER
        constexpr
    #endif
    typename enable_if
    <
        !internal::has_member_construct<Alloc, U*, Args...>::value,
        void
    >::type
    construct(Alloc&, U* _p, Args&&... _args)
    {
        #if RE_STD_LANG_IS_CPP20_OR_HIGHER
            re_std::construct_at(_p, re_std::forward<Args>(_args)...);
        #else
            ::new (static_cast<void*>(_p))
                U(re_std::forward<Args>(_args)...);
        #endif
    }

    // -------------------------------------------------------------------------
    // destroy
    // -------------------------------------------------------------------------

    template<typename U>
    static
    #if RE_STD_LANG_IS_CPP20_OR_HIGHER
        constexpr
    #endif
    typename enable_if
    <
        internal::has_member_destroy<Alloc, U*>::value,
        void
    >::type
    destroy(Alloc& _a, U* _p)
    {
        _a.destroy(_p);
    }

    template<typename U>
    static
    #if RE_STD_LANG_IS_CPP20_OR_HIGHER
        constexpr
    #endif
    typename enable_if
    <
        !internal::has_member_destroy<Alloc, U*>::value,
        void
    >::type
    destroy(Alloc&, U* _p)
    {
        re_std::destroy_at(_p);
    }

    // -------------------------------------------------------------------------
    // max_size  /  select_on_container_copy_construction
    //
    // Both have detect-or-fallback semantics. The dispatchers live in
    // internal:: (below) so they don't leak through the public surface
    // of allocator_traits.
    // -------------------------------------------------------------------------

    static
    #if RE_STD_LANG_IS_CPP20_OR_HIGHER
        constexpr
    #endif
    size_type max_size(const Alloc& _a) RE_STD_NOEXCEPT
    {
        return internal::alloc_max_size_dispatch<size_type, value_type>
                   (_a, 0);
    }

    static
    #if RE_STD_LANG_IS_CPP20_OR_HIGHER
        constexpr
    #endif
    Alloc select_on_container_copy_construction(const Alloc& _a)
    {
        return internal::alloc_socc_dispatch(_a, 0);
    }
};




}  // re_std
#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER

#endif  // RE_STD_MEMORY_ALLOCATOR_TRAITS_HPP
