/******************************************************************************
* djinterp [traits]                                            cli_traits.hpp
*
* SFINAE-based traits for compile-time CLI configurability detection.
*   Provides type traits that detect whether a container or user-defined
* type exposes the structural members required for CLI-driven compile-time
* configuration.  Detection is performed via the void_t / decltype SFINAE
* idiom without concepts or tag dispatching.  Every trait is a zero-cost
* compile-time computation; no code is emitted at runtime.
*
* DETECTED MEMBERS:
*   _T::options_type           — nested alias naming the flag enum
*   _T::option_flags           — static constexpr resolved flag set
*   _T::default_options        — static constexpr fallback flag set
*   _T::option_table()         — constexpr accessor to string_kv table
*   _T::option_flags_count     — static constexpr count of recognized
*                                 flag values
*
* TABLE OF CONTENTS
* =================
* I.    VOID_T HELPER
*       1.  void_t_impl  (internal helper struct)
*       2.  void_t       (public alias)
*
* II.   EXPRESSION ALIASES (internal)
*       1.  options_type_expr
*       2.  option_flags_expr
*       3.  default_options_expr
*       4.  option_table_expr
*       5.  option_flags_count_expr
*
* III.  ATOMIC DETECTION TRAITS
*       1.  has_options_type       (primary + specialization)
*       2.  has_option_flags       (primary + specialization)
*       3.  has_default_options    (primary + specialization)
*       4.  has_option_table       (primary + specialization)
*       5.  has_option_flags_count (primary + specialization)
*
* IV.   COMPOSITE TRAITS
*       1.  is_option_constructible
*       2.  is_cli_configurable
*       3.  is_self_describing
*
* V.    CONVENIENCE ALIASES (_v variable templates)
*       1.  has_options_type_v
*       2.  has_option_flags_v
*       3.  has_default_options_v
*       4.  has_option_table_v
*       5.  is_option_constructible_v
*       6.  is_cli_configurable_v
*       7.  is_self_describing_v
*
* VI.   SFINAE GUARDS (enable_if aliases)
*       1.  enable_if_option_constructible
*       2.  enable_if_cli_configurable
*       3.  enable_if_has_defaults
*
*
* path:      /inc/traits/cli_traits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.02.15
******************************************************************************/

#ifndef DJINTERP_TRAITS_CLI_TRAITS_
#define DJINTERP_TRAITS_CLI_TRAITS_ 1

#include <cstddef>
#include <type_traits>


///////////////////////////////////////////////////////////////////////////////
///        I.    VOID_T HELPER                                              ///
///////////////////////////////////////////////////////////////////////////////
// Re-declared locally so this header remains self-contained.
// Identical definitions merge harmlessly across translation
// units via the ODR.

namespace internal {

    // void_t_impl
    //   trait: maps an arbitrary type parameter pack to void.
    // Acts as the SFINAE injection point for the detector
    // pattern: when any type in _Ts... is ill-formed, the
    // partial specialization that depends on void_t_impl is
    // silently discarded.
    template<typename... _Ts>
    struct void_t_impl
    {
        using type = void;
    };

} // namespace internal

// void_t
//   type: convenience alias that resolves to void when every
// type in _Ts... is well-formed.  This is the public entry
// point used by every detection trait in this header to trigger
// SFINAE substitution failure on missing members.
template<typename... _Ts>
using void_t = typename internal::void_t_impl<_Ts...>::type;


///////////////////////////////////////////////////////////////////////////////
///        II.   EXPRESSION ALIASES (internal)                              ///
///////////////////////////////////////////////////////////////////////////////
// Each alias names a single expression whose well-formedness is
// the condition for the corresponding detection trait in section
// III.  Keeping them separate from the traits themselves allows
// each trait to remain a clean primary-template / specialization
// pair with no inline decltype noise.

namespace internal {

    // options_type_expr
    //   type: resolves to _T::options_type if that nested alias
    // exists.  Used by has_options_type to detect whether _T
    // declares a flag enum type for its configuration axis.
    template<typename _T>
    using options_type_expr = typename _T::options_type;

    // option_flags_expr
    //   type: resolves to the type of _T::option_flags if that
    // static constexpr member exists.  The std::declval<const
    // _T*>() is a dependent-context anchor that prevents eager
    // evaluation; the comma operator discards it and yields the
    // member's type.  Used by has_option_flags.
    template<typename _T>
    using option_flags_expr =
        decltype(std::declval<const _T*>(), _T::option_flags);

    // default_options_expr
    //   type: resolves to the type of _T::default_options if
    // that static constexpr member exists.  Used by
    // has_default_options to detect whether _T provides a
    // fallback flag set for when the user supplies none.
    template<typename _T>
    using default_options_expr =
        decltype(std::declval<const _T*>(), _T::default_options);

    // option_table_expr
    //   type: resolves to the return type of _T::option_table()
    // if that static constexpr method exists.  Used by
    // has_option_table to detect whether _T exposes a
    // string_kv lookup table for CLI string resolution.
    template<typename _T>
    using option_table_expr =
        decltype(_T::option_table());

    // option_flags_count_expr
    //   type: resolves to the type of _T::option_flags_count if
    // that static constexpr member exists.  Used by
    // has_option_flags_count to detect whether _T advertises
    // how many discrete flag values it recognizes, enabling
    // complete enumeration at compile time.
    template<typename _T>
    using option_flags_count_expr =
        decltype(std::declval<const _T*>(),
                 _T::option_flags_count);

} // namespace internal


///////////////////////////////////////////////////////////////////////////////
///        III.  ATOMIC DETECTION TRAITS                                    ///
///////////////////////////////////////////////////////////////////////////////
// Each trait follows the same two-template pattern:
//   1. A primary template inheriting std::false_type (the
//      "not detected" case).
//   2. A partial specialization whose second parameter is
//      void_t<expression_alias<_T>>.  When the expression is
//      well-formed, void_t yields void, which matches the
//      specialization and selects std::true_type.  When the
//      expression is ill-formed, SFINAE discards the
//      specialization and the primary template wins.

// has_options_type
//   trait: primary template for detecting _T::options_type
// (failure case).  Inherits std::false_type, indicating that
// _T does not expose a nested options_type alias naming its
// configuration enum.
template<typename _T,
         typename = void>
struct has_options_type : std::false_type
{
};

// has_options_type (success case)
//   trait: partial specialization selected when
// _T::options_type is a well-formed type expression.
// Inherits std::true_type.
template<typename _T>
struct has_options_type<
    _T,
    void_t<internal::options_type_expr<_T>>
> : std::true_type
{
};

// has_option_flags
//   trait: primary template for detecting _T::option_flags
// (failure case).  Inherits std::false_type, indicating that
// _T does not expose a static constexpr option_flags member
// holding the resolved compile-time flag set.
template<typename _T,
         typename = void>
struct has_option_flags : std::false_type
{
};

// has_option_flags (success case)
//   trait: partial specialization selected when
// _T::option_flags is a well-formed expression.
// Inherits std::true_type.
template<typename _T>
struct has_option_flags<
    _T,
    void_t<internal::option_flags_expr<_T>>
> : std::true_type
{
};

// has_default_options
//   trait: primary template for detecting _T::default_options
// (failure case).  Inherits std::false_type, indicating that
// _T does not expose a static constexpr default_options member
// providing the fallback flag set applied when no flags are
// supplied by the user.
template<typename _T,
         typename = void>
struct has_default_options : std::false_type
{
};

// has_default_options (success case)
//   trait: partial specialization selected when
// _T::default_options is a well-formed expression.
// Inherits std::true_type.
template<typename _T>
struct has_default_options<
    _T,
    void_t<internal::default_options_expr<_T>>
> : std::true_type
{
};

// has_option_table
//   trait: primary template for detecting _T::option_table()
// (failure case).  Inherits std::false_type, indicating that
// _T does not expose a static constexpr method returning a
// reference to a string_kv table for CLI string resolution.
template<typename _T,
         typename = void>
struct has_option_table : std::false_type
{
};

// has_option_table (success case)
//   trait: partial specialization selected when the call
// expression _T::option_table() is well-formed.
// Inherits std::true_type.
template<typename _T>
struct has_option_table<
    _T,
    void_t<internal::option_table_expr<_T>>
> : std::true_type
{
};

// has_option_flags_count
//   trait: primary template for detecting
// _T::option_flags_count (failure case).  Inherits
// std::false_type, indicating that _T does not expose a
// static constexpr member advertising how many discrete flag
// values it recognizes.
template<typename _T,
         typename = void>
struct has_option_flags_count : std::false_type
{
};

// has_option_flags_count (success case)
//   trait: partial specialization selected when
// _T::option_flags_count is a well-formed expression.
// Inherits std::true_type.
template<typename _T>
struct has_option_flags_count<
    _T,
    void_t<internal::option_flags_count_expr<_T>>
> : std::true_type
{
};


///////////////////////////////////////////////////////////////////////////////
///        IV.   COMPOSITE TRAITS                                          ///
///////////////////////////////////////////////////////////////////////////////
// These combine the atomic traits from section III into
// higher-level predicates that answer broader structural
// questions.  Each stores its result in a static constexpr
// bool `value` member following the standard type-trait
// convention.

// is_option_constructible
//   trait: true when _T is parameterized by compile-time
// option flags.  Requires both an options_type alias (naming
// the enum type) and an option_flags member (holding the
// resolved flag value for this instantiation).  A type that
// satisfies this trait can be queried for its compile-time
// configuration but may not yet support CLI string resolution
// or default fallback.
template<typename _T>
struct is_option_constructible
{
    static constexpr bool value =
        ( has_options_type<_T>::value &&
          has_option_flags<_T>::value );
};

// is_cli_configurable
//   trait: true when _T can be fully driven from CLI strings.
// Requires option constructibility (options_type + option_flags),
// a default_options member (so unspecified axes can fall back to
// sensible defaults), and an option_table() method (so that
// human-readable strings like "writable" or "ordered" can be
// resolved to flag bits via the string_kv lookup machinery in
// container_options.hpp).  Any container satisfying this trait
// can participate in a CLI argument parser without additional
// boilerplate.
template<typename _T>
struct is_cli_configurable
{
    static constexpr bool value =
        ( is_option_constructible<_T>::value &&
          has_default_options<_T>::value     &&
          has_option_table<_T>::value );
};

// is_self_describing
//   trait: true when _T provides both a string_kv table and a
// count of recognized flag values, enabling compile-time
// enumeration of all valid option strings.  Useful for
// generating help text, validation messages, or shell
// completion lists without hard-coding option names outside
// the container itself.
template<typename _T>
struct is_self_describing
{
    static constexpr bool value =
        ( has_option_table<_T>::value       &&
          has_option_flags_count<_T>::value );
};


///////////////////////////////////////////////////////////////////////////////
///        V.    CONVENIENCE ALIASES                                        ///
///////////////////////////////////////////////////////////////////////////////
// Variable templates providing shorthand access to the ::value
// member of each trait.  These follow the standard library
// convention of suffixing the trait name with `_v`.

// has_options_type_v
//   constant: shorthand for has_options_type<_T>::value.
template<typename _T>
static constexpr bool has_options_type_v =
    has_options_type<_T>::value;

// has_option_flags_v
//   constant: shorthand for has_option_flags<_T>::value.
template<typename _T>
static constexpr bool has_option_flags_v =
    has_option_flags<_T>::value;

// has_default_options_v
//   constant: shorthand for has_default_options<_T>::value.
template<typename _T>
static constexpr bool has_default_options_v =
    has_default_options<_T>::value;

// has_option_table_v
//   constant: shorthand for has_option_table<_T>::value.
template<typename _T>
static constexpr bool has_option_table_v =
    has_option_table<_T>::value;

// is_option_constructible_v
//   constant: shorthand for
// is_option_constructible<_T>::value.
template<typename _T>
static constexpr bool is_option_constructible_v =
    is_option_constructible<_T>::value;

// is_cli_configurable_v
//   constant: shorthand for
// is_cli_configurable<_T>::value.
template<typename _T>
static constexpr bool is_cli_configurable_v =
    is_cli_configurable<_T>::value;

// is_self_describing_v
//   constant: shorthand for
// is_self_describing<_T>::value.
template<typename _T>
static constexpr bool is_self_describing_v =
    is_self_describing<_T>::value;


///////////////////////////////////////////////////////////////////////////////
///        VI.   SFINAE GUARDS                                             ///
///////////////////////////////////////////////////////////////////////////////
// Type aliases wrapping std::enable_if for gating free function
// overloads or class template specializations on CLI-related
// properties.  When the condition is false the alias is
// ill-formed, removing the overload or specialization from the
// candidate set via SFINAE.
//
// Usage:
//   template<typename _T,
//            typename = enable_if_cli_configurable<_T>>
//   void configure_from_cli(_T& _container,
//                           int _argc,
//                           const char** _argv);

// enable_if_option_constructible
//   type: SFINAE guard that produces void when _T satisfies
// is_option_constructible (exposes both options_type and
// option_flags).  Ill-formed otherwise, removing the
// dependent declaration from overload resolution.
template<typename _T>
using enable_if_option_constructible =
    typename std::enable_if<
        is_option_constructible<_T>::value
    >::type;

// enable_if_cli_configurable
//   type: SFINAE guard that produces void when _T satisfies
// is_cli_configurable (option constructible + default options
// + string table).  Ill-formed otherwise.
template<typename _T>
using enable_if_cli_configurable =
    typename std::enable_if<
        is_cli_configurable<_T>::value
    >::type;

// enable_if_has_defaults
//   type: SFINAE guard that produces void when _T satisfies
// has_default_options (exposes a static constexpr
// default_options member).  Ill-formed otherwise.
template<typename _T>
using enable_if_has_defaults =
    typename std::enable_if<
        has_default_options<_T>::value
    >::type;


#endif  // DJINTERP_TRAITS_CLI_TRAITS_
