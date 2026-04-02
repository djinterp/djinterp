/******************************************************************************
* djinterp [options]                                              options.hpp
*
* Master options module for the djinterp framework.
*   Provides a unified, SFINAE-dispatched interface for applying options
* to any configurable type, exporting configuration as options, and
* parsing CLI arguments or config text into options.  Strategy selection
* is performed at compile time via option_set_traits.hpp.
*
*   The module is fully type-agnostic: all operations are parameterized
* on the target and option_set types.  No assumption is made about the
* target — it may be a container, a service, a configuration object, a
* view, or any user-defined type.  Vendor-specific or user-defined
* option_set implementations are supported as long as they satisfy the
* structural contracts detected by option_set_traits.hpp.
*
* USAGE:
*   // apply options to any configurable type
*   my_type t;
*   my_option_set opts = { ... };
*   djinterp::apply_options(t, opts);
*
*   // export current options
*   auto exported = djinterp::get_options(t);
*
*   // parse CLI arguments into a target's options
*   djinterp::parse_args(t, argc, argv);
*
*   // generate help text
*   auto help = djinterp::help_string(t);
*
*   // validate all entries
*   auto failures = djinterp::validate_options(opts);
*
*   // merge one set into another
*   djinterp::merge_options(target_set, source_set);
*
* SUB-MODULES:
*   option_set_traits.hpp  — entry, set, and target detection
*
* TABLE OF CONTENTS
* =================
* I.      Apply Options
* II.     Export Options
* III.    CLI Parsing
* IV.     Help Text Generation
* V.      Option Validation
* VI.     Option Merging
* VII.    Bounded Option Defaults
*
*
* path:      /inc/options/options.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                      date: 2026.03.23
******************************************************************************/

#ifndef DJINTERP_OPTIONS_
#define DJINTERP_OPTIONS_ 1

#include <cstddef>
#include <string>
#include <type_traits>
#include <vector>
#include "../djinterp.hpp"
#include "option_set_traits.hpp"

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    #include <optional>
    #include <string_view>
#endif


NS_DJINTERP

// =============================================================================
// I.   Apply Options
// =============================================================================
// Applies an option_set to a target using the best
// available strategy detected at compile time.

NS_INTERNAL

    // --- native apply_options path ---

    template<typename _Target,
             typename _OptionSet>
    inline void
    apply_native
    (
        _Target&          _target,
        const _OptionSet& _options
    )
    {
        _target.apply_options(_options);

        return;
    }

    // --- native configure path ---

    template<typename _Target,
             typename _OptionSet>
    inline void
    apply_configure
    (
        _Target&          _target,
        const _OptionSet& _options
    )
    {
        _target.configure(_options);

        return;
    }

    // --- per-option set_option path ---
    // iterates the option_set and calls
    // set_option(key, value) for each entry.

    template<typename _Target,
             typename _OptionSet>
    inline void
    apply_per_option
    (
        _Target&          _target,
        const _OptionSet& _options
    )
    {
        for (const auto& entry : _options)
        {
            _target.set_option(entry.key, entry.value);
        }

        return;
    }

NS_END  // internal


// apply_options
//   function: applies an option_set to a target.
// Dispatches at compile time to the best available
// strategy.
// Enabled when the target is configurable from _OptionSet.
template<typename _Target,
         typename _OptionSet>
inline typename std::enable_if<
    is_options_configurable<_Target, _OptionSet>::value,
    void
>::type
apply_options
(
    _Target&          _target,
    const _OptionSet& _options
)
{
    constexpr auto strategy =
        option_configure_strategy<
            _Target, _OptionSet>::value;

    if constexpr (
        strategy ==
        configure_strategy::native_apply)
    {
        internal::apply_native(_target, _options);
    }
    else if constexpr (
        strategy ==
        configure_strategy::native_configure)
    {
        internal::apply_configure(_target, _options);
    }
    else if constexpr (
        strategy ==
        configure_strategy::per_option)
    {
        internal::apply_per_option(_target, _options);
    }
    else
    {
        static_assert(
            strategy !=
            configure_strategy::unsupported,
            "target type does not support option "
            "configuration from the given option_set");
    }

    return;
}


// =============================================================================
// II.  Export Options
// =============================================================================
// Exports the current configuration of a target as an
// option collection.

// get_options (to_options path)
//   function: retrieves the current options from a target
// via to_options().  Returns whatever type the target's
// to_options() method returns.
template<typename _Target>
inline auto
get_options(const _Target& _source)
    -> typename std::enable_if<
           has_to_options_method<_Target>::value,
           decltype(
               std::declval<const _Target&>()
                   .to_options())
       >::type
{
    return _source.to_options();
}

// get_options (get_options fallback)
//   function: overload for targets that have
// get_options() but not to_options().
template<typename _Target>
inline auto
get_options(const _Target& _source)
    -> typename std::enable_if<
           ( !has_to_options_method<_Target>::value &&
             has_get_options_method<_Target>::value ),
           decltype(
               std::declval<const _Target&>()
                   .get_options())
       >::type
{
    return _source.get_options();
}


// =============================================================================
// III. CLI Parsing
// =============================================================================
// Parses command-line arguments into a target's options.

// parse_args (native path)
//   function: parses argc/argv into a target's options.
// Dispatches to the target's own parse_args method.
template<typename _Target>
inline typename std::enable_if<
    has_parse_args_method<_Target>::value,
    void
>::type
parse_args
(
    _Target&          _target,
    int               _argc,
    const char* const _argv[]
)
{
    _target.parse_args(_argc, _argv);

    return;
}

// parse_args (single-option fallback)
//   function: overload for targets that have
// parse_option(string) but not parse_args(argc, argv).
// Iterates argv[1..argc-1] and calls parse_option for
// each argument.
template<typename _Target>
inline typename std::enable_if<
    ( !has_parse_args_method<_Target>::value &&
      has_parse_option_method<_Target>::value ),
    void
>::type
parse_args
(
    _Target&          _target,
    int               _argc,
    const char* const _argv[]
)
{
    for (int i = 1; i < _argc; ++i)
    {
        if (_argv[i])
        {
            _target.parse_option(
                std::string(_argv[i]));
        }
    }

    return;
}

// parse_option
//   function: parses a single "key=value" or "--key value"
// string into a target's options.
template<typename _Target>
inline typename std::enable_if<
    has_parse_option_method<_Target>::value,
    void
>::type
parse_option
(
    _Target&           _target,
    const std::string& _arg
)
{
    _target.parse_option(_arg);

    return;
}

// parse_config
//   function: parses a config string (multi-line or file
// contents) into a target's options.
template<typename _Target>
inline typename std::enable_if<
    has_parse_config_method<_Target>::value,
    void
>::type
parse_config
(
    _Target&           _target,
    const std::string& _config_text
)
{
    _target.parse_config(_config_text);

    return;
}


// =============================================================================
// IV.  Help Text Generation
// =============================================================================

// help_string
//   function: generates a help/usage string from the
// target's options.
template<typename _Target>
inline typename std::enable_if<
    has_help_string_method<_Target>::value,
    std::string
>::type
help_string(const _Target& _source)
{
    return _source.help_string();
}


// =============================================================================
// V.   Option Validation
// =============================================================================
// Validates all options in a set against their constraints
// (bounds, choices, custom validators).  Operates on the
// option_set itself, not the target.

NS_INTERNAL

    // validate_entry_if_possible
    //   helper: calls entry.validate(entry.value) if the
    // entry has a validate callable, otherwise returns true.
    template<typename _Entry>
    inline typename std::enable_if<
        has_validator<_Entry>::value,
        bool
    >::type
    validate_entry(const _Entry& _entry)
    {
        return _entry.validate(_entry.value);
    }

    template<typename _Entry>
    inline typename std::enable_if<
        !has_validator<_Entry>::value,
        bool
    >::type
    validate_entry(const _Entry&)
    {
        return true;
    }

NS_END  // internal

// validate_options
//   function: validates all option entries in an option_set.
// Returns a vector of keys for entries that failed
// validation.  Returns empty if all pass.
// Enabled when _OptionSet is iterable and its entries are
// option entries.
template<typename _OptionSet>
inline typename std::enable_if<
    ( is_iterable_option_set<_OptionSet>::value &&
      has_value_type_alias<_OptionSet>::value   &&
      is_option_entry<
          typename _OptionSet::value_type>::value ),
    std::vector<std::string>
>::type
validate_options
(
    const _OptionSet& _source
)
{
    std::vector<std::string> failures;

    for (const auto& entry : _source)
    {
        if (!internal::validate_entry(entry))
        {
            // attempt to extract key as string
            if constexpr (
                has_string_key<
                    typename _OptionSet::value_type
                >::value)
            {
                failures.push_back(
                    std::string(entry.key));
            }
            else
            {
                failures.push_back(
                    "<key:" +
                    std::to_string(
                        static_cast<int>(
                            entry.key)) +
                    ">");
            }
        }
    }

    return failures;
}


// =============================================================================
// VI.  Option Merging
// =============================================================================
// Merges options from one source into a target, preferring
// the source values for conflicting keys.  Both operands
// are option collections, not necessarily the end target
// types.

// merge_options (per-entry path)
//   function: merges options from _source into _target.
// For option collections whose entries have .key and
// .value, iterates the source and applies each entry
// via set_option.
template<typename _Target,
         typename _Source>
inline typename std::enable_if<
    ( is_iterable_option_set<_Source>::value   &&
      has_value_type_alias<_Source>::value     &&
      is_option_entry<
          typename _Source::value_type>::value &&
      has_set_option_method<
          _Target,
          option_key_t<typename _Source::value_type>,
          option_value_t<
              typename _Source::value_type>>::value ),
    void
>::type
merge_options
(
    _Target&       _target,
    const _Source&  _source
)
{
    for (const auto& entry : _source)
    {
        _target.set_option(entry.key, entry.value);
    }

    return;
}

// merge_options (bulk apply fallback)
//   function: overload for targets that accept the source
// as a bulk option_set via apply_options or configure,
// but do not expose per-entry set_option.
template<typename _Target,
         typename _Source>
inline typename std::enable_if<
    ( is_options_configurable<_Target, _Source>::value &&
      !(is_iterable_option_set<_Source>::value   &&
        has_value_type_alias<_Source>::value     &&
        is_option_entry<
            typename _Source::value_type>::value &&
        has_set_option_method<
            _Target,
            option_key_t<typename _Source::value_type>,
            option_value_t<
                typename _Source::value_type>>::value) ),
    void
>::type
merge_options
(
    _Target&       _target,
    const _Source&  _source
)
{
    apply_options(_target, _source);

    return;
}


// =============================================================================
// VII. Bounded Option Defaults
// =============================================================================
// Constexpr utilities for options with min, max, and step
// constraints.  option_bounds provides a lightweight value
// type for carrying range metadata alongside an option entry.
// option_bounds_for extracts bounds from an entry that
// already has them, filling in sensible defaults for any
// missing members.

// option_bounds
//   struct: constexpr range descriptor carrying the minimum
// value, maximum value, and step increment for a bounded
// option.  All members default-constructible.
//
// Example:
//   constexpr option_bounds<int> volume_range =
//       { .min = 0, .max = 100, .step = 1 };
//
//   constexpr option_bounds<double> opacity_range =
//       { .min = 0.0, .max = 1.0, .step = 0.01 };
template<typename _Value>
struct option_bounds
{
    using value_type = _Value;

    _Value min;
    _Value max;
    _Value step;

    // default construction (zero-initialized)
    constexpr option_bounds()
        : min{},
          max{},
          step{}
    {}

    // explicit construction
    constexpr option_bounds(
            _Value _min,
            _Value _max,
            _Value _step
        )
            : min(_min),
              max(_max),
              step(_step)
    {}

    // contains
    //   function: returns true when _v falls within
    // [min, max] inclusive.
    constexpr bool
    contains(_Value _v) const
    {
        return ( (_v >= min) &&
                 (_v <= max) );
    }

    // clamp
    //   function: constrains _v to [min, max].
    constexpr _Value
    clamp(_Value _v) const
    {
        return (_v < min) ? min
             : (_v > max) ? max
             :              _v;
    }

    // snap
    //   function: rounds _v to the nearest step increment
    // relative to min, then clamps to [min, max].
    // Only meaningful when step > 0.
    constexpr _Value
    snap(_Value _v) const
    {
        if (step <= _Value{})
        {
            return clamp(_v);
        }

        _Value offset  = _v - min;
        _Value steps   = offset / step;
        _Value snapped = min + (steps * step);

        return clamp(snapped);
    }
};


NS_INTERNAL

    // bounds_extract_min
    //   helper: extracts .min_value if present, else returns
    // a default-constructed _Value.
    template<typename _Entry,
             bool = has_min_value<_Entry>::value>
    struct bounds_extract_min
    {
        static constexpr auto
        get(const _Entry&)
        {
            using value_clean =
                typename std::remove_cv<
                    typename std::remove_reference<
                        option_value_t<_Entry>
                    >::type
                >::type;

            return value_clean{};
        }
    };

    template<typename _Entry>
    struct bounds_extract_min<_Entry, true>
    {
        static constexpr auto
        get(const _Entry& _e)
        {
            return _e.min_value;
        }
    };

    // bounds_extract_max
    //   helper: extracts .max_value if present, else returns
    // a default-constructed _Value.
    template<typename _Entry,
             bool = has_max_value<_Entry>::value>
    struct bounds_extract_max
    {
        static constexpr auto
        get(const _Entry&)
        {
            using value_clean =
                typename std::remove_cv<
                    typename std::remove_reference<
                        option_value_t<_Entry>
                    >::type
                >::type;

            return value_clean{};
        }
    };

    template<typename _Entry>
    struct bounds_extract_max<_Entry, true>
    {
        static constexpr auto
        get(const _Entry& _e)
        {
            return _e.max_value;
        }
    };

    // bounds_extract_step
    //   helper: extracts .step if present, else returns
    // _Value{1} for integral types, _Value{} for others.
    template<typename _Entry,
             bool = has_step_value<_Entry>::value>
    struct bounds_extract_step
    {
        static constexpr auto
        get(const _Entry&)
        {
            using value_clean =
                typename std::remove_cv<
                    typename std::remove_reference<
                        option_value_t<_Entry>
                    >::type
                >::type;

            return std::is_integral<value_clean>::value
                       ? value_clean{1}
                       : value_clean{};
        }
    };

    template<typename _Entry>
    struct bounds_extract_step<_Entry, true>
    {
        static constexpr auto
        get(const _Entry& _e)
        {
            return _e.step;
        }
    };

NS_END  // internal

// option_bounds_for
//   function: constructs an option_bounds from an option
// entry, extracting .min_value, .max_value, and .step
// where present and filling sensible defaults where absent.
// Requires that the entry has a detectable .value member.
template<typename _Entry>
inline constexpr auto
option_bounds_for(const _Entry& _entry)
    -> typename std::enable_if<
           is_option_entry<_Entry>::value,
           option_bounds<
               typename std::remove_cv<
                   typename std::remove_reference<
                       option_value_t<_Entry>
                   >::type
               >::type>
       >::type
{
    using value_clean =
        typename std::remove_cv<
            typename std::remove_reference<
                option_value_t<_Entry>
            >::type
        >::type;

    return option_bounds<value_clean>(
        internal::bounds_extract_min<_Entry>::get(_entry),
        internal::bounds_extract_max<_Entry>::get(_entry),
        internal::bounds_extract_step<_Entry>::get(_entry));
}


NS_END  // djinterp


#endif  // DJINTERP_OPTIONS_
