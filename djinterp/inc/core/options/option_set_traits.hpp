/******************************************************************************
* djinterp [options]                                    option_set_traits.hpp
*
* SFINAE-based traits for option entries, option_set containers, and
* options-configurable target types.
*   Provides compile-time detection and classification of option entry
* capabilities, option_set interface compliance, and target-level
* configurability.  Modelled on the same two-tier architecture as
* registry_traits.hpp (entry-level + container-level), extended with
* a third tier for detecting whether an arbitrary type can accept,
* export, or parse options.
*
*   All detection is purely structural: no tag types are required.  Option
* entries declare capabilities through member presence — a plain struct
* with .key, .value, and optional columns (description, default_value,
* short_name, required, etc.) is all that is needed.  Target types
* declare configurability through method presence — apply_options,
* configure, set_option, to_options, parse_args, and so on.  Users
* extend the system by adding their own members and writing matching
* expression aliases following the pattern in Section I and XII.
*
*   Uses the void_t / detector SFINAE idiom from type_traits.hpp.
*
*
* TABLE OF CONTENTS
* =================
* I.      ENTRY-LEVEL EXPRESSION ALIASES
*         1.  key_expr                (.key member)
*         2.  value_expr              (.value member)
*         3.  default_value_expr      (.default_value member)
*         4.  description_expr        (.description member)
*         5.  short_name_expr         (.short_name member)
*         6.  long_name_expr          (.long_name member)
*         7.  alias_expr              (.alias member)
*         8.  required_expr           (.required member)
*         9.  hidden_expr             (.hidden member)
*         10. deprecated_expr         (.deprecated member)
*         11. category_expr           (.category member)
*         12. env_var_expr            (.env_var member)
*         13. choices_expr            (.choices member)
*         14. validate_expr           (.validate callable)
*         15. min_value_expr          (.min_value member)
*         16. max_value_expr          (.max_value member)
*         17. step_value_expr         (.step member)
*         18. help_text_expr          (.help_text member)
*
* II.     ENTRY-LEVEL has_* TRAITS
*
* III.    ENTRY TYPE EXTRACTION
*         1.  option_key_t            (key type or nonesuch)
*         2.  option_value_t          (value type or nonesuch)
*         3.  option_default_t        (default_value type or nonesuch)
*         4.  option_description_t    (description type or nonesuch)
*         5.  option_short_name_t     (short_name type or nonesuch)
*         6.  option_category_t       (category type or nonesuch)
*         7.  option_env_var_t        (env_var type or nonesuch)
*         8.  option_choices_t        (choices type or nonesuch)
*         9.  option_step_t           (step type or nonesuch)
*
* IV.     KEY TYPE CLASSIFICATION
*         1.  has_string_key          (key is string-like)
*         2.  has_enum_key            (key is an enumeration)
*         3.  has_scoped_enum_key     (key is a scoped enum)
*         4.  has_integral_key        (key is integral)
*         5.  has_comparable_key      (key supports operator<)
*         6.  has_equality_key        (key supports operator==)
*
* V.      VALUE TYPE CLASSIFICATION
*         1.  has_boolean_value       (value is bool)
*         2.  has_arithmetic_value    (value is arithmetic)
*         3.  has_string_value        (value is string-like)
*
* VI.     COMPOSITE ENTRY TRAITS
*         1.  is_option_entry         (key + value)
*         2.  is_named_option         (has short_name or long_name)
*         3.  is_documented_option    (has description or help_text)
*         4.  is_defaultable_option   (has default_value)
*         5.  is_required_option      (has required member)
*         6.  is_constrained_option   (has validate or bounds or choices)
*         7.  is_bounded_option       (has min_value and max_value)
*         8.  is_stepped_option      (bounded + step)
*         9.  is_categorized_option   (has category member)
*         10. is_env_mapped_option    (has env_var member)
*         11. is_fully_documented     (description + default + named)
*         12. is_self_validating      (validate + bounded)
*         13. option_known_column_count (count of detected columns)
*
* VII.    ENTRY COMPATIBILITY TRAITS
*         1.  options_share_key_type      (same key type)
*         2.  options_share_value_type    (same value type)
*         3.  options_are_compatible      (same key + value types)
*         4.  option_is_superset_of       (all columns of _Sub + more)
*
* VIII.   CONTAINER-LEVEL EXPRESSION ALIASES
*
* IX.     CONTAINER-LEVEL has_* TRAITS
*
* X.      COMPOSITE CONTAINER TRAITS
*         1.  is_option_set               (minimum contract)
*         2.  is_iterable_option_set      (begin + end)
*         3.  is_searchable_option_set    (find + contains)
*         4.  is_static_option_set        (has backing_array)
*         5.  is_dynamic_option_set       (no backing_array)
*         6.  is_sorted_option_set        (has binary search)
*
* XI.     CONVENIENCE ALIASES (sections I-X)
*
* XII.    TARGET-LEVEL EXPRESSION ALIASES
*         A.  unparameterized (single _T)
*             1.  to_options_method_expr
*             2.  get_options_method_expr
*             3.  help_string_method_expr
*             4.  parse_args_method_expr
*             5.  parse_option_method_expr
*             6.  parse_config_method_expr
*         B.  parameterized (multi-type)
*             7.  apply_options_method_expr<_T, _OptionSet>
*             8.  configure_method_expr<_T, _OptionSet>
*             9.  set_option_method_expr<_T, _Key, _Value>
*
* XIII.   TARGET-LEVEL has_* TRAITS
*
* XIV.    CONFIGURE STRATEGY
*         1.  configure_strategy    (enum)
*         2.  option_configure_strategy   (trait)
*
* XV.     COMPOSITE TARGET TRAITS
*         1.  is_options_configurable     (can accept options)
*         2.  is_option_producing         (can export options)
*         3.  is_option_parseable         (can parse CLI/config)
*         4.  is_option_describable       (has help_string)
*
* XVI.    TARGET-LEVEL CONVENIENCE ALIASES
*
*
* path:      /inc/options/option_set_traits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.XX.XX
******************************************************************************/

#ifndef DJINTERP_OPTION_SET_TRAITS_
#define DJINTERP_OPTION_SET_TRAITS_ 1

#include <cstddef>
#include <string>
#include <type_traits>
#include "../djinterp.hpp"
#include "../type_traits.hpp"

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    #include <string_view>
#endif


NS_DJINTERP


///////////////////////////////////////////////////////////////////////////////
///        I.    ENTRY-LEVEL EXPRESSION ALIASES                             ///
///////////////////////////////////////////////////////////////////////////////
// Each alias attempts to form a type from a member access
// expression on _T.  If the member does not exist, the
// expression is ill-formed and the detector maps it to
// false_type / nonesuch.  Users add new column detectors by
// following this pattern with their own member names.

NS_INTERNAL

    // ----------------------------------------------------------------
    // mandatory base members
    // ----------------------------------------------------------------

    // key_expr
    //   detector: expression alias for .key member.
    template<typename _T>
    using key_expr =
        decltype(std::declval<const _T&>().key);

    // value_expr
    //   detector: expression alias for .value member.
    template<typename _T>
    using value_expr =
        decltype(std::declval<const _T&>().value);

    // ----------------------------------------------------------------
    // optional column members — identity and naming
    // ----------------------------------------------------------------

    // short_name_expr
    //   detector: expression alias for .short_name member.
    // Typically a single character or short string used as
    // a CLI flag abbreviation (e.g. 'v' for --verbose).
    template<typename _T>
    using short_name_expr =
        decltype(std::declval<const _T&>().short_name);

    // long_name_expr
    //   detector: expression alias for .long_name member.
    // Full-length option identifier (e.g. "verbose").
    template<typename _T>
    using long_name_expr =
        decltype(std::declval<const _T&>().long_name);

    // alias_expr
    //   detector: expression alias for .alias member.
    // Alternative name(s) for the option.
    template<typename _T>
    using alias_expr =
        decltype(std::declval<const _T&>().alias);

    // ----------------------------------------------------------------
    // optional column members — documentation
    // ----------------------------------------------------------------

    // description_expr
    //   detector: expression alias for .description member.
    template<typename _T>
    using description_expr =
        decltype(std::declval<const _T&>().description);

    // help_text_expr
    //   detector: expression alias for .help_text member.
    // Extended help beyond the one-line description.
    template<typename _T>
    using help_text_expr =
        decltype(std::declval<const _T&>().help_text);

    // category_expr
    //   detector: expression alias for .category member.
    // Grouping label for help-text sections.
    template<typename _T>
    using category_expr =
        decltype(std::declval<const _T&>().category);

    // ----------------------------------------------------------------
    // optional column members — defaults and constraints
    // ----------------------------------------------------------------

    // default_value_expr
    //   detector: expression alias for .default_value member.
    template<typename _T>
    using default_value_expr =
        decltype(std::declval<const _T&>().default_value);

    // required_expr
    //   detector: expression alias for .required member.
    template<typename _T>
    using required_expr =
        decltype(std::declval<const _T&>().required);

    // min_value_expr
    //   detector: expression alias for .min_value member.
    template<typename _T>
    using min_value_expr =
        decltype(std::declval<const _T&>().min_value);

    // max_value_expr
    //   detector: expression alias for .max_value member.
    template<typename _T>
    using max_value_expr =
        decltype(std::declval<const _T&>().max_value);

    // step_value_expr
    //   detector: expression alias for .step member.
    // Defines the increment granularity between min_value
    // and max_value for bounded options.
    template<typename _T>
    using step_value_expr =
        decltype(std::declval<const _T&>().step);

    // choices_expr
    //   detector: expression alias for .choices member.
    // A constrained set of valid values (e.g. array, span,
    // or iterable of permitted values).
    template<typename _T>
    using choices_expr =
        decltype(std::declval<const _T&>().choices);

    // validate_expr
    //   detector: expression alias detecting a .validate
    // callable that accepts the entry's own value type by
    // const ref.  Ill-formed when either .value or .validate
    // is absent.
    template<typename _T>
    using validate_expr = decltype(
        std::declval<const _T&>().validate(
            std::declval<
                const decltype(
                    std::declval<const _T&>().value)&>()));

    // ----------------------------------------------------------------
    // optional column members — visibility and lifecycle
    // ----------------------------------------------------------------

    // hidden_expr
    //   detector: expression alias for .hidden member.
    // When true, the option is omitted from help output.
    template<typename _T>
    using hidden_expr =
        decltype(std::declval<const _T&>().hidden);

    // deprecated_expr
    //   detector: expression alias for .deprecated member.
    // Marks the option as deprecated with an optional
    // replacement message.
    template<typename _T>
    using deprecated_expr =
        decltype(std::declval<const _T&>().deprecated);

    // ----------------------------------------------------------------
    // optional column members — environment mapping
    // ----------------------------------------------------------------

    // env_var_expr
    //   detector: expression alias for .env_var member.
    // Environment variable name from which this option's
    // value may be sourced as a fallback.
    template<typename _T>
    using env_var_expr =
        decltype(std::declval<const _T&>().env_var);

NS_END  // internal


///////////////////////////////////////////////////////////////////////////////
///        II.   ENTRY-LEVEL has_* TRAITS                                   ///
///////////////////////////////////////////////////////////////////////////////

// ----------------------------------------------------------------
// mandatory base members
// ----------------------------------------------------------------

// has_key
//   trait: detects whether _T has a .key member.
template<typename _T>
struct has_key
{
    static constexpr bool value =
        is_detected<internal::key_expr, _T>::value;
};

// has_value
//   trait: detects whether _T has a .value member.
template<typename _T>
struct has_value
{
    static constexpr bool value =
        is_detected<internal::value_expr, _T>::value;
};

// ----------------------------------------------------------------
// identity and naming
// ----------------------------------------------------------------

// has_short_name
//   trait: detects whether _T has a .short_name member.
template<typename _T>
struct has_short_name
{
    static constexpr bool value =
        is_detected<internal::short_name_expr, _T>::value;
};

// has_long_name
//   trait: detects whether _T has a .long_name member.
template<typename _T>
struct has_long_name
{
    static constexpr bool value =
        is_detected<internal::long_name_expr, _T>::value;
};

// has_alias
//   trait: detects whether _T has an .alias member.
template<typename _T>
struct has_alias
{
    static constexpr bool value =
        is_detected<internal::alias_expr, _T>::value;
};

// ----------------------------------------------------------------
// documentation
// ----------------------------------------------------------------

// has_description
//   trait: detects whether _T has a .description member.
template<typename _T>
struct has_description
{
    static constexpr bool value =
        is_detected<internal::description_expr, _T>::value;
};

// has_help_text
//   trait: detects whether _T has a .help_text member.
template<typename _T>
struct has_help_text
{
    static constexpr bool value =
        is_detected<internal::help_text_expr, _T>::value;
};

// has_category
//   trait: detects whether _T has a .category member.
template<typename _T>
struct has_category
{
    static constexpr bool value =
        is_detected<internal::category_expr, _T>::value;
};

// ----------------------------------------------------------------
// defaults and constraints
// ----------------------------------------------------------------

// has_default_value
//   trait: detects whether _T has a .default_value member.
template<typename _T>
struct has_default_value
{
    static constexpr bool value =
        is_detected<internal::default_value_expr, _T>::value;
};

// has_required
//   trait: detects whether _T has a .required member.
template<typename _T>
struct has_required
{
    static constexpr bool value =
        is_detected<internal::required_expr, _T>::value;
};

// has_min_value
//   trait: detects whether _T has a .min_value member.
template<typename _T>
struct has_min_value
{
    static constexpr bool value =
        is_detected<internal::min_value_expr, _T>::value;
};

// has_max_value
//   trait: detects whether _T has a .max_value member.
template<typename _T>
struct has_max_value
{
    static constexpr bool value =
        is_detected<internal::max_value_expr, _T>::value;
};

// has_step_value
//   trait: detects whether _T has a .step member defining
// the increment granularity for bounded options.
template<typename _T>
struct has_step_value
{
    static constexpr bool value =
        is_detected<internal::step_value_expr, _T>::value;
};

// has_choices
//   trait: detects whether _T has a .choices member.
template<typename _T>
struct has_choices
{
    static constexpr bool value =
        is_detected<internal::choices_expr, _T>::value;
};

// has_validator
//   trait: detects whether _T has a .validate callable that
// accepts const value& and returns a result.
template<typename _T>
struct has_validator
{
    static constexpr bool value =
        is_detected<internal::validate_expr, _T>::value;
};

// ----------------------------------------------------------------
// visibility and lifecycle
// ----------------------------------------------------------------

// has_hidden
//   trait: detects whether _T has a .hidden member.
template<typename _T>
struct has_hidden
{
    static constexpr bool value =
        is_detected<internal::hidden_expr, _T>::value;
};

// has_deprecated
//   trait: detects whether _T has a .deprecated member.
template<typename _T>
struct has_deprecated
{
    static constexpr bool value =
        is_detected<internal::deprecated_expr, _T>::value;
};

// ----------------------------------------------------------------
// environment mapping
// ----------------------------------------------------------------

// has_env_var
//   trait: detects whether _T has an .env_var member.
template<typename _T>
struct has_env_var
{
    static constexpr bool value =
        is_detected<internal::env_var_expr, _T>::value;
};


///////////////////////////////////////////////////////////////////////////////
///        III.  ENTRY TYPE EXTRACTION                                      ///
///////////////////////////////////////////////////////////////////////////////
// Extracts the underlying type of a detected member, falling
// back to nonesuch when the member is absent.  These aliases
// are safe to use in unevaluated contexts even when the member
// does not exist.

// option_key_t
//   type: extracts the key type from an option entry, or
// nonesuch if unavailable.
template<typename _T>
using option_key_t =
    detected_or_t<nonesuch, internal::key_expr, _T>;

// option_value_t
//   type: extracts the value type from an option entry, or
// nonesuch if unavailable.
template<typename _T>
using option_value_t =
    detected_or_t<nonesuch, internal::value_expr, _T>;

// option_default_t
//   type: extracts the default_value type from an option
// entry, or nonesuch if unavailable.
template<typename _T>
using option_default_t =
    detected_or_t<nonesuch, internal::default_value_expr, _T>;

// option_description_t
//   type: extracts the description type from an option
// entry, or nonesuch if unavailable.
template<typename _T>
using option_description_t =
    detected_or_t<nonesuch, internal::description_expr, _T>;

// option_short_name_t
//   type: extracts the short_name type from an option entry,
// or nonesuch if unavailable.
template<typename _T>
using option_short_name_t =
    detected_or_t<nonesuch, internal::short_name_expr, _T>;

// option_category_t
//   type: extracts the category type from an option entry,
// or nonesuch if unavailable.
template<typename _T>
using option_category_t =
    detected_or_t<nonesuch, internal::category_expr, _T>;

// option_env_var_t
//   type: extracts the env_var type from an option entry,
// or nonesuch if unavailable.
template<typename _T>
using option_env_var_t =
    detected_or_t<nonesuch, internal::env_var_expr, _T>;

// option_choices_t
//   type: extracts the choices type from an option entry,
// or nonesuch if unavailable.
template<typename _T>
using option_choices_t =
    detected_or_t<nonesuch, internal::choices_expr, _T>;

// option_step_t
//   type: extracts the step type from an option entry,
// or nonesuch if unavailable.
template<typename _T>
using option_step_t =
    detected_or_t<nonesuch, internal::step_value_expr, _T>;


///////////////////////////////////////////////////////////////////////////////
///        IV.   KEY TYPE CLASSIFICATION                                    ///
///////////////////////////////////////////////////////////////////////////////
// Classifies the key type itself (when detectable) to enable
// generic algorithms that branch on key representation.
// All traits operate on clean_t<option_key_t<_T>> and are
// false when .key is absent.

NS_INTERNAL

    // key_type_or_void
    //   helper: resolves to clean_t of the key type when
    // present, or void when absent, so that downstream
    // type queries can safely instantiate.
    template<typename _T,
             bool = has_key<_T>::value>
    struct key_type_or_void
    {
        using type = void;
    };

    template<typename _T>
    struct key_type_or_void<_T, true>
    {
        using type =
            typename std::remove_cv<
                typename std::remove_reference<
                    option_key_t<_T>>::type>::type;
    };

    // string_like_check
    //   helper: true when _K is const char*, char*,
    // std::string, or std::string_view (or cv-qualified
    // variants thereof).  string_view detection is gated
    // behind C++17.
    template<typename _K>
    struct string_like_check
    {
    private:
        using bare = typename std::remove_cv<
            typename std::remove_pointer<
                typename std::decay<_K>::type>::type>::type;

        static constexpr bool is_char_ptr =
            ( std::is_same<bare, char>::value    ||
              std::is_same<bare, wchar_t>::value );

        static constexpr bool is_std_string =
            ( std::is_same<_K, std::string>::value       ||
              std::is_same<_K, const std::string>::value );

    #if D_ENV_LANG_IS_CPP17_OR_HIGHER
        static constexpr bool is_string_view =
            ( std::is_same<_K, std::string_view>::value       ||
              std::is_same<_K, const std::string_view>::value );
    #else
        static constexpr bool is_string_view = false;
    #endif

    public:
        static constexpr bool value =
            ( is_char_ptr     ||
              is_std_string   ||
              is_string_view );
    };

    // comparable_check
    //   helper: true when a < b is well-formed for _K.
    template<typename _K>
    using less_than_expr = decltype(
        std::declval<const _K&>() < std::declval<const _K&>());

    // equality_check
    //   helper: true when a == b is well-formed for _K.
    template<typename _K>
    using equality_expr = decltype(
        std::declval<const _K&>() ==
            std::declval<const _K&>());

NS_END  // internal

// has_string_key
//   trait: true when _T has a .key whose type is string-like
// (const char*, std::string, char[], wchar_t variants).
template<typename _T>
struct has_string_key
{
private:
    using key_clean =
        typename internal::key_type_or_void<_T>::type;

public:
    static constexpr bool value =
        ( has_key<_T>::value &&
          internal::string_like_check<key_clean>::value );
};

// has_enum_key
//   trait: true when _T has a .key whose type is an
// enumeration (scoped or unscoped).
template<typename _T>
struct has_enum_key
{
private:
    using key_clean =
        typename internal::key_type_or_void<_T>::type;

public:
    static constexpr bool value =
        ( has_key<_T>::value &&
          std::is_enum<key_clean>::value );
};

// has_scoped_enum_key
//   trait: true when _T has a .key whose type is a scoped
// enumeration (enum class / enum struct).  Uses the portable
// is_scoped_enum from type_traits.hpp.
template<typename _T>
struct has_scoped_enum_key
{
private:
    using key_clean =
        typename internal::key_type_or_void<_T>::type;

public:
    static constexpr bool value =
        ( has_key<_T>::value &&
          djinterp::is_scoped_enum<key_clean>::value );
};

// has_integral_key
//   trait: true when _T has a .key whose type is integral
// (int, unsigned, short, long, etc.) but not bool.
template<typename _T>
struct has_integral_key
{
private:
    using key_clean =
        typename internal::key_type_or_void<_T>::type;

public:
    static constexpr bool value =
        ( has_key<_T>::value                          &&
          std::is_integral<key_clean>::value           &&
          !std::is_same<key_clean, bool>::value );
};

// has_comparable_key
//   trait: true when _T has a .key that supports operator<.
// Required for sorted option_set instances.
template<typename _T>
struct has_comparable_key
{
private:
    using key_clean =
        typename internal::key_type_or_void<_T>::type;

public:
    static constexpr bool value =
        ( has_key<_T>::value &&
          is_detected<internal::less_than_expr,
                      key_clean>::value );
};

// has_equality_key
//   trait: true when _T has a .key that supports operator==.
// Required for any lookup operation.
template<typename _T>
struct has_equality_key
{
private:
    using key_clean =
        typename internal::key_type_or_void<_T>::type;

public:
    static constexpr bool value =
        ( has_key<_T>::value &&
          is_detected<internal::equality_expr,
                      key_clean>::value );
};


///////////////////////////////////////////////////////////////////////////////
///        V.    VALUE TYPE CLASSIFICATION                                  ///
///////////////////////////////////////////////////////////////////////////////
// Classifies the value type itself to help algorithms choose
// parsing, validation, and display strategies.

NS_INTERNAL

    // value_type_or_void
    //   helper: resolves to clean_t of the value type when
    // present, or void when absent.
    template<typename _T,
             bool = has_value<_T>::value>
    struct value_type_or_void
    {
        using type = void;
    };

    template<typename _T>
    struct value_type_or_void<_T, true>
    {
        using type =
            typename std::remove_cv<
                typename std::remove_reference<
                    option_value_t<_T>>::type>::type;
    };

NS_END  // internal

// has_boolean_value
//   trait: true when _T has a .value of type bool.
// Enables flag-style option handling (--verbose toggles
// a boolean).
template<typename _T>
struct has_boolean_value
{
private:
    using val_clean =
        typename internal::value_type_or_void<_T>::type;

public:
    static constexpr bool value =
        ( has_value<_T>::value &&
          std::is_same<val_clean, bool>::value );
};

// has_arithmetic_value
//   trait: true when _T has a .value of arithmetic type
// (integral or floating-point, including bool).
template<typename _T>
struct has_arithmetic_value
{
private:
    using val_clean =
        typename internal::value_type_or_void<_T>::type;

public:
    static constexpr bool value =
        ( has_value<_T>::value &&
          std::is_arithmetic<val_clean>::value );
};

// has_string_value
//   trait: true when _T has a .value whose type is
// string-like.
template<typename _T>
struct has_string_value
{
private:
    using val_clean =
        typename internal::value_type_or_void<_T>::type;

public:
    static constexpr bool value =
        ( has_value<_T>::value &&
          internal::string_like_check<val_clean>::value );
};


///////////////////////////////////////////////////////////////////////////////
///        VI.   COMPOSITE ENTRY TRAITS                                     ///
///////////////////////////////////////////////////////////////////////////////

// is_option_entry
//   trait: true when _T satisfies the minimum option entry
// contract (has .key and .value members).
template<typename _T>
struct is_option_entry
{
private:
    using clean_type = clean_t<_T>;

public:
    static constexpr bool value =
        ( has_key<clean_type>::value &&
          has_value<clean_type>::value );
};

// is_named_option
//   trait: true when _T carries at least one naming member
// (short_name or long_name), enabling CLI-style identification.
template<typename _T>
struct is_named_option
{
    static constexpr bool value =
        ( has_short_name<_T>::value ||
          has_long_name<_T>::value );
};

// is_documented_option
//   trait: true when _T carries at least one documentation
// member (description or help_text).
template<typename _T>
struct is_documented_option
{
    static constexpr bool value =
        ( has_description<_T>::value ||
          has_help_text<_T>::value );
};

// is_defaultable_option
//   trait: true when _T has a .default_value member.
template<typename _T>
struct is_defaultable_option
{
    static constexpr bool value =
        has_default_value<_T>::value;
};

// is_required_option
//   trait: true when _T has a .required member.  Note:
// this detects the presence of the member, not its
// runtime value.  Compile-time enforcement of the
// required flag is left to the container.
template<typename _T>
struct is_required_option
{
    static constexpr bool value =
        has_required<_T>::value;
};

// is_constrained_option
//   trait: true when _T carries at least one constraint
// mechanism: a callable validator, range bounds, or a
// discrete choices set.
template<typename _T>
struct is_constrained_option
{
    static constexpr bool value =
        ( has_validator<_T>::value  ||
          has_min_value<_T>::value  ||
          has_max_value<_T>::value  ||
          has_choices<_T>::value );
};

// is_bounded_option
//   trait: true when _T has both .min_value and .max_value
// members, enabling range-checked validation.
template<typename _T>
struct is_bounded_option
{
    static constexpr bool value =
        ( has_min_value<_T>::value &&
          has_max_value<_T>::value );
};

// is_stepped_option
//   trait: true when _T is bounded and also has a .step
// member, enabling discrete-increment validation.
template<typename _T>
struct is_stepped_option
{
    static constexpr bool value =
        ( is_bounded_option<_T>::value &&
          has_step_value<_T>::value );
};

// is_categorized_option
//   trait: true when _T has a .category member for grouping
// in help-text output.
template<typename _T>
struct is_categorized_option
{
    static constexpr bool value =
        has_category<_T>::value;
};

// is_env_mapped_option
//   trait: true when _T has an .env_var member, enabling
// environment-variable fallback.
template<typename _T>
struct is_env_mapped_option
{
    static constexpr bool value =
        has_env_var<_T>::value;
};

// is_fully_documented
//   trait: true when _T has documentation, a default value,
// and at least one naming member — sufficient for automatic
// help-text generation.
template<typename _T>
struct is_fully_documented
{
    static constexpr bool value =
        ( is_documented_option<_T>::value   &&
          is_defaultable_option<_T>::value  &&
          is_named_option<_T>::value );
};

// is_self_validating
//   trait: true when _T has both a callable validator and
// range bounds, enabling complete automated validation.
template<typename _T>
struct is_self_validating
{
    static constexpr bool value =
        ( has_validator<_T>::value &&
          is_bounded_option<_T>::value );
};

// option_known_column_count
//   trait: counts the number of detected optional columns.
// NOTE: this counts only the columns known to this header;
// user-defined columns outside this set are not counted.
template<typename _T>
struct option_known_column_count
{
    static constexpr std::size_t value =
        has_short_name<_T>::value     +
        has_long_name<_T>::value      +
        has_alias<_T>::value          +
        has_description<_T>::value    +
        has_help_text<_T>::value      +
        has_category<_T>::value       +
        has_default_value<_T>::value  +
        has_required<_T>::value       +
        has_min_value<_T>::value      +
        has_max_value<_T>::value      +
        has_step_value<_T>::value     +
        has_choices<_T>::value        +
        has_validator<_T>::value      +
        has_hidden<_T>::value         +
        has_deprecated<_T>::value     +
        has_env_var<_T>::value;
};


///////////////////////////////////////////////////////////////////////////////
///        VII.  ENTRY COMPATIBILITY TRAITS                                 ///
///////////////////////////////////////////////////////////////////////////////

// options_share_key_type
//   trait: true when both _E1 and _E2 have .key members of
// the same type.
template<typename _E1,
         typename _E2>
struct options_share_key_type
{
    static constexpr bool value =
        ( has_key<_E1>::value &&
          has_key<_E2>::value &&
          std::is_same<option_key_t<_E1>,
                       option_key_t<_E2>>::value );
};

// options_share_value_type
//   trait: true when both _E1 and _E2 have .value members
// of the same type.
template<typename _E1,
         typename _E2>
struct options_share_value_type
{
    static constexpr bool value =
        ( has_value<_E1>::value &&
          has_value<_E2>::value &&
          std::is_same<option_value_t<_E1>,
                       option_value_t<_E2>>::value );
};

// options_are_compatible
//   trait: true when _E1 and _E2 share both key and value
// types.
template<typename _E1,
         typename _E2>
struct options_are_compatible
{
    static constexpr bool value =
        ( options_share_key_type<_E1, _E2>::value &&
          options_share_value_type<_E1, _E2>::value );
};

// option_is_superset_of
//   trait: true when _Super has every optional column that
// _Sub has.  Useful for verifying that a richer option type
// can substitute for a leaner one without losing data.
// NOTE: detection is limited to columns known to this header.
template<typename _Super,
         typename _Sub>
struct option_is_superset_of
{
private:
    // for each column in _Sub, _Super must also have it
    static constexpr bool check_short_name =
        ( !has_short_name<_Sub>::value ||
           has_short_name<_Super>::value );

    static constexpr bool check_long_name =
        ( !has_long_name<_Sub>::value ||
           has_long_name<_Super>::value );

    static constexpr bool check_alias =
        ( !has_alias<_Sub>::value ||
           has_alias<_Super>::value );

    static constexpr bool check_description =
        ( !has_description<_Sub>::value ||
           has_description<_Super>::value );

    static constexpr bool check_help_text =
        ( !has_help_text<_Sub>::value ||
           has_help_text<_Super>::value );

    static constexpr bool check_category =
        ( !has_category<_Sub>::value ||
           has_category<_Super>::value );

    static constexpr bool check_default =
        ( !has_default_value<_Sub>::value ||
           has_default_value<_Super>::value );

    static constexpr bool check_required =
        ( !has_required<_Sub>::value ||
           has_required<_Super>::value );

    static constexpr bool check_min =
        ( !has_min_value<_Sub>::value ||
           has_min_value<_Super>::value );

    static constexpr bool check_max =
        ( !has_max_value<_Sub>::value ||
           has_max_value<_Super>::value );

    static constexpr bool check_step =
        ( !has_step_value<_Sub>::value ||
           has_step_value<_Super>::value );

    static constexpr bool check_choices =
        ( !has_choices<_Sub>::value ||
           has_choices<_Super>::value );

    static constexpr bool check_validator =
        ( !has_validator<_Sub>::value ||
           has_validator<_Super>::value );

    static constexpr bool check_hidden =
        ( !has_hidden<_Sub>::value ||
           has_hidden<_Super>::value );

    static constexpr bool check_deprecated =
        ( !has_deprecated<_Sub>::value ||
           has_deprecated<_Super>::value );

    static constexpr bool check_env_var =
        ( !has_env_var<_Sub>::value ||
           has_env_var<_Super>::value );

public:
    static constexpr bool value =
        ( options_are_compatible<_Super, _Sub>::value &&
          check_short_name  &&
          check_long_name   &&
          check_alias       &&
          check_description &&
          check_help_text   &&
          check_category    &&
          check_default     &&
          check_required    &&
          check_min         &&
          check_max         &&
          check_step        &&
          check_choices     &&
          check_validator   &&
          check_hidden      &&
          check_deprecated  &&
          check_env_var );
};


///////////////////////////////////////////////////////////////////////////////
///        VIII. CONTAINER-LEVEL EXPRESSION ALIASES                         ///
///////////////////////////////////////////////////////////////////////////////
// Detects whether a container type exposes the interface
// expected of an option_set: nested type aliases, lookup
// methods, iteration, and optional features.  Each expression
// is probed in a void_t context.

NS_INTERNAL

    // ----------------------------------------------------------------
    // nested type aliases
    // ----------------------------------------------------------------

    // key_type_alias_expr
    //   detector: expression alias for ::key_type.
    template<typename _O>
    using key_type_alias_expr = typename _O::key_type;

    // mapped_type_alias_expr
    //   detector: expression alias for ::mapped_type.
    template<typename _O>
    using mapped_type_alias_expr = typename _O::mapped_type;

    // value_type_alias_expr
    //   detector: expression alias for ::value_type
    // (the pair type in option_set).
    template<typename _O>
    using value_type_alias_expr = typename _O::value_type;

    // extent_expr
    //   detector: expression alias for ::extent static member.
    template<typename _O>
    using extent_expr = decltype(_O::extent);

    // ----------------------------------------------------------------
    // lookup methods
    // ----------------------------------------------------------------

    // find_method_expr
    //   detector: expression alias for .find(key).
    template<typename _O>
    using find_method_expr = decltype(
        std::declval<const _O&>().find(
            std::declval<
                const typename _O::key_type&>()));

    // contains_method_expr
    //   detector: expression alias for .contains(key).
    template<typename _O>
    using contains_method_expr = decltype(
        std::declval<const _O&>().contains(
            std::declval<
                const typename _O::key_type&>()));

    // value_or_method_expr
    //   detector: expression alias for
    // .value_or(key, fallback).
    template<typename _O>
    using value_or_method_expr = decltype(
        std::declval<const _O&>().value_or(
            std::declval<
                const typename _O::key_type&>(),
            std::declval<
                const typename _O::mapped_type&>()));

    // ----------------------------------------------------------------
    // capacity methods
    // ----------------------------------------------------------------

    // size_method_expr
    //   detector: expression alias for .size().
    template<typename _O>
    using size_method_expr =
        decltype(std::declval<const _O&>().size());

    // empty_method_expr
    //   detector: expression alias for .empty().
    template<typename _O>
    using empty_method_expr =
        decltype(std::declval<const _O&>().empty());

    // ----------------------------------------------------------------
    // iteration methods
    // ----------------------------------------------------------------

    // begin_method_expr
    //   detector: expression alias for .begin().
    template<typename _O>
    using begin_method_expr =
        decltype(std::declval<const _O&>().begin());

    // end_method_expr
    //   detector: expression alias for .end().
    template<typename _O>
    using end_method_expr =
        decltype(std::declval<const _O&>().end());

    // ----------------------------------------------------------------
    // optional features
    // ----------------------------------------------------------------

    // backing_array_method_expr
    //   detector: expression alias for .backing_array().
    // Present only on static-extent option_sets.
    template<typename _O>
    using backing_array_method_expr =
        decltype(std::declval<const _O&>().backing_array());

    // count_if_method_expr
    //   detector: expression alias for .count_if(pred).
    template<typename _O>
    using count_if_method_expr = decltype(
        std::declval<const _O&>().count_if(
            std::declval<bool(*)(
                const typename _O::value_type&)>()));

    // data_method_expr
    //   detector: expression alias for .data() -> pointer.
    template<typename _O>
    using data_method_expr =
        decltype(std::declval<const _O&>().data());

NS_END  // internal


///////////////////////////////////////////////////////////////////////////////
///        IX.   CONTAINER-LEVEL has_* TRAITS                               ///
///////////////////////////////////////////////////////////////////////////////

// ----------------------------------------------------------------
// nested type aliases
// ----------------------------------------------------------------

// has_key_type_alias
//   trait: detects whether _O has a ::key_type nested type.
template<typename _O>
struct has_key_type_alias
{
    static constexpr bool value =
        is_detected<internal::key_type_alias_expr, _O>::value;
};

// has_mapped_type_alias
//   trait: detects whether _O has a ::mapped_type nested type.
template<typename _O>
struct has_mapped_type_alias
{
    static constexpr bool value =
        is_detected<
            internal::mapped_type_alias_expr, _O>::value;
};

// has_value_type_alias
//   trait: detects whether _O has a ::value_type nested type.
template<typename _O>
struct has_value_type_alias
{
    static constexpr bool value =
        is_detected<
            internal::value_type_alias_expr, _O>::value;
};

// has_extent
//   trait: detects whether _O has a ::extent static member.
template<typename _O>
struct has_extent
{
    static constexpr bool value =
        is_detected<internal::extent_expr, _O>::value;
};

// ----------------------------------------------------------------
// lookup methods
// ----------------------------------------------------------------

// has_find_method
//   trait: detects whether _O has a .find(key) method.
template<typename _O>
struct has_find_method
{
    static constexpr bool value =
        is_detected<internal::find_method_expr, _O>::value;
};

// has_contains_method
//   trait: detects whether _O has a .contains(key) method.
template<typename _O>
struct has_contains_method
{
    static constexpr bool value =
        is_detected<
            internal::contains_method_expr, _O>::value;
};

// has_value_or_method
//   trait: detects whether _O has a .value_or(key, fallback)
// method.
template<typename _O>
struct has_value_or_method
{
    static constexpr bool value =
        is_detected<
            internal::value_or_method_expr, _O>::value;
};

// ----------------------------------------------------------------
// capacity methods
// ----------------------------------------------------------------

// has_size_method
//   trait: detects whether _O has a .size() method.
template<typename _O>
struct has_size_method
{
    static constexpr bool value =
        is_detected<internal::size_method_expr, _O>::value;
};

// has_empty_method
//   trait: detects whether _O has an .empty() method.
template<typename _O>
struct has_empty_method
{
    static constexpr bool value =
        is_detected<internal::empty_method_expr, _O>::value;
};

// ----------------------------------------------------------------
// iteration methods
// ----------------------------------------------------------------

// has_begin_method
//   trait: detects whether _O has a .begin() method.
template<typename _O>
struct has_begin_method
{
    static constexpr bool value =
        is_detected<internal::begin_method_expr, _O>::value;
};

// has_end_method
//   trait: detects whether _O has an .end() method.
template<typename _O>
struct has_end_method
{
    static constexpr bool value =
        is_detected<internal::end_method_expr, _O>::value;
};

// ----------------------------------------------------------------
// optional features
// ----------------------------------------------------------------

// has_backing_array
//   trait: detects whether _O has a .backing_array() method.
template<typename _O>
struct has_backing_array
{
    static constexpr bool value =
        is_detected<
            internal::backing_array_method_expr, _O>::value;
};

// has_count_if_method
//   trait: detects whether _O has a .count_if(pred) method.
template<typename _O>
struct has_count_if_method
{
    static constexpr bool value =
        is_detected<
            internal::count_if_method_expr, _O>::value;
};

// has_data_method
//   trait: detects whether _O has a .data() method.
template<typename _O>
struct has_data_method
{
    static constexpr bool value =
        is_detected<internal::data_method_expr, _O>::value;
};


///////////////////////////////////////////////////////////////////////////////
///        X.    COMPOSITE CONTAINER TRAITS                                 ///
///////////////////////////////////////////////////////////////////////////////

// is_option_set
//   trait: true when _O satisfies the minimum option_set
// container contract: exposes ::key_type, supports keyed
// lookup via .find and .contains, reports size via .size,
// and is iterable via .begin/.end.
template<typename _O>
struct is_option_set
{
private:
    using clean_type = clean_t<_O>;

public:
    static constexpr bool value =
        ( has_key_type_alias<clean_type>::value    &&
          has_find_method<clean_type>::value       &&
          has_contains_method<clean_type>::value   &&
          has_size_method<clean_type>::value       &&
          has_begin_method<clean_type>::value      &&
          has_end_method<clean_type>::value );
};

// is_iterable_option_set
//   trait: true when _O supports range-based iteration
// via .begin() and .end().
template<typename _O>
struct is_iterable_option_set
{
    static constexpr bool value =
        ( has_begin_method<_O>::value &&
          has_end_method<_O>::value );
};

// is_searchable_option_set
//   trait: true when _O supports keyed lookup via .find()
// and .contains().
template<typename _O>
struct is_searchable_option_set
{
    static constexpr bool value =
        ( has_find_method<_O>::value &&
          has_contains_method<_O>::value );
};

// is_static_option_set
//   trait: true when _O satisfies the option_set contract
// and exposes .backing_array(), indicating static-extent
// (compile-time-sized) storage.
template<typename _O>
struct is_static_option_set
{
    static constexpr bool value =
        ( is_option_set<_O>::value &&
          has_backing_array<_O>::value );
};

// is_dynamic_option_set
//   trait: true when _O satisfies the option_set contract
// but does NOT expose .backing_array(), indicating heap-
// backed dynamic-extent storage.
template<typename _O>
struct is_dynamic_option_set
{
    static constexpr bool value =
        ( is_option_set<_O>::value &&
          !has_backing_array<_O>::value );
};

// is_sorted_option_set
//   trait: true when _O satisfies the option_set contract
// and its key type supports operator<, which is a
// prerequisite for the binary-search lookup path.
// Note: this is a necessary condition, not a guarantee
// that the instance was constructed with _Sorted=true.
// Runtime sorted state is not detectable at compile time
// without a tag; this trait confirms sortability.
template<typename _O>
struct is_sorted_option_set
{
private:
    using clean_type = clean_t<_O>;

public:
    static constexpr bool value =
        ( is_option_set<clean_type>::value        &&
          has_key_type_alias<clean_type>::value    &&
          is_detected<internal::less_than_expr,
              typename clean_type::key_type>::value );
};


///////////////////////////////////////////////////////////////////////////////
///        XI.   CONVENIENCE ALIASES                                        ///
///////////////////////////////////////////////////////////////////////////////

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    // ================================================================
    // entry-level _v aliases
    // ================================================================

    template<typename _T>
    constexpr bool has_key_v = has_key<_T>::value;

    template<typename _T>
    constexpr bool has_value_v = has_value<_T>::value;

    template<typename _T>
    constexpr bool has_short_name_v = has_short_name<_T>::value;

    template<typename _T>
    constexpr bool has_long_name_v = has_long_name<_T>::value;

    template<typename _T>
    constexpr bool has_alias_v = has_alias<_T>::value;

    template<typename _T>
    constexpr bool has_description_v =
        has_description<_T>::value;

    template<typename _T>
    constexpr bool has_help_text_v = has_help_text<_T>::value;

    template<typename _T>
    constexpr bool has_category_v = has_category<_T>::value;

    template<typename _T>
    constexpr bool has_default_value_v =
        has_default_value<_T>::value;

    template<typename _T>
    constexpr bool has_required_v = has_required<_T>::value;

    template<typename _T>
    constexpr bool has_min_value_v = has_min_value<_T>::value;

    template<typename _T>
    constexpr bool has_max_value_v = has_max_value<_T>::value;

    template<typename _T>
    constexpr bool has_step_value_v =
        has_step_value<_T>::value;

    template<typename _T>
    constexpr bool has_choices_v = has_choices<_T>::value;

    template<typename _T>
    constexpr bool has_validator_v = has_validator<_T>::value;

    template<typename _T>
    constexpr bool has_hidden_v = has_hidden<_T>::value;

    template<typename _T>
    constexpr bool has_deprecated_v = has_deprecated<_T>::value;

    template<typename _T>
    constexpr bool has_env_var_v = has_env_var<_T>::value;

    // ================================================================
    // key / value classification _v aliases
    // ================================================================

    template<typename _T>
    constexpr bool has_string_key_v =
        has_string_key<_T>::value;

    template<typename _T>
    constexpr bool has_enum_key_v = has_enum_key<_T>::value;

    template<typename _T>
    constexpr bool has_scoped_enum_key_v =
        has_scoped_enum_key<_T>::value;

    template<typename _T>
    constexpr bool has_integral_key_v =
        has_integral_key<_T>::value;

    template<typename _T>
    constexpr bool has_comparable_key_v =
        has_comparable_key<_T>::value;

    template<typename _T>
    constexpr bool has_equality_key_v =
        has_equality_key<_T>::value;

    template<typename _T>
    constexpr bool has_boolean_value_v =
        has_boolean_value<_T>::value;

    template<typename _T>
    constexpr bool has_arithmetic_value_v =
        has_arithmetic_value<_T>::value;

    template<typename _T>
    constexpr bool has_string_value_v =
        has_string_value<_T>::value;

    // ================================================================
    // composite entry _v aliases
    // ================================================================

    template<typename _T>
    constexpr bool is_option_entry_v =
        is_option_entry<_T>::value;

    template<typename _T>
    constexpr bool is_named_option_v =
        is_named_option<_T>::value;

    template<typename _T>
    constexpr bool is_documented_option_v =
        is_documented_option<_T>::value;

    template<typename _T>
    constexpr bool is_defaultable_option_v =
        is_defaultable_option<_T>::value;

    template<typename _T>
    constexpr bool is_required_option_v =
        is_required_option<_T>::value;

    template<typename _T>
    constexpr bool is_constrained_option_v =
        is_constrained_option<_T>::value;

    template<typename _T>
    constexpr bool is_bounded_option_v =
        is_bounded_option<_T>::value;

    template<typename _T>
    constexpr bool is_stepped_option_v =
        is_stepped_option<_T>::value;

    template<typename _T>
    constexpr bool is_categorized_option_v =
        is_categorized_option<_T>::value;

    template<typename _T>
    constexpr bool is_env_mapped_option_v =
        is_env_mapped_option<_T>::value;

    template<typename _T>
    constexpr bool is_fully_documented_v =
        is_fully_documented<_T>::value;

    template<typename _T>
    constexpr bool is_self_validating_v =
        is_self_validating<_T>::value;

    template<typename _T>
    constexpr std::size_t option_known_column_count_v =
        option_known_column_count<_T>::value;

    // ================================================================
    // entry compatibility _v aliases
    // ================================================================

    template<typename _E1, typename _E2>
    constexpr bool options_share_key_type_v =
        options_share_key_type<_E1, _E2>::value;

    template<typename _E1, typename _E2>
    constexpr bool options_share_value_type_v =
        options_share_value_type<_E1, _E2>::value;

    template<typename _E1, typename _E2>
    constexpr bool options_are_compatible_v =
        options_are_compatible<_E1, _E2>::value;

    template<typename _Super, typename _Sub>
    constexpr bool option_is_superset_of_v =
        option_is_superset_of<_Super, _Sub>::value;

    // ================================================================
    // container-level _v aliases
    // ================================================================

    template<typename _O>
    constexpr bool has_key_type_alias_v =
        has_key_type_alias<_O>::value;

    template<typename _O>
    constexpr bool has_mapped_type_alias_v =
        has_mapped_type_alias<_O>::value;

    template<typename _O>
    constexpr bool has_value_type_alias_v =
        has_value_type_alias<_O>::value;

    template<typename _O>
    constexpr bool has_extent_v = has_extent<_O>::value;

    template<typename _O>
    constexpr bool has_find_method_v =
        has_find_method<_O>::value;

    template<typename _O>
    constexpr bool has_contains_method_v =
        has_contains_method<_O>::value;

    template<typename _O>
    constexpr bool has_value_or_method_v =
        has_value_or_method<_O>::value;

    template<typename _O>
    constexpr bool has_size_method_v =
        has_size_method<_O>::value;

    template<typename _O>
    constexpr bool has_empty_method_v =
        has_empty_method<_O>::value;

    template<typename _O>
    constexpr bool has_begin_method_v =
        has_begin_method<_O>::value;

    template<typename _O>
    constexpr bool has_end_method_v =
        has_end_method<_O>::value;

    template<typename _O>
    constexpr bool has_backing_array_v =
        has_backing_array<_O>::value;

    template<typename _O>
    constexpr bool has_count_if_method_v =
        has_count_if_method<_O>::value;

    template<typename _O>
    constexpr bool has_data_method_v =
        has_data_method<_O>::value;

    // ================================================================
    // composite container _v aliases
    // ================================================================

    template<typename _O>
    constexpr bool is_option_set_v =
        is_option_set<_O>::value;

    template<typename _O>
    constexpr bool is_iterable_option_set_v =
        is_iterable_option_set<_O>::value;

    template<typename _O>
    constexpr bool is_searchable_option_set_v =
        is_searchable_option_set<_O>::value;

    template<typename _O>
    constexpr bool is_static_option_set_v =
        is_static_option_set<_O>::value;

    template<typename _O>
    constexpr bool is_dynamic_option_set_v =
        is_dynamic_option_set<_O>::value;

    template<typename _O>
    constexpr bool is_sorted_option_set_v =
        is_sorted_option_set<_O>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES


///////////////////////////////////////////////////////////////////////////////
///        XII.  TARGET-LEVEL EXPRESSION ALIASES                            ///
///////////////////////////////////////////////////////////////////////////////
// Detects whether an arbitrary type _T exposes methods for
// accepting, exporting, or parsing options.  No assumption
// is made about _T — it may be a container, a service, a
// configuration object, or any user-defined type.
//
// Expression aliases are split into two groups:
//   A. Unparameterized — method signature is fully determined
//      by _T alone (fixed argument types or no arguments).
//   B. Parameterized — the method signature depends on one or
//      more additional type parameters (e.g. the option_set
//      type, or key and value types).

NS_INTERNAL

    // ----------------------------------------------------------------
    // A.  unparameterized (single _T)
    // ----------------------------------------------------------------

    // to_options_method_expr
    //   detector: expression alias for .to_options() const.
    template<typename _T>
    using to_options_method_expr = decltype(
        std::declval<const _T&>().to_options());

    // get_options_method_expr
    //   detector: expression alias for .get_options() const.
    template<typename _T>
    using get_options_method_expr = decltype(
        std::declval<const _T&>().get_options());

    // help_string_method_expr
    //   detector: expression alias for .help_string() const.
    template<typename _T>
    using help_string_method_expr = decltype(
        std::declval<const _T&>().help_string());

    // parse_args_method_expr
    //   detector: expression alias for
    // .parse_args(int, const char* const[]).
    template<typename _T>
    using parse_args_method_expr = decltype(
        std::declval<_T&>().parse_args(
            std::declval<int>(),
            std::declval<const char* const*>()));

    // parse_option_method_expr
    //   detector: expression alias for
    // .parse_option(const std::string&).
    template<typename _T>
    using parse_option_method_expr = decltype(
        std::declval<_T&>().parse_option(
            std::declval<const std::string&>()));

    // parse_config_method_expr
    //   detector: expression alias for
    // .parse_config(const std::string&).
    template<typename _T>
    using parse_config_method_expr = decltype(
        std::declval<_T&>().parse_config(
            std::declval<const std::string&>()));

    // ----------------------------------------------------------------
    // B.  parameterized (multi-type)
    // ----------------------------------------------------------------
    // These aliases accept additional type parameters beyond _T.
    // Use with is_detected<alias, _T, _Extra...>.

    // apply_options_method_expr
    //   detector: expression alias for
    // .apply_options(const _OptionSet&).
    template<typename _T,
             typename _OptionSet>
    using apply_options_method_expr = decltype(
        std::declval<_T&>().apply_options(
            std::declval<const _OptionSet&>()));

    // configure_method_expr
    //   detector: expression alias for
    // .configure(const _OptionSet&).
    template<typename _T,
             typename _OptionSet>
    using configure_method_expr = decltype(
        std::declval<_T&>().configure(
            std::declval<const _OptionSet&>()));

    // set_option_method_expr
    //   detector: expression alias for
    // .set_option(const _Key&, const _Value&).
    template<typename _T,
             typename _Key,
             typename _Value>
    using set_option_method_expr = decltype(
        std::declval<_T&>().set_option(
            std::declval<const _Key&>(),
            std::declval<const _Value&>()));

NS_END  // internal


///////////////////////////////////////////////////////////////////////////////
///        XIII. TARGET-LEVEL has_* TRAITS                                  ///
///////////////////////////////////////////////////////////////////////////////

// ----------------------------------------------------------------
// unparameterized (single _T)
// ----------------------------------------------------------------

// has_to_options_method
//   trait: detects whether _T has a .to_options() const method.
template<typename _T>
struct has_to_options_method
{
    static constexpr bool value =
        is_detected<
            internal::to_options_method_expr, _T>::value;
};

// has_get_options_method
//   trait: detects whether _T has a .get_options() const method.
template<typename _T>
struct has_get_options_method
{
    static constexpr bool value =
        is_detected<
            internal::get_options_method_expr, _T>::value;
};

// has_help_string_method
//   trait: detects whether _T has a .help_string() const method
// returning a string or string-like type.
template<typename _T>
struct has_help_string_method
{
    static constexpr bool value =
        is_detected<
            internal::help_string_method_expr, _T>::value;
};

// has_parse_args_method
//   trait: detects whether _T has a
// .parse_args(int, const char* const[]) method.
template<typename _T>
struct has_parse_args_method
{
    static constexpr bool value =
        is_detected<
            internal::parse_args_method_expr, _T>::value;
};

// has_parse_option_method
//   trait: detects whether _T has a
// .parse_option(const std::string&) method.
template<typename _T>
struct has_parse_option_method
{
    static constexpr bool value =
        is_detected<
            internal::parse_option_method_expr, _T>::value;
};

// has_parse_config_method
//   trait: detects whether _T has a
// .parse_config(const std::string&) method.
template<typename _T>
struct has_parse_config_method
{
    static constexpr bool value =
        is_detected<
            internal::parse_config_method_expr, _T>::value;
};

// ----------------------------------------------------------------
// parameterized (multi-type)
// ----------------------------------------------------------------

// has_apply_options_method
//   trait: detects whether _T has an
// .apply_options(const _OptionSet&) method.
template<typename _T,
         typename _OptionSet>
struct has_apply_options_method
{
    static constexpr bool value =
        is_detected<internal::apply_options_method_expr,
                    _T,
                    _OptionSet>::value;
};

// has_configure_method
//   trait: detects whether _T has a
// .configure(const _OptionSet&) method.
template<typename _T,
         typename _OptionSet>
struct has_configure_method
{
    static constexpr bool value =
        is_detected<internal::configure_method_expr,
                    _T,
                    _OptionSet>::value;
};

// has_set_option_method
//   trait: detects whether _T has a
// .set_option(const _Key&, const _Value&) method.
template<typename _T,
         typename _Key,
         typename _Value>
struct has_set_option_method
{
    static constexpr bool value =
        is_detected<internal::set_option_method_expr,
                    _T,
                    _Key,
                    _Value>::value;
};


///////////////////////////////////////////////////////////////////////////////
///        XIV.  CONFIGURE STRATEGY                                         ///
///////////////////////////////////////////////////////////////////////////////
// Selects the best available strategy for applying an
// option_set _OptionSet to a target type _T.  Strategies
// are ordered from most efficient (native bulk apply) to
// least (per-entry fallback).  When no path exists, the
// strategy is unsupported.

// configure_strategy
//   enum: compile-time strategy tag for option application
// dispatch.  Values ordered from most efficient (bulk)
// to least (per-entry fallback).
//   native_apply     — _T.apply_options(opts)     [bulk, preferred]
//   native_configure — _T.configure(opts)         [bulk, alternate]
//   per_option       — _T.set_option(k, v) loop   [entry-by-entry]
//   unsupported      — no apply path available
enum class configure_strategy
{
    native_apply,
    native_configure,
    per_option,
    unsupported
};

NS_INTERNAL

    // option_configure_strategy_helper
    //   helper: resolves the best configure strategy for
    // <_T, _OptionSet>.  Checked in priority order.
    template<typename _T,
             typename _OptionSet,
             bool _HasApply     = has_apply_options_method<
                                      _T, _OptionSet>::value,
             bool _HasConfigure = has_configure_method<
                                      _T, _OptionSet>::value>
    struct option_configure_strategy_helper
    {
        static constexpr configure_strategy value =
            configure_strategy::unsupported;
    };

    // specialization: native apply_options path
    template<typename _T,
             typename _OptionSet,
             bool     _HasConfigure>
    struct option_configure_strategy_helper<
        _T, _OptionSet, true, _HasConfigure>
    {
        static constexpr configure_strategy value =
            configure_strategy::native_apply;
    };

    // specialization: native configure path
    template<typename _T,
             typename _OptionSet>
    struct option_configure_strategy_helper<
        _T, _OptionSet, false, true>
    {
        static constexpr configure_strategy value =
            configure_strategy::native_configure;
    };

    // per_option_strategy_check
    //   helper: detects the per-option path when the
    // option_set's entry type exposes .key and .value
    // and _T has a matching set_option method.
    // Falls back to unsupported when entry types cannot
    // be deduced.
    template<typename _T,
             typename _OptionSet,
             bool = ( has_value_type_alias<_OptionSet>::value &&
                      has_begin_method<_OptionSet>::value     &&
                      has_end_method<_OptionSet>::value )>
    struct per_option_strategy_check
    {
        static constexpr bool value = false;
    };

    template<typename _T,
             typename _OptionSet>
    struct per_option_strategy_check<_T, _OptionSet, true>
    {
    private:
        using entry_type = typename _OptionSet::value_type;

    public:
        static constexpr bool value =
            ( has_key<entry_type>::value   &&
              has_value<entry_type>::value &&
              is_detected<set_option_method_expr,
                          _T,
                          option_key_t<entry_type>,
                          option_value_t<entry_type>>::value );
    };

NS_END  // internal

// option_configure_strategy
//   trait: resolves the best configure strategy for applying
// _OptionSet to _T.  Checks native paths first, then falls
// back to per-option iteration.
template<typename _T,
         typename _OptionSet>
struct option_configure_strategy
{
private:
    static constexpr configure_strategy native =
        internal::option_configure_strategy_helper<
            _T, _OptionSet>::value;

public:
    static constexpr configure_strategy value =
        (native != configure_strategy::unsupported)
            ? native
            : ( internal::per_option_strategy_check<
                    _T, _OptionSet>::value
                ? configure_strategy::per_option
                : configure_strategy::unsupported );
};


///////////////////////////////////////////////////////////////////////////////
///        XV.   COMPOSITE TARGET TRAITS                                    ///
///////////////////////////////////////////////////////////////////////////////

// is_options_configurable
//   trait: true when _T can be configured from _OptionSet
// via at least one strategy path (apply_options, configure,
// or per-option set_option).
template<typename _T,
         typename _OptionSet>
struct is_options_configurable
{
    static constexpr bool value =
        ( option_configure_strategy<
              _T, _OptionSet>::value !=
          configure_strategy::unsupported );
};

// is_option_producing
//   trait: true when _T can export its current configuration
// as an option collection via to_options() or get_options().
template<typename _T>
struct is_option_producing
{
    static constexpr bool value =
        ( has_to_options_method<_T>::value ||
          has_get_options_method<_T>::value );
};

// is_option_parseable
//   trait: true when _T can accept configuration from string
// sources — CLI arguments, individual option strings, or
// config file contents.
template<typename _T>
struct is_option_parseable
{
    static constexpr bool value =
        ( has_parse_args_method<_T>::value   ||
          has_parse_option_method<_T>::value ||
          has_parse_config_method<_T>::value );
};

// is_option_describable
//   trait: true when _T can produce help/usage text
// describing its available options.
template<typename _T>
struct is_option_describable
{
    static constexpr bool value =
        has_help_string_method<_T>::value;
};


///////////////////////////////////////////////////////////////////////////////
///        XVI.  TARGET-LEVEL CONVENIENCE ALIASES                           ///
///////////////////////////////////////////////////////////////////////////////

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    // ================================================================
    // target-level has_* _v aliases
    // ================================================================

    template<typename _T>
    constexpr bool has_to_options_method_v =
        has_to_options_method<_T>::value;

    template<typename _T>
    constexpr bool has_get_options_method_v =
        has_get_options_method<_T>::value;

    template<typename _T>
    constexpr bool has_help_string_method_v =
        has_help_string_method<_T>::value;

    template<typename _T>
    constexpr bool has_parse_args_method_v =
        has_parse_args_method<_T>::value;

    template<typename _T>
    constexpr bool has_parse_option_method_v =
        has_parse_option_method<_T>::value;

    template<typename _T>
    constexpr bool has_parse_config_method_v =
        has_parse_config_method<_T>::value;

    template<typename _T, typename _OptionSet>
    constexpr bool has_apply_options_method_v =
        has_apply_options_method<_T, _OptionSet>::value;

    template<typename _T, typename _OptionSet>
    constexpr bool has_configure_method_v =
        has_configure_method<_T, _OptionSet>::value;

    template<typename _T, typename _Key, typename _Value>
    constexpr bool has_set_option_method_v =
        has_set_option_method<_T, _Key, _Value>::value;

    // ================================================================
    // strategy _v alias
    // ================================================================

    template<typename _T, typename _OptionSet>
    constexpr configure_strategy
        option_configure_strategy_v =
            option_configure_strategy<_T, _OptionSet>::value;

    // ================================================================
    // composite target _v aliases
    // ================================================================

    template<typename _T, typename _OptionSet>
    constexpr bool is_options_configurable_v =
        is_options_configurable<_T, _OptionSet>::value;

    template<typename _T>
    constexpr bool is_option_producing_v =
        is_option_producing<_T>::value;

    template<typename _T>
    constexpr bool is_option_parseable_v =
        is_option_parseable<_T>::value;

    template<typename _T>
    constexpr bool is_option_describable_v =
        is_option_describable<_T>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES


NS_END  // djinterp


#endif  // DJINTERP_OPTION_SET_TRAITS_
