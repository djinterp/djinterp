/*******************************************************************************
* djinterp [env]                                              env_cpp_features.h
*
* djinterp C++ feature detection.
*   Compile-time detection of C++ language and standard-library features,
* read from the SD-6 feature-test macros (__cpp_* and __cpp_lib_*): C++11 to
* C++26 language features, C++14 to C++26 library features, and aggregate
* checks for each standard.
*   D_ENV_CPP_FEATURE_LANG_* name language features, D_ENV_CPP_FEATURE_STL_*
* library features, and D_ENV_CPP_FEATURE_HAS_ALL_* the aggregates. Each
* feature has five macros: the flag, 1 if its feature-test macro is defined
* and 0 otherwise; _VAL, that macro's value, or 0L; and _NAME, _DESC and
* _VERS, the macro's name, a description, and the standard that introduced
* it, as strings.
*   The library feature-test macros come from <version> and from each library
* header, and this header includes neither, so the D_ENV_CPP_FEATURE_STL_*
* flags see only what the translation unit included before it.
*
*
* path:      /inc/djinterp/env/cpp/env_cpp_features.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.01.15
*                                                            revised: 2026.10.01
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  LANGUAGE FEATURES
    -----------------
    1.  C++11 language features
         1.  D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES
         2.  D_ENV_CPP_FEATURE_LANG_ATTRIBUTES
         3.  D_ENV_CPP_FEATURE_LANG_CONSTEXPR
         4.  D_ENV_CPP_FEATURE_LANG_DECLTYPE
         5.  D_ENV_CPP_FEATURE_LANG_DELEGATING_CONSTRUCTORS
         6.  D_ENV_CPP_FEATURE_LANG_INHERITING_CONSTRUCTORS
         7.  D_ENV_CPP_FEATURE_LANG_INITIALIZER_LISTS
         8.  D_ENV_CPP_FEATURE_LANG_LAMBDAS
         9.  D_ENV_CPP_FEATURE_LANG_NSDMI
         10. D_ENV_CPP_FEATURE_LANG_RANGE_BASED_FOR
         11. D_ENV_CPP_FEATURE_LANG_RAW_STRINGS
         12. D_ENV_CPP_FEATURE_LANG_REF_QUALIFIERS
         13. D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES
         14. D_ENV_CPP_FEATURE_LANG_STATIC_ASSERT
         15. D_ENV_CPP_FEATURE_LANG_THREADSAFE_STATIC_INIT
         16. D_ENV_CPP_FEATURE_LANG_UNICODE_CHARACTERS
         17. D_ENV_CPP_FEATURE_LANG_UNICODE_LITERALS
         18. D_ENV_CPP_FEATURE_LANG_USER_DEFINED_LITERALS
         19. D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES
    2.  C++14 language features
         1.  D_ENV_CPP_FEATURE_LANG_AGGREGATE_NSDMI
         2.  D_ENV_CPP_FEATURE_LANG_BINARY_LITERALS
         3.  D_ENV_CPP_FEATURE_LANG_DECLTYPE_AUTO
         4.  D_ENV_CPP_FEATURE_LANG_ENUMERATOR_ATTRIBUTES
         5.  D_ENV_CPP_FEATURE_LANG_GENERIC_LAMBDAS
         6.  D_ENV_CPP_FEATURE_LANG_INIT_CAPTURES
         7.  D_ENV_CPP_FEATURE_LANG_NAMESPACE_ATTRIBUTES
         8.  D_ENV_CPP_FEATURE_LANG_NONTYPE_TEMPLATE_ARGS
         9.  D_ENV_CPP_FEATURE_LANG_RETURN_TYPE_DEDUCTION
         10. D_ENV_CPP_FEATURE_LANG_SIZED_DEALLOCATION
         11. D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    3.  C++17 language features
         1.  D_ENV_CPP_FEATURE_LANG_AGGREGATE_BASES
         2.  D_ENV_CPP_FEATURE_LANG_ALIGNED_NEW
         3.  D_ENV_CPP_FEATURE_LANG_CAPTURE_STAR_THIS
         4.  D_ENV_CPP_FEATURE_LANG_CONSTEXPR_IN_DECLTYPE
         5.  D_ENV_CPP_FEATURE_LANG_DEDUCTION_GUIDES
         6.  D_ENV_CPP_FEATURE_LANG_FOLD_EXPRESSIONS
         7.  D_ENV_CPP_FEATURE_LANG_GUARANTEED_COPY_ELISION
         8.  D_ENV_CPP_FEATURE_LANG_HEX_FLOAT
         9.  D_ENV_CPP_FEATURE_LANG_IF_CONSTEXPR
         10. D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
         11. D_ENV_CPP_FEATURE_LANG_NOEXCEPT_FUNCTION_TYPE
         12. D_ENV_CPP_FEATURE_LANG_NONTYPE_TEMPLATE_PARAMETER_AUTO
         13. D_ENV_CPP_FEATURE_LANG_STRUCTURED_BINDINGS
         14. D_ENV_CPP_FEATURE_LANG_TEMPLATE_TEMPLATE_ARGS
         15. D_ENV_CPP_FEATURE_LANG_VARIADIC_USING
    4.  C++20 language features
         1.  D_ENV_CPP_FEATURE_LANG_AGGREGATE_PAREN_INIT
         2.  D_ENV_CPP_FEATURE_LANG_CHAR8_T
         3.  D_ENV_CPP_FEATURE_LANG_CONCEPTS
         4.  D_ENV_CPP_FEATURE_LANG_CONDITIONAL_EXPLICIT
         5.  D_ENV_CPP_FEATURE_LANG_CONSTEVAL
         6.  D_ENV_CPP_FEATURE_LANG_CONSTEXPR_DYNAMIC_ALLOC
         7.  D_ENV_CPP_FEATURE_LANG_CONSTINIT
         8.  D_ENV_CPP_FEATURE_LANG_DESIGNATED_INITIALIZERS
         9.  D_ENV_CPP_FEATURE_LANG_IMPL_COROUTINE
         10. D_ENV_CPP_FEATURE_LANG_IMPL_DESTROYING_DELETE
         11. D_ENV_CPP_FEATURE_LANG_IMPL_THREE_WAY_COMPARISON
         12. D_ENV_CPP_FEATURE_LANG_MODULES
         13. D_ENV_CPP_FEATURE_LANG_USING_ENUM
    5.  C++23 language features
         1.  D_ENV_CPP_FEATURE_LANG_AUTO_CAST
         2.  D_ENV_CPP_FEATURE_LANG_EXPLICIT_THIS_PARAMETER
         3.  D_ENV_CPP_FEATURE_LANG_IF_CONSTEVAL
         4.  D_ENV_CPP_FEATURE_LANG_IMPLICIT_MOVE
         5.  D_ENV_CPP_FEATURE_LANG_MULTIDIMENSIONAL_SUBSCRIPT
         6.  D_ENV_CPP_FEATURE_LANG_NAMED_CHARACTER_ESCAPES
         7.  D_ENV_CPP_FEATURE_LANG_SIZE_T_SUFFIX
         8.  D_ENV_CPP_FEATURE_LANG_STATIC_CALL_OPERATOR
    6.  C++26 language features
         1.  D_ENV_CPP_FEATURE_LANG_CONSTEXPR_EXCEPTIONS
         2.  D_ENV_CPP_FEATURE_LANG_CONTRACTS
         3.  D_ENV_CPP_FEATURE_LANG_DELETED_FUNCTION
         4.  D_ENV_CPP_FEATURE_LANG_PACK_INDEXING
         5.  D_ENV_CPP_FEATURE_LANG_PLACEHOLDER_VARIABLES
         6.  D_ENV_CPP_FEATURE_LANG_PP_EMBED
         7.  D_ENV_CPP_FEATURE_LANG_TEMPLATE_PARAMETERS
         8.  D_ENV_CPP_FEATURE_LANG_TRIVIAL_RELOCATABILITY
         9.  D_ENV_CPP_FEATURE_LANG_TRIVIAL_UNION
         10. D_ENV_CPP_FEATURE_LANG_VARIADIC_FRIEND
2.  STANDARD LIBRARY FEATURES
    -------------------------
    1.  C++14 library features
         1.  D_ENV_CPP_FEATURE_STL_CHRONO_UDLS
         2.  D_ENV_CPP_FEATURE_STL_COMPLEX_UDLS
    2.  C++17 library features
         1.  D_ENV_CPP_FEATURE_STL_ADDRESSOF_CONSTEXPR
         2.  D_ENV_CPP_FEATURE_STL_ANY
         3.  D_ENV_CPP_FEATURE_STL_APPLY
         4.  D_ENV_CPP_FEATURE_STL_ARRAY_CONSTEXPR
         5.  D_ENV_CPP_FEATURE_STL_AS_CONST
         6.  D_ENV_CPP_FEATURE_STL_BOOL_CONSTANT
         7.  D_ENV_CPP_FEATURE_STL_BOYER_MOORE_SEARCHER
         8.  D_ENV_CPP_FEATURE_STL_BYTE
         9.  D_ENV_CPP_FEATURE_STL_CLAMP
         10. D_ENV_CPP_FEATURE_STL_FILESYSTEM
         11. D_ENV_CPP_FEATURE_STL_OPTIONAL
         12. D_ENV_CPP_FEATURE_STL_VARIANT
    3.  C++20 library features
         1.  D_ENV_CPP_FEATURE_STL_ASSUME_ALIGNED
         2.  D_ENV_CPP_FEATURE_STL_ATOMIC_FLAG_TEST
         3.  D_ENV_CPP_FEATURE_STL_ATOMIC_FLOAT
         4.  D_ENV_CPP_FEATURE_STL_ATOMIC_REF
         5.  D_ENV_CPP_FEATURE_STL_ATOMIC_WAIT
         6.  D_ENV_CPP_FEATURE_STL_BARRIER
         7.  D_ENV_CPP_FEATURE_STL_BIND_FRONT
         8.  D_ENV_CPP_FEATURE_STL_BIT_CAST
         9.  D_ENV_CPP_FEATURE_STL_BITOPS
         10. D_ENV_CPP_FEATURE_STL_BOUNDED_ARRAY_TRAITS
         11. D_ENV_CPP_FEATURE_STL_CHAR8_T
         12. D_ENV_CPP_FEATURE_STL_CONCEPTS
         13. D_ENV_CPP_FEATURE_STL_CONSTEXPR_ALGORITHMS
         14. D_ENV_CPP_FEATURE_STL_CONSTEXPR_COMPLEX
         15. D_ENV_CPP_FEATURE_STL_CONSTEXPR_DYNAMIC_ALLOC
         16. D_ENV_CPP_FEATURE_STL_CONSTEXPR_STRING
         17. D_ENV_CPP_FEATURE_STL_CONSTEXPR_VECTOR
         18. D_ENV_CPP_FEATURE_STL_COROUTINE
         19. D_ENV_CPP_FEATURE_STL_ENDIAN
         20. D_ENV_CPP_FEATURE_STL_FORMAT
         21. D_ENV_CPP_FEATURE_STL_JTHREAD
         22. D_ENV_CPP_FEATURE_STL_LATCH
         23. D_ENV_CPP_FEATURE_STL_MATH_CONSTANTS
         24. D_ENV_CPP_FEATURE_STL_RANGES
         25. D_ENV_CPP_FEATURE_STL_SEMAPHORE
         26. D_ENV_CPP_FEATURE_STL_SOURCE_LOCATION
         27. D_ENV_CPP_FEATURE_STL_SPAN
         28. D_ENV_CPP_FEATURE_STL_THREE_WAY_COMPARISON
         29. D_ENV_CPP_FEATURE_STL_TO_ARRAY
         30. D_ENV_CPP_FEATURE_STL_IS_CONSTANT_EVALUATED
    4.  C++23 library features
         1.  D_ENV_CPP_FEATURE_STL_ADAPTOR_ITERATOR_PAIR_CONSTRUCTOR
         2.  D_ENV_CPP_FEATURE_STL_ASSOCIATIVE_HETEROGENEOUS_ERASURE
         3.  D_ENV_CPP_FEATURE_STL_BIND_BACK
         4.  D_ENV_CPP_FEATURE_STL_BYTESWAP
         5.  D_ENV_CPP_FEATURE_STL_CONSTEXPR_BITSET
         6.  D_ENV_CPP_FEATURE_STL_CONSTEXPR_CHARCONV
         7.  D_ENV_CPP_FEATURE_STL_CONSTEXPR_CMATH
         8.  D_ENV_CPP_FEATURE_STL_EXPECTED
         9.  D_ENV_CPP_FEATURE_STL_FLAT_MAP
         10. D_ENV_CPP_FEATURE_STL_FLAT_SET
         11. D_ENV_CPP_FEATURE_STL_GENERATOR
         12. D_ENV_CPP_FEATURE_STL_MDSPAN
         13. D_ENV_CPP_FEATURE_STL_MOVE_ONLY_FUNCTION
         14. D_ENV_CPP_FEATURE_STL_PRINT
         15. D_ENV_CPP_FEATURE_STL_RANGES_TO_CONTAINER
         16. D_ENV_CPP_FEATURE_STL_SPANSTREAM
         17. D_ENV_CPP_FEATURE_STL_STACKTRACE
         18. D_ENV_CPP_FEATURE_STL_STDATOMIC_H
         19. D_ENV_CPP_FEATURE_STL_STRING_CONTAINS
         20. D_ENV_CPP_FEATURE_STL_STRING_RESIZE_AND_OVERWRITE
         21. D_ENV_CPP_FEATURE_STL_UNREACHABLE
    5.  C++26 library features
         1.  D_ENV_CPP_FEATURE_STL_ALGORITHM_DEFAULT_VALUE_TYPE
         2.  D_ENV_CPP_FEATURE_STL_ASSOCIATIVE_HETEROGENEOUS_INSERTION
         3.  D_ENV_CPP_FEATURE_STL_ATOMIC_MIN_MAX
         4.  D_ENV_CPP_FEATURE_STL_CONSTEXPR_ATOMIC
         5.  D_ENV_CPP_FEATURE_STL_CONSTEXPR_DEQUE
         6.  D_ENV_CPP_FEATURE_STL_CONTRACTS
         7.  D_ENV_CPP_FEATURE_STL_COPYABLE_FUNCTION
         8.  D_ENV_CPP_FEATURE_STL_DEBUGGING
         9.  D_ENV_CPP_FEATURE_STL_FORMAT_PATH
         10. D_ENV_CPP_FEATURE_STL_FUNCTION_REF
         11. D_ENV_CPP_FEATURE_STL_HAZARD_POINTER
         12. D_ENV_CPP_FEATURE_STL_HIVE
         13. D_ENV_CPP_FEATURE_STL_INPLACE_VECTOR
         14. D_ENV_CPP_FEATURE_STL_LINALG
         15. D_ENV_CPP_FEATURE_STL_POLYMORPHIC
         16. D_ENV_CPP_FEATURE_STL_RCU
         17. D_ENV_CPP_FEATURE_STL_SATURATION_ARITHMETIC
         18. D_ENV_CPP_FEATURE_STL_SENDERS
         19. D_ENV_CPP_FEATURE_STL_SIMD
         20. D_ENV_CPP_FEATURE_STL_TEXT_ENCODING
3.  AGGREGATE FEATURE CHECKS
    ------------------------
    1.  C++11
         1.  D_ENV_CPP_FEATURE_HAS_ALL_LANG_CPP11
         2.  D_ENV_CPP_FEATURE_HAS_ALL_CPP11
    2.  C++14
         1.  D_ENV_CPP_FEATURE_HAS_ALL_LANG_CPP14
         2.  D_ENV_CPP_FEATURE_HAS_ALL_STL_CPP14
         3.  D_ENV_CPP_FEATURE_HAS_ALL_CPP14
    3.  C++17
         1.  D_ENV_CPP_FEATURE_HAS_ALL_LANG_CPP17
         2.  D_ENV_CPP_FEATURE_HAS_ALL_STL_CPP17
         3.  D_ENV_CPP_FEATURE_HAS_ALL_CPP17
    4.  C++20
         1.  D_ENV_CPP_FEATURE_HAS_ALL_LANG_CPP20
         2.  D_ENV_CPP_FEATURE_HAS_ALL_STL_CPP20
         3.  D_ENV_CPP_FEATURE_HAS_ALL_CPP20
    5.  C++23
         1.  D_ENV_CPP_FEATURE_HAS_ALL_LANG_CPP23
         2.  D_ENV_CPP_FEATURE_HAS_ALL_STL_CPP23
         3.  D_ENV_CPP_FEATURE_HAS_ALL_CPP23
    6.  C++26
         1.  D_ENV_CPP_FEATURE_HAS_ALL_LANG_CPP26
         2.  D_ENV_CPP_FEATURE_HAS_ALL_STL_CPP26
         3.  D_ENV_CPP_FEATURE_HAS_ALL_CPP26
*/

#ifndef DJINTERP_ENV_CPP_ENV_CPP_FEATURES_H
#define DJINTERP_ENV_CPP_ENV_CPP_FEATURES_H 1


//==============================================================================
// 1.  LANGUAGE FEATURES
//==============================================================================
// One item per feature-test macro, grouped by the standard that introduced
// it. Each item defines the feature's five macros; see the banner.


// 1.1    C++11 language features
//------------------------------------------------------------------------------
// 1.1.1
// D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES
//   feature: 1 if __cpp_alias_templates is defined, 0 otherwise.
#ifdef __cpp_alias_templates
    #define D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES     1
    #define D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES_VAL __cpp_alias_templates
#else
    #define D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES     0
    #define D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES_VAL 0L
#endif  // __cpp_alias_templates
#define D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES_NAME "__cpp_alias_templates"
#define D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES_DESC "Alias templates"
#define D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES_VERS "(C++11)"

// 1.1.2
// D_ENV_CPP_FEATURE_LANG_ATTRIBUTES
//   feature: 1 if __cpp_attributes is defined, 0 otherwise.
#ifdef __cpp_attributes
    #define D_ENV_CPP_FEATURE_LANG_ATTRIBUTES     1
    #define D_ENV_CPP_FEATURE_LANG_ATTRIBUTES_VAL __cpp_attributes
#else
    #define D_ENV_CPP_FEATURE_LANG_ATTRIBUTES     0
    #define D_ENV_CPP_FEATURE_LANG_ATTRIBUTES_VAL 0L
#endif  // __cpp_attributes
#define D_ENV_CPP_FEATURE_LANG_ATTRIBUTES_NAME "__cpp_attributes"
#define D_ENV_CPP_FEATURE_LANG_ATTRIBUTES_DESC "Attributes"
#define D_ENV_CPP_FEATURE_LANG_ATTRIBUTES_VERS "(C++11)"

// 1.1.3
// D_ENV_CPP_FEATURE_LANG_CONSTEXPR
//   feature: 1 if __cpp_constexpr is defined, 0 otherwise.
#ifdef __cpp_constexpr
    #define D_ENV_CPP_FEATURE_LANG_CONSTEXPR     1
    #define D_ENV_CPP_FEATURE_LANG_CONSTEXPR_VAL __cpp_constexpr
#else
    #define D_ENV_CPP_FEATURE_LANG_CONSTEXPR     0
    #define D_ENV_CPP_FEATURE_LANG_CONSTEXPR_VAL 0L
#endif  // __cpp_constexpr
#define D_ENV_CPP_FEATURE_LANG_CONSTEXPR_NAME "__cpp_constexpr"
#define D_ENV_CPP_FEATURE_LANG_CONSTEXPR_DESC "constexpr"
#define D_ENV_CPP_FEATURE_LANG_CONSTEXPR_VERS "(C++11)"

// 1.1.4
// D_ENV_CPP_FEATURE_LANG_DECLTYPE
//   feature: 1 if __cpp_decltype is defined, 0 otherwise.
#ifdef __cpp_decltype
    #define D_ENV_CPP_FEATURE_LANG_DECLTYPE     1
    #define D_ENV_CPP_FEATURE_LANG_DECLTYPE_VAL __cpp_decltype
#else
    #define D_ENV_CPP_FEATURE_LANG_DECLTYPE     0
    #define D_ENV_CPP_FEATURE_LANG_DECLTYPE_VAL 0L
#endif  // __cpp_decltype
#define D_ENV_CPP_FEATURE_LANG_DECLTYPE_NAME "__cpp_decltype"
#define D_ENV_CPP_FEATURE_LANG_DECLTYPE_DESC "decltype"
#define D_ENV_CPP_FEATURE_LANG_DECLTYPE_VERS "(C++11)"

// 1.1.5
// D_ENV_CPP_FEATURE_LANG_DELEGATING_CONSTRUCTORS
//   feature: 1 if __cpp_delegating_constructors is defined, 0 otherwise.
#ifdef __cpp_delegating_constructors
    #define D_ENV_CPP_FEATURE_LANG_DELEGATING_CONSTRUCTORS 1
    #define D_ENV_CPP_FEATURE_LANG_DELEGATING_CONSTRUCTORS_VAL                 \
        __cpp_delegating_constructors
#else
    #define D_ENV_CPP_FEATURE_LANG_DELEGATING_CONSTRUCTORS 0
    #define D_ENV_CPP_FEATURE_LANG_DELEGATING_CONSTRUCTORS_VAL 0L
#endif  // __cpp_delegating_constructors
#define D_ENV_CPP_FEATURE_LANG_DELEGATING_CONSTRUCTORS_NAME                    \
    "__cpp_delegating_constructors"
#define D_ENV_CPP_FEATURE_LANG_DELEGATING_CONSTRUCTORS_DESC                    \
    "Delegating constructors"
#define D_ENV_CPP_FEATURE_LANG_DELEGATING_CONSTRUCTORS_VERS "(C++11)"

// 1.1.6
// D_ENV_CPP_FEATURE_LANG_INHERITING_CONSTRUCTORS
//   feature: 1 if __cpp_inheriting_constructors is defined, 0 otherwise.
#ifdef __cpp_inheriting_constructors
    #define D_ENV_CPP_FEATURE_LANG_INHERITING_CONSTRUCTORS 1
    #define D_ENV_CPP_FEATURE_LANG_INHERITING_CONSTRUCTORS_VAL                 \
        __cpp_inheriting_constructors
#else
    #define D_ENV_CPP_FEATURE_LANG_INHERITING_CONSTRUCTORS 0
    #define D_ENV_CPP_FEATURE_LANG_INHERITING_CONSTRUCTORS_VAL 0L
#endif  // __cpp_inheriting_constructors
#define D_ENV_CPP_FEATURE_LANG_INHERITING_CONSTRUCTORS_NAME                    \
    "__cpp_inheriting_constructors"
#define D_ENV_CPP_FEATURE_LANG_INHERITING_CONSTRUCTORS_DESC                    \
    "Inheriting constructors"
#define D_ENV_CPP_FEATURE_LANG_INHERITING_CONSTRUCTORS_VERS "(C++11)"

// 1.1.7
// D_ENV_CPP_FEATURE_LANG_INITIALIZER_LISTS
//   feature: 1 if __cpp_initializer_lists is defined, 0 otherwise.
#ifdef __cpp_initializer_lists
    #define D_ENV_CPP_FEATURE_LANG_INITIALIZER_LISTS     1
    #define D_ENV_CPP_FEATURE_LANG_INITIALIZER_LISTS_VAL __cpp_initializer_lists
#else
    #define D_ENV_CPP_FEATURE_LANG_INITIALIZER_LISTS     0
    #define D_ENV_CPP_FEATURE_LANG_INITIALIZER_LISTS_VAL 0L
#endif  // __cpp_initializer_lists
#define D_ENV_CPP_FEATURE_LANG_INITIALIZER_LISTS_NAME "__cpp_initializer_lists"
#define D_ENV_CPP_FEATURE_LANG_INITIALIZER_LISTS_DESC                          \
    "List-initialization and std::initializer_list"
#define D_ENV_CPP_FEATURE_LANG_INITIALIZER_LISTS_VERS "(C++11)"

// 1.1.8
// D_ENV_CPP_FEATURE_LANG_LAMBDAS
//   feature: 1 if __cpp_lambdas is defined, 0 otherwise.
#ifdef __cpp_lambdas
    #define D_ENV_CPP_FEATURE_LANG_LAMBDAS     1
    #define D_ENV_CPP_FEATURE_LANG_LAMBDAS_VAL __cpp_lambdas
#else
    #define D_ENV_CPP_FEATURE_LANG_LAMBDAS     0
    #define D_ENV_CPP_FEATURE_LANG_LAMBDAS_VAL 0L
#endif  // __cpp_lambdas
#define D_ENV_CPP_FEATURE_LANG_LAMBDAS_NAME "__cpp_lambdas"
#define D_ENV_CPP_FEATURE_LANG_LAMBDAS_DESC "Lambda expressions"
#define D_ENV_CPP_FEATURE_LANG_LAMBDAS_VERS "(C++11)"

// 1.1.9
// D_ENV_CPP_FEATURE_LANG_NSDMI
//   feature: 1 if __cpp_nsdmi is defined, 0 otherwise.
#ifdef __cpp_nsdmi
    #define D_ENV_CPP_FEATURE_LANG_NSDMI     1
    #define D_ENV_CPP_FEATURE_LANG_NSDMI_VAL __cpp_nsdmi
#else
    #define D_ENV_CPP_FEATURE_LANG_NSDMI     0
    #define D_ENV_CPP_FEATURE_LANG_NSDMI_VAL 0L
#endif  // __cpp_nsdmi
#define D_ENV_CPP_FEATURE_LANG_NSDMI_NAME "__cpp_nsdmi"
#define D_ENV_CPP_FEATURE_LANG_NSDMI_DESC "Non-static data member initializers"
#define D_ENV_CPP_FEATURE_LANG_NSDMI_VERS "(C++11)"

// 1.1.10
// D_ENV_CPP_FEATURE_LANG_RANGE_BASED_FOR
//   feature: 1 if __cpp_range_based_for is defined, 0 otherwise.
#ifdef __cpp_range_based_for
    #define D_ENV_CPP_FEATURE_LANG_RANGE_BASED_FOR     1
    #define D_ENV_CPP_FEATURE_LANG_RANGE_BASED_FOR_VAL __cpp_range_based_for
#else
    #define D_ENV_CPP_FEATURE_LANG_RANGE_BASED_FOR     0
    #define D_ENV_CPP_FEATURE_LANG_RANGE_BASED_FOR_VAL 0L
#endif  // __cpp_range_based_for
#define D_ENV_CPP_FEATURE_LANG_RANGE_BASED_FOR_NAME "__cpp_range_based_for"
#define D_ENV_CPP_FEATURE_LANG_RANGE_BASED_FOR_DESC "Range-based for loop"
#define D_ENV_CPP_FEATURE_LANG_RANGE_BASED_FOR_VERS "(C++11)"

// 1.1.11
// D_ENV_CPP_FEATURE_LANG_RAW_STRINGS
//   feature: 1 if __cpp_raw_strings is defined, 0 otherwise.
#ifdef __cpp_raw_strings
    #define D_ENV_CPP_FEATURE_LANG_RAW_STRINGS     1
    #define D_ENV_CPP_FEATURE_LANG_RAW_STRINGS_VAL __cpp_raw_strings
#else
    #define D_ENV_CPP_FEATURE_LANG_RAW_STRINGS     0
    #define D_ENV_CPP_FEATURE_LANG_RAW_STRINGS_VAL 0L
#endif  // __cpp_raw_strings
#define D_ENV_CPP_FEATURE_LANG_RAW_STRINGS_NAME "__cpp_raw_strings"
#define D_ENV_CPP_FEATURE_LANG_RAW_STRINGS_DESC "Raw string literals"
#define D_ENV_CPP_FEATURE_LANG_RAW_STRINGS_VERS "(C++11)"

// 1.1.12
// D_ENV_CPP_FEATURE_LANG_REF_QUALIFIERS
//   feature: 1 if __cpp_ref_qualifiers is defined, 0 otherwise.
#ifdef __cpp_ref_qualifiers
    #define D_ENV_CPP_FEATURE_LANG_REF_QUALIFIERS     1
    #define D_ENV_CPP_FEATURE_LANG_REF_QUALIFIERS_VAL __cpp_ref_qualifiers
#else
    #define D_ENV_CPP_FEATURE_LANG_REF_QUALIFIERS     0
    #define D_ENV_CPP_FEATURE_LANG_REF_QUALIFIERS_VAL 0L
#endif  // __cpp_ref_qualifiers
#define D_ENV_CPP_FEATURE_LANG_REF_QUALIFIERS_NAME "__cpp_ref_qualifiers"
#define D_ENV_CPP_FEATURE_LANG_REF_QUALIFIERS_DESC "ref-qualifiers"
#define D_ENV_CPP_FEATURE_LANG_REF_QUALIFIERS_VERS "(C++11)"

// 1.1.13
// D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES
//   feature: 1 if __cpp_rvalue_references is defined, 0 otherwise.
#ifdef __cpp_rvalue_references
    #define D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES     1
    #define D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES_VAL __cpp_rvalue_references
#else
    #define D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES     0
    #define D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES_VAL 0L
#endif  // __cpp_rvalue_references
#define D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES_NAME "__cpp_rvalue_references"
#define D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES_DESC "Rvalue reference"
#define D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES_VERS "(C++11)"

// 1.1.14
// D_ENV_CPP_FEATURE_LANG_STATIC_ASSERT
//   feature: 1 if __cpp_static_assert is defined, 0 otherwise.
#ifdef __cpp_static_assert
    #define D_ENV_CPP_FEATURE_LANG_STATIC_ASSERT     1
    #define D_ENV_CPP_FEATURE_LANG_STATIC_ASSERT_VAL __cpp_static_assert
#else
    #define D_ENV_CPP_FEATURE_LANG_STATIC_ASSERT     0
    #define D_ENV_CPP_FEATURE_LANG_STATIC_ASSERT_VAL 0L
#endif  // __cpp_static_assert
#define D_ENV_CPP_FEATURE_LANG_STATIC_ASSERT_NAME "__cpp_static_assert"
#define D_ENV_CPP_FEATURE_LANG_STATIC_ASSERT_DESC "static_assert"
#define D_ENV_CPP_FEATURE_LANG_STATIC_ASSERT_VERS "(C++11)"

// 1.1.15
// D_ENV_CPP_FEATURE_LANG_THREADSAFE_STATIC_INIT
//   feature: 1 if __cpp_threadsafe_static_init is defined, 0 otherwise.
#ifdef __cpp_threadsafe_static_init
    #define D_ENV_CPP_FEATURE_LANG_THREADSAFE_STATIC_INIT 1
    #define D_ENV_CPP_FEATURE_LANG_THREADSAFE_STATIC_INIT_VAL                  \
        __cpp_threadsafe_static_init
#else
    #define D_ENV_CPP_FEATURE_LANG_THREADSAFE_STATIC_INIT 0
    #define D_ENV_CPP_FEATURE_LANG_THREADSAFE_STATIC_INIT_VAL 0L
#endif  // __cpp_threadsafe_static_init
#define D_ENV_CPP_FEATURE_LANG_THREADSAFE_STATIC_INIT_NAME                     \
    "__cpp_threadsafe_static_init"
#define D_ENV_CPP_FEATURE_LANG_THREADSAFE_STATIC_INIT_DESC                     \
    "Dynamic initialization and destruction with concurrency"
#define D_ENV_CPP_FEATURE_LANG_THREADSAFE_STATIC_INIT_VERS "(C++11)"

// 1.1.16
// D_ENV_CPP_FEATURE_LANG_UNICODE_CHARACTERS
//   feature: 1 if __cpp_unicode_characters is defined, 0 otherwise.
#ifdef __cpp_unicode_characters
    #define D_ENV_CPP_FEATURE_LANG_UNICODE_CHARACTERS 1
    #define D_ENV_CPP_FEATURE_LANG_UNICODE_CHARACTERS_VAL                      \
        __cpp_unicode_characters
#else
    #define D_ENV_CPP_FEATURE_LANG_UNICODE_CHARACTERS 0
    #define D_ENV_CPP_FEATURE_LANG_UNICODE_CHARACTERS_VAL 0L
#endif  // __cpp_unicode_characters
#define D_ENV_CPP_FEATURE_LANG_UNICODE_CHARACTERS_NAME                         \
    "__cpp_unicode_characters"
#define D_ENV_CPP_FEATURE_LANG_UNICODE_CHARACTERS_DESC                         \
    "New character types (char16_t and char32_t)"
#define D_ENV_CPP_FEATURE_LANG_UNICODE_CHARACTERS_VERS "(C++11)"

// 1.1.17
// D_ENV_CPP_FEATURE_LANG_UNICODE_LITERALS
//   feature: 1 if __cpp_unicode_literals is defined, 0 otherwise.
#ifdef __cpp_unicode_literals
    #define D_ENV_CPP_FEATURE_LANG_UNICODE_LITERALS     1
    #define D_ENV_CPP_FEATURE_LANG_UNICODE_LITERALS_VAL __cpp_unicode_literals
#else
    #define D_ENV_CPP_FEATURE_LANG_UNICODE_LITERALS     0
    #define D_ENV_CPP_FEATURE_LANG_UNICODE_LITERALS_VAL 0L
#endif  // __cpp_unicode_literals
#define D_ENV_CPP_FEATURE_LANG_UNICODE_LITERALS_NAME "__cpp_unicode_literals"
#define D_ENV_CPP_FEATURE_LANG_UNICODE_LITERALS_DESC "Unicode string literals"
#define D_ENV_CPP_FEATURE_LANG_UNICODE_LITERALS_VERS "(C++11)"

// 1.1.18
// D_ENV_CPP_FEATURE_LANG_USER_DEFINED_LITERALS
//   feature: 1 if __cpp_user_defined_literals is defined, 0 otherwise.
#ifdef __cpp_user_defined_literals
    #define D_ENV_CPP_FEATURE_LANG_USER_DEFINED_LITERALS 1
    #define D_ENV_CPP_FEATURE_LANG_USER_DEFINED_LITERALS_VAL                   \
        __cpp_user_defined_literals
#else
    #define D_ENV_CPP_FEATURE_LANG_USER_DEFINED_LITERALS 0
    #define D_ENV_CPP_FEATURE_LANG_USER_DEFINED_LITERALS_VAL 0L
#endif  // __cpp_user_defined_literals
#define D_ENV_CPP_FEATURE_LANG_USER_DEFINED_LITERALS_NAME                      \
    "__cpp_user_defined_literals"
#define D_ENV_CPP_FEATURE_LANG_USER_DEFINED_LITERALS_DESC                      \
    "User-defined literals"
#define D_ENV_CPP_FEATURE_LANG_USER_DEFINED_LITERALS_VERS "(C++11)"

// 1.1.19
// D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES
//   feature: 1 if __cpp_variadic_templates is defined, 0 otherwise.
#ifdef __cpp_variadic_templates
    #define D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES 1
    #define D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES_VAL                      \
        __cpp_variadic_templates
#else
    #define D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES 0
    #define D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES_VAL 0L
#endif  // __cpp_variadic_templates
#define D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES_NAME                         \
    "__cpp_variadic_templates"
#define D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES_DESC "Variadic templates"
#define D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES_VERS "(C++11)"

// 1.2    C++14 language features
//------------------------------------------------------------------------------
// 1.2.1
// D_ENV_CPP_FEATURE_LANG_AGGREGATE_NSDMI
//   feature: 1 if __cpp_aggregate_nsdmi is defined, 0 otherwise.
#ifdef __cpp_aggregate_nsdmi
    #define D_ENV_CPP_FEATURE_LANG_AGGREGATE_NSDMI     1
    #define D_ENV_CPP_FEATURE_LANG_AGGREGATE_NSDMI_VAL __cpp_aggregate_nsdmi
#else
    #define D_ENV_CPP_FEATURE_LANG_AGGREGATE_NSDMI     0
    #define D_ENV_CPP_FEATURE_LANG_AGGREGATE_NSDMI_VAL 0L
#endif  // __cpp_aggregate_nsdmi
#define D_ENV_CPP_FEATURE_LANG_AGGREGATE_NSDMI_NAME "__cpp_aggregate_nsdmi"
#define D_ENV_CPP_FEATURE_LANG_AGGREGATE_NSDMI_DESC                            \
    "Aggregate classes with default member initializers"
#define D_ENV_CPP_FEATURE_LANG_AGGREGATE_NSDMI_VERS "(C++14)"

// 1.2.2
// D_ENV_CPP_FEATURE_LANG_BINARY_LITERALS
//   feature: 1 if __cpp_binary_literals is defined, 0 otherwise.
#ifdef __cpp_binary_literals
    #define D_ENV_CPP_FEATURE_LANG_BINARY_LITERALS     1
    #define D_ENV_CPP_FEATURE_LANG_BINARY_LITERALS_VAL __cpp_binary_literals
#else
    #define D_ENV_CPP_FEATURE_LANG_BINARY_LITERALS     0
    #define D_ENV_CPP_FEATURE_LANG_BINARY_LITERALS_VAL 0L
#endif  // __cpp_binary_literals
#define D_ENV_CPP_FEATURE_LANG_BINARY_LITERALS_NAME "__cpp_binary_literals"
#define D_ENV_CPP_FEATURE_LANG_BINARY_LITERALS_DESC "Binary literals"
#define D_ENV_CPP_FEATURE_LANG_BINARY_LITERALS_VERS "(C++14)"

// 1.2.3
// D_ENV_CPP_FEATURE_LANG_DECLTYPE_AUTO
//   feature: 1 if __cpp_decltype_auto is defined, 0 otherwise.
#ifdef __cpp_decltype_auto
    #define D_ENV_CPP_FEATURE_LANG_DECLTYPE_AUTO     1
    #define D_ENV_CPP_FEATURE_LANG_DECLTYPE_AUTO_VAL __cpp_decltype_auto
#else
    #define D_ENV_CPP_FEATURE_LANG_DECLTYPE_AUTO     0
    #define D_ENV_CPP_FEATURE_LANG_DECLTYPE_AUTO_VAL 0L
#endif  // __cpp_decltype_auto
#define D_ENV_CPP_FEATURE_LANG_DECLTYPE_AUTO_NAME "__cpp_decltype_auto"
#define D_ENV_CPP_FEATURE_LANG_DECLTYPE_AUTO_DESC                              \
    "Return type deduction for normal functions"
#define D_ENV_CPP_FEATURE_LANG_DECLTYPE_AUTO_VERS "(C++14)"

// 1.2.4
// D_ENV_CPP_FEATURE_LANG_ENUMERATOR_ATTRIBUTES
//   feature: 1 if __cpp_enumerator_attributes is defined, 0 otherwise.
#ifdef __cpp_enumerator_attributes
    #define D_ENV_CPP_FEATURE_LANG_ENUMERATOR_ATTRIBUTES 1
    #define D_ENV_CPP_FEATURE_LANG_ENUMERATOR_ATTRIBUTES_VAL                   \
        __cpp_enumerator_attributes
#else
    #define D_ENV_CPP_FEATURE_LANG_ENUMERATOR_ATTRIBUTES 0
    #define D_ENV_CPP_FEATURE_LANG_ENUMERATOR_ATTRIBUTES_VAL 0L
#endif  // __cpp_enumerator_attributes
#define D_ENV_CPP_FEATURE_LANG_ENUMERATOR_ATTRIBUTES_NAME                      \
    "__cpp_enumerator_attributes"
#define D_ENV_CPP_FEATURE_LANG_ENUMERATOR_ATTRIBUTES_DESC                      \
    "Attributes for enumerators"
#define D_ENV_CPP_FEATURE_LANG_ENUMERATOR_ATTRIBUTES_VERS "(C++14)"

// 1.2.5
// D_ENV_CPP_FEATURE_LANG_GENERIC_LAMBDAS
//   feature: 1 if __cpp_generic_lambdas is defined, 0 otherwise.
#ifdef __cpp_generic_lambdas
    #define D_ENV_CPP_FEATURE_LANG_GENERIC_LAMBDAS     1
    #define D_ENV_CPP_FEATURE_LANG_GENERIC_LAMBDAS_VAL __cpp_generic_lambdas
#else
    #define D_ENV_CPP_FEATURE_LANG_GENERIC_LAMBDAS     0
    #define D_ENV_CPP_FEATURE_LANG_GENERIC_LAMBDAS_VAL 0L
#endif  // __cpp_generic_lambdas
#define D_ENV_CPP_FEATURE_LANG_GENERIC_LAMBDAS_NAME "__cpp_generic_lambdas"
#define D_ENV_CPP_FEATURE_LANG_GENERIC_LAMBDAS_DESC "Generic lambda expressions"
#define D_ENV_CPP_FEATURE_LANG_GENERIC_LAMBDAS_VERS "(C++14)"

// 1.2.6
// D_ENV_CPP_FEATURE_LANG_INIT_CAPTURES
//   feature: 1 if __cpp_init_captures is defined, 0 otherwise.
#ifdef __cpp_init_captures
    #define D_ENV_CPP_FEATURE_LANG_INIT_CAPTURES     1
    #define D_ENV_CPP_FEATURE_LANG_INIT_CAPTURES_VAL __cpp_init_captures
#else
    #define D_ENV_CPP_FEATURE_LANG_INIT_CAPTURES     0
    #define D_ENV_CPP_FEATURE_LANG_INIT_CAPTURES_VAL 0L
#endif  // __cpp_init_captures
#define D_ENV_CPP_FEATURE_LANG_INIT_CAPTURES_NAME "__cpp_init_captures"
#define D_ENV_CPP_FEATURE_LANG_INIT_CAPTURES_DESC "Lambda init-capture"
#define D_ENV_CPP_FEATURE_LANG_INIT_CAPTURES_VERS "(C++14)"

// 1.2.7
// D_ENV_CPP_FEATURE_LANG_NAMESPACE_ATTRIBUTES
//   feature: 1 if __cpp_namespace_attributes is defined, 0 otherwise.
#ifdef __cpp_namespace_attributes
    #define D_ENV_CPP_FEATURE_LANG_NAMESPACE_ATTRIBUTES 1
    #define D_ENV_CPP_FEATURE_LANG_NAMESPACE_ATTRIBUTES_VAL                    \
        __cpp_namespace_attributes
#else
    #define D_ENV_CPP_FEATURE_LANG_NAMESPACE_ATTRIBUTES 0
    #define D_ENV_CPP_FEATURE_LANG_NAMESPACE_ATTRIBUTES_VAL 0L
#endif  // __cpp_namespace_attributes
#define D_ENV_CPP_FEATURE_LANG_NAMESPACE_ATTRIBUTES_NAME                       \
    "__cpp_namespace_attributes"
#define D_ENV_CPP_FEATURE_LANG_NAMESPACE_ATTRIBUTES_DESC                       \
    "Attributes for namespaces"
#define D_ENV_CPP_FEATURE_LANG_NAMESPACE_ATTRIBUTES_VERS "(C++14)"

// 1.2.8
// D_ENV_CPP_FEATURE_LANG_NONTYPE_TEMPLATE_ARGS
//   feature: 1 if __cpp_nontype_template_args is defined, 0 otherwise.
#ifdef __cpp_nontype_template_args
    #define D_ENV_CPP_FEATURE_LANG_NONTYPE_TEMPLATE_ARGS 1
    #define D_ENV_CPP_FEATURE_LANG_NONTYPE_TEMPLATE_ARGS_VAL                   \
        __cpp_nontype_template_args
#else
    #define D_ENV_CPP_FEATURE_LANG_NONTYPE_TEMPLATE_ARGS 0
    #define D_ENV_CPP_FEATURE_LANG_NONTYPE_TEMPLATE_ARGS_VAL 0L
#endif  // __cpp_nontype_template_args
#define D_ENV_CPP_FEATURE_LANG_NONTYPE_TEMPLATE_ARGS_NAME                      \
    "__cpp_nontype_template_args"
#define D_ENV_CPP_FEATURE_LANG_NONTYPE_TEMPLATE_ARGS_DESC                      \
    "Allow constant evaluation for all constant template arguments"
#define D_ENV_CPP_FEATURE_LANG_NONTYPE_TEMPLATE_ARGS_VERS "(C++14)"

// 1.2.9
// D_ENV_CPP_FEATURE_LANG_RETURN_TYPE_DEDUCTION
//   feature: 1 if __cpp_return_type_deduction is defined, 0 otherwise.
#ifdef __cpp_return_type_deduction
    #define D_ENV_CPP_FEATURE_LANG_RETURN_TYPE_DEDUCTION 1
    #define D_ENV_CPP_FEATURE_LANG_RETURN_TYPE_DEDUCTION_VAL                   \
        __cpp_return_type_deduction
#else
    #define D_ENV_CPP_FEATURE_LANG_RETURN_TYPE_DEDUCTION 0
    #define D_ENV_CPP_FEATURE_LANG_RETURN_TYPE_DEDUCTION_VAL 0L
#endif  // __cpp_return_type_deduction
#define D_ENV_CPP_FEATURE_LANG_RETURN_TYPE_DEDUCTION_NAME                      \
    "__cpp_return_type_deduction"
#define D_ENV_CPP_FEATURE_LANG_RETURN_TYPE_DEDUCTION_DESC                      \
    "Return type deduction for normal functions"
#define D_ENV_CPP_FEATURE_LANG_RETURN_TYPE_DEDUCTION_VERS "(C++14)"

// 1.2.10
// D_ENV_CPP_FEATURE_LANG_SIZED_DEALLOCATION
//   feature: 1 if __cpp_sized_deallocation is defined, 0 otherwise.
#ifdef __cpp_sized_deallocation
    #define D_ENV_CPP_FEATURE_LANG_SIZED_DEALLOCATION 1
    #define D_ENV_CPP_FEATURE_LANG_SIZED_DEALLOCATION_VAL                      \
        __cpp_sized_deallocation
#else
    #define D_ENV_CPP_FEATURE_LANG_SIZED_DEALLOCATION 0
    #define D_ENV_CPP_FEATURE_LANG_SIZED_DEALLOCATION_VAL 0L
#endif  // __cpp_sized_deallocation
#define D_ENV_CPP_FEATURE_LANG_SIZED_DEALLOCATION_NAME                         \
    "__cpp_sized_deallocation"
#define D_ENV_CPP_FEATURE_LANG_SIZED_DEALLOCATION_DESC "Sized deallocation"
#define D_ENV_CPP_FEATURE_LANG_SIZED_DEALLOCATION_VERS "(C++14)"

// 1.2.11
// D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
//   feature: 1 if __cpp_variable_templates is defined, 0 otherwise.
#ifdef __cpp_variable_templates
    #define D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES 1
    #define D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES_VAL                      \
        __cpp_variable_templates
#else
    #define D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES 0
    #define D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES_VAL 0L
#endif  // __cpp_variable_templates
#define D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES_NAME                         \
    "__cpp_variable_templates"
#define D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES_DESC "Variable templates"
#define D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES_VERS "(C++14)"

// 1.3    C++17 language features
//------------------------------------------------------------------------------
// 1.3.1
// D_ENV_CPP_FEATURE_LANG_AGGREGATE_BASES
//   feature: 1 if __cpp_aggregate_bases is defined, 0 otherwise.
#ifdef __cpp_aggregate_bases
    #define D_ENV_CPP_FEATURE_LANG_AGGREGATE_BASES     1
    #define D_ENV_CPP_FEATURE_LANG_AGGREGATE_BASES_VAL __cpp_aggregate_bases
#else
    #define D_ENV_CPP_FEATURE_LANG_AGGREGATE_BASES     0
    #define D_ENV_CPP_FEATURE_LANG_AGGREGATE_BASES_VAL 0L
#endif  // __cpp_aggregate_bases
#define D_ENV_CPP_FEATURE_LANG_AGGREGATE_BASES_NAME "__cpp_aggregate_bases"
#define D_ENV_CPP_FEATURE_LANG_AGGREGATE_BASES_DESC                            \
    "Aggregate classes with base classes"
#define D_ENV_CPP_FEATURE_LANG_AGGREGATE_BASES_VERS "(C++17)"

// 1.3.2
// D_ENV_CPP_FEATURE_LANG_ALIGNED_NEW
//   feature: 1 if __cpp_aligned_new is defined, 0 otherwise.
#ifdef __cpp_aligned_new
    #define D_ENV_CPP_FEATURE_LANG_ALIGNED_NEW     1
    #define D_ENV_CPP_FEATURE_LANG_ALIGNED_NEW_VAL __cpp_aligned_new
#else
    #define D_ENV_CPP_FEATURE_LANG_ALIGNED_NEW     0
    #define D_ENV_CPP_FEATURE_LANG_ALIGNED_NEW_VAL 0L
#endif  // __cpp_aligned_new
#define D_ENV_CPP_FEATURE_LANG_ALIGNED_NEW_NAME "__cpp_aligned_new"
#define D_ENV_CPP_FEATURE_LANG_ALIGNED_NEW_DESC                                \
    "Dynamic memory allocation for over-aligned data"
#define D_ENV_CPP_FEATURE_LANG_ALIGNED_NEW_VERS "(C++17)"

// 1.3.3
// D_ENV_CPP_FEATURE_LANG_CAPTURE_STAR_THIS
//   feature: 1 if __cpp_capture_star_this is defined, 0 otherwise.
#ifdef __cpp_capture_star_this
    #define D_ENV_CPP_FEATURE_LANG_CAPTURE_STAR_THIS     1
    #define D_ENV_CPP_FEATURE_LANG_CAPTURE_STAR_THIS_VAL __cpp_capture_star_this
#else
    #define D_ENV_CPP_FEATURE_LANG_CAPTURE_STAR_THIS     0
    #define D_ENV_CPP_FEATURE_LANG_CAPTURE_STAR_THIS_VAL 0L
#endif  // __cpp_capture_star_this
#define D_ENV_CPP_FEATURE_LANG_CAPTURE_STAR_THIS_NAME "__cpp_capture_star_this"
#define D_ENV_CPP_FEATURE_LANG_CAPTURE_STAR_THIS_DESC                          \
    "Lambda capture of *this by value as [=,*this]"
#define D_ENV_CPP_FEATURE_LANG_CAPTURE_STAR_THIS_VERS "(C++17)"

// 1.3.4
// D_ENV_CPP_FEATURE_LANG_CONSTEXPR_IN_DECLTYPE
//   feature: 1 if __cpp_constexpr_in_decltype is defined, 0 otherwise.
#ifdef __cpp_constexpr_in_decltype
    #define D_ENV_CPP_FEATURE_LANG_CONSTEXPR_IN_DECLTYPE 1
    #define D_ENV_CPP_FEATURE_LANG_CONSTEXPR_IN_DECLTYPE_VAL                   \
        __cpp_constexpr_in_decltype
#else
    #define D_ENV_CPP_FEATURE_LANG_CONSTEXPR_IN_DECLTYPE 0
    #define D_ENV_CPP_FEATURE_LANG_CONSTEXPR_IN_DECLTYPE_VAL 0L
#endif  // __cpp_constexpr_in_decltype
#define D_ENV_CPP_FEATURE_LANG_CONSTEXPR_IN_DECLTYPE_NAME                      \
    "__cpp_constexpr_in_decltype"
#define D_ENV_CPP_FEATURE_LANG_CONSTEXPR_IN_DECLTYPE_DESC                      \
    "Generation of function and variable definitions when needed for constant evaluation"
#define D_ENV_CPP_FEATURE_LANG_CONSTEXPR_IN_DECLTYPE_VERS "(C++17)"

// 1.3.5
// D_ENV_CPP_FEATURE_LANG_DEDUCTION_GUIDES
//   feature: 1 if __cpp_deduction_guides is defined, 0 otherwise.
#ifdef __cpp_deduction_guides
    #define D_ENV_CPP_FEATURE_LANG_DEDUCTION_GUIDES     1
    #define D_ENV_CPP_FEATURE_LANG_DEDUCTION_GUIDES_VAL __cpp_deduction_guides
#else
    #define D_ENV_CPP_FEATURE_LANG_DEDUCTION_GUIDES     0
    #define D_ENV_CPP_FEATURE_LANG_DEDUCTION_GUIDES_VAL 0L
#endif  // __cpp_deduction_guides
#define D_ENV_CPP_FEATURE_LANG_DEDUCTION_GUIDES_NAME "__cpp_deduction_guides"
#define D_ENV_CPP_FEATURE_LANG_DEDUCTION_GUIDES_DESC                           \
    "Template argument deduction for class templates (CTAD)"
#define D_ENV_CPP_FEATURE_LANG_DEDUCTION_GUIDES_VERS "(C++17)"

// 1.3.6
// D_ENV_CPP_FEATURE_LANG_FOLD_EXPRESSIONS
//   feature: 1 if __cpp_fold_expressions is defined, 0 otherwise.
#ifdef __cpp_fold_expressions
    #define D_ENV_CPP_FEATURE_LANG_FOLD_EXPRESSIONS     1
    #define D_ENV_CPP_FEATURE_LANG_FOLD_EXPRESSIONS_VAL __cpp_fold_expressions
#else
    #define D_ENV_CPP_FEATURE_LANG_FOLD_EXPRESSIONS     0
    #define D_ENV_CPP_FEATURE_LANG_FOLD_EXPRESSIONS_VAL 0L
#endif  // __cpp_fold_expressions
#define D_ENV_CPP_FEATURE_LANG_FOLD_EXPRESSIONS_NAME "__cpp_fold_expressions"
#define D_ENV_CPP_FEATURE_LANG_FOLD_EXPRESSIONS_DESC "Fold expressions"
#define D_ENV_CPP_FEATURE_LANG_FOLD_EXPRESSIONS_VERS "(C++17)"

// 1.3.7
// D_ENV_CPP_FEATURE_LANG_GUARANTEED_COPY_ELISION
//   feature: 1 if __cpp_guaranteed_copy_elision is defined, 0 otherwise.
#ifdef __cpp_guaranteed_copy_elision
    #define D_ENV_CPP_FEATURE_LANG_GUARANTEED_COPY_ELISION 1
    #define D_ENV_CPP_FEATURE_LANG_GUARANTEED_COPY_ELISION_VAL                 \
        __cpp_guaranteed_copy_elision
#else
    #define D_ENV_CPP_FEATURE_LANG_GUARANTEED_COPY_ELISION 0
    #define D_ENV_CPP_FEATURE_LANG_GUARANTEED_COPY_ELISION_VAL 0L
#endif  // __cpp_guaranteed_copy_elision
#define D_ENV_CPP_FEATURE_LANG_GUARANTEED_COPY_ELISION_NAME                    \
    "__cpp_guaranteed_copy_elision"
#define D_ENV_CPP_FEATURE_LANG_GUARANTEED_COPY_ELISION_DESC                    \
    "Guaranteed copy elision through simplified value categories"
#define D_ENV_CPP_FEATURE_LANG_GUARANTEED_COPY_ELISION_VERS "(C++17)"

// 1.3.8
// D_ENV_CPP_FEATURE_LANG_HEX_FLOAT
//   feature: 1 if __cpp_hex_float is defined, 0 otherwise.
#ifdef __cpp_hex_float
    #define D_ENV_CPP_FEATURE_LANG_HEX_FLOAT     1
    #define D_ENV_CPP_FEATURE_LANG_HEX_FLOAT_VAL __cpp_hex_float
#else
    #define D_ENV_CPP_FEATURE_LANG_HEX_FLOAT     0
    #define D_ENV_CPP_FEATURE_LANG_HEX_FLOAT_VAL 0L
#endif  // __cpp_hex_float
#define D_ENV_CPP_FEATURE_LANG_HEX_FLOAT_NAME "__cpp_hex_float"
#define D_ENV_CPP_FEATURE_LANG_HEX_FLOAT_DESC "Hexadecimal floating literals"
#define D_ENV_CPP_FEATURE_LANG_HEX_FLOAT_VERS "(C++17)"

// 1.3.9
// D_ENV_CPP_FEATURE_LANG_IF_CONSTEXPR
//   feature: 1 if __cpp_if_constexpr is defined, 0 otherwise.
#ifdef __cpp_if_constexpr
    #define D_ENV_CPP_FEATURE_LANG_IF_CONSTEXPR     1
    #define D_ENV_CPP_FEATURE_LANG_IF_CONSTEXPR_VAL __cpp_if_constexpr
#else
    #define D_ENV_CPP_FEATURE_LANG_IF_CONSTEXPR     0
    #define D_ENV_CPP_FEATURE_LANG_IF_CONSTEXPR_VAL 0L
#endif  // __cpp_if_constexpr
#define D_ENV_CPP_FEATURE_LANG_IF_CONSTEXPR_NAME "__cpp_if_constexpr"
#define D_ENV_CPP_FEATURE_LANG_IF_CONSTEXPR_DESC "if constexpr"
#define D_ENV_CPP_FEATURE_LANG_IF_CONSTEXPR_VERS "(C++17)"

// 1.3.10
// D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
//   feature: 1 if __cpp_inline_variables is defined, 0 otherwise.
#ifdef __cpp_inline_variables
    #define D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES     1
    #define D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES_VAL __cpp_inline_variables
#else
    #define D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES     0
    #define D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES_VAL 0L
#endif  // __cpp_inline_variables
#define D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES_NAME "__cpp_inline_variables"
#define D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES_DESC "Inline variables"
#define D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES_VERS "(C++17)"

// 1.3.11
// D_ENV_CPP_FEATURE_LANG_NOEXCEPT_FUNCTION_TYPE
//   feature: 1 if __cpp_noexcept_function_type is defined, 0 otherwise.
#ifdef __cpp_noexcept_function_type
    #define D_ENV_CPP_FEATURE_LANG_NOEXCEPT_FUNCTION_TYPE 1
    #define D_ENV_CPP_FEATURE_LANG_NOEXCEPT_FUNCTION_TYPE_VAL                  \
        __cpp_noexcept_function_type
#else
    #define D_ENV_CPP_FEATURE_LANG_NOEXCEPT_FUNCTION_TYPE 0
    #define D_ENV_CPP_FEATURE_LANG_NOEXCEPT_FUNCTION_TYPE_VAL 0L
#endif  // __cpp_noexcept_function_type
#define D_ENV_CPP_FEATURE_LANG_NOEXCEPT_FUNCTION_TYPE_NAME                     \
    "__cpp_noexcept_function_type"
#define D_ENV_CPP_FEATURE_LANG_NOEXCEPT_FUNCTION_TYPE_DESC                     \
    "Make exception specifications be part of the type system"
#define D_ENV_CPP_FEATURE_LANG_NOEXCEPT_FUNCTION_TYPE_VERS "(C++17)"

// 1.3.12
// D_ENV_CPP_FEATURE_LANG_NONTYPE_TEMPLATE_PARAMETER_AUTO
//   feature: 1 if __cpp_nontype_template_parameter_auto is defined, 0
// otherwise.
#ifdef __cpp_nontype_template_parameter_auto
    #define D_ENV_CPP_FEATURE_LANG_NONTYPE_TEMPLATE_PARAMETER_AUTO 1
    #define D_ENV_CPP_FEATURE_LANG_NONTYPE_TEMPLATE_PARAMETER_AUTO_VAL         \
        __cpp_nontype_template_parameter_auto
#else
    #define D_ENV_CPP_FEATURE_LANG_NONTYPE_TEMPLATE_PARAMETER_AUTO 0
    #define D_ENV_CPP_FEATURE_LANG_NONTYPE_TEMPLATE_PARAMETER_AUTO_VAL 0L
#endif  // __cpp_nontype_template_parameter_auto
#define D_ENV_CPP_FEATURE_LANG_NONTYPE_TEMPLATE_PARAMETER_AUTO_NAME            \
    "__cpp_nontype_template_parameter_auto"
#define D_ENV_CPP_FEATURE_LANG_NONTYPE_TEMPLATE_PARAMETER_AUTO_DESC            \
    "Declaring constant template parameter with auto"
#define D_ENV_CPP_FEATURE_LANG_NONTYPE_TEMPLATE_PARAMETER_AUTO_VERS "(C++17)"

// 1.3.13
// D_ENV_CPP_FEATURE_LANG_STRUCTURED_BINDINGS
//   feature: 1 if __cpp_structured_bindings is defined, 0 otherwise.
#ifdef __cpp_structured_bindings
    #define D_ENV_CPP_FEATURE_LANG_STRUCTURED_BINDINGS 1
    #define D_ENV_CPP_FEATURE_LANG_STRUCTURED_BINDINGS_VAL                     \
        __cpp_structured_bindings
#else
    #define D_ENV_CPP_FEATURE_LANG_STRUCTURED_BINDINGS 0
    #define D_ENV_CPP_FEATURE_LANG_STRUCTURED_BINDINGS_VAL 0L
#endif  // __cpp_structured_bindings
#define D_ENV_CPP_FEATURE_LANG_STRUCTURED_BINDINGS_NAME                        \
    "__cpp_structured_bindings"
#define D_ENV_CPP_FEATURE_LANG_STRUCTURED_BINDINGS_DESC "Structured bindings"
#define D_ENV_CPP_FEATURE_LANG_STRUCTURED_BINDINGS_VERS "(C++17)"

// 1.3.14
// D_ENV_CPP_FEATURE_LANG_TEMPLATE_TEMPLATE_ARGS
//   feature: 1 if __cpp_template_template_args is defined, 0 otherwise.
#ifdef __cpp_template_template_args
    #define D_ENV_CPP_FEATURE_LANG_TEMPLATE_TEMPLATE_ARGS 1
    #define D_ENV_CPP_FEATURE_LANG_TEMPLATE_TEMPLATE_ARGS_VAL                  \
        __cpp_template_template_args
#else
    #define D_ENV_CPP_FEATURE_LANG_TEMPLATE_TEMPLATE_ARGS 0
    #define D_ENV_CPP_FEATURE_LANG_TEMPLATE_TEMPLATE_ARGS_VAL 0L
#endif  // __cpp_template_template_args
#define D_ENV_CPP_FEATURE_LANG_TEMPLATE_TEMPLATE_ARGS_NAME                     \
    "__cpp_template_template_args"
#define D_ENV_CPP_FEATURE_LANG_TEMPLATE_TEMPLATE_ARGS_DESC                     \
    "Matching of template template arguments"
#define D_ENV_CPP_FEATURE_LANG_TEMPLATE_TEMPLATE_ARGS_VERS "(C++17)"

// 1.3.15
// D_ENV_CPP_FEATURE_LANG_VARIADIC_USING
//   feature: 1 if __cpp_variadic_using is defined, 0 otherwise.
#ifdef __cpp_variadic_using
    #define D_ENV_CPP_FEATURE_LANG_VARIADIC_USING     1
    #define D_ENV_CPP_FEATURE_LANG_VARIADIC_USING_VAL __cpp_variadic_using
#else
    #define D_ENV_CPP_FEATURE_LANG_VARIADIC_USING     0
    #define D_ENV_CPP_FEATURE_LANG_VARIADIC_USING_VAL 0L
#endif  // __cpp_variadic_using
#define D_ENV_CPP_FEATURE_LANG_VARIADIC_USING_NAME "__cpp_variadic_using"
#define D_ENV_CPP_FEATURE_LANG_VARIADIC_USING_DESC                             \
    "Pack expansions in using-declarations"
#define D_ENV_CPP_FEATURE_LANG_VARIADIC_USING_VERS "(C++17)"

// 1.4    C++20 language features
//------------------------------------------------------------------------------
// 1.4.1
// D_ENV_CPP_FEATURE_LANG_AGGREGATE_PAREN_INIT
//   feature: 1 if __cpp_aggregate_paren_init is defined, 0 otherwise.
#ifdef __cpp_aggregate_paren_init
    #define D_ENV_CPP_FEATURE_LANG_AGGREGATE_PAREN_INIT 1
    #define D_ENV_CPP_FEATURE_LANG_AGGREGATE_PAREN_INIT_VAL                    \
        __cpp_aggregate_paren_init
#else
    #define D_ENV_CPP_FEATURE_LANG_AGGREGATE_PAREN_INIT 0
    #define D_ENV_CPP_FEATURE_LANG_AGGREGATE_PAREN_INIT_VAL 0L
#endif  // __cpp_aggregate_paren_init
#define D_ENV_CPP_FEATURE_LANG_AGGREGATE_PAREN_INIT_NAME                       \
    "__cpp_aggregate_paren_init"
#define D_ENV_CPP_FEATURE_LANG_AGGREGATE_PAREN_INIT_DESC                       \
    "Aggregate initialization in the form of direct initialization"
#define D_ENV_CPP_FEATURE_LANG_AGGREGATE_PAREN_INIT_VERS "(C++20)"

// 1.4.2
// D_ENV_CPP_FEATURE_LANG_CHAR8_T
//   feature: 1 if __cpp_char8_t is defined, 0 otherwise.
#ifdef __cpp_char8_t
    #define D_ENV_CPP_FEATURE_LANG_CHAR8_T     1
    #define D_ENV_CPP_FEATURE_LANG_CHAR8_T_VAL __cpp_char8_t
#else
    #define D_ENV_CPP_FEATURE_LANG_CHAR8_T     0
    #define D_ENV_CPP_FEATURE_LANG_CHAR8_T_VAL 0L
#endif  // __cpp_char8_t
#define D_ENV_CPP_FEATURE_LANG_CHAR8_T_NAME "__cpp_char8_t"
#define D_ENV_CPP_FEATURE_LANG_CHAR8_T_DESC "char8_t"
#define D_ENV_CPP_FEATURE_LANG_CHAR8_T_VERS "(C++20)"

// 1.4.3
// D_ENV_CPP_FEATURE_LANG_CONCEPTS
//   feature: 1 if __cpp_concepts is defined, 0 otherwise.
// D_ENV_CPP_FEATURE_LANG_CONCEPTS_TS is 1 when __cpp_concepts == 201507L;
// D_ENV_CPP_FEATURE_LANG_CONCEPTS_CPP20 is 1 when __cpp_concepts >= 201907L.
#ifdef __cpp_concepts
    #define D_ENV_CPP_FEATURE_LANG_CONCEPTS     1
    #define D_ENV_CPP_FEATURE_LANG_CONCEPTS_VAL __cpp_concepts
#else
    #define D_ENV_CPP_FEATURE_LANG_CONCEPTS     0
    #define D_ENV_CPP_FEATURE_LANG_CONCEPTS_VAL 0L
#endif  // __cpp_concepts
#if ( (defined(__cpp_concepts)) &&                                             \
      (__cpp_concepts == 201507L) )
    #define D_ENV_CPP_FEATURE_LANG_CONCEPTS_TS 1
#else
    #define D_ENV_CPP_FEATURE_LANG_CONCEPTS_TS 0
#endif
#if ( (defined(__cpp_concepts)) &&                                             \
      (__cpp_concepts >= 201907L) )
    #define D_ENV_CPP_FEATURE_LANG_CONCEPTS_CPP20 1
#else
    #define D_ENV_CPP_FEATURE_LANG_CONCEPTS_CPP20 0
#endif
#define D_ENV_CPP_FEATURE_LANG_CONCEPTS_NAME "__cpp_concepts"
#define D_ENV_CPP_FEATURE_LANG_CONCEPTS_DESC "Concepts"
#define D_ENV_CPP_FEATURE_LANG_CONCEPTS_VERS "(C++20)"

// 1.4.4
// D_ENV_CPP_FEATURE_LANG_CONDITIONAL_EXPLICIT
//   feature: 1 if __cpp_conditional_explicit is defined, 0 otherwise.
#ifdef __cpp_conditional_explicit
    #define D_ENV_CPP_FEATURE_LANG_CONDITIONAL_EXPLICIT 1
    #define D_ENV_CPP_FEATURE_LANG_CONDITIONAL_EXPLICIT_VAL                    \
        __cpp_conditional_explicit
#else
    #define D_ENV_CPP_FEATURE_LANG_CONDITIONAL_EXPLICIT 0
    #define D_ENV_CPP_FEATURE_LANG_CONDITIONAL_EXPLICIT_VAL 0L
#endif  // __cpp_conditional_explicit
#define D_ENV_CPP_FEATURE_LANG_CONDITIONAL_EXPLICIT_NAME                       \
    "__cpp_conditional_explicit"
#define D_ENV_CPP_FEATURE_LANG_CONDITIONAL_EXPLICIT_DESC "explicit(bool)"
#define D_ENV_CPP_FEATURE_LANG_CONDITIONAL_EXPLICIT_VERS "(C++20)"

// 1.4.5
// D_ENV_CPP_FEATURE_LANG_CONSTEVAL
//   feature: 1 if __cpp_consteval is defined, 0 otherwise.
#ifdef __cpp_consteval
    #define D_ENV_CPP_FEATURE_LANG_CONSTEVAL     1
    #define D_ENV_CPP_FEATURE_LANG_CONSTEVAL_VAL __cpp_consteval
#else
    #define D_ENV_CPP_FEATURE_LANG_CONSTEVAL     0
    #define D_ENV_CPP_FEATURE_LANG_CONSTEVAL_VAL 0L
#endif  // __cpp_consteval
#define D_ENV_CPP_FEATURE_LANG_CONSTEVAL_NAME "__cpp_consteval"
#define D_ENV_CPP_FEATURE_LANG_CONSTEVAL_DESC "Immediate functions"
#define D_ENV_CPP_FEATURE_LANG_CONSTEVAL_VERS "(C++20)"

// 1.4.6
// D_ENV_CPP_FEATURE_LANG_CONSTEXPR_DYNAMIC_ALLOC
//   feature: 1 if __cpp_constexpr_dynamic_alloc is defined, 0 otherwise.
#ifdef __cpp_constexpr_dynamic_alloc
    #define D_ENV_CPP_FEATURE_LANG_CONSTEXPR_DYNAMIC_ALLOC 1
    #define D_ENV_CPP_FEATURE_LANG_CONSTEXPR_DYNAMIC_ALLOC_VAL                 \
        __cpp_constexpr_dynamic_alloc
#else
    #define D_ENV_CPP_FEATURE_LANG_CONSTEXPR_DYNAMIC_ALLOC 0
    #define D_ENV_CPP_FEATURE_LANG_CONSTEXPR_DYNAMIC_ALLOC_VAL 0L
#endif  // __cpp_constexpr_dynamic_alloc
#define D_ENV_CPP_FEATURE_LANG_CONSTEXPR_DYNAMIC_ALLOC_NAME                    \
    "__cpp_constexpr_dynamic_alloc"
#define D_ENV_CPP_FEATURE_LANG_CONSTEXPR_DYNAMIC_ALLOC_DESC                    \
    "Operations for dynamic storage duration in constexpr functions"
#define D_ENV_CPP_FEATURE_LANG_CONSTEXPR_DYNAMIC_ALLOC_VERS "(C++20)"

// 1.4.7
// D_ENV_CPP_FEATURE_LANG_CONSTINIT
//   feature: 1 if __cpp_constinit is defined, 0 otherwise.
#ifdef __cpp_constinit
    #define D_ENV_CPP_FEATURE_LANG_CONSTINIT     1
    #define D_ENV_CPP_FEATURE_LANG_CONSTINIT_VAL __cpp_constinit
#else
    #define D_ENV_CPP_FEATURE_LANG_CONSTINIT     0
    #define D_ENV_CPP_FEATURE_LANG_CONSTINIT_VAL 0L
#endif  // __cpp_constinit
#define D_ENV_CPP_FEATURE_LANG_CONSTINIT_NAME "__cpp_constinit"
#define D_ENV_CPP_FEATURE_LANG_CONSTINIT_DESC "constinit"
#define D_ENV_CPP_FEATURE_LANG_CONSTINIT_VERS "(C++20)"

// 1.4.8
// D_ENV_CPP_FEATURE_LANG_DESIGNATED_INITIALIZERS
//   feature: 1 if __cpp_designated_initializers is defined, 0 otherwise.
#ifdef __cpp_designated_initializers
    #define D_ENV_CPP_FEATURE_LANG_DESIGNATED_INITIALIZERS 1
    #define D_ENV_CPP_FEATURE_LANG_DESIGNATED_INITIALIZERS_VAL                 \
        __cpp_designated_initializers
#else
    #define D_ENV_CPP_FEATURE_LANG_DESIGNATED_INITIALIZERS 0
    #define D_ENV_CPP_FEATURE_LANG_DESIGNATED_INITIALIZERS_VAL 0L
#endif  // __cpp_designated_initializers
#define D_ENV_CPP_FEATURE_LANG_DESIGNATED_INITIALIZERS_NAME                    \
    "__cpp_designated_initializers"
#define D_ENV_CPP_FEATURE_LANG_DESIGNATED_INITIALIZERS_DESC                    \
    "Designated initializers"
#define D_ENV_CPP_FEATURE_LANG_DESIGNATED_INITIALIZERS_VERS "(C++20)"

// 1.4.9
// D_ENV_CPP_FEATURE_LANG_IMPL_COROUTINE
//   feature: 1 if __cpp_IMPL_coroutine is defined, 0 otherwise.
#ifdef __cpp_IMPL_coroutine
    #define D_ENV_CPP_FEATURE_LANG_IMPL_COROUTINE     1
    #define D_ENV_CPP_FEATURE_LANG_IMPL_COROUTINE_VAL __cpp_IMPL_coroutine
#else
    #define D_ENV_CPP_FEATURE_LANG_IMPL_COROUTINE     0
    #define D_ENV_CPP_FEATURE_LANG_IMPL_COROUTINE_VAL 0L
#endif  // __cpp_IMPL_coroutine
#define D_ENV_CPP_FEATURE_LANG_IMPL_COROUTINE_NAME "__cpp_IMPL_coroutine"
#define D_ENV_CPP_FEATURE_LANG_IMPL_COROUTINE_DESC                             \
    "Coroutines (compiler support)"
#define D_ENV_CPP_FEATURE_LANG_IMPL_COROUTINE_VERS "(C++20)"

// 1.4.10
// D_ENV_CPP_FEATURE_LANG_IMPL_DESTROYING_DELETE
//   feature: 1 if __cpp_IMPL_destroying_delete is defined, 0 otherwise.
#ifdef __cpp_IMPL_destroying_delete
    #define D_ENV_CPP_FEATURE_LANG_IMPL_DESTROYING_DELETE 1
    #define D_ENV_CPP_FEATURE_LANG_IMPL_DESTROYING_DELETE_VAL                  \
        __cpp_IMPL_destroying_delete
#else
    #define D_ENV_CPP_FEATURE_LANG_IMPL_DESTROYING_DELETE 0
    #define D_ENV_CPP_FEATURE_LANG_IMPL_DESTROYING_DELETE_VAL 0L
#endif  // __cpp_IMPL_destroying_delete
#define D_ENV_CPP_FEATURE_LANG_IMPL_DESTROYING_DELETE_NAME                     \
    "__cpp_IMPL_destroying_delete"
#define D_ENV_CPP_FEATURE_LANG_IMPL_DESTROYING_DELETE_DESC                     \
    "Destroying operator delete (compiler support)"
#define D_ENV_CPP_FEATURE_LANG_IMPL_DESTROYING_DELETE_VERS "(C++20)"

// 1.4.11
// D_ENV_CPP_FEATURE_LANG_IMPL_THREE_WAY_COMPARISON
//   feature: 1 if __cpp_IMPL_three_way_comparison is defined, 0 otherwise.
#ifdef __cpp_IMPL_three_way_comparison
    #define D_ENV_CPP_FEATURE_LANG_IMPL_THREE_WAY_COMPARISON 1
    #define D_ENV_CPP_FEATURE_LANG_IMPL_THREE_WAY_COMPARISON_VAL               \
        __cpp_IMPL_three_way_comparison
#else
    #define D_ENV_CPP_FEATURE_LANG_IMPL_THREE_WAY_COMPARISON 0
    #define D_ENV_CPP_FEATURE_LANG_IMPL_THREE_WAY_COMPARISON_VAL 0L
#endif  // __cpp_IMPL_three_way_comparison
#define D_ENV_CPP_FEATURE_LANG_IMPL_THREE_WAY_COMPARISON_NAME                  \
    "__cpp_IMPL_three_way_comparison"
#define D_ENV_CPP_FEATURE_LANG_IMPL_THREE_WAY_COMPARISON_DESC                  \
    "Three-way comparison (compiler support)"
#define D_ENV_CPP_FEATURE_LANG_IMPL_THREE_WAY_COMPARISON_VERS "(C++20)"

// 1.4.12
// D_ENV_CPP_FEATURE_LANG_MODULES
//   feature: 1 if __cpp_modules is defined, 0 otherwise.
#ifdef __cpp_modules
    #define D_ENV_CPP_FEATURE_LANG_MODULES     1
    #define D_ENV_CPP_FEATURE_LANG_MODULES_VAL __cpp_modules
#else
    #define D_ENV_CPP_FEATURE_LANG_MODULES     0
    #define D_ENV_CPP_FEATURE_LANG_MODULES_VAL 0L
#endif  // __cpp_modules
#define D_ENV_CPP_FEATURE_LANG_MODULES_NAME "__cpp_modules"
#define D_ENV_CPP_FEATURE_LANG_MODULES_DESC "Modules"
#define D_ENV_CPP_FEATURE_LANG_MODULES_VERS "(C++20)"

// 1.4.13
// D_ENV_CPP_FEATURE_LANG_USING_ENUM
//   feature: 1 if __cpp_using_enum is defined, 0 otherwise.
#ifdef __cpp_using_enum
    #define D_ENV_CPP_FEATURE_LANG_USING_ENUM     1
    #define D_ENV_CPP_FEATURE_LANG_USING_ENUM_VAL __cpp_using_enum
#else
    #define D_ENV_CPP_FEATURE_LANG_USING_ENUM     0
    #define D_ENV_CPP_FEATURE_LANG_USING_ENUM_VAL 0L
#endif  // __cpp_using_enum
#define D_ENV_CPP_FEATURE_LANG_USING_ENUM_NAME "__cpp_using_enum"
#define D_ENV_CPP_FEATURE_LANG_USING_ENUM_DESC "using enum"
#define D_ENV_CPP_FEATURE_LANG_USING_ENUM_VERS "(C++20)"

// 1.5    C++23 language features
//------------------------------------------------------------------------------
// 1.5.1
// D_ENV_CPP_FEATURE_LANG_AUTO_CAST
//   feature: 1 if __cpp_auto_cast is defined, 0 otherwise.
#ifdef __cpp_auto_cast
    #define D_ENV_CPP_FEATURE_LANG_AUTO_CAST     1
    #define D_ENV_CPP_FEATURE_LANG_AUTO_CAST_VAL __cpp_auto_cast
#else
    #define D_ENV_CPP_FEATURE_LANG_AUTO_CAST     0
    #define D_ENV_CPP_FEATURE_LANG_AUTO_CAST_VAL 0L
#endif  // __cpp_auto_cast
#define D_ENV_CPP_FEATURE_LANG_AUTO_CAST_NAME "__cpp_auto_cast"
#define D_ENV_CPP_FEATURE_LANG_AUTO_CAST_DESC "auto(x) and auto{x}"
#define D_ENV_CPP_FEATURE_LANG_AUTO_CAST_VERS "(C++23)"

// 1.5.2
// D_ENV_CPP_FEATURE_LANG_EXPLICIT_THIS_PARAMETER
//   feature: 1 if __cpp_explicit_this_parameter is defined, 0 otherwise.
#ifdef __cpp_explicit_this_parameter
    #define D_ENV_CPP_FEATURE_LANG_EXPLICIT_THIS_PARAMETER 1
    #define D_ENV_CPP_FEATURE_LANG_EXPLICIT_THIS_PARAMETER_VAL                 \
        __cpp_explicit_this_parameter
#else
    #define D_ENV_CPP_FEATURE_LANG_EXPLICIT_THIS_PARAMETER 0
    #define D_ENV_CPP_FEATURE_LANG_EXPLICIT_THIS_PARAMETER_VAL 0L
#endif  // __cpp_explicit_this_parameter
#define D_ENV_CPP_FEATURE_LANG_EXPLICIT_THIS_PARAMETER_NAME                    \
    "__cpp_explicit_this_parameter"
#define D_ENV_CPP_FEATURE_LANG_EXPLICIT_THIS_PARAMETER_DESC                    \
    "Explicit object parameter"
#define D_ENV_CPP_FEATURE_LANG_EXPLICIT_THIS_PARAMETER_VERS "(C++23)"

// 1.5.3
// D_ENV_CPP_FEATURE_LANG_IF_CONSTEVAL
//   feature: 1 if __cpp_if_consteval is defined, 0 otherwise.
#ifdef __cpp_if_consteval
    #define D_ENV_CPP_FEATURE_LANG_IF_CONSTEVAL     1
    #define D_ENV_CPP_FEATURE_LANG_IF_CONSTEVAL_VAL __cpp_if_consteval
#else
    #define D_ENV_CPP_FEATURE_LANG_IF_CONSTEVAL     0
    #define D_ENV_CPP_FEATURE_LANG_IF_CONSTEVAL_VAL 0L
#endif  // __cpp_if_consteval
#define D_ENV_CPP_FEATURE_LANG_IF_CONSTEVAL_NAME "__cpp_if_consteval"
#define D_ENV_CPP_FEATURE_LANG_IF_CONSTEVAL_DESC "if consteval"
#define D_ENV_CPP_FEATURE_LANG_IF_CONSTEVAL_VERS "(C++23)"

// 1.5.4
// D_ENV_CPP_FEATURE_LANG_IMPLICIT_MOVE
//   feature: 1 if __cpp_implicit_move is defined, 0 otherwise.
#ifdef __cpp_implicit_move
    #define D_ENV_CPP_FEATURE_LANG_IMPLICIT_MOVE     1
    #define D_ENV_CPP_FEATURE_LANG_IMPLICIT_MOVE_VAL __cpp_implicit_move
#else
    #define D_ENV_CPP_FEATURE_LANG_IMPLICIT_MOVE     0
    #define D_ENV_CPP_FEATURE_LANG_IMPLICIT_MOVE_VAL 0L
#endif  // __cpp_implicit_move
#define D_ENV_CPP_FEATURE_LANG_IMPLICIT_MOVE_NAME "__cpp_implicit_move"
#define D_ENV_CPP_FEATURE_LANG_IMPLICIT_MOVE_DESC "Simpler implicit move"
#define D_ENV_CPP_FEATURE_LANG_IMPLICIT_MOVE_VERS "(C++23)"

// 1.5.5
// D_ENV_CPP_FEATURE_LANG_MULTIDIMENSIONAL_SUBSCRIPT
//   feature: 1 if __cpp_multidimensional_subscript is defined, 0 otherwise.
#ifdef __cpp_multidimensional_subscript
    #define D_ENV_CPP_FEATURE_LANG_MULTIDIMENSIONAL_SUBSCRIPT 1
    #define D_ENV_CPP_FEATURE_LANG_MULTIDIMENSIONAL_SUBSCRIPT_VAL              \
        __cpp_multidimensional_subscript
#else
    #define D_ENV_CPP_FEATURE_LANG_MULTIDIMENSIONAL_SUBSCRIPT 0
    #define D_ENV_CPP_FEATURE_LANG_MULTIDIMENSIONAL_SUBSCRIPT_VAL 0L
#endif  // __cpp_multidimensional_subscript
#define D_ENV_CPP_FEATURE_LANG_MULTIDIMENSIONAL_SUBSCRIPT_NAME                 \
    "__cpp_multidimensional_subscript"
#define D_ENV_CPP_FEATURE_LANG_MULTIDIMENSIONAL_SUBSCRIPT_DESC                 \
    "Multidimensional subscript operator"
#define D_ENV_CPP_FEATURE_LANG_MULTIDIMENSIONAL_SUBSCRIPT_VERS "(C++23)"

// 1.5.6
// D_ENV_CPP_FEATURE_LANG_NAMED_CHARACTER_ESCAPES
//   feature: 1 if __cpp_named_character_escapes is defined, 0 otherwise.
#ifdef __cpp_named_character_escapes
    #define D_ENV_CPP_FEATURE_LANG_NAMED_CHARACTER_ESCAPES 1
    #define D_ENV_CPP_FEATURE_LANG_NAMED_CHARACTER_ESCAPES_VAL                 \
        __cpp_named_character_escapes
#else
    #define D_ENV_CPP_FEATURE_LANG_NAMED_CHARACTER_ESCAPES 0
    #define D_ENV_CPP_FEATURE_LANG_NAMED_CHARACTER_ESCAPES_VAL 0L
#endif  // __cpp_named_character_escapes
#define D_ENV_CPP_FEATURE_LANG_NAMED_CHARACTER_ESCAPES_NAME                    \
    "__cpp_named_character_escapes"
#define D_ENV_CPP_FEATURE_LANG_NAMED_CHARACTER_ESCAPES_DESC                    \
    "Named universal character escapes"
#define D_ENV_CPP_FEATURE_LANG_NAMED_CHARACTER_ESCAPES_VERS "(C++23)"

// 1.5.7
// D_ENV_CPP_FEATURE_LANG_SIZE_T_SUFFIX
//   feature: 1 if __cpp_size_t_suffix is defined, 0 otherwise.
#ifdef __cpp_size_t_suffix
    #define D_ENV_CPP_FEATURE_LANG_SIZE_T_SUFFIX     1
    #define D_ENV_CPP_FEATURE_LANG_SIZE_T_SUFFIX_VAL __cpp_size_t_suffix
#else
    #define D_ENV_CPP_FEATURE_LANG_SIZE_T_SUFFIX     0
    #define D_ENV_CPP_FEATURE_LANG_SIZE_T_SUFFIX_VAL 0L
#endif  // __cpp_size_t_suffix
#define D_ENV_CPP_FEATURE_LANG_SIZE_T_SUFFIX_NAME "__cpp_size_t_suffix"
#define D_ENV_CPP_FEATURE_LANG_SIZE_T_SUFFIX_DESC                              \
    "Literal suffixes for std::size_t and its signed version"
#define D_ENV_CPP_FEATURE_LANG_SIZE_T_SUFFIX_VERS "(C++23)"

// 1.5.8
// D_ENV_CPP_FEATURE_LANG_STATIC_CALL_OPERATOR
//   feature: 1 if __cpp_static_call_operator is defined, 0 otherwise.
#ifdef __cpp_static_call_operator
    #define D_ENV_CPP_FEATURE_LANG_STATIC_CALL_OPERATOR 1
    #define D_ENV_CPP_FEATURE_LANG_STATIC_CALL_OPERATOR_VAL                    \
        __cpp_static_call_operator
#else
    #define D_ENV_CPP_FEATURE_LANG_STATIC_CALL_OPERATOR 0
    #define D_ENV_CPP_FEATURE_LANG_STATIC_CALL_OPERATOR_VAL 0L
#endif  // __cpp_static_call_operator
#define D_ENV_CPP_FEATURE_LANG_STATIC_CALL_OPERATOR_NAME                       \
    "__cpp_static_call_operator"
#define D_ENV_CPP_FEATURE_LANG_STATIC_CALL_OPERATOR_DESC "Static operator()"
#define D_ENV_CPP_FEATURE_LANG_STATIC_CALL_OPERATOR_VERS "(C++23)"

// 1.6    C++26 language features
//------------------------------------------------------------------------------
// 1.6.1
// D_ENV_CPP_FEATURE_LANG_CONSTEXPR_EXCEPTIONS
//   feature: 1 if __cpp_constexpr_exceptions is defined, 0 otherwise.
#ifdef __cpp_constexpr_exceptions
    #define D_ENV_CPP_FEATURE_LANG_CONSTEXPR_EXCEPTIONS 1
    #define D_ENV_CPP_FEATURE_LANG_CONSTEXPR_EXCEPTIONS_VAL                    \
        __cpp_constexpr_exceptions
#else
    #define D_ENV_CPP_FEATURE_LANG_CONSTEXPR_EXCEPTIONS 0
    #define D_ENV_CPP_FEATURE_LANG_CONSTEXPR_EXCEPTIONS_VAL 0L
#endif  // __cpp_constexpr_exceptions
#define D_ENV_CPP_FEATURE_LANG_CONSTEXPR_EXCEPTIONS_NAME                       \
    "__cpp_constexpr_exceptions"
#define D_ENV_CPP_FEATURE_LANG_CONSTEXPR_EXCEPTIONS_DESC "constexpr exceptions"
#define D_ENV_CPP_FEATURE_LANG_CONSTEXPR_EXCEPTIONS_VERS "(C++26)"

// 1.6.2
// D_ENV_CPP_FEATURE_LANG_CONTRACTS
//   feature: 1 if __cpp_contracts is defined, 0 otherwise.
#ifdef __cpp_contracts
    #define D_ENV_CPP_FEATURE_LANG_CONTRACTS     1
    #define D_ENV_CPP_FEATURE_LANG_CONTRACTS_VAL __cpp_contracts
#else
    #define D_ENV_CPP_FEATURE_LANG_CONTRACTS     0
    #define D_ENV_CPP_FEATURE_LANG_CONTRACTS_VAL 0L
#endif  // __cpp_contracts
#define D_ENV_CPP_FEATURE_LANG_CONTRACTS_NAME "__cpp_contracts"
#define D_ENV_CPP_FEATURE_LANG_CONTRACTS_DESC "Contracts"
#define D_ENV_CPP_FEATURE_LANG_CONTRACTS_VERS "(C++26)"

// 1.6.3
// D_ENV_CPP_FEATURE_LANG_DELETED_FUNCTION
//   feature: 1 if __cpp_deleted_function is defined, 0 otherwise.
#ifdef __cpp_deleted_function
    #define D_ENV_CPP_FEATURE_LANG_DELETED_FUNCTION     1
    #define D_ENV_CPP_FEATURE_LANG_DELETED_FUNCTION_VAL __cpp_deleted_function
#else
    #define D_ENV_CPP_FEATURE_LANG_DELETED_FUNCTION     0
    #define D_ENV_CPP_FEATURE_LANG_DELETED_FUNCTION_VAL 0L
#endif  // __cpp_deleted_function
#define D_ENV_CPP_FEATURE_LANG_DELETED_FUNCTION_NAME "__cpp_deleted_function"
#define D_ENV_CPP_FEATURE_LANG_DELETED_FUNCTION_DESC                           \
    "Deleted function definitions with messages"
#define D_ENV_CPP_FEATURE_LANG_DELETED_FUNCTION_VERS "(C++26)"

// 1.6.4
// D_ENV_CPP_FEATURE_LANG_PACK_INDEXING
//   feature: 1 if __cpp_pack_indexing is defined, 0 otherwise.
#ifdef __cpp_pack_indexing
    #define D_ENV_CPP_FEATURE_LANG_PACK_INDEXING     1
    #define D_ENV_CPP_FEATURE_LANG_PACK_INDEXING_VAL __cpp_pack_indexing
#else
    #define D_ENV_CPP_FEATURE_LANG_PACK_INDEXING     0
    #define D_ENV_CPP_FEATURE_LANG_PACK_INDEXING_VAL 0L
#endif  // __cpp_pack_indexing
#define D_ENV_CPP_FEATURE_LANG_PACK_INDEXING_NAME "__cpp_pack_indexing"
#define D_ENV_CPP_FEATURE_LANG_PACK_INDEXING_DESC "Pack indexing"
#define D_ENV_CPP_FEATURE_LANG_PACK_INDEXING_VERS "(C++26)"

// 1.6.5
// D_ENV_CPP_FEATURE_LANG_PLACEHOLDER_VARIABLES
//   feature: 1 if __cpp_placeholder_variables is defined, 0 otherwise.
#ifdef __cpp_placeholder_variables
    #define D_ENV_CPP_FEATURE_LANG_PLACEHOLDER_VARIABLES 1
    #define D_ENV_CPP_FEATURE_LANG_PLACEHOLDER_VARIABLES_VAL                   \
        __cpp_placeholder_variables
#else
    #define D_ENV_CPP_FEATURE_LANG_PLACEHOLDER_VARIABLES 0
    #define D_ENV_CPP_FEATURE_LANG_PLACEHOLDER_VARIABLES_VAL 0L
#endif  // __cpp_placeholder_variables
#define D_ENV_CPP_FEATURE_LANG_PLACEHOLDER_VARIABLES_NAME                      \
    "__cpp_placeholder_variables"
#define D_ENV_CPP_FEATURE_LANG_PLACEHOLDER_VARIABLES_DESC                      \
    "A nice placeholder with no name"
#define D_ENV_CPP_FEATURE_LANG_PLACEHOLDER_VARIABLES_VERS "(C++26)"

// 1.6.6
// D_ENV_CPP_FEATURE_LANG_PP_EMBED
//   feature: 1 if __cpp_pp_embed is defined, 0 otherwise.
#ifdef __cpp_pp_embed
    #define D_ENV_CPP_FEATURE_LANG_PP_EMBED     1
    #define D_ENV_CPP_FEATURE_LANG_PP_EMBED_VAL __cpp_pp_embed
#else
    #define D_ENV_CPP_FEATURE_LANG_PP_EMBED     0
    #define D_ENV_CPP_FEATURE_LANG_PP_EMBED_VAL 0L
#endif  // __cpp_pp_embed
#define D_ENV_CPP_FEATURE_LANG_PP_EMBED_NAME "__cpp_pp_embed"
#define D_ENV_CPP_FEATURE_LANG_PP_EMBED_DESC "#embed"
#define D_ENV_CPP_FEATURE_LANG_PP_EMBED_VERS "(C++26)"

// 1.6.7
// D_ENV_CPP_FEATURE_LANG_TEMPLATE_PARAMETERS
//   feature: 1 if __cpp_template_parameters is defined, 0 otherwise.
#ifdef __cpp_template_parameters
    #define D_ENV_CPP_FEATURE_LANG_TEMPLATE_PARAMETERS 1
    #define D_ENV_CPP_FEATURE_LANG_TEMPLATE_PARAMETERS_VAL                     \
        __cpp_template_parameters
#else
    #define D_ENV_CPP_FEATURE_LANG_TEMPLATE_PARAMETERS 0
    #define D_ENV_CPP_FEATURE_LANG_TEMPLATE_PARAMETERS_VAL 0L
#endif  // __cpp_template_parameters
#define D_ENV_CPP_FEATURE_LANG_TEMPLATE_PARAMETERS_NAME                        \
    "__cpp_template_parameters"
#define D_ENV_CPP_FEATURE_LANG_TEMPLATE_PARAMETERS_DESC                        \
    "Concept and variable-template template-parameters"
#define D_ENV_CPP_FEATURE_LANG_TEMPLATE_PARAMETERS_VERS "(C++26)"

// 1.6.8
// D_ENV_CPP_FEATURE_LANG_TRIVIAL_RELOCATABILITY
//   feature: 1 if __cpp_trivial_relocatability is defined, 0 otherwise.
#ifdef __cpp_trivial_relocatability
    #define D_ENV_CPP_FEATURE_LANG_TRIVIAL_RELOCATABILITY 1
    #define D_ENV_CPP_FEATURE_LANG_TRIVIAL_RELOCATABILITY_VAL                  \
        __cpp_trivial_relocatability
#else
    #define D_ENV_CPP_FEATURE_LANG_TRIVIAL_RELOCATABILITY 0
    #define D_ENV_CPP_FEATURE_LANG_TRIVIAL_RELOCATABILITY_VAL 0L
#endif  // __cpp_trivial_relocatability
#define D_ENV_CPP_FEATURE_LANG_TRIVIAL_RELOCATABILITY_NAME                     \
    "__cpp_trivial_relocatability"
#define D_ENV_CPP_FEATURE_LANG_TRIVIAL_RELOCATABILITY_DESC                     \
    "Trivial relocatability"
#define D_ENV_CPP_FEATURE_LANG_TRIVIAL_RELOCATABILITY_VERS "(C++26)"

// 1.6.9
// D_ENV_CPP_FEATURE_LANG_TRIVIAL_UNION
//   feature: 1 if __cpp_trivial_union is defined, 0 otherwise.
#ifdef __cpp_trivial_union
    #define D_ENV_CPP_FEATURE_LANG_TRIVIAL_UNION     1
    #define D_ENV_CPP_FEATURE_LANG_TRIVIAL_UNION_VAL __cpp_trivial_union
#else
    #define D_ENV_CPP_FEATURE_LANG_TRIVIAL_UNION     0
    #define D_ENV_CPP_FEATURE_LANG_TRIVIAL_UNION_VAL 0L
#endif  // __cpp_trivial_union
#define D_ENV_CPP_FEATURE_LANG_TRIVIAL_UNION_NAME "__cpp_trivial_union"
#define D_ENV_CPP_FEATURE_LANG_TRIVIAL_UNION_DESC "Trivial unions"
#define D_ENV_CPP_FEATURE_LANG_TRIVIAL_UNION_VERS "(C++26)"

// 1.6.10
// D_ENV_CPP_FEATURE_LANG_VARIADIC_FRIEND
//   feature: 1 if __cpp_variadic_friend is defined, 0 otherwise.
#ifdef __cpp_variadic_friend
    #define D_ENV_CPP_FEATURE_LANG_VARIADIC_FRIEND     1
    #define D_ENV_CPP_FEATURE_LANG_VARIADIC_FRIEND_VAL __cpp_variadic_friend
#else
    #define D_ENV_CPP_FEATURE_LANG_VARIADIC_FRIEND     0
    #define D_ENV_CPP_FEATURE_LANG_VARIADIC_FRIEND_VAL 0L
#endif  // __cpp_variadic_friend
#define D_ENV_CPP_FEATURE_LANG_VARIADIC_FRIEND_NAME "__cpp_variadic_friend"
#define D_ENV_CPP_FEATURE_LANG_VARIADIC_FRIEND_DESC                            \
    "Variadic friend declarations"
#define D_ENV_CPP_FEATURE_LANG_VARIADIC_FRIEND_VERS "(C++26)"

//==============================================================================
// 2.  STANDARD LIBRARY FEATURES
//==============================================================================
// Laid out as section 1. These flags see only the library feature-test
// macros the translation unit has already brought in; see the banner.


// 2.1    C++14 library features
//------------------------------------------------------------------------------
// 2.1.1
// D_ENV_CPP_FEATURE_STL_CHRONO_UDLS
//   feature: 1 if __cpp_lib_chrono_udls is defined, 0 otherwise.
#ifdef __cpp_lib_chrono_udls
    #define D_ENV_CPP_FEATURE_STL_CHRONO_UDLS     1
    #define D_ENV_CPP_FEATURE_STL_CHRONO_UDLS_VAL __cpp_lib_chrono_udls
#else
    #define D_ENV_CPP_FEATURE_STL_CHRONO_UDLS     0
    #define D_ENV_CPP_FEATURE_STL_CHRONO_UDLS_VAL 0L
#endif  // __cpp_lib_chrono_udls
#define D_ENV_CPP_FEATURE_STL_CHRONO_UDLS_NAME "__cpp_lib_chrono_udls"
#define D_ENV_CPP_FEATURE_STL_CHRONO_UDLS_DESC                                 \
    "User-defined literals for time types"
#define D_ENV_CPP_FEATURE_STL_CHRONO_UDLS_VERS "(C++14)"

// 2.1.2
// D_ENV_CPP_FEATURE_STL_COMPLEX_UDLS
//   feature: 1 if __cpp_lib_complex_udls is defined, 0 otherwise.
#ifdef __cpp_lib_complex_udls
    #define D_ENV_CPP_FEATURE_STL_COMPLEX_UDLS     1
    #define D_ENV_CPP_FEATURE_STL_COMPLEX_UDLS_VAL __cpp_lib_complex_udls
#else
    #define D_ENV_CPP_FEATURE_STL_COMPLEX_UDLS     0
    #define D_ENV_CPP_FEATURE_STL_COMPLEX_UDLS_VAL 0L
#endif  // __cpp_lib_complex_udls
#define D_ENV_CPP_FEATURE_STL_COMPLEX_UDLS_NAME "__cpp_lib_complex_udls"
#define D_ENV_CPP_FEATURE_STL_COMPLEX_UDLS_DESC                                \
    "User-defined Literals for std::complex"
#define D_ENV_CPP_FEATURE_STL_COMPLEX_UDLS_VERS "(C++14)"

// 2.2    C++17 library features
//------------------------------------------------------------------------------
// 2.2.1
// D_ENV_CPP_FEATURE_STL_ADDRESSOF_CONSTEXPR
//   feature: 1 if __cpp_lib_addressof_constexpr is defined, 0 otherwise.
#ifdef __cpp_lib_addressof_constexpr
    #define D_ENV_CPP_FEATURE_STL_ADDRESSOF_CONSTEXPR 1
    #define D_ENV_CPP_FEATURE_STL_ADDRESSOF_CONSTEXPR_VAL                      \
        __cpp_lib_addressof_constexpr
#else
    #define D_ENV_CPP_FEATURE_STL_ADDRESSOF_CONSTEXPR 0
    #define D_ENV_CPP_FEATURE_STL_ADDRESSOF_CONSTEXPR_VAL 0L
#endif  // __cpp_lib_addressof_constexpr
#define D_ENV_CPP_FEATURE_STL_ADDRESSOF_CONSTEXPR_NAME                         \
    "__cpp_lib_addressof_constexpr"
#define D_ENV_CPP_FEATURE_STL_ADDRESSOF_CONSTEXPR_DESC                         \
    "Constexpr std::addressof"
#define D_ENV_CPP_FEATURE_STL_ADDRESSOF_CONSTEXPR_VERS "(C++17)"

// 2.2.2
// D_ENV_CPP_FEATURE_STL_ANY
//   feature: 1 if __cpp_lib_any is defined, 0 otherwise.
#ifdef __cpp_lib_any
    #define D_ENV_CPP_FEATURE_STL_ANY     1
    #define D_ENV_CPP_FEATURE_STL_ANY_VAL __cpp_lib_any
#else
    #define D_ENV_CPP_FEATURE_STL_ANY     0
    #define D_ENV_CPP_FEATURE_STL_ANY_VAL 0L
#endif  // __cpp_lib_any
#define D_ENV_CPP_FEATURE_STL_ANY_NAME "__cpp_lib_any"
#define D_ENV_CPP_FEATURE_STL_ANY_DESC "std::any"
#define D_ENV_CPP_FEATURE_STL_ANY_VERS "(C++17)"

// 2.2.3
// D_ENV_CPP_FEATURE_STL_APPLY
//   feature: 1 if __cpp_lib_apply is defined, 0 otherwise.
#ifdef __cpp_lib_apply
    #define D_ENV_CPP_FEATURE_STL_APPLY     1
    #define D_ENV_CPP_FEATURE_STL_APPLY_VAL __cpp_lib_apply
#else
    #define D_ENV_CPP_FEATURE_STL_APPLY     0
    #define D_ENV_CPP_FEATURE_STL_APPLY_VAL 0L
#endif  // __cpp_lib_apply
#define D_ENV_CPP_FEATURE_STL_APPLY_NAME "__cpp_lib_apply"
#define D_ENV_CPP_FEATURE_STL_APPLY_DESC "std::apply"
#define D_ENV_CPP_FEATURE_STL_APPLY_VERS "(C++17)"

// 2.2.4
// D_ENV_CPP_FEATURE_STL_ARRAY_CONSTEXPR
//   feature: 1 if __cpp_lib_array_constexpr is defined, 0 otherwise.
#ifdef __cpp_lib_array_constexpr
    #define D_ENV_CPP_FEATURE_STL_ARRAY_CONSTEXPR     1
    #define D_ENV_CPP_FEATURE_STL_ARRAY_CONSTEXPR_VAL __cpp_lib_array_constexpr
#else
    #define D_ENV_CPP_FEATURE_STL_ARRAY_CONSTEXPR     0
    #define D_ENV_CPP_FEATURE_STL_ARRAY_CONSTEXPR_VAL 0L
#endif  // __cpp_lib_array_constexpr
#define D_ENV_CPP_FEATURE_STL_ARRAY_CONSTEXPR_NAME "__cpp_lib_array_constexpr"
#define D_ENV_CPP_FEATURE_STL_ARRAY_CONSTEXPR_DESC                             \
    "Constexpr for std::reverse_iterator, std::move_iterator, std::array"
#define D_ENV_CPP_FEATURE_STL_ARRAY_CONSTEXPR_VERS "(C++17)"

// 2.2.5
// D_ENV_CPP_FEATURE_STL_AS_CONST
//   feature: 1 if __cpp_lib_as_const is defined, 0 otherwise.
#ifdef __cpp_lib_as_const
    #define D_ENV_CPP_FEATURE_STL_AS_CONST     1
    #define D_ENV_CPP_FEATURE_STL_AS_CONST_VAL __cpp_lib_as_const
#else
    #define D_ENV_CPP_FEATURE_STL_AS_CONST     0
    #define D_ENV_CPP_FEATURE_STL_AS_CONST_VAL 0L
#endif  // __cpp_lib_as_const
#define D_ENV_CPP_FEATURE_STL_AS_CONST_NAME "__cpp_lib_as_const"
#define D_ENV_CPP_FEATURE_STL_AS_CONST_DESC "std::as_const"
#define D_ENV_CPP_FEATURE_STL_AS_CONST_VERS "(C++17)"

// 2.2.6
// D_ENV_CPP_FEATURE_STL_BOOL_CONSTANT
//   feature: 1 if __cpp_lib_bool_constant is defined, 0 otherwise.
#ifdef __cpp_lib_bool_constant
    #define D_ENV_CPP_FEATURE_STL_BOOL_CONSTANT     1
    #define D_ENV_CPP_FEATURE_STL_BOOL_CONSTANT_VAL __cpp_lib_bool_constant
#else
    #define D_ENV_CPP_FEATURE_STL_BOOL_CONSTANT     0
    #define D_ENV_CPP_FEATURE_STL_BOOL_CONSTANT_VAL 0L
#endif  // __cpp_lib_bool_constant
#define D_ENV_CPP_FEATURE_STL_BOOL_CONSTANT_NAME "__cpp_lib_bool_constant"
#define D_ENV_CPP_FEATURE_STL_BOOL_CONSTANT_DESC "std::bool_constant"
#define D_ENV_CPP_FEATURE_STL_BOOL_CONSTANT_VERS "(C++17)"

// 2.2.7
// D_ENV_CPP_FEATURE_STL_BOYER_MOORE_SEARCHER
//   feature: 1 if __cpp_lib_boyer_moore_searcher is defined, 0 otherwise.
#ifdef __cpp_lib_boyer_moore_searcher
    #define D_ENV_CPP_FEATURE_STL_BOYER_MOORE_SEARCHER 1
    #define D_ENV_CPP_FEATURE_STL_BOYER_MOORE_SEARCHER_VAL                     \
        __cpp_lib_boyer_moore_searcher
#else
    #define D_ENV_CPP_FEATURE_STL_BOYER_MOORE_SEARCHER 0
    #define D_ENV_CPP_FEATURE_STL_BOYER_MOORE_SEARCHER_VAL 0L
#endif  // __cpp_lib_boyer_moore_searcher
#define D_ENV_CPP_FEATURE_STL_BOYER_MOORE_SEARCHER_NAME                        \
    "__cpp_lib_boyer_moore_searcher"
#define D_ENV_CPP_FEATURE_STL_BOYER_MOORE_SEARCHER_DESC "Searchers"
#define D_ENV_CPP_FEATURE_STL_BOYER_MOORE_SEARCHER_VERS "(C++17)"

// 2.2.8
// D_ENV_CPP_FEATURE_STL_BYTE
//   feature: 1 if __cpp_lib_byte is defined, 0 otherwise.
#ifdef __cpp_lib_byte
    #define D_ENV_CPP_FEATURE_STL_BYTE     1
    #define D_ENV_CPP_FEATURE_STL_BYTE_VAL __cpp_lib_byte
#else
    #define D_ENV_CPP_FEATURE_STL_BYTE     0
    #define D_ENV_CPP_FEATURE_STL_BYTE_VAL 0L
#endif  // __cpp_lib_byte
#define D_ENV_CPP_FEATURE_STL_BYTE_NAME "__cpp_lib_byte"
#define D_ENV_CPP_FEATURE_STL_BYTE_DESC "std::byte"
#define D_ENV_CPP_FEATURE_STL_BYTE_VERS "(C++17)"

// 2.2.9
// D_ENV_CPP_FEATURE_STL_CLAMP
//   feature: 1 if __cpp_lib_clamp is defined, 0 otherwise.
#ifdef __cpp_lib_clamp
    #define D_ENV_CPP_FEATURE_STL_CLAMP     1
    #define D_ENV_CPP_FEATURE_STL_CLAMP_VAL __cpp_lib_clamp
#else
    #define D_ENV_CPP_FEATURE_STL_CLAMP     0
    #define D_ENV_CPP_FEATURE_STL_CLAMP_VAL 0L
#endif  // __cpp_lib_clamp
#define D_ENV_CPP_FEATURE_STL_CLAMP_NAME "__cpp_lib_clamp"
#define D_ENV_CPP_FEATURE_STL_CLAMP_DESC "std::clamp"
#define D_ENV_CPP_FEATURE_STL_CLAMP_VERS "(C++17)"

// 2.2.10
// D_ENV_CPP_FEATURE_STL_FILESYSTEM
//   feature: 1 if __cpp_lib_filesystem is defined, 0 otherwise.
#ifdef __cpp_lib_filesystem
    #define D_ENV_CPP_FEATURE_STL_FILESYSTEM     1
    #define D_ENV_CPP_FEATURE_STL_FILESYSTEM_VAL __cpp_lib_filesystem
#else
    #define D_ENV_CPP_FEATURE_STL_FILESYSTEM     0
    #define D_ENV_CPP_FEATURE_STL_FILESYSTEM_VAL 0L
#endif  // __cpp_lib_filesystem
#define D_ENV_CPP_FEATURE_STL_FILESYSTEM_NAME "__cpp_lib_filesystem"
#define D_ENV_CPP_FEATURE_STL_FILESYSTEM_DESC "Filesystem library"
#define D_ENV_CPP_FEATURE_STL_FILESYSTEM_VERS "(C++17)"

// 2.2.11
// D_ENV_CPP_FEATURE_STL_OPTIONAL
//   feature: 1 if __cpp_lib_optional is defined, 0 otherwise.
#ifdef __cpp_lib_optional
    #define D_ENV_CPP_FEATURE_STL_OPTIONAL     1
    #define D_ENV_CPP_FEATURE_STL_OPTIONAL_VAL __cpp_lib_optional
#else
    #define D_ENV_CPP_FEATURE_STL_OPTIONAL     0
    #define D_ENV_CPP_FEATURE_STL_OPTIONAL_VAL 0L
#endif  // __cpp_lib_optional
#define D_ENV_CPP_FEATURE_STL_OPTIONAL_NAME "__cpp_lib_optional"
#define D_ENV_CPP_FEATURE_STL_OPTIONAL_DESC "std::optional"
#define D_ENV_CPP_FEATURE_STL_OPTIONAL_VERS "(C++17)"

// 2.2.12
// D_ENV_CPP_FEATURE_STL_VARIANT
//   feature: 1 if __cpp_lib_variant is defined, 0 otherwise.
#ifdef __cpp_lib_variant
    #define D_ENV_CPP_FEATURE_STL_VARIANT     1
    #define D_ENV_CPP_FEATURE_STL_VARIANT_VAL __cpp_lib_variant
#else
    #define D_ENV_CPP_FEATURE_STL_VARIANT     0
    #define D_ENV_CPP_FEATURE_STL_VARIANT_VAL 0L
#endif  // __cpp_lib_variant
#define D_ENV_CPP_FEATURE_STL_VARIANT_NAME "__cpp_lib_variant"
#define D_ENV_CPP_FEATURE_STL_VARIANT_DESC "std::variant"
#define D_ENV_CPP_FEATURE_STL_VARIANT_VERS "(C++17)"

// 2.3    C++20 library features
//------------------------------------------------------------------------------
// 2.3.1
// D_ENV_CPP_FEATURE_STL_ASSUME_ALIGNED
//   feature: 1 if __cpp_lib_assume_aligned is defined, 0 otherwise.
#ifdef __cpp_lib_assume_aligned
    #define D_ENV_CPP_FEATURE_STL_ASSUME_ALIGNED     1
    #define D_ENV_CPP_FEATURE_STL_ASSUME_ALIGNED_VAL __cpp_lib_assume_aligned
#else
    #define D_ENV_CPP_FEATURE_STL_ASSUME_ALIGNED     0
    #define D_ENV_CPP_FEATURE_STL_ASSUME_ALIGNED_VAL 0L
#endif  // __cpp_lib_assume_aligned
#define D_ENV_CPP_FEATURE_STL_ASSUME_ALIGNED_NAME "__cpp_lib_assume_aligned"
#define D_ENV_CPP_FEATURE_STL_ASSUME_ALIGNED_DESC "std::assume_aligned"
#define D_ENV_CPP_FEATURE_STL_ASSUME_ALIGNED_VERS "(C++20)"

// 2.3.2
// D_ENV_CPP_FEATURE_STL_ATOMIC_FLAG_TEST
//   feature: 1 if __cpp_lib_atomic_flag_test is defined, 0 otherwise.
#ifdef __cpp_lib_atomic_flag_test
    #define D_ENV_CPP_FEATURE_STL_ATOMIC_FLAG_TEST 1
    #define D_ENV_CPP_FEATURE_STL_ATOMIC_FLAG_TEST_VAL                         \
        __cpp_lib_atomic_flag_test
#else
    #define D_ENV_CPP_FEATURE_STL_ATOMIC_FLAG_TEST 0
    #define D_ENV_CPP_FEATURE_STL_ATOMIC_FLAG_TEST_VAL 0L
#endif  // __cpp_lib_atomic_flag_test
#define D_ENV_CPP_FEATURE_STL_ATOMIC_FLAG_TEST_NAME "__cpp_lib_atomic_flag_test"
#define D_ENV_CPP_FEATURE_STL_ATOMIC_FLAG_TEST_DESC "std::atomic_flag::test"
#define D_ENV_CPP_FEATURE_STL_ATOMIC_FLAG_TEST_VERS "(C++20)"

// 2.3.3
// D_ENV_CPP_FEATURE_STL_ATOMIC_FLOAT
//   feature: 1 if __cpp_lib_atomic_float is defined, 0 otherwise.
#ifdef __cpp_lib_atomic_float
    #define D_ENV_CPP_FEATURE_STL_ATOMIC_FLOAT     1
    #define D_ENV_CPP_FEATURE_STL_ATOMIC_FLOAT_VAL __cpp_lib_atomic_float
#else
    #define D_ENV_CPP_FEATURE_STL_ATOMIC_FLOAT     0
    #define D_ENV_CPP_FEATURE_STL_ATOMIC_FLOAT_VAL 0L
#endif  // __cpp_lib_atomic_float
#define D_ENV_CPP_FEATURE_STL_ATOMIC_FLOAT_NAME "__cpp_lib_atomic_float"
#define D_ENV_CPP_FEATURE_STL_ATOMIC_FLOAT_DESC "Floating-point atomic"
#define D_ENV_CPP_FEATURE_STL_ATOMIC_FLOAT_VERS "(C++20)"

// 2.3.4
// D_ENV_CPP_FEATURE_STL_ATOMIC_REF
//   feature: 1 if __cpp_lib_atomic_ref is defined, 0 otherwise.
#ifdef __cpp_lib_atomic_ref
    #define D_ENV_CPP_FEATURE_STL_ATOMIC_REF     1
    #define D_ENV_CPP_FEATURE_STL_ATOMIC_REF_VAL __cpp_lib_atomic_ref
#else
    #define D_ENV_CPP_FEATURE_STL_ATOMIC_REF     0
    #define D_ENV_CPP_FEATURE_STL_ATOMIC_REF_VAL 0L
#endif  // __cpp_lib_atomic_ref
#define D_ENV_CPP_FEATURE_STL_ATOMIC_REF_NAME "__cpp_lib_atomic_ref"
#define D_ENV_CPP_FEATURE_STL_ATOMIC_REF_DESC "std::atomic_ref"
#define D_ENV_CPP_FEATURE_STL_ATOMIC_REF_VERS "(C++20)"

// 2.3.5
// D_ENV_CPP_FEATURE_STL_ATOMIC_WAIT
//   feature: 1 if __cpp_lib_atomic_wait is defined, 0 otherwise.
#ifdef __cpp_lib_atomic_wait
    #define D_ENV_CPP_FEATURE_STL_ATOMIC_WAIT     1
    #define D_ENV_CPP_FEATURE_STL_ATOMIC_WAIT_VAL __cpp_lib_atomic_wait
#else
    #define D_ENV_CPP_FEATURE_STL_ATOMIC_WAIT     0
    #define D_ENV_CPP_FEATURE_STL_ATOMIC_WAIT_VAL 0L
#endif  // __cpp_lib_atomic_wait
#define D_ENV_CPP_FEATURE_STL_ATOMIC_WAIT_NAME "__cpp_lib_atomic_wait"
#define D_ENV_CPP_FEATURE_STL_ATOMIC_WAIT_DESC "Efficient std::atomic waiting"
#define D_ENV_CPP_FEATURE_STL_ATOMIC_WAIT_VERS "(C++20)"

// 2.3.6
// D_ENV_CPP_FEATURE_STL_BARRIER
//   feature: 1 if __cpp_lib_barrier is defined, 0 otherwise.
#ifdef __cpp_lib_barrier
    #define D_ENV_CPP_FEATURE_STL_BARRIER     1
    #define D_ENV_CPP_FEATURE_STL_BARRIER_VAL __cpp_lib_barrier
#else
    #define D_ENV_CPP_FEATURE_STL_BARRIER     0
    #define D_ENV_CPP_FEATURE_STL_BARRIER_VAL 0L
#endif  // __cpp_lib_barrier
#define D_ENV_CPP_FEATURE_STL_BARRIER_NAME "__cpp_lib_barrier"
#define D_ENV_CPP_FEATURE_STL_BARRIER_DESC "std::barrier"
#define D_ENV_CPP_FEATURE_STL_BARRIER_VERS "(C++20)"

// 2.3.7
// D_ENV_CPP_FEATURE_STL_BIND_FRONT
//   feature: 1 if __cpp_lib_bind_front is defined, 0 otherwise.
#ifdef __cpp_lib_bind_front
    #define D_ENV_CPP_FEATURE_STL_BIND_FRONT     1
    #define D_ENV_CPP_FEATURE_STL_BIND_FRONT_VAL __cpp_lib_bind_front
#else
    #define D_ENV_CPP_FEATURE_STL_BIND_FRONT     0
    #define D_ENV_CPP_FEATURE_STL_BIND_FRONT_VAL 0L
#endif  // __cpp_lib_bind_front
#define D_ENV_CPP_FEATURE_STL_BIND_FRONT_NAME "__cpp_lib_bind_front"
#define D_ENV_CPP_FEATURE_STL_BIND_FRONT_DESC "std::bind_front"
#define D_ENV_CPP_FEATURE_STL_BIND_FRONT_VERS "(C++20)"

// 2.3.8
// D_ENV_CPP_FEATURE_STL_BIT_CAST
//   feature: 1 if __cpp_lib_bit_cast is defined, 0 otherwise.
#ifdef __cpp_lib_bit_cast
    #define D_ENV_CPP_FEATURE_STL_BIT_CAST     1
    #define D_ENV_CPP_FEATURE_STL_BIT_CAST_VAL __cpp_lib_bit_cast
#else
    #define D_ENV_CPP_FEATURE_STL_BIT_CAST     0
    #define D_ENV_CPP_FEATURE_STL_BIT_CAST_VAL 0L
#endif  // __cpp_lib_bit_cast
#define D_ENV_CPP_FEATURE_STL_BIT_CAST_NAME "__cpp_lib_bit_cast"
#define D_ENV_CPP_FEATURE_STL_BIT_CAST_DESC "std::bit_cast"
#define D_ENV_CPP_FEATURE_STL_BIT_CAST_VERS "(C++20)"

// 2.3.9
// D_ENV_CPP_FEATURE_STL_BITOPS
//   feature: 1 if __cpp_lib_bitops is defined, 0 otherwise.
#ifdef __cpp_lib_bitops
    #define D_ENV_CPP_FEATURE_STL_BITOPS     1
    #define D_ENV_CPP_FEATURE_STL_BITOPS_VAL __cpp_lib_bitops
#else
    #define D_ENV_CPP_FEATURE_STL_BITOPS     0
    #define D_ENV_CPP_FEATURE_STL_BITOPS_VAL 0L
#endif  // __cpp_lib_bitops
#define D_ENV_CPP_FEATURE_STL_BITOPS_NAME "__cpp_lib_bitops"
#define D_ENV_CPP_FEATURE_STL_BITOPS_DESC "Bit operations"
#define D_ENV_CPP_FEATURE_STL_BITOPS_VERS "(C++20)"

// 2.3.10
// D_ENV_CPP_FEATURE_STL_BOUNDED_ARRAY_TRAITS
//   feature: 1 if __cpp_lib_bounded_array_traits is defined, 0 otherwise.
#ifdef __cpp_lib_bounded_array_traits
    #define D_ENV_CPP_FEATURE_STL_BOUNDED_ARRAY_TRAITS 1
    #define D_ENV_CPP_FEATURE_STL_BOUNDED_ARRAY_TRAITS_VAL                     \
        __cpp_lib_bounded_array_traits
#else
    #define D_ENV_CPP_FEATURE_STL_BOUNDED_ARRAY_TRAITS 0
    #define D_ENV_CPP_FEATURE_STL_BOUNDED_ARRAY_TRAITS_VAL 0L
#endif  // __cpp_lib_bounded_array_traits
#define D_ENV_CPP_FEATURE_STL_BOUNDED_ARRAY_TRAITS_NAME                        \
    "__cpp_lib_bounded_array_traits"
#define D_ENV_CPP_FEATURE_STL_BOUNDED_ARRAY_TRAITS_DESC                        \
    "std::is_bounded_array, std::is_unbounded_array"
#define D_ENV_CPP_FEATURE_STL_BOUNDED_ARRAY_TRAITS_VERS "(C++20)"

// 2.3.11
// D_ENV_CPP_FEATURE_STL_CHAR8_T
//   feature: 1 if __cpp_lib_char8_t is defined, 0 otherwise.
#ifdef __cpp_lib_char8_t
    #define D_ENV_CPP_FEATURE_STL_CHAR8_T     1
    #define D_ENV_CPP_FEATURE_STL_CHAR8_T_VAL __cpp_lib_char8_t
#else
    #define D_ENV_CPP_FEATURE_STL_CHAR8_T     0
    #define D_ENV_CPP_FEATURE_STL_CHAR8_T_VAL 0L
#endif  // __cpp_lib_char8_t
#define D_ENV_CPP_FEATURE_STL_CHAR8_T_NAME "__cpp_lib_char8_t"
#define D_ENV_CPP_FEATURE_STL_CHAR8_T_DESC "Library support for char8_t"
#define D_ENV_CPP_FEATURE_STL_CHAR8_T_VERS "(C++20)"

// 2.3.12
// D_ENV_CPP_FEATURE_STL_CONCEPTS
//   feature: 1 if __cpp_lib_concepts is defined, 0 otherwise.
#ifdef __cpp_lib_concepts
    #define D_ENV_CPP_FEATURE_STL_CONCEPTS     1
    #define D_ENV_CPP_FEATURE_STL_CONCEPTS_VAL __cpp_lib_concepts
#else
    #define D_ENV_CPP_FEATURE_STL_CONCEPTS     0
    #define D_ENV_CPP_FEATURE_STL_CONCEPTS_VAL 0L
#endif  // __cpp_lib_concepts
#define D_ENV_CPP_FEATURE_STL_CONCEPTS_NAME "__cpp_lib_concepts"
#define D_ENV_CPP_FEATURE_STL_CONCEPTS_DESC "Standard library concepts"
#define D_ENV_CPP_FEATURE_STL_CONCEPTS_VERS "(C++20)"

// 2.3.13
// D_ENV_CPP_FEATURE_STL_CONSTEXPR_ALGORITHMS
//   feature: 1 if __cpp_lib_constexpr_algorithms is defined, 0 otherwise.
#ifdef __cpp_lib_constexpr_algorithms
    #define D_ENV_CPP_FEATURE_STL_CONSTEXPR_ALGORITHMS 1
    #define D_ENV_CPP_FEATURE_STL_CONSTEXPR_ALGORITHMS_VAL                     \
        __cpp_lib_constexpr_algorithms
#else
    #define D_ENV_CPP_FEATURE_STL_CONSTEXPR_ALGORITHMS 0
    #define D_ENV_CPP_FEATURE_STL_CONSTEXPR_ALGORITHMS_VAL 0L
#endif  // __cpp_lib_constexpr_algorithms
#define D_ENV_CPP_FEATURE_STL_CONSTEXPR_ALGORITHMS_NAME                        \
    "__cpp_lib_constexpr_algorithms"
#define D_ENV_CPP_FEATURE_STL_CONSTEXPR_ALGORITHMS_DESC                        \
    "Constexpr for algorithms"
#define D_ENV_CPP_FEATURE_STL_CONSTEXPR_ALGORITHMS_VERS "(C++20)"

// 2.3.14
// D_ENV_CPP_FEATURE_STL_CONSTEXPR_COMPLEX
//   feature: 1 if __cpp_lib_constexpr_complex is defined, 0 otherwise.
#ifdef __cpp_lib_constexpr_complex
    #define D_ENV_CPP_FEATURE_STL_CONSTEXPR_COMPLEX 1
    #define D_ENV_CPP_FEATURE_STL_CONSTEXPR_COMPLEX_VAL                        \
        __cpp_lib_constexpr_complex
#else
    #define D_ENV_CPP_FEATURE_STL_CONSTEXPR_COMPLEX 0
    #define D_ENV_CPP_FEATURE_STL_CONSTEXPR_COMPLEX_VAL 0L
#endif  // __cpp_lib_constexpr_complex
#define D_ENV_CPP_FEATURE_STL_CONSTEXPR_COMPLEX_NAME                           \
    "__cpp_lib_constexpr_complex"
#define D_ENV_CPP_FEATURE_STL_CONSTEXPR_COMPLEX_DESC                           \
    "Constexpr for std::complex"
#define D_ENV_CPP_FEATURE_STL_CONSTEXPR_COMPLEX_VERS "(C++20)"

// 2.3.15
// D_ENV_CPP_FEATURE_STL_CONSTEXPR_DYNAMIC_ALLOC
//   feature: 1 if __cpp_lib_constexpr_dynamic_alloc is defined, 0 otherwise.
#ifdef __cpp_lib_constexpr_dynamic_alloc
    #define D_ENV_CPP_FEATURE_STL_CONSTEXPR_DYNAMIC_ALLOC 1
    #define D_ENV_CPP_FEATURE_STL_CONSTEXPR_DYNAMIC_ALLOC_VAL                  \
        __cpp_lib_constexpr_dynamic_alloc
#else
    #define D_ENV_CPP_FEATURE_STL_CONSTEXPR_DYNAMIC_ALLOC 0
    #define D_ENV_CPP_FEATURE_STL_CONSTEXPR_DYNAMIC_ALLOC_VAL 0L
#endif  // __cpp_lib_constexpr_dynamic_alloc
#define D_ENV_CPP_FEATURE_STL_CONSTEXPR_DYNAMIC_ALLOC_NAME                     \
    "__cpp_lib_constexpr_dynamic_alloc"
#define D_ENV_CPP_FEATURE_STL_CONSTEXPR_DYNAMIC_ALLOC_DESC                     \
    "Constexpr for std::allocator and related utilities"
#define D_ENV_CPP_FEATURE_STL_CONSTEXPR_DYNAMIC_ALLOC_VERS "(C++20)"

// 2.3.16
// D_ENV_CPP_FEATURE_STL_CONSTEXPR_STRING
//   feature: 1 if __cpp_lib_constexpr_string is defined, 0 otherwise.
#ifdef __cpp_lib_constexpr_string
    #define D_ENV_CPP_FEATURE_STL_CONSTEXPR_STRING 1
    #define D_ENV_CPP_FEATURE_STL_CONSTEXPR_STRING_VAL                         \
        __cpp_lib_constexpr_string
#else
    #define D_ENV_CPP_FEATURE_STL_CONSTEXPR_STRING 0
    #define D_ENV_CPP_FEATURE_STL_CONSTEXPR_STRING_VAL 0L
#endif  // __cpp_lib_constexpr_string
#define D_ENV_CPP_FEATURE_STL_CONSTEXPR_STRING_NAME "__cpp_lib_constexpr_string"
#define D_ENV_CPP_FEATURE_STL_CONSTEXPR_STRING_DESC "constexpr std::string"
#define D_ENV_CPP_FEATURE_STL_CONSTEXPR_STRING_VERS "(C++20)"

// 2.3.17
// D_ENV_CPP_FEATURE_STL_CONSTEXPR_VECTOR
//   feature: 1 if __cpp_lib_constexpr_vector is defined, 0 otherwise.
#ifdef __cpp_lib_constexpr_vector
    #define D_ENV_CPP_FEATURE_STL_CONSTEXPR_VECTOR 1
    #define D_ENV_CPP_FEATURE_STL_CONSTEXPR_VECTOR_VAL                         \
        __cpp_lib_constexpr_vector
#else
    #define D_ENV_CPP_FEATURE_STL_CONSTEXPR_VECTOR 0
    #define D_ENV_CPP_FEATURE_STL_CONSTEXPR_VECTOR_VAL 0L
#endif  // __cpp_lib_constexpr_vector
#define D_ENV_CPP_FEATURE_STL_CONSTEXPR_VECTOR_NAME "__cpp_lib_constexpr_vector"
#define D_ENV_CPP_FEATURE_STL_CONSTEXPR_VECTOR_DESC "Constexpr for std::vector"
#define D_ENV_CPP_FEATURE_STL_CONSTEXPR_VECTOR_VERS "(C++20)"

// 2.3.18
// D_ENV_CPP_FEATURE_STL_COROUTINE
//   feature: 1 if __cpp_lib_coroutine is defined, 0 otherwise.
#ifdef __cpp_lib_coroutine
    #define D_ENV_CPP_FEATURE_STL_COROUTINE     1
    #define D_ENV_CPP_FEATURE_STL_COROUTINE_VAL __cpp_lib_coroutine
#else
    #define D_ENV_CPP_FEATURE_STL_COROUTINE     0
    #define D_ENV_CPP_FEATURE_STL_COROUTINE_VAL 0L
#endif  // __cpp_lib_coroutine
#define D_ENV_CPP_FEATURE_STL_COROUTINE_NAME "__cpp_lib_coroutine"
#define D_ENV_CPP_FEATURE_STL_COROUTINE_DESC "Coroutines (library support)"
#define D_ENV_CPP_FEATURE_STL_COROUTINE_VERS "(C++20)"

// 2.3.19
// D_ENV_CPP_FEATURE_STL_ENDIAN
//   feature: 1 if __cpp_lib_endian is defined, 0 otherwise.
#ifdef __cpp_lib_endian
    #define D_ENV_CPP_FEATURE_STL_ENDIAN     1
    #define D_ENV_CPP_FEATURE_STL_ENDIAN_VAL __cpp_lib_endian
#else
    #define D_ENV_CPP_FEATURE_STL_ENDIAN     0
    #define D_ENV_CPP_FEATURE_STL_ENDIAN_VAL 0L
#endif  // __cpp_lib_endian
#define D_ENV_CPP_FEATURE_STL_ENDIAN_NAME "__cpp_lib_endian"
#define D_ENV_CPP_FEATURE_STL_ENDIAN_DESC "std::endian"
#define D_ENV_CPP_FEATURE_STL_ENDIAN_VERS "(C++20)"

// 2.3.20
// D_ENV_CPP_FEATURE_STL_FORMAT
//   feature: 1 if __cpp_lib_format is defined, 0 otherwise.
#ifdef __cpp_lib_format
    #define D_ENV_CPP_FEATURE_STL_FORMAT     1
    #define D_ENV_CPP_FEATURE_STL_FORMAT_VAL __cpp_lib_format
#else
    #define D_ENV_CPP_FEATURE_STL_FORMAT     0
    #define D_ENV_CPP_FEATURE_STL_FORMAT_VAL 0L
#endif  // __cpp_lib_format
#define D_ENV_CPP_FEATURE_STL_FORMAT_NAME "__cpp_lib_format"
#define D_ENV_CPP_FEATURE_STL_FORMAT_DESC "Text formatting"
#define D_ENV_CPP_FEATURE_STL_FORMAT_VERS "(C++20)"

// 2.3.21
// D_ENV_CPP_FEATURE_STL_JTHREAD
//   feature: 1 if __cpp_lib_jthread is defined, 0 otherwise.
#ifdef __cpp_lib_jthread
    #define D_ENV_CPP_FEATURE_STL_JTHREAD     1
    #define D_ENV_CPP_FEATURE_STL_JTHREAD_VAL __cpp_lib_jthread
#else
    #define D_ENV_CPP_FEATURE_STL_JTHREAD     0
    #define D_ENV_CPP_FEATURE_STL_JTHREAD_VAL 0L
#endif  // __cpp_lib_jthread
#define D_ENV_CPP_FEATURE_STL_JTHREAD_NAME "__cpp_lib_jthread"
#define D_ENV_CPP_FEATURE_STL_JTHREAD_DESC "Stop token and joining thread"
#define D_ENV_CPP_FEATURE_STL_JTHREAD_VERS "(C++20)"

// 2.3.22
// D_ENV_CPP_FEATURE_STL_LATCH
//   feature: 1 if __cpp_lib_latch is defined, 0 otherwise.
#ifdef __cpp_lib_latch
    #define D_ENV_CPP_FEATURE_STL_LATCH     1
    #define D_ENV_CPP_FEATURE_STL_LATCH_VAL __cpp_lib_latch
#else
    #define D_ENV_CPP_FEATURE_STL_LATCH     0
    #define D_ENV_CPP_FEATURE_STL_LATCH_VAL 0L
#endif  // __cpp_lib_latch
#define D_ENV_CPP_FEATURE_STL_LATCH_NAME "__cpp_lib_latch"
#define D_ENV_CPP_FEATURE_STL_LATCH_DESC "std::latch"
#define D_ENV_CPP_FEATURE_STL_LATCH_VERS "(C++20)"

// 2.3.23
// D_ENV_CPP_FEATURE_STL_MATH_CONSTANTS
//   feature: 1 if __cpp_lib_math_constants is defined, 0 otherwise.
#ifdef __cpp_lib_math_constants
    #define D_ENV_CPP_FEATURE_STL_MATH_CONSTANTS     1
    #define D_ENV_CPP_FEATURE_STL_MATH_CONSTANTS_VAL __cpp_lib_math_constants
#else
    #define D_ENV_CPP_FEATURE_STL_MATH_CONSTANTS     0
    #define D_ENV_CPP_FEATURE_STL_MATH_CONSTANTS_VAL 0L
#endif  // __cpp_lib_math_constants
#define D_ENV_CPP_FEATURE_STL_MATH_CONSTANTS_NAME "__cpp_lib_math_constants"
#define D_ENV_CPP_FEATURE_STL_MATH_CONSTANTS_DESC "Mathematical constants"
#define D_ENV_CPP_FEATURE_STL_MATH_CONSTANTS_VERS "(C++20)"

// 2.3.24
// D_ENV_CPP_FEATURE_STL_RANGES
//   feature: 1 if __cpp_lib_ranges is defined, 0 otherwise.
#ifdef __cpp_lib_ranges
    #define D_ENV_CPP_FEATURE_STL_RANGES     1
    #define D_ENV_CPP_FEATURE_STL_RANGES_VAL __cpp_lib_ranges
#else
    #define D_ENV_CPP_FEATURE_STL_RANGES     0
    #define D_ENV_CPP_FEATURE_STL_RANGES_VAL 0L
#endif  // __cpp_lib_ranges
#define D_ENV_CPP_FEATURE_STL_RANGES_NAME "__cpp_lib_ranges"
#define D_ENV_CPP_FEATURE_STL_RANGES_DESC                                      \
    "Ranges library and constrained algorithms"
#define D_ENV_CPP_FEATURE_STL_RANGES_VERS "(C++20)"

// 2.3.25
// D_ENV_CPP_FEATURE_STL_SEMAPHORE
//   feature: 1 if __cpp_lib_semaphore is defined, 0 otherwise.
#ifdef __cpp_lib_semaphore
    #define D_ENV_CPP_FEATURE_STL_SEMAPHORE     1
    #define D_ENV_CPP_FEATURE_STL_SEMAPHORE_VAL __cpp_lib_semaphore
#else
    #define D_ENV_CPP_FEATURE_STL_SEMAPHORE     0
    #define D_ENV_CPP_FEATURE_STL_SEMAPHORE_VAL 0L
#endif  // __cpp_lib_semaphore
#define D_ENV_CPP_FEATURE_STL_SEMAPHORE_NAME "__cpp_lib_semaphore"
#define D_ENV_CPP_FEATURE_STL_SEMAPHORE_DESC                                   \
    "std::counting_semaphore, std::binary_semaphore"
#define D_ENV_CPP_FEATURE_STL_SEMAPHORE_VERS "(C++20)"

// 2.3.26
// D_ENV_CPP_FEATURE_STL_SOURCE_LOCATION
//   feature: 1 if __cpp_lib_source_location is defined, 0 otherwise.
#ifdef __cpp_lib_source_location
    #define D_ENV_CPP_FEATURE_STL_SOURCE_LOCATION     1
    #define D_ENV_CPP_FEATURE_STL_SOURCE_LOCATION_VAL __cpp_lib_source_location
#else
    #define D_ENV_CPP_FEATURE_STL_SOURCE_LOCATION     0
    #define D_ENV_CPP_FEATURE_STL_SOURCE_LOCATION_VAL 0L
#endif  // __cpp_lib_source_location
#define D_ENV_CPP_FEATURE_STL_SOURCE_LOCATION_NAME "__cpp_lib_source_location"
#define D_ENV_CPP_FEATURE_STL_SOURCE_LOCATION_DESC                             \
    "Source-code information capture"
#define D_ENV_CPP_FEATURE_STL_SOURCE_LOCATION_VERS "(C++20)"

// 2.3.27
// D_ENV_CPP_FEATURE_STL_SPAN
//   feature: 1 if __cpp_lib_span is defined, 0 otherwise.
#ifdef __cpp_lib_span
    #define D_ENV_CPP_FEATURE_STL_SPAN     1
    #define D_ENV_CPP_FEATURE_STL_SPAN_VAL __cpp_lib_span
#else
    #define D_ENV_CPP_FEATURE_STL_SPAN     0
    #define D_ENV_CPP_FEATURE_STL_SPAN_VAL 0L
#endif  // __cpp_lib_span
#define D_ENV_CPP_FEATURE_STL_SPAN_NAME "__cpp_lib_span"
#define D_ENV_CPP_FEATURE_STL_SPAN_DESC "std::span"
#define D_ENV_CPP_FEATURE_STL_SPAN_VERS "(C++20)"

// 2.3.28
// D_ENV_CPP_FEATURE_STL_THREE_WAY_COMPARISON
//   feature: 1 if __cpp_lib_three_way_comparison is defined, 0 otherwise.
#ifdef __cpp_lib_three_way_comparison
    #define D_ENV_CPP_FEATURE_STL_THREE_WAY_COMPARISON 1
    #define D_ENV_CPP_FEATURE_STL_THREE_WAY_COMPARISON_VAL                     \
        __cpp_lib_three_way_comparison
#else
    #define D_ENV_CPP_FEATURE_STL_THREE_WAY_COMPARISON 0
    #define D_ENV_CPP_FEATURE_STL_THREE_WAY_COMPARISON_VAL 0L
#endif  // __cpp_lib_three_way_comparison
#define D_ENV_CPP_FEATURE_STL_THREE_WAY_COMPARISON_NAME                        \
    "__cpp_lib_three_way_comparison"
#define D_ENV_CPP_FEATURE_STL_THREE_WAY_COMPARISON_DESC                        \
    "Three-way comparison (library support)"
#define D_ENV_CPP_FEATURE_STL_THREE_WAY_COMPARISON_VERS "(C++20)"

// 2.3.29
// D_ENV_CPP_FEATURE_STL_TO_ARRAY
//   feature: 1 if __cpp_lib_to_array is defined, 0 otherwise.
#ifdef __cpp_lib_to_array
    #define D_ENV_CPP_FEATURE_STL_TO_ARRAY     1
    #define D_ENV_CPP_FEATURE_STL_TO_ARRAY_VAL __cpp_lib_to_array
#else
    #define D_ENV_CPP_FEATURE_STL_TO_ARRAY     0
    #define D_ENV_CPP_FEATURE_STL_TO_ARRAY_VAL 0L
#endif  // __cpp_lib_to_array
#define D_ENV_CPP_FEATURE_STL_TO_ARRAY_NAME "__cpp_lib_to_array"
#define D_ENV_CPP_FEATURE_STL_TO_ARRAY_DESC "std::to_array"
#define D_ENV_CPP_FEATURE_STL_TO_ARRAY_VERS "(C++20)"

// 2.3.30
// D_ENV_CPP_FEATURE_STL_IS_CONSTANT_EVALUATED
//   feature: 1 if __cpp_lib_is_constant_evaluated is defined, 0 otherwise.
// math/ and parse/ test it, and it was missing, so their #if read an
// undefined name as 0 at every level (-Wundef).
#ifdef __cpp_lib_is_constant_evaluated
    #define D_ENV_CPP_FEATURE_STL_IS_CONSTANT_EVALUATED     1
    #define D_ENV_CPP_FEATURE_STL_IS_CONSTANT_EVALUATED_VAL                    \
        __cpp_lib_is_constant_evaluated
#else
    #define D_ENV_CPP_FEATURE_STL_IS_CONSTANT_EVALUATED     0
    #define D_ENV_CPP_FEATURE_STL_IS_CONSTANT_EVALUATED_VAL 0L
#endif  // __cpp_lib_is_constant_evaluated
#define D_ENV_CPP_FEATURE_STL_IS_CONSTANT_EVALUATED_NAME                       \
    "__cpp_lib_is_constant_evaluated"
#define D_ENV_CPP_FEATURE_STL_IS_CONSTANT_EVALUATED_DESC                       \
    "std::is_constant_evaluated"
#define D_ENV_CPP_FEATURE_STL_IS_CONSTANT_EVALUATED_VERS "(C++20)"

// 2.4    C++23 library features
//------------------------------------------------------------------------------
// 2.4.1
// D_ENV_CPP_FEATURE_STL_ADAPTOR_ITERATOR_PAIR_CONSTRUCTOR
//   feature: 1 if __cpp_lib_adaptor_iterator_pair_constructor is defined, 0
// otherwise.
#ifdef __cpp_lib_adaptor_iterator_pair_constructor
    #define D_ENV_CPP_FEATURE_STL_ADAPTOR_ITERATOR_PAIR_CONSTRUCTOR 1
    #define D_ENV_CPP_FEATURE_STL_ADAPTOR_ITERATOR_PAIR_CONSTRUCTOR_VAL        \
        __cpp_lib_adaptor_iterator_pair_constructor
#else
    #define D_ENV_CPP_FEATURE_STL_ADAPTOR_ITERATOR_PAIR_CONSTRUCTOR 0
    #define D_ENV_CPP_FEATURE_STL_ADAPTOR_ITERATOR_PAIR_CONSTRUCTOR_VAL 0L
#endif  // __cpp_lib_adaptor_iterator_pair_constructor
#define D_ENV_CPP_FEATURE_STL_ADAPTOR_ITERATOR_PAIR_CONSTRUCTOR_NAME           \
    "__cpp_lib_adaptor_iterator_pair_constructor"
#define D_ENV_CPP_FEATURE_STL_ADAPTOR_ITERATOR_PAIR_CONSTRUCTOR_DESC           \
    "Iterator pair constructors for std::stack and std::queue"
#define D_ENV_CPP_FEATURE_STL_ADAPTOR_ITERATOR_PAIR_CONSTRUCTOR_VERS "(C++23)"

// 2.4.2
// D_ENV_CPP_FEATURE_STL_ASSOCIATIVE_HETEROGENEOUS_ERASURE
//   feature: 1 if __cpp_lib_associative_heterogeneous_erasure is defined, 0
// otherwise.
#ifdef __cpp_lib_associative_heterogeneous_erasure
    #define D_ENV_CPP_FEATURE_STL_ASSOCIATIVE_HETEROGENEOUS_ERASURE 1
    #define D_ENV_CPP_FEATURE_STL_ASSOCIATIVE_HETEROGENEOUS_ERASURE_VAL        \
        __cpp_lib_associative_heterogeneous_erasure
#else
    #define D_ENV_CPP_FEATURE_STL_ASSOCIATIVE_HETEROGENEOUS_ERASURE 0
    #define D_ENV_CPP_FEATURE_STL_ASSOCIATIVE_HETEROGENEOUS_ERASURE_VAL 0L
#endif  // __cpp_lib_associative_heterogeneous_erasure
#define D_ENV_CPP_FEATURE_STL_ASSOCIATIVE_HETEROGENEOUS_ERASURE_NAME           \
    "__cpp_lib_associative_heterogeneous_erasure"
#define D_ENV_CPP_FEATURE_STL_ASSOCIATIVE_HETEROGENEOUS_ERASURE_DESC           \
    "Heterogeneous erasure in associative containers"
#define D_ENV_CPP_FEATURE_STL_ASSOCIATIVE_HETEROGENEOUS_ERASURE_VERS "(C++23)"

// 2.4.3
// D_ENV_CPP_FEATURE_STL_BIND_BACK
//   feature: 1 if __cpp_lib_bind_back is defined, 0 otherwise.
#ifdef __cpp_lib_bind_back
    #define D_ENV_CPP_FEATURE_STL_BIND_BACK     1
    #define D_ENV_CPP_FEATURE_STL_BIND_BACK_VAL __cpp_lib_bind_back
#else
    #define D_ENV_CPP_FEATURE_STL_BIND_BACK     0
    #define D_ENV_CPP_FEATURE_STL_BIND_BACK_VAL 0L
#endif  // __cpp_lib_bind_back
#define D_ENV_CPP_FEATURE_STL_BIND_BACK_NAME "__cpp_lib_bind_back"
#define D_ENV_CPP_FEATURE_STL_BIND_BACK_DESC "std::bind_back"
#define D_ENV_CPP_FEATURE_STL_BIND_BACK_VERS "(C++23)"

// 2.4.4
// D_ENV_CPP_FEATURE_STL_BYTESWAP
//   feature: 1 if __cpp_lib_byteswap is defined, 0 otherwise.
#ifdef __cpp_lib_byteswap
    #define D_ENV_CPP_FEATURE_STL_BYTESWAP     1
    #define D_ENV_CPP_FEATURE_STL_BYTESWAP_VAL __cpp_lib_byteswap
#else
    #define D_ENV_CPP_FEATURE_STL_BYTESWAP     0
    #define D_ENV_CPP_FEATURE_STL_BYTESWAP_VAL 0L
#endif  // __cpp_lib_byteswap
#define D_ENV_CPP_FEATURE_STL_BYTESWAP_NAME "__cpp_lib_byteswap"
#define D_ENV_CPP_FEATURE_STL_BYTESWAP_DESC "std::byteswap"
#define D_ENV_CPP_FEATURE_STL_BYTESWAP_VERS "(C++23)"

// 2.4.5
// D_ENV_CPP_FEATURE_STL_CONSTEXPR_BITSET
//   feature: 1 if __cpp_lib_constexpr_bitset is defined, 0 otherwise.
#ifdef __cpp_lib_constexpr_bitset
    #define D_ENV_CPP_FEATURE_STL_CONSTEXPR_BITSET 1
    #define D_ENV_CPP_FEATURE_STL_CONSTEXPR_BITSET_VAL                         \
        __cpp_lib_constexpr_bitset
#else
    #define D_ENV_CPP_FEATURE_STL_CONSTEXPR_BITSET 0
    #define D_ENV_CPP_FEATURE_STL_CONSTEXPR_BITSET_VAL 0L
#endif  // __cpp_lib_constexpr_bitset
#define D_ENV_CPP_FEATURE_STL_CONSTEXPR_BITSET_NAME "__cpp_lib_constexpr_bitset"
#define D_ENV_CPP_FEATURE_STL_CONSTEXPR_BITSET_DESC                            \
    "A more constexpr std::bitset"
#define D_ENV_CPP_FEATURE_STL_CONSTEXPR_BITSET_VERS "(C++23)"

// 2.4.6
// D_ENV_CPP_FEATURE_STL_CONSTEXPR_CHARCONV
//   feature: 1 if __cpp_lib_constexpr_charconv is defined, 0 otherwise.
#ifdef __cpp_lib_constexpr_charconv
    #define D_ENV_CPP_FEATURE_STL_CONSTEXPR_CHARCONV 1
    #define D_ENV_CPP_FEATURE_STL_CONSTEXPR_CHARCONV_VAL                       \
        __cpp_lib_constexpr_charconv
#else
    #define D_ENV_CPP_FEATURE_STL_CONSTEXPR_CHARCONV 0
    #define D_ENV_CPP_FEATURE_STL_CONSTEXPR_CHARCONV_VAL 0L
#endif  // __cpp_lib_constexpr_charconv
#define D_ENV_CPP_FEATURE_STL_CONSTEXPR_CHARCONV_NAME                          \
    "__cpp_lib_constexpr_charconv"
#define D_ENV_CPP_FEATURE_STL_CONSTEXPR_CHARCONV_DESC                          \
    "Constexpr for std::to_chars and std::from_chars"
#define D_ENV_CPP_FEATURE_STL_CONSTEXPR_CHARCONV_VERS "(C++23)"

// 2.4.7
// D_ENV_CPP_FEATURE_STL_CONSTEXPR_CMATH
//   feature: 1 if __cpp_lib_constexpr_cmath is defined, 0 otherwise.
#ifdef __cpp_lib_constexpr_cmath
    #define D_ENV_CPP_FEATURE_STL_CONSTEXPR_CMATH     1
    #define D_ENV_CPP_FEATURE_STL_CONSTEXPR_CMATH_VAL __cpp_lib_constexpr_cmath
#else
    #define D_ENV_CPP_FEATURE_STL_CONSTEXPR_CMATH     0
    #define D_ENV_CPP_FEATURE_STL_CONSTEXPR_CMATH_VAL 0L
#endif  // __cpp_lib_constexpr_cmath
#define D_ENV_CPP_FEATURE_STL_CONSTEXPR_CMATH_NAME "__cpp_lib_constexpr_cmath"
#define D_ENV_CPP_FEATURE_STL_CONSTEXPR_CMATH_DESC                             \
    "Constexpr for mathematical functions in <cmath>"
#define D_ENV_CPP_FEATURE_STL_CONSTEXPR_CMATH_VERS "(C++23)"

// 2.4.8
// D_ENV_CPP_FEATURE_STL_EXPECTED
//   feature: 1 if __cpp_lib_expected is defined, 0 otherwise.
#ifdef __cpp_lib_expected
    #define D_ENV_CPP_FEATURE_STL_EXPECTED     1
    #define D_ENV_CPP_FEATURE_STL_EXPECTED_VAL __cpp_lib_expected
#else
    #define D_ENV_CPP_FEATURE_STL_EXPECTED     0
    #define D_ENV_CPP_FEATURE_STL_EXPECTED_VAL 0L
#endif  // __cpp_lib_expected
#define D_ENV_CPP_FEATURE_STL_EXPECTED_NAME "__cpp_lib_expected"
#define D_ENV_CPP_FEATURE_STL_EXPECTED_DESC "class template std::expected"
#define D_ENV_CPP_FEATURE_STL_EXPECTED_VERS "(C++23)"

// 2.4.9
// D_ENV_CPP_FEATURE_STL_FLAT_MAP
//   feature: 1 if __cpp_lib_flat_map is defined, 0 otherwise.
#ifdef __cpp_lib_flat_map
    #define D_ENV_CPP_FEATURE_STL_FLAT_MAP     1
    #define D_ENV_CPP_FEATURE_STL_FLAT_MAP_VAL __cpp_lib_flat_map
#else
    #define D_ENV_CPP_FEATURE_STL_FLAT_MAP     0
    #define D_ENV_CPP_FEATURE_STL_FLAT_MAP_VAL 0L
#endif  // __cpp_lib_flat_map
#define D_ENV_CPP_FEATURE_STL_FLAT_MAP_NAME "__cpp_lib_flat_map"
#define D_ENV_CPP_FEATURE_STL_FLAT_MAP_DESC                                    \
    "std::flat_map and std::flat_multimap"
#define D_ENV_CPP_FEATURE_STL_FLAT_MAP_VERS "(C++23)"

// 2.4.10
// D_ENV_CPP_FEATURE_STL_FLAT_SET
//   feature: 1 if __cpp_lib_flat_set is defined, 0 otherwise.
#ifdef __cpp_lib_flat_set
    #define D_ENV_CPP_FEATURE_STL_FLAT_SET     1
    #define D_ENV_CPP_FEATURE_STL_FLAT_SET_VAL __cpp_lib_flat_set
#else
    #define D_ENV_CPP_FEATURE_STL_FLAT_SET     0
    #define D_ENV_CPP_FEATURE_STL_FLAT_SET_VAL 0L
#endif  // __cpp_lib_flat_set
#define D_ENV_CPP_FEATURE_STL_FLAT_SET_NAME "__cpp_lib_flat_set"
#define D_ENV_CPP_FEATURE_STL_FLAT_SET_DESC                                    \
    "std::flat_set and std::flat_multiset"
#define D_ENV_CPP_FEATURE_STL_FLAT_SET_VERS "(C++23)"

// 2.4.11
// D_ENV_CPP_FEATURE_STL_GENERATOR
//   feature: 1 if __cpp_lib_generator is defined, 0 otherwise.
#ifdef __cpp_lib_generator
    #define D_ENV_CPP_FEATURE_STL_GENERATOR     1
    #define D_ENV_CPP_FEATURE_STL_GENERATOR_VAL __cpp_lib_generator
#else
    #define D_ENV_CPP_FEATURE_STL_GENERATOR     0
    #define D_ENV_CPP_FEATURE_STL_GENERATOR_VAL 0L
#endif  // __cpp_lib_generator
#define D_ENV_CPP_FEATURE_STL_GENERATOR_NAME "__cpp_lib_generator"
#define D_ENV_CPP_FEATURE_STL_GENERATOR_DESC                                   \
    "std::generator: Synchronous coroutine generator for ranges"
#define D_ENV_CPP_FEATURE_STL_GENERATOR_VERS "(C++23)"

// 2.4.12
// D_ENV_CPP_FEATURE_STL_MDSPAN
//   feature: 1 if __cpp_lib_mdspan is defined, 0 otherwise.
#ifdef __cpp_lib_mdspan
    #define D_ENV_CPP_FEATURE_STL_MDSPAN     1
    #define D_ENV_CPP_FEATURE_STL_MDSPAN_VAL __cpp_lib_mdspan
#else
    #define D_ENV_CPP_FEATURE_STL_MDSPAN     0
    #define D_ENV_CPP_FEATURE_STL_MDSPAN_VAL 0L
#endif  // __cpp_lib_mdspan
#define D_ENV_CPP_FEATURE_STL_MDSPAN_NAME "__cpp_lib_mdspan"
#define D_ENV_CPP_FEATURE_STL_MDSPAN_DESC "std::mdspan"
#define D_ENV_CPP_FEATURE_STL_MDSPAN_VERS "(C++23)"

// 2.4.13
// D_ENV_CPP_FEATURE_STL_MOVE_ONLY_FUNCTION
//   feature: 1 if __cpp_lib_move_only_function is defined, 0 otherwise.
#ifdef __cpp_lib_move_only_function
    #define D_ENV_CPP_FEATURE_STL_MOVE_ONLY_FUNCTION 1
    #define D_ENV_CPP_FEATURE_STL_MOVE_ONLY_FUNCTION_VAL                       \
        __cpp_lib_move_only_function
#else
    #define D_ENV_CPP_FEATURE_STL_MOVE_ONLY_FUNCTION 0
    #define D_ENV_CPP_FEATURE_STL_MOVE_ONLY_FUNCTION_VAL 0L
#endif  // __cpp_lib_move_only_function
#define D_ENV_CPP_FEATURE_STL_MOVE_ONLY_FUNCTION_NAME                          \
    "__cpp_lib_move_only_function"
#define D_ENV_CPP_FEATURE_STL_MOVE_ONLY_FUNCTION_DESC "std::move_only_function"
#define D_ENV_CPP_FEATURE_STL_MOVE_ONLY_FUNCTION_VERS "(C++23)"

// 2.4.14
// D_ENV_CPP_FEATURE_STL_PRINT
//   feature: 1 if __cpp_lib_print is defined, 0 otherwise.
#ifdef __cpp_lib_print
    #define D_ENV_CPP_FEATURE_STL_PRINT     1
    #define D_ENV_CPP_FEATURE_STL_PRINT_VAL __cpp_lib_print
#else
    #define D_ENV_CPP_FEATURE_STL_PRINT     0
    #define D_ENV_CPP_FEATURE_STL_PRINT_VAL 0L
#endif  // __cpp_lib_print
#define D_ENV_CPP_FEATURE_STL_PRINT_NAME "__cpp_lib_print"
#define D_ENV_CPP_FEATURE_STL_PRINT_DESC "Formatted output"
#define D_ENV_CPP_FEATURE_STL_PRINT_VERS "(C++23)"

// 2.4.15
// D_ENV_CPP_FEATURE_STL_RANGES_TO_CONTAINER
//   feature: 1 if __cpp_lib_ranges_to_container is defined, 0 otherwise.
#ifdef __cpp_lib_ranges_to_container
    #define D_ENV_CPP_FEATURE_STL_RANGES_TO_CONTAINER 1
    #define D_ENV_CPP_FEATURE_STL_RANGES_TO_CONTAINER_VAL                      \
        __cpp_lib_ranges_to_container
#else
    #define D_ENV_CPP_FEATURE_STL_RANGES_TO_CONTAINER 0
    #define D_ENV_CPP_FEATURE_STL_RANGES_TO_CONTAINER_VAL 0L
#endif  // __cpp_lib_ranges_to_container
#define D_ENV_CPP_FEATURE_STL_RANGES_TO_CONTAINER_NAME                         \
    "__cpp_lib_ranges_to_container"
#define D_ENV_CPP_FEATURE_STL_RANGES_TO_CONTAINER_DESC "std::ranges::to"
#define D_ENV_CPP_FEATURE_STL_RANGES_TO_CONTAINER_VERS "(C++23)"

// 2.4.16
// D_ENV_CPP_FEATURE_STL_SPANSTREAM
//   feature: 1 if __cpp_lib_spanstream is defined, 0 otherwise.
#ifdef __cpp_lib_spanstream
    #define D_ENV_CPP_FEATURE_STL_SPANSTREAM     1
    #define D_ENV_CPP_FEATURE_STL_SPANSTREAM_VAL __cpp_lib_spanstream
#else
    #define D_ENV_CPP_FEATURE_STL_SPANSTREAM     0
    #define D_ENV_CPP_FEATURE_STL_SPANSTREAM_VAL 0L
#endif  // __cpp_lib_spanstream
#define D_ENV_CPP_FEATURE_STL_SPANSTREAM_NAME "__cpp_lib_spanstream"
#define D_ENV_CPP_FEATURE_STL_SPANSTREAM_DESC "std::spanbuf, std::spanstream"
#define D_ENV_CPP_FEATURE_STL_SPANSTREAM_VERS "(C++23)"

// 2.4.17
// D_ENV_CPP_FEATURE_STL_STACKTRACE
//   feature: 1 if __cpp_lib_stacktrace is defined, 0 otherwise.
#ifdef __cpp_lib_stacktrace
    #define D_ENV_CPP_FEATURE_STL_STACKTRACE     1
    #define D_ENV_CPP_FEATURE_STL_STACKTRACE_VAL __cpp_lib_stacktrace
#else
    #define D_ENV_CPP_FEATURE_STL_STACKTRACE     0
    #define D_ENV_CPP_FEATURE_STL_STACKTRACE_VAL 0L
#endif  // __cpp_lib_stacktrace
#define D_ENV_CPP_FEATURE_STL_STACKTRACE_NAME "__cpp_lib_stacktrace"
#define D_ENV_CPP_FEATURE_STL_STACKTRACE_DESC "Stacktrace library"
#define D_ENV_CPP_FEATURE_STL_STACKTRACE_VERS "(C++23)"

// 2.4.18
// D_ENV_CPP_FEATURE_STL_STDATOMIC_H
//   feature: 1 if __cpp_lib_stdatomic_h is defined, 0 otherwise.
#ifdef __cpp_lib_stdatomic_h
    #define D_ENV_CPP_FEATURE_STL_STDATOMIC_H     1
    #define D_ENV_CPP_FEATURE_STL_STDATOMIC_H_VAL __cpp_lib_stdatomic_h
#else
    #define D_ENV_CPP_FEATURE_STL_STDATOMIC_H     0
    #define D_ENV_CPP_FEATURE_STL_STDATOMIC_H_VAL 0L
#endif  // __cpp_lib_stdatomic_h
#define D_ENV_CPP_FEATURE_STL_STDATOMIC_H_NAME "__cpp_lib_stdatomic_h"
#define D_ENV_CPP_FEATURE_STL_STDATOMIC_H_DESC                                 \
    "Compatibility header for C atomic operations"
#define D_ENV_CPP_FEATURE_STL_STDATOMIC_H_VERS "(C++23)"

// 2.4.19
// D_ENV_CPP_FEATURE_STL_STRING_CONTAINS
//   feature: 1 if __cpp_lib_string_contains is defined, 0 otherwise.
#ifdef __cpp_lib_string_contains
    #define D_ENV_CPP_FEATURE_STL_STRING_CONTAINS     1
    #define D_ENV_CPP_FEATURE_STL_STRING_CONTAINS_VAL __cpp_lib_string_contains
#else
    #define D_ENV_CPP_FEATURE_STL_STRING_CONTAINS     0
    #define D_ENV_CPP_FEATURE_STL_STRING_CONTAINS_VAL 0L
#endif  // __cpp_lib_string_contains
#define D_ENV_CPP_FEATURE_STL_STRING_CONTAINS_NAME "__cpp_lib_string_contains"
#define D_ENV_CPP_FEATURE_STL_STRING_CONTAINS_DESC                             \
    "contains() for std::basic_string and std::basic_string_view"
#define D_ENV_CPP_FEATURE_STL_STRING_CONTAINS_VERS "(C++23)"

// 2.4.20
// D_ENV_CPP_FEATURE_STL_STRING_RESIZE_AND_OVERWRITE
//   feature: 1 if __cpp_lib_string_resize_and_overwrite is defined, 0
// otherwise.
#ifdef __cpp_lib_string_resize_and_overwrite
    #define D_ENV_CPP_FEATURE_STL_STRING_RESIZE_AND_OVERWRITE 1
    #define D_ENV_CPP_FEATURE_STL_STRING_RESIZE_AND_OVERWRITE_VAL              \
        __cpp_lib_string_resize_and_overwrite
#else
    #define D_ENV_CPP_FEATURE_STL_STRING_RESIZE_AND_OVERWRITE 0
    #define D_ENV_CPP_FEATURE_STL_STRING_RESIZE_AND_OVERWRITE_VAL 0L
#endif  // __cpp_lib_string_resize_and_overwrite
#define D_ENV_CPP_FEATURE_STL_STRING_RESIZE_AND_OVERWRITE_NAME                 \
    "__cpp_lib_string_resize_and_overwrite"
#define D_ENV_CPP_FEATURE_STL_STRING_RESIZE_AND_OVERWRITE_DESC                 \
    "std::basic_string::resize_and_overwrite"
#define D_ENV_CPP_FEATURE_STL_STRING_RESIZE_AND_OVERWRITE_VERS "(C++23)"

// 2.4.21
// D_ENV_CPP_FEATURE_STL_UNREACHABLE
//   feature: 1 if __cpp_lib_unreachable is defined, 0 otherwise.
#ifdef __cpp_lib_unreachable
    #define D_ENV_CPP_FEATURE_STL_UNREACHABLE     1
    #define D_ENV_CPP_FEATURE_STL_UNREACHABLE_VAL __cpp_lib_unreachable
#else
    #define D_ENV_CPP_FEATURE_STL_UNREACHABLE     0
    #define D_ENV_CPP_FEATURE_STL_UNREACHABLE_VAL 0L
#endif  // __cpp_lib_unreachable
#define D_ENV_CPP_FEATURE_STL_UNREACHABLE_NAME "__cpp_lib_unreachable"
#define D_ENV_CPP_FEATURE_STL_UNREACHABLE_DESC "std::unreachable"
#define D_ENV_CPP_FEATURE_STL_UNREACHABLE_VERS "(C++23)"

// 2.5    C++26 library features
//------------------------------------------------------------------------------
// 2.5.1
// D_ENV_CPP_FEATURE_STL_ALGORITHM_DEFAULT_VALUE_TYPE
//   feature: 1 if __cpp_lib_algorithm_default_value_type is defined, 0
// otherwise.
#ifdef __cpp_lib_algorithm_default_value_type
    #define D_ENV_CPP_FEATURE_STL_ALGORITHM_DEFAULT_VALUE_TYPE 1
    #define D_ENV_CPP_FEATURE_STL_ALGORITHM_DEFAULT_VALUE_TYPE_VAL             \
        __cpp_lib_algorithm_default_value_type
#else
    #define D_ENV_CPP_FEATURE_STL_ALGORITHM_DEFAULT_VALUE_TYPE 0
    #define D_ENV_CPP_FEATURE_STL_ALGORITHM_DEFAULT_VALUE_TYPE_VAL 0L
#endif  // __cpp_lib_algorithm_default_value_type
#define D_ENV_CPP_FEATURE_STL_ALGORITHM_DEFAULT_VALUE_TYPE_NAME                \
    "__cpp_lib_algorithm_default_value_type"
#define D_ENV_CPP_FEATURE_STL_ALGORITHM_DEFAULT_VALUE_TYPE_DESC                \
    "Enabling list-initialization for algorithms"
#define D_ENV_CPP_FEATURE_STL_ALGORITHM_DEFAULT_VALUE_TYPE_VERS "(C++26)"

// 2.5.2
// D_ENV_CPP_FEATURE_STL_ASSOCIATIVE_HETEROGENEOUS_INSERTION
//   feature: 1 if __cpp_lib_associative_heterogeneous_insertion is defined, 0
// otherwise.
#ifdef __cpp_lib_associative_heterogeneous_insertion
    #define D_ENV_CPP_FEATURE_STL_ASSOCIATIVE_HETEROGENEOUS_INSERTION 1
    #define D_ENV_CPP_FEATURE_STL_ASSOCIATIVE_HETEROGENEOUS_INSERTION_VAL      \
        __cpp_lib_associative_heterogeneous_insertion
#else
    #define D_ENV_CPP_FEATURE_STL_ASSOCIATIVE_HETEROGENEOUS_INSERTION 0
    #define D_ENV_CPP_FEATURE_STL_ASSOCIATIVE_HETEROGENEOUS_INSERTION_VAL 0L
#endif  // __cpp_lib_associative_heterogeneous_insertion
#define D_ENV_CPP_FEATURE_STL_ASSOCIATIVE_HETEROGENEOUS_INSERTION_NAME         \
    "__cpp_lib_associative_heterogeneous_insertion"
#define D_ENV_CPP_FEATURE_STL_ASSOCIATIVE_HETEROGENEOUS_INSERTION_DESC         \
    "Heterogeneous overloads for associative containers"
#define D_ENV_CPP_FEATURE_STL_ASSOCIATIVE_HETEROGENEOUS_INSERTION_VERS "(C++26)"

// 2.5.3
// D_ENV_CPP_FEATURE_STL_ATOMIC_MIN_MAX
//   feature: 1 if __cpp_lib_atomic_min_max is defined, 0 otherwise.
#ifdef __cpp_lib_atomic_min_max
    #define D_ENV_CPP_FEATURE_STL_ATOMIC_MIN_MAX     1
    #define D_ENV_CPP_FEATURE_STL_ATOMIC_MIN_MAX_VAL __cpp_lib_atomic_min_max
#else
    #define D_ENV_CPP_FEATURE_STL_ATOMIC_MIN_MAX     0
    #define D_ENV_CPP_FEATURE_STL_ATOMIC_MIN_MAX_VAL 0L
#endif  // __cpp_lib_atomic_min_max
#define D_ENV_CPP_FEATURE_STL_ATOMIC_MIN_MAX_NAME "__cpp_lib_atomic_min_max"
#define D_ENV_CPP_FEATURE_STL_ATOMIC_MIN_MAX_DESC "Atomic minimum/maximum"
#define D_ENV_CPP_FEATURE_STL_ATOMIC_MIN_MAX_VERS "(C++26)"

// 2.5.4
// D_ENV_CPP_FEATURE_STL_CONSTEXPR_ATOMIC
//   feature: 1 if __cpp_lib_constexpr_atomic is defined, 0 otherwise.
#ifdef __cpp_lib_constexpr_atomic
    #define D_ENV_CPP_FEATURE_STL_CONSTEXPR_ATOMIC 1
    #define D_ENV_CPP_FEATURE_STL_CONSTEXPR_ATOMIC_VAL                         \
        __cpp_lib_constexpr_atomic
#else
    #define D_ENV_CPP_FEATURE_STL_CONSTEXPR_ATOMIC 0
    #define D_ENV_CPP_FEATURE_STL_CONSTEXPR_ATOMIC_VAL 0L
#endif  // __cpp_lib_constexpr_atomic
#define D_ENV_CPP_FEATURE_STL_CONSTEXPR_ATOMIC_NAME "__cpp_lib_constexpr_atomic"
#define D_ENV_CPP_FEATURE_STL_CONSTEXPR_ATOMIC_DESC                            \
    "constexpr std::atomic and std::atomic_ref"
#define D_ENV_CPP_FEATURE_STL_CONSTEXPR_ATOMIC_VERS "(C++26)"

// 2.5.5
// D_ENV_CPP_FEATURE_STL_CONSTEXPR_DEQUE
//   feature: 1 if __cpp_lib_constexpr_deque is defined, 0 otherwise.
#ifdef __cpp_lib_constexpr_deque
    #define D_ENV_CPP_FEATURE_STL_CONSTEXPR_DEQUE     1
    #define D_ENV_CPP_FEATURE_STL_CONSTEXPR_DEQUE_VAL __cpp_lib_constexpr_deque
#else
    #define D_ENV_CPP_FEATURE_STL_CONSTEXPR_DEQUE     0
    #define D_ENV_CPP_FEATURE_STL_CONSTEXPR_DEQUE_VAL 0L
#endif  // __cpp_lib_constexpr_deque
#define D_ENV_CPP_FEATURE_STL_CONSTEXPR_DEQUE_NAME "__cpp_lib_constexpr_deque"
#define D_ENV_CPP_FEATURE_STL_CONSTEXPR_DEQUE_DESC "constexpr std::deque"
#define D_ENV_CPP_FEATURE_STL_CONSTEXPR_DEQUE_VERS "(C++26)"

// 2.5.6
// D_ENV_CPP_FEATURE_STL_CONTRACTS
//   feature: 1 if __cpp_lib_contracts is defined, 0 otherwise.
#ifdef __cpp_lib_contracts
    #define D_ENV_CPP_FEATURE_STL_CONTRACTS     1
    #define D_ENV_CPP_FEATURE_STL_CONTRACTS_VAL __cpp_lib_contracts
#else
    #define D_ENV_CPP_FEATURE_STL_CONTRACTS     0
    #define D_ENV_CPP_FEATURE_STL_CONTRACTS_VAL 0L
#endif  // __cpp_lib_contracts
#define D_ENV_CPP_FEATURE_STL_CONTRACTS_NAME "__cpp_lib_contracts"
#define D_ENV_CPP_FEATURE_STL_CONTRACTS_DESC "<contracts>: Contracts support"
#define D_ENV_CPP_FEATURE_STL_CONTRACTS_VERS "(C++26)"

// 2.5.7
// D_ENV_CPP_FEATURE_STL_COPYABLE_FUNCTION
//   feature: 1 if __cpp_lib_copyable_function is defined, 0 otherwise.
#ifdef __cpp_lib_copyable_function
    #define D_ENV_CPP_FEATURE_STL_COPYABLE_FUNCTION 1
    #define D_ENV_CPP_FEATURE_STL_COPYABLE_FUNCTION_VAL                        \
        __cpp_lib_copyable_function
#else
    #define D_ENV_CPP_FEATURE_STL_COPYABLE_FUNCTION 0
    #define D_ENV_CPP_FEATURE_STL_COPYABLE_FUNCTION_VAL 0L
#endif  // __cpp_lib_copyable_function
#define D_ENV_CPP_FEATURE_STL_COPYABLE_FUNCTION_NAME                           \
    "__cpp_lib_copyable_function"
#define D_ENV_CPP_FEATURE_STL_COPYABLE_FUNCTION_DESC "std::copyable_function"
#define D_ENV_CPP_FEATURE_STL_COPYABLE_FUNCTION_VERS "(C++26)"

// 2.5.8
// D_ENV_CPP_FEATURE_STL_DEBUGGING
//   feature: 1 if __cpp_lib_debugging is defined, 0 otherwise.
#ifdef __cpp_lib_debugging
    #define D_ENV_CPP_FEATURE_STL_DEBUGGING     1
    #define D_ENV_CPP_FEATURE_STL_DEBUGGING_VAL __cpp_lib_debugging
#else
    #define D_ENV_CPP_FEATURE_STL_DEBUGGING     0
    #define D_ENV_CPP_FEATURE_STL_DEBUGGING_VAL 0L
#endif  // __cpp_lib_debugging
#define D_ENV_CPP_FEATURE_STL_DEBUGGING_NAME "__cpp_lib_debugging"
#define D_ENV_CPP_FEATURE_STL_DEBUGGING_DESC "<debugging>: Debugging support"
#define D_ENV_CPP_FEATURE_STL_DEBUGGING_VERS "(C++26)"

// 2.5.9
// D_ENV_CPP_FEATURE_STL_FORMAT_PATH
//   feature: 1 if __cpp_lib_format_path is defined, 0 otherwise.
#ifdef __cpp_lib_format_path
    #define D_ENV_CPP_FEATURE_STL_FORMAT_PATH     1
    #define D_ENV_CPP_FEATURE_STL_FORMAT_PATH_VAL __cpp_lib_format_path
#else
    #define D_ENV_CPP_FEATURE_STL_FORMAT_PATH     0
    #define D_ENV_CPP_FEATURE_STL_FORMAT_PATH_VAL 0L
#endif  // __cpp_lib_format_path
#define D_ENV_CPP_FEATURE_STL_FORMAT_PATH_NAME "__cpp_lib_format_path"
#define D_ENV_CPP_FEATURE_STL_FORMAT_PATH_DESC                                 \
    "Formatting of std::filesystem::path"
#define D_ENV_CPP_FEATURE_STL_FORMAT_PATH_VERS "(C++26)"

// 2.5.10
// D_ENV_CPP_FEATURE_STL_FUNCTION_REF
//   feature: 1 if __cpp_lib_function_ref is defined, 0 otherwise.
#ifdef __cpp_lib_function_ref
    #define D_ENV_CPP_FEATURE_STL_FUNCTION_REF     1
    #define D_ENV_CPP_FEATURE_STL_FUNCTION_REF_VAL __cpp_lib_function_ref
#else
    #define D_ENV_CPP_FEATURE_STL_FUNCTION_REF     0
    #define D_ENV_CPP_FEATURE_STL_FUNCTION_REF_VAL 0L
#endif  // __cpp_lib_function_ref
#define D_ENV_CPP_FEATURE_STL_FUNCTION_REF_NAME "__cpp_lib_function_ref"
#define D_ENV_CPP_FEATURE_STL_FUNCTION_REF_DESC                                \
    "std::function_ref: A type-erased callable reference"
#define D_ENV_CPP_FEATURE_STL_FUNCTION_REF_VERS "(C++26)"

// 2.5.11
// D_ENV_CPP_FEATURE_STL_HAZARD_POINTER
//   feature: 1 if __cpp_lib_hazard_pointer is defined, 0 otherwise.
#ifdef __cpp_lib_hazard_pointer
    #define D_ENV_CPP_FEATURE_STL_HAZARD_POINTER     1
    #define D_ENV_CPP_FEATURE_STL_HAZARD_POINTER_VAL __cpp_lib_hazard_pointer
#else
    #define D_ENV_CPP_FEATURE_STL_HAZARD_POINTER     0
    #define D_ENV_CPP_FEATURE_STL_HAZARD_POINTER_VAL 0L
#endif  // __cpp_lib_hazard_pointer
#define D_ENV_CPP_FEATURE_STL_HAZARD_POINTER_NAME "__cpp_lib_hazard_pointer"
#define D_ENV_CPP_FEATURE_STL_HAZARD_POINTER_DESC                              \
    "<hazard_pointer>: Hazard pointers"
#define D_ENV_CPP_FEATURE_STL_HAZARD_POINTER_VERS "(C++26)"

// 2.5.12
// D_ENV_CPP_FEATURE_STL_HIVE
//   feature: 1 if __cpp_lib_hive is defined, 0 otherwise.
#ifdef __cpp_lib_hive
    #define D_ENV_CPP_FEATURE_STL_HIVE     1
    #define D_ENV_CPP_FEATURE_STL_HIVE_VAL __cpp_lib_hive
#else
    #define D_ENV_CPP_FEATURE_STL_HIVE     0
    #define D_ENV_CPP_FEATURE_STL_HIVE_VAL 0L
#endif  // __cpp_lib_hive
#define D_ENV_CPP_FEATURE_STL_HIVE_NAME "__cpp_lib_hive"
#define D_ENV_CPP_FEATURE_STL_HIVE_DESC "<hive>: a bucket-based container"
#define D_ENV_CPP_FEATURE_STL_HIVE_VERS "(C++26)"

// 2.5.13
// D_ENV_CPP_FEATURE_STL_INPLACE_VECTOR
//   feature: 1 if __cpp_lib_inplace_vector is defined, 0 otherwise.
#ifdef __cpp_lib_inplace_vector
    #define D_ENV_CPP_FEATURE_STL_INPLACE_VECTOR     1
    #define D_ENV_CPP_FEATURE_STL_INPLACE_VECTOR_VAL __cpp_lib_inplace_vector
#else
    #define D_ENV_CPP_FEATURE_STL_INPLACE_VECTOR     0
    #define D_ENV_CPP_FEATURE_STL_INPLACE_VECTOR_VAL 0L
#endif  // __cpp_lib_inplace_vector
#define D_ENV_CPP_FEATURE_STL_INPLACE_VECTOR_NAME "__cpp_lib_inplace_vector"
#define D_ENV_CPP_FEATURE_STL_INPLACE_VECTOR_DESC "std::inplace_vector"
#define D_ENV_CPP_FEATURE_STL_INPLACE_VECTOR_VERS "(C++26)"

// 2.5.14
// D_ENV_CPP_FEATURE_STL_LINALG
//   feature: 1 if __cpp_lib_linalg is defined, 0 otherwise.
#ifdef __cpp_lib_linalg
    #define D_ENV_CPP_FEATURE_STL_LINALG     1
    #define D_ENV_CPP_FEATURE_STL_LINALG_VAL __cpp_lib_linalg
#else
    #define D_ENV_CPP_FEATURE_STL_LINALG     0
    #define D_ENV_CPP_FEATURE_STL_LINALG_VAL 0L
#endif  // __cpp_lib_linalg
#define D_ENV_CPP_FEATURE_STL_LINALG_NAME "__cpp_lib_linalg"
#define D_ENV_CPP_FEATURE_STL_LINALG_DESC                                      \
    "A free function linear algebra interface based on the BLAS"
#define D_ENV_CPP_FEATURE_STL_LINALG_VERS "(C++26)"

// 2.5.15
// D_ENV_CPP_FEATURE_STL_POLYMORPHIC
//   feature: 1 if __cpp_lib_polymorphic is defined, 0 otherwise.
#ifdef __cpp_lib_polymorphic
    #define D_ENV_CPP_FEATURE_STL_POLYMORPHIC     1
    #define D_ENV_CPP_FEATURE_STL_POLYMORPHIC_VAL __cpp_lib_polymorphic
#else
    #define D_ENV_CPP_FEATURE_STL_POLYMORPHIC     0
    #define D_ENV_CPP_FEATURE_STL_POLYMORPHIC_VAL 0L
#endif  // __cpp_lib_polymorphic
#define D_ENV_CPP_FEATURE_STL_POLYMORPHIC_NAME "__cpp_lib_polymorphic"
#define D_ENV_CPP_FEATURE_STL_POLYMORPHIC_DESC "std::polymorphic"
#define D_ENV_CPP_FEATURE_STL_POLYMORPHIC_VERS "(C++26)"

// 2.5.16
// D_ENV_CPP_FEATURE_STL_RCU
//   feature: 1 if __cpp_lib_rcu is defined, 0 otherwise.
#ifdef __cpp_lib_rcu
    #define D_ENV_CPP_FEATURE_STL_RCU     1
    #define D_ENV_CPP_FEATURE_STL_RCU_VAL __cpp_lib_rcu
#else
    #define D_ENV_CPP_FEATURE_STL_RCU     0
    #define D_ENV_CPP_FEATURE_STL_RCU_VAL 0L
#endif  // __cpp_lib_rcu
#define D_ENV_CPP_FEATURE_STL_RCU_NAME "__cpp_lib_rcu"
#define D_ENV_CPP_FEATURE_STL_RCU_DESC "<rcu>: Read-Copy Update (RCU)"
#define D_ENV_CPP_FEATURE_STL_RCU_VERS "(C++26)"

// 2.5.17
// D_ENV_CPP_FEATURE_STL_SATURATION_ARITHMETIC
//   feature: 1 if __cpp_lib_saturation_arithmetic is defined, 0 otherwise.
#ifdef __cpp_lib_saturation_arithmetic
    #define D_ENV_CPP_FEATURE_STL_SATURATION_ARITHMETIC 1
    #define D_ENV_CPP_FEATURE_STL_SATURATION_ARITHMETIC_VAL                    \
        __cpp_lib_saturation_arithmetic
#else
    #define D_ENV_CPP_FEATURE_STL_SATURATION_ARITHMETIC 0
    #define D_ENV_CPP_FEATURE_STL_SATURATION_ARITHMETIC_VAL 0L
#endif  // __cpp_lib_saturation_arithmetic
#define D_ENV_CPP_FEATURE_STL_SATURATION_ARITHMETIC_NAME                       \
    "__cpp_lib_saturation_arithmetic"
#define D_ENV_CPP_FEATURE_STL_SATURATION_ARITHMETIC_DESC "Saturation arithmetic"
#define D_ENV_CPP_FEATURE_STL_SATURATION_ARITHMETIC_VERS "(C++26)"

// 2.5.18
// D_ENV_CPP_FEATURE_STL_SENDERS
//   feature: 1 if __cpp_lib_senders is defined, 0 otherwise.
#ifdef __cpp_lib_senders
    #define D_ENV_CPP_FEATURE_STL_SENDERS     1
    #define D_ENV_CPP_FEATURE_STL_SENDERS_VAL __cpp_lib_senders
#else
    #define D_ENV_CPP_FEATURE_STL_SENDERS     0
    #define D_ENV_CPP_FEATURE_STL_SENDERS_VAL 0L
#endif  // __cpp_lib_senders
#define D_ENV_CPP_FEATURE_STL_SENDERS_NAME "__cpp_lib_senders"
#define D_ENV_CPP_FEATURE_STL_SENDERS_DESC                                     \
    "std::execution: Sender-receiver model"
#define D_ENV_CPP_FEATURE_STL_SENDERS_VERS "(C++26)"

// 2.5.19
// D_ENV_CPP_FEATURE_STL_SIMD
//   feature: 1 if __cpp_lib_simd is defined, 0 otherwise.
#ifdef __cpp_lib_simd
    #define D_ENV_CPP_FEATURE_STL_SIMD     1
    #define D_ENV_CPP_FEATURE_STL_SIMD_VAL __cpp_lib_simd
#else
    #define D_ENV_CPP_FEATURE_STL_SIMD     0
    #define D_ENV_CPP_FEATURE_STL_SIMD_VAL 0L
#endif  // __cpp_lib_simd
#define D_ENV_CPP_FEATURE_STL_SIMD_NAME "__cpp_lib_simd"
#define D_ENV_CPP_FEATURE_STL_SIMD_DESC "<simd>: Data-parallel types"
#define D_ENV_CPP_FEATURE_STL_SIMD_VERS "(C++26)"

// 2.5.20
// D_ENV_CPP_FEATURE_STL_TEXT_ENCODING
//   feature: 1 if __cpp_lib_text_encoding is defined, 0 otherwise.
#ifdef __cpp_lib_text_encoding
    #define D_ENV_CPP_FEATURE_STL_TEXT_ENCODING     1
    #define D_ENV_CPP_FEATURE_STL_TEXT_ENCODING_VAL __cpp_lib_text_encoding
#else
    #define D_ENV_CPP_FEATURE_STL_TEXT_ENCODING     0
    #define D_ENV_CPP_FEATURE_STL_TEXT_ENCODING_VAL 0L
#endif  // __cpp_lib_text_encoding
#define D_ENV_CPP_FEATURE_STL_TEXT_ENCODING_NAME "__cpp_lib_text_encoding"
#define D_ENV_CPP_FEATURE_STL_TEXT_ENCODING_DESC "std::text_encoding"
#define D_ENV_CPP_FEATURE_STL_TEXT_ENCODING_VERS "(C++26)"

//==============================================================================
// 3.  AGGREGATE FEATURE CHECKS
//==============================================================================
// 1 when every feature of one standard in sections 1 and 2 is available:
// its language features, its library features, or both.


// 3.1    C++11
//------------------------------------------------------------------------------
// 3.1.1
// D_ENV_CPP_FEATURE_HAS_ALL_LANG_CPP11
//   feature: 1 if all C++11 language features are available.
#define D_ENV_CPP_FEATURE_HAS_ALL_LANG_CPP11                                   \
    ( (D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES)         &&                      \
      (D_ENV_CPP_FEATURE_LANG_ATTRIBUTES)              &&                      \
      (D_ENV_CPP_FEATURE_LANG_CONSTEXPR)               &&                      \
      (D_ENV_CPP_FEATURE_LANG_DECLTYPE)                &&                      \
      (D_ENV_CPP_FEATURE_LANG_DELEGATING_CONSTRUCTORS) &&                      \
      (D_ENV_CPP_FEATURE_LANG_INHERITING_CONSTRUCTORS) &&                      \
      (D_ENV_CPP_FEATURE_LANG_INITIALIZER_LISTS)       &&                      \
      (D_ENV_CPP_FEATURE_LANG_LAMBDAS)                 &&                      \
      (D_ENV_CPP_FEATURE_LANG_NSDMI)                   &&                      \
      (D_ENV_CPP_FEATURE_LANG_RANGE_BASED_FOR)         &&                      \
      (D_ENV_CPP_FEATURE_LANG_RAW_STRINGS)             &&                      \
      (D_ENV_CPP_FEATURE_LANG_REF_QUALIFIERS)          &&                      \
      (D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES)       &&                      \
      (D_ENV_CPP_FEATURE_LANG_STATIC_ASSERT)           &&                      \
      (D_ENV_CPP_FEATURE_LANG_THREADSAFE_STATIC_INIT)  &&                      \
      (D_ENV_CPP_FEATURE_LANG_UNICODE_CHARACTERS)      &&                      \
      (D_ENV_CPP_FEATURE_LANG_UNICODE_LITERALS)        &&                      \
      (D_ENV_CPP_FEATURE_LANG_USER_DEFINED_LITERALS)   &&                      \
      (D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES) )

// 3.1.2
// D_ENV_CPP_FEATURE_HAS_ALL_CPP11
//   feature: 1 if all C++11 features are available.
#define D_ENV_CPP_FEATURE_HAS_ALL_CPP11                                        \
    (D_ENV_CPP_FEATURE_HAS_ALL_LANG_CPP11)

// 3.2    C++14
//------------------------------------------------------------------------------
// 3.2.1
// D_ENV_CPP_FEATURE_HAS_ALL_LANG_CPP14
//   feature: 1 if all C++14 language features are available.
#define D_ENV_CPP_FEATURE_HAS_ALL_LANG_CPP14                                   \
    ( (D_ENV_CPP_FEATURE_LANG_AGGREGATE_NSDMI)       &&                        \
      (D_ENV_CPP_FEATURE_LANG_BINARY_LITERALS)       &&                        \
      (D_ENV_CPP_FEATURE_LANG_DECLTYPE_AUTO)         &&                        \
      (D_ENV_CPP_FEATURE_LANG_ENUMERATOR_ATTRIBUTES) &&                        \
      (D_ENV_CPP_FEATURE_LANG_GENERIC_LAMBDAS)       &&                        \
      (D_ENV_CPP_FEATURE_LANG_INIT_CAPTURES)         &&                        \
      (D_ENV_CPP_FEATURE_LANG_NAMESPACE_ATTRIBUTES)  &&                        \
      (D_ENV_CPP_FEATURE_LANG_NONTYPE_TEMPLATE_ARGS) &&                        \
      (D_ENV_CPP_FEATURE_LANG_RETURN_TYPE_DEDUCTION) &&                        \
      (D_ENV_CPP_FEATURE_LANG_SIZED_DEALLOCATION)    &&                        \
      (D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES) )

// 3.2.2
// D_ENV_CPP_FEATURE_HAS_ALL_STL_CPP14
//   feature: 1 if all C++14 library features are available.
#define D_ENV_CPP_FEATURE_HAS_ALL_STL_CPP14                                    \
    ( (D_ENV_CPP_FEATURE_STL_CHRONO_UDLS) &&                                   \
      (D_ENV_CPP_FEATURE_STL_COMPLEX_UDLS) )

// 3.2.3
// D_ENV_CPP_FEATURE_HAS_ALL_CPP14
//   feature: 1 if all C++14 features are available.
#define D_ENV_CPP_FEATURE_HAS_ALL_CPP14                                        \
    ( (D_ENV_CPP_FEATURE_HAS_ALL_LANG_CPP14) &&                                \
      (D_ENV_CPP_FEATURE_HAS_ALL_STL_CPP14) )

// 3.3    C++17
//------------------------------------------------------------------------------
// 3.3.1
// D_ENV_CPP_FEATURE_HAS_ALL_LANG_CPP17
//   feature: 1 if all C++17 language features are available.
#define D_ENV_CPP_FEATURE_HAS_ALL_LANG_CPP17                                   \
    ( (D_ENV_CPP_FEATURE_LANG_AGGREGATE_BASES)                 &&              \
      (D_ENV_CPP_FEATURE_LANG_ALIGNED_NEW)                     &&              \
      (D_ENV_CPP_FEATURE_LANG_CAPTURE_STAR_THIS)               &&              \
      (D_ENV_CPP_FEATURE_LANG_CONSTEXPR_IN_DECLTYPE)           &&              \
      (D_ENV_CPP_FEATURE_LANG_DEDUCTION_GUIDES)                &&              \
      (D_ENV_CPP_FEATURE_LANG_FOLD_EXPRESSIONS)                &&              \
      (D_ENV_CPP_FEATURE_LANG_GUARANTEED_COPY_ELISION)         &&              \
      (D_ENV_CPP_FEATURE_LANG_HEX_FLOAT)                       &&              \
      (D_ENV_CPP_FEATURE_LANG_IF_CONSTEXPR)                    &&              \
      (D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES)                &&              \
      (D_ENV_CPP_FEATURE_LANG_NOEXCEPT_FUNCTION_TYPE)          &&              \
      (D_ENV_CPP_FEATURE_LANG_NONTYPE_TEMPLATE_PARAMETER_AUTO) &&              \
      (D_ENV_CPP_FEATURE_LANG_STRUCTURED_BINDINGS)             &&              \
      (D_ENV_CPP_FEATURE_LANG_TEMPLATE_TEMPLATE_ARGS)          &&              \
      (D_ENV_CPP_FEATURE_LANG_VARIADIC_USING) )

// 3.3.2
// D_ENV_CPP_FEATURE_HAS_ALL_STL_CPP17
//   feature: 1 if all C++17 library features are available.
#define D_ENV_CPP_FEATURE_HAS_ALL_STL_CPP17                                    \
    ( (D_ENV_CPP_FEATURE_STL_ADDRESSOF_CONSTEXPR)  &&                          \
      (D_ENV_CPP_FEATURE_STL_ANY)                  &&                          \
      (D_ENV_CPP_FEATURE_STL_APPLY)                &&                          \
      (D_ENV_CPP_FEATURE_STL_ARRAY_CONSTEXPR)      &&                          \
      (D_ENV_CPP_FEATURE_STL_AS_CONST)             &&                          \
      (D_ENV_CPP_FEATURE_STL_BOOL_CONSTANT)        &&                          \
      (D_ENV_CPP_FEATURE_STL_BOYER_MOORE_SEARCHER) &&                          \
      (D_ENV_CPP_FEATURE_STL_BYTE)                 &&                          \
      (D_ENV_CPP_FEATURE_STL_CLAMP)                &&                          \
      (D_ENV_CPP_FEATURE_STL_FILESYSTEM)           &&                          \
      (D_ENV_CPP_FEATURE_STL_OPTIONAL)             &&                          \
      (D_ENV_CPP_FEATURE_STL_VARIANT) )

// 3.3.3
// D_ENV_CPP_FEATURE_HAS_ALL_CPP17
//   feature: 1 if all C++17 features are available.
#define D_ENV_CPP_FEATURE_HAS_ALL_CPP17                                        \
    ( (D_ENV_CPP_FEATURE_HAS_ALL_LANG_CPP17) &&                                \
      (D_ENV_CPP_FEATURE_HAS_ALL_STL_CPP17) )

// 3.4    C++20
//------------------------------------------------------------------------------
// 3.4.1
// D_ENV_CPP_FEATURE_HAS_ALL_LANG_CPP20
//   feature: 1 if all C++20 language features are available.
#define D_ENV_CPP_FEATURE_HAS_ALL_LANG_CPP20                                   \
    ( (D_ENV_CPP_FEATURE_LANG_AGGREGATE_PAREN_INIT)      &&                    \
      (D_ENV_CPP_FEATURE_LANG_CHAR8_T)                   &&                    \
      (D_ENV_CPP_FEATURE_LANG_CONCEPTS)                  &&                    \
      (D_ENV_CPP_FEATURE_LANG_CONDITIONAL_EXPLICIT)      &&                    \
      (D_ENV_CPP_FEATURE_LANG_CONSTEVAL)                 &&                    \
      (D_ENV_CPP_FEATURE_LANG_CONSTEXPR_DYNAMIC_ALLOC)   &&                    \
      (D_ENV_CPP_FEATURE_LANG_CONSTINIT)                 &&                    \
      (D_ENV_CPP_FEATURE_LANG_DESIGNATED_INITIALIZERS)   &&                    \
      (D_ENV_CPP_FEATURE_LANG_IMPL_COROUTINE)            &&                    \
      (D_ENV_CPP_FEATURE_LANG_IMPL_DESTROYING_DELETE)    &&                    \
      (D_ENV_CPP_FEATURE_LANG_IMPL_THREE_WAY_COMPARISON) &&                    \
      (D_ENV_CPP_FEATURE_LANG_MODULES)                   &&                    \
      (D_ENV_CPP_FEATURE_LANG_USING_ENUM) )

// 3.4.2
// D_ENV_CPP_FEATURE_HAS_ALL_STL_CPP20
//   feature: 1 if all C++20 library features are available.
#define D_ENV_CPP_FEATURE_HAS_ALL_STL_CPP20                                    \
    ( (D_ENV_CPP_FEATURE_STL_ASSUME_ALIGNED)          &&                       \
      (D_ENV_CPP_FEATURE_STL_ATOMIC_FLAG_TEST)        &&                       \
      (D_ENV_CPP_FEATURE_STL_ATOMIC_FLOAT)            &&                       \
      (D_ENV_CPP_FEATURE_STL_ATOMIC_REF)              &&                       \
      (D_ENV_CPP_FEATURE_STL_ATOMIC_WAIT)             &&                       \
      (D_ENV_CPP_FEATURE_STL_BARRIER)                 &&                       \
      (D_ENV_CPP_FEATURE_STL_BIND_FRONT)              &&                       \
      (D_ENV_CPP_FEATURE_STL_BIT_CAST)                &&                       \
      (D_ENV_CPP_FEATURE_STL_BITOPS)                  &&                       \
      (D_ENV_CPP_FEATURE_STL_BOUNDED_ARRAY_TRAITS)    &&                       \
      (D_ENV_CPP_FEATURE_STL_CHAR8_T)                 &&                       \
      (D_ENV_CPP_FEATURE_STL_CONCEPTS)                &&                       \
      (D_ENV_CPP_FEATURE_STL_CONSTEXPR_ALGORITHMS)    &&                       \
      (D_ENV_CPP_FEATURE_STL_CONSTEXPR_COMPLEX)       &&                       \
      (D_ENV_CPP_FEATURE_STL_CONSTEXPR_DYNAMIC_ALLOC) &&                       \
      (D_ENV_CPP_FEATURE_STL_CONSTEXPR_STRING)        &&                       \
      (D_ENV_CPP_FEATURE_STL_CONSTEXPR_VECTOR)        &&                       \
      (D_ENV_CPP_FEATURE_STL_COROUTINE)               &&                       \
      (D_ENV_CPP_FEATURE_STL_ENDIAN)                  &&                       \
      (D_ENV_CPP_FEATURE_STL_FORMAT)                  &&                       \
      (D_ENV_CPP_FEATURE_STL_JTHREAD)                 &&                       \
      (D_ENV_CPP_FEATURE_STL_LATCH)                   &&                       \
      (D_ENV_CPP_FEATURE_STL_MATH_CONSTANTS)          &&                       \
      (D_ENV_CPP_FEATURE_STL_RANGES)                  &&                       \
      (D_ENV_CPP_FEATURE_STL_SEMAPHORE)               &&                       \
      (D_ENV_CPP_FEATURE_STL_SOURCE_LOCATION)         &&                       \
      (D_ENV_CPP_FEATURE_STL_SPAN)                    &&                       \
      (D_ENV_CPP_FEATURE_STL_THREE_WAY_COMPARISON)    &&                       \
      (D_ENV_CPP_FEATURE_STL_TO_ARRAY) )

// 3.4.3
// D_ENV_CPP_FEATURE_HAS_ALL_CPP20
//   feature: 1 if all C++20 features are available.
#define D_ENV_CPP_FEATURE_HAS_ALL_CPP20                                        \
    ( (D_ENV_CPP_FEATURE_HAS_ALL_LANG_CPP20) &&                                \
      (D_ENV_CPP_FEATURE_HAS_ALL_STL_CPP20) )

// 3.5    C++23
//------------------------------------------------------------------------------
// 3.5.1
// D_ENV_CPP_FEATURE_HAS_ALL_LANG_CPP23
//   feature: 1 if all C++23 language features are available.
#define D_ENV_CPP_FEATURE_HAS_ALL_LANG_CPP23                                   \
    ( (D_ENV_CPP_FEATURE_LANG_AUTO_CAST)                  &&                   \
      (D_ENV_CPP_FEATURE_LANG_EXPLICIT_THIS_PARAMETER)    &&                   \
      (D_ENV_CPP_FEATURE_LANG_IF_CONSTEVAL)               &&                   \
      (D_ENV_CPP_FEATURE_LANG_IMPLICIT_MOVE)              &&                   \
      (D_ENV_CPP_FEATURE_LANG_MULTIDIMENSIONAL_SUBSCRIPT) &&                   \
      (D_ENV_CPP_FEATURE_LANG_NAMED_CHARACTER_ESCAPES)    &&                   \
      (D_ENV_CPP_FEATURE_LANG_SIZE_T_SUFFIX)              &&                   \
      (D_ENV_CPP_FEATURE_LANG_STATIC_CALL_OPERATOR) )

// 3.5.2
// D_ENV_CPP_FEATURE_HAS_ALL_STL_CPP23
//   feature: 1 if all C++23 library features are available.
#define D_ENV_CPP_FEATURE_HAS_ALL_STL_CPP23                                    \
    ( (D_ENV_CPP_FEATURE_STL_ADAPTOR_ITERATOR_PAIR_CONSTRUCTOR) &&             \
      (D_ENV_CPP_FEATURE_STL_ASSOCIATIVE_HETEROGENEOUS_ERASURE) &&             \
      (D_ENV_CPP_FEATURE_STL_BIND_BACK)                         &&             \
      (D_ENV_CPP_FEATURE_STL_BYTESWAP)                          &&             \
      (D_ENV_CPP_FEATURE_STL_CONSTEXPR_BITSET)                  &&             \
      (D_ENV_CPP_FEATURE_STL_CONSTEXPR_CHARCONV)                &&             \
      (D_ENV_CPP_FEATURE_STL_CONSTEXPR_CMATH)                   &&             \
      (D_ENV_CPP_FEATURE_STL_EXPECTED)                          &&             \
      (D_ENV_CPP_FEATURE_STL_FLAT_MAP)                          &&             \
      (D_ENV_CPP_FEATURE_STL_FLAT_SET)                          &&             \
      (D_ENV_CPP_FEATURE_STL_GENERATOR)                         &&             \
      (D_ENV_CPP_FEATURE_STL_MDSPAN)                            &&             \
      (D_ENV_CPP_FEATURE_STL_MOVE_ONLY_FUNCTION)                &&             \
      (D_ENV_CPP_FEATURE_STL_PRINT)                             &&             \
      (D_ENV_CPP_FEATURE_STL_RANGES_TO_CONTAINER)               &&             \
      (D_ENV_CPP_FEATURE_STL_SPANSTREAM)                        &&             \
      (D_ENV_CPP_FEATURE_STL_STACKTRACE)                        &&             \
      (D_ENV_CPP_FEATURE_STL_STDATOMIC_H)                       &&             \
      (D_ENV_CPP_FEATURE_STL_STRING_CONTAINS)                   &&             \
      (D_ENV_CPP_FEATURE_STL_STRING_RESIZE_AND_OVERWRITE)       &&             \
      (D_ENV_CPP_FEATURE_STL_UNREACHABLE) )

// 3.5.3
// D_ENV_CPP_FEATURE_HAS_ALL_CPP23
//   feature: 1 if all C++23 features are available.
#define D_ENV_CPP_FEATURE_HAS_ALL_CPP23                                        \
    ( (D_ENV_CPP_FEATURE_HAS_ALL_LANG_CPP23) &&                                \
      (D_ENV_CPP_FEATURE_HAS_ALL_STL_CPP23) )

// 3.6    C++26
//------------------------------------------------------------------------------
// 3.6.1
// D_ENV_CPP_FEATURE_HAS_ALL_LANG_CPP26
//   feature: 1 if all C++26 language features are available.
#define D_ENV_CPP_FEATURE_HAS_ALL_LANG_CPP26                                   \
    ( (D_ENV_CPP_FEATURE_LANG_CONSTEXPR_EXCEPTIONS)   &&                       \
      (D_ENV_CPP_FEATURE_LANG_CONTRACTS)              &&                       \
      (D_ENV_CPP_FEATURE_LANG_DELETED_FUNCTION)       &&                       \
      (D_ENV_CPP_FEATURE_LANG_PACK_INDEXING)          &&                       \
      (D_ENV_CPP_FEATURE_LANG_PLACEHOLDER_VARIABLES)  &&                       \
      (D_ENV_CPP_FEATURE_LANG_PP_EMBED)               &&                       \
      (D_ENV_CPP_FEATURE_LANG_TEMPLATE_PARAMETERS)    &&                       \
      (D_ENV_CPP_FEATURE_LANG_TRIVIAL_RELOCATABILITY) &&                       \
      (D_ENV_CPP_FEATURE_LANG_TRIVIAL_UNION)          &&                       \
      (D_ENV_CPP_FEATURE_LANG_VARIADIC_FRIEND) )

// 3.6.2
// D_ENV_CPP_FEATURE_HAS_ALL_STL_CPP26
//   feature: 1 if all C++26 library features are available.
#define D_ENV_CPP_FEATURE_HAS_ALL_STL_CPP26                                    \
    ( (D_ENV_CPP_FEATURE_STL_ALGORITHM_DEFAULT_VALUE_TYPE)        &&           \
      (D_ENV_CPP_FEATURE_STL_ASSOCIATIVE_HETEROGENEOUS_INSERTION) &&           \
      (D_ENV_CPP_FEATURE_STL_ATOMIC_MIN_MAX)                      &&           \
      (D_ENV_CPP_FEATURE_STL_CONSTEXPR_ATOMIC)                    &&           \
      (D_ENV_CPP_FEATURE_STL_CONSTEXPR_DEQUE)                     &&           \
      (D_ENV_CPP_FEATURE_STL_CONTRACTS)                           &&           \
      (D_ENV_CPP_FEATURE_STL_COPYABLE_FUNCTION)                   &&           \
      (D_ENV_CPP_FEATURE_STL_DEBUGGING)                           &&           \
      (D_ENV_CPP_FEATURE_STL_FORMAT_PATH)                         &&           \
      (D_ENV_CPP_FEATURE_STL_FUNCTION_REF)                        &&           \
      (D_ENV_CPP_FEATURE_STL_HAZARD_POINTER)                      &&           \
      (D_ENV_CPP_FEATURE_STL_HIVE)                                &&           \
      (D_ENV_CPP_FEATURE_STL_INPLACE_VECTOR)                      &&           \
      (D_ENV_CPP_FEATURE_STL_LINALG)                              &&           \
      (D_ENV_CPP_FEATURE_STL_POLYMORPHIC)                         &&           \
      (D_ENV_CPP_FEATURE_STL_RCU)                                 &&           \
      (D_ENV_CPP_FEATURE_STL_SATURATION_ARITHMETIC)               &&           \
      (D_ENV_CPP_FEATURE_STL_SENDERS)                             &&           \
      (D_ENV_CPP_FEATURE_STL_SIMD)                                &&           \
      (D_ENV_CPP_FEATURE_STL_TEXT_ENCODING) )

// 3.6.3
// D_ENV_CPP_FEATURE_HAS_ALL_CPP26
//   feature: 1 if all C++26 features are available.
#define D_ENV_CPP_FEATURE_HAS_ALL_CPP26                                        \
    ( (D_ENV_CPP_FEATURE_HAS_ALL_LANG_CPP26) &&                                \
      (D_ENV_CPP_FEATURE_HAS_ALL_STL_CPP26) )


#endif  // DJINTERP_ENV_CPP_ENV_CPP_FEATURES_H
