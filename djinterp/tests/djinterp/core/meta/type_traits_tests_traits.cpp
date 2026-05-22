/******************************************************************************
* djinterp [test]                                   type_traits_tests_traits.cpp
*
*   Unit tests for the "container-shape" detection traits in Section III
* of type_traits.hpp:
*     - has_max_size            + _v
*     - has_nested_template_type + _v
*     - has_variadic_constructor + _v
*     - is_allocator            + _v
*     - is_bounded              + _v
*     - is_nonvoid              + _v
*     - is_sized                + _v
*     - is_valid_size_type      + _v
*
*   Each trait is exercised against:
*     - a positive subject crafted to satisfy the predicate exactly
*     - a negative subject lacking the feature(s)
*     - a near-miss subject (e.g. has size_type but no max_size, or
*       size_type that isn't size_t-convertible) where the trait's
*       composite predicate must reject correctly
*
*
* path:      /inc/djinterp/test/type_traits_tests_traits.cpp
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.20
******************************************************************************/
#include "./type_traits_tests.hpp"


NS_DJINTERP
NS_TEST

namespace
{

// has_max_size_subject_t
//   subject: size_type AND `static constexpr` max_size of EXACTLY
// `const size_type` (the trait checks this with std::is_same). Positive
// case.
struct has_max_size_subject_t
{
    using size_type = std::size_t;
    static constexpr const size_type max_size = 1024;
};

// has_max_size_wrong_type_t
//   subject: size_type and max_size exist, but max_size is `const int`
// (not the type's own size_type).  The trait demands type-identity via
// std::is_same -- this should be rejected.
struct has_max_size_wrong_type_t
{
    using size_type = std::size_t;
    static constexpr const int max_size = 1024;
};

// no_max_size_t
//   subject: has size_type but no max_size. Negative case.
struct no_max_size_t
{
    using size_type = std::size_t;
};

// has_nested_template_type_subject_t
//   subject: has nested template alias `type<X> = X`. Positive case.
struct has_nested_template_type_subject_t
{
    template<typename _X>
    using type = _X;
};

// has_nested_template_type_negative_t
//   subject: has a NON-template `type` (regular typedef). Negative case
// for has_nested_template_type because the probe is
// `typename _Type::template type<int>`, which fails unless `type` is
// a template.
struct has_nested_template_type_negative_t
{
    using type = int;  // non-template -- can't be instantiated with <int>
};

// has_variadic_constructor_subject_t
//   subject: a class with a single-arg ctor accepting itself by value
// (the trait probes `decltype(_Type(std::declval<_Type>()))`).  Positive
// case.
struct has_variadic_constructor_subject_t
{
    has_variadic_constructor_subject_t() = default;
    has_variadic_constructor_subject_t(const has_variadic_constructor_subject_t&) = default;
};

// not_constructible_from_self_t
//   subject: copy ctor deleted -> probe fails.  Negative case.
struct not_constructible_from_self_t
{
    not_constructible_from_self_t() = default;
    not_constructible_from_self_t(const not_constructible_from_self_t&) = delete;
};

// is_sized_strange_t
//   subject: size() exists and returns size_t, but size_type is unrelated
// (a struct, not convertible to size_t).  Should be rejected by is_sized.
struct is_sized_strange_t
{
    struct opaque {};
    using size_type = opaque;

    std::size_t
    size() const
    {
        return 0;
    }
};

// is_sized_negative_t
//   subject: has size_type but no size() method.  Negative case.
struct is_sized_negative_t
{
    using size_type = std::size_t;
};

}  // namespace


// =========================================================================
// I.   has_max_size  (compile-time)
// =========================================================================

static_assert(has_max_size<has_max_size_subject_t>::value == true,
              "has_max_size<positive subject> -> true");
static_assert(has_max_size<has_max_size_wrong_type_t>::value == false,
              "has_max_size: max_size wrong type -> false (strict type identity)");
static_assert(has_max_size<no_max_size_t>::value == false,
              "has_max_size: no max_size -> false");
static_assert(has_max_size<int>::value == false,
              "has_max_size<int> -> false");

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    static_assert(has_max_size_v<has_max_size_subject_t> == true,
                  "has_max_size_v: matches struct (true)");
    static_assert(has_max_size_v<no_max_size_t> == false,
                  "has_max_size_v: matches struct (false)");
#endif


// =========================================================================
// II.  has_nested_template_type  (compile-time)
// =========================================================================

static_assert(has_nested_template_type<has_nested_template_type_subject_t>::value == true,
              "has_nested_template_type: nested alias template -> true");
static_assert(has_nested_template_type<has_nested_template_type_negative_t>::value == false,
              "has_nested_template_type: non-template `type` -> false");
static_assert(has_nested_template_type<int>::value == false,
              "has_nested_template_type<int> -> false");

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    static_assert(has_nested_template_type_v<has_nested_template_type_subject_t> == true,
                  "has_nested_template_type_v: matches struct (true)");
    static_assert(has_nested_template_type_v<has_nested_template_type_negative_t> == false,
                  "has_nested_template_type_v: matches struct (false)");
#endif


// =========================================================================
// III. has_variadic_constructor  (compile-time)
// =========================================================================
//   The trait's probe is `decltype(_Type(std::declval<_Type>()))` --
// i.e. constructible from itself.  Any copy- or move-constructible type
// passes; only types that delete or hide the relevant constructor fail.

static_assert(has_variadic_constructor<has_variadic_constructor_subject_t>::value == true,
              "has_variadic_constructor<copy-constructible> -> true");
static_assert(has_variadic_constructor<int>::value == true,
              "has_variadic_constructor<int> -> true (builtin)");
static_assert(has_variadic_constructor<not_constructible_from_self_t>::value == false,
              "has_variadic_constructor<copy-deleted> -> false");

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    static_assert(has_variadic_constructor_v<int> == true,
                  "has_variadic_constructor_v: matches struct (true)");
    static_assert(has_variadic_constructor_v<not_constructible_from_self_t> == false,
                  "has_variadic_constructor_v: matches struct (false)");
#endif


// =========================================================================
// IV.  is_allocator  (compile-time)
// =========================================================================
//   KNOWN BUGS:
//
//   B5: is_allocator's probes are not SFINAE-friendly. The first probe
//       `typename std::allocator_traits<_Type>::value_type` triggers
//       hard errors when _Type is anything std::allocator_traits' primary
//       template can't process gracefully (arithmetic types, void, class
//       types without a `value_type` nested typedef).  So negative cases
//       like is_allocator<int> simply cannot be compiled.
//
//   B6: The allocate/deallocate probes use `std::declval<_Type>()` --
//       which is an RVALUE -- but std::allocator_traits::allocate and
//       deallocate take an `Alloc&` (lvalue reference).  An rvalue
//       won't bind to a non-const lvalue reference, so the probe is
//       a substitution failure even for valid allocator types like
//       std::allocator<int>.  The fix would be `std::declval<_Type&>()`.
//
//   Combined effect: the trait reports false for actual allocator types
// AND hard-errors on most other types.  We document the observable
// (broken) behaviour with allocators:

static_assert(is_allocator<std::allocator<int>>::value == false,
              "[BUG B6] is_allocator<allocator<int>> wrongly false (probe rvalue won't bind)");
static_assert(is_allocator<std::allocator<char>>::value == false,
              "[BUG B6] is_allocator<allocator<char>> wrongly false (probe rvalue won't bind)");

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    static_assert(is_allocator_v<std::allocator<int>> == false,
                  "[BUG B6] is_allocator_v matches struct (wrongly false)");
#endif


// =========================================================================
// V.   is_bounded  (compile-time)
// =========================================================================
//   is_bounded<T, Trait> := Trait<T>::value AND has_max_size<T>::value.

// has_max_size_subject_t satisfies has_max_size; is_class is also true
static_assert(is_bounded<has_max_size_subject_t, std::is_class>::value == true,
              "is_bounded<positive, is_class> -> true (both predicates pass)");

// passes has_max_size but fails the secondary trait
static_assert(is_bounded<has_max_size_subject_t, std::is_arithmetic>::value == false,
              "is_bounded<positive, is_arithmetic> -> false (secondary trait fails)");

// fails has_max_size
static_assert(is_bounded<no_max_size_t, std::is_class>::value == false,
              "is_bounded<no_max_size, is_class> -> false");

// fails both
static_assert(is_bounded<int, std::is_class>::value == false,
              "is_bounded<int, is_class> -> false");

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    static_assert(is_bounded_v<has_max_size_subject_t, std::is_class> == true,
                  "is_bounded_v matches struct (true)");
    static_assert(is_bounded_v<int, std::is_class> == false,
                  "is_bounded_v matches struct (false)");
#endif


// =========================================================================
// VI.  is_nonvoid  (compile-time)
// =========================================================================

// positive cases
static_assert(is_nonvoid<int>::value == true,
              "is_nonvoid<int> -> true");
static_assert(is_nonvoid<int*>::value == true,
              "is_nonvoid<int*> -> true (pointer to void is still nonvoid)");
static_assert(is_nonvoid<void*>::value == true,
              "is_nonvoid<void*> -> true (pointer to void is itself nonvoid)");
static_assert(is_nonvoid<std::vector<int>>::value == true,
              "is_nonvoid<vector<int>> -> true");

// negative cases (cv-void)
static_assert(is_nonvoid<void>::value == false,
              "is_nonvoid<void> -> false");
static_assert(is_nonvoid<const void>::value == false,
              "is_nonvoid<const void> -> false");
static_assert(is_nonvoid<volatile void>::value == false,
              "is_nonvoid<volatile void> -> false");

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    static_assert(is_nonvoid_v<int> == true,
                  "is_nonvoid_v: matches struct (true)");
    static_assert(is_nonvoid_v<void> == false,
                  "is_nonvoid_v: matches struct (false)");
#endif


// =========================================================================
// VII. is_sized  (compile-time)
// =========================================================================
//   Requires: size_type typedef, size() method, both convertible to
// std::size_t.

using sized_subject = type_traits_test_types::with_size_and_size_type;

// positive case
static_assert(is_sized<sized_subject>::value == true,
              "is_sized<with_size_and_size_type> -> true");

// also positive: std::vector<int>
static_assert(is_sized<std::vector<int>>::value == true,
              "is_sized<vector<int>> -> true");

// negative: missing size()
static_assert(is_sized<is_sized_negative_t>::value == false,
              "is_sized: missing size() -> false");

// negative: size_type isn't convertible to size_t
static_assert(is_sized<is_sized_strange_t>::value == false,
              "is_sized: size_type not convertible to size_t -> false");

// negative: builtin
static_assert(is_sized<int>::value == false,
              "is_sized<int> -> false");

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    static_assert(is_sized_v<sized_subject> == true,
                  "is_sized_v: matches struct (true)");
    static_assert(is_sized_v<int> == false,
                  "is_sized_v: matches struct (false)");
#endif


// =========================================================================
// VIII. is_valid_size_type  (compile-time)
// =========================================================================
//   is_valid_size_type<T> := is_unsigned<T> AND is_arithmetic<T>.

// positive cases -- unsigned arithmetic
static_assert(is_valid_size_type<std::size_t>::value == true,
              "is_valid_size_type<size_t> -> true");
static_assert(is_valid_size_type<unsigned int>::value == true,
              "is_valid_size_type<unsigned int> -> true");
static_assert(is_valid_size_type<unsigned char>::value == true,
              "is_valid_size_type<unsigned char> -> true");
static_assert(is_valid_size_type<unsigned long long>::value == true,
              "is_valid_size_type<unsigned long long> -> true");

// negative cases -- signed arithmetic
static_assert(is_valid_size_type<int>::value == false,
              "is_valid_size_type<int> -> false (signed)");
static_assert(is_valid_size_type<long>::value == false,
              "is_valid_size_type<long> -> false (signed)");

// negative cases -- non-arithmetic
static_assert(is_valid_size_type<void>::value == false,
              "is_valid_size_type<void> -> false");
static_assert(is_valid_size_type<int*>::value == false,
              "is_valid_size_type<int*> -> false");
static_assert(is_valid_size_type<std::vector<int>>::value == false,
              "is_valid_size_type<vector<int>> -> false");

// negative case -- floating point (arithmetic but signed)
static_assert(is_valid_size_type<float>::value == false,
              "is_valid_size_type<float> -> false");
static_assert(is_valid_size_type<double>::value == false,
              "is_valid_size_type<double> -> false");

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    static_assert(is_valid_size_type_v<std::size_t> == true,
                  "is_valid_size_type_v: matches struct (true)");
    static_assert(is_valid_size_type_v<int> == false,
                  "is_valid_size_type_v: matches struct (false)");
#endif


// =========================================================================
// IX.  RUNTIME DRIVER
// =========================================================================

void
type_traits_tests_traits(
    test_handler& _test_handler
)
{
    // ---- has_max_size ----
    record_assertion(_test_handler, 
        has_max_size<has_max_size_subject_t>::value == true,
        "has_max_size: positive case");
    record_assertion(_test_handler, 
        has_max_size<has_max_size_wrong_type_t>::value == false,
        "has_max_size: wrong type for max_size -> false");
    record_assertion(_test_handler, 
        has_max_size<no_max_size_t>::value == false,
        "has_max_size: no max_size -> false");
    record_assertion(_test_handler, 
        has_max_size<int>::value == false,
        "has_max_size<int> -> false");

    // ---- has_nested_template_type ----
    record_assertion(_test_handler, 
        has_nested_template_type<has_nested_template_type_subject_t>::value == true,
        "has_nested_template_type: positive");
    record_assertion(_test_handler, 
        has_nested_template_type<has_nested_template_type_negative_t>::value == false,
        "has_nested_template_type: non-template `type` -> false");
    record_assertion(_test_handler, 
        has_nested_template_type<int>::value == false,
        "has_nested_template_type<int> -> false");

    // ---- has_variadic_constructor ----
    record_assertion(_test_handler, 
        has_variadic_constructor<has_variadic_constructor_subject_t>::value == true,
        "has_variadic_constructor: positive");
    record_assertion(_test_handler, 
        has_variadic_constructor<int>::value == true,
        "has_variadic_constructor<int> -> true");
    record_assertion(_test_handler, 
        has_variadic_constructor<not_constructible_from_self_t>::value == false,
        "has_variadic_constructor: copy-deleted -> false");

    // ---- is_allocator (broken, B5+B6) ----
    record_assertion(_test_handler, 
        is_allocator<std::allocator<int>>::value == false,
        "[BUG B6] is_allocator<allocator<int>> wrongly false (rvalue probe)");
    record_assertion(_test_handler, 
        is_allocator<std::allocator<char>>::value == false,
        "[BUG B6] is_allocator<allocator<char>> wrongly false (rvalue probe)");

    // ---- is_bounded ----
    record_assertion(_test_handler, 
        is_bounded<has_max_size_subject_t, std::is_class>::value == true,
        "is_bounded<positive, is_class>");
    record_assertion(_test_handler, 
        is_bounded<has_max_size_subject_t, std::is_arithmetic>::value == false,
        "is_bounded<positive, is_arithmetic> -> false (2nd trait fails)");
    record_assertion(_test_handler, 
        is_bounded<no_max_size_t, std::is_class>::value == false,
        "is_bounded<no_max_size, is_class> -> false");
    record_assertion(_test_handler, 
        is_bounded<int, std::is_class>::value == false,
        "is_bounded<int, is_class> -> false");

    // ---- is_nonvoid ----
    record_assertion(_test_handler, 
        is_nonvoid<int>::value == true,
        "is_nonvoid<int>");
    record_assertion(_test_handler, 
        is_nonvoid<int*>::value == true,
        "is_nonvoid<int*>");
    record_assertion(_test_handler, 
        is_nonvoid<void*>::value == true,
        "is_nonvoid<void*>");
    record_assertion(_test_handler, 
        is_nonvoid<void>::value == false,
        "is_nonvoid<void>");
    record_assertion(_test_handler, 
        is_nonvoid<const void>::value == false,
        "is_nonvoid<const void>");
    record_assertion(_test_handler, 
        is_nonvoid<volatile void>::value == false,
        "is_nonvoid<volatile void>");

    // ---- is_sized ----
    record_assertion(_test_handler, 
        is_sized<sized_subject>::value == true,
        "is_sized<sized_subject>");
    record_assertion(_test_handler, 
        is_sized<std::vector<int>>::value == true,
        "is_sized<vector<int>>");
    record_assertion(_test_handler, 
        is_sized<is_sized_negative_t>::value == false,
        "is_sized: missing size() -> false");
    record_assertion(_test_handler, 
        is_sized<is_sized_strange_t>::value == false,
        "is_sized: size_type not convertible to size_t -> false");
    record_assertion(_test_handler, 
        is_sized<int>::value == false,
        "is_sized<int> -> false");

    // ---- is_valid_size_type ----
    record_assertion(_test_handler, 
        is_valid_size_type<std::size_t>::value == true,
        "is_valid_size_type<size_t>");
    record_assertion(_test_handler, 
        is_valid_size_type<unsigned int>::value == true,
        "is_valid_size_type<unsigned int>");
    record_assertion(_test_handler, 
        is_valid_size_type<unsigned char>::value == true,
        "is_valid_size_type<unsigned char>");
    record_assertion(_test_handler, 
        is_valid_size_type<int>::value == false,
        "is_valid_size_type<int> -> false (signed)");
    record_assertion(_test_handler, 
        is_valid_size_type<void>::value == false,
        "is_valid_size_type<void> -> false");
    record_assertion(_test_handler, 
        is_valid_size_type<int*>::value == false,
        "is_valid_size_type<int*> -> false");
    record_assertion(_test_handler, 
        is_valid_size_type<float>::value == false,
        "is_valid_size_type<float> -> false");

    return;
}


NS_END  // test
NS_END  // djinterp
