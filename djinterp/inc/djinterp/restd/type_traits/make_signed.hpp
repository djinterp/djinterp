/******************************************************************************
* djinterp [restd]                                              make_signed.hpp
*
* make_signed trait header:
*   Yields the signed integral type corresponding to _Type. Per
* [meta.trans.sign]:
*   - if _Type is a signed integer, _Type is yielded;
*   - if _Type is an unsigned integer, the corresponding signed type;
*   - if _Type is char or bool, the corresponding signed integer type
*     of the same size;
*   - cv-qualifiers on the input are preserved on the output.
*
*     make_signed<unsigned int>::type     -> int
*     make_signed<int>::type              -> int            (idempotent)
*     make_signed<char>::type             -> signed char
*     make_signed<unsigned long>::type    -> long
*     make_signed<const unsigned int>::type -> const int
*
*   PORTABILITY:
*   Implementation maps the standard unsigned integers to their signed
* counterparts via internal explicit specializations. Floating-point
* types and bool are not handled (per the standard, applying make_signed
* to them is ill-formed; this implementation simply provides no `type`
* member in those cases, causing a substitution failure suitable for
* SFINAE).
*   Enumeration types are not handled here. The standard requires
* yielding the underlying integer type's signed counterpart; that path
* depends on the `__underlying_type` intrinsic (see underlying_type.hpp)
* and is left to a future refinement.
*
*
* path:      /inc/djinterp/restd/type_traits/make_signed.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.28
******************************************************************************/

#ifndef DJINTERP_RESTD_TYPE_TRAITS_MAKE_SIGNED_
#define DJINTERP_RESTD_TYPE_TRAITS_MAKE_SIGNED_ 1

// djinterp
#include "../../core/djinterp.hpp"


NS_RESTD


// =============================================================================
// I.   MAKE_SIGNED
// =============================================================================

NS_INTERNAL

    // make_signed_unqualified
    //   helper: maps the unqualified integral type. No primary template
    // body, so non-integrals trigger SFINAE.
    template<typename _Type>
    struct make_signed_unqualified;

    // already-signed integers: identity
    template<>
    struct make_signed_unqualified<signed char>
    { typedef signed char type; };

    template<>
    struct make_signed_unqualified<short>
    { typedef short type; };

    template<>
    struct make_signed_unqualified<int>
    { typedef int type; };

    template<>
    struct make_signed_unqualified<long>
    { typedef long type; };

    // unsigned integers: drop the unsigned
    template<>
    struct make_signed_unqualified<unsigned char>
    { typedef signed char type; };

    template<>
    struct make_signed_unqualified<unsigned short>
    { typedef short type; };

    template<>
    struct make_signed_unqualified<unsigned int>
    { typedef int type; };

    template<>
    struct make_signed_unqualified<unsigned long>
    { typedef long type; };

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    template<>
    struct make_signed_unqualified<long long>
    { typedef long long type; };

    template<>
    struct make_signed_unqualified<unsigned long long>
    { typedef long long type; };
#endif

    // plain `char` is implementation-defined; map to `signed char`.
    template<>
    struct make_signed_unqualified<char>
    { typedef signed char type; };

NS_END  // internal


// make_signed
//   trait: dispatches by stripping cv on the input, mapping via the
// internal helper, and re-applying cv. The four cv-flavors are handled
// by explicit specialization on the trait itself rather than by a
// remove_cv / add_cv composition (cleaner SFINAE, and avoids depending
// on add_cv's full machinery).
template<typename _Type>
struct make_signed
{
    typedef typename internal::make_signed_unqualified<_Type>::type type;
};

template<typename _Type>
struct make_signed<const _Type>
{
    typedef const typename internal::make_signed_unqualified<_Type>::type type;
};

template<typename _Type>
struct make_signed<volatile _Type>
{
    typedef volatile typename internal::make_signed_unqualified<_Type>::type type;
};

template<typename _Type>
struct make_signed<const volatile _Type>
{
    typedef const volatile
        typename internal::make_signed_unqualified<_Type>::type type;
};


// =============================================================================
// II.  MAKE_SIGNED_T (C++11+ alias)
// =============================================================================

#if D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES

    // make_signed_t
    //   alias: convenience alias for make_signed<_Type>::type.
    template<typename _Type>
    using make_signed_t = typename make_signed<_Type>::type;

#endif  // D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES


NS_END  // restd


#endif  // DJINTERP_RESTD_TYPE_TRAITS_MAKE_SIGNED_
