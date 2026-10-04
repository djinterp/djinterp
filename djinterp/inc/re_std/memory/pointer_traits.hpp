/***********************************************************************
* re_std                                                pointer_traits.hpp
*
* uniform interface for pointer-like types:
*   pointer_traits<_Ptr> exposes a fixed set of typedefs and the
* pointer_to() static member, regardless of whether _Ptr is a raw
* pointer or a fancy pointer (boost::interprocess::offset_ptr,
* shared memory pointers, GC handles, etc.). It is the customisation
* point that allocator_traits and the smart-pointer family route
* through.
*
* primary template detection:
*   element_type     = _Ptr::element_type if defined, else the first
*                      template argument extracted from _Ptr.
*   difference_type  = _Ptr::difference_type if defined, else
*                      ptrdiff_t.
*   rebind<_U>       = _Ptr::template rebind<_U> if defined, else
*                      template-arg substitution on _Ptr.
*   pointer_to(_r)   = _Ptr::pointer_to(_r) (no fallback; if _Ptr
*                      lacks it, the call is ill-formed).
*
* raw pointer specialisation pointer_traits<_T*>:
*   element_type     = _T
*   difference_type  = ptrdiff_t
*   rebind<_U>       = _U*
*   pointer_to(_r)   = re_std::addressof(_r), constexpr.
*
* C++11+ floor:
*   The primary template needs alias templates (rebind), variadic
* templates (extracting the head template arg), and SFINAE on member
* types (void_t). All three are C++11. On C++98/03 the header is empty;
* code that needs pointer_traits must itself be gated on
* D_ENV_LANG_IS_CPP11_OR_HIGHER.
*
*
* path:      /inc/djinterp/re_std/memory/pointer_traits.hpp
* link(s):   TBA
* author(s): re_std contributors                         date: 2026.05.01
***********************************************************************/

#ifndef DJINTERP_RE_STD_MEMORY_POINTER_TRAITS_
#define DJINTERP_RE_STD_MEMORY_POINTER_TRAITS_ 1

#include "djinterp.hpp"
#include <cstddef>  // ptrdiff_t

#include "re_std/memory/addressof.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

    #include "re_std/type_traits/void_t.hpp"


namespace re_std
{

// =============================================================================
// internal detection helpers
// =============================================================================

namespace internal
{

    // ---------- element_type detection ----------
    //   If _Ptr::element_type is defined, use it. Otherwise extract
    //   the first template argument from _Ptr.

    template<typename _Ptr, typename = void>
    struct ptr_element_type
    {
        // primary: no nested element_type. Fall back to head-arg.
    };

    template<typename _Ptr>
    struct ptr_element_type
    <
        _Ptr,
        typename void_t<typename _Ptr::element_type>::type
    >
    {
        typedef typename _Ptr::element_type type;
    };

    // Head-arg extraction. Specialises on a class template instantiated
    // with one or more type arguments; yields the first arg.
    template<typename _Ptr>
    struct ptr_head_arg
    {};

    template
    <
        template<typename, typename...> class _Tmpl,
        typename _Head,
        typename... _Tail
    >
    struct ptr_head_arg<_Tmpl<_Head, _Tail...> >
    {
        typedef _Head type;
    };

    // Compose: prefer ptr_element_type::type, else ptr_head_arg::type.
    template<typename _Ptr, typename = void>
    struct ptr_element_type_or_head
        : ptr_head_arg<_Ptr>
    {};

    template<typename _Ptr>
    struct ptr_element_type_or_head
    <
        _Ptr,
        typename void_t<typename _Ptr::element_type>::type
    >
    {
        typedef typename _Ptr::element_type type;
    };


    // ---------- difference_type detection ----------
    //   If _Ptr::difference_type is defined, use it; else ptrdiff_t.

    template<typename _Ptr, typename = void>
    struct ptr_difference_type
    {
        typedef std::ptrdiff_t type;
    };

    template<typename _Ptr>
    struct ptr_difference_type
    <
        _Ptr,
        typename void_t<typename _Ptr::difference_type>::type
    >
    {
        typedef typename _Ptr::difference_type type;
    };


    // ---------- rebind detection ----------
    //   If _Ptr::template rebind<_U> exists, use it.
    //   Else: re-instantiate _Ptr's class template, substituting _U
    //         for the head argument.

    template
    <
        typename _Ptr,
        typename _U,
        typename = void
    >
    struct ptr_rebind_substituted
    {};

    template
    <
        template<typename, typename...> class _Tmpl,
        typename _Head,
        typename... _Tail,
        typename _U
    >
    struct ptr_rebind_substituted<_Tmpl<_Head, _Tail...>, _U>
    {
        typedef _Tmpl<_U, _Tail...> type;
    };

    template
    <
        typename _Ptr,
        typename _U,
        typename = void
    >
    struct ptr_rebind
        : ptr_rebind_substituted<_Ptr, _U>
    {};

    template<typename _Ptr, typename _U>
    struct ptr_rebind
    <
        _Ptr,
        _U,
        typename void_t
        <
            typename _Ptr::template rebind<_U>
        >::type
    >
    {
        typedef typename _Ptr::template rebind<_U> type;
    };

}  // namespace internal


// =============================================================================
// pointer_traits  -  primary template
// =============================================================================

// pointer_traits<_Ptr>
//   trait: uniform pointer interface for fancy pointers.
template<typename _Ptr>
struct pointer_traits
{
    typedef _Ptr pointer;

    typedef typename internal::ptr_element_type_or_head<_Ptr>::type
        element_type;

    typedef typename internal::ptr_difference_type<_Ptr>::type
        difference_type;

    #if D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES
        template<typename _U>
        using rebind = typename internal::ptr_rebind<_Ptr, _U>::type;
    #endif

    static pointer pointer_to(element_type& _r)
    {
        return _Ptr::pointer_to(_r);
    }
};


// =============================================================================
// pointer_traits<_T*>  -  raw pointer specialisation
// =============================================================================

// pointer_traits<_T*>
//   trait: specialisation for raw pointers. Always constexpr-friendly.
template<typename _T>
struct pointer_traits<_T*>
{
    typedef _T*               pointer;
    typedef _T                element_type;
    typedef std::ptrdiff_t    difference_type;

    #if D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES
        template<typename _U>
        using rebind = _U*;
    #endif

    static D_CONSTEXPR pointer pointer_to(element_type& _r) D_NOEXCEPT
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


}  // namespace re_std

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // DJINTERP_RE_STD_MEMORY_POINTER_TRAITS_
