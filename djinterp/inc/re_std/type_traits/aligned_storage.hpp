/*******************************************************************************
* djinterp [re_std]                                          aligned_storage.hpp
*
* aligned_storage trait:
*   Yields `type` as a POD type suitable for use as uninitialized storage
* for an object of size at most Len bytes and alignment at least Align.
* When Align is omitted, the default is the platform's maximum useful
* alignment, computed from a union of fundamental types.
*
*   STANDARD STATUS:
*   Introduced in C++11. Deprecated in C++23 (P1413R3) on the grounds
* that users can equivalently write a small struct directly:
*     struct buf { alignas(N) unsigned char data[Len]; };
* re_std retains the trait on all C++11+ tiers per project policy
* ("available where the language permits"). Migrating new code to the
* equivalent struct form is encouraged when feasible.
*
*   No [[deprecated]] attribute is emitted by default. If you want the
* compiler to surface std's deprecation, see the integration patches doc
* for a one-line addition.
*
*   ALIGNMENT DEFAULT:
*   The default alignment is computed from a local union of fundamental
* types (char through long double, plus pointers and a function pointer).
* This is intentionally NOT std::max_align_t -- using it would pull a
* std-namespace name into the trait's default-argument expression, which
* is awkward for a trait whose entire raison d'etre is to be std-free.
* The local union approach matches the practical behavior of
* std::max_align_t on every platform we target.
*
*   PORTABILITY:
*   Available on C++11 and later. C++98/03 omits the trait (no alignas,
* no alignof). Vendor-specific alignment attributes
* (`__attribute__((aligned))`, `__declspec(align)`) could provide a
* C++03 fallback but are not implemented here.
*
*   DEPENDENCIES:
*   <cstddef> for std::size_t. No re_std traits required.
*
*
* path:      /inc/re_std/type_traits/aligned_storage.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.30
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_TYPE_TRAITS_ALIGNED_STORAGE_HPP
#define RE_STD_TYPE_TRAITS_ALIGNED_STORAGE_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_IS_CPP11_OR_HIGHER

// std
#include <cstddef>  // std::size_t


namespace re_std
{


    namespace internal
    {

        // max_align_helper
        //   union: holds one member of each fundamental type whose
        //          alignment can be implementation-extreme. The
        //          alignment of this union (computed via alignof) is
        //          the platform's effective max alignment, used as the
        //          default for aligned_storage's Align parameter.
        union max_align_helper
        {
            char         m_char;
            short        m_short;
            int          m_int;
            long         m_long;
            long long    m_long_long;
            float        m_float;
            double       m_double;
            long double  m_long_double;
            void*        m_void_ptr;
            void       (*m_func_ptr)();
        };

    }  // internal


    // aligned_storage
    //   trait: yields `type` as a POD struct suitable for use as
    //          uninitialized storage for an object of at most Len
    //          bytes and at least Align-byte alignment.
    template<std::size_t Len,
             std::size_t Align = alignof(internal::max_align_helper)>
    struct aligned_storage
    {
        struct type
        {
            alignas(Align) unsigned char m_data[Len];
        };
    };


    // aligned_storage_t
    //   alias: convenience alias yielding aligned_storage<...>::type
    //          directly. Available wherever alias templates are.
    #if RE_STD_LANG_HAS_ALIAS_TEMPLATES
        template<std::size_t Len,
                 std::size_t Align = alignof(internal::max_align_helper)>
        using aligned_storage_t = typename aligned_storage<Len, Align>::type;
    #endif


}  // re_std


#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER

#endif  // RE_STD_TYPE_TRAITS_ALIGNED_STORAGE_HPP
