/******************************************************************************
* djinterp [core]                                              cpp_named98.h
*
* C++98 named requirement type traits:
*   Provides compile-time detection of fundamental named requirements using
* C++98-compatible sizeof-based SFINAE techniques. These traits predate
* <type_traits> and rely on overload resolution with sized return types.
*
* SUPPORTED REQUIREMENTS:
*   - DefaultConstructible  (is_default_constructible)
*   - CopyConstructible     (is_copy_constructible)
*   - CopyAssignable        (is_copy_assignable)
*   - Destructible          (is_destructible)
*   - ScalarType            (is_scalar, limited - see note)
*
* LIMITATIONS:
*   - MoveConstructible and MoveAssignable are NOT available (no rvalue
*     references in C++98).
*   - sizeof-based SFINAE cannot detect private/deleted constructors on all
*     compilers. Traits default to true for non-class types and may require
*     manual specialization for complex user-defined types.
*   - Expression SFINAE is not fully supported in C++98; these traits use
*     function-signature SFINAE which is more portable but less precise.
*   - is_scalar uses explicit specializations and cannot detect enumeration
*     types without compiler intrinsics. Enums will report false.
*
* NAMING CONVENTIONS:
*   djinterp::traits::is_default_constructible<_T>::value
*   djinterp::traits::is_copy_constructible<_T>::value
*   djinterp::traits::is_copy_assignable<_T>::value
*   djinterp::traits::is_destructible<_T>::value
*
* path:      \inc\cpp_named98.h
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.02.27
******************************************************************************/

#ifndef DJINTERP_CPP_NAMED98_
#define DJINTERP_CPP_NAMED98_ 1

// require env.h to be included first
#ifndef DJINTERP_ENVIRONMENT_
    #error "cpp_named98.h requires env.h to be included first"
#endif

// only meaningful in C++ mode
#ifndef __cplusplus
    #error "cpp_named98.h can only be used in C++ compilation mode"
#endif

// C++98 SFINAE-based implementations are only active when <type_traits>
// is not available; C++11 supersedes these with compiler-intrinsic traits.
#if !D_ENV_LANG_IS_CPP11_OR_HIGHER

#include <cstddef>


NS_DJINTERP
NS_TRAITS

NS_INTERNAL

    // yes_type
    //   type: sizeof-distinguishable return type for SFINAE success.
    typedef char yes_type;

    // no_type
    //   type: sizeof-distinguishable return type for SFINAE failure.
    struct no_type
    {
        char padding[2];
    };

    // default_constructible_test
    //   trait: internal SFINAE test for default construction viability.
    template<typename _T>
    struct default_constructible_test
    {
    private:
        template<typename _U>
        static yes_type test(int (*)[sizeof(new _U)]);

        template<typename _U>
        static no_type test(...);

    public:
        static const bool value =
            (sizeof(test<_T>(0)) == sizeof(yes_type));
    };

    // copy_constructible_test
    //   trait: internal SFINAE test for copy construction viability.
    template<typename _T>
    struct copy_constructible_test
    {
    private:
        static _T& make_ref();

        template<typename _U>
        static yes_type test(int (*)[sizeof(_U(make_ref()))]);

        template<typename _U>
        static no_type test(...);

    public:
        static const bool value =
            (sizeof(test<_T>(0)) == sizeof(yes_type));
    };

    // copy_assignable_test
    //   trait: internal SFINAE test for copy assignment viability.
    template<typename _T>
    struct copy_assignable_test
    {
    private:
        static _T& make_lref();
        static const _T& make_cref();

        template<typename _U>
        static yes_type test(int (*)[sizeof(make_lref() = make_cref(),
                                            1)]);

        template<typename _U>
        static no_type test(...);

    public:
        static const bool value =
            (sizeof(test<_T>(0)) == sizeof(yes_type));
    };

    // destructible_test
    //   trait: internal SFINAE test for destructor viability.
    // note: in C++98 almost all complete types are destructible; this test
    // catches incomplete types and types with inaccessible destructors on
    // compilers that support sizeof-based SFINAE in this context.
    template<typename _T>
    struct destructible_test
    {
    private:
        template<typename _U>
        static yes_type test(int (*)[sizeof(static_cast<_U*>(0)->~_U(),
                                            1)]);

        template<typename _U>
        static no_type test(...);

    public:
        static const bool value =
            (sizeof(test<_T>(0)) == sizeof(yes_type));
    };

NS_END  // internal


// =========================================================================
// I.   NAMED REQUIREMENT TRAITS (C++98)
// =========================================================================

// is_default_constructible
//   trait: evaluates to true if _T can be default-constructed.
// note: may produce false positives for types with private constructors on
// some C++98 compilers. See cpp_named11.h for a precise implementation.
template<typename _T>
struct is_default_constructible
{
    static const bool value =
        internal::default_constructible_test<_T>::value;
};

// is_copy_constructible
//   trait: evaluates to true if _T can be copy-constructed from a
// const lvalue reference.
template<typename _T>
struct is_copy_constructible
{
    static const bool value =
        internal::copy_constructible_test<_T>::value;
};

// is_copy_assignable
//   trait: evaluates to true if _T can be copy-assigned from a
// const lvalue reference.
template<typename _T>
struct is_copy_assignable
{
    static const bool value =
        internal::copy_assignable_test<_T>::value;
};

// is_destructible
//   trait: evaluates to true if _T has an accessible destructor.
template<typename _T>
struct is_destructible
{
    static const bool value =
        internal::destructible_test<_T>::value;
};


// =========================================================================
// II.  TYPE CLASSIFICATION TRAITS (C++98)
// =========================================================================

NS_INTERNAL

    // remove_cv_98
    //   trait: strips top-level const and volatile qualifiers (C++98).
    template<typename _T> struct remove_cv_98                      { typedef _T type; };
    template<typename _T> struct remove_cv_98<const _T>            { typedef _T type; };
    template<typename _T> struct remove_cv_98<volatile _T>         { typedef _T type; };
    template<typename _T> struct remove_cv_98<const volatile _T>   { typedef _T type; };

    // is_scalar_helper
    //   trait: primary template; defaults to non-scalar.
    template<typename _T>
    struct is_scalar_helper
    {
        static const bool value = false;
    };

    // is_scalar_helper specializations
    //   trait: explicit specializations for arithmetic types.
    template<> struct is_scalar_helper<bool>               { static const bool value = true; };
    template<> struct is_scalar_helper<char>               { static const bool value = true; };
    template<> struct is_scalar_helper<signed char>        { static const bool value = true; };
    template<> struct is_scalar_helper<unsigned char>      { static const bool value = true; };
    template<> struct is_scalar_helper<wchar_t>            { static const bool value = true; };
    template<> struct is_scalar_helper<short>              { static const bool value = true; };
    template<> struct is_scalar_helper<unsigned short>     { static const bool value = true; };
    template<> struct is_scalar_helper<int>                { static const bool value = true; };
    template<> struct is_scalar_helper<unsigned int>       { static const bool value = true; };
    template<> struct is_scalar_helper<long>               { static const bool value = true; };
    template<> struct is_scalar_helper<unsigned long>      { static const bool value = true; };
    template<> struct is_scalar_helper<float>              { static const bool value = true; };
    template<> struct is_scalar_helper<double>             { static const bool value = true; };
    template<> struct is_scalar_helper<long double>        { static const bool value = true; };

    // long long extension (common in C++98 compilers, standard in C++11)
#if defined(__LONG_LONG_MAX__) || defined(LLONG_MAX)
    template<> struct is_scalar_helper<long long>          { static const bool value = true; };
    template<> struct is_scalar_helper<unsigned long long> { static const bool value = true; };
#endif

    // is_scalar_helper<_T*>
    //   trait: partial specialization for pointer types.
    template<typename _T>
    struct is_scalar_helper<_T*>
    {
        static const bool value = true;
    };

    // is_scalar_helper<_T _C::*>
    //   trait: partial specialization for pointer-to-member types.
    template<typename _T,
             typename _C>
    struct is_scalar_helper<_T _C::*>
    {
        static const bool value = true;
    };

NS_END  // internal

// is_scalar
//   trait: evaluates to true if _T is a scalar type (arithmetic,
// pointer, pointer-to-member, or enumeration).
// note: C++98 implementation cannot detect enumeration types without
// compiler intrinsics. Enum types will report false. See cpp_named11.h
// for a complete implementation.
template<typename _T>
struct is_scalar
{
    static const bool value =
        internal::is_scalar_helper<
            typename internal::remove_cv_98<_T>::type>::value;
};


// =========================================================================
// III. COMPARISON TRAITS (C++98)
// =========================================================================

NS_INTERNAL

    // equality_comparable_test
    //   trait: internal SFINAE test for operator== viability.
    template<typename _T>
    struct equality_comparable_test
    {
    private:
        static _T& make_ref();

        template<typename _U>
        static yes_type test(int (*)[sizeof(make_ref() == make_ref(),
                                            1)]);

        template<typename _U>
        static no_type test(...);

    public:
        static const bool value =
            (sizeof(test<_T>(0)) == sizeof(yes_type));
    };

    // less_than_comparable_test
    //   trait: internal SFINAE test for operator< viability.
    template<typename _T>
    struct less_than_comparable_test
    {
    private:
        static _T& make_ref();

        template<typename _U>
        static yes_type test(int (*)[sizeof(make_ref() < make_ref(),
                                            1)]);

        template<typename _U>
        static no_type test(...);

    public:
        static const bool value =
            (sizeof(test<_T>(0)) == sizeof(yes_type));
    };

NS_END  // internal

// is_equality_comparable
//   trait: evaluates to true if _T supports operator==.
// note: C++98 implementation tests syntactic validity only; does not
// verify that the result is convertible to bool. See cpp_named11.h for
// a precise implementation.
template<typename _T>
struct is_equality_comparable
{
    static const bool value =
        internal::equality_comparable_test<_T>::value;
};

// is_less_than_comparable
//   trait: evaluates to true if _T supports operator<.
// note: same limitations as is_equality_comparable above.
template<typename _T>
struct is_less_than_comparable
{
    static const bool value =
        internal::less_than_comparable_test<_T>::value;
};


// =========================================================================
// IV.  CONTAINER TRAITS (C++98)
// =========================================================================

NS_INTERNAL

    // container_test
    //   trait: internal SFINAE test for basic Container member
    // viability (begin, end, size).
    template<typename _C>
    struct container_test
    {
    private:
        static _C& make_ref();

        template<typename _U>
        static yes_type test(int (*)[sizeof(
            make_ref().begin(),
            make_ref().end(),
            make_ref().size(),
            1)]);

        template<typename _U>
        static no_type test(...);

    public:
        static const bool value =
            (sizeof(test<_C>(0)) == sizeof(yes_type));
    };

NS_END  // internal

// is_container
//   trait: evaluates to true if _C has begin(), end(), and size()
// members (partial Container check).
// note: C++98 implementation cannot verify nested types (value_type,
// iterator, etc.). See cpp_named11.h for a thorough implementation.
template<typename _C>
struct is_container
{
    static const bool value =
        internal::container_test<_C>::value;
};


// =========================================================================
// V.   ITERATOR TRAITS (C++98)
// =========================================================================

NS_INTERNAL

    // legacy_iterator_test
    //   trait: internal SFINAE test for LegacyIterator viability:
    // dereferenceable (*it) and incrementable (++it).
    template<typename _I>
    struct legacy_iterator_test
    {
    private:
        static _I& make_ref();

        template<typename _U>
        static yes_type test(int (*)[sizeof(*make_ref(),
                                            ++make_ref(),
                                            1)]);

        template<typename _U>
        static no_type test(...);

    public:
        static const bool value =
            (sizeof(test<_I>(0)) == sizeof(yes_type));
    };

NS_END  // internal

// is_legacy_iterator
//   trait: evaluates to true if _I supports dereference (*it) and
// prefix increment (++it) (partial LegacyIterator check).
// note: C++98 implementation cannot verify iterator_traits nested
// types or CopyConstructible/CopyAssignable requirements. See
// cpp_named11.h for a thorough implementation.
template<typename _I>
struct is_legacy_iterator
{
    static const bool value =
        internal::legacy_iterator_test<_I>::value;
};


NS_END  // traits
NS_END  // djinterp

#endif  // !D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_CPP_NAMED98_
