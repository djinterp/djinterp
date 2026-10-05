/*******************************************************************************
* djinterp [re_std]                                           pointer_traits.hpp
*
* uniform interface for pointer-like types:
*   pointer_traits<Ptr> exposes a fixed set of typedefs and the
* pointer_to() static member, regardless of whether Ptr is a raw
* pointer or a fancy pointer (boost::interprocess::offset_ptr,
* shared memory pointers, GC handles, etc.). It is the customisation
* point that allocator_traits and the smart-pointer family route
* through.
*
* primary template detection:
*   element_type     = Ptr::element_type if defined, else the first
*                      template argument extracted from Ptr.
*   difference_type  = Ptr::difference_type if defined, else
*                      ptrdiff_t.
*   rebind<U>       = Ptr::template rebind<U> if defined, else
*                      template-arg substitution on Ptr.
*   pointer_to(_r)   = Ptr::pointer_to(_r) (no fallback; if Ptr
*                      lacks it, the call is ill-formed).
*
* raw pointer specialisation pointer_traits<T*>:
*   element_type     = T
*   difference_type  = ptrdiff_t
*   rebind<U>       = U*
*   pointer_to(_r)   = re_std::addressof(_r), constexpr.
*
* C++11+ floor:
*   The primary template needs alias templates (rebind), variadic
* templates (extracting the head template arg), and SFINAE on member
* types (void_t). All three are C++11. On C++98/03 the header is empty;
* code that needs pointer_traits must itself be gated on
* RE_STD_LANG_IS_CPP11_OR_HIGHER.
*
*
* path:      /inc/re_std/memory/pointer_traits.hpp
* link(s):   TBA
* author(s): re_std contributors                             created: 2026.05.01
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_MEMORY_POINTER_TRAITS_HPP
#define RE_STD_MEMORY_POINTER_TRAITS_HPP 1

// std
#include <cstddef>  // ptrdiff_t
// re_std
#include "../config.hpp"  // RE_STD_* configuration
#include "re_std/memory/addressof.hpp"


#if RE_STD_LANG_IS_CPP11_OR_HIGHER

    #include "re_std/type_traits/void_t.hpp"


namespace re_std
{

// =============================================================================
// internal detection helpers
// =============================================================================

namespace internal
{

    // ---------- element_type detection ----------
    //   If Ptr::element_type is defined, use it. Otherwise extract
    //   the first template argument from Ptr.

    template<typename Ptr, typename = void>
    struct ptr_element_type
    {
        // primary: no nested element_type. Fall back to head-arg.
    };

    template<typename Ptr>
    struct ptr_element_type
    <
        Ptr,
        void_t<typename Ptr::element_type>
    >
    {
        typedef typename Ptr::element_type type;
    };

    // Head-arg extraction. Specialises on a class template instantiated
    // with one or more type arguments; yields the first arg.
    template<typename Ptr>
    struct ptr_head_arg
    {};

    template
    <
        template<typename, typename...> class Tmpl,
        typename Head,
        typename... Tail
    >
    struct ptr_head_arg<Tmpl<Head, Tail...> >
    {
        typedef Head type;
    };

    // Compose: prefer ptr_element_type::type, else ptr_head_arg::type.
    template<typename Ptr, typename = void>
    struct ptr_element_type_or_head
        : ptr_head_arg<Ptr>
    {};

    template<typename Ptr>
    struct ptr_element_type_or_head
    <
        Ptr,
        void_t<typename Ptr::element_type>
    >
    {
        typedef typename Ptr::element_type type;
    };


    // ---------- difference_type detection ----------
    //   If Ptr::difference_type is defined, use it; else ptrdiff_t.

    template<typename Ptr, typename = void>
    struct ptr_difference_type
    {
        typedef std::ptrdiff_t type;
    };

    template<typename Ptr>
    struct ptr_difference_type
    <
        Ptr,
        void_t<typename Ptr::difference_type>
    >
    {
        typedef typename Ptr::difference_type type;
    };


    // ---------- rebind detection ----------
    //   If Ptr::template rebind<U> exists, use it.
    //   Else: re-instantiate Ptr's class template, substituting U
    //         for the head argument.

    template
    <
        typename Ptr,
        typename U,
        typename = void
    >
    struct ptr_rebind_substituted
    {};

    template
    <
        template<typename, typename...> class Tmpl,
        typename Head,
        typename... Tail,
        typename U
    >
    struct ptr_rebind_substituted<Tmpl<Head, Tail...>, U>
    {
        typedef Tmpl<U, Tail...> type;
    };

    template
    <
        typename Ptr,
        typename U,
        typename = void
    >
    struct ptr_rebind
        : ptr_rebind_substituted<Ptr, U>
    {};

    template<typename Ptr, typename U>
    struct ptr_rebind
    <
        Ptr,
        U,
        void_t
        <
            typename Ptr::template rebind<U>
        >
    >
    {
        typedef typename Ptr::template rebind<U> type;
    };

}  // internal
// =============================================================================
// pointer_traits  -  primary template
// =============================================================================

// pointer_traits<Ptr>
//   trait: uniform pointer interface for fancy pointers.
template<typename Ptr>
struct pointer_traits
{
    typedef Ptr pointer;

    typedef typename internal::ptr_element_type_or_head<Ptr>::type
        element_type;

    typedef typename internal::ptr_difference_type<Ptr>::type
        difference_type;

    #if RE_STD_LANG_HAS_ALIAS_TEMPLATES
        template<typename U>
        using rebind = typename internal::ptr_rebind<Ptr, U>::type;
    #endif

    static pointer pointer_to(element_type& _r)
    {
        return Ptr::pointer_to(_r);
    }
};


// =============================================================================
// pointer_traits<T*>  -  raw pointer specialisation
// =============================================================================

// pointer_traits<T*>
//   trait: specialisation for raw pointers. Always constexpr-friendly.
template<typename T>
struct pointer_traits<T*>
{
    typedef T*               pointer;
    typedef T                element_type;
    typedef std::ptrdiff_t    difference_type;

    #if RE_STD_LANG_HAS_ALIAS_TEMPLATES
        template<typename U>
        using rebind = U*;
    #endif

    static RE_STD_CONSTEXPR pointer pointer_to(element_type& _r) RE_STD_NOEXCEPT
    {
        return re_std::addressof(_r);
    }
};


// pointer_traits<void*> and cv-qualified variants need their own
// element_type rule: there is no useful element_type for `void*`, but
// the standard nominally still defines one (void). The general
// raw-pointer specialisation above already produces element_type = void
// for these cases, and pointer_to is well-formed because void& is
// ill-formed and so the function is never instantiated. No further
// specialisation needed.


}  // re_std
#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER

#endif  // RE_STD_MEMORY_POINTER_TRAITS_HPP
