/*******************************************************************************
* djinterp [re_std]                                            make_unsigned.hpp
*
* make_unsigned trait header:
*   Yields the unsigned integral type corresponding to Type. Per
* [meta.trans.sign]:
*   - if Type is unsigned, Type is yielded (idempotent);
*   - if Type is signed, the corresponding unsigned type;
*   - cv-qualifiers on the input are preserved on the output.
*
*     make_unsigned<int>::type           -> unsigned int
*     make_unsigned<unsigned int>::type  -> unsigned int  (idempotent)
*     make_unsigned<long>::type          -> unsigned long
*     make_unsigned<const int>::type     -> const unsigned int
*
*   PORTABILITY:
*   See make_signed for a discussion of bool, enum, and floating-point
* handling. This trait mirrors make_signed in scope: integral mapping
* only.
*
*
* path:      /inc/re_std/type_traits/make_unsigned.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.28
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_TYPE_TRAITS_MAKE_UNSIGNED_HPP
#define RE_STD_TYPE_TRAITS_MAKE_UNSIGNED_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


namespace re_std
{


// =============================================================================
// I.   MAKE_UNSIGNED
// =============================================================================

namespace internal
{

    // make_unsigned_unqualified
    //   trait: maps the unqualified integral type.
    template<typename Type>
    struct make_unsigned_unqualified;

    // signed -> unsigned
    template<>
    struct make_unsigned_unqualified<signed char>
    { typedef unsigned char type; };

    template<>
    struct make_unsigned_unqualified<short>
    { typedef unsigned short type; };

    template<>
    struct make_unsigned_unqualified<int>
    { typedef unsigned int type; };

    template<>
    struct make_unsigned_unqualified<long>
    { typedef unsigned long type; };

    // already-unsigned: identity
    template<>
    struct make_unsigned_unqualified<unsigned char>
    { typedef unsigned char type; };

    template<>
    struct make_unsigned_unqualified<unsigned short>
    { typedef unsigned short type; };

    template<>
    struct make_unsigned_unqualified<unsigned int>
    { typedef unsigned int type; };

    template<>
    struct make_unsigned_unqualified<unsigned long>
    { typedef unsigned long type; };

#if RE_STD_LANG_IS_CPP11_OR_HIGHER
    template<>
    struct make_unsigned_unqualified<long long>
    { typedef unsigned long long type; };

    template<>
    struct make_unsigned_unqualified<unsigned long long>
    { typedef unsigned long long type; };
#endif

    // plain `char` -> `unsigned char`.
    template<>
    struct make_unsigned_unqualified<char>
    { typedef unsigned char type; };

}  // internal


// make_unsigned
//   trait: dispatches via cv-pattern specialization onto the internal
// mapping helper.
template<typename Type>
struct make_unsigned
{
    typedef typename internal::make_unsigned_unqualified<Type>::type type;
};

template<typename Type>
struct make_unsigned<const Type>
{
    typedef const typename internal::make_unsigned_unqualified<Type>::type type;
};

template<typename Type>
struct make_unsigned<volatile Type>
{
    typedef volatile
        typename internal::make_unsigned_unqualified<Type>::type type;
};

template<typename Type>
struct make_unsigned<const volatile Type>
{
    typedef const volatile
        typename internal::make_unsigned_unqualified<Type>::type type;
};


// =============================================================================
// II.  MAKE_UNSIGNED_T (C++11+ alias)
// =============================================================================

#if RE_STD_LANG_HAS_ALIAS_TEMPLATES

    // make_unsigned_t
    //   alias: convenience alias for make_unsigned<Type>::type.
    template<typename Type>
    using make_unsigned_t = typename make_unsigned<Type>::type;

#endif  // RE_STD_LANG_HAS_ALIAS_TEMPLATES


}  // re_std


#endif  // RE_STD_TYPE_TRAITS_MAKE_UNSIGNED_HPP
