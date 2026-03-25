/******************************************************************************
* djinterp [container]                             container_option_traits.hpp
*
* Container-aware option traits for the djinterp framework.
*   Bridges the container classification system (container_traits.hpp)
* with the option_set trait system (option_set_traits.hpp) to provide
* compile-time detection of how containers relate to options:
*
*   1. Option storage:     the container IS an option_set or stores
*      option entries.
*   2. Configurable:       the container can be configured by
*      applying an option_set to it (via .apply_options(),
*      .configure(), or constructor-from-options).
*   3. Option-producing:   the container can export its current
*      configuration as an option_set (via .to_options() or
*      .get_options()).
*   4. CLI-parseable:      the container's options can be populated
*      from string key=value pairs (suitable for main() argv or
*      other CLI contexts).
*
*   All detection is purely structural SFINAE.  The option_set_traits
* from djinterp:: are reused for entry-level and container-level
* option_set detection.
*
* DEPENDENCIES:
*   container_traits.hpp     - container classification
*   option_set_traits.hpp    - option entry / option_set detection
*
* TABLE OF CONTENTS
* =================
* I.      Option Storage Detection
* II.     Configurable Detection
* III.    Option-Producing Detection
* IV.     CLI Protocol Detection
* V.      Options Template Construction Detection
* VI.     Strategy Classification
* VII.    Convenience Predicates
* VIII.   Combined Classification
*
*
* path:      /inc/container/meta/container_option_traits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.03.23
******************************************************************************/

#ifndef DJINTERP_CONTAINER_OPTION_TRAITS_
#define DJINTERP_CONTAINER_OPTION_TRAITS_ 1

#include <cstddef>
#include <string>
#include <type_traits>
#include "../djinterp.hpp"
#include "../type_traits.hpp"
#include "container_traits.hpp"
#include "../../options/option_set_traits.hpp"


NS_DJINTERP
NS_CONTAINER
NS_TRAITS

// =============================================================================
// I.   Option Storage Detection
// =============================================================================
// Determines whether a container IS an option_set or stores
// option entries as its elements.

NS_INTERNAL

    // safe_value_type (self-contained)
    template<typename _Type, typename = void>
    struct opt_safe_value_type
    {
        using type = void;
    };

    template<typename _Type>
    struct opt_safe_value_type<_Type,
        std::void_t<typename _Type::value_type>>
    {
        using type = typename _Type::value_type;
    };

    template<typename _Type>
    using opt_safe_value_type_t =
        typename opt_safe_value_type<_Type>::type;

NS_END  // internal

// is_option_set_container
//   type trait: true if the container itself satisfies the
// option_set contract (keyed lookup + iteration +
// key_type alias).
// Delegates to djinterp::is_option_set from
// option_set_traits.hpp.
template<typename _Type>
struct is_option_set_container
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        djinterp::is_option_set<clean_type>::value;
};

template<typename _Type>
inline constexpr bool is_option_set_container_v =
    is_option_set_container<_Type>::value;

// has_option_entry_elements
//   type trait: true if the container's value_type satisfies
// the option entry contract (.key + .value).
// Delegates to djinterp::is_option_entry.
template<typename _Type>
struct has_option_entry_elements
{
    using clean_type = clean_t<_Type>;
    using elem_type  =
        internal::opt_safe_value_type_t<clean_type>;

    static constexpr bool value =
        djinterp::is_option_entry<elem_type>::value;
};

template<typename _Type>
inline constexpr bool has_option_entry_elements_v =
    has_option_entry_elements<_Type>::value;

// has_named_option_elements
//   type trait: true if the container's value_type has
// CLI naming members (short_name or long_name).
template<typename _Type>
struct has_named_option_elements
{
    using clean_type = clean_t<_Type>;
    using elem_type  =
        internal::opt_safe_value_type_t<clean_type>;

    static constexpr bool value =
        djinterp::is_named_option<elem_type>::value;
};

template<typename _Type>
inline constexpr bool has_named_option_elements_v =
    has_named_option_elements<_Type>::value;

// has_documented_option_elements
//   type trait: true if the container's value_type has
// documentation members (description or help_text).
template<typename _Type>
struct has_documented_option_elements
{
    using clean_type = clean_t<_Type>;
    using elem_type  =
        internal::opt_safe_value_type_t<clean_type>;

    static constexpr bool value =
        djinterp::is_documented_option<
            elem_type>::value;
};

template<typename _Type>
inline constexpr bool has_documented_option_elements_v =
    has_documented_option_elements<_Type>::value;

// has_constrained_option_elements
//   type trait: true if the container's value_type has
// constraint members (validate, bounds, or choices).
template<typename _Type>
struct has_constrained_option_elements
{
    using clean_type = clean_t<_Type>;
    using elem_type  =
        internal::opt_safe_value_type_t<clean_type>;

    static constexpr bool value =
        djinterp::is_constrained_option<
            elem_type>::value;
};

template<typename _Type>
inline constexpr bool
    has_constrained_option_elements_v =
        has_constrained_option_elements<
            _Type>::value;

// has_env_mapped_option_elements
//   type trait: true if the container's value_type has
// an env_var member for environment-variable fallback.
template<typename _Type>
struct has_env_mapped_option_elements
{
    using clean_type = clean_t<_Type>;
    using elem_type  =
        internal::opt_safe_value_type_t<clean_type>;

    static constexpr bool value =
        djinterp::is_env_mapped_option<
            elem_type>::value;
};

template<typename _Type>
inline constexpr bool
    has_env_mapped_option_elements_v =
        has_env_mapped_option_elements<
            _Type>::value;


// =============================================================================
// II.  Configurable Detection
// =============================================================================
// Detects whether a container can receive configuration from
// an option_set.

// has_apply_options_method
//   type trait: true if container has an
// .apply_options(option_set) method.
D_TYPE_TRAIT_TRUE(has_apply_options_method,
    decltype(&_Type::apply_options))

// has_configure_method
//   type trait: true if container has a .configure(...)
// method for applying settings.
D_TYPE_TRAIT_TRUE(has_configure_method,
    decltype(&_Type::configure))

// has_set_option_method
//   type trait: true if container has a
// .set_option(key, value) method for individual option
// updates.
D_TYPE_TRAIT_TRUE(has_set_option_method,
    decltype(&_Type::set_option))

// has_get_option_method
//   type trait: true if container has a
// .get_option(key) method for querying individual
// option values.
D_TYPE_TRAIT_TRUE(has_get_option_method,
    decltype(&_Type::get_option))

// is_configurable_container
//   type trait: true if container can be configured via
// any supported mechanism.
template<typename _Type>
struct is_configurable_container
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        ( has_apply_options_method_v<clean_type> ||
          has_configure_method_v<clean_type>     ||
          has_set_option_method_v<clean_type> );
};

template<typename _Type>
inline constexpr bool is_configurable_container_v =
    is_configurable_container<_Type>::value;


// =============================================================================
// III. Option-Producing Detection
// =============================================================================
// Detects whether a container can export its current
// configuration as an option_set.

// has_to_options_method
//   type trait: true if container has a .to_options() method
// that returns an option_set or collection of option entries.
D_TYPE_TRAIT_TRUE(has_to_options_method,
    decltype(std::declval<const _Type&>().to_options()))

// has_get_options_method
//   type trait: true if container has a .get_options() method
// returning the current configuration as options.
D_TYPE_TRAIT_TRUE(has_get_options_method,
    decltype(
        std::declval<const _Type&>().get_options()))

// is_option_producing_container
//   type trait: true if container can export options via any
// mechanism.
template<typename _Type>
struct is_option_producing_container
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        ( has_to_options_method_v<clean_type> ||
          has_get_options_method_v<clean_type> );
};

template<typename _Type>
inline constexpr bool is_option_producing_container_v =
    is_option_producing_container<_Type>::value;


// =============================================================================
// IV.  CLI Protocol Detection
// =============================================================================
// Detects whether options can be populated from string
// key=value pairs (for main() argv, config file parsing,
// or interactive CLI contexts).

// has_parse_option_method
//   type trait: true if container has a
// .parse_option(string) method that accepts a single
// "key=value" CLI argument.
D_TYPE_TRAIT_TRUE(has_parse_option_method,
    decltype(std::declval<_Type&>().parse_option(
        std::declval<const std::string&>())))

// has_parse_args_method
//   type trait: true if container has a
// .parse_args(argc, argv) method for bulk CLI argument
// parsing.
D_TYPE_TRAIT_TRUE(has_parse_args_method,
    decltype(std::declval<_Type&>().parse_args(
        std::declval<int>(),
        std::declval<const char* const*>())))

// has_parse_config_method
//   type trait: true if container has a
// .parse_config(string) method for parsing a config file
// or multi-line config block.
D_TYPE_TRAIT_TRUE(has_parse_config_method,
    decltype(std::declval<_Type&>().parse_config(
        std::declval<const std::string&>())))

// has_help_string_method
//   type trait: true if container can generate a help/usage
// string from its options.
D_TYPE_TRAIT_TRUE(has_help_string_method,
    decltype(
        std::declval<const _Type&>().help_string()))

// is_cli_capable_container
//   type trait: true if container supports CLI-style option
// parsing via any mechanism.
template<typename _Type>
struct is_cli_capable_container
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        ( has_parse_option_method_v<clean_type> ||
          has_parse_args_method_v<clean_type>   ||
          has_parse_config_method_v<clean_type> );
};

template<typename _Type>
inline constexpr bool is_cli_capable_container_v =
    is_cli_capable_container<_Type>::value;


// =============================================================================
// V.   Options Template Construction Detection
// =============================================================================
// Detects whether a container can be constructed or
// initialized from an options template.

NS_INTERNAL

    // constructible_from_option_set_check
    //   helper: detects if _Type has a constructor or
    // static factory accepting an option_set-like argument.
    template<typename _Type, typename = void>
    struct has_from_options_check : std::false_type
    {};

    template<typename _Type>
    struct has_from_options_check<_Type,
        std::void_t<decltype(&_Type::from_options)>>
        : std::true_type
    {};

NS_END  // internal

// has_from_options_factory
//   type trait: true if container has a static
// .from_options() factory method.
template<typename _Type>
struct has_from_options_factory
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        internal::has_from_options_check<
            clean_type>::value;
};

template<typename _Type>
inline constexpr bool has_from_options_factory_v =
    has_from_options_factory<_Type>::value;


// =============================================================================
// VI.  Strategy Classification
// =============================================================================

// --- configure strategy ---

// DOptionConfigureStrategy
//   enum: compile-time configuration strategy tags.
enum class DOptionConfigureStrategy
{
    // container has .apply_options(option_set) — delegate
    native_apply,

    // container has .configure() — delegate
    native_configure,

    // container has .set_option(key, value) — iterate
    // option_set and apply each entry individually
    per_option,

    // container is itself an option_set — merge
    option_set_merge,

    // no configuration path available
    unsupported
};

NS_INTERNAL

    template<typename _Type>
    struct option_configure_strategy_impl
    {
        using clean_type = clean_t<_Type>;

        static constexpr DOptionConfigureStrategy value =
            has_apply_options_method_v<clean_type>
                ? DOptionConfigureStrategy::
                      native_apply

            : has_configure_method_v<clean_type>
                ? DOptionConfigureStrategy::
                      native_configure

            : has_set_option_method_v<clean_type>
                ? DOptionConfigureStrategy::per_option

            : is_option_set_container_v<clean_type>
                ? DOptionConfigureStrategy::
                      option_set_merge

            : DOptionConfigureStrategy::unsupported;
    };

NS_END  // internal

// container_option_configure_strategy
//   type trait: determines the best configuration strategy.
template<typename _Type>
struct container_option_configure_strategy
{
    static constexpr DOptionConfigureStrategy value =
        internal::option_configure_strategy_impl<
            _Type>::value;
};

template<typename _Type>
inline constexpr DOptionConfigureStrategy
    container_option_configure_strategy_v =
        container_option_configure_strategy<
            _Type>::value;

// --- CLI parse strategy ---

// DOptionParseStrategy
//   enum: compile-time CLI parse strategy tags.
enum class DOptionParseStrategy
{
    // container has .parse_args(argc, argv) — bulk
    native_args,

    // container has .parse_option(string) — one at a time
    native_single,

    // container has .parse_config(string) — config block
    native_config,

    // container is an option_set with string keys —
    // generic key=value parsing
    generic_kv,

    // no parse path available
    unsupported
};

NS_INTERNAL

    template<typename _Type>
    struct option_parse_strategy_impl
    {
        using clean_type = clean_t<_Type>;

        static constexpr DOptionParseStrategy value =
            has_parse_args_method_v<clean_type>
                ? DOptionParseStrategy::native_args

            : has_parse_option_method_v<clean_type>
                ? DOptionParseStrategy::native_single

            : has_parse_config_method_v<clean_type>
                ? DOptionParseStrategy::native_config

            : ( is_option_set_container_v<clean_type> &&
                djinterp::has_key_type_alias<
                    clean_type>::value )
                ? DOptionParseStrategy::generic_kv

            : DOptionParseStrategy::unsupported;
    };

NS_END  // internal

// container_option_parse_strategy
//   type trait: determines the best CLI parse strategy.
template<typename _Type>
struct container_option_parse_strategy
{
    static constexpr DOptionParseStrategy value =
        internal::option_parse_strategy_impl<
            _Type>::value;
};

template<typename _Type>
inline constexpr DOptionParseStrategy
    container_option_parse_strategy_v =
        container_option_parse_strategy<
            _Type>::value;


// =============================================================================
// VII. Convenience Predicates
// =============================================================================

// has_any_option_support
//   type trait: true if the container has any options-related
// capability.
template<typename _Type>
struct has_any_option_support
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        ( is_option_set_container_v<clean_type>      ||
          has_option_entry_elements_v<clean_type>     ||
          is_configurable_container_v<clean_type>     ||
          is_option_producing_container_v<clean_type> ||
          is_cli_capable_container_v<clean_type>      ||
          has_from_options_factory_v<clean_type> );
};

template<typename _Type>
inline constexpr bool has_any_option_support_v =
    has_any_option_support<_Type>::value;

// is_option_round_trip_capable
//   type trait: true if the container can both receive and
// export options (configurable + producing).
template<typename _Type>
struct is_option_round_trip_capable
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        ( is_configurable_container_v<clean_type> &&
          is_option_producing_container_v<
              clean_type> );
};

template<typename _Type>
inline constexpr bool is_option_round_trip_capable_v =
    is_option_round_trip_capable<_Type>::value;


// =============================================================================
// VIII. Combined Classification
// =============================================================================

// container_option_class
//   struct: complete option classification of a container
// type.  All members are static constexpr.
template<typename _Type>
struct container_option_class
{
    // option storage
    static constexpr bool is_option_set =
        is_option_set_container_v<_Type>;
    static constexpr bool has_option_entries =
        has_option_entry_elements_v<_Type>;
    static constexpr bool has_named_entries =
        has_named_option_elements_v<_Type>;
    static constexpr bool has_documented_entries =
        has_documented_option_elements_v<_Type>;
    static constexpr bool has_constrained_entries =
        has_constrained_option_elements_v<_Type>;
    static constexpr bool has_env_mapped_entries =
        has_env_mapped_option_elements_v<_Type>;

    // configurable
    static constexpr bool has_apply_options =
        has_apply_options_method_v<_Type>;
    static constexpr bool has_configure =
        has_configure_method_v<_Type>;
    static constexpr bool has_set_option =
        has_set_option_method_v<_Type>;
    static constexpr bool has_get_option =
        has_get_option_method_v<_Type>;
    static constexpr bool is_configurable =
        is_configurable_container_v<_Type>;

    // option-producing
    static constexpr bool has_to_options =
        has_to_options_method_v<_Type>;
    static constexpr bool has_get_options =
        has_get_options_method_v<_Type>;
    static constexpr bool is_producing =
        is_option_producing_container_v<_Type>;

    // CLI
    static constexpr bool has_parse_option =
        has_parse_option_method_v<_Type>;
    static constexpr bool has_parse_args =
        has_parse_args_method_v<_Type>;
    static constexpr bool has_parse_config =
        has_parse_config_method_v<_Type>;
    static constexpr bool has_help_string =
        has_help_string_method_v<_Type>;
    static constexpr bool is_cli_capable =
        is_cli_capable_container_v<_Type>;

    // construction
    static constexpr bool has_from_options =
        has_from_options_factory_v<_Type>;

    // strategies
    static constexpr DOptionConfigureStrategy
        configure_strategy =
            container_option_configure_strategy_v<
                _Type>;
    static constexpr DOptionParseStrategy
        parse_strategy =
            container_option_parse_strategy_v<_Type>;

    // aggregate
    static constexpr bool has_option_support =
        has_any_option_support_v<_Type>;
    static constexpr bool is_round_trip =
        is_option_round_trip_capable_v<_Type>;
};


NS_END  // traits
NS_END  // container
NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_OPTION_TRAITS_
