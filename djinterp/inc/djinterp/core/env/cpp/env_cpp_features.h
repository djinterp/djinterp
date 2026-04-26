/******************************************************************************
* djinterp [env]                                          env_cpp_features.hpp
* 
* djinterp C++ feature detection header:
* This header provides comprehensive compile-time detection of C++ language
* and standard library features, including:
*   - Language features (C++11 through C++26)
*   - Standard library features (C++14 through C++26)
*   - Feature grouping by C++ version
*   - Aggregate feature availability checks
*
*   The header creates a unified D_ENV_CPP_FEATURE_* macro interface enabling
* portable code that adapts to different C++ standard versions and compiler
* capabilities. All detection is performed at compile-time with zero runtime
* overhead.
*
* NAMING CONVENTIONS:
*   D_ENV_CPP_FEATURE_LANG_*        - Language features (__cpp_*)
*   D_ENV_CPP_FEATURE_STL_*         - Library features (__cpp_lib_*)
*   D_ENV_CPP_FEATURE_HAS_ALL_*     - Aggregate availability checks
*
*   Each feature has four associated macros:
*     - [NAME]       : 1 if enabled, 0 if not
*     - [NAME]_NAME  : The actual __cpp* macro name (as string)
*     - [NAME]_DESC  : Human-readable description
*     - [NAME]_VAL   : The macro value (or 0L if not defined)
*     - [NAME]_VERS  : C++ version string
*
* 
* path:      /inc/djinterp/core/env/cpp/env_cpp_features.hpp
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.01.15
******************************************************************************/

#ifndef DJINTERP_ENV_CPP_FEATURES_
#define DJINTERP_ENV_CPP_FEATURES_


// =============================================================================
// I.   LANGUAGE FEATURES
// =============================================================================

// -----------------------------------------------------------------------------
// C++11 Language Features
// -----------------------------------------------------------------------------

// D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_alias_templates
    #define D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES  1
#else
    #define D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES  0
#endif

// D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES_NAME  "__cpp_alias_templates"

// D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES_DESC  "Alias templates"

// D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES_VAL
//   constant: feature test value
#ifdef __cpp_alias_templates
    #define D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES_VAL  __cpp_alias_templates
#else
    #define D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES_VAL  0L
#endif

// D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES_VERS  "(C++11)"


// D_ENV_CPP_FEATURE_LANG_ATTRIBUTES
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_attributes
    #define D_ENV_CPP_FEATURE_LANG_ATTRIBUTES  1
#else
    #define D_ENV_CPP_FEATURE_LANG_ATTRIBUTES  0
#endif

// D_ENV_CPP_FEATURE_LANG_ATTRIBUTES_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_LANG_ATTRIBUTES_NAME  "__cpp_attributes"

// D_ENV_CPP_FEATURE_LANG_ATTRIBUTES_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_LANG_ATTRIBUTES_DESC  "Attributes"

// D_ENV_CPP_FEATURE_LANG_ATTRIBUTES_VAL
//   constant: feature test value
#ifdef __cpp_attributes
    #define D_ENV_CPP_FEATURE_LANG_ATTRIBUTES_VAL  __cpp_attributes
#else
    #define D_ENV_CPP_FEATURE_LANG_ATTRIBUTES_VAL  0L
#endif

// D_ENV_CPP_FEATURE_LANG_ATTRIBUTES_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_LANG_ATTRIBUTES_VERS  "(C++11)"


// D_ENV_CPP_FEATURE_LANG_CONSTEXPR
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_constexpr
    #define D_ENV_CPP_FEATURE_LANG_CONSTEXPR  1
#else
    #define D_ENV_CPP_FEATURE_LANG_CONSTEXPR  0
#endif

// D_ENV_CPP_FEATURE_LANG_CONSTEXPR_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_LANG_CONSTEXPR_NAME  "__cpp_constexpr"

// D_ENV_CPP_FEATURE_LANG_CONSTEXPR_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_LANG_CONSTEXPR_DESC  "constexpr"

// D_ENV_CPP_FEATURE_LANG_CONSTEXPR_VAL
//   constant: feature test value
#ifdef __cpp_constexpr
    #define D_ENV_CPP_FEATURE_LANG_CONSTEXPR_VAL  __cpp_constexpr
#else
    #define D_ENV_CPP_FEATURE_LANG_CONSTEXPR_VAL  0L
#endif

// D_ENV_CPP_FEATURE_LANG_CONSTEXPR_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_LANG_CONSTEXPR_VERS  "(C++11)"


// D_ENV_CPP_FEATURE_LANG_DECLTYPE
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_decltype
    #define D_ENV_CPP_FEATURE_LANG_DECLTYPE  1
#else
    #define D_ENV_CPP_FEATURE_LANG_DECLTYPE  0
#endif

// D_ENV_CPP_FEATURE_LANG_DECLTYPE_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_LANG_DECLTYPE_NAME  "__cpp_decltype"

// D_ENV_CPP_FEATURE_LANG_DECLTYPE_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_LANG_DECLTYPE_DESC  "decltype"

// D_ENV_CPP_FEATURE_LANG_DECLTYPE_VAL
//   constant: feature test value
#ifdef __cpp_decltype
    #define D_ENV_CPP_FEATURE_LANG_DECLTYPE_VAL  __cpp_decltype
#else
    #define D_ENV_CPP_FEATURE_LANG_DECLTYPE_VAL  0L
#endif

// D_ENV_CPP_FEATURE_LANG_DECLTYPE_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_LANG_DECLTYPE_VERS  "(C++11)"


// D_ENV_CPP_FEATURE_LANG_DELEGATING_CONSTRUCTORS
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_delegating_constructors
    #define D_ENV_CPP_FEATURE_LANG_DELEGATING_CONSTRUCTORS  1
#else
    #define D_ENV_CPP_FEATURE_LANG_DELEGATING_CONSTRUCTORS  0
#endif

// D_ENV_CPP_FEATURE_LANG_DELEGATING_CONSTRUCTORS_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_LANG_DELEGATING_CONSTRUCTORS_NAME  "__cpp_delegating_constructors"

// D_ENV_CPP_FEATURE_LANG_DELEGATING_CONSTRUCTORS_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_LANG_DELEGATING_CONSTRUCTORS_DESC  "Delegating constructors"

// D_ENV_CPP_FEATURE_LANG_DELEGATING_CONSTRUCTORS_VAL
//   constant: feature test value
#ifdef __cpp_delegating_constructors
    #define D_ENV_CPP_FEATURE_LANG_DELEGATING_CONSTRUCTORS_VAL  __cpp_delegating_constructors
#else
    #define D_ENV_CPP_FEATURE_LANG_DELEGATING_CONSTRUCTORS_VAL  0L
#endif

// D_ENV_CPP_FEATURE_LANG_DELEGATING_CONSTRUCTORS_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_LANG_DELEGATING_CONSTRUCTORS_VERS  "(C++11)"


// D_ENV_CPP_FEATURE_LANG_INHERITING_CONSTRUCTORS
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_inheriting_constructors
    #define D_ENV_CPP_FEATURE_LANG_INHERITING_CONSTRUCTORS  1
#else
    #define D_ENV_CPP_FEATURE_LANG_INHERITING_CONSTRUCTORS  0
#endif

// D_ENV_CPP_FEATURE_LANG_INHERITING_CONSTRUCTORS_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_LANG_INHERITING_CONSTRUCTORS_NAME  "__cpp_inheriting_constructors"

// D_ENV_CPP_FEATURE_LANG_INHERITING_CONSTRUCTORS_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_LANG_INHERITING_CONSTRUCTORS_DESC  "Inheriting constructors"

// D_ENV_CPP_FEATURE_LANG_INHERITING_CONSTRUCTORS_VAL
//   constant: feature test value
#ifdef __cpp_inheriting_constructors
    #define D_ENV_CPP_FEATURE_LANG_INHERITING_CONSTRUCTORS_VAL  __cpp_inheriting_constructors
#else
    #define D_ENV_CPP_FEATURE_LANG_INHERITING_CONSTRUCTORS_VAL  0L
#endif

// D_ENV_CPP_FEATURE_LANG_INHERITING_CONSTRUCTORS_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_LANG_INHERITING_CONSTRUCTORS_VERS  "(C++11)"


// D_ENV_CPP_FEATURE_LANG_INITIALIZER_LISTS
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_initializer_lists
    #define D_ENV_CPP_FEATURE_LANG_INITIALIZER_LISTS  1
#else
    #define D_ENV_CPP_FEATURE_LANG_INITIALIZER_LISTS  0
#endif

// D_ENV_CPP_FEATURE_LANG_INITIALIZER_LISTS_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_LANG_INITIALIZER_LISTS_NAME  "__cpp_initializer_lists"

// D_ENV_CPP_FEATURE_LANG_INITIALIZER_LISTS_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_LANG_INITIALIZER_LISTS_DESC  "List-initialization and std::initializer_list"

// D_ENV_CPP_FEATURE_LANG_INITIALIZER_LISTS_VAL
//   constant: feature test value
#ifdef __cpp_initializer_lists
    #define D_ENV_CPP_FEATURE_LANG_INITIALIZER_LISTS_VAL  __cpp_initializer_lists
#else
    #define D_ENV_CPP_FEATURE_LANG_INITIALIZER_LISTS_VAL  0L
#endif

// D_ENV_CPP_FEATURE_LANG_INITIALIZER_LISTS_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_LANG_INITIALIZER_LISTS_VERS  "(C++11)"


// D_ENV_CPP_FEATURE_LANG_LAMBDAS
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_lambdas
    #define D_ENV_CPP_FEATURE_LANG_LAMBDAS  1
#else
    #define D_ENV_CPP_FEATURE_LANG_LAMBDAS  0
#endif

// D_ENV_CPP_FEATURE_LANG_LAMBDAS_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_LANG_LAMBDAS_NAME  "__cpp_lambdas"

// D_ENV_CPP_FEATURE_LANG_LAMBDAS_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_LANG_LAMBDAS_DESC  "Lambda expressions"

// D_ENV_CPP_FEATURE_LANG_LAMBDAS_VAL
//   constant: feature test value
#ifdef __cpp_lambdas
    #define D_ENV_CPP_FEATURE_LANG_LAMBDAS_VAL  __cpp_lambdas
#else
    #define D_ENV_CPP_FEATURE_LANG_LAMBDAS_VAL  0L
#endif

// D_ENV_CPP_FEATURE_LANG_LAMBDAS_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_LANG_LAMBDAS_VERS  "(C++11)"


// D_ENV_CPP_FEATURE_LANG_NSDMI
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_nsdmi
    #define D_ENV_CPP_FEATURE_LANG_NSDMI  1
#else
    #define D_ENV_CPP_FEATURE_LANG_NSDMI  0
#endif

// D_ENV_CPP_FEATURE_LANG_NSDMI_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_LANG_NSDMI_NAME  "__cpp_nsdmi"

// D_ENV_CPP_FEATURE_LANG_NSDMI_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_LANG_NSDMI_DESC  "Non-static data member initializers"

// D_ENV_CPP_FEATURE_LANG_NSDMI_VAL
//   constant: feature test value
#ifdef __cpp_nsdmi
    #define D_ENV_CPP_FEATURE_LANG_NSDMI_VAL  __cpp_nsdmi
#else
    #define D_ENV_CPP_FEATURE_LANG_NSDMI_VAL  0L
#endif

// D_ENV_CPP_FEATURE_LANG_NSDMI_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_LANG_NSDMI_VERS  "(C++11)"


// D_ENV_CPP_FEATURE_LANG_RANGE_BASED_FOR
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_range_based_for
    #define D_ENV_CPP_FEATURE_LANG_RANGE_BASED_FOR  1
#else
    #define D_ENV_CPP_FEATURE_LANG_RANGE_BASED_FOR  0
#endif

// D_ENV_CPP_FEATURE_LANG_RANGE_BASED_FOR_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_LANG_RANGE_BASED_FOR_NAME  "__cpp_range_based_for"

// D_ENV_CPP_FEATURE_LANG_RANGE_BASED_FOR_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_LANG_RANGE_BASED_FOR_DESC  "Range-based for loop"

// D_ENV_CPP_FEATURE_LANG_RANGE_BASED_FOR_VAL
//   constant: feature test value
#ifdef __cpp_range_based_for
    #define D_ENV_CPP_FEATURE_LANG_RANGE_BASED_FOR_VAL  __cpp_range_based_for
#else
    #define D_ENV_CPP_FEATURE_LANG_RANGE_BASED_FOR_VAL  0L
#endif

// D_ENV_CPP_FEATURE_LANG_RANGE_BASED_FOR_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_LANG_RANGE_BASED_FOR_VERS  "(C++11)"


// D_ENV_CPP_FEATURE_LANG_RAW_STRINGS
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_raw_strings
    #define D_ENV_CPP_FEATURE_LANG_RAW_STRINGS  1
#else
    #define D_ENV_CPP_FEATURE_LANG_RAW_STRINGS  0
#endif

// D_ENV_CPP_FEATURE_LANG_RAW_STRINGS_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_LANG_RAW_STRINGS_NAME  "__cpp_raw_strings"

// D_ENV_CPP_FEATURE_LANG_RAW_STRINGS_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_LANG_RAW_STRINGS_DESC  "Raw string literals"

// D_ENV_CPP_FEATURE_LANG_RAW_STRINGS_VAL
//   constant: feature test value
#ifdef __cpp_raw_strings
    #define D_ENV_CPP_FEATURE_LANG_RAW_STRINGS_VAL  __cpp_raw_strings
#else
    #define D_ENV_CPP_FEATURE_LANG_RAW_STRINGS_VAL  0L
#endif

// D_ENV_CPP_FEATURE_LANG_RAW_STRINGS_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_LANG_RAW_STRINGS_VERS  "(C++11)"


// D_ENV_CPP_FEATURE_LANG_REF_QUALIFIERS
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_ref_qualifiers
    #define D_ENV_CPP_FEATURE_LANG_REF_QUALIFIERS  1
#else
    #define D_ENV_CPP_FEATURE_LANG_REF_QUALIFIERS  0
#endif

// D_ENV_CPP_FEATURE_LANG_REF_QUALIFIERS_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_LANG_REF_QUALIFIERS_NAME  "__cpp_ref_qualifiers"

// D_ENV_CPP_FEATURE_LANG_REF_QUALIFIERS_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_LANG_REF_QUALIFIERS_DESC  "ref-qualifiers"

// D_ENV_CPP_FEATURE_LANG_REF_QUALIFIERS_VAL
//   constant: feature test value
#ifdef __cpp_ref_qualifiers
    #define D_ENV_CPP_FEATURE_LANG_REF_QUALIFIERS_VAL  __cpp_ref_qualifiers
#else
    #define D_ENV_CPP_FEATURE_LANG_REF_QUALIFIERS_VAL  0L
#endif

// D_ENV_CPP_FEATURE_LANG_REF_QUALIFIERS_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_LANG_REF_QUALIFIERS_VERS  "(C++11)"


// D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_rvalue_references
    #define D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES  1
#else
    #define D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES  0
#endif

// D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES_NAME  "__cpp_rvalue_references"

// D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES_DESC  "Rvalue reference"

// D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES_VAL
//   constant: feature test value
#ifdef __cpp_rvalue_references
    #define D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES_VAL  __cpp_rvalue_references
#else
    #define D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES_VAL  0L
#endif

// D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES_VERS  "(C++11)"


// D_ENV_CPP_FEATURE_LANG_STATIC_ASSERT
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_static_assert
    #define D_ENV_CPP_FEATURE_LANG_STATIC_ASSERT  1
#else
    #define D_ENV_CPP_FEATURE_LANG_STATIC_ASSERT  0
#endif

// D_ENV_CPP_FEATURE_LANG_STATIC_ASSERT_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_LANG_STATIC_ASSERT_NAME  "__cpp_static_assert"

// D_ENV_CPP_FEATURE_LANG_STATIC_ASSERT_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_LANG_STATIC_ASSERT_DESC  "static_assert"

// D_ENV_CPP_FEATURE_LANG_STATIC_ASSERT_VAL
//   constant: feature test value
#ifdef __cpp_static_assert
    #define D_ENV_CPP_FEATURE_LANG_STATIC_ASSERT_VAL  __cpp_static_assert
#else
    #define D_ENV_CPP_FEATURE_LANG_STATIC_ASSERT_VAL  0L
#endif

// D_ENV_CPP_FEATURE_LANG_STATIC_ASSERT_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_LANG_STATIC_ASSERT_VERS  "(C++11)"


// D_ENV_CPP_FEATURE_LANG_THREADSAFE_STATIC_INIT
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_threadsafe_static_init
    #define D_ENV_CPP_FEATURE_LANG_THREADSAFE_STATIC_INIT  1
#else
    #define D_ENV_CPP_FEATURE_LANG_THREADSAFE_STATIC_INIT  0
#endif

// D_ENV_CPP_FEATURE_LANG_THREADSAFE_STATIC_INIT_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_LANG_THREADSAFE_STATIC_INIT_NAME  "__cpp_threadsafe_static_init"

// D_ENV_CPP_FEATURE_LANG_THREADSAFE_STATIC_INIT_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_LANG_THREADSAFE_STATIC_INIT_DESC  "Dynamic initialization and destruction with concurrency"

// D_ENV_CPP_FEATURE_LANG_THREADSAFE_STATIC_INIT_VAL
//   constant: feature test value
#ifdef __cpp_threadsafe_static_init
    #define D_ENV_CPP_FEATURE_LANG_THREADSAFE_STATIC_INIT_VAL  __cpp_threadsafe_static_init
#else
    #define D_ENV_CPP_FEATURE_LANG_THREADSAFE_STATIC_INIT_VAL  0L
#endif

// D_ENV_CPP_FEATURE_LANG_THREADSAFE_STATIC_INIT_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_LANG_THREADSAFE_STATIC_INIT_VERS  "(C++11)"


// D_ENV_CPP_FEATURE_LANG_UNICODE_CHARACTERS
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_unicode_characters
    #define D_ENV_CPP_FEATURE_LANG_UNICODE_CHARACTERS  1
#else
    #define D_ENV_CPP_FEATURE_LANG_UNICODE_CHARACTERS  0
#endif

// D_ENV_CPP_FEATURE_LANG_UNICODE_CHARACTERS_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_LANG_UNICODE_CHARACTERS_NAME  "__cpp_unicode_characters"

// D_ENV_CPP_FEATURE_LANG_UNICODE_CHARACTERS_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_LANG_UNICODE_CHARACTERS_DESC  "New character types (char16_t and char32_t)"

// D_ENV_CPP_FEATURE_LANG_UNICODE_CHARACTERS_VAL
//   constant: feature test value
#ifdef __cpp_unicode_characters
    #define D_ENV_CPP_FEATURE_LANG_UNICODE_CHARACTERS_VAL  __cpp_unicode_characters
#else
    #define D_ENV_CPP_FEATURE_LANG_UNICODE_CHARACTERS_VAL  0L
#endif

// D_ENV_CPP_FEATURE_LANG_UNICODE_CHARACTERS_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_LANG_UNICODE_CHARACTERS_VERS  "(C++11)"


// D_ENV_CPP_FEATURE_LANG_UNICODE_LITERALS
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_unicode_literals
    #define D_ENV_CPP_FEATURE_LANG_UNICODE_LITERALS  1
#else
    #define D_ENV_CPP_FEATURE_LANG_UNICODE_LITERALS  0
#endif

// D_ENV_CPP_FEATURE_LANG_UNICODE_LITERALS_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_LANG_UNICODE_LITERALS_NAME  "__cpp_unicode_literals"

// D_ENV_CPP_FEATURE_LANG_UNICODE_LITERALS_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_LANG_UNICODE_LITERALS_DESC  "Unicode string literals"

// D_ENV_CPP_FEATURE_LANG_UNICODE_LITERALS_VAL
//   constant: feature test value
#ifdef __cpp_unicode_literals
    #define D_ENV_CPP_FEATURE_LANG_UNICODE_LITERALS_VAL  __cpp_unicode_literals
#else
    #define D_ENV_CPP_FEATURE_LANG_UNICODE_LITERALS_VAL  0L
#endif

// D_ENV_CPP_FEATURE_LANG_UNICODE_LITERALS_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_LANG_UNICODE_LITERALS_VERS  "(C++11)"


// D_ENV_CPP_FEATURE_LANG_USER_DEFINED_LITERALS
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_user_defined_literals
    #define D_ENV_CPP_FEATURE_LANG_USER_DEFINED_LITERALS  1
#else
    #define D_ENV_CPP_FEATURE_LANG_USER_DEFINED_LITERALS  0
#endif

// D_ENV_CPP_FEATURE_LANG_USER_DEFINED_LITERALS_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_LANG_USER_DEFINED_LITERALS_NAME  "__cpp_user_defined_literals"

// D_ENV_CPP_FEATURE_LANG_USER_DEFINED_LITERALS_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_LANG_USER_DEFINED_LITERALS_DESC  "User-defined literals"

// D_ENV_CPP_FEATURE_LANG_USER_DEFINED_LITERALS_VAL
//   constant: feature test value
#ifdef __cpp_user_defined_literals
    #define D_ENV_CPP_FEATURE_LANG_USER_DEFINED_LITERALS_VAL  __cpp_user_defined_literals
#else
    #define D_ENV_CPP_FEATURE_LANG_USER_DEFINED_LITERALS_VAL  0L
#endif

// D_ENV_CPP_FEATURE_LANG_USER_DEFINED_LITERALS_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_LANG_USER_DEFINED_LITERALS_VERS  "(C++11)"


// D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_variadic_templates
    #define D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES  1
#else
    #define D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES  0
#endif

// D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES_NAME  "__cpp_variadic_templates"

// D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES_DESC  "Variadic templates"

// D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES_VAL
//   constant: feature test value
#ifdef __cpp_variadic_templates
    #define D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES_VAL  __cpp_variadic_templates
#else
    #define D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES_VAL  0L
#endif

// D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES_VERS  "(C++11)"


// -----------------------------------------------------------------------------
// C++14 Language Features
// -----------------------------------------------------------------------------

// D_ENV_CPP_FEATURE_LANG_AGGREGATE_NSDMI
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_aggregate_nsdmi
    #define D_ENV_CPP_FEATURE_LANG_AGGREGATE_NSDMI  1
#else
    #define D_ENV_CPP_FEATURE_LANG_AGGREGATE_NSDMI  0
#endif

// D_ENV_CPP_FEATURE_LANG_AGGREGATE_NSDMI_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_LANG_AGGREGATE_NSDMI_NAME  "__cpp_aggregate_nsdmi"

// D_ENV_CPP_FEATURE_LANG_AGGREGATE_NSDMI_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_LANG_AGGREGATE_NSDMI_DESC  "Aggregate classes with default member initializers"

// D_ENV_CPP_FEATURE_LANG_AGGREGATE_NSDMI_VAL
//   constant: feature test value
#ifdef __cpp_aggregate_nsdmi
    #define D_ENV_CPP_FEATURE_LANG_AGGREGATE_NSDMI_VAL  __cpp_aggregate_nsdmi
#else
    #define D_ENV_CPP_FEATURE_LANG_AGGREGATE_NSDMI_VAL  0L
#endif

// D_ENV_CPP_FEATURE_LANG_AGGREGATE_NSDMI_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_LANG_AGGREGATE_NSDMI_VERS  "(C++14)"


// D_ENV_CPP_FEATURE_LANG_BINARY_LITERALS
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_binary_literals
    #define D_ENV_CPP_FEATURE_LANG_BINARY_LITERALS  1
#else
    #define D_ENV_CPP_FEATURE_LANG_BINARY_LITERALS  0
#endif

// D_ENV_CPP_FEATURE_LANG_BINARY_LITERALS_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_LANG_BINARY_LITERALS_NAME  "__cpp_binary_literals"

// D_ENV_CPP_FEATURE_LANG_BINARY_LITERALS_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_LANG_BINARY_LITERALS_DESC  "Binary literals"

// D_ENV_CPP_FEATURE_LANG_BINARY_LITERALS_VAL
//   constant: feature test value
#ifdef __cpp_binary_literals
    #define D_ENV_CPP_FEATURE_LANG_BINARY_LITERALS_VAL  __cpp_binary_literals
#else
    #define D_ENV_CPP_FEATURE_LANG_BINARY_LITERALS_VAL  0L
#endif

// D_ENV_CPP_FEATURE_LANG_BINARY_LITERALS_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_LANG_BINARY_LITERALS_VERS  "(C++14)"


// D_ENV_CPP_FEATURE_LANG_DECLTYPE_AUTO
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_decltype_auto
    #define D_ENV_CPP_FEATURE_LANG_DECLTYPE_AUTO  1
#else
    #define D_ENV_CPP_FEATURE_LANG_DECLTYPE_AUTO  0
#endif

// D_ENV_CPP_FEATURE_LANG_DECLTYPE_AUTO_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_LANG_DECLTYPE_AUTO_NAME  "__cpp_decltype_auto"

// D_ENV_CPP_FEATURE_LANG_DECLTYPE_AUTO_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_LANG_DECLTYPE_AUTO_DESC  "Return type deduction for normal functions"

// D_ENV_CPP_FEATURE_LANG_DECLTYPE_AUTO_VAL
//   constant: feature test value
#ifdef __cpp_decltype_auto
    #define D_ENV_CPP_FEATURE_LANG_DECLTYPE_AUTO_VAL  __cpp_decltype_auto
#else
    #define D_ENV_CPP_FEATURE_LANG_DECLTYPE_AUTO_VAL  0L
#endif

// D_ENV_CPP_FEATURE_LANG_DECLTYPE_AUTO_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_LANG_DECLTYPE_AUTO_VERS  "(C++14)"


// D_ENV_CPP_FEATURE_LANG_ENUMERATOR_ATTRIBUTES
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_enumerator_attributes
    #define D_ENV_CPP_FEATURE_LANG_ENUMERATOR_ATTRIBUTES  1
#else
    #define D_ENV_CPP_FEATURE_LANG_ENUMERATOR_ATTRIBUTES  0
#endif

// D_ENV_CPP_FEATURE_LANG_ENUMERATOR_ATTRIBUTES_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_LANG_ENUMERATOR_ATTRIBUTES_NAME  "__cpp_enumerator_attributes"

// D_ENV_CPP_FEATURE_LANG_ENUMERATOR_ATTRIBUTES_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_LANG_ENUMERATOR_ATTRIBUTES_DESC  "Attributes for enumerators"

// D_ENV_CPP_FEATURE_LANG_ENUMERATOR_ATTRIBUTES_VAL
//   constant: feature test value
#ifdef __cpp_enumerator_attributes
    #define D_ENV_CPP_FEATURE_LANG_ENUMERATOR_ATTRIBUTES_VAL  __cpp_enumerator_attributes
#else
    #define D_ENV_CPP_FEATURE_LANG_ENUMERATOR_ATTRIBUTES_VAL  0L
#endif

// D_ENV_CPP_FEATURE_LANG_ENUMERATOR_ATTRIBUTES_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_LANG_ENUMERATOR_ATTRIBUTES_VERS  "(C++14)"


// D_ENV_CPP_FEATURE_LANG_GENERIC_LAMBDAS
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_generic_lambdas
    #define D_ENV_CPP_FEATURE_LANG_GENERIC_LAMBDAS  1
#else
    #define D_ENV_CPP_FEATURE_LANG_GENERIC_LAMBDAS  0
#endif

// D_ENV_CPP_FEATURE_LANG_GENERIC_LAMBDAS_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_LANG_GENERIC_LAMBDAS_NAME  "__cpp_generic_lambdas"

// D_ENV_CPP_FEATURE_LANG_GENERIC_LAMBDAS_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_LANG_GENERIC_LAMBDAS_DESC  "Generic lambda expressions"

// D_ENV_CPP_FEATURE_LANG_GENERIC_LAMBDAS_VAL
//   constant: feature test value
#ifdef __cpp_generic_lambdas
    #define D_ENV_CPP_FEATURE_LANG_GENERIC_LAMBDAS_VAL  __cpp_generic_lambdas
#else
    #define D_ENV_CPP_FEATURE_LANG_GENERIC_LAMBDAS_VAL  0L
#endif

// D_ENV_CPP_FEATURE_LANG_GENERIC_LAMBDAS_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_LANG_GENERIC_LAMBDAS_VERS  "(C++14)"


// D_ENV_CPP_FEATURE_LANG_INIT_CAPTURES
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_init_captures
    #define D_ENV_CPP_FEATURE_LANG_INIT_CAPTURES  1
#else
    #define D_ENV_CPP_FEATURE_LANG_INIT_CAPTURES  0
#endif

// D_ENV_CPP_FEATURE_LANG_INIT_CAPTURES_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_LANG_INIT_CAPTURES_NAME  "__cpp_init_captures"

// D_ENV_CPP_FEATURE_LANG_INIT_CAPTURES_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_LANG_INIT_CAPTURES_DESC  "Lambda init-capture"

// D_ENV_CPP_FEATURE_LANG_INIT_CAPTURES_VAL
//   constant: feature test value
#ifdef __cpp_init_captures
    #define D_ENV_CPP_FEATURE_LANG_INIT_CAPTURES_VAL  __cpp_init_captures
#else
    #define D_ENV_CPP_FEATURE_LANG_INIT_CAPTURES_VAL  0L
#endif

// D_ENV_CPP_FEATURE_LANG_INIT_CAPTURES_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_LANG_INIT_CAPTURES_VERS  "(C++14)"


// D_ENV_CPP_FEATURE_LANG_NAMESPACE_ATTRIBUTES
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_namespace_attributes
    #define D_ENV_CPP_FEATURE_LANG_NAMESPACE_ATTRIBUTES  1
#else
    #define D_ENV_CPP_FEATURE_LANG_NAMESPACE_ATTRIBUTES  0
#endif

// D_ENV_CPP_FEATURE_LANG_NAMESPACE_ATTRIBUTES_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_LANG_NAMESPACE_ATTRIBUTES_NAME  "__cpp_namespace_attributes"

// D_ENV_CPP_FEATURE_LANG_NAMESPACE_ATTRIBUTES_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_LANG_NAMESPACE_ATTRIBUTES_DESC  "Attributes for namespaces"

// D_ENV_CPP_FEATURE_LANG_NAMESPACE_ATTRIBUTES_VAL
//   constant: feature test value
#ifdef __cpp_namespace_attributes
    #define D_ENV_CPP_FEATURE_LANG_NAMESPACE_ATTRIBUTES_VAL  __cpp_namespace_attributes
#else
    #define D_ENV_CPP_FEATURE_LANG_NAMESPACE_ATTRIBUTES_VAL  0L
#endif

// D_ENV_CPP_FEATURE_LANG_NAMESPACE_ATTRIBUTES_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_LANG_NAMESPACE_ATTRIBUTES_VERS  "(C++14)"


// D_ENV_CPP_FEATURE_LANG_NONTYPE_TEMPLATE_ARGS
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_nontype_template_args
    #define D_ENV_CPP_FEATURE_LANG_NONTYPE_TEMPLATE_ARGS  1
#else
    #define D_ENV_CPP_FEATURE_LANG_NONTYPE_TEMPLATE_ARGS  0
#endif

// D_ENV_CPP_FEATURE_LANG_NONTYPE_TEMPLATE_ARGS_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_LANG_NONTYPE_TEMPLATE_ARGS_NAME  "__cpp_nontype_template_args"

// D_ENV_CPP_FEATURE_LANG_NONTYPE_TEMPLATE_ARGS_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_LANG_NONTYPE_TEMPLATE_ARGS_DESC  "Allow constant evaluation for all constant template arguments"

// D_ENV_CPP_FEATURE_LANG_NONTYPE_TEMPLATE_ARGS_VAL
//   constant: feature test value
#ifdef __cpp_nontype_template_args
    #define D_ENV_CPP_FEATURE_LANG_NONTYPE_TEMPLATE_ARGS_VAL  __cpp_nontype_template_args
#else
    #define D_ENV_CPP_FEATURE_LANG_NONTYPE_TEMPLATE_ARGS_VAL  0L
#endif

// D_ENV_CPP_FEATURE_LANG_NONTYPE_TEMPLATE_ARGS_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_LANG_NONTYPE_TEMPLATE_ARGS_VERS  "(C++14)"


// D_ENV_CPP_FEATURE_LANG_RETURN_TYPE_DEDUCTION
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_return_type_deduction
    #define D_ENV_CPP_FEATURE_LANG_RETURN_TYPE_DEDUCTION  1
#else
    #define D_ENV_CPP_FEATURE_LANG_RETURN_TYPE_DEDUCTION  0
#endif

// D_ENV_CPP_FEATURE_LANG_RETURN_TYPE_DEDUCTION_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_LANG_RETURN_TYPE_DEDUCTION_NAME  "__cpp_return_type_deduction"

// D_ENV_CPP_FEATURE_LANG_RETURN_TYPE_DEDUCTION_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_LANG_RETURN_TYPE_DEDUCTION_DESC  "Return type deduction for normal functions"

// D_ENV_CPP_FEATURE_LANG_RETURN_TYPE_DEDUCTION_VAL
//   constant: feature test value
#ifdef __cpp_return_type_deduction
    #define D_ENV_CPP_FEATURE_LANG_RETURN_TYPE_DEDUCTION_VAL  __cpp_return_type_deduction
#else
    #define D_ENV_CPP_FEATURE_LANG_RETURN_TYPE_DEDUCTION_VAL  0L
#endif

// D_ENV_CPP_FEATURE_LANG_RETURN_TYPE_DEDUCTION_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_LANG_RETURN_TYPE_DEDUCTION_VERS  "(C++14)"


// D_ENV_CPP_FEATURE_LANG_SIZED_DEALLOCATION
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_sized_deallocation
    #define D_ENV_CPP_FEATURE_LANG_SIZED_DEALLOCATION  1
#else
    #define D_ENV_CPP_FEATURE_LANG_SIZED_DEALLOCATION  0
#endif

// D_ENV_CPP_FEATURE_LANG_SIZED_DEALLOCATION_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_LANG_SIZED_DEALLOCATION_NAME  "__cpp_sized_deallocation"

// D_ENV_CPP_FEATURE_LANG_SIZED_DEALLOCATION_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_LANG_SIZED_DEALLOCATION_DESC  "Sized deallocation"

// D_ENV_CPP_FEATURE_LANG_SIZED_DEALLOCATION_VAL
//   constant: feature test value
#ifdef __cpp_sized_deallocation
    #define D_ENV_CPP_FEATURE_LANG_SIZED_DEALLOCATION_VAL  __cpp_sized_deallocation
#else
    #define D_ENV_CPP_FEATURE_LANG_SIZED_DEALLOCATION_VAL  0L
#endif

// D_ENV_CPP_FEATURE_LANG_SIZED_DEALLOCATION_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_LANG_SIZED_DEALLOCATION_VERS  "(C++14)"


// D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_variable_templates
    #define D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES  1
#else
    #define D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES  0
#endif

// D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES_NAME  "__cpp_variable_templates"

// D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES_DESC  "Variable templates"

// D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES_VAL
//   constant: feature test value
#ifdef __cpp_variable_templates
    #define D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES_VAL  __cpp_variable_templates
#else
    #define D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES_VAL  0L
#endif

// D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES_VERS  "(C++14)"


// -----------------------------------------------------------------------------
// C++17 Language Features
// -----------------------------------------------------------------------------

// D_ENV_CPP_FEATURE_LANG_AGGREGATE_BASES
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_aggregate_bases
    #define D_ENV_CPP_FEATURE_LANG_AGGREGATE_BASES  1
#else
    #define D_ENV_CPP_FEATURE_LANG_AGGREGATE_BASES  0
#endif

// D_ENV_CPP_FEATURE_LANG_AGGREGATE_BASES_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_LANG_AGGREGATE_BASES_NAME  "__cpp_aggregate_bases"

// D_ENV_CPP_FEATURE_LANG_AGGREGATE_BASES_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_LANG_AGGREGATE_BASES_DESC  "Aggregate classes with base classes"

// D_ENV_CPP_FEATURE_LANG_AGGREGATE_BASES_VAL
//   constant: feature test value
#ifdef __cpp_aggregate_bases
    #define D_ENV_CPP_FEATURE_LANG_AGGREGATE_BASES_VAL  __cpp_aggregate_bases
#else
    #define D_ENV_CPP_FEATURE_LANG_AGGREGATE_BASES_VAL  0L
#endif

// D_ENV_CPP_FEATURE_LANG_AGGREGATE_BASES_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_LANG_AGGREGATE_BASES_VERS  "(C++17)"


// D_ENV_CPP_FEATURE_LANG_ALIGNED_NEW
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_aligned_new
    #define D_ENV_CPP_FEATURE_LANG_ALIGNED_NEW  1
#else
    #define D_ENV_CPP_FEATURE_LANG_ALIGNED_NEW  0
#endif

// D_ENV_CPP_FEATURE_LANG_ALIGNED_NEW_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_LANG_ALIGNED_NEW_NAME  "__cpp_aligned_new"

// D_ENV_CPP_FEATURE_LANG_ALIGNED_NEW_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_LANG_ALIGNED_NEW_DESC  "Dynamic memory allocation for over-aligned data"

// D_ENV_CPP_FEATURE_LANG_ALIGNED_NEW_VAL
//   constant: feature test value
#ifdef __cpp_aligned_new
    #define D_ENV_CPP_FEATURE_LANG_ALIGNED_NEW_VAL  __cpp_aligned_new
#else
    #define D_ENV_CPP_FEATURE_LANG_ALIGNED_NEW_VAL  0L
#endif

// D_ENV_CPP_FEATURE_LANG_ALIGNED_NEW_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_LANG_ALIGNED_NEW_VERS  "(C++17)"


// D_ENV_CPP_FEATURE_LANG_CAPTURE_STAR_THIS
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_capture_star_this
    #define D_ENV_CPP_FEATURE_LANG_CAPTURE_STAR_THIS  1
#else
    #define D_ENV_CPP_FEATURE_LANG_CAPTURE_STAR_THIS  0
#endif

// D_ENV_CPP_FEATURE_LANG_CAPTURE_STAR_THIS_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_LANG_CAPTURE_STAR_THIS_NAME  "__cpp_capture_star_this"

// D_ENV_CPP_FEATURE_LANG_CAPTURE_STAR_THIS_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_LANG_CAPTURE_STAR_THIS_DESC  "Lambda capture of *this by value as [=,*this]"

// D_ENV_CPP_FEATURE_LANG_CAPTURE_STAR_THIS_VAL
//   constant: feature test value
#ifdef __cpp_capture_star_this
    #define D_ENV_CPP_FEATURE_LANG_CAPTURE_STAR_THIS_VAL  __cpp_capture_star_this
#else
    #define D_ENV_CPP_FEATURE_LANG_CAPTURE_STAR_THIS_VAL  0L
#endif

// D_ENV_CPP_FEATURE_LANG_CAPTURE_STAR_THIS_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_LANG_CAPTURE_STAR_THIS_VERS  "(C++17)"


// D_ENV_CPP_FEATURE_LANG_CONSTEXPR_IN_DECLTYPE
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_constexpr_in_decltype
    #define D_ENV_CPP_FEATURE_LANG_CONSTEXPR_IN_DECLTYPE  1
#else
    #define D_ENV_CPP_FEATURE_LANG_CONSTEXPR_IN_DECLTYPE  0
#endif

// D_ENV_CPP_FEATURE_LANG_CONSTEXPR_IN_DECLTYPE_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_LANG_CONSTEXPR_IN_DECLTYPE_NAME  "__cpp_constexpr_in_decltype"

// D_ENV_CPP_FEATURE_LANG_CONSTEXPR_IN_DECLTYPE_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_LANG_CONSTEXPR_IN_DECLTYPE_DESC  "Generation of function and variable definitions when needed for constant evaluation"

// D_ENV_CPP_FEATURE_LANG_CONSTEXPR_IN_DECLTYPE_VAL
//   constant: feature test value
#ifdef __cpp_constexpr_in_decltype
    #define D_ENV_CPP_FEATURE_LANG_CONSTEXPR_IN_DECLTYPE_VAL  __cpp_constexpr_in_decltype
#else
    #define D_ENV_CPP_FEATURE_LANG_CONSTEXPR_IN_DECLTYPE_VAL  0L
#endif

// D_ENV_CPP_FEATURE_LANG_CONSTEXPR_IN_DECLTYPE_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_LANG_CONSTEXPR_IN_DECLTYPE_VERS  "(C++17)"


// D_ENV_CPP_FEATURE_LANG_DEDUCTION_GUIDES
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_deduction_guides
    #define D_ENV_CPP_FEATURE_LANG_DEDUCTION_GUIDES  1
#else
    #define D_ENV_CPP_FEATURE_LANG_DEDUCTION_GUIDES  0
#endif

// D_ENV_CPP_FEATURE_LANG_DEDUCTION_GUIDES_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_LANG_DEDUCTION_GUIDES_NAME  "__cpp_deduction_guides"

// D_ENV_CPP_FEATURE_LANG_DEDUCTION_GUIDES_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_LANG_DEDUCTION_GUIDES_DESC  "Template argument deduction for class templates (CTAD)"

// D_ENV_CPP_FEATURE_LANG_DEDUCTION_GUIDES_VAL
//   constant: feature test value
#ifdef __cpp_deduction_guides
    #define D_ENV_CPP_FEATURE_LANG_DEDUCTION_GUIDES_VAL  __cpp_deduction_guides
#else
    #define D_ENV_CPP_FEATURE_LANG_DEDUCTION_GUIDES_VAL  0L
#endif

// D_ENV_CPP_FEATURE_LANG_DEDUCTION_GUIDES_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_LANG_DEDUCTION_GUIDES_VERS  "(C++17)"


// D_ENV_CPP_FEATURE_LANG_FOLD_EXPRESSIONS
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_fold_expressions
    #define D_ENV_CPP_FEATURE_LANG_FOLD_EXPRESSIONS  1
#else
    #define D_ENV_CPP_FEATURE_LANG_FOLD_EXPRESSIONS  0
#endif

// D_ENV_CPP_FEATURE_LANG_FOLD_EXPRESSIONS_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_LANG_FOLD_EXPRESSIONS_NAME  "__cpp_fold_expressions"

// D_ENV_CPP_FEATURE_LANG_FOLD_EXPRESSIONS_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_LANG_FOLD_EXPRESSIONS_DESC  "Fold expressions"

// D_ENV_CPP_FEATURE_LANG_FOLD_EXPRESSIONS_VAL
//   constant: feature test value
#ifdef __cpp_fold_expressions
    #define D_ENV_CPP_FEATURE_LANG_FOLD_EXPRESSIONS_VAL  __cpp_fold_expressions
#else
    #define D_ENV_CPP_FEATURE_LANG_FOLD_EXPRESSIONS_VAL  0L
#endif

// D_ENV_CPP_FEATURE_LANG_FOLD_EXPRESSIONS_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_LANG_FOLD_EXPRESSIONS_VERS  "(C++17)"


// D_ENV_CPP_FEATURE_LANG_GUARANTEED_COPY_ELISION
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_guaranteed_copy_elision
    #define D_ENV_CPP_FEATURE_LANG_GUARANTEED_COPY_ELISION  1
#else
    #define D_ENV_CPP_FEATURE_LANG_GUARANTEED_COPY_ELISION  0
#endif

// D_ENV_CPP_FEATURE_LANG_GUARANTEED_COPY_ELISION_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_LANG_GUARANTEED_COPY_ELISION_NAME  "__cpp_guaranteed_copy_elision"

// D_ENV_CPP_FEATURE_LANG_GUARANTEED_COPY_ELISION_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_LANG_GUARANTEED_COPY_ELISION_DESC  "Guaranteed copy elision through simplified value categories"

// D_ENV_CPP_FEATURE_LANG_GUARANTEED_COPY_ELISION_VAL
//   constant: feature test value
#ifdef __cpp_guaranteed_copy_elision
    #define D_ENV_CPP_FEATURE_LANG_GUARANTEED_COPY_ELISION_VAL  __cpp_guaranteed_copy_elision
#else
    #define D_ENV_CPP_FEATURE_LANG_GUARANTEED_COPY_ELISION_VAL  0L
#endif

// D_ENV_CPP_FEATURE_LANG_GUARANTEED_COPY_ELISION_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_LANG_GUARANTEED_COPY_ELISION_VERS  "(C++17)"


// D_ENV_CPP_FEATURE_LANG_HEX_FLOAT
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_hex_float
    #define D_ENV_CPP_FEATURE_LANG_HEX_FLOAT  1
#else
    #define D_ENV_CPP_FEATURE_LANG_HEX_FLOAT  0
#endif

// D_ENV_CPP_FEATURE_LANG_HEX_FLOAT_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_LANG_HEX_FLOAT_NAME  "__cpp_hex_float"

// D_ENV_CPP_FEATURE_LANG_HEX_FLOAT_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_LANG_HEX_FLOAT_DESC  "Hexadecimal floating literals"

// D_ENV_CPP_FEATURE_LANG_HEX_FLOAT_VAL
//   constant: feature test value
#ifdef __cpp_hex_float
    #define D_ENV_CPP_FEATURE_LANG_HEX_FLOAT_VAL  __cpp_hex_float
#else
    #define D_ENV_CPP_FEATURE_LANG_HEX_FLOAT_VAL  0L
#endif

// D_ENV_CPP_FEATURE_LANG_HEX_FLOAT_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_LANG_HEX_FLOAT_VERS  "(C++17)"


// D_ENV_CPP_FEATURE_LANG_IF_CONSTEXPR
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_if_constexpr
    #define D_ENV_CPP_FEATURE_LANG_IF_CONSTEXPR  1
#else
    #define D_ENV_CPP_FEATURE_LANG_IF_CONSTEXPR  0
#endif

// D_ENV_CPP_FEATURE_LANG_IF_CONSTEXPR_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_LANG_IF_CONSTEXPR_NAME  "__cpp_if_constexpr"

// D_ENV_CPP_FEATURE_LANG_IF_CONSTEXPR_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_LANG_IF_CONSTEXPR_DESC  "if constexpr"

// D_ENV_CPP_FEATURE_LANG_IF_CONSTEXPR_VAL
//   constant: feature test value
#ifdef __cpp_if_constexpr
    #define D_ENV_CPP_FEATURE_LANG_IF_CONSTEXPR_VAL  __cpp_if_constexpr
#else
    #define D_ENV_CPP_FEATURE_LANG_IF_CONSTEXPR_VAL  0L
#endif

// D_ENV_CPP_FEATURE_LANG_IF_CONSTEXPR_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_LANG_IF_CONSTEXPR_VERS  "(C++17)"


// D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_inline_variables
    #define D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES  1
#else
    #define D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES  0
#endif

// D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES_NAME  "__cpp_inline_variables"

// D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES_DESC  "Inline variables"

// D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES_VAL
//   constant: feature test value
#ifdef __cpp_inline_variables
    #define D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES_VAL  __cpp_inline_variables
#else
    #define D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES_VAL  0L
#endif

// D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES_VERS  "(C++17)"


// D_ENV_CPP_FEATURE_LANG_NOEXCEPT_FUNCTION_TYPE
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_noexcept_function_type
    #define D_ENV_CPP_FEATURE_LANG_NOEXCEPT_FUNCTION_TYPE  1
#else
    #define D_ENV_CPP_FEATURE_LANG_NOEXCEPT_FUNCTION_TYPE  0
#endif

// D_ENV_CPP_FEATURE_LANG_NOEXCEPT_FUNCTION_TYPE_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_LANG_NOEXCEPT_FUNCTION_TYPE_NAME  "__cpp_noexcept_function_type"

// D_ENV_CPP_FEATURE_LANG_NOEXCEPT_FUNCTION_TYPE_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_LANG_NOEXCEPT_FUNCTION_TYPE_DESC  "Make exception specifications be part of the type system"

// D_ENV_CPP_FEATURE_LANG_NOEXCEPT_FUNCTION_TYPE_VAL
//   constant: feature test value
#ifdef __cpp_noexcept_function_type
    #define D_ENV_CPP_FEATURE_LANG_NOEXCEPT_FUNCTION_TYPE_VAL  __cpp_noexcept_function_type
#else
    #define D_ENV_CPP_FEATURE_LANG_NOEXCEPT_FUNCTION_TYPE_VAL  0L
#endif

// D_ENV_CPP_FEATURE_LANG_NOEXCEPT_FUNCTION_TYPE_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_LANG_NOEXCEPT_FUNCTION_TYPE_VERS  "(C++17)"


// D_ENV_CPP_FEATURE_LANG_NONTYPE_TEMPLATE_PARAMETER_AUTO
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_nontype_template_parameter_auto
    #define D_ENV_CPP_FEATURE_LANG_NONTYPE_TEMPLATE_PARAMETER_AUTO  1
#else
    #define D_ENV_CPP_FEATURE_LANG_NONTYPE_TEMPLATE_PARAMETER_AUTO  0
#endif

// D_ENV_CPP_FEATURE_LANG_NONTYPE_TEMPLATE_PARAMETER_AUTO_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_LANG_NONTYPE_TEMPLATE_PARAMETER_AUTO_NAME  "__cpp_nontype_template_parameter_auto"

// D_ENV_CPP_FEATURE_LANG_NONTYPE_TEMPLATE_PARAMETER_AUTO_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_LANG_NONTYPE_TEMPLATE_PARAMETER_AUTO_DESC  "Declaring constant template parameter with auto"

// D_ENV_CPP_FEATURE_LANG_NONTYPE_TEMPLATE_PARAMETER_AUTO_VAL
//   constant: feature test value
#ifdef __cpp_nontype_template_parameter_auto
    #define D_ENV_CPP_FEATURE_LANG_NONTYPE_TEMPLATE_PARAMETER_AUTO_VAL  __cpp_nontype_template_parameter_auto
#else
    #define D_ENV_CPP_FEATURE_LANG_NONTYPE_TEMPLATE_PARAMETER_AUTO_VAL  0L
#endif

// D_ENV_CPP_FEATURE_LANG_NONTYPE_TEMPLATE_PARAMETER_AUTO_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_LANG_NONTYPE_TEMPLATE_PARAMETER_AUTO_VERS  "(C++17)"


// D_ENV_CPP_FEATURE_LANG_STRUCTURED_BINDINGS
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_structured_bindings
    #define D_ENV_CPP_FEATURE_LANG_STRUCTURED_BINDINGS  1
#else
    #define D_ENV_CPP_FEATURE_LANG_STRUCTURED_BINDINGS  0
#endif

// D_ENV_CPP_FEATURE_LANG_STRUCTURED_BINDINGS_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_LANG_STRUCTURED_BINDINGS_NAME  "__cpp_structured_bindings"

// D_ENV_CPP_FEATURE_LANG_STRUCTURED_BINDINGS_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_LANG_STRUCTURED_BINDINGS_DESC  "Structured bindings"

// D_ENV_CPP_FEATURE_LANG_STRUCTURED_BINDINGS_VAL
//   constant: feature test value
#ifdef __cpp_structured_bindings
    #define D_ENV_CPP_FEATURE_LANG_STRUCTURED_BINDINGS_VAL  __cpp_structured_bindings
#else
    #define D_ENV_CPP_FEATURE_LANG_STRUCTURED_BINDINGS_VAL  0L
#endif

// D_ENV_CPP_FEATURE_LANG_STRUCTURED_BINDINGS_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_LANG_STRUCTURED_BINDINGS_VERS  "(C++17)"


// D_ENV_CPP_FEATURE_LANG_TEMPLATE_TEMPLATE_ARGS
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_template_template_args
    #define D_ENV_CPP_FEATURE_LANG_TEMPLATE_TEMPLATE_ARGS  1
#else
    #define D_ENV_CPP_FEATURE_LANG_TEMPLATE_TEMPLATE_ARGS  0
#endif

// D_ENV_CPP_FEATURE_LANG_TEMPLATE_TEMPLATE_ARGS_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_LANG_TEMPLATE_TEMPLATE_ARGS_NAME  "__cpp_template_template_args"

// D_ENV_CPP_FEATURE_LANG_TEMPLATE_TEMPLATE_ARGS_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_LANG_TEMPLATE_TEMPLATE_ARGS_DESC  "Matching of template template arguments"

// D_ENV_CPP_FEATURE_LANG_TEMPLATE_TEMPLATE_ARGS_VAL
//   constant: feature test value
#ifdef __cpp_template_template_args
    #define D_ENV_CPP_FEATURE_LANG_TEMPLATE_TEMPLATE_ARGS_VAL  __cpp_template_template_args
#else
    #define D_ENV_CPP_FEATURE_LANG_TEMPLATE_TEMPLATE_ARGS_VAL  0L
#endif

// D_ENV_CPP_FEATURE_LANG_TEMPLATE_TEMPLATE_ARGS_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_LANG_TEMPLATE_TEMPLATE_ARGS_VERS  "(C++17)"


// D_ENV_CPP_FEATURE_LANG_VARIADIC_USING
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_variadic_using
    #define D_ENV_CPP_FEATURE_LANG_VARIADIC_USING  1
#else
    #define D_ENV_CPP_FEATURE_LANG_VARIADIC_USING  0
#endif

// D_ENV_CPP_FEATURE_LANG_VARIADIC_USING_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_LANG_VARIADIC_USING_NAME  "__cpp_variadic_using"

// D_ENV_CPP_FEATURE_LANG_VARIADIC_USING_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_LANG_VARIADIC_USING_DESC  "Pack expansions in using-declarations"

// D_ENV_CPP_FEATURE_LANG_VARIADIC_USING_VAL
//   constant: feature test value
#ifdef __cpp_variadic_using
    #define D_ENV_CPP_FEATURE_LANG_VARIADIC_USING_VAL  __cpp_variadic_using
#else
    #define D_ENV_CPP_FEATURE_LANG_VARIADIC_USING_VAL  0L
#endif

// D_ENV_CPP_FEATURE_LANG_VARIADIC_USING_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_LANG_VARIADIC_USING_VERS  "(C++17)"


// -----------------------------------------------------------------------------
// C++20 Language Features
// -----------------------------------------------------------------------------

// D_ENV_CPP_FEATURE_LANG_AGGREGATE_PAREN_INIT
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_aggregate_paren_init
    #define D_ENV_CPP_FEATURE_LANG_AGGREGATE_PAREN_INIT  1
#else
    #define D_ENV_CPP_FEATURE_LANG_AGGREGATE_PAREN_INIT  0
#endif

// D_ENV_CPP_FEATURE_LANG_AGGREGATE_PAREN_INIT_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_LANG_AGGREGATE_PAREN_INIT_NAME  "__cpp_aggregate_paren_init"

// D_ENV_CPP_FEATURE_LANG_AGGREGATE_PAREN_INIT_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_LANG_AGGREGATE_PAREN_INIT_DESC  "Aggregate initialization in the form of direct initialization"

// D_ENV_CPP_FEATURE_LANG_AGGREGATE_PAREN_INIT_VAL
//   constant: feature test value
#ifdef __cpp_aggregate_paren_init
    #define D_ENV_CPP_FEATURE_LANG_AGGREGATE_PAREN_INIT_VAL  __cpp_aggregate_paren_init
#else
    #define D_ENV_CPP_FEATURE_LANG_AGGREGATE_PAREN_INIT_VAL  0L
#endif

// D_ENV_CPP_FEATURE_LANG_AGGREGATE_PAREN_INIT_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_LANG_AGGREGATE_PAREN_INIT_VERS  "(C++20)"


// D_ENV_CPP_FEATURE_LANG_CHAR8_T
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_char8_t
    #define D_ENV_CPP_FEATURE_LANG_CHAR8_T  1
#else
    #define D_ENV_CPP_FEATURE_LANG_CHAR8_T  0
#endif

// D_ENV_CPP_FEATURE_LANG_CHAR8_T_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_LANG_CHAR8_T_NAME  "__cpp_char8_t"

// D_ENV_CPP_FEATURE_LANG_CHAR8_T_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_LANG_CHAR8_T_DESC  "char8_t"

// D_ENV_CPP_FEATURE_LANG_CHAR8_T_VAL
//   constant: feature test value
#ifdef __cpp_char8_t
    #define D_ENV_CPP_FEATURE_LANG_CHAR8_T_VAL  __cpp_char8_t
#else
    #define D_ENV_CPP_FEATURE_LANG_CHAR8_T_VAL  0L
#endif

// D_ENV_CPP_FEATURE_LANG_CHAR8_T_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_LANG_CHAR8_T_VERS  "(C++20)"


// D_ENV_CPP_FEATURE_LANG_CONCEPTS
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_concepts
    #define D_ENV_CPP_FEATURE_LANG_CONCEPTS  1
#else
    #define D_ENV_CPP_FEATURE_LANG_CONCEPTS  0
#endif

// D_ENV_CPP_FEATURE_LANG_CONCEPTS_TS
//   constant: Concepts TS support flag (1 = enabled, 0 = disabled)
#if defined(__cpp_concepts) && (__cpp_concepts == 201507L)
    #define D_ENV_CPP_FEATURE_LANG_CONCEPTS_TS  1
#else
    #define D_ENV_CPP_FEATURE_LANG_CONCEPTS_TS  0
#endif

// D_ENV_CPP_FEATURE_LANG_CONCEPTS_CPP20
//   constant: C++20 concepts support flag (1 = enabled, 0 = disabled)
#if defined(__cpp_concepts) && (__cpp_concepts >= 201907L)
    #define D_ENV_CPP_FEATURE_LANG_CONCEPTS_CPP20  1
#else
    #define D_ENV_CPP_FEATURE_LANG_CONCEPTS_CPP20  0
#endif

// D_ENV_CPP_FEATURE_LANG_CONCEPTS_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_LANG_CONCEPTS_NAME  "__cpp_concepts"

// D_ENV_CPP_FEATURE_LANG_CONCEPTS_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_LANG_CONCEPTS_DESC  "Concepts"

// D_ENV_CPP_FEATURE_LANG_CONCEPTS_VAL
//   constant: feature test value
#ifdef __cpp_concepts
    #define D_ENV_CPP_FEATURE_LANG_CONCEPTS_VAL  __cpp_concepts
#else
    #define D_ENV_CPP_FEATURE_LANG_CONCEPTS_VAL  0L
#endif

// D_ENV_CPP_FEATURE_LANG_CONCEPTS_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_LANG_CONCEPTS_VERS  "(C++20)"


// D_ENV_CPP_FEATURE_LANG_CONDITIONAL_EXPLICIT
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_conditional_explicit
    #define D_ENV_CPP_FEATURE_LANG_CONDITIONAL_EXPLICIT  1
#else
    #define D_ENV_CPP_FEATURE_LANG_CONDITIONAL_EXPLICIT  0
#endif

// D_ENV_CPP_FEATURE_LANG_CONDITIONAL_EXPLICIT_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_LANG_CONDITIONAL_EXPLICIT_NAME  "__cpp_conditional_explicit"

// D_ENV_CPP_FEATURE_LANG_CONDITIONAL_EXPLICIT_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_LANG_CONDITIONAL_EXPLICIT_DESC  "explicit(bool)"

// D_ENV_CPP_FEATURE_LANG_CONDITIONAL_EXPLICIT_VAL
//   constant: feature test value
#ifdef __cpp_conditional_explicit
    #define D_ENV_CPP_FEATURE_LANG_CONDITIONAL_EXPLICIT_VAL  __cpp_conditional_explicit
#else
    #define D_ENV_CPP_FEATURE_LANG_CONDITIONAL_EXPLICIT_VAL  0L
#endif

// D_ENV_CPP_FEATURE_LANG_CONDITIONAL_EXPLICIT_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_LANG_CONDITIONAL_EXPLICIT_VERS  "(C++20)"


// D_ENV_CPP_FEATURE_LANG_CONSTEVAL
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_consteval
    #define D_ENV_CPP_FEATURE_LANG_CONSTEVAL  1
#else
    #define D_ENV_CPP_FEATURE_LANG_CONSTEVAL  0
#endif

// D_ENV_CPP_FEATURE_LANG_CONSTEVAL_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_LANG_CONSTEVAL_NAME  "__cpp_consteval"

// D_ENV_CPP_FEATURE_LANG_CONSTEVAL_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_LANG_CONSTEVAL_DESC  "Immediate functions"

// D_ENV_CPP_FEATURE_LANG_CONSTEVAL_VAL
//   constant: feature test value
#ifdef __cpp_consteval
    #define D_ENV_CPP_FEATURE_LANG_CONSTEVAL_VAL  __cpp_consteval
#else
    #define D_ENV_CPP_FEATURE_LANG_CONSTEVAL_VAL  0L
#endif

// D_ENV_CPP_FEATURE_LANG_CONSTEVAL_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_LANG_CONSTEVAL_VERS  "(C++20)"


// D_ENV_CPP_FEATURE_LANG_CONSTEXPR_DYNAMIC_ALLOC
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_constexpr_dynamic_alloc
    #define D_ENV_CPP_FEATURE_LANG_CONSTEXPR_DYNAMIC_ALLOC  1
#else
    #define D_ENV_CPP_FEATURE_LANG_CONSTEXPR_DYNAMIC_ALLOC  0
#endif

// D_ENV_CPP_FEATURE_LANG_CONSTEXPR_DYNAMIC_ALLOC_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_LANG_CONSTEXPR_DYNAMIC_ALLOC_NAME  "__cpp_constexpr_dynamic_alloc"

// D_ENV_CPP_FEATURE_LANG_CONSTEXPR_DYNAMIC_ALLOC_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_LANG_CONSTEXPR_DYNAMIC_ALLOC_DESC  "Operations for dynamic storage duration in constexpr functions"

// D_ENV_CPP_FEATURE_LANG_CONSTEXPR_DYNAMIC_ALLOC_VAL
//   constant: feature test value
#ifdef __cpp_constexpr_dynamic_alloc
    #define D_ENV_CPP_FEATURE_LANG_CONSTEXPR_DYNAMIC_ALLOC_VAL  __cpp_constexpr_dynamic_alloc
#else
    #define D_ENV_CPP_FEATURE_LANG_CONSTEXPR_DYNAMIC_ALLOC_VAL  0L
#endif

// D_ENV_CPP_FEATURE_LANG_CONSTEXPR_DYNAMIC_ALLOC_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_LANG_CONSTEXPR_DYNAMIC_ALLOC_VERS  "(C++20)"


// D_ENV_CPP_FEATURE_LANG_CONSTINIT
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_constinit
    #define D_ENV_CPP_FEATURE_LANG_CONSTINIT  1
#else
    #define D_ENV_CPP_FEATURE_LANG_CONSTINIT  0
#endif

// D_ENV_CPP_FEATURE_LANG_CONSTINIT_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_LANG_CONSTINIT_NAME  "__cpp_constinit"

// D_ENV_CPP_FEATURE_LANG_CONSTINIT_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_LANG_CONSTINIT_DESC  "constinit"

// D_ENV_CPP_FEATURE_LANG_CONSTINIT_VAL
//   constant: feature test value
#ifdef __cpp_constinit
    #define D_ENV_CPP_FEATURE_LANG_CONSTINIT_VAL  __cpp_constinit
#else
    #define D_ENV_CPP_FEATURE_LANG_CONSTINIT_VAL  0L
#endif

// D_ENV_CPP_FEATURE_LANG_CONSTINIT_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_LANG_CONSTINIT_VERS  "(C++20)"


// D_ENV_CPP_FEATURE_LANG_DESIGNATED_INITIALIZERS
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_designated_initializers
    #define D_ENV_CPP_FEATURE_LANG_DESIGNATED_INITIALIZERS  1
#else
    #define D_ENV_CPP_FEATURE_LANG_DESIGNATED_INITIALIZERS  0
#endif

// D_ENV_CPP_FEATURE_LANG_DESIGNATED_INITIALIZERS_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_LANG_DESIGNATED_INITIALIZERS_NAME  "__cpp_designated_initializers"

// D_ENV_CPP_FEATURE_LANG_DESIGNATED_INITIALIZERS_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_LANG_DESIGNATED_INITIALIZERS_DESC  "Designated initializers"

// D_ENV_CPP_FEATURE_LANG_DESIGNATED_INITIALIZERS_VAL
//   constant: feature test value
#ifdef __cpp_designated_initializers
    #define D_ENV_CPP_FEATURE_LANG_DESIGNATED_INITIALIZERS_VAL  __cpp_designated_initializers
#else
    #define D_ENV_CPP_FEATURE_LANG_DESIGNATED_INITIALIZERS_VAL  0L
#endif

// D_ENV_CPP_FEATURE_LANG_DESIGNATED_INITIALIZERS_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_LANG_DESIGNATED_INITIALIZERS_VERS  "(C++20)"


// D_ENV_CPP_FEATURE_LANG_IMPL_COROUTINE
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_IMPL_coroutine
    #define D_ENV_CPP_FEATURE_LANG_IMPL_COROUTINE  1
#else
    #define D_ENV_CPP_FEATURE_LANG_IMPL_COROUTINE  0
#endif

// D_ENV_CPP_FEATURE_LANG_IMPL_COROUTINE_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_LANG_IMPL_COROUTINE_NAME  "__cpp_IMPL_coroutine"

// D_ENV_CPP_FEATURE_LANG_IMPL_COROUTINE_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_LANG_IMPL_COROUTINE_DESC  "Coroutines (compiler support)"

// D_ENV_CPP_FEATURE_LANG_IMPL_COROUTINE_VAL
//   constant: feature test value
#ifdef __cpp_IMPL_coroutine
    #define D_ENV_CPP_FEATURE_LANG_IMPL_COROUTINE_VAL  __cpp_IMPL_coroutine
#else
    #define D_ENV_CPP_FEATURE_LANG_IMPL_COROUTINE_VAL  0L
#endif

// D_ENV_CPP_FEATURE_LANG_IMPL_COROUTINE_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_LANG_IMPL_COROUTINE_VERS  "(C++20)"


// D_ENV_CPP_FEATURE_LANG_IMPL_DESTROYING_DELETE
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_IMPL_destroying_delete
    #define D_ENV_CPP_FEATURE_LANG_IMPL_DESTROYING_DELETE  1
#else
    #define D_ENV_CPP_FEATURE_LANG_IMPL_DESTROYING_DELETE  0
#endif

// D_ENV_CPP_FEATURE_LANG_IMPL_DESTROYING_DELETE_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_LANG_IMPL_DESTROYING_DELETE_NAME  "__cpp_IMPL_destroying_delete"

// D_ENV_CPP_FEATURE_LANG_IMPL_DESTROYING_DELETE_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_LANG_IMPL_DESTROYING_DELETE_DESC  "Destroying operator delete (compiler support)"

// D_ENV_CPP_FEATURE_LANG_IMPL_DESTROYING_DELETE_VAL
//   constant: feature test value
#ifdef __cpp_IMPL_destroying_delete
    #define D_ENV_CPP_FEATURE_LANG_IMPL_DESTROYING_DELETE_VAL  __cpp_IMPL_destroying_delete
#else
    #define D_ENV_CPP_FEATURE_LANG_IMPL_DESTROYING_DELETE_VAL  0L
#endif

// D_ENV_CPP_FEATURE_LANG_IMPL_DESTROYING_DELETE_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_LANG_IMPL_DESTROYING_DELETE_VERS  "(C++20)"


// D_ENV_CPP_FEATURE_LANG_IMPL_THREE_WAY_COMPARISON
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_IMPL_three_way_comparison
    #define D_ENV_CPP_FEATURE_LANG_IMPL_THREE_WAY_COMPARISON  1
#else
    #define D_ENV_CPP_FEATURE_LANG_IMPL_THREE_WAY_COMPARISON  0
#endif

// D_ENV_CPP_FEATURE_LANG_IMPL_THREE_WAY_COMPARISON_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_LANG_IMPL_THREE_WAY_COMPARISON_NAME  "__cpp_IMPL_three_way_comparison"

// D_ENV_CPP_FEATURE_LANG_IMPL_THREE_WAY_COMPARISON_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_LANG_IMPL_THREE_WAY_COMPARISON_DESC  "Three-way comparison (compiler support)"

// D_ENV_CPP_FEATURE_LANG_IMPL_THREE_WAY_COMPARISON_VAL
//   constant: feature test value
#ifdef __cpp_IMPL_three_way_comparison
    #define D_ENV_CPP_FEATURE_LANG_IMPL_THREE_WAY_COMPARISON_VAL  __cpp_IMPL_three_way_comparison
#else
    #define D_ENV_CPP_FEATURE_LANG_IMPL_THREE_WAY_COMPARISON_VAL  0L
#endif

// D_ENV_CPP_FEATURE_LANG_IMPL_THREE_WAY_COMPARISON_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_LANG_IMPL_THREE_WAY_COMPARISON_VERS  "(C++20)"


// D_ENV_CPP_FEATURE_LANG_MODULES
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_modules
    #define D_ENV_CPP_FEATURE_LANG_MODULES  1
#else
    #define D_ENV_CPP_FEATURE_LANG_MODULES  0
#endif

// D_ENV_CPP_FEATURE_LANG_MODULES_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_LANG_MODULES_NAME  "__cpp_modules"

// D_ENV_CPP_FEATURE_LANG_MODULES_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_LANG_MODULES_DESC  "Modules"

// D_ENV_CPP_FEATURE_LANG_MODULES_VAL
//   constant: feature test value
#ifdef __cpp_modules
    #define D_ENV_CPP_FEATURE_LANG_MODULES_VAL  __cpp_modules
#else
    #define D_ENV_CPP_FEATURE_LANG_MODULES_VAL  0L
#endif

// D_ENV_CPP_FEATURE_LANG_MODULES_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_LANG_MODULES_VERS  "(C++20)"


// D_ENV_CPP_FEATURE_LANG_USING_ENUM
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_using_enum
    #define D_ENV_CPP_FEATURE_LANG_USING_ENUM  1
#else
    #define D_ENV_CPP_FEATURE_LANG_USING_ENUM  0
#endif

// D_ENV_CPP_FEATURE_LANG_USING_ENUM_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_LANG_USING_ENUM_NAME  "__cpp_using_enum"

// D_ENV_CPP_FEATURE_LANG_USING_ENUM_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_LANG_USING_ENUM_DESC  "using enum"

// D_ENV_CPP_FEATURE_LANG_USING_ENUM_VAL
//   constant: feature test value
#ifdef __cpp_using_enum
    #define D_ENV_CPP_FEATURE_LANG_USING_ENUM_VAL  __cpp_using_enum
#else
    #define D_ENV_CPP_FEATURE_LANG_USING_ENUM_VAL  0L
#endif

// D_ENV_CPP_FEATURE_LANG_USING_ENUM_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_LANG_USING_ENUM_VERS  "(C++20)"


// -----------------------------------------------------------------------------
// C++23 Language Features
// -----------------------------------------------------------------------------

// D_ENV_CPP_FEATURE_LANG_AUTO_CAST
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_auto_cast
    #define D_ENV_CPP_FEATURE_LANG_AUTO_CAST  1
#else
    #define D_ENV_CPP_FEATURE_LANG_AUTO_CAST  0
#endif

// D_ENV_CPP_FEATURE_LANG_AUTO_CAST_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_LANG_AUTO_CAST_NAME  "__cpp_auto_cast"

// D_ENV_CPP_FEATURE_LANG_AUTO_CAST_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_LANG_AUTO_CAST_DESC  "auto(x) and auto{x}"

// D_ENV_CPP_FEATURE_LANG_AUTO_CAST_VAL
//   constant: feature test value
#ifdef __cpp_auto_cast
    #define D_ENV_CPP_FEATURE_LANG_AUTO_CAST_VAL  __cpp_auto_cast
#else
    #define D_ENV_CPP_FEATURE_LANG_AUTO_CAST_VAL  0L
#endif

// D_ENV_CPP_FEATURE_LANG_AUTO_CAST_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_LANG_AUTO_CAST_VERS  "(C++23)"


// D_ENV_CPP_FEATURE_LANG_EXPLICIT_THIS_PARAMETER
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_explicit_this_parameter
    #define D_ENV_CPP_FEATURE_LANG_EXPLICIT_THIS_PARAMETER  1
#else
    #define D_ENV_CPP_FEATURE_LANG_EXPLICIT_THIS_PARAMETER  0
#endif

// D_ENV_CPP_FEATURE_LANG_EXPLICIT_THIS_PARAMETER_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_LANG_EXPLICIT_THIS_PARAMETER_NAME  "__cpp_explicit_this_parameter"

// D_ENV_CPP_FEATURE_LANG_EXPLICIT_THIS_PARAMETER_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_LANG_EXPLICIT_THIS_PARAMETER_DESC  "Explicit object parameter"

// D_ENV_CPP_FEATURE_LANG_EXPLICIT_THIS_PARAMETER_VAL
//   constant: feature test value
#ifdef __cpp_explicit_this_parameter
    #define D_ENV_CPP_FEATURE_LANG_EXPLICIT_THIS_PARAMETER_VAL  __cpp_explicit_this_parameter
#else
    #define D_ENV_CPP_FEATURE_LANG_EXPLICIT_THIS_PARAMETER_VAL  0L
#endif

// D_ENV_CPP_FEATURE_LANG_EXPLICIT_THIS_PARAMETER_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_LANG_EXPLICIT_THIS_PARAMETER_VERS  "(C++23)"


// D_ENV_CPP_FEATURE_LANG_IF_CONSTEVAL
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_if_consteval
    #define D_ENV_CPP_FEATURE_LANG_IF_CONSTEVAL  1
#else
    #define D_ENV_CPP_FEATURE_LANG_IF_CONSTEVAL  0
#endif

// D_ENV_CPP_FEATURE_LANG_IF_CONSTEVAL_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_LANG_IF_CONSTEVAL_NAME  "__cpp_if_consteval"

// D_ENV_CPP_FEATURE_LANG_IF_CONSTEVAL_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_LANG_IF_CONSTEVAL_DESC  "if consteval"

// D_ENV_CPP_FEATURE_LANG_IF_CONSTEVAL_VAL
//   constant: feature test value
#ifdef __cpp_if_consteval
    #define D_ENV_CPP_FEATURE_LANG_IF_CONSTEVAL_VAL  __cpp_if_consteval
#else
    #define D_ENV_CPP_FEATURE_LANG_IF_CONSTEVAL_VAL  0L
#endif

// D_ENV_CPP_FEATURE_LANG_IF_CONSTEVAL_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_LANG_IF_CONSTEVAL_VERS  "(C++23)"


// D_ENV_CPP_FEATURE_LANG_IMPLICIT_MOVE
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_implicit_move
    #define D_ENV_CPP_FEATURE_LANG_IMPLICIT_MOVE  1
#else
    #define D_ENV_CPP_FEATURE_LANG_IMPLICIT_MOVE  0
#endif

// D_ENV_CPP_FEATURE_LANG_IMPLICIT_MOVE_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_LANG_IMPLICIT_MOVE_NAME  "__cpp_implicit_move"

// D_ENV_CPP_FEATURE_LANG_IMPLICIT_MOVE_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_LANG_IMPLICIT_MOVE_DESC  "Simpler implicit move"

// D_ENV_CPP_FEATURE_LANG_IMPLICIT_MOVE_VAL
//   constant: feature test value
#ifdef __cpp_implicit_move
    #define D_ENV_CPP_FEATURE_LANG_IMPLICIT_MOVE_VAL  __cpp_implicit_move
#else
    #define D_ENV_CPP_FEATURE_LANG_IMPLICIT_MOVE_VAL  0L
#endif

// D_ENV_CPP_FEATURE_LANG_IMPLICIT_MOVE_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_LANG_IMPLICIT_MOVE_VERS  "(C++23)"


// D_ENV_CPP_FEATURE_LANG_MULTIDIMENSIONAL_SUBSCRIPT
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_multidimensional_subscript
    #define D_ENV_CPP_FEATURE_LANG_MULTIDIMENSIONAL_SUBSCRIPT  1
#else
    #define D_ENV_CPP_FEATURE_LANG_MULTIDIMENSIONAL_SUBSCRIPT  0
#endif

// D_ENV_CPP_FEATURE_LANG_MULTIDIMENSIONAL_SUBSCRIPT_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_LANG_MULTIDIMENSIONAL_SUBSCRIPT_NAME  "__cpp_multidimensional_subscript"

// D_ENV_CPP_FEATURE_LANG_MULTIDIMENSIONAL_SUBSCRIPT_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_LANG_MULTIDIMENSIONAL_SUBSCRIPT_DESC  "Multidimensional subscript operator"

// D_ENV_CPP_FEATURE_LANG_MULTIDIMENSIONAL_SUBSCRIPT_VAL
//   constant: feature test value
#ifdef __cpp_multidimensional_subscript
    #define D_ENV_CPP_FEATURE_LANG_MULTIDIMENSIONAL_SUBSCRIPT_VAL  __cpp_multidimensional_subscript
#else
    #define D_ENV_CPP_FEATURE_LANG_MULTIDIMENSIONAL_SUBSCRIPT_VAL  0L
#endif

// D_ENV_CPP_FEATURE_LANG_MULTIDIMENSIONAL_SUBSCRIPT_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_LANG_MULTIDIMENSIONAL_SUBSCRIPT_VERS  "(C++23)"


// D_ENV_CPP_FEATURE_LANG_NAMED_CHARACTER_ESCAPES
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_named_character_escapes
    #define D_ENV_CPP_FEATURE_LANG_NAMED_CHARACTER_ESCAPES  1
#else
    #define D_ENV_CPP_FEATURE_LANG_NAMED_CHARACTER_ESCAPES  0
#endif

// D_ENV_CPP_FEATURE_LANG_NAMED_CHARACTER_ESCAPES_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_LANG_NAMED_CHARACTER_ESCAPES_NAME  "__cpp_named_character_escapes"

// D_ENV_CPP_FEATURE_LANG_NAMED_CHARACTER_ESCAPES_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_LANG_NAMED_CHARACTER_ESCAPES_DESC  "Named universal character escapes"

// D_ENV_CPP_FEATURE_LANG_NAMED_CHARACTER_ESCAPES_VAL
//   constant: feature test value
#ifdef __cpp_named_character_escapes
    #define D_ENV_CPP_FEATURE_LANG_NAMED_CHARACTER_ESCAPES_VAL  __cpp_named_character_escapes
#else
    #define D_ENV_CPP_FEATURE_LANG_NAMED_CHARACTER_ESCAPES_VAL  0L
#endif

// D_ENV_CPP_FEATURE_LANG_NAMED_CHARACTER_ESCAPES_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_LANG_NAMED_CHARACTER_ESCAPES_VERS  "(C++23)"


// D_ENV_CPP_FEATURE_LANG_SIZE_T_SUFFIX
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_size_t_suffix
    #define D_ENV_CPP_FEATURE_LANG_SIZE_T_SUFFIX  1
#else
    #define D_ENV_CPP_FEATURE_LANG_SIZE_T_SUFFIX  0
#endif

// D_ENV_CPP_FEATURE_LANG_SIZE_T_SUFFIX_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_LANG_SIZE_T_SUFFIX_NAME  "__cpp_size_t_suffix"

// D_ENV_CPP_FEATURE_LANG_SIZE_T_SUFFIX_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_LANG_SIZE_T_SUFFIX_DESC  "Literal suffixes for std::size_t and its signed version"

// D_ENV_CPP_FEATURE_LANG_SIZE_T_SUFFIX_VAL
//   constant: feature test value
#ifdef __cpp_size_t_suffix
    #define D_ENV_CPP_FEATURE_LANG_SIZE_T_SUFFIX_VAL  __cpp_size_t_suffix
#else
    #define D_ENV_CPP_FEATURE_LANG_SIZE_T_SUFFIX_VAL  0L
#endif

// D_ENV_CPP_FEATURE_LANG_SIZE_T_SUFFIX_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_LANG_SIZE_T_SUFFIX_VERS  "(C++23)"


// D_ENV_CPP_FEATURE_LANG_STATIC_CALL_OPERATOR
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_static_call_operator
    #define D_ENV_CPP_FEATURE_LANG_STATIC_CALL_OPERATOR  1
#else
    #define D_ENV_CPP_FEATURE_LANG_STATIC_CALL_OPERATOR  0
#endif

// D_ENV_CPP_FEATURE_LANG_STATIC_CALL_OPERATOR_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_LANG_STATIC_CALL_OPERATOR_NAME  "__cpp_static_call_operator"

// D_ENV_CPP_FEATURE_LANG_STATIC_CALL_OPERATOR_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_LANG_STATIC_CALL_OPERATOR_DESC  "Static operator()"

// D_ENV_CPP_FEATURE_LANG_STATIC_CALL_OPERATOR_VAL
//   constant: feature test value
#ifdef __cpp_static_call_operator
    #define D_ENV_CPP_FEATURE_LANG_STATIC_CALL_OPERATOR_VAL  __cpp_static_call_operator
#else
    #define D_ENV_CPP_FEATURE_LANG_STATIC_CALL_OPERATOR_VAL  0L
#endif

// D_ENV_CPP_FEATURE_LANG_STATIC_CALL_OPERATOR_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_LANG_STATIC_CALL_OPERATOR_VERS  "(C++23)"


// -----------------------------------------------------------------------------
// C++26 Language Features
// -----------------------------------------------------------------------------

// D_ENV_CPP_FEATURE_LANG_CONSTEXPR_EXCEPTIONS
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_constexpr_exceptions
    #define D_ENV_CPP_FEATURE_LANG_CONSTEXPR_EXCEPTIONS  1
#else
    #define D_ENV_CPP_FEATURE_LANG_CONSTEXPR_EXCEPTIONS  0
#endif

// D_ENV_CPP_FEATURE_LANG_CONSTEXPR_EXCEPTIONS_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_LANG_CONSTEXPR_EXCEPTIONS_NAME  "__cpp_constexpr_exceptions"

// D_ENV_CPP_FEATURE_LANG_CONSTEXPR_EXCEPTIONS_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_LANG_CONSTEXPR_EXCEPTIONS_DESC  "constexpr exceptions"

// D_ENV_CPP_FEATURE_LANG_CONSTEXPR_EXCEPTIONS_VAL
//   constant: feature test value
#ifdef __cpp_constexpr_exceptions
    #define D_ENV_CPP_FEATURE_LANG_CONSTEXPR_EXCEPTIONS_VAL  __cpp_constexpr_exceptions
#else
    #define D_ENV_CPP_FEATURE_LANG_CONSTEXPR_EXCEPTIONS_VAL  0L
#endif

// D_ENV_CPP_FEATURE_LANG_CONSTEXPR_EXCEPTIONS_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_LANG_CONSTEXPR_EXCEPTIONS_VERS  "(C++26)"


// D_ENV_CPP_FEATURE_LANG_CONTRACTS
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_contracts
    #define D_ENV_CPP_FEATURE_LANG_CONTRACTS  1
#else
    #define D_ENV_CPP_FEATURE_LANG_CONTRACTS  0
#endif

// D_ENV_CPP_FEATURE_LANG_CONTRACTS_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_LANG_CONTRACTS_NAME  "__cpp_contracts"

// D_ENV_CPP_FEATURE_LANG_CONTRACTS_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_LANG_CONTRACTS_DESC  "Contracts"

// D_ENV_CPP_FEATURE_LANG_CONTRACTS_VAL
//   constant: feature test value
#ifdef __cpp_contracts
    #define D_ENV_CPP_FEATURE_LANG_CONTRACTS_VAL  __cpp_contracts
#else
    #define D_ENV_CPP_FEATURE_LANG_CONTRACTS_VAL  0L
#endif

// D_ENV_CPP_FEATURE_LANG_CONTRACTS_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_LANG_CONTRACTS_VERS  "(C++26)"


// D_ENV_CPP_FEATURE_LANG_DELETED_FUNCTION
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_deleted_function
    #define D_ENV_CPP_FEATURE_LANG_DELETED_FUNCTION  1
#else
    #define D_ENV_CPP_FEATURE_LANG_DELETED_FUNCTION  0
#endif

// D_ENV_CPP_FEATURE_LANG_DELETED_FUNCTION_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_LANG_DELETED_FUNCTION_NAME  "__cpp_deleted_function"

// D_ENV_CPP_FEATURE_LANG_DELETED_FUNCTION_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_LANG_DELETED_FUNCTION_DESC  "Deleted function definitions with messages"

// D_ENV_CPP_FEATURE_LANG_DELETED_FUNCTION_VAL
//   constant: feature test value
#ifdef __cpp_deleted_function
    #define D_ENV_CPP_FEATURE_LANG_DELETED_FUNCTION_VAL  __cpp_deleted_function
#else
    #define D_ENV_CPP_FEATURE_LANG_DELETED_FUNCTION_VAL  0L
#endif

// D_ENV_CPP_FEATURE_LANG_DELETED_FUNCTION_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_LANG_DELETED_FUNCTION_VERS  "(C++26)"


// D_ENV_CPP_FEATURE_LANG_PACK_INDEXING
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_pack_indexing
    #define D_ENV_CPP_FEATURE_LANG_PACK_INDEXING  1
#else
    #define D_ENV_CPP_FEATURE_LANG_PACK_INDEXING  0
#endif

// D_ENV_CPP_FEATURE_LANG_PACK_INDEXING_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_LANG_PACK_INDEXING_NAME  "__cpp_pack_indexing"

// D_ENV_CPP_FEATURE_LANG_PACK_INDEXING_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_LANG_PACK_INDEXING_DESC  "Pack indexing"

// D_ENV_CPP_FEATURE_LANG_PACK_INDEXING_VAL
//   constant: feature test value
#ifdef __cpp_pack_indexing
    #define D_ENV_CPP_FEATURE_LANG_PACK_INDEXING_VAL  __cpp_pack_indexing
#else
    #define D_ENV_CPP_FEATURE_LANG_PACK_INDEXING_VAL  0L
#endif

// D_ENV_CPP_FEATURE_LANG_PACK_INDEXING_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_LANG_PACK_INDEXING_VERS  "(C++26)"


// D_ENV_CPP_FEATURE_LANG_PLACEHOLDER_VARIABLES
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_placeholder_variables
    #define D_ENV_CPP_FEATURE_LANG_PLACEHOLDER_VARIABLES  1
#else
    #define D_ENV_CPP_FEATURE_LANG_PLACEHOLDER_VARIABLES  0
#endif

// D_ENV_CPP_FEATURE_LANG_PLACEHOLDER_VARIABLES_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_LANG_PLACEHOLDER_VARIABLES_NAME  "__cpp_placeholder_variables"

// D_ENV_CPP_FEATURE_LANG_PLACEHOLDER_VARIABLES_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_LANG_PLACEHOLDER_VARIABLES_DESC  "A nice placeholder with no name"

// D_ENV_CPP_FEATURE_LANG_PLACEHOLDER_VARIABLES_VAL
//   constant: feature test value
#ifdef __cpp_placeholder_variables
    #define D_ENV_CPP_FEATURE_LANG_PLACEHOLDER_VARIABLES_VAL  __cpp_placeholder_variables
#else
    #define D_ENV_CPP_FEATURE_LANG_PLACEHOLDER_VARIABLES_VAL  0L
#endif

// D_ENV_CPP_FEATURE_LANG_PLACEHOLDER_VARIABLES_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_LANG_PLACEHOLDER_VARIABLES_VERS  "(C++26)"


// D_ENV_CPP_FEATURE_LANG_PP_EMBED
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_pp_embed
    #define D_ENV_CPP_FEATURE_LANG_PP_EMBED  1
#else
    #define D_ENV_CPP_FEATURE_LANG_PP_EMBED  0
#endif

// D_ENV_CPP_FEATURE_LANG_PP_EMBED_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_LANG_PP_EMBED_NAME  "__cpp_pp_embed"

// D_ENV_CPP_FEATURE_LANG_PP_EMBED_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_LANG_PP_EMBED_DESC  "#embed"

// D_ENV_CPP_FEATURE_LANG_PP_EMBED_VAL
//   constant: feature test value
#ifdef __cpp_pp_embed
    #define D_ENV_CPP_FEATURE_LANG_PP_EMBED_VAL  __cpp_pp_embed
#else
    #define D_ENV_CPP_FEATURE_LANG_PP_EMBED_VAL  0L
#endif

// D_ENV_CPP_FEATURE_LANG_PP_EMBED_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_LANG_PP_EMBED_VERS  "(C++26)"


// D_ENV_CPP_FEATURE_LANG_TEMPLATE_PARAMETERS
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_template_parameters
    #define D_ENV_CPP_FEATURE_LANG_TEMPLATE_PARAMETERS  1
#else
    #define D_ENV_CPP_FEATURE_LANG_TEMPLATE_PARAMETERS  0
#endif

// D_ENV_CPP_FEATURE_LANG_TEMPLATE_PARAMETERS_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_LANG_TEMPLATE_PARAMETERS_NAME  "__cpp_template_parameters"

// D_ENV_CPP_FEATURE_LANG_TEMPLATE_PARAMETERS_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_LANG_TEMPLATE_PARAMETERS_DESC  "Concept and variable-template template-parameters"

// D_ENV_CPP_FEATURE_LANG_TEMPLATE_PARAMETERS_VAL
//   constant: feature test value
#ifdef __cpp_template_parameters
    #define D_ENV_CPP_FEATURE_LANG_TEMPLATE_PARAMETERS_VAL  __cpp_template_parameters
#else
    #define D_ENV_CPP_FEATURE_LANG_TEMPLATE_PARAMETERS_VAL  0L
#endif

// D_ENV_CPP_FEATURE_LANG_TEMPLATE_PARAMETERS_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_LANG_TEMPLATE_PARAMETERS_VERS  "(C++26)"


// D_ENV_CPP_FEATURE_LANG_TRIVIAL_RELOCATABILITY
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_trivial_relocatability
    #define D_ENV_CPP_FEATURE_LANG_TRIVIAL_RELOCATABILITY  1
#else
    #define D_ENV_CPP_FEATURE_LANG_TRIVIAL_RELOCATABILITY  0
#endif

// D_ENV_CPP_FEATURE_LANG_TRIVIAL_RELOCATABILITY_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_LANG_TRIVIAL_RELOCATABILITY_NAME  "__cpp_trivial_relocatability"

// D_ENV_CPP_FEATURE_LANG_TRIVIAL_RELOCATABILITY_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_LANG_TRIVIAL_RELOCATABILITY_DESC  "Trivial relocatability"

// D_ENV_CPP_FEATURE_LANG_TRIVIAL_RELOCATABILITY_VAL
//   constant: feature test value
#ifdef __cpp_trivial_relocatability
    #define D_ENV_CPP_FEATURE_LANG_TRIVIAL_RELOCATABILITY_VAL  __cpp_trivial_relocatability
#else
    #define D_ENV_CPP_FEATURE_LANG_TRIVIAL_RELOCATABILITY_VAL  0L
#endif

// D_ENV_CPP_FEATURE_LANG_TRIVIAL_RELOCATABILITY_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_LANG_TRIVIAL_RELOCATABILITY_VERS  "(C++26)"


// D_ENV_CPP_FEATURE_LANG_TRIVIAL_UNION
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_trivial_union
    #define D_ENV_CPP_FEATURE_LANG_TRIVIAL_UNION  1
#else
    #define D_ENV_CPP_FEATURE_LANG_TRIVIAL_UNION  0
#endif

// D_ENV_CPP_FEATURE_LANG_TRIVIAL_UNION_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_LANG_TRIVIAL_UNION_NAME  "__cpp_trivial_union"

// D_ENV_CPP_FEATURE_LANG_TRIVIAL_UNION_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_LANG_TRIVIAL_UNION_DESC  "Trivial unions"

// D_ENV_CPP_FEATURE_LANG_TRIVIAL_UNION_VAL
//   constant: feature test value
#ifdef __cpp_trivial_union
    #define D_ENV_CPP_FEATURE_LANG_TRIVIAL_UNION_VAL  __cpp_trivial_union
#else
    #define D_ENV_CPP_FEATURE_LANG_TRIVIAL_UNION_VAL  0L
#endif

// D_ENV_CPP_FEATURE_LANG_TRIVIAL_UNION_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_LANG_TRIVIAL_UNION_VERS  "(C++26)"


// D_ENV_CPP_FEATURE_LANG_VARIADIC_FRIEND
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_variadic_friend
    #define D_ENV_CPP_FEATURE_LANG_VARIADIC_FRIEND  1
#else
    #define D_ENV_CPP_FEATURE_LANG_VARIADIC_FRIEND  0
#endif

// D_ENV_CPP_FEATURE_LANG_VARIADIC_FRIEND_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_LANG_VARIADIC_FRIEND_NAME  "__cpp_variadic_friend"

// D_ENV_CPP_FEATURE_LANG_VARIADIC_FRIEND_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_LANG_VARIADIC_FRIEND_DESC  "Variadic friend declarations"

// D_ENV_CPP_FEATURE_LANG_VARIADIC_FRIEND_VAL
//   constant: feature test value
#ifdef __cpp_variadic_friend
    #define D_ENV_CPP_FEATURE_LANG_VARIADIC_FRIEND_VAL  __cpp_variadic_friend
#else
    #define D_ENV_CPP_FEATURE_LANG_VARIADIC_FRIEND_VAL  0L
#endif

// D_ENV_CPP_FEATURE_LANG_VARIADIC_FRIEND_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_LANG_VARIADIC_FRIEND_VERS  "(C++26)"


// =============================================================================
// II.  STANDARD LIBRARY FEATURES
// =============================================================================

// -----------------------------------------------------------------------------
// C++14 Library Features
// -----------------------------------------------------------------------------

// D_ENV_CPP_FEATURE_STL_CHRONO_UDLS
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_lib_chrono_udls
    #define D_ENV_CPP_FEATURE_STL_CHRONO_UDLS  1
#else
    #define D_ENV_CPP_FEATURE_STL_CHRONO_UDLS  0
#endif

// D_ENV_CPP_FEATURE_STL_CHRONO_UDLS_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_STL_CHRONO_UDLS_NAME  "__cpp_lib_chrono_udls"

// D_ENV_CPP_FEATURE_STL_CHRONO_UDLS_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_STL_CHRONO_UDLS_DESC  "User-defined literals for time types"

// D_ENV_CPP_FEATURE_STL_CHRONO_UDLS_VAL
//   constant: feature test value
#ifdef __cpp_lib_chrono_udls
    #define D_ENV_CPP_FEATURE_STL_CHRONO_UDLS_VAL  __cpp_lib_chrono_udls
#else
    #define D_ENV_CPP_FEATURE_STL_CHRONO_UDLS_VAL  0L
#endif

// D_ENV_CPP_FEATURE_STL_CHRONO_UDLS_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_STL_CHRONO_UDLS_VERS  "(C++14)"


// D_ENV_CPP_FEATURE_STL_COMPLEX_UDLS
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_lib_complex_udls
    #define D_ENV_CPP_FEATURE_STL_COMPLEX_UDLS  1
#else
    #define D_ENV_CPP_FEATURE_STL_COMPLEX_UDLS  0
#endif

// D_ENV_CPP_FEATURE_STL_COMPLEX_UDLS_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_STL_COMPLEX_UDLS_NAME  "__cpp_lib_complex_udls"

// D_ENV_CPP_FEATURE_STL_COMPLEX_UDLS_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_STL_COMPLEX_UDLS_DESC  "User-defined Literals for std::complex"

// D_ENV_CPP_FEATURE_STL_COMPLEX_UDLS_VAL
//   constant: feature test value
#ifdef __cpp_lib_complex_udls
    #define D_ENV_CPP_FEATURE_STL_COMPLEX_UDLS_VAL  __cpp_lib_complex_udls
#else
    #define D_ENV_CPP_FEATURE_STL_COMPLEX_UDLS_VAL  0L
#endif

// D_ENV_CPP_FEATURE_STL_COMPLEX_UDLS_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_STL_COMPLEX_UDLS_VERS  "(C++14)"


// -----------------------------------------------------------------------------
// C++17 Library Features
// -----------------------------------------------------------------------------

// D_ENV_CPP_FEATURE_STL_ADDRESSOF_CONSTEXPR
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_lib_addressof_constexpr
    #define D_ENV_CPP_FEATURE_STL_ADDRESSOF_CONSTEXPR  1
#else
    #define D_ENV_CPP_FEATURE_STL_ADDRESSOF_CONSTEXPR  0
#endif

// D_ENV_CPP_FEATURE_STL_ADDRESSOF_CONSTEXPR_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_STL_ADDRESSOF_CONSTEXPR_NAME  "__cpp_lib_addressof_constexpr"

// D_ENV_CPP_FEATURE_STL_ADDRESSOF_CONSTEXPR_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_STL_ADDRESSOF_CONSTEXPR_DESC  "Constexpr std::addressof"

// D_ENV_CPP_FEATURE_STL_ADDRESSOF_CONSTEXPR_VAL
//   constant: feature test value
#ifdef __cpp_lib_addressof_constexpr
    #define D_ENV_CPP_FEATURE_STL_ADDRESSOF_CONSTEXPR_VAL  __cpp_lib_addressof_constexpr
#else
    #define D_ENV_CPP_FEATURE_STL_ADDRESSOF_CONSTEXPR_VAL  0L
#endif

// D_ENV_CPP_FEATURE_STL_ADDRESSOF_CONSTEXPR_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_STL_ADDRESSOF_CONSTEXPR_VERS  "(C++17)"


// D_ENV_CPP_FEATURE_STL_ANY
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_lib_any
    #define D_ENV_CPP_FEATURE_STL_ANY  1
#else
    #define D_ENV_CPP_FEATURE_STL_ANY  0
#endif

// D_ENV_CPP_FEATURE_STL_ANY_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_STL_ANY_NAME  "__cpp_lib_any"

// D_ENV_CPP_FEATURE_STL_ANY_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_STL_ANY_DESC  "std::any"

// D_ENV_CPP_FEATURE_STL_ANY_VAL
//   constant: feature test value
#ifdef __cpp_lib_any
    #define D_ENV_CPP_FEATURE_STL_ANY_VAL  __cpp_lib_any
#else
    #define D_ENV_CPP_FEATURE_STL_ANY_VAL  0L
#endif

// D_ENV_CPP_FEATURE_STL_ANY_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_STL_ANY_VERS  "(C++17)"


// D_ENV_CPP_FEATURE_STL_APPLY
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_lib_apply
    #define D_ENV_CPP_FEATURE_STL_APPLY  1
#else
    #define D_ENV_CPP_FEATURE_STL_APPLY  0
#endif

// D_ENV_CPP_FEATURE_STL_APPLY_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_STL_APPLY_NAME  "__cpp_lib_apply"

// D_ENV_CPP_FEATURE_STL_APPLY_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_STL_APPLY_DESC  "std::apply"

// D_ENV_CPP_FEATURE_STL_APPLY_VAL
//   constant: feature test value
#ifdef __cpp_lib_apply
    #define D_ENV_CPP_FEATURE_STL_APPLY_VAL  __cpp_lib_apply
#else
    #define D_ENV_CPP_FEATURE_STL_APPLY_VAL  0L
#endif

// D_ENV_CPP_FEATURE_STL_APPLY_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_STL_APPLY_VERS  "(C++17)"


// D_ENV_CPP_FEATURE_STL_ARRAY_CONSTEXPR
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_lib_array_constexpr
    #define D_ENV_CPP_FEATURE_STL_ARRAY_CONSTEXPR  1
#else
    #define D_ENV_CPP_FEATURE_STL_ARRAY_CONSTEXPR  0
#endif

// D_ENV_CPP_FEATURE_STL_ARRAY_CONSTEXPR_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_STL_ARRAY_CONSTEXPR_NAME  "__cpp_lib_array_constexpr"

// D_ENV_CPP_FEATURE_STL_ARRAY_CONSTEXPR_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_STL_ARRAY_CONSTEXPR_DESC  "Constexpr for std::reverse_iterator, std::move_iterator, std::array"

// D_ENV_CPP_FEATURE_STL_ARRAY_CONSTEXPR_VAL
//   constant: feature test value
#ifdef __cpp_lib_array_constexpr
    #define D_ENV_CPP_FEATURE_STL_ARRAY_CONSTEXPR_VAL  __cpp_lib_array_constexpr
#else
    #define D_ENV_CPP_FEATURE_STL_ARRAY_CONSTEXPR_VAL  0L
#endif

// D_ENV_CPP_FEATURE_STL_ARRAY_CONSTEXPR_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_STL_ARRAY_CONSTEXPR_VERS  "(C++17)"


// D_ENV_CPP_FEATURE_STL_AS_CONST
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_lib_as_const
    #define D_ENV_CPP_FEATURE_STL_AS_CONST  1
#else
    #define D_ENV_CPP_FEATURE_STL_AS_CONST  0
#endif

// D_ENV_CPP_FEATURE_STL_AS_CONST_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_STL_AS_CONST_NAME  "__cpp_lib_as_const"

// D_ENV_CPP_FEATURE_STL_AS_CONST_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_STL_AS_CONST_DESC  "std::as_const"

// D_ENV_CPP_FEATURE_STL_AS_CONST_VAL
//   constant: feature test value
#ifdef __cpp_lib_as_const
    #define D_ENV_CPP_FEATURE_STL_AS_CONST_VAL  __cpp_lib_as_const
#else
    #define D_ENV_CPP_FEATURE_STL_AS_CONST_VAL  0L
#endif

// D_ENV_CPP_FEATURE_STL_AS_CONST_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_STL_AS_CONST_VERS  "(C++17)"


// D_ENV_CPP_FEATURE_STL_BOOL_CONSTANT
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_lib_bool_constant
    #define D_ENV_CPP_FEATURE_STL_BOOL_CONSTANT  1
#else
    #define D_ENV_CPP_FEATURE_STL_BOOL_CONSTANT  0
#endif

// D_ENV_CPP_FEATURE_STL_BOOL_CONSTANT_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_STL_BOOL_CONSTANT_NAME  "__cpp_lib_bool_constant"

// D_ENV_CPP_FEATURE_STL_BOOL_CONSTANT_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_STL_BOOL_CONSTANT_DESC  "std::bool_constant"

// D_ENV_CPP_FEATURE_STL_BOOL_CONSTANT_VAL
//   constant: feature test value
#ifdef __cpp_lib_bool_constant
    #define D_ENV_CPP_FEATURE_STL_BOOL_CONSTANT_VAL  __cpp_lib_bool_constant
#else
    #define D_ENV_CPP_FEATURE_STL_BOOL_CONSTANT_VAL  0L
#endif

// D_ENV_CPP_FEATURE_STL_BOOL_CONSTANT_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_STL_BOOL_CONSTANT_VERS  "(C++17)"


// D_ENV_CPP_FEATURE_STL_BOYER_MOORE_SEARCHER
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_lib_boyer_moore_searcher
    #define D_ENV_CPP_FEATURE_STL_BOYER_MOORE_SEARCHER  1
#else
    #define D_ENV_CPP_FEATURE_STL_BOYER_MOORE_SEARCHER  0
#endif

// D_ENV_CPP_FEATURE_STL_BOYER_MOORE_SEARCHER_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_STL_BOYER_MOORE_SEARCHER_NAME  "__cpp_lib_boyer_moore_searcher"

// D_ENV_CPP_FEATURE_STL_BOYER_MOORE_SEARCHER_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_STL_BOYER_MOORE_SEARCHER_DESC  "Searchers"

// D_ENV_CPP_FEATURE_STL_BOYER_MOORE_SEARCHER_VAL
//   constant: feature test value
#ifdef __cpp_lib_boyer_moore_searcher
    #define D_ENV_CPP_FEATURE_STL_BOYER_MOORE_SEARCHER_VAL  __cpp_lib_boyer_moore_searcher
#else
    #define D_ENV_CPP_FEATURE_STL_BOYER_MOORE_SEARCHER_VAL  0L
#endif

// D_ENV_CPP_FEATURE_STL_BOYER_MOORE_SEARCHER_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_STL_BOYER_MOORE_SEARCHER_VERS  "(C++17)"


// D_ENV_CPP_FEATURE_STL_BYTE
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_lib_byte
    #define D_ENV_CPP_FEATURE_STL_BYTE  1
#else
    #define D_ENV_CPP_FEATURE_STL_BYTE  0
#endif

// D_ENV_CPP_FEATURE_STL_BYTE_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_STL_BYTE_NAME  "__cpp_lib_byte"

// D_ENV_CPP_FEATURE_STL_BYTE_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_STL_BYTE_DESC  "std::byte"

// D_ENV_CPP_FEATURE_STL_BYTE_VAL
//   constant: feature test value
#ifdef __cpp_lib_byte
    #define D_ENV_CPP_FEATURE_STL_BYTE_VAL  __cpp_lib_byte
#else
    #define D_ENV_CPP_FEATURE_STL_BYTE_VAL  0L
#endif

// D_ENV_CPP_FEATURE_STL_BYTE_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_STL_BYTE_VERS  "(C++17)"


// D_ENV_CPP_FEATURE_STL_CLAMP
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_lib_clamp
    #define D_ENV_CPP_FEATURE_STL_CLAMP  1
#else
    #define D_ENV_CPP_FEATURE_STL_CLAMP  0
#endif

// D_ENV_CPP_FEATURE_STL_CLAMP_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_STL_CLAMP_NAME  "__cpp_lib_clamp"

// D_ENV_CPP_FEATURE_STL_CLAMP_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_STL_CLAMP_DESC  "std::clamp"

// D_ENV_CPP_FEATURE_STL_CLAMP_VAL
//   constant: feature test value
#ifdef __cpp_lib_clamp
    #define D_ENV_CPP_FEATURE_STL_CLAMP_VAL  __cpp_lib_clamp
#else
    #define D_ENV_CPP_FEATURE_STL_CLAMP_VAL  0L
#endif

// D_ENV_CPP_FEATURE_STL_CLAMP_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_STL_CLAMP_VERS  "(C++17)"


// D_ENV_CPP_FEATURE_STL_FILESYSTEM
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_lib_filesystem
    #define D_ENV_CPP_FEATURE_STL_FILESYSTEM  1
#else
    #define D_ENV_CPP_FEATURE_STL_FILESYSTEM  0
#endif

// D_ENV_CPP_FEATURE_STL_FILESYSTEM_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_STL_FILESYSTEM_NAME  "__cpp_lib_filesystem"

// D_ENV_CPP_FEATURE_STL_FILESYSTEM_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_STL_FILESYSTEM_DESC  "Filesystem library"

// D_ENV_CPP_FEATURE_STL_FILESYSTEM_VAL
//   constant: feature test value
#ifdef __cpp_lib_filesystem
    #define D_ENV_CPP_FEATURE_STL_FILESYSTEM_VAL  __cpp_lib_filesystem
#else
    #define D_ENV_CPP_FEATURE_STL_FILESYSTEM_VAL  0L
#endif

// D_ENV_CPP_FEATURE_STL_FILESYSTEM_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_STL_FILESYSTEM_VERS  "(C++17)"


// D_ENV_CPP_FEATURE_STL_OPTIONAL
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_lib_optional
    #define D_ENV_CPP_FEATURE_STL_OPTIONAL  1
#else
    #define D_ENV_CPP_FEATURE_STL_OPTIONAL  0
#endif

// D_ENV_CPP_FEATURE_STL_OPTIONAL_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_STL_OPTIONAL_NAME  "__cpp_lib_optional"

// D_ENV_CPP_FEATURE_STL_OPTIONAL_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_STL_OPTIONAL_DESC  "std::optional"

// D_ENV_CPP_FEATURE_STL_OPTIONAL_VAL
//   constant: feature test value
#ifdef __cpp_lib_optional
    #define D_ENV_CPP_FEATURE_STL_OPTIONAL_VAL  __cpp_lib_optional
#else
    #define D_ENV_CPP_FEATURE_STL_OPTIONAL_VAL  0L
#endif

// D_ENV_CPP_FEATURE_STL_OPTIONAL_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_STL_OPTIONAL_VERS  "(C++17)"


// D_ENV_CPP_FEATURE_STL_VARIANT
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_lib_variant
    #define D_ENV_CPP_FEATURE_STL_VARIANT  1
#else
    #define D_ENV_CPP_FEATURE_STL_VARIANT  0
#endif

// D_ENV_CPP_FEATURE_STL_VARIANT_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_STL_VARIANT_NAME  "__cpp_lib_variant"

// D_ENV_CPP_FEATURE_STL_VARIANT_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_STL_VARIANT_DESC  "std::variant"

// D_ENV_CPP_FEATURE_STL_VARIANT_VAL
//   constant: feature test value
#ifdef __cpp_lib_variant
    #define D_ENV_CPP_FEATURE_STL_VARIANT_VAL  __cpp_lib_variant
#else
    #define D_ENV_CPP_FEATURE_STL_VARIANT_VAL  0L
#endif

// D_ENV_CPP_FEATURE_STL_VARIANT_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_STL_VARIANT_VERS  "(C++17)"


// -----------------------------------------------------------------------------
// C++20 Library Features
// -----------------------------------------------------------------------------

// D_ENV_CPP_FEATURE_STL_ASSUME_ALIGNED
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_lib_assume_aligned
    #define D_ENV_CPP_FEATURE_STL_ASSUME_ALIGNED  1
#else
    #define D_ENV_CPP_FEATURE_STL_ASSUME_ALIGNED  0
#endif

// D_ENV_CPP_FEATURE_STL_ASSUME_ALIGNED_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_STL_ASSUME_ALIGNED_NAME  "__cpp_lib_assume_aligned"

// D_ENV_CPP_FEATURE_STL_ASSUME_ALIGNED_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_STL_ASSUME_ALIGNED_DESC  "std::assume_aligned"

// D_ENV_CPP_FEATURE_STL_ASSUME_ALIGNED_VAL
//   constant: feature test value
#ifdef __cpp_lib_assume_aligned
    #define D_ENV_CPP_FEATURE_STL_ASSUME_ALIGNED_VAL  __cpp_lib_assume_aligned
#else
    #define D_ENV_CPP_FEATURE_STL_ASSUME_ALIGNED_VAL  0L
#endif

// D_ENV_CPP_FEATURE_STL_ASSUME_ALIGNED_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_STL_ASSUME_ALIGNED_VERS  "(C++20)"


// D_ENV_CPP_FEATURE_STL_ATOMIC_FLAG_TEST
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_lib_atomic_flag_test
    #define D_ENV_CPP_FEATURE_STL_ATOMIC_FLAG_TEST  1
#else
    #define D_ENV_CPP_FEATURE_STL_ATOMIC_FLAG_TEST  0
#endif

// D_ENV_CPP_FEATURE_STL_ATOMIC_FLAG_TEST_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_STL_ATOMIC_FLAG_TEST_NAME  "__cpp_lib_atomic_flag_test"

// D_ENV_CPP_FEATURE_STL_ATOMIC_FLAG_TEST_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_STL_ATOMIC_FLAG_TEST_DESC  "std::atomic_flag::test"

// D_ENV_CPP_FEATURE_STL_ATOMIC_FLAG_TEST_VAL
//   constant: feature test value
#ifdef __cpp_lib_atomic_flag_test
    #define D_ENV_CPP_FEATURE_STL_ATOMIC_FLAG_TEST_VAL  __cpp_lib_atomic_flag_test
#else
    #define D_ENV_CPP_FEATURE_STL_ATOMIC_FLAG_TEST_VAL  0L
#endif

// D_ENV_CPP_FEATURE_STL_ATOMIC_FLAG_TEST_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_STL_ATOMIC_FLAG_TEST_VERS  "(C++20)"


// D_ENV_CPP_FEATURE_STL_ATOMIC_FLOAT
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_lib_atomic_float
    #define D_ENV_CPP_FEATURE_STL_ATOMIC_FLOAT  1
#else
    #define D_ENV_CPP_FEATURE_STL_ATOMIC_FLOAT  0
#endif

// D_ENV_CPP_FEATURE_STL_ATOMIC_FLOAT_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_STL_ATOMIC_FLOAT_NAME  "__cpp_lib_atomic_float"

// D_ENV_CPP_FEATURE_STL_ATOMIC_FLOAT_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_STL_ATOMIC_FLOAT_DESC  "Floating-point atomic"

// D_ENV_CPP_FEATURE_STL_ATOMIC_FLOAT_VAL
//   constant: feature test value
#ifdef __cpp_lib_atomic_float
    #define D_ENV_CPP_FEATURE_STL_ATOMIC_FLOAT_VAL  __cpp_lib_atomic_float
#else
    #define D_ENV_CPP_FEATURE_STL_ATOMIC_FLOAT_VAL  0L
#endif

// D_ENV_CPP_FEATURE_STL_ATOMIC_FLOAT_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_STL_ATOMIC_FLOAT_VERS  "(C++20)"


// D_ENV_CPP_FEATURE_STL_ATOMIC_REF
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_lib_atomic_ref
    #define D_ENV_CPP_FEATURE_STL_ATOMIC_REF  1
#else
    #define D_ENV_CPP_FEATURE_STL_ATOMIC_REF  0
#endif

// D_ENV_CPP_FEATURE_STL_ATOMIC_REF_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_STL_ATOMIC_REF_NAME  "__cpp_lib_atomic_ref"

// D_ENV_CPP_FEATURE_STL_ATOMIC_REF_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_STL_ATOMIC_REF_DESC  "std::atomic_ref"

// D_ENV_CPP_FEATURE_STL_ATOMIC_REF_VAL
//   constant: feature test value
#ifdef __cpp_lib_atomic_ref
    #define D_ENV_CPP_FEATURE_STL_ATOMIC_REF_VAL  __cpp_lib_atomic_ref
#else
    #define D_ENV_CPP_FEATURE_STL_ATOMIC_REF_VAL  0L
#endif

// D_ENV_CPP_FEATURE_STL_ATOMIC_REF_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_STL_ATOMIC_REF_VERS  "(C++20)"


// D_ENV_CPP_FEATURE_STL_ATOMIC_WAIT
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_lib_atomic_wait
    #define D_ENV_CPP_FEATURE_STL_ATOMIC_WAIT  1
#else
    #define D_ENV_CPP_FEATURE_STL_ATOMIC_WAIT  0
#endif

// D_ENV_CPP_FEATURE_STL_ATOMIC_WAIT_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_STL_ATOMIC_WAIT_NAME  "__cpp_lib_atomic_wait"

// D_ENV_CPP_FEATURE_STL_ATOMIC_WAIT_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_STL_ATOMIC_WAIT_DESC  "Efficient std::atomic waiting"

// D_ENV_CPP_FEATURE_STL_ATOMIC_WAIT_VAL
//   constant: feature test value
#ifdef __cpp_lib_atomic_wait
    #define D_ENV_CPP_FEATURE_STL_ATOMIC_WAIT_VAL  __cpp_lib_atomic_wait
#else
    #define D_ENV_CPP_FEATURE_STL_ATOMIC_WAIT_VAL  0L
#endif

// D_ENV_CPP_FEATURE_STL_ATOMIC_WAIT_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_STL_ATOMIC_WAIT_VERS  "(C++20)"


// D_ENV_CPP_FEATURE_STL_BARRIER
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_lib_barrier
    #define D_ENV_CPP_FEATURE_STL_BARRIER  1
#else
    #define D_ENV_CPP_FEATURE_STL_BARRIER  0
#endif

// D_ENV_CPP_FEATURE_STL_BARRIER_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_STL_BARRIER_NAME  "__cpp_lib_barrier"

// D_ENV_CPP_FEATURE_STL_BARRIER_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_STL_BARRIER_DESC  "std::barrier"

// D_ENV_CPP_FEATURE_STL_BARRIER_VAL
//   constant: feature test value
#ifdef __cpp_lib_barrier
    #define D_ENV_CPP_FEATURE_STL_BARRIER_VAL  __cpp_lib_barrier
#else
    #define D_ENV_CPP_FEATURE_STL_BARRIER_VAL  0L
#endif

// D_ENV_CPP_FEATURE_STL_BARRIER_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_STL_BARRIER_VERS  "(C++20)"


// D_ENV_CPP_FEATURE_STL_BIND_FRONT
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_lib_bind_front
    #define D_ENV_CPP_FEATURE_STL_BIND_FRONT  1
#else
    #define D_ENV_CPP_FEATURE_STL_BIND_FRONT  0
#endif

// D_ENV_CPP_FEATURE_STL_BIND_FRONT_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_STL_BIND_FRONT_NAME  "__cpp_lib_bind_front"

// D_ENV_CPP_FEATURE_STL_BIND_FRONT_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_STL_BIND_FRONT_DESC  "std::bind_front"

// D_ENV_CPP_FEATURE_STL_BIND_FRONT_VAL
//   constant: feature test value
#ifdef __cpp_lib_bind_front
    #define D_ENV_CPP_FEATURE_STL_BIND_FRONT_VAL  __cpp_lib_bind_front
#else
    #define D_ENV_CPP_FEATURE_STL_BIND_FRONT_VAL  0L
#endif

// D_ENV_CPP_FEATURE_STL_BIND_FRONT_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_STL_BIND_FRONT_VERS  "(C++20)"


// D_ENV_CPP_FEATURE_STL_BIT_CAST
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_lib_bit_cast
    #define D_ENV_CPP_FEATURE_STL_BIT_CAST  1
#else
    #define D_ENV_CPP_FEATURE_STL_BIT_CAST  0
#endif

// D_ENV_CPP_FEATURE_STL_BIT_CAST_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_STL_BIT_CAST_NAME  "__cpp_lib_bit_cast"

// D_ENV_CPP_FEATURE_STL_BIT_CAST_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_STL_BIT_CAST_DESC  "std::bit_cast"

// D_ENV_CPP_FEATURE_STL_BIT_CAST_VAL
//   constant: feature test value
#ifdef __cpp_lib_bit_cast
    #define D_ENV_CPP_FEATURE_STL_BIT_CAST_VAL  __cpp_lib_bit_cast
#else
    #define D_ENV_CPP_FEATURE_STL_BIT_CAST_VAL  0L
#endif

// D_ENV_CPP_FEATURE_STL_BIT_CAST_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_STL_BIT_CAST_VERS  "(C++20)"


// D_ENV_CPP_FEATURE_STL_BITOPS
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_lib_bitops
    #define D_ENV_CPP_FEATURE_STL_BITOPS  1
#else
    #define D_ENV_CPP_FEATURE_STL_BITOPS  0
#endif

// D_ENV_CPP_FEATURE_STL_BITOPS_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_STL_BITOPS_NAME  "__cpp_lib_bitops"

// D_ENV_CPP_FEATURE_STL_BITOPS_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_STL_BITOPS_DESC  "Bit operations"

// D_ENV_CPP_FEATURE_STL_BITOPS_VAL
//   constant: feature test value
#ifdef __cpp_lib_bitops
    #define D_ENV_CPP_FEATURE_STL_BITOPS_VAL  __cpp_lib_bitops
#else
    #define D_ENV_CPP_FEATURE_STL_BITOPS_VAL  0L
#endif

// D_ENV_CPP_FEATURE_STL_BITOPS_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_STL_BITOPS_VERS  "(C++20)"


// D_ENV_CPP_FEATURE_STL_BOUNDED_ARRAY_TRAITS
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_lib_bounded_array_traits
    #define D_ENV_CPP_FEATURE_STL_BOUNDED_ARRAY_TRAITS  1
#else
    #define D_ENV_CPP_FEATURE_STL_BOUNDED_ARRAY_TRAITS  0
#endif

// D_ENV_CPP_FEATURE_STL_BOUNDED_ARRAY_TRAITS_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_STL_BOUNDED_ARRAY_TRAITS_NAME  "__cpp_lib_bounded_array_traits"

// D_ENV_CPP_FEATURE_STL_BOUNDED_ARRAY_TRAITS_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_STL_BOUNDED_ARRAY_TRAITS_DESC  "std::is_bounded_array, std::is_unbounded_array"

// D_ENV_CPP_FEATURE_STL_BOUNDED_ARRAY_TRAITS_VAL
//   constant: feature test value
#ifdef __cpp_lib_bounded_array_traits
    #define D_ENV_CPP_FEATURE_STL_BOUNDED_ARRAY_TRAITS_VAL  __cpp_lib_bounded_array_traits
#else
    #define D_ENV_CPP_FEATURE_STL_BOUNDED_ARRAY_TRAITS_VAL  0L
#endif

// D_ENV_CPP_FEATURE_STL_BOUNDED_ARRAY_TRAITS_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_STL_BOUNDED_ARRAY_TRAITS_VERS  "(C++20)"


// D_ENV_CPP_FEATURE_STL_CHAR8_T
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_lib_char8_t
    #define D_ENV_CPP_FEATURE_STL_CHAR8_T  1
#else
    #define D_ENV_CPP_FEATURE_STL_CHAR8_T  0
#endif

// D_ENV_CPP_FEATURE_STL_CHAR8_T_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_STL_CHAR8_T_NAME  "__cpp_lib_char8_t"

// D_ENV_CPP_FEATURE_STL_CHAR8_T_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_STL_CHAR8_T_DESC  "Library support for char8_t"

// D_ENV_CPP_FEATURE_STL_CHAR8_T_VAL
//   constant: feature test value
#ifdef __cpp_lib_char8_t
    #define D_ENV_CPP_FEATURE_STL_CHAR8_T_VAL  __cpp_lib_char8_t
#else
    #define D_ENV_CPP_FEATURE_STL_CHAR8_T_VAL  0L
#endif

// D_ENV_CPP_FEATURE_STL_CHAR8_T_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_STL_CHAR8_T_VERS  "(C++20)"


// D_ENV_CPP_FEATURE_STL_CONCEPTS
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_lib_concepts
    #define D_ENV_CPP_FEATURE_STL_CONCEPTS  1
#else
    #define D_ENV_CPP_FEATURE_STL_CONCEPTS  0
#endif

// D_ENV_CPP_FEATURE_STL_CONCEPTS_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_STL_CONCEPTS_NAME  "__cpp_lib_concepts"

// D_ENV_CPP_FEATURE_STL_CONCEPTS_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_STL_CONCEPTS_DESC  "Standard library concepts"

// D_ENV_CPP_FEATURE_STL_CONCEPTS_VAL
//   constant: feature test value
#ifdef __cpp_lib_concepts
    #define D_ENV_CPP_FEATURE_STL_CONCEPTS_VAL  __cpp_lib_concepts
#else
    #define D_ENV_CPP_FEATURE_STL_CONCEPTS_VAL  0L
#endif

// D_ENV_CPP_FEATURE_STL_CONCEPTS_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_STL_CONCEPTS_VERS  "(C++20)"


// D_ENV_CPP_FEATURE_STL_CONSTEXPR_ALGORITHMS
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_lib_constexpr_algorithms
    #define D_ENV_CPP_FEATURE_STL_CONSTEXPR_ALGORITHMS  1
#else
    #define D_ENV_CPP_FEATURE_STL_CONSTEXPR_ALGORITHMS  0
#endif

// D_ENV_CPP_FEATURE_STL_CONSTEXPR_ALGORITHMS_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_STL_CONSTEXPR_ALGORITHMS_NAME  "__cpp_lib_constexpr_algorithms"

// D_ENV_CPP_FEATURE_STL_CONSTEXPR_ALGORITHMS_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_STL_CONSTEXPR_ALGORITHMS_DESC  "Constexpr for algorithms"

// D_ENV_CPP_FEATURE_STL_CONSTEXPR_ALGORITHMS_VAL
//   constant: feature test value
#ifdef __cpp_lib_constexpr_algorithms
    #define D_ENV_CPP_FEATURE_STL_CONSTEXPR_ALGORITHMS_VAL  __cpp_lib_constexpr_algorithms
#else
    #define D_ENV_CPP_FEATURE_STL_CONSTEXPR_ALGORITHMS_VAL  0L
#endif

// D_ENV_CPP_FEATURE_STL_CONSTEXPR_ALGORITHMS_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_STL_CONSTEXPR_ALGORITHMS_VERS  "(C++20)"


// D_ENV_CPP_FEATURE_STL_CONSTEXPR_COMPLEX
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_lib_constexpr_complex
    #define D_ENV_CPP_FEATURE_STL_CONSTEXPR_COMPLEX  1
#else
    #define D_ENV_CPP_FEATURE_STL_CONSTEXPR_COMPLEX  0
#endif

// D_ENV_CPP_FEATURE_STL_CONSTEXPR_COMPLEX_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_STL_CONSTEXPR_COMPLEX_NAME  "__cpp_lib_constexpr_complex"

// D_ENV_CPP_FEATURE_STL_CONSTEXPR_COMPLEX_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_STL_CONSTEXPR_COMPLEX_DESC  "Constexpr for std::complex"

// D_ENV_CPP_FEATURE_STL_CONSTEXPR_COMPLEX_VAL
//   constant: feature test value
#ifdef __cpp_lib_constexpr_complex
    #define D_ENV_CPP_FEATURE_STL_CONSTEXPR_COMPLEX_VAL  __cpp_lib_constexpr_complex
#else
    #define D_ENV_CPP_FEATURE_STL_CONSTEXPR_COMPLEX_VAL  0L
#endif

// D_ENV_CPP_FEATURE_STL_CONSTEXPR_COMPLEX_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_STL_CONSTEXPR_COMPLEX_VERS  "(C++20)"


// D_ENV_CPP_FEATURE_STL_CONSTEXPR_DYNAMIC_ALLOC
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_lib_constexpr_dynamic_alloc
    #define D_ENV_CPP_FEATURE_STL_CONSTEXPR_DYNAMIC_ALLOC  1
#else
    #define D_ENV_CPP_FEATURE_STL_CONSTEXPR_DYNAMIC_ALLOC  0
#endif

// D_ENV_CPP_FEATURE_STL_CONSTEXPR_DYNAMIC_ALLOC_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_STL_CONSTEXPR_DYNAMIC_ALLOC_NAME  "__cpp_lib_constexpr_dynamic_alloc"

// D_ENV_CPP_FEATURE_STL_CONSTEXPR_DYNAMIC_ALLOC_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_STL_CONSTEXPR_DYNAMIC_ALLOC_DESC  "Constexpr for std::allocator and related utilities"

// D_ENV_CPP_FEATURE_STL_CONSTEXPR_DYNAMIC_ALLOC_VAL
//   constant: feature test value
#ifdef __cpp_lib_constexpr_dynamic_alloc
    #define D_ENV_CPP_FEATURE_STL_CONSTEXPR_DYNAMIC_ALLOC_VAL  __cpp_lib_constexpr_dynamic_alloc
#else
    #define D_ENV_CPP_FEATURE_STL_CONSTEXPR_DYNAMIC_ALLOC_VAL  0L
#endif

// D_ENV_CPP_FEATURE_STL_CONSTEXPR_DYNAMIC_ALLOC_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_STL_CONSTEXPR_DYNAMIC_ALLOC_VERS  "(C++20)"


// D_ENV_CPP_FEATURE_STL_CONSTEXPR_STRING
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_lib_constexpr_string
    #define D_ENV_CPP_FEATURE_STL_CONSTEXPR_STRING  1
#else
    #define D_ENV_CPP_FEATURE_STL_CONSTEXPR_STRING  0
#endif

// D_ENV_CPP_FEATURE_STL_CONSTEXPR_STRING_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_STL_CONSTEXPR_STRING_NAME  "__cpp_lib_constexpr_string"

// D_ENV_CPP_FEATURE_STL_CONSTEXPR_STRING_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_STL_CONSTEXPR_STRING_DESC  "constexpr std::string"

// D_ENV_CPP_FEATURE_STL_CONSTEXPR_STRING_VAL
//   constant: feature test value
#ifdef __cpp_lib_constexpr_string
    #define D_ENV_CPP_FEATURE_STL_CONSTEXPR_STRING_VAL  __cpp_lib_constexpr_string
#else
    #define D_ENV_CPP_FEATURE_STL_CONSTEXPR_STRING_VAL  0L
#endif

// D_ENV_CPP_FEATURE_STL_CONSTEXPR_STRING_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_STL_CONSTEXPR_STRING_VERS  "(C++20)"


// D_ENV_CPP_FEATURE_STL_CONSTEXPR_VECTOR
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_lib_constexpr_vector
    #define D_ENV_CPP_FEATURE_STL_CONSTEXPR_VECTOR  1
#else
    #define D_ENV_CPP_FEATURE_STL_CONSTEXPR_VECTOR  0
#endif

// D_ENV_CPP_FEATURE_STL_CONSTEXPR_VECTOR_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_STL_CONSTEXPR_VECTOR_NAME  "__cpp_lib_constexpr_vector"

// D_ENV_CPP_FEATURE_STL_CONSTEXPR_VECTOR_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_STL_CONSTEXPR_VECTOR_DESC  "Constexpr for std::vector"

// D_ENV_CPP_FEATURE_STL_CONSTEXPR_VECTOR_VAL
//   constant: feature test value
#ifdef __cpp_lib_constexpr_vector
    #define D_ENV_CPP_FEATURE_STL_CONSTEXPR_VECTOR_VAL  __cpp_lib_constexpr_vector
#else
    #define D_ENV_CPP_FEATURE_STL_CONSTEXPR_VECTOR_VAL  0L
#endif

// D_ENV_CPP_FEATURE_STL_CONSTEXPR_VECTOR_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_STL_CONSTEXPR_VECTOR_VERS  "(C++20)"


// D_ENV_CPP_FEATURE_STL_COROUTINE
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_lib_coroutine
    #define D_ENV_CPP_FEATURE_STL_COROUTINE  1
#else
    #define D_ENV_CPP_FEATURE_STL_COROUTINE  0
#endif

// D_ENV_CPP_FEATURE_STL_COROUTINE_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_STL_COROUTINE_NAME  "__cpp_lib_coroutine"

// D_ENV_CPP_FEATURE_STL_COROUTINE_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_STL_COROUTINE_DESC  "Coroutines (library support)"

// D_ENV_CPP_FEATURE_STL_COROUTINE_VAL
//   constant: feature test value
#ifdef __cpp_lib_coroutine
    #define D_ENV_CPP_FEATURE_STL_COROUTINE_VAL  __cpp_lib_coroutine
#else
    #define D_ENV_CPP_FEATURE_STL_COROUTINE_VAL  0L
#endif

// D_ENV_CPP_FEATURE_STL_COROUTINE_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_STL_COROUTINE_VERS  "(C++20)"


// D_ENV_CPP_FEATURE_STL_ENDIAN
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_lib_endian
    #define D_ENV_CPP_FEATURE_STL_ENDIAN  1
#else
    #define D_ENV_CPP_FEATURE_STL_ENDIAN  0
#endif

// D_ENV_CPP_FEATURE_STL_ENDIAN_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_STL_ENDIAN_NAME  "__cpp_lib_endian"

// D_ENV_CPP_FEATURE_STL_ENDIAN_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_STL_ENDIAN_DESC  "std::endian"

// D_ENV_CPP_FEATURE_STL_ENDIAN_VAL
//   constant: feature test value
#ifdef __cpp_lib_endian
    #define D_ENV_CPP_FEATURE_STL_ENDIAN_VAL  __cpp_lib_endian
#else
    #define D_ENV_CPP_FEATURE_STL_ENDIAN_VAL  0L
#endif

// D_ENV_CPP_FEATURE_STL_ENDIAN_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_STL_ENDIAN_VERS  "(C++20)"


// D_ENV_CPP_FEATURE_STL_FORMAT
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_lib_format
    #define D_ENV_CPP_FEATURE_STL_FORMAT  1
#else
    #define D_ENV_CPP_FEATURE_STL_FORMAT  0
#endif

// D_ENV_CPP_FEATURE_STL_FORMAT_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_STL_FORMAT_NAME  "__cpp_lib_format"

// D_ENV_CPP_FEATURE_STL_FORMAT_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_STL_FORMAT_DESC  "Text formatting"

// D_ENV_CPP_FEATURE_STL_FORMAT_VAL
//   constant: feature test value
#ifdef __cpp_lib_format
    #define D_ENV_CPP_FEATURE_STL_FORMAT_VAL  __cpp_lib_format
#else
    #define D_ENV_CPP_FEATURE_STL_FORMAT_VAL  0L
#endif

// D_ENV_CPP_FEATURE_STL_FORMAT_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_STL_FORMAT_VERS  "(C++20)"


// D_ENV_CPP_FEATURE_STL_JTHREAD
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_lib_jthread
    #define D_ENV_CPP_FEATURE_STL_JTHREAD  1
#else
    #define D_ENV_CPP_FEATURE_STL_JTHREAD  0
#endif

// D_ENV_CPP_FEATURE_STL_JTHREAD_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_STL_JTHREAD_NAME  "__cpp_lib_jthread"

// D_ENV_CPP_FEATURE_STL_JTHREAD_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_STL_JTHREAD_DESC  "Stop token and joining thread"

// D_ENV_CPP_FEATURE_STL_JTHREAD_VAL
//   constant: feature test value
#ifdef __cpp_lib_jthread
    #define D_ENV_CPP_FEATURE_STL_JTHREAD_VAL  __cpp_lib_jthread
#else
    #define D_ENV_CPP_FEATURE_STL_JTHREAD_VAL  0L
#endif

// D_ENV_CPP_FEATURE_STL_JTHREAD_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_STL_JTHREAD_VERS  "(C++20)"


// D_ENV_CPP_FEATURE_STL_LATCH
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_lib_latch
    #define D_ENV_CPP_FEATURE_STL_LATCH  1
#else
    #define D_ENV_CPP_FEATURE_STL_LATCH  0
#endif

// D_ENV_CPP_FEATURE_STL_LATCH_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_STL_LATCH_NAME  "__cpp_lib_latch"

// D_ENV_CPP_FEATURE_STL_LATCH_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_STL_LATCH_DESC  "std::latch"

// D_ENV_CPP_FEATURE_STL_LATCH_VAL
//   constant: feature test value
#ifdef __cpp_lib_latch
    #define D_ENV_CPP_FEATURE_STL_LATCH_VAL  __cpp_lib_latch
#else
    #define D_ENV_CPP_FEATURE_STL_LATCH_VAL  0L
#endif

// D_ENV_CPP_FEATURE_STL_LATCH_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_STL_LATCH_VERS  "(C++20)"


// D_ENV_CPP_FEATURE_STL_MATH_CONSTANTS
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_lib_math_constants
    #define D_ENV_CPP_FEATURE_STL_MATH_CONSTANTS  1
#else
    #define D_ENV_CPP_FEATURE_STL_MATH_CONSTANTS  0
#endif

// D_ENV_CPP_FEATURE_STL_MATH_CONSTANTS_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_STL_MATH_CONSTANTS_NAME  "__cpp_lib_math_constants"

// D_ENV_CPP_FEATURE_STL_MATH_CONSTANTS_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_STL_MATH_CONSTANTS_DESC  "Mathematical constants"

// D_ENV_CPP_FEATURE_STL_MATH_CONSTANTS_VAL
//   constant: feature test value
#ifdef __cpp_lib_math_constants
    #define D_ENV_CPP_FEATURE_STL_MATH_CONSTANTS_VAL  __cpp_lib_math_constants
#else
    #define D_ENV_CPP_FEATURE_STL_MATH_CONSTANTS_VAL  0L
#endif

// D_ENV_CPP_FEATURE_STL_MATH_CONSTANTS_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_STL_MATH_CONSTANTS_VERS  "(C++20)"


// D_ENV_CPP_FEATURE_STL_RANGES
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_lib_ranges
    #define D_ENV_CPP_FEATURE_STL_RANGES  1
#else
    #define D_ENV_CPP_FEATURE_STL_RANGES  0
#endif

// D_ENV_CPP_FEATURE_STL_RANGES_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_STL_RANGES_NAME  "__cpp_lib_ranges"

// D_ENV_CPP_FEATURE_STL_RANGES_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_STL_RANGES_DESC  "Ranges library and constrained algorithms"

// D_ENV_CPP_FEATURE_STL_RANGES_VAL
//   constant: feature test value
#ifdef __cpp_lib_ranges
    #define D_ENV_CPP_FEATURE_STL_RANGES_VAL  __cpp_lib_ranges
#else
    #define D_ENV_CPP_FEATURE_STL_RANGES_VAL  0L
#endif

// D_ENV_CPP_FEATURE_STL_RANGES_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_STL_RANGES_VERS  "(C++20)"


// D_ENV_CPP_FEATURE_STL_SEMAPHORE
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_lib_semaphore
    #define D_ENV_CPP_FEATURE_STL_SEMAPHORE  1
#else
    #define D_ENV_CPP_FEATURE_STL_SEMAPHORE  0
#endif

// D_ENV_CPP_FEATURE_STL_SEMAPHORE_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_STL_SEMAPHORE_NAME  "__cpp_lib_semaphore"

// D_ENV_CPP_FEATURE_STL_SEMAPHORE_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_STL_SEMAPHORE_DESC  "std::counting_semaphore, std::binary_semaphore"

// D_ENV_CPP_FEATURE_STL_SEMAPHORE_VAL
//   constant: feature test value
#ifdef __cpp_lib_semaphore
    #define D_ENV_CPP_FEATURE_STL_SEMAPHORE_VAL  __cpp_lib_semaphore
#else
    #define D_ENV_CPP_FEATURE_STL_SEMAPHORE_VAL  0L
#endif

// D_ENV_CPP_FEATURE_STL_SEMAPHORE_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_STL_SEMAPHORE_VERS  "(C++20)"


// D_ENV_CPP_FEATURE_STL_SOURCE_LOCATION
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_lib_source_location
    #define D_ENV_CPP_FEATURE_STL_SOURCE_LOCATION  1
#else
    #define D_ENV_CPP_FEATURE_STL_SOURCE_LOCATION  0
#endif

// D_ENV_CPP_FEATURE_STL_SOURCE_LOCATION_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_STL_SOURCE_LOCATION_NAME  "__cpp_lib_source_location"

// D_ENV_CPP_FEATURE_STL_SOURCE_LOCATION_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_STL_SOURCE_LOCATION_DESC  "Source-code information capture"

// D_ENV_CPP_FEATURE_STL_SOURCE_LOCATION_VAL
//   constant: feature test value
#ifdef __cpp_lib_source_location
    #define D_ENV_CPP_FEATURE_STL_SOURCE_LOCATION_VAL  __cpp_lib_source_location
#else
    #define D_ENV_CPP_FEATURE_STL_SOURCE_LOCATION_VAL  0L
#endif

// D_ENV_CPP_FEATURE_STL_SOURCE_LOCATION_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_STL_SOURCE_LOCATION_VERS  "(C++20)"


// D_ENV_CPP_FEATURE_STL_SPAN
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_lib_span
    #define D_ENV_CPP_FEATURE_STL_SPAN  1
#else
    #define D_ENV_CPP_FEATURE_STL_SPAN  0
#endif

// D_ENV_CPP_FEATURE_STL_SPAN_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_STL_SPAN_NAME  "__cpp_lib_span"

// D_ENV_CPP_FEATURE_STL_SPAN_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_STL_SPAN_DESC  "std::span"

// D_ENV_CPP_FEATURE_STL_SPAN_VAL
//   constant: feature test value
#ifdef __cpp_lib_span
    #define D_ENV_CPP_FEATURE_STL_SPAN_VAL  __cpp_lib_span
#else
    #define D_ENV_CPP_FEATURE_STL_SPAN_VAL  0L
#endif

// D_ENV_CPP_FEATURE_STL_SPAN_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_STL_SPAN_VERS  "(C++20)"


// D_ENV_CPP_FEATURE_STL_THREE_WAY_COMPARISON
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_lib_three_way_comparison
    #define D_ENV_CPP_FEATURE_STL_THREE_WAY_COMPARISON  1
#else
    #define D_ENV_CPP_FEATURE_STL_THREE_WAY_COMPARISON  0
#endif

// D_ENV_CPP_FEATURE_STL_THREE_WAY_COMPARISON_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_STL_THREE_WAY_COMPARISON_NAME  "__cpp_lib_three_way_comparison"

// D_ENV_CPP_FEATURE_STL_THREE_WAY_COMPARISON_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_STL_THREE_WAY_COMPARISON_DESC  "Three-way comparison (library support)"

// D_ENV_CPP_FEATURE_STL_THREE_WAY_COMPARISON_VAL
//   constant: feature test value
#ifdef __cpp_lib_three_way_comparison
    #define D_ENV_CPP_FEATURE_STL_THREE_WAY_COMPARISON_VAL  __cpp_lib_three_way_comparison
#else
    #define D_ENV_CPP_FEATURE_STL_THREE_WAY_COMPARISON_VAL  0L
#endif

// D_ENV_CPP_FEATURE_STL_THREE_WAY_COMPARISON_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_STL_THREE_WAY_COMPARISON_VERS  "(C++20)"


// D_ENV_CPP_FEATURE_STL_TO_ARRAY
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_lib_to_array
    #define D_ENV_CPP_FEATURE_STL_TO_ARRAY  1
#else
    #define D_ENV_CPP_FEATURE_STL_TO_ARRAY  0
#endif

// D_ENV_CPP_FEATURE_STL_TO_ARRAY_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_STL_TO_ARRAY_NAME  "__cpp_lib_to_array"

// D_ENV_CPP_FEATURE_STL_TO_ARRAY_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_STL_TO_ARRAY_DESC  "std::to_array"

// D_ENV_CPP_FEATURE_STL_TO_ARRAY_VAL
//   constant: feature test value
#ifdef __cpp_lib_to_array
    #define D_ENV_CPP_FEATURE_STL_TO_ARRAY_VAL  __cpp_lib_to_array
#else
    #define D_ENV_CPP_FEATURE_STL_TO_ARRAY_VAL  0L
#endif

// D_ENV_CPP_FEATURE_STL_TO_ARRAY_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_STL_TO_ARRAY_VERS  "(C++20)"


// -----------------------------------------------------------------------------
// C++23 Library Features
// -----------------------------------------------------------------------------

// D_ENV_CPP_FEATURE_STL_ADAPTOR_ITERATOR_PAIR_CONSTRUCTOR
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_lib_adaptor_iterator_pair_constructor
    #define D_ENV_CPP_FEATURE_STL_ADAPTOR_ITERATOR_PAIR_CONSTRUCTOR  1
#else
    #define D_ENV_CPP_FEATURE_STL_ADAPTOR_ITERATOR_PAIR_CONSTRUCTOR  0
#endif

// D_ENV_CPP_FEATURE_STL_ADAPTOR_ITERATOR_PAIR_CONSTRUCTOR_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_STL_ADAPTOR_ITERATOR_PAIR_CONSTRUCTOR_NAME  "__cpp_lib_adaptor_iterator_pair_constructor"

// D_ENV_CPP_FEATURE_STL_ADAPTOR_ITERATOR_PAIR_CONSTRUCTOR_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_STL_ADAPTOR_ITERATOR_PAIR_CONSTRUCTOR_DESC  "Iterator pair constructors for std::stack and std::queue"

// D_ENV_CPP_FEATURE_STL_ADAPTOR_ITERATOR_PAIR_CONSTRUCTOR_VAL
//   constant: feature test value
#ifdef __cpp_lib_adaptor_iterator_pair_constructor
    #define D_ENV_CPP_FEATURE_STL_ADAPTOR_ITERATOR_PAIR_CONSTRUCTOR_VAL  __cpp_lib_adaptor_iterator_pair_constructor
#else
    #define D_ENV_CPP_FEATURE_STL_ADAPTOR_ITERATOR_PAIR_CONSTRUCTOR_VAL  0L
#endif

// D_ENV_CPP_FEATURE_STL_ADAPTOR_ITERATOR_PAIR_CONSTRUCTOR_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_STL_ADAPTOR_ITERATOR_PAIR_CONSTRUCTOR_VERS  "(C++23)"


// D_ENV_CPP_FEATURE_STL_ASSOCIATIVE_HETEROGENEOUS_ERASURE
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_lib_associative_heterogeneous_erasure
    #define D_ENV_CPP_FEATURE_STL_ASSOCIATIVE_HETEROGENEOUS_ERASURE  1
#else
    #define D_ENV_CPP_FEATURE_STL_ASSOCIATIVE_HETEROGENEOUS_ERASURE  0
#endif

// D_ENV_CPP_FEATURE_STL_ASSOCIATIVE_HETEROGENEOUS_ERASURE_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_STL_ASSOCIATIVE_HETEROGENEOUS_ERASURE_NAME  "__cpp_lib_associative_heterogeneous_erasure"

// D_ENV_CPP_FEATURE_STL_ASSOCIATIVE_HETEROGENEOUS_ERASURE_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_STL_ASSOCIATIVE_HETEROGENEOUS_ERASURE_DESC  "Heterogeneous erasure in associative containers"

// D_ENV_CPP_FEATURE_STL_ASSOCIATIVE_HETEROGENEOUS_ERASURE_VAL
//   constant: feature test value
#ifdef __cpp_lib_associative_heterogeneous_erasure
    #define D_ENV_CPP_FEATURE_STL_ASSOCIATIVE_HETEROGENEOUS_ERASURE_VAL  __cpp_lib_associative_heterogeneous_erasure
#else
    #define D_ENV_CPP_FEATURE_STL_ASSOCIATIVE_HETEROGENEOUS_ERASURE_VAL  0L
#endif

// D_ENV_CPP_FEATURE_STL_ASSOCIATIVE_HETEROGENEOUS_ERASURE_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_STL_ASSOCIATIVE_HETEROGENEOUS_ERASURE_VERS  "(C++23)"


// D_ENV_CPP_FEATURE_STL_BIND_BACK
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_lib_bind_back
    #define D_ENV_CPP_FEATURE_STL_BIND_BACK  1
#else
    #define D_ENV_CPP_FEATURE_STL_BIND_BACK  0
#endif

// D_ENV_CPP_FEATURE_STL_BIND_BACK_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_STL_BIND_BACK_NAME  "__cpp_lib_bind_back"

// D_ENV_CPP_FEATURE_STL_BIND_BACK_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_STL_BIND_BACK_DESC  "std::bind_back"

// D_ENV_CPP_FEATURE_STL_BIND_BACK_VAL
//   constant: feature test value
#ifdef __cpp_lib_bind_back
    #define D_ENV_CPP_FEATURE_STL_BIND_BACK_VAL  __cpp_lib_bind_back
#else
    #define D_ENV_CPP_FEATURE_STL_BIND_BACK_VAL  0L
#endif

// D_ENV_CPP_FEATURE_STL_BIND_BACK_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_STL_BIND_BACK_VERS  "(C++23)"


// D_ENV_CPP_FEATURE_STL_BYTESWAP
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_lib_byteswap
    #define D_ENV_CPP_FEATURE_STL_BYTESWAP  1
#else
    #define D_ENV_CPP_FEATURE_STL_BYTESWAP  0
#endif

// D_ENV_CPP_FEATURE_STL_BYTESWAP_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_STL_BYTESWAP_NAME  "__cpp_lib_byteswap"

// D_ENV_CPP_FEATURE_STL_BYTESWAP_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_STL_BYTESWAP_DESC  "std::byteswap"

// D_ENV_CPP_FEATURE_STL_BYTESWAP_VAL
//   constant: feature test value
#ifdef __cpp_lib_byteswap
    #define D_ENV_CPP_FEATURE_STL_BYTESWAP_VAL  __cpp_lib_byteswap
#else
    #define D_ENV_CPP_FEATURE_STL_BYTESWAP_VAL  0L
#endif

// D_ENV_CPP_FEATURE_STL_BYTESWAP_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_STL_BYTESWAP_VERS  "(C++23)"


// D_ENV_CPP_FEATURE_STL_CONSTEXPR_BITSET
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_lib_constexpr_bitset
    #define D_ENV_CPP_FEATURE_STL_CONSTEXPR_BITSET  1
#else
    #define D_ENV_CPP_FEATURE_STL_CONSTEXPR_BITSET  0
#endif

// D_ENV_CPP_FEATURE_STL_CONSTEXPR_BITSET_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_STL_CONSTEXPR_BITSET_NAME  "__cpp_lib_constexpr_bitset"

// D_ENV_CPP_FEATURE_STL_CONSTEXPR_BITSET_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_STL_CONSTEXPR_BITSET_DESC  "A more constexpr std::bitset"

// D_ENV_CPP_FEATURE_STL_CONSTEXPR_BITSET_VAL
//   constant: feature test value
#ifdef __cpp_lib_constexpr_bitset
    #define D_ENV_CPP_FEATURE_STL_CONSTEXPR_BITSET_VAL  __cpp_lib_constexpr_bitset
#else
    #define D_ENV_CPP_FEATURE_STL_CONSTEXPR_BITSET_VAL  0L
#endif

// D_ENV_CPP_FEATURE_STL_CONSTEXPR_BITSET_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_STL_CONSTEXPR_BITSET_VERS  "(C++23)"


// D_ENV_CPP_FEATURE_STL_CONSTEXPR_CHARCONV
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_lib_constexpr_charconv
    #define D_ENV_CPP_FEATURE_STL_CONSTEXPR_CHARCONV  1
#else
    #define D_ENV_CPP_FEATURE_STL_CONSTEXPR_CHARCONV  0
#endif

// D_ENV_CPP_FEATURE_STL_CONSTEXPR_CHARCONV_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_STL_CONSTEXPR_CHARCONV_NAME  "__cpp_lib_constexpr_charconv"

// D_ENV_CPP_FEATURE_STL_CONSTEXPR_CHARCONV_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_STL_CONSTEXPR_CHARCONV_DESC  "Constexpr for std::to_chars and std::from_chars"

// D_ENV_CPP_FEATURE_STL_CONSTEXPR_CHARCONV_VAL
//   constant: feature test value
#ifdef __cpp_lib_constexpr_charconv
    #define D_ENV_CPP_FEATURE_STL_CONSTEXPR_CHARCONV_VAL  __cpp_lib_constexpr_charconv
#else
    #define D_ENV_CPP_FEATURE_STL_CONSTEXPR_CHARCONV_VAL  0L
#endif

// D_ENV_CPP_FEATURE_STL_CONSTEXPR_CHARCONV_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_STL_CONSTEXPR_CHARCONV_VERS  "(C++23)"


// D_ENV_CPP_FEATURE_STL_CONSTEXPR_CMATH
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_lib_constexpr_cmath
    #define D_ENV_CPP_FEATURE_STL_CONSTEXPR_CMATH  1
#else
    #define D_ENV_CPP_FEATURE_STL_CONSTEXPR_CMATH  0
#endif

// D_ENV_CPP_FEATURE_STL_CONSTEXPR_CMATH_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_STL_CONSTEXPR_CMATH_NAME  "__cpp_lib_constexpr_cmath"

// D_ENV_CPP_FEATURE_STL_CONSTEXPR_CMATH_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_STL_CONSTEXPR_CMATH_DESC  "Constexpr for mathematical functions in <cmath>"

// D_ENV_CPP_FEATURE_STL_CONSTEXPR_CMATH_VAL
//   constant: feature test value
#ifdef __cpp_lib_constexpr_cmath
    #define D_ENV_CPP_FEATURE_STL_CONSTEXPR_CMATH_VAL  __cpp_lib_constexpr_cmath
#else
    #define D_ENV_CPP_FEATURE_STL_CONSTEXPR_CMATH_VAL  0L
#endif

// D_ENV_CPP_FEATURE_STL_CONSTEXPR_CMATH_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_STL_CONSTEXPR_CMATH_VERS  "(C++23)"


// D_ENV_CPP_FEATURE_STL_EXPECTED
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_lib_expected
    #define D_ENV_CPP_FEATURE_STL_EXPECTED  1
#else
    #define D_ENV_CPP_FEATURE_STL_EXPECTED  0
#endif

// D_ENV_CPP_FEATURE_STL_EXPECTED_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_STL_EXPECTED_NAME  "__cpp_lib_expected"

// D_ENV_CPP_FEATURE_STL_EXPECTED_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_STL_EXPECTED_DESC  "class template std::expected"

// D_ENV_CPP_FEATURE_STL_EXPECTED_VAL
//   constant: feature test value
#ifdef __cpp_lib_expected
    #define D_ENV_CPP_FEATURE_STL_EXPECTED_VAL  __cpp_lib_expected
#else
    #define D_ENV_CPP_FEATURE_STL_EXPECTED_VAL  0L
#endif

// D_ENV_CPP_FEATURE_STL_EXPECTED_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_STL_EXPECTED_VERS  "(C++23)"


// D_ENV_CPP_FEATURE_STL_FLAT_MAP
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_lib_flat_map
    #define D_ENV_CPP_FEATURE_STL_FLAT_MAP  1
#else
    #define D_ENV_CPP_FEATURE_STL_FLAT_MAP  0
#endif

// D_ENV_CPP_FEATURE_STL_FLAT_MAP_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_STL_FLAT_MAP_NAME  "__cpp_lib_flat_map"

// D_ENV_CPP_FEATURE_STL_FLAT_MAP_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_STL_FLAT_MAP_DESC  "std::flat_map and std::flat_multimap"

// D_ENV_CPP_FEATURE_STL_FLAT_MAP_VAL
//   constant: feature test value
#ifdef __cpp_lib_flat_map
    #define D_ENV_CPP_FEATURE_STL_FLAT_MAP_VAL  __cpp_lib_flat_map
#else
    #define D_ENV_CPP_FEATURE_STL_FLAT_MAP_VAL  0L
#endif

// D_ENV_CPP_FEATURE_STL_FLAT_MAP_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_STL_FLAT_MAP_VERS  "(C++23)"


// D_ENV_CPP_FEATURE_STL_FLAT_SET
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_lib_flat_set
    #define D_ENV_CPP_FEATURE_STL_FLAT_SET  1
#else
    #define D_ENV_CPP_FEATURE_STL_FLAT_SET  0
#endif

// D_ENV_CPP_FEATURE_STL_FLAT_SET_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_STL_FLAT_SET_NAME  "__cpp_lib_flat_set"

// D_ENV_CPP_FEATURE_STL_FLAT_SET_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_STL_FLAT_SET_DESC  "std::flat_set and std::flat_multiset"

// D_ENV_CPP_FEATURE_STL_FLAT_SET_VAL
//   constant: feature test value
#ifdef __cpp_lib_flat_set
    #define D_ENV_CPP_FEATURE_STL_FLAT_SET_VAL  __cpp_lib_flat_set
#else
    #define D_ENV_CPP_FEATURE_STL_FLAT_SET_VAL  0L
#endif

// D_ENV_CPP_FEATURE_STL_FLAT_SET_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_STL_FLAT_SET_VERS  "(C++23)"


// D_ENV_CPP_FEATURE_STL_GENERATOR
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_lib_generator
    #define D_ENV_CPP_FEATURE_STL_GENERATOR  1
#else
    #define D_ENV_CPP_FEATURE_STL_GENERATOR  0
#endif

// D_ENV_CPP_FEATURE_STL_GENERATOR_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_STL_GENERATOR_NAME  "__cpp_lib_generator"

// D_ENV_CPP_FEATURE_STL_GENERATOR_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_STL_GENERATOR_DESC  "std::generator: Synchronous coroutine generator for ranges"

// D_ENV_CPP_FEATURE_STL_GENERATOR_VAL
//   constant: feature test value
#ifdef __cpp_lib_generator
    #define D_ENV_CPP_FEATURE_STL_GENERATOR_VAL  __cpp_lib_generator
#else
    #define D_ENV_CPP_FEATURE_STL_GENERATOR_VAL  0L
#endif

// D_ENV_CPP_FEATURE_STL_GENERATOR_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_STL_GENERATOR_VERS  "(C++23)"


// D_ENV_CPP_FEATURE_STL_MDSPAN
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_lib_mdspan
    #define D_ENV_CPP_FEATURE_STL_MDSPAN  1
#else
    #define D_ENV_CPP_FEATURE_STL_MDSPAN  0
#endif

// D_ENV_CPP_FEATURE_STL_MDSPAN_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_STL_MDSPAN_NAME  "__cpp_lib_mdspan"

// D_ENV_CPP_FEATURE_STL_MDSPAN_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_STL_MDSPAN_DESC  "std::mdspan"

// D_ENV_CPP_FEATURE_STL_MDSPAN_VAL
//   constant: feature test value
#ifdef __cpp_lib_mdspan
    #define D_ENV_CPP_FEATURE_STL_MDSPAN_VAL  __cpp_lib_mdspan
#else
    #define D_ENV_CPP_FEATURE_STL_MDSPAN_VAL  0L
#endif

// D_ENV_CPP_FEATURE_STL_MDSPAN_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_STL_MDSPAN_VERS  "(C++23)"


// D_ENV_CPP_FEATURE_STL_MOVE_ONLY_FUNCTION
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_lib_move_only_function
    #define D_ENV_CPP_FEATURE_STL_MOVE_ONLY_FUNCTION  1
#else
    #define D_ENV_CPP_FEATURE_STL_MOVE_ONLY_FUNCTION  0
#endif

// D_ENV_CPP_FEATURE_STL_MOVE_ONLY_FUNCTION_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_STL_MOVE_ONLY_FUNCTION_NAME  "__cpp_lib_move_only_function"

// D_ENV_CPP_FEATURE_STL_MOVE_ONLY_FUNCTION_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_STL_MOVE_ONLY_FUNCTION_DESC  "std::move_only_function"

// D_ENV_CPP_FEATURE_STL_MOVE_ONLY_FUNCTION_VAL
//   constant: feature test value
#ifdef __cpp_lib_move_only_function
    #define D_ENV_CPP_FEATURE_STL_MOVE_ONLY_FUNCTION_VAL  __cpp_lib_move_only_function
#else
    #define D_ENV_CPP_FEATURE_STL_MOVE_ONLY_FUNCTION_VAL  0L
#endif

// D_ENV_CPP_FEATURE_STL_MOVE_ONLY_FUNCTION_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_STL_MOVE_ONLY_FUNCTION_VERS  "(C++23)"


// D_ENV_CPP_FEATURE_STL_PRINT
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_lib_print
    #define D_ENV_CPP_FEATURE_STL_PRINT  1
#else
    #define D_ENV_CPP_FEATURE_STL_PRINT  0
#endif

// D_ENV_CPP_FEATURE_STL_PRINT_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_STL_PRINT_NAME  "__cpp_lib_print"

// D_ENV_CPP_FEATURE_STL_PRINT_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_STL_PRINT_DESC  "Formatted output"

// D_ENV_CPP_FEATURE_STL_PRINT_VAL
//   constant: feature test value
#ifdef __cpp_lib_print
    #define D_ENV_CPP_FEATURE_STL_PRINT_VAL  __cpp_lib_print
#else
    #define D_ENV_CPP_FEATURE_STL_PRINT_VAL  0L
#endif

// D_ENV_CPP_FEATURE_STL_PRINT_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_STL_PRINT_VERS  "(C++23)"


// D_ENV_CPP_FEATURE_STL_RANGES_TO_CONTAINER
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_lib_ranges_to_container
    #define D_ENV_CPP_FEATURE_STL_RANGES_TO_CONTAINER  1
#else
    #define D_ENV_CPP_FEATURE_STL_RANGES_TO_CONTAINER  0
#endif

// D_ENV_CPP_FEATURE_STL_RANGES_TO_CONTAINER_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_STL_RANGES_TO_CONTAINER_NAME  "__cpp_lib_ranges_to_container"

// D_ENV_CPP_FEATURE_STL_RANGES_TO_CONTAINER_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_STL_RANGES_TO_CONTAINER_DESC  "std::ranges::to"

// D_ENV_CPP_FEATURE_STL_RANGES_TO_CONTAINER_VAL
//   constant: feature test value
#ifdef __cpp_lib_ranges_to_container
    #define D_ENV_CPP_FEATURE_STL_RANGES_TO_CONTAINER_VAL  __cpp_lib_ranges_to_container
#else
    #define D_ENV_CPP_FEATURE_STL_RANGES_TO_CONTAINER_VAL  0L
#endif

// D_ENV_CPP_FEATURE_STL_RANGES_TO_CONTAINER_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_STL_RANGES_TO_CONTAINER_VERS  "(C++23)"


// D_ENV_CPP_FEATURE_STL_SPANSTREAM
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_lib_spanstream
    #define D_ENV_CPP_FEATURE_STL_SPANSTREAM  1
#else
    #define D_ENV_CPP_FEATURE_STL_SPANSTREAM  0
#endif

// D_ENV_CPP_FEATURE_STL_SPANSTREAM_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_STL_SPANSTREAM_NAME  "__cpp_lib_spanstream"

// D_ENV_CPP_FEATURE_STL_SPANSTREAM_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_STL_SPANSTREAM_DESC  "std::spanbuf, std::spanstream"

// D_ENV_CPP_FEATURE_STL_SPANSTREAM_VAL
//   constant: feature test value
#ifdef __cpp_lib_spanstream
    #define D_ENV_CPP_FEATURE_STL_SPANSTREAM_VAL  __cpp_lib_spanstream
#else
    #define D_ENV_CPP_FEATURE_STL_SPANSTREAM_VAL  0L
#endif

// D_ENV_CPP_FEATURE_STL_SPANSTREAM_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_STL_SPANSTREAM_VERS  "(C++23)"


// D_ENV_CPP_FEATURE_STL_STACKTRACE
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_lib_stacktrace
    #define D_ENV_CPP_FEATURE_STL_STACKTRACE  1
#else
    #define D_ENV_CPP_FEATURE_STL_STACKTRACE  0
#endif

// D_ENV_CPP_FEATURE_STL_STACKTRACE_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_STL_STACKTRACE_NAME  "__cpp_lib_stacktrace"

// D_ENV_CPP_FEATURE_STL_STACKTRACE_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_STL_STACKTRACE_DESC  "Stacktrace library"

// D_ENV_CPP_FEATURE_STL_STACKTRACE_VAL
//   constant: feature test value
#ifdef __cpp_lib_stacktrace
    #define D_ENV_CPP_FEATURE_STL_STACKTRACE_VAL  __cpp_lib_stacktrace
#else
    #define D_ENV_CPP_FEATURE_STL_STACKTRACE_VAL  0L
#endif

// D_ENV_CPP_FEATURE_STL_STACKTRACE_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_STL_STACKTRACE_VERS  "(C++23)"


// D_ENV_CPP_FEATURE_STL_STDATOMIC_H
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_lib_stdatomic_h
    #define D_ENV_CPP_FEATURE_STL_STDATOMIC_H  1
#else
    #define D_ENV_CPP_FEATURE_STL_STDATOMIC_H  0
#endif

// D_ENV_CPP_FEATURE_STL_STDATOMIC_H_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_STL_STDATOMIC_H_NAME  "__cpp_lib_stdatomic_h"

// D_ENV_CPP_FEATURE_STL_STDATOMIC_H_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_STL_STDATOMIC_H_DESC  "Compatibility header for C atomic operations"

// D_ENV_CPP_FEATURE_STL_STDATOMIC_H_VAL
//   constant: feature test value
#ifdef __cpp_lib_stdatomic_h
    #define D_ENV_CPP_FEATURE_STL_STDATOMIC_H_VAL  __cpp_lib_stdatomic_h
#else
    #define D_ENV_CPP_FEATURE_STL_STDATOMIC_H_VAL  0L
#endif

// D_ENV_CPP_FEATURE_STL_STDATOMIC_H_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_STL_STDATOMIC_H_VERS  "(C++23)"


// D_ENV_CPP_FEATURE_STL_STRING_CONTAINS
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_lib_string_contains
    #define D_ENV_CPP_FEATURE_STL_STRING_CONTAINS  1
#else
    #define D_ENV_CPP_FEATURE_STL_STRING_CONTAINS  0
#endif

// D_ENV_CPP_FEATURE_STL_STRING_CONTAINS_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_STL_STRING_CONTAINS_NAME  "__cpp_lib_string_contains"

// D_ENV_CPP_FEATURE_STL_STRING_CONTAINS_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_STL_STRING_CONTAINS_DESC  "contains() for std::basic_string and std::basic_string_view"

// D_ENV_CPP_FEATURE_STL_STRING_CONTAINS_VAL
//   constant: feature test value
#ifdef __cpp_lib_string_contains
    #define D_ENV_CPP_FEATURE_STL_STRING_CONTAINS_VAL  __cpp_lib_string_contains
#else
    #define D_ENV_CPP_FEATURE_STL_STRING_CONTAINS_VAL  0L
#endif

// D_ENV_CPP_FEATURE_STL_STRING_CONTAINS_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_STL_STRING_CONTAINS_VERS  "(C++23)"


// D_ENV_CPP_FEATURE_STL_STRING_RESIZE_AND_OVERWRITE
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_lib_string_resize_and_overwrite
    #define D_ENV_CPP_FEATURE_STL_STRING_RESIZE_AND_OVERWRITE  1
#else
    #define D_ENV_CPP_FEATURE_STL_STRING_RESIZE_AND_OVERWRITE  0
#endif

// D_ENV_CPP_FEATURE_STL_STRING_RESIZE_AND_OVERWRITE_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_STL_STRING_RESIZE_AND_OVERWRITE_NAME  "__cpp_lib_string_resize_and_overwrite"

// D_ENV_CPP_FEATURE_STL_STRING_RESIZE_AND_OVERWRITE_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_STL_STRING_RESIZE_AND_OVERWRITE_DESC  "std::basic_string::resize_and_overwrite"

// D_ENV_CPP_FEATURE_STL_STRING_RESIZE_AND_OVERWRITE_VAL
//   constant: feature test value
#ifdef __cpp_lib_string_resize_and_overwrite
    #define D_ENV_CPP_FEATURE_STL_STRING_RESIZE_AND_OVERWRITE_VAL  __cpp_lib_string_resize_and_overwrite
#else
    #define D_ENV_CPP_FEATURE_STL_STRING_RESIZE_AND_OVERWRITE_VAL  0L
#endif

// D_ENV_CPP_FEATURE_STL_STRING_RESIZE_AND_OVERWRITE_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_STL_STRING_RESIZE_AND_OVERWRITE_VERS  "(C++23)"


// D_ENV_CPP_FEATURE_STL_UNREACHABLE
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_lib_unreachable
    #define D_ENV_CPP_FEATURE_STL_UNREACHABLE  1
#else
    #define D_ENV_CPP_FEATURE_STL_UNREACHABLE  0
#endif

// D_ENV_CPP_FEATURE_STL_UNREACHABLE_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_STL_UNREACHABLE_NAME  "__cpp_lib_unreachable"

// D_ENV_CPP_FEATURE_STL_UNREACHABLE_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_STL_UNREACHABLE_DESC  "std::unreachable"

// D_ENV_CPP_FEATURE_STL_UNREACHABLE_VAL
//   constant: feature test value
#ifdef __cpp_lib_unreachable
    #define D_ENV_CPP_FEATURE_STL_UNREACHABLE_VAL  __cpp_lib_unreachable
#else
    #define D_ENV_CPP_FEATURE_STL_UNREACHABLE_VAL  0L
#endif

// D_ENV_CPP_FEATURE_STL_UNREACHABLE_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_STL_UNREACHABLE_VERS  "(C++23)"


// -----------------------------------------------------------------------------
// C++26 Library Features
// -----------------------------------------------------------------------------

// D_ENV_CPP_FEATURE_STL_ALGORITHM_DEFAULT_VALUE_TYPE
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_lib_algorithm_default_value_type
    #define D_ENV_CPP_FEATURE_STL_ALGORITHM_DEFAULT_VALUE_TYPE  1
#else
    #define D_ENV_CPP_FEATURE_STL_ALGORITHM_DEFAULT_VALUE_TYPE  0
#endif

// D_ENV_CPP_FEATURE_STL_ALGORITHM_DEFAULT_VALUE_TYPE_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_STL_ALGORITHM_DEFAULT_VALUE_TYPE_NAME  "__cpp_lib_algorithm_default_value_type"

// D_ENV_CPP_FEATURE_STL_ALGORITHM_DEFAULT_VALUE_TYPE_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_STL_ALGORITHM_DEFAULT_VALUE_TYPE_DESC  "Enabling list-initialization for algorithms"

// D_ENV_CPP_FEATURE_STL_ALGORITHM_DEFAULT_VALUE_TYPE_VAL
//   constant: feature test value
#ifdef __cpp_lib_algorithm_default_value_type
    #define D_ENV_CPP_FEATURE_STL_ALGORITHM_DEFAULT_VALUE_TYPE_VAL  __cpp_lib_algorithm_default_value_type
#else
    #define D_ENV_CPP_FEATURE_STL_ALGORITHM_DEFAULT_VALUE_TYPE_VAL  0L
#endif

// D_ENV_CPP_FEATURE_STL_ALGORITHM_DEFAULT_VALUE_TYPE_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_STL_ALGORITHM_DEFAULT_VALUE_TYPE_VERS  "(C++26)"


// D_ENV_CPP_FEATURE_STL_ASSOCIATIVE_HETEROGENEOUS_INSERTION
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_lib_associative_heterogeneous_insertion
    #define D_ENV_CPP_FEATURE_STL_ASSOCIATIVE_HETEROGENEOUS_INSERTION  1
#else
    #define D_ENV_CPP_FEATURE_STL_ASSOCIATIVE_HETEROGENEOUS_INSERTION  0
#endif

// D_ENV_CPP_FEATURE_STL_ASSOCIATIVE_HETEROGENEOUS_INSERTION_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_STL_ASSOCIATIVE_HETEROGENEOUS_INSERTION_NAME  "__cpp_lib_associative_heterogeneous_insertion"

// D_ENV_CPP_FEATURE_STL_ASSOCIATIVE_HETEROGENEOUS_INSERTION_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_STL_ASSOCIATIVE_HETEROGENEOUS_INSERTION_DESC  "Heterogeneous overloads for associative containers"

// D_ENV_CPP_FEATURE_STL_ASSOCIATIVE_HETEROGENEOUS_INSERTION_VAL
//   constant: feature test value
#ifdef __cpp_lib_associative_heterogeneous_insertion
    #define D_ENV_CPP_FEATURE_STL_ASSOCIATIVE_HETEROGENEOUS_INSERTION_VAL  __cpp_lib_associative_heterogeneous_insertion
#else
    #define D_ENV_CPP_FEATURE_STL_ASSOCIATIVE_HETEROGENEOUS_INSERTION_VAL  0L
#endif

// D_ENV_CPP_FEATURE_STL_ASSOCIATIVE_HETEROGENEOUS_INSERTION_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_STL_ASSOCIATIVE_HETEROGENEOUS_INSERTION_VERS  "(C++26)"


// D_ENV_CPP_FEATURE_STL_ATOMIC_MIN_MAX
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_lib_atomic_min_max
    #define D_ENV_CPP_FEATURE_STL_ATOMIC_MIN_MAX  1
#else
    #define D_ENV_CPP_FEATURE_STL_ATOMIC_MIN_MAX  0
#endif

// D_ENV_CPP_FEATURE_STL_ATOMIC_MIN_MAX_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_STL_ATOMIC_MIN_MAX_NAME  "__cpp_lib_atomic_min_max"

// D_ENV_CPP_FEATURE_STL_ATOMIC_MIN_MAX_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_STL_ATOMIC_MIN_MAX_DESC  "Atomic minimum/maximum"

// D_ENV_CPP_FEATURE_STL_ATOMIC_MIN_MAX_VAL
//   constant: feature test value
#ifdef __cpp_lib_atomic_min_max
    #define D_ENV_CPP_FEATURE_STL_ATOMIC_MIN_MAX_VAL  __cpp_lib_atomic_min_max
#else
    #define D_ENV_CPP_FEATURE_STL_ATOMIC_MIN_MAX_VAL  0L
#endif

// D_ENV_CPP_FEATURE_STL_ATOMIC_MIN_MAX_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_STL_ATOMIC_MIN_MAX_VERS  "(C++26)"


// D_ENV_CPP_FEATURE_STL_CONSTEXPR_ATOMIC
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_lib_constexpr_atomic
    #define D_ENV_CPP_FEATURE_STL_CONSTEXPR_ATOMIC  1
#else
    #define D_ENV_CPP_FEATURE_STL_CONSTEXPR_ATOMIC  0
#endif

// D_ENV_CPP_FEATURE_STL_CONSTEXPR_ATOMIC_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_STL_CONSTEXPR_ATOMIC_NAME  "__cpp_lib_constexpr_atomic"

// D_ENV_CPP_FEATURE_STL_CONSTEXPR_ATOMIC_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_STL_CONSTEXPR_ATOMIC_DESC  "constexpr std::atomic and std::atomic_ref"

// D_ENV_CPP_FEATURE_STL_CONSTEXPR_ATOMIC_VAL
//   constant: feature test value
#ifdef __cpp_lib_constexpr_atomic
    #define D_ENV_CPP_FEATURE_STL_CONSTEXPR_ATOMIC_VAL  __cpp_lib_constexpr_atomic
#else
    #define D_ENV_CPP_FEATURE_STL_CONSTEXPR_ATOMIC_VAL  0L
#endif

// D_ENV_CPP_FEATURE_STL_CONSTEXPR_ATOMIC_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_STL_CONSTEXPR_ATOMIC_VERS  "(C++26)"


// D_ENV_CPP_FEATURE_STL_CONSTEXPR_DEQUE
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_lib_constexpr_deque
    #define D_ENV_CPP_FEATURE_STL_CONSTEXPR_DEQUE  1
#else
    #define D_ENV_CPP_FEATURE_STL_CONSTEXPR_DEQUE  0
#endif

// D_ENV_CPP_FEATURE_STL_CONSTEXPR_DEQUE_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_STL_CONSTEXPR_DEQUE_NAME  "__cpp_lib_constexpr_deque"

// D_ENV_CPP_FEATURE_STL_CONSTEXPR_DEQUE_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_STL_CONSTEXPR_DEQUE_DESC  "constexpr std::deque"

// D_ENV_CPP_FEATURE_STL_CONSTEXPR_DEQUE_VAL
//   constant: feature test value
#ifdef __cpp_lib_constexpr_deque
    #define D_ENV_CPP_FEATURE_STL_CONSTEXPR_DEQUE_VAL  __cpp_lib_constexpr_deque
#else
    #define D_ENV_CPP_FEATURE_STL_CONSTEXPR_DEQUE_VAL  0L
#endif

// D_ENV_CPP_FEATURE_STL_CONSTEXPR_DEQUE_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_STL_CONSTEXPR_DEQUE_VERS  "(C++26)"


// D_ENV_CPP_FEATURE_STL_CONTRACTS
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_lib_contracts
    #define D_ENV_CPP_FEATURE_STL_CONTRACTS  1
#else
    #define D_ENV_CPP_FEATURE_STL_CONTRACTS  0
#endif

// D_ENV_CPP_FEATURE_STL_CONTRACTS_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_STL_CONTRACTS_NAME  "__cpp_lib_contracts"

// D_ENV_CPP_FEATURE_STL_CONTRACTS_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_STL_CONTRACTS_DESC  "<contracts>: Contracts support"

// D_ENV_CPP_FEATURE_STL_CONTRACTS_VAL
//   constant: feature test value
#ifdef __cpp_lib_contracts
    #define D_ENV_CPP_FEATURE_STL_CONTRACTS_VAL  __cpp_lib_contracts
#else
    #define D_ENV_CPP_FEATURE_STL_CONTRACTS_VAL  0L
#endif

// D_ENV_CPP_FEATURE_STL_CONTRACTS_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_STL_CONTRACTS_VERS  "(C++26)"


// D_ENV_CPP_FEATURE_STL_COPYABLE_FUNCTION
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_lib_copyable_function
    #define D_ENV_CPP_FEATURE_STL_COPYABLE_FUNCTION  1
#else
    #define D_ENV_CPP_FEATURE_STL_COPYABLE_FUNCTION  0
#endif

// D_ENV_CPP_FEATURE_STL_COPYABLE_FUNCTION_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_STL_COPYABLE_FUNCTION_NAME  "__cpp_lib_copyable_function"

// D_ENV_CPP_FEATURE_STL_COPYABLE_FUNCTION_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_STL_COPYABLE_FUNCTION_DESC  "std::copyable_function"

// D_ENV_CPP_FEATURE_STL_COPYABLE_FUNCTION_VAL
//   constant: feature test value
#ifdef __cpp_lib_copyable_function
    #define D_ENV_CPP_FEATURE_STL_COPYABLE_FUNCTION_VAL  __cpp_lib_copyable_function
#else
    #define D_ENV_CPP_FEATURE_STL_COPYABLE_FUNCTION_VAL  0L
#endif

// D_ENV_CPP_FEATURE_STL_COPYABLE_FUNCTION_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_STL_COPYABLE_FUNCTION_VERS  "(C++26)"


// D_ENV_CPP_FEATURE_STL_DEBUGGING
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_lib_debugging
    #define D_ENV_CPP_FEATURE_STL_DEBUGGING  1
#else
    #define D_ENV_CPP_FEATURE_STL_DEBUGGING  0
#endif

// D_ENV_CPP_FEATURE_STL_DEBUGGING_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_STL_DEBUGGING_NAME  "__cpp_lib_debugging"

// D_ENV_CPP_FEATURE_STL_DEBUGGING_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_STL_DEBUGGING_DESC  "<debugging>: Debugging support"

// D_ENV_CPP_FEATURE_STL_DEBUGGING_VAL
//   constant: feature test value
#ifdef __cpp_lib_debugging
    #define D_ENV_CPP_FEATURE_STL_DEBUGGING_VAL  __cpp_lib_debugging
#else
    #define D_ENV_CPP_FEATURE_STL_DEBUGGING_VAL  0L
#endif

// D_ENV_CPP_FEATURE_STL_DEBUGGING_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_STL_DEBUGGING_VERS  "(C++26)"


// D_ENV_CPP_FEATURE_STL_FORMAT_PATH
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_lib_format_path
    #define D_ENV_CPP_FEATURE_STL_FORMAT_PATH  1
#else
    #define D_ENV_CPP_FEATURE_STL_FORMAT_PATH  0
#endif

// D_ENV_CPP_FEATURE_STL_FORMAT_PATH_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_STL_FORMAT_PATH_NAME  "__cpp_lib_format_path"

// D_ENV_CPP_FEATURE_STL_FORMAT_PATH_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_STL_FORMAT_PATH_DESC  "Formatting of std::filesystem::path"

// D_ENV_CPP_FEATURE_STL_FORMAT_PATH_VAL
//   constant: feature test value
#ifdef __cpp_lib_format_path
    #define D_ENV_CPP_FEATURE_STL_FORMAT_PATH_VAL  __cpp_lib_format_path
#else
    #define D_ENV_CPP_FEATURE_STL_FORMAT_PATH_VAL  0L
#endif

// D_ENV_CPP_FEATURE_STL_FORMAT_PATH_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_STL_FORMAT_PATH_VERS  "(C++26)"


// D_ENV_CPP_FEATURE_STL_FUNCTION_REF
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_lib_function_ref
    #define D_ENV_CPP_FEATURE_STL_FUNCTION_REF  1
#else
    #define D_ENV_CPP_FEATURE_STL_FUNCTION_REF  0
#endif

// D_ENV_CPP_FEATURE_STL_FUNCTION_REF_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_STL_FUNCTION_REF_NAME  "__cpp_lib_function_ref"

// D_ENV_CPP_FEATURE_STL_FUNCTION_REF_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_STL_FUNCTION_REF_DESC  "std::function_ref: A type-erased callable reference"

// D_ENV_CPP_FEATURE_STL_FUNCTION_REF_VAL
//   constant: feature test value
#ifdef __cpp_lib_function_ref
    #define D_ENV_CPP_FEATURE_STL_FUNCTION_REF_VAL  __cpp_lib_function_ref
#else
    #define D_ENV_CPP_FEATURE_STL_FUNCTION_REF_VAL  0L
#endif

// D_ENV_CPP_FEATURE_STL_FUNCTION_REF_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_STL_FUNCTION_REF_VERS  "(C++26)"


// D_ENV_CPP_FEATURE_STL_HAZARD_POINTER
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_lib_hazard_pointer
    #define D_ENV_CPP_FEATURE_STL_HAZARD_POINTER  1
#else
    #define D_ENV_CPP_FEATURE_STL_HAZARD_POINTER  0
#endif

// D_ENV_CPP_FEATURE_STL_HAZARD_POINTER_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_STL_HAZARD_POINTER_NAME  "__cpp_lib_hazard_pointer"

// D_ENV_CPP_FEATURE_STL_HAZARD_POINTER_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_STL_HAZARD_POINTER_DESC  "<hazard_pointer>: Hazard pointers"

// D_ENV_CPP_FEATURE_STL_HAZARD_POINTER_VAL
//   constant: feature test value
#ifdef __cpp_lib_hazard_pointer
    #define D_ENV_CPP_FEATURE_STL_HAZARD_POINTER_VAL  __cpp_lib_hazard_pointer
#else
    #define D_ENV_CPP_FEATURE_STL_HAZARD_POINTER_VAL  0L
#endif

// D_ENV_CPP_FEATURE_STL_HAZARD_POINTER_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_STL_HAZARD_POINTER_VERS  "(C++26)"


// D_ENV_CPP_FEATURE_STL_HIVE
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_lib_hive
    #define D_ENV_CPP_FEATURE_STL_HIVE  1
#else
    #define D_ENV_CPP_FEATURE_STL_HIVE  0
#endif

// D_ENV_CPP_FEATURE_STL_HIVE_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_STL_HIVE_NAME  "__cpp_lib_hive"

// D_ENV_CPP_FEATURE_STL_HIVE_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_STL_HIVE_DESC  "<hive>: a bucket-based container"

// D_ENV_CPP_FEATURE_STL_HIVE_VAL
//   constant: feature test value
#ifdef __cpp_lib_hive
    #define D_ENV_CPP_FEATURE_STL_HIVE_VAL  __cpp_lib_hive
#else
    #define D_ENV_CPP_FEATURE_STL_HIVE_VAL  0L
#endif

// D_ENV_CPP_FEATURE_STL_HIVE_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_STL_HIVE_VERS  "(C++26)"


// D_ENV_CPP_FEATURE_STL_INPLACE_VECTOR
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_lib_inplace_vector
    #define D_ENV_CPP_FEATURE_STL_INPLACE_VECTOR  1
#else
    #define D_ENV_CPP_FEATURE_STL_INPLACE_VECTOR  0
#endif

// D_ENV_CPP_FEATURE_STL_INPLACE_VECTOR_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_STL_INPLACE_VECTOR_NAME  "__cpp_lib_inplace_vector"

// D_ENV_CPP_FEATURE_STL_INPLACE_VECTOR_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_STL_INPLACE_VECTOR_DESC  "std::inplace_vector"

// D_ENV_CPP_FEATURE_STL_INPLACE_VECTOR_VAL
//   constant: feature test value
#ifdef __cpp_lib_inplace_vector
    #define D_ENV_CPP_FEATURE_STL_INPLACE_VECTOR_VAL  __cpp_lib_inplace_vector
#else
    #define D_ENV_CPP_FEATURE_STL_INPLACE_VECTOR_VAL  0L
#endif

// D_ENV_CPP_FEATURE_STL_INPLACE_VECTOR_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_STL_INPLACE_VECTOR_VERS  "(C++26)"


// D_ENV_CPP_FEATURE_STL_LINALG
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_lib_linalg
    #define D_ENV_CPP_FEATURE_STL_LINALG  1
#else
    #define D_ENV_CPP_FEATURE_STL_LINALG  0
#endif

// D_ENV_CPP_FEATURE_STL_LINALG_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_STL_LINALG_NAME  "__cpp_lib_linalg"

// D_ENV_CPP_FEATURE_STL_LINALG_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_STL_LINALG_DESC  "A free function linear algebra interface based on the BLAS"

// D_ENV_CPP_FEATURE_STL_LINALG_VAL
//   constant: feature test value
#ifdef __cpp_lib_linalg
    #define D_ENV_CPP_FEATURE_STL_LINALG_VAL  __cpp_lib_linalg
#else
    #define D_ENV_CPP_FEATURE_STL_LINALG_VAL  0L
#endif

// D_ENV_CPP_FEATURE_STL_LINALG_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_STL_LINALG_VERS  "(C++26)"


// D_ENV_CPP_FEATURE_STL_POLYMORPHIC
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_lib_polymorphic
    #define D_ENV_CPP_FEATURE_STL_POLYMORPHIC  1
#else
    #define D_ENV_CPP_FEATURE_STL_POLYMORPHIC  0
#endif

// D_ENV_CPP_FEATURE_STL_POLYMORPHIC_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_STL_POLYMORPHIC_NAME  "__cpp_lib_polymorphic"

// D_ENV_CPP_FEATURE_STL_POLYMORPHIC_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_STL_POLYMORPHIC_DESC  "std::polymorphic"

// D_ENV_CPP_FEATURE_STL_POLYMORPHIC_VAL
//   constant: feature test value
#ifdef __cpp_lib_polymorphic
    #define D_ENV_CPP_FEATURE_STL_POLYMORPHIC_VAL  __cpp_lib_polymorphic
#else
    #define D_ENV_CPP_FEATURE_STL_POLYMORPHIC_VAL  0L
#endif

// D_ENV_CPP_FEATURE_STL_POLYMORPHIC_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_STL_POLYMORPHIC_VERS  "(C++26)"


// D_ENV_CPP_FEATURE_STL_RCU
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_lib_rcu
    #define D_ENV_CPP_FEATURE_STL_RCU  1
#else
    #define D_ENV_CPP_FEATURE_STL_RCU  0
#endif

// D_ENV_CPP_FEATURE_STL_RCU_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_STL_RCU_NAME  "__cpp_lib_rcu"

// D_ENV_CPP_FEATURE_STL_RCU_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_STL_RCU_DESC  "<rcu>: Read-Copy Update (RCU)"

// D_ENV_CPP_FEATURE_STL_RCU_VAL
//   constant: feature test value
#ifdef __cpp_lib_rcu
    #define D_ENV_CPP_FEATURE_STL_RCU_VAL  __cpp_lib_rcu
#else
    #define D_ENV_CPP_FEATURE_STL_RCU_VAL  0L
#endif

// D_ENV_CPP_FEATURE_STL_RCU_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_STL_RCU_VERS  "(C++26)"


// D_ENV_CPP_FEATURE_STL_SATURATION_ARITHMETIC
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_lib_saturation_arithmetic
    #define D_ENV_CPP_FEATURE_STL_SATURATION_ARITHMETIC  1
#else
    #define D_ENV_CPP_FEATURE_STL_SATURATION_ARITHMETIC  0
#endif

// D_ENV_CPP_FEATURE_STL_SATURATION_ARITHMETIC_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_STL_SATURATION_ARITHMETIC_NAME  "__cpp_lib_saturation_arithmetic"

// D_ENV_CPP_FEATURE_STL_SATURATION_ARITHMETIC_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_STL_SATURATION_ARITHMETIC_DESC  "Saturation arithmetic"

// D_ENV_CPP_FEATURE_STL_SATURATION_ARITHMETIC_VAL
//   constant: feature test value
#ifdef __cpp_lib_saturation_arithmetic
    #define D_ENV_CPP_FEATURE_STL_SATURATION_ARITHMETIC_VAL  __cpp_lib_saturation_arithmetic
#else
    #define D_ENV_CPP_FEATURE_STL_SATURATION_ARITHMETIC_VAL  0L
#endif

// D_ENV_CPP_FEATURE_STL_SATURATION_ARITHMETIC_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_STL_SATURATION_ARITHMETIC_VERS  "(C++26)"


// D_ENV_CPP_FEATURE_STL_SENDERS
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_lib_senders
    #define D_ENV_CPP_FEATURE_STL_SENDERS  1
#else
    #define D_ENV_CPP_FEATURE_STL_SENDERS  0
#endif

// D_ENV_CPP_FEATURE_STL_SENDERS_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_STL_SENDERS_NAME  "__cpp_lib_senders"

// D_ENV_CPP_FEATURE_STL_SENDERS_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_STL_SENDERS_DESC  "std::execution: Sender-receiver model"

// D_ENV_CPP_FEATURE_STL_SENDERS_VAL
//   constant: feature test value
#ifdef __cpp_lib_senders
    #define D_ENV_CPP_FEATURE_STL_SENDERS_VAL  __cpp_lib_senders
#else
    #define D_ENV_CPP_FEATURE_STL_SENDERS_VAL  0L
#endif

// D_ENV_CPP_FEATURE_STL_SENDERS_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_STL_SENDERS_VERS  "(C++26)"


// D_ENV_CPP_FEATURE_STL_SIMD
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_lib_simd
    #define D_ENV_CPP_FEATURE_STL_SIMD  1
#else
    #define D_ENV_CPP_FEATURE_STL_SIMD  0
#endif

// D_ENV_CPP_FEATURE_STL_SIMD_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_STL_SIMD_NAME  "__cpp_lib_simd"

// D_ENV_CPP_FEATURE_STL_SIMD_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_STL_SIMD_DESC  "<simd>: Data-parallel types"

// D_ENV_CPP_FEATURE_STL_SIMD_VAL
//   constant: feature test value
#ifdef __cpp_lib_simd
    #define D_ENV_CPP_FEATURE_STL_SIMD_VAL  __cpp_lib_simd
#else
    #define D_ENV_CPP_FEATURE_STL_SIMD_VAL  0L
#endif

// D_ENV_CPP_FEATURE_STL_SIMD_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_STL_SIMD_VERS  "(C++26)"


// D_ENV_CPP_FEATURE_STL_TEXT_ENCODING
//   constant: feature enabled flag (1 = enabled, 0 = disabled)
#ifdef __cpp_lib_text_encoding
    #define D_ENV_CPP_FEATURE_STL_TEXT_ENCODING  1
#else
    #define D_ENV_CPP_FEATURE_STL_TEXT_ENCODING  0
#endif

// D_ENV_CPP_FEATURE_STL_TEXT_ENCODING_NAME
//   constant: feature macro name
#define D_ENV_CPP_FEATURE_STL_TEXT_ENCODING_NAME  "__cpp_lib_text_encoding"

// D_ENV_CPP_FEATURE_STL_TEXT_ENCODING_DESC
//   constant: feature description
#define D_ENV_CPP_FEATURE_STL_TEXT_ENCODING_DESC  "std::text_encoding"

// D_ENV_CPP_FEATURE_STL_TEXT_ENCODING_VAL
//   constant: feature test value
#ifdef __cpp_lib_text_encoding
    #define D_ENV_CPP_FEATURE_STL_TEXT_ENCODING_VAL  __cpp_lib_text_encoding
#else
    #define D_ENV_CPP_FEATURE_STL_TEXT_ENCODING_VAL  0L
#endif

// D_ENV_CPP_FEATURE_STL_TEXT_ENCODING_VERS
//   constant: C++ standard version
#define D_ENV_CPP_FEATURE_STL_TEXT_ENCODING_VERS  "(C++26)"


// =============================================================================
// III. AGGREGATE FEATURE CHECKS
// =============================================================================

// D_ENV_CPP_FEATURE_HAS_ALL_LANG_CPP11
//   constant: 1 if all C++11 language features available
#define D_ENV_CPP_FEATURE_HAS_ALL_LANG_CPP11                             \
    ( D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES                         &&  \
      D_ENV_CPP_FEATURE_LANG_ATTRIBUTES                              &&  \
      D_ENV_CPP_FEATURE_LANG_CONSTEXPR                               &&  \
      D_ENV_CPP_FEATURE_LANG_DECLTYPE                                &&  \
      D_ENV_CPP_FEATURE_LANG_DELEGATING_CONSTRUCTORS                 &&  \
      D_ENV_CPP_FEATURE_LANG_INHERITING_CONSTRUCTORS                 &&  \
      D_ENV_CPP_FEATURE_LANG_INITIALIZER_LISTS                       &&  \
      D_ENV_CPP_FEATURE_LANG_LAMBDAS                                 &&  \
      D_ENV_CPP_FEATURE_LANG_NSDMI                                   &&  \
      D_ENV_CPP_FEATURE_LANG_RANGE_BASED_FOR                         &&  \
      D_ENV_CPP_FEATURE_LANG_RAW_STRINGS                             &&  \
      D_ENV_CPP_FEATURE_LANG_REF_QUALIFIERS                          &&  \
      D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES                       &&  \
      D_ENV_CPP_FEATURE_LANG_STATIC_ASSERT                           &&  \
      D_ENV_CPP_FEATURE_LANG_THREADSAFE_STATIC_INIT                  &&  \
      D_ENV_CPP_FEATURE_LANG_UNICODE_CHARACTERS                      &&  \
      D_ENV_CPP_FEATURE_LANG_UNICODE_LITERALS                        &&  \
      D_ENV_CPP_FEATURE_LANG_USER_DEFINED_LITERALS                   &&  \
      D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES )                    
                                                                     
// D_ENV_CPP_FEATURE_HAS_ALL_CPP11                                   
//   constant: 1 if all C++11 features available                     
#define D_ENV_CPP_FEATURE_HAS_ALL_CPP11                                  \
    ( D_ENV_CPP_FEATURE_HAS_ALL_LANG_CPP11 )                         
                                                                     
                                                                     
// D_ENV_CPP_FEATURE_HAS_ALL_LANG_CPP14                              
//   constant: 1 if all C++14 language features available            
#define D_ENV_CPP_FEATURE_HAS_ALL_LANG_CPP14                             \
    ( D_ENV_CPP_FEATURE_LANG_AGGREGATE_NSDMI                         &&  \
      D_ENV_CPP_FEATURE_LANG_BINARY_LITERALS                         &&  \
      D_ENV_CPP_FEATURE_LANG_DECLTYPE_AUTO                           &&  \
      D_ENV_CPP_FEATURE_LANG_ENUMERATOR_ATTRIBUTES                   &&  \
      D_ENV_CPP_FEATURE_LANG_GENERIC_LAMBDAS                         &&  \
      D_ENV_CPP_FEATURE_LANG_INIT_CAPTURES                           &&  \
      D_ENV_CPP_FEATURE_LANG_NAMESPACE_ATTRIBUTES                    &&  \
      D_ENV_CPP_FEATURE_LANG_NONTYPE_TEMPLATE_ARGS                   &&  \
      D_ENV_CPP_FEATURE_LANG_RETURN_TYPE_DEDUCTION                   &&  \
      D_ENV_CPP_FEATURE_LANG_SIZED_DEALLOCATION                      &&  \
      D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES )                    
                                                                     
// D_ENV_CPP_FEATURE_HAS_ALL_STL_CPP14                               
//   constant: 1 if all C++14 library features available             
#define D_ENV_CPP_FEATURE_HAS_ALL_STL_CPP14                              \
    ( D_ENV_CPP_FEATURE_STL_CHRONO_UDLS                              &&  \
      D_ENV_CPP_FEATURE_STL_COMPLEX_UDLS )                           
                                                                     
// D_ENV_CPP_FEATURE_HAS_ALL_CPP14                                   
//   constant: 1 if all C++14 features available                     
#define D_ENV_CPP_FEATURE_HAS_ALL_CPP14                                  \
    ( D_ENV_CPP_FEATURE_HAS_ALL_LANG_CPP14                           &&  \
      D_ENV_CPP_FEATURE_HAS_ALL_STL_CPP14 )                          
                                                                     
// D_ENV_CPP_FEATURE_HAS_ALL_LANG_CPP17                              
//   constant: 1 if all C++17 language features available            
#define D_ENV_CPP_FEATURE_HAS_ALL_LANG_CPP17                             \
    ( D_ENV_CPP_FEATURE_LANG_AGGREGATE_BASES                         &&  \
      D_ENV_CPP_FEATURE_LANG_ALIGNED_NEW                             &&  \
      D_ENV_CPP_FEATURE_LANG_CAPTURE_STAR_THIS                       &&  \
      D_ENV_CPP_FEATURE_LANG_CONSTEXPR_IN_DECLTYPE                   &&  \
      D_ENV_CPP_FEATURE_LANG_DEDUCTION_GUIDES                        &&  \
      D_ENV_CPP_FEATURE_LANG_FOLD_EXPRESSIONS                        &&  \
      D_ENV_CPP_FEATURE_LANG_GUARANTEED_COPY_ELISION                 &&  \
      D_ENV_CPP_FEATURE_LANG_HEX_FLOAT                               &&  \
      D_ENV_CPP_FEATURE_LANG_IF_CONSTEXPR                            &&  \
      D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES                        &&  \
      D_ENV_CPP_FEATURE_LANG_NOEXCEPT_FUNCTION_TYPE                  &&  \
      D_ENV_CPP_FEATURE_LANG_NONTYPE_TEMPLATE_PARAMETER_AUTO         &&  \
      D_ENV_CPP_FEATURE_LANG_STRUCTURED_BINDINGS                     &&  \
      D_ENV_CPP_FEATURE_LANG_TEMPLATE_TEMPLATE_ARGS                  &&  \
      D_ENV_CPP_FEATURE_LANG_VARIADIC_USING )                        
                                                                     
// D_ENV_CPP_FEATURE_HAS_ALL_STL_CPP17                               
//   constant: 1 if all C++17 library features available             
#define D_ENV_CPP_FEATURE_HAS_ALL_STL_CPP17                              \
    ( D_ENV_CPP_FEATURE_STL_ADDRESSOF_CONSTEXPR                      &&  \
      D_ENV_CPP_FEATURE_STL_ANY                                      &&  \
      D_ENV_CPP_FEATURE_STL_APPLY                                    &&  \
      D_ENV_CPP_FEATURE_STL_ARRAY_CONSTEXPR                          &&  \
      D_ENV_CPP_FEATURE_STL_AS_CONST                                 &&  \
      D_ENV_CPP_FEATURE_STL_BOOL_CONSTANT                            &&  \
      D_ENV_CPP_FEATURE_STL_BOYER_MOORE_SEARCHER                     &&  \
      D_ENV_CPP_FEATURE_STL_BYTE                                     &&  \
      D_ENV_CPP_FEATURE_STL_CLAMP                                    &&  \
      D_ENV_CPP_FEATURE_STL_FILESYSTEM                               &&  \
      D_ENV_CPP_FEATURE_STL_OPTIONAL                                 &&  \
      D_ENV_CPP_FEATURE_STL_VARIANT )                                
                                                                     
// D_ENV_CPP_FEATURE_HAS_ALL_CPP17                                   
//   constant: 1 if all C++17 features available                     
#define D_ENV_CPP_FEATURE_HAS_ALL_CPP17                                  \
    ( D_ENV_CPP_FEATURE_HAS_ALL_LANG_CPP17                           &&  \
      D_ENV_CPP_FEATURE_HAS_ALL_STL_CPP17 )                          
                                                                     
                                                                     
// D_ENV_CPP_FEATURE_HAS_ALL_LANG_CPP20                              
//   constant: 1 if all C++20 language features available            
#define D_ENV_CPP_FEATURE_HAS_ALL_LANG_CPP20                             \
    ( D_ENV_CPP_FEATURE_LANG_AGGREGATE_PAREN_INIT                    &&  \
      D_ENV_CPP_FEATURE_LANG_CHAR8_T                                 &&  \
      D_ENV_CPP_FEATURE_LANG_CONCEPTS                                &&  \
      D_ENV_CPP_FEATURE_LANG_CONDITIONAL_EXPLICIT                    &&  \
      D_ENV_CPP_FEATURE_LANG_CONSTEVAL                               &&  \
      D_ENV_CPP_FEATURE_LANG_CONSTEXPR_DYNAMIC_ALLOC                 &&  \
      D_ENV_CPP_FEATURE_LANG_CONSTINIT                               &&  \
      D_ENV_CPP_FEATURE_LANG_DESIGNATED_INITIALIZERS                 &&  \
      D_ENV_CPP_FEATURE_LANG_IMPL_COROUTINE                          &&  \
      D_ENV_CPP_FEATURE_LANG_IMPL_DESTROYING_DELETE                  &&  \
      D_ENV_CPP_FEATURE_LANG_IMPL_THREE_WAY_COMPARISON               &&  \
      D_ENV_CPP_FEATURE_LANG_MODULES                                 &&  \
      D_ENV_CPP_FEATURE_LANG_USING_ENUM )                            
                                                                     
// D_ENV_CPP_FEATURE_HAS_ALL_STL_CPP20                               
//   constant: 1 if all C++20 library features available             
#define D_ENV_CPP_FEATURE_HAS_ALL_STL_CPP20                              \
    ( D_ENV_CPP_FEATURE_STL_ASSUME_ALIGNED                           &&  \
      D_ENV_CPP_FEATURE_STL_ATOMIC_FLAG_TEST                         &&  \
      D_ENV_CPP_FEATURE_STL_ATOMIC_FLOAT                             &&  \
      D_ENV_CPP_FEATURE_STL_ATOMIC_REF                               &&  \
      D_ENV_CPP_FEATURE_STL_ATOMIC_WAIT                              &&  \
      D_ENV_CPP_FEATURE_STL_BARRIER                                  &&  \
      D_ENV_CPP_FEATURE_STL_BIND_FRONT                               &&  \
      D_ENV_CPP_FEATURE_STL_BIT_CAST                                 &&  \
      D_ENV_CPP_FEATURE_STL_BITOPS                                   &&  \
      D_ENV_CPP_FEATURE_STL_BOUNDED_ARRAY_TRAITS                     &&  \
      D_ENV_CPP_FEATURE_STL_CHAR8_T                                  &&  \
      D_ENV_CPP_FEATURE_STL_CONCEPTS                                 &&  \
      D_ENV_CPP_FEATURE_STL_CONSTEXPR_ALGORITHMS                     &&  \
      D_ENV_CPP_FEATURE_STL_CONSTEXPR_COMPLEX                        &&  \
      D_ENV_CPP_FEATURE_STL_CONSTEXPR_DYNAMIC_ALLOC                  &&  \
      D_ENV_CPP_FEATURE_STL_CONSTEXPR_STRING                         &&  \
      D_ENV_CPP_FEATURE_STL_CONSTEXPR_VECTOR                         &&  \
      D_ENV_CPP_FEATURE_STL_COROUTINE                                &&  \
      D_ENV_CPP_FEATURE_STL_ENDIAN                                   &&  \
      D_ENV_CPP_FEATURE_STL_FORMAT                                   &&  \
      D_ENV_CPP_FEATURE_STL_JTHREAD                                  &&  \
      D_ENV_CPP_FEATURE_STL_LATCH                                    &&  \
      D_ENV_CPP_FEATURE_STL_MATH_CONSTANTS                           &&  \
      D_ENV_CPP_FEATURE_STL_RANGES                                   &&  \
      D_ENV_CPP_FEATURE_STL_SEMAPHORE                                &&  \
      D_ENV_CPP_FEATURE_STL_SOURCE_LOCATION                          &&  \
      D_ENV_CPP_FEATURE_STL_SPAN                                     &&  \
      D_ENV_CPP_FEATURE_STL_THREE_WAY_COMPARISON                     &&  \
      D_ENV_CPP_FEATURE_STL_TO_ARRAY )                               
                                                                     
// D_ENV_CPP_FEATURE_HAS_ALL_CPP20                                   
//   constant: 1 if all C++20 features available                     
#define D_ENV_CPP_FEATURE_HAS_ALL_CPP20                                  \
    ( D_ENV_CPP_FEATURE_HAS_ALL_LANG_CPP20                           &&  \
      D_ENV_CPP_FEATURE_HAS_ALL_STL_CPP20 )                          
                                                                     
// D_ENV_CPP_FEATURE_HAS_ALL_LANG_CPP23                              
//   constant: 1 if all C++23 language features available            
#define D_ENV_CPP_FEATURE_HAS_ALL_LANG_CPP23                             \
    ( D_ENV_CPP_FEATURE_LANG_AUTO_CAST                               &&  \
      D_ENV_CPP_FEATURE_LANG_EXPLICIT_THIS_PARAMETER                 &&  \
      D_ENV_CPP_FEATURE_LANG_IF_CONSTEVAL                            &&  \
      D_ENV_CPP_FEATURE_LANG_IMPLICIT_MOVE                           &&  \
      D_ENV_CPP_FEATURE_LANG_MULTIDIMENSIONAL_SUBSCRIPT              &&  \
      D_ENV_CPP_FEATURE_LANG_NAMED_CHARACTER_ESCAPES                 &&  \
      D_ENV_CPP_FEATURE_LANG_SIZE_T_SUFFIX                           &&  \
      D_ENV_CPP_FEATURE_LANG_STATIC_CALL_OPERATOR )                  
                                                                     
// D_ENV_CPP_FEATURE_HAS_ALL_STL_CPP23                               
//   constant: 1 if all C++23 library features available             
#define D_ENV_CPP_FEATURE_HAS_ALL_STL_CPP23                              \
    ( D_ENV_CPP_FEATURE_STL_ADAPTOR_ITERATOR_PAIR_CONSTRUCTOR        &&  \
      D_ENV_CPP_FEATURE_STL_ASSOCIATIVE_HETEROGENEOUS_ERASURE        &&  \
      D_ENV_CPP_FEATURE_STL_BIND_BACK                                &&  \
      D_ENV_CPP_FEATURE_STL_BYTESWAP                                 &&  \
      D_ENV_CPP_FEATURE_STL_CONSTEXPR_BITSET                         &&  \
      D_ENV_CPP_FEATURE_STL_CONSTEXPR_CHARCONV                       &&  \
      D_ENV_CPP_FEATURE_STL_CONSTEXPR_CMATH                          &&  \
      D_ENV_CPP_FEATURE_STL_EXPECTED                                 &&  \
      D_ENV_CPP_FEATURE_STL_FLAT_MAP                                 &&  \
      D_ENV_CPP_FEATURE_STL_FLAT_SET                                 &&  \
      D_ENV_CPP_FEATURE_STL_GENERATOR                                &&  \
      D_ENV_CPP_FEATURE_STL_MDSPAN                                   &&  \
      D_ENV_CPP_FEATURE_STL_MOVE_ONLY_FUNCTION                       &&  \
      D_ENV_CPP_FEATURE_STL_PRINT                                    &&  \
      D_ENV_CPP_FEATURE_STL_RANGES_TO_CONTAINER                      &&  \
      D_ENV_CPP_FEATURE_STL_SPANSTREAM                               &&  \
      D_ENV_CPP_FEATURE_STL_STACKTRACE                               &&  \
      D_ENV_CPP_FEATURE_STL_STDATOMIC_H                              &&  \
      D_ENV_CPP_FEATURE_STL_STRING_CONTAINS                          &&  \
      D_ENV_CPP_FEATURE_STL_STRING_RESIZE_AND_OVERWRITE              &&  \
      D_ENV_CPP_FEATURE_STL_UNREACHABLE )                            
                                                                     
// D_ENV_CPP_FEATURE_HAS_ALL_CPP23                                   
//   constant: 1 if all C++23 features available                     
#define D_ENV_CPP_FEATURE_HAS_ALL_CPP23                                  \
    ( D_ENV_CPP_FEATURE_HAS_ALL_LANG_CPP23                           &&  \
      D_ENV_CPP_FEATURE_HAS_ALL_STL_CPP23 )                          
                                                                     
                                                                     
// D_ENV_CPP_FEATURE_HAS_ALL_LANG_CPP26                              
//   constant: 1 if all C++26 language features available            
#define D_ENV_CPP_FEATURE_HAS_ALL_LANG_CPP26                             \
    ( D_ENV_CPP_FEATURE_LANG_CONSTEXPR_EXCEPTIONS                    &&  \
      D_ENV_CPP_FEATURE_LANG_CONTRACTS                               &&  \
      D_ENV_CPP_FEATURE_LANG_DELETED_FUNCTION                        &&  \
      D_ENV_CPP_FEATURE_LANG_PACK_INDEXING                           &&  \
      D_ENV_CPP_FEATURE_LANG_PLACEHOLDER_VARIABLES                   &&  \
      D_ENV_CPP_FEATURE_LANG_PP_EMBED                                &&  \
      D_ENV_CPP_FEATURE_LANG_TEMPLATE_PARAMETERS                     &&  \
      D_ENV_CPP_FEATURE_LANG_TRIVIAL_RELOCATABILITY                  &&  \
      D_ENV_CPP_FEATURE_LANG_TRIVIAL_UNION                           &&  \
      D_ENV_CPP_FEATURE_LANG_VARIADIC_FRIEND )                       
                                                                     
// D_ENV_CPP_FEATURE_HAS_ALL_STL_CPP26                               
//   constant: 1 if all C++26 library features available             
#define D_ENV_CPP_FEATURE_HAS_ALL_STL_CPP26                              \
    ( D_ENV_CPP_FEATURE_STL_ALGORITHM_DEFAULT_VALUE_TYPE             &&  \
      D_ENV_CPP_FEATURE_STL_ASSOCIATIVE_HETEROGENEOUS_INSERTION      &&  \
      D_ENV_CPP_FEATURE_STL_ATOMIC_MIN_MAX                           &&  \
      D_ENV_CPP_FEATURE_STL_CONSTEXPR_ATOMIC                         &&  \
      D_ENV_CPP_FEATURE_STL_CONSTEXPR_DEQUE                          &&  \
      D_ENV_CPP_FEATURE_STL_CONTRACTS                                &&  \
      D_ENV_CPP_FEATURE_STL_COPYABLE_FUNCTION                        &&  \
      D_ENV_CPP_FEATURE_STL_DEBUGGING                                &&  \
      D_ENV_CPP_FEATURE_STL_FORMAT_PATH                              &&  \
      D_ENV_CPP_FEATURE_STL_FUNCTION_REF                             &&  \
      D_ENV_CPP_FEATURE_STL_HAZARD_POINTER                           &&  \
      D_ENV_CPP_FEATURE_STL_HIVE                                     &&  \
      D_ENV_CPP_FEATURE_STL_INPLACE_VECTOR                           &&  \
      D_ENV_CPP_FEATURE_STL_LINALG                                   &&  \
      D_ENV_CPP_FEATURE_STL_POLYMORPHIC                              &&  \
      D_ENV_CPP_FEATURE_STL_RCU                                      &&  \
      D_ENV_CPP_FEATURE_STL_SATURATION_ARITHMETIC                    &&  \
      D_ENV_CPP_FEATURE_STL_SENDERS                                  &&  \
      D_ENV_CPP_FEATURE_STL_SIMD                                     &&  \
      D_ENV_CPP_FEATURE_STL_TEXT_ENCODING )                          
                                                                     
// D_ENV_CPP_FEATURE_HAS_ALL_CPP26                                   
//   constant: 1 if all C++26 features available                     
#define D_ENV_CPP_FEATURE_HAS_ALL_CPP26                                  \
    ( D_ENV_CPP_FEATURE_HAS_ALL_LANG_CPP26                           &&  \
      D_ENV_CPP_FEATURE_HAS_ALL_STL_CPP26 )


#endif  // DJINTERP_ENV_CPP_FEATURES_
