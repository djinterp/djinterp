/******************************************************************************
* djinterp [restd]                                            make_unsigned.hpp
*
* make_unsigned trait header:
*   Yields the unsigned integral type corresponding to _Type. Per
* [meta.trans.sign]:
*   - if _Type is unsigned, _Type is yielded (idempotent);
*   - if _Type is signed, the corresponding unsigned type;
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
* path:      /inc/djinterp/restd/type_traits/make_unsigned.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.28
******************************************************************************/

#ifndef DJINTERP_RESTD_TYPE_TRAITS_MAKE_UNSIGNED_
#define DJINTERP_RESTD_TYPE_TRAITS_MAKE_UNSIGNED_ 1

// djinterp
#include "../../core/djinterp.hpp"


NS_RESTD


// =============================================================================
// I.   MAKE_UNSIGNED
// =============================================================================

NS_INTERNAL

    // make_unsigned_unqualified
    //   helper: maps the unqualified integral type.
    template<typename _Type>
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

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
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

NS_END  // internal


// make_unsigned
//   trait: dispatches via cv-pattern specialization onto the internal
// mapping helper.
template<typename _Type>
struct make_unsigned
{
    typedef typename internal::make_unsigned_unqualified<_Type>::type type;
};

template<typename _Type>
struct make_unsigned<const _Type>
{
    typedef const typename internal::make_unsigned_unqualified<_Type>::type type;
};

template<typename _Type>
struct make_unsigned<volatile _Type>
{
    typedef volatile
        typename internal::make_unsigned_unqualified<_Type>::type type;
};

template<typename _Type>
struct make_unsigned<const volatile _Type>
{
    typedef const volatile
        typename internal::make_unsigned_unqualified<_Type>::type type;
};


// =============================================================================
// II.  MAKE_UNSIGNED_T (C++11+ alias)
// =============================================================================

#if D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES

    // make_unsigned_t
    //   alias: convenience alias for make_unsigned<_Type>::type.
    template<typename _Type>
    using make_unsigned_t = typename make_unsigned<_Type>::type;

#endif  // D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES


NS_END  // restd


#endif  // DJINTERP_RESTD_TYPE_TRAITS_MAKE_UNSIGNED_
