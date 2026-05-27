/******************************************************************************
* djinterp [cli]                                            cli_binding_set.hpp
*
*   The cli_binding_set<> core type plus the structural machinery needed
* to CONSTRUCT one safely:
*
*     - is_cli_binding_entry  : every entry must be a cli_binding<...>
*     - uniformity check      : every binding's key_type must match
*     - keys unique           : reuses value_pack_unique
*     - primary names unique  : string-aware constexpr nested-loop
*     - short forms unique    : sentinel-aware ('\0' is absence)
*
*   Queries OVER an instantiated binding set (contains_key, contains_name,
* find_*, etc.) live in cli_binding_set_traits.hpp.  Concept analogs in
* cli_binding_set_concepts.hpp.
*
*   This header has NO dependency on the option module.
*
*   NOTE ON ALIAS / NEGATION UNIQUENESS
*
*   The CURRENT pass validates only the binding's PRIMARY name.  Pairwise
* uniqueness across the full set of names (primaries + every cli_alias<>
* + every cli_negate<>) is a TODO - the lookup traits in
* cli_binding_set_traits.hpp are already alias/negate-aware, so day-one
* parsing dispatch works correctly; what's deferred is the assertion that
* the user hasn't bound the same name twice via different mechanisms.
* When that lands, it goes in the run_set_checks block below.
*
*
* path:      /inc/djinterp/core/cli/cli_binding_set.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.25
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    structural helpers     (all_cli_bindings, all_same_key_type)
II.   uniqueness helpers     (primary names, shorts)
III.  run_set_checks         (the static_assert block)
IV.   cli_binding_set
*/

#ifndef DJINTERP_CLI_BINDING_SET_
#define DJINTERP_CLI_BINDING_SET_ 1

// std
#include <array>
#include <cstddef>
#include <string_view>
#include <tuple>
#include <type_traits>
// djinterp
#include "../djinterp.hpp"
#include "../util/lookup.hpp"           // value_pack_unique
#include "./cli.hpp"
#include "./cli_binding.hpp"
#include "./cli_binding_traits.hpp"
#include "./cli_binding_tags.hpp"       // cli_binding_short_tag*, has_short_v


NS_DJINTERP


// ===========================================================================
// I.   structural helpers
// ===========================================================================

NS_INTERNAL

    // all_cli_bindings
    //   helper: every type in the pack is a cli_binding<...>
    // specialization.  Mirrors option_set's all_keyed; uses a
    // tighter, nominal-binding-shape check here because cli
    // bindings are not open-world.
    template<typename...>
    struct all_cli_bindings : std::true_type
    {};

    template<typename    _First,
             typename... _Rest>
    struct all_cli_bindings<_First, _Rest...>
        : std::integral_constant<bool,
            ( is_cli_binding_v<_First> &&
              all_cli_bindings<_Rest...>::value )>
    {};

    // all_same_key_type
    //   helper: every binding shares the same key_type.
    template<typename...>
    struct all_same_key_type : std::true_type
    {};

    template<typename _First>
    struct all_same_key_type<_First> : std::true_type
    {};

    template<typename    _First,
             typename    _Second,
             typename... _Rest>
    struct all_same_key_type<_First, _Second, _Rest...>
        : std::integral_constant<bool,
            ( std::is_same<
                  typename _First::key_type,
                  typename _Second::key_type>::value &&
              all_same_key_type<_Second, _Rest...>::value )>
    {};

NS_END  // internal


// ===========================================================================
// II.  uniqueness helpers
// ===========================================================================

NS_INTERNAL

    // all_primary_names_unique
    //   helper: pairwise-distinct check across the bindings'
    // PRIMARY names (the _Name template parameter of each
    // cli_binding<>).  Implemented as a constexpr nested loop
    // over a std::array<std::string_view, N> since fixed_strings
    // of differing sizes can't go through value_pack_unique.
    //
    //   TODO: extend to cover cli_alias<> and cli_negate<> tag
    // names across the set.
    template<typename _Tuple>
    struct all_primary_names_unique;

    template<>
    struct all_primary_names_unique<std::tuple<>>
    {
        static constexpr bool value = true;
    };

    template<typename... _Bindings>
    struct all_primary_names_unique<std::tuple<_Bindings...>>
    {
    private:
        static constexpr std::size_t count = sizeof...(_Bindings);

        static constexpr std::array<std::string_view, count> names =
            { _Bindings::name_view()... };

        static constexpr bool check() noexcept
        {
            for (std::size_t i = 0; i < count; ++i)
            {
                for (std::size_t j = i + 1; j < count; ++j)
                {
                    if (names[i] == names[j])
                    {
                        return false;
                    }
                }
            }

            return true;
        }

    public:
        static constexpr bool value = check();
    };


    // short_value_or_none
    //   helper: yields the binding's short-form character if it
    // has one, else cli_no_short ('\0').  Used by the shorts-
    // uniqueness check to keep absent shorts from colliding.
    template<typename _Binding,
             bool     = cli_binding_has_short_v<_Binding>>
    struct short_value_or_none
    {
        static constexpr char value = cli_no_short;
    };

    template<typename _Binding>
    struct short_value_or_none<_Binding, true>
    {
        static constexpr char value = cli_binding_short_tag_t<_Binding>::value;
    };


    // all_shorts_unique
    //   helper: pairwise-distinct check across non-absent short
    // forms.  Entries equal to cli_no_short are skipped (they
    // mean "no short form for this binding"), so multiple
    // bindings without shorts don't collide.
    template<typename _Tuple>
    struct all_shorts_unique;

    template<>
    struct all_shorts_unique<std::tuple<>>
    {
        static constexpr bool value = true;
    };

    template<typename... _Bindings>
    struct all_shorts_unique<std::tuple<_Bindings...>>
    {
    private:
        static constexpr std::size_t count = sizeof...(_Bindings);

        static constexpr std::array<char, count> shorts =
            { short_value_or_none<_Bindings>::value... };

        static constexpr bool check() noexcept
        {
            for (std::size_t i = 0; i < count; ++i)
            {
                if (shorts[i] == cli_no_short)
                {
                    continue;
                }

                for (std::size_t j = i + 1; j < count; ++j)
                {
                    if (shorts[i] == shorts[j])
                    {
                        return false;
                    }
                }
            }

            return true;
        }

    public:
        static constexpr bool value = check();
    };

NS_END  // internal


// ===========================================================================
// III. run_set_checks
// ===========================================================================

NS_INTERNAL

    // run_binding_set_checks
    //   helper: instantiated by cli_binding_set on its bindings
    // tuple to fire the structural + uniformity + uniqueness
    // static_asserts.  Exposes ::value so callers can depend on
    // it and force instantiation.
    template<typename _Tuple>
    struct run_binding_set_checks;

    template<>
    struct run_binding_set_checks<std::tuple<>>
    {
        static constexpr bool value = true;
    };

    template<typename    _First,
             typename... _Rest>
    struct run_binding_set_checks<std::tuple<_First, _Rest...>>
    {
        static_assert(
            all_cli_bindings<_First, _Rest...>::value,
            "cli_binding_set: every entry must be a cli_binding<...> "
            "specialization.");

        static_assert(
            all_same_key_type<_First, _Rest...>::value,
            "cli_binding_set: all bindings must share the same "
            "key_type.  Use a single enum / class / scope for every "
            "key in the set (same constraint option_set imposes).");

        static_assert(
            value_pack_unique<
                _First::key,
                _Rest::key...
            >::value,
            "cli_binding_set: all binding keys must be unique.");

        static_assert(
            all_primary_names_unique<
                std::tuple<_First, _Rest...>
            >::value,
            "cli_binding_set: all binding PRIMARY names must be "
            "unique.  (Alias / negation uniqueness is not yet "
            "checked here - see cli_binding_set.hpp note.)");

        static_assert(
            all_shorts_unique<
                std::tuple<_First, _Rest...>
            >::value,
            "cli_binding_set: all short-form characters must be "
            "unique across the set.  Bindings without a cli_short<> "
            "tag are excluded from the check.");

        static constexpr bool value = true;
    };

NS_END  // internal


// ===========================================================================
// IV.  cli_binding_set
// ===========================================================================

// cli_binding_set
//   type: a structurally-validated collection of cli_binding<>
// entries.  Unlike option_set, cli_binding_set does NOT support
// multi-expanding entries - every binding is a single, terminal
// cli_binding<> specialization.  The opposing_unary_pair shape on
// the option side is handled by the bridge, which knows to bind
// each expanded option independently.
//
//   See cli_binding_set_traits.hpp for queries over an instantiated
// set (find_by_key / find_by_name / find_by_short, etc.).
template<typename... _Bindings>
struct cli_binding_set
{
private:
    using bindings_tuple = std::tuple<_Bindings...>;

    // force the checks to fire by depending on ::value.
    static_assert(internal::run_binding_set_checks<bindings_tuple>::value,
                  "internal: run_binding_set_checks did not return true "
                  "(see preceding assertion for the real diagnostic).");

public:
    // size / empty
    static constexpr std::size_t size  = sizeof...(_Bindings);
    static constexpr bool        empty = (size == 0);

    // bindings_t
    //   type: the std::tuple<...> of bindings, useful for tuple-
    // walking helpers.
    using bindings_t = bindings_tuple;

    // binding_at
    //   type: positional access into the bindings list.
    template<std::size_t _I>
    using binding_at = std::tuple_element_t<_I, bindings_tuple>;
};


NS_END  // djinterp


#endif  // DJINTERP_CLI_BINDING_SET_
