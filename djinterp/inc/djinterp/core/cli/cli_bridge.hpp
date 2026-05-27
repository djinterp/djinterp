/******************************************************************************
* djinterp [cli]                                                cli_bridge.hpp
*
*   The CLI bridge: ties together an option_set<> (the schema) with a
* cli_binding_set<> (the CLI naming) and produces a unified, validated
* runtime-and-compile-time surface.  This is the ONLY header that knows
* about BOTH the option module and the cli module; neither side knows
* about the other directly.
*
*   The bridge is responsible for:
*
*     1. Cross-validation:
*        - key_type must agree between the two sets
*        - every binding's key must exist in the option_set
*        (optionally, every option key has a binding - opt-in via
*         cli_require_complete; not enforced by default)
*
*     2. Per-binding DERIVED metadata from BOTH sources:
*        - cli_kind   : derived from option's value-carrier tags,
*                       overridable via cli_positional on the binding
*        - cli_arity  : follows from kind in the simple cases
*        - help text  : binding's cli_help_override if present, else
*                       the option's description<> tag, else empty
*        - hidden     : binding's cli_hidden tag
*
*     3. Materialization in two flavors:
*        - constexpr std::array<cli_descriptor_view, N> for cheap,
*          dispatch-free iteration of metadata
*        - a runtime polymorphic registry of const cli_descriptor*
*          pointing at function-local static cli_bridge_descriptor
*          instances, for code that wants the abstract base
*
*   CONSTEXPR / RUNTIME DUALITY
*
*   The compile-time bridge is the "source of truth": every per-binding
* property the bridge derives is a static constexpr value or alias,
* available at compile time without instantiation.  The runtime layer
* simply re-exposes the same compile-time data through the cli_descriptor
* virtual interface, at the cost of one vtable per binding instance
* (which lives forever as a function-local static).
*
*
* path:      /inc/djinterp/core/cli/cli_bridge.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.25
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    cross-validation helpers     (key universe agreement)
II.   per-binding derivation       (kind, arity, help, hidden)
III.  cli_bridge                   (the unified type)
IV.   cli_bridge_descriptor        (polymorphic per-binding impl)
V.    runtime registry             (function-local static array)
VI.   cli_require_complete         (opt-in completeness assertion)
*/

#ifndef DJINTERP_CLI_BRIDGE_
#define DJINTERP_CLI_BRIDGE_ 1

// std
#include <array>
#include <cstddef>
#include <string_view>
#include <type_traits>
#include <utility>
// djinterp - option side
#include "../djinterp.hpp"
#include "../options/option.hpp"
#include "../options/option_set.hpp"
#include "../options/option_set_traits.hpp"
#include "../options/option_tags.hpp"        // description, actual, default_,
                                             // val_type
#include "../options/option_traits.hpp"      // option_has_value_v
// djinterp - cli side
#include "./cli.hpp"
#include "./cli_binding.hpp"
#include "./cli_binding_traits.hpp"
#include "./cli_binding_tags.hpp"
#include "./cli_binding_set.hpp"
#include "./cli_binding_set_traits.hpp"


NS_DJINTERP


// ===========================================================================
// I.   cross-validation helpers
// ===========================================================================

NS_INTERNAL

    // every_binding_key_in_set
    //   helper: true iff every binding's key exists in the option_set.
    template<typename _OptionSet,
             typename... _Bindings>
    struct every_binding_key_in_set
        : std::integral_constant<bool,
            ( (option_set_contains_v<_OptionSet, _Bindings::key>) && ... )>
    {};

    // run_bridge_checks
    //   helper: instantiated by cli_bridge to fire the cross-validation
    // static_asserts.
    template<typename _OptionSet,
             typename _Bindings>
    struct run_bridge_checks;

    template<typename _OptionSet,
             typename... _Bindings>
    struct run_bridge_checks<_OptionSet, std::tuple<_Bindings...>>
    {
        static_assert(
            is_option_set_v<_OptionSet>,
            "cli_bridge: first parameter must be an option_set<...>.");

        // key_type agreement (only meaningful when both sets are non-empty)
        static_assert(
            ( (sizeof...(_Bindings) == 0) ||
              (_OptionSet::size == 0)     ||
              std::is_same<
                  option_set_key_type_t<_OptionSet>,
                  typename std::tuple_element<0,
                      std::tuple<_Bindings...>>::type::key_type
              >::value ),
            "cli_bridge: option_set and cli_binding_set must share the "
            "same key_type.");

        // every binding key must exist in the option_set
        static_assert(
            every_binding_key_in_set<_OptionSet, _Bindings...>::value,
            "cli_bridge: every binding's key must exist in the bound "
            "option_set.  A binding references a key that is absent "
            "from the schema.");

        static constexpr bool value = true;
    };

NS_END  // internal


// ===========================================================================
// II.  per-binding derivation
// ===========================================================================

NS_INTERNAL

    // derive_kind
    //   helper: produce a cli_kind for a (binding, option) pair.
    //
    //   Rules:
    //     1. cli_positional on the binding wins outright.
    //     2. Otherwise, if the option exposes ANY value-carrier tag
    //        (value<>, actual<>, default_<>, val_type<>), kind=value.
    //     3. Otherwise kind=flag.
    //
    //   multi_value / subcommand detection is not derivable from the
    // current option-tag set and is left for a future pass.
    template<typename _Binding,
             typename _Option>
    constexpr cli_kind
    derive_kind() noexcept
    {
        if constexpr (cli_binding_has_positional_v<_Binding>)
        {
            return cli_kind::positional;
        }
        else if constexpr (
            option_has_value_v<_Option>     ||
            option_has_actual_v<_Option>    ||
            option_has_default_v<_Option>   ||
            option_has_val_type_v<_Option> )
        {
            return cli_kind::value;
        }
        else
        {
            return cli_kind::flag;
        }
    }

    // derive_arity
    //   helper: produce a cli_arity from a derived cli_kind.  Simple
    // mapping for now - extended when multi_value / optional value
    // are introduced via dedicated option tags.
    constexpr cli_arity
    derive_arity(cli_kind _k) noexcept
    {
        switch (_k)
        {
            case cli_kind::flag:        return cli_arity::none;
            case cli_kind::value:       return cli_arity::one;
            case cli_kind::multi_value: return cli_arity::many;
            case cli_kind::positional:  return cli_arity::one;
            case cli_kind::subcommand:  return cli_arity::none;
        }

        return cli_arity::none;
    }

    // derive_short
    //   helper: produce the short-form character for a binding, or
    // cli_no_short if none was provided.
    template<typename _Binding>
    constexpr char
    derive_short() noexcept
    {
        if constexpr (cli_binding_has_short_v<_Binding>)
        {
            return cli_binding_short_tag_t<_Binding>::value;
        }
        else
        {
            return cli_no_short;
        }
    }

    // derive_help
    //   helper: produce the help text for a (binding, option) pair.
    //
    //   Rules:
    //     1. cli_help_override on the binding wins.
    //     2. Otherwise, the option's description<> tag (if any).
    //     3. Otherwise, an empty view.
    template<typename _Binding,
             typename _Option>
    constexpr std::string_view
    derive_help() noexcept
    {
        if constexpr (cli_binding_has_help_override_v<_Binding>)
        {
            return cli_binding_help_override_tag_t<_Binding>::view();
        }
        else if constexpr (option_has_description_v<_Option>)
        {
            return option_description_tag_t<_Option>::text.view();
        }
        else
        {
            return std::string_view{};
        }
    }

    // derive_hidden
    //   helper: true iff the binding carries cli_hidden.
    template<typename _Binding>
    constexpr bool
    derive_hidden() noexcept
    {
        return cli_binding_has_hidden_v<_Binding>;
    }

NS_END  // internal


// ===========================================================================
// III. cli_bridge
// ===========================================================================

// cli_bridge
//   type: the validated union of an option_set with a
// cli_binding_set.  Exposes per-binding derived metadata at
// compile time and a constexpr std::array of cli_descriptor_views
// for fast metadata-only iteration.  Runtime polymorphic
// descriptors are provided separately by cli_bridge_runtime_views
// (below).
template<typename _OptionSet,
         typename _Bindings>
struct cli_bridge
{
private:
    using bindings_tuple = typename _Bindings::bindings_t;

    // fire cross-validation
    static_assert(internal::run_bridge_checks<_OptionSet,
                                              bindings_tuple>::value,
                  "internal: run_bridge_checks did not return true "
                  "(see preceding assertion for the real diagnostic).");

public:
    using option_set_t  = _OptionSet;
    using binding_set_t = _Bindings;

    // size / empty
    static constexpr std::size_t size  = _Bindings::size;
    static constexpr bool        empty = (size == 0);

    // binding_at
    //   alias: positional access into the binding set.
    template<std::size_t _I>
    using binding_at = typename _Bindings::template binding_at<_I>;

    // option_at
    //   alias: the option in _OptionSet that the I-th binding maps to.
    template<std::size_t _I>
    using option_at = option_set_find_t<_OptionSet, binding_at<_I>::key>;

    // ---------------- per-binding derived properties ----------------

    template<std::size_t _I>
    static constexpr cli_kind kind_at =
        internal::derive_kind<binding_at<_I>, option_at<_I>>();

    template<std::size_t _I>
    static constexpr cli_arity arity_at =
        internal::derive_arity(kind_at<_I>);

    template<std::size_t _I>
    static constexpr char short_at =
        internal::derive_short<binding_at<_I>>();

    template<std::size_t _I>
    static constexpr std::string_view help_at =
        internal::derive_help<binding_at<_I>, option_at<_I>>();

    template<std::size_t _I>
    static constexpr bool hidden_at =
        internal::derive_hidden<binding_at<_I>>();

    // ---------------- compile-time view at index I ------------------

    template<std::size_t _I>
    static constexpr cli_descriptor_view view_at =
        cli_descriptor_view{
            binding_at<_I>::name_view(),
            help_at<_I>,
            short_at<_I>,
            kind_at<_I>,
            arity_at<_I>,
            hidden_at<_I>
        };

private:
    template<std::size_t... _Is>
    static constexpr std::array<cli_descriptor_view, sizeof...(_Is)>
    make_views(std::index_sequence<_Is...>) noexcept
    {
        return { view_at<_Is>... };
    }

public:
    // views
    //   constexpr static array of all per-binding views.  Cheap,
    // dispatch-free, lives in static storage.  Use when only
    // metadata is needed and the call site is performance-sensitive.
    static constexpr auto views =
        make_views(std::make_index_sequence<size>{});
};


// ===========================================================================
// IV.  cli_bridge_descriptor
// ===========================================================================

// cli_bridge_descriptor
//   class: concrete polymorphic implementation of cli_descriptor,
// parameterized over a cli_bridge and a binding index.  All six
// accessors return values pulled from the bridge's compile-time
// constants - no runtime computation, just vtable dispatch.
//
//   Users typically don't instantiate this directly; the runtime
// registry (next section) wraps and exposes function-local static
// instances of this template.
template<typename     _Bridge,
         std::size_t  _I>
struct cli_bridge_descriptor : cli_descriptor
{
    std::string_view
    name() const noexcept override
    {
        return _Bridge::template binding_at<_I>::name_view();
    }

    char
    short_form() const noexcept override
    {
        return _Bridge::template short_at<_I>;
    }

    cli_kind
    kind() const noexcept override
    {
        return _Bridge::template kind_at<_I>;
    }

    cli_arity
    arity() const noexcept override
    {
        return _Bridge::template arity_at<_I>;
    }

    std::string_view
    help() const noexcept override
    {
        return _Bridge::template help_at<_I>;
    }

    bool
    hidden() const noexcept override
    {
        return _Bridge::template hidden_at<_I>;
    }
};


// ===========================================================================
// V.   runtime registry
// ===========================================================================

NS_INTERNAL

    // runtime_descriptor_at
    //   helper: returns a pointer to the function-local static
    // cli_bridge_descriptor<_Bridge, _I> instance.  Static
    // storage duration; address-stable for the program lifetime.
    template<typename     _Bridge,
             std::size_t  _I>
    const cli_descriptor*
    runtime_descriptor_at() noexcept
    {
        static const cli_bridge_descriptor<_Bridge, _I> instance{};
        return &instance;
    }

    // build_runtime_array
    //   helper: assemble the full per-binding descriptor pointer
    // array from an index sequence.
    template<typename       _Bridge,
             std::size_t... _Is>
    std::array<const cli_descriptor*, sizeof...(_Is)>
    build_runtime_array(std::index_sequence<_Is...>) noexcept
    {
        return { runtime_descriptor_at<_Bridge, _Is>()... };
    }

NS_END  // internal


// cli_bridge_runtime_descriptors
//   function: returns a reference to a function-local static
// std::array of cli_descriptor pointers, one per binding, built
// lazily on first call.  Use this when the consumer needs the
// abstract base class (e.g. polymorphic dispatch, heterogeneous
// containers, plugin-like extension points).
//
//   The returned array and the descriptors it points to both
// have static storage duration - safe to keep references and
// pointers indefinitely.
template<typename _Bridge>
const std::array<const cli_descriptor*, _Bridge::size>&
cli_bridge_runtime_descriptors() noexcept
{
    static const auto array =
        internal::build_runtime_array<_Bridge>(
            std::make_index_sequence<_Bridge::size>{});

    return array;
}


// ===========================================================================
// VI.  cli_require_complete
// ===========================================================================

NS_INTERNAL

    // every_option_key_in_bindings
    //   helper: true iff every option in _OptionSet has a binding
    // in _Bindings.
    template<typename _Bindings,
             typename _OptionsTuple>
    struct every_option_key_in_bindings;

    template<typename _Bindings>
    struct every_option_key_in_bindings<_Bindings, std::tuple<>>
        : std::true_type
    {};

    template<typename     _Bindings,
             typename...  _Options>
    struct every_option_key_in_bindings<_Bindings, std::tuple<_Options...>>
        : std::integral_constant<bool,
            ( (cli_binding_set_contains_key_v<_Bindings, _Options::key>)
              && ... )>
    {};

NS_END  // internal


// cli_require_complete
//   trait: opt-in completeness check.  Static-asserts that every
// option in _Bridge::option_set_t has a corresponding binding.
// Use as a side-effect static_assert at namespace scope:
//
//     using my_bridge   = cli_bridge<my_set, my_bindings>;
//     static_assert(cli_require_complete<my_bridge>::value,
//                   "my_bridge: not all options have CLI bindings.");
//
//   Not part of cli_bridge's default contract because some
// bridges intentionally bind only a subset (CLI surface vs. all
// configurable options).
template<typename _Bridge>
struct cli_require_complete
{
private:
    using flat = typename _Bridge::option_set_t::flat_options_t;

public:
    static constexpr bool value =
        internal::every_option_key_in_bindings<
            typename _Bridge::binding_set_t,
            flat
        >::value;
};

template<typename _Bridge>
inline constexpr bool cli_require_complete_v =
    cli_require_complete<_Bridge>::value;


NS_END  // djinterp


#endif  // DJINTERP_CLI_BRIDGE_
