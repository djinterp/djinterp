/******************************************************************************
* djinterp [options]                                    option_pair_traits.hpp
*
* SFINAE-based traits for option entries (types exposing .key and .value).
*   Provides compile-time detection and classification of individual option
* entries: mandatory member detection (.key, .value), optional column
* detection (description, default_value, bounds, choices, etc.), key and
* value type classification, composite entry traits, and entry
* compatibility queries.
*
*   All detection is purely structural: no tag types are required.  An
* option entry is any type with .key and .value members.  Optional
* columns are detected by member presence.  Users extend the system by
* adding their own members and writing matching expression aliases
* following the pattern in Section I.
*
*   Uses the void_t / detector SFINAE idiom from type_traits.hpp.
*
*
* path:      /inc/options/option_pair_traits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.06
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.      ENTRY-LEVEL EXPRESSION ALIASES
        1.  key_expr                (.key member)
        2.  value_expr              (.value member)
        3.  default_value_expr      (.default_value member)
        4.  description_expr        (.description member)
        5.  short_name_expr         (.short_name member)
        6.  long_name_expr          (.long_name member)
        7.  alias_expr              (.alias member)
        8.  required_expr           (.required member)
        9.  hidden_expr             (.hidden member)
        10. deprecated_expr         (.deprecated member)
        11. category_expr           (.category member)
        12. env_var_expr            (.env_var member)
        13. choices_expr            (.choices member)
        14. validate_expr           (.validate callable)
        15. min_value_expr          (.min_value member)
        16. max_value_expr          (.max_value member)
        17. step_value_expr         (.step member)
        18. help_text_expr          (.help_text member)
II.     ENTRY-LEVEL has_* TRAITS
III.    ENTRY TYPE EXTRACTION
        1.  option_key_t            (key type or nonesuch)
        2.  option_value_t          (value type or nonesuch)
        3.  option_default_t        (default_value type or nonesuch)
        4.  option_description_t    (description type or nonesuch)
        5.  option_short_name_t     (short_name type or nonesuch)
        6.  option_category_t       (category type or nonesuch)
        7.  option_env_var_t        (env_var type or nonesuch)
        8.  option_choices_t        (choices type or nonesuch)
        9.  option_step_t           (step type or nonesuch)
IV.     KEY TYPE CLASSIFICATION
        1.  has_string_key          (key is string-like)
        2.  has_enum_key            (key is an enumeration)
        3.  has_scoped_enum_key     (key is a scoped enum)
        4.  has_integral_key        (key is integral)
        5.  has_comparable_key      (key supports operator<)
        6.  has_equality_key        (key supports operator==)
V.      VALUE TYPE CLASSIFICATION
        1.  has_boolean_value       (value is bool)
        2.  has_arithmetic_value    (value is arithmetic)
        3.  has_string_value        (value is string-like)
        4.  has_any_value           (value is compat::any)
        5.  has_enum_value          (value is an enumeration)
        6.  has_pointer_value       (value is a pointer)
VI.     COMPOSITE ENTRY TRAITS
        1.  is_option_entry         (key + value)
        2.  is_option_pair          (option_pair specialization)
        3.  is_named_option         (has short_name or long_name)
        4.  is_documented_option    (has description or help_text)
        5.  is_defaultable_option   (has default_value)
        6.  is_required_option      (has required member)
        7.  is_constrained_option   (has validate or bounds or choices)
        8.  is_bounded_option       (has min_value and max_value)
        9.  is_stepped_option       (bounded + step)
        10. is_categorized_option   (has category member)
        11. is_env_mapped_option    (has env_var member)
        12. is_fully_documented     (description + default + named)
        13. is_self_validating      (validate + bounded)
        14. option_known_column_count (count of detected columns)
VII.    ENTRY COMPATIBILITY TRAITS
        1.  options_share_key_type      (same key type)
        2.  options_share_value_type    (same value type)
        3.  options_are_compatible      (same key + value types)
        4.  option_is_superset_of       (all columns of _Sub + more)
VIII.   CONVENIENCE ALIASES (sections I-VII)
*/

#ifndef DJINTERP_OPTION_PAIR_TRAITS_
#define DJINTERP_OPTION_PAIR_TRAITS_ 1

#include <cstddef>
#include <string>
#include <type_traits>
#include "../djinterp.hpp"
#include "../meta/type_traits.hpp"

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    #include <string_view>
#endif


NS_DJINTERP

// forward-declare option_pair for is_option_pair detection
template<typename _Key,
         typename _Value>
struct option_pair;


///////////////////////////////////////////////////////////////////////////////
///        I.    ENTRY-LEVEL EXPRESSION ALIASES                             ///
///////////////////////////////////////////////////////////////////////////////
// Each alias attempts to form a type from a member access
// expression on _Type.  If the member does not exist, the
// expression is ill-formed and the detector maps it to
// false_type / nonesuch.  Users add new column detectors by
// following this pattern with their own member names.

NS_INTERNAL

    // mandatory base members
    // ----------------------------------------------------------------
    // key_expr
    //   detector: expression alias for .key member.
    template<typename _Type>
    using key_expr = decltype(std::declval<const _Type&>().key);

    // value_expr
    //   detector: expression alias for .value member.
    template<typename _Type>
    using value_expr = decltype(std::declval<const _Type&>().value);

NS_END  // internal


///////////////////////////////////////////////////////////////////////////////
///        II.   ENTRY-LEVEL has_* TRAITS                                   ///
///////////////////////////////////////////////////////////////////////////////

// ----------------------------------------------------------------
// mandatory base members
// ----------------------------------------------------------------

// has_key
//   trait: detects whether _Type has a .key member.
template<typename _Type>
struct has_key
{
    static constexpr bool value = is_detected<internal::key_expr, _Type>::value;
};

// has_value
//   trait: detects whether _Type has a .value member.
template<typename _Type>
struct has_value
{
    static constexpr bool value = is_detected<internal::value_expr, _Type>::value;
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
template<typename _Type>
using option_key_t = detected_or_t<nonesuch, internal::key_expr, _Type>;

// option_value_t
//   type: extracts the value type from an option entry, or
// nonesuch if unavailable.
template<typename _Type>
using option_value_t = detected_or_t<nonesuch, internal::value_expr, _Type>;

///////////////////////////////////////////////////////////////////////////////
///        IV.   KEY TYPE CLASSIFICATION                                    ///
///////////////////////////////////////////////////////////////////////////////
// Classifies the key type itself (when detectable) to enable
// generic algorithms that branch on key representation.
// All traits operate on clean_t<option_key_t<_Type>> and are
// false when .key is absent.

NS_INTERNAL

    // key_type_or_void
    //   helper: resolves to clean_t of the key type when
    // present, or void when absent, so that downstream
    // type queries can safely instantiate.
    template<typename _Type,
             bool = has_key<_Type>::value>
    struct key_type_or_void
    {
        using type = void;
    };

    template<typename _Type>
    struct key_type_or_void<_Type, true>
    {
        using type = typename std::remove_cv<
                typename std::remove_reference<option_key_t<_Type>>::type
        >::type;
    };

    // string_like_check
    //   helper: true when _Key is const char*, char*,
    // std::string, or std::string_view (or cv-qualified
    // variants thereof).  string_view detection is gated
    // behind C++17.
    template<typename _Key>
    struct string_like_check
    {
    private:
        using bare = typename std::remove_cv<
            typename std::remove_pointer<
                typename std::decay<_Key>::type>::type>::type;

        static constexpr bool is_char_ptr =
            ( std::is_same<bare, char>::value    ||
              std::is_same<bare, wchar_t>::value );

        static constexpr bool is_std_string =
            ( std::is_same<_Key, std::string>::value       ||
              std::is_same<_Key, const std::string>::value );

    #if D_ENV_LANG_IS_CPP17_OR_HIGHER
        static constexpr bool is_string_view =
            ( std::is_same<_Key, std::string_view>::value       ||
              std::is_same<_Key, const std::string_view>::value );
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
    //   helper: true when a < b is well-formed for _Key.
    template<typename _Key>
    using less_than_expr = decltype(
        std::declval<const _Key&>() < std::declval<const _Key&>());

    // equality_check
    //   helper: true when a == b is well-formed for _Key.
    template<typename _Key>
    using equality_expr = decltype(
        std::declval<const _Key&>() ==
            std::declval<const _Key&>());

NS_END  // internal

// has_string_key
//   trait: true when _Type has a .key whose type is string-like
// (const char*, std::string, char[], wchar_t variants).
template<typename _Type>
struct has_string_key
{
private:
    using keycleaned =
        typename internal::key_type_or_void<_Type>::type;

public:
    static constexpr bool value =
        ( has_key<_Type>::value &&
          internal::string_like_check<keycleaned>::value );
};

// has_enum_key
//   trait: true when _Type has a .key whose type is an
// enumeration (scoped or unscoped).
template<typename _Type>
struct has_enum_key
{
private:
    using keycleaned =
        typename internal::key_type_or_void<_Type>::type;

public:
    static constexpr bool value =
        ( has_key<_Type>::value &&
          std::is_enum<keycleaned>::value );
};

// has_scoped_enum_key
//   trait: true when _Type has a .key whose type is a scoped
// enumeration (enum class / enum struct).  Uses the portable
// is_scoped_enum from type_traits.hpp.
template<typename _Type>
struct has_scoped_enum_key
{
private:
    using keycleaned =
        typename internal::key_type_or_void<_Type>::type;

public:
    static constexpr bool value =
        ( has_key<_Type>::value &&
          is_scoped_enum<keycleaned>::value );
};

// has_integral_key
//   trait: true when _Type has a .key whose type is integral
// (int, unsigned, short, long, etc.) but not bool.
template<typename _Type>
struct has_integral_key
{
private:
    using keycleaned =
        typename internal::key_type_or_void<_Type>::type;

public:
    static constexpr bool value =
        ( has_key<_Type>::value                          &&
          std::is_integral<keycleaned>::value           &&
          !std::is_same<keycleaned, bool>::value );
};

// has_comparable_key
//   trait: true when _Type has a .key that supports operator<.
// Required for sorted option_set instances.
template<typename _Type>
struct has_comparable_key
{
private:
    using keycleaned =
        typename internal::key_type_or_void<_Type>::type;

public:
    static constexpr bool value =
        ( has_key<_Type>::value &&
          is_detected<internal::less_than_expr,
                      keycleaned>::value );
};

// has_equality_key
//   trait: true when _Type has a .key that supports operator==.
// Required for any lookup operation.
template<typename _Type>
struct has_equality_key
{
private:
    using keycleaned =
        typename internal::key_type_or_void<_Type>::type;

public:
    static constexpr bool value =
        ( has_key<_Type>::value &&
          is_detected<internal::equality_expr,
                      keycleaned>::value );
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
    template<typename _Type,
             bool = has_value<_Type>::value>
    struct value_type_or_void
    {
        using type = void;
    };

    template<typename _Type>
    struct value_type_or_void<_Type, true>
    {
        using type =
            typename std::remove_cv<
                typename std::remove_reference<
                    option_value_t<_Type>>::type>::type;
    };

    // is_any_check
    //   helper: detects whether _Value is compat::any by
    // checking for the holds<T>() member template.
    // This avoids a hard dependency on compat/std/any.hpp.
    template<typename _Value>
    using any_holds_expr = decltype(
        std::declval<const _Value&>().template holds<int>());

NS_END  // internal

// has_boolean_value
//   trait: true when _Type has a .value of type bool.
// Enables flag-style option handling (--verbose toggles
// a boolean).
template<typename _Type>
struct has_boolean_value
{
private:
    using valcleaned =
        typename internal::value_type_or_void<_Type>::type;

public:
    static constexpr bool value =
        ( has_value<_Type>::value &&
          std::is_same<valcleaned, bool>::value );
};

// has_arithmetic_value
//   trait: true when _Type has a .value of arithmetic type
// (integral or floating-point, including bool).
template<typename _Type>
struct has_arithmetic_value
{
private:
    using valcleaned =
        typename internal::value_type_or_void<_Type>::type;

public:
    static constexpr bool value =
        ( has_value<_Type>::value &&
          std::is_arithmetic<valcleaned>::value );
};

// has_string_value
//   trait: true when _Type has a .value whose type is
// string-like.
template<typename _Type>
struct has_string_value
{
private:
    using valcleaned =
        typename internal::value_type_or_void<_Type>::type;

public:
    static constexpr bool value =
        ( has_value<_Type>::value &&
          internal::string_like_check<valcleaned>::value );
};

// has_any_value
//   trait: true when _Type has a .value whose type is a
// type-erased any container (detected structurally via
// the holds<T>() member template).  Indicates the entry
// carries a heterogeneous value.
template<typename _Type>
struct has_any_value
{
private:
    using valcleaned =
        typename internal::value_type_or_void<_Type>::type;

public:
    static constexpr bool value =
        ( has_value<_Type>::value &&
          is_detected<internal::any_holds_expr,
                      valcleaned>::value );
};

// has_enum_value
//   trait: true when _Type has a .value whose type is an
// enumeration (scoped or unscoped).
template<typename _Type>
struct has_enum_value
{
private:
    using valcleaned =
        typename internal::value_type_or_void<_Type>::type;

public:
    static constexpr bool value =
        ( has_value<_Type>::value &&
          std::is_enum<valcleaned>::value );
};

// has_pointer_value
//   trait: true when _Type has a .value whose type is a pointer.
template<typename _Type>
struct has_pointer_value
{
private:
    using valcleaned =
        typename internal::value_type_or_void<_Type>::type;

public:
    static constexpr bool value =
        ( has_value<_Type>::value &&
          std::is_pointer<valcleaned>::value );
};


///////////////////////////////////////////////////////////////////////////////
///        VI.   COMPOSITE ENTRY TRAITS                                     ///
///////////////////////////////////////////////////////////////////////////////

// is_option_entry
//   trait: true when _Type satisfies the minimum option entry
// contract (has .key and .value members).
template<typename _Type>
struct is_option_entry
{
private:
    using clean_type = clean_t<_Type>;

public:
    static constexpr bool value =
        ( has_key<clean_type>::value &&
          has_value<clean_type>::value );
};

// is_option_pair
//   trait: true when _Type is a specialization of option_pair.
// Primary template: false.
template<typename _Type>
struct is_option_pair : std::false_type
{};

// is_option_pair<option_pair<K,V>>
//   trait: specialization recognizing option_pair.
template<typename _Key,
         typename _Value>
struct is_option_pair<option_pair<_Key, _Value>> : std::true_type
{};

///////////////////////////////////////////////////////////////////////////////
///        VII.  ENTRY COMPATIBILITY TRAITS                                 ///
///////////////////////////////////////////////////////////////////////////////

// options_share_key_type
//   trait: true when both _E1 and _E2 have .key members of the same type.
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


///////////////////////////////////////////////////////////////////////////////
///        VIII. CONVENIENCE ALIASES                                        ///
///////////////////////////////////////////////////////////////////////////////

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    // ================================================================
    // key / value classification _v aliases
    // ================================================================

    template<typename _Type>
    constexpr bool has_string_key_v =
        has_string_key<_Type>::value;

    template<typename _Type>
    constexpr bool has_enum_key_v = has_enum_key<_Type>::value;

    template<typename _Type>
    constexpr bool has_scoped_enum_key_v =
        has_scoped_enum_key<_Type>::value;

    template<typename _Type>
    constexpr bool has_integral_key_v =
        has_integral_key<_Type>::value;

    template<typename _Type>
    constexpr bool has_comparable_key_v =
        has_comparable_key<_Type>::value;

    template<typename _Type>
    constexpr bool has_equality_key_v =
        has_equality_key<_Type>::value;

    template<typename _Type>
    constexpr bool has_boolean_value_v =
        has_boolean_value<_Type>::value;

    template<typename _Type>
    constexpr bool has_arithmetic_value_v =
        has_arithmetic_value<_Type>::value;

    template<typename _Type>
    constexpr bool has_string_value_v =
        has_string_value<_Type>::value;

    template<typename _Type>
    constexpr bool has_any_value_v =
        has_any_value<_Type>::value;

    template<typename _Type>
    constexpr bool has_enum_value_v =
        has_enum_value<_Type>::value;

    template<typename _Type>
    constexpr bool has_pointer_value_v =
        has_pointer_value<_Type>::value;

    // ================================================================
    // composite entry _v aliases
    // ================================================================

    template<typename _Type>
    constexpr bool is_option_entry_v =
        is_option_entry<_Type>::value;

    template<typename _Type>
    constexpr bool is_option_pair_v =
        is_option_pair<_Type>::value;
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

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES


NS_END  // djinterp


#endif  // DJINTERP_OPTION_PAIR_TRAITS_