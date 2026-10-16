/*******************************************************************************
* djinterp [test]                            concurrency_strategy_tags_tests.hpp
*
*   Declarations for the concurrency_strategy_tags.hpp unit-test suite, plus
* the helper types the sections share.
*
*   WHAT THE HEADER UNDER TEST OFFERS, AND HOW IT IS TESTED:
*   The header is a VOCABULARY header - it declares no functions and holds no
* state, so every claim it makes is a claim about TYPES.  The suite therefore
* tests it the way the trait system consumes it: it (a) reads the shape of the
* seven tag structs, (b) exercises the `using concurrency_strategy_tag = ...;`
* fast path with self-tagging mocks, and (c) pins each enumerator of the four
* signature enums to its documented value, order, and scoping.
*
*   THE HELPERS BELOW ARE DELIBERATELY LOCAL.  has_strategy_tag /
* strategy_tag_of re-implement, in miniature, the fast path that
* concurrency_strategy_traits.hpp runs - and mock_signature / mock_signature_of
* re-implement the signature aggregate and the strategy -> signature mapping
* that the same trait header owns.  Neither is imported, because importing them
* would defeat the point: the header under test claims to be usable BENEATH the
* trait layer, with no dependency but djinterp.hpp.  A suite that included the
* trait header could not observe that claim being broken.  The mapping in
* mock_signature_of is illustrative only; the canonical one is the trait
* header's.
*
*   The tests are flat in djinterp::testing and every section TU includes this
* header and nothing else from djinterp - so the whole suite is also a standing
* proof that the header stands alone.
*
*   PORTABILITY: the suite requires C++11 (the framework's floor).  Section I
* of the header under test is C++98-shaped and section II is C++11-gated; both
* claims are themselves tested (tests_tag_types_are_cpp98_pods and
* tests_vocabulary_is_gated_to_cpp11).
*
*
* path:      /tests/djinterp/core/sync/concurrency_strategy_tags_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.13
*                                                            revised: 2026.09.21
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    HELPERS
      -------
      a. the strategy-tag protocol (a local reading of the fast path)
      b. self-tagging mock types
      c.    the tag-dispatch overload set
            d. the signature vocabulary (C++11+)

II.   TEST DECLARATIONS
      -----------------
      a. the header contract
      b. the seven strategy tags
      c.    tag identity + composition
            d. the self-tagging protocol
            e. concurrency_progress
            f. concurrency_arity
            g. iteration_consistency
            h. reclamation_obligation
      i.    the signature vocabulary
*/

#ifndef DJINTERP_TESTS_CONCURRENCY_STRATEGY_TAGS_
#define DJINTERP_TESTS_CONCURRENCY_STRATEGY_TAGS_ 1

// std
#include <cstddef>
#include <type_traits>
// djinterp
#include "../../../../inc/djinterp/core/sync/concurrency_strategy_tags.hpp"


// The suite's floor, and the framework's: report_builder
// (test_report_runner.hpp) requires C++11, so a C++98 build of these tests
// could not be RUN even though section I of the header under test would
// compile.  Section II's own C++11 gate is tested from above, in
// tests_vocabulary_is_gated_to_cpp11.
#if !D_ENV_LANG_IS_CPP11_OR_HIGHER
    #error "concurrency_strategy_tags_tests.hpp requires C++11 or higher"
#endif


NS_DJINTERP
NS_TESTING

///////////////////////////////////////////////////////////////////////////////
///                I.   HELPERS                                             ///
///////////////////////////////////////////////////////////////////////////////

// a.   the strategy-tag protocol (a local reading of the fast path)
//////////////////////////////////////////

// has_strategy_tag
//   trait: does the type name a `concurrency_strategy_tag` member alias?
// The detection the trait system's fast path performs, re-spelled here so the
// suite never leans on the trait header (primary template: it does not).
//
//   SFINAE-safe by construction: a missing alias - and an INACCESSIBLE one -
// is a substitution failure, not a hard error.
template<typename _Type,
         typename _Enable = void>
struct has_strategy_tag : std::false_type
{};

// has_strategy_tag<_Type, void_t<...>>
//   trait: specialization matching a type that declares the alias.
template<typename _Type>
struct has_strategy_tag<_Type,
                        void_t<typename _Type::concurrency_strategy_tag>>
    : std::true_type
{};

// strategy_tag_of
//   trait: the tag a self-tagging type declares.  Only well-formed when
// has_strategy_tag<_Type>::value is true.
template<typename _Type>
struct strategy_tag_of
{
    using type = typename _Type::concurrency_strategy_tag;
};

// strategy_tag_of_t
//   type: convenience alias for strategy_tag_of<_Type>::type.
template<typename _Type>
using strategy_tag_of_t = typename strategy_tag_of<_Type>::type;

// is_strategy_tag
//   trait: is the type one of the SEVEN tags the header defines?  A structural
// alias (has_strategy_tag) says a type declared SOMETHING; this says the
// something is a tag the header knows.
template<typename _Type>
struct is_strategy_tag
{
    static D_CONSTEXPR bool value =
        ( std::is_same<_Type, none_strategy_tag>::value   ||
          std::is_same<_Type, locked_strategy_tag>::value ||
          std::is_same<_Type, cow_strategy_tag>::value    ||
          std::is_same<_Type, rcu_strategy_tag>::value    ||
          std::is_same<_Type, atomic_strategy_tag>::value ||
          std::is_same<_Type, hazard_strategy_tag>::value ||
          std::is_same<_Type, hybrid_strategy_tag>::value );
};

// k_not_a_tag
//   constant: the strategy_tag_index of a type that is not one of the seven.
D_STATIC D_CONSTEXPR int k_not_a_tag = -1;

// k_strategy_tag_count
//   constant: how many tags section I of the header declares.
D_STATIC D_CONSTEXPR int k_strategy_tag_count = 7;

// strategy_tag_index
//   trait: the tag's position in the header's declaration order (primary
// template: not a tag).  Doubles as the suite's proof that a tag can KEY a
// template specialization - which is the whole of its job in the trait system.
template<typename _Type>
struct strategy_tag_index
{
    static D_CONSTEXPR int value = k_not_a_tag;
};

// strategy_tag_index<...>
//   trait: one specialization per tag, in declaration order.
template<>
struct strategy_tag_index<none_strategy_tag>
{
    static D_CONSTEXPR int value = 0;
};

template<>
struct strategy_tag_index<locked_strategy_tag>
{
    static D_CONSTEXPR int value = 1;
};

template<>
struct strategy_tag_index<cow_strategy_tag>
{
    static D_CONSTEXPR int value = 2;
};

template<>
struct strategy_tag_index<rcu_strategy_tag>
{
    static D_CONSTEXPR int value = 3;
};

template<>
struct strategy_tag_index<atomic_strategy_tag>
{
    static D_CONSTEXPR int value = 4;
};

template<>
struct strategy_tag_index<hazard_strategy_tag>
{
    static D_CONSTEXPR int value = 5;
};

template<>
struct strategy_tag_index<hybrid_strategy_tag>
{
    static D_CONSTEXPR int value = 6;
};


// b.   self-tagging mock types
//////////////////////////////////////////
//   One per tag, each shaped exactly like the production primitives that
// self-tag today (atomic.hpp, cow.hpp, rcu.hpp, hazard_pointer.hpp):
//     using concurrency_strategy_tag = <tag>;
// The suite reads them back through has_strategy_tag / strategy_tag_of.

// mock_none
//   type: a type declaring the `none` strategy.
struct mock_none
{
    using concurrency_strategy_tag = none_strategy_tag;
};

// mock_locked
//   type: a type declaring the `locked` strategy (the header's own example).
struct mock_locked
{
    using concurrency_strategy_tag = locked_strategy_tag;
};

// mock_cow
//   type: a type declaring the `cow` strategy (cf. cow_state).
struct mock_cow
{
    using concurrency_strategy_tag = cow_strategy_tag;
};

// mock_rcu
//   type: a type declaring the `rcu` strategy (cf. rcu_protected).
struct mock_rcu
{
    using concurrency_strategy_tag = rcu_strategy_tag;
};

// mock_atomic
//   type: a type declaring the `atomic` strategy (cf. atomic_size).
struct mock_atomic
{
    using concurrency_strategy_tag = atomic_strategy_tag;
};

// mock_hazard
//   type: a type declaring the `hazard` strategy (cf. hazard_domain).
struct mock_hazard
{
    using concurrency_strategy_tag = hazard_strategy_tag;
};

// mock_hybrid
//   type: a type declaring the `hybrid` strategy (locked metadata around a
// cow payload, say).
struct mock_hybrid
{
    using concurrency_strategy_tag = hybrid_strategy_tag;
};

// mock_untagged
//   type: a type that declares NO strategy - the negative every detection
// test needs.
struct mock_untagged
{
    std::size_t size;
};


// c.   the tag-dispatch overload set
//////////////////////////////////////////
//   Seven overloads taking a tag BY VALUE.  Nothing but overload resolution
// picks the arm, which is the second job the tags do (the first being keying a
// specialization - see strategy_tag_index).  Each returns its tag's index so a
// test can observe which arm ran.

D_INLINE int dispatch_on(none_strategy_tag)   { return 0; }
D_INLINE int dispatch_on(locked_strategy_tag) { return 1; }
D_INLINE int dispatch_on(cow_strategy_tag)    { return 2; }
D_INLINE int dispatch_on(rcu_strategy_tag)    { return 3; }
D_INLINE int dispatch_on(atomic_strategy_tag) { return 4; }
D_INLINE int dispatch_on(hazard_strategy_tag) { return 5; }
D_INLINE int dispatch_on(hybrid_strategy_tag) { return 6; }


// d.   the signature vocabulary
//////////////////////////////////////////

// underlying
//   function: the enumerator's value in its underlying type - C++23's
// std::to_underlying, spelled portably.
template<typename _Enum>
D_CONSTEXPR typename std::underlying_type<_Enum>::type
underlying(
    _Enum _e
) D_NOEXCEPT
{
    return static_cast<typename std::underlying_type<_Enum>::type>(_e);
}

// is_scoped_enumeration
//   trait: the type is a SCOPED enumeration - an enum that does NOT implicitly
// convert to an integer.  C++23's std::is_scoped_enum, spelled portably; the
// conversion to `int` is the observable difference between `enum class E` and
// `enum E` (an unscoped enum promotes, whatever its underlying type).
template<typename _Enum>
struct is_scoped_enumeration
{
    static D_CONSTEXPR bool value =
        ( std::is_enum<_Enum>::value &&
          !std::is_convertible<_Enum, int>::value );
};

// progress_name
//   function: the switch a consumer of the vocabulary writes.  Returns
// nullptr for a value outside the four named grades - the default arm a
// scoped enum still needs, because its VALUE RANGE is that of its underlying
// type, not the set of its enumerators.
D_INLINE const char*
progress_name(
    concurrency_progress _p
) D_NOEXCEPT
{
    switch (_p)
    {
        case concurrency_progress::sequential: { return "sequential"; }
        case concurrency_progress::blocking:   { return "blocking";   }
        case concurrency_progress::lock_free:  { return "lock_free";  }
        case concurrency_progress::wait_free:  { return "wait_free";  }
        default:                               { return nullptr;      }
    }
}

// arity_name
//   function: as progress_name, for the arity coordinate.
D_INLINE const char*
arity_name(
    concurrency_arity _a
) D_NOEXCEPT
{
    switch (_a)
    {
        case concurrency_arity::swmr: { return "swmr";  }
        case concurrency_arity::mwmr: { return "mwmr";  }
        default:                      { return nullptr; }
    }
}

// iteration_name
//   function: as progress_name, for the iteration coordinate.
D_INLINE const char*
iteration_name(
    iteration_consistency _i
) D_NOEXCEPT
{
    switch (_i)
    {
        case iteration_consistency::inconsistent:
        {
            return "inconsistent";
        }
        case iteration_consistency::weakly_consistent:
        {
            return "weakly_consistent";
        }
        case iteration_consistency::snapshot:
        {
            return "snapshot";
        }
        default:
        {
            return nullptr;
        }
    }
}

// reclamation_name
//   function: as progress_name, for the reclamation coordinate.
D_INLINE const char*
reclamation_name(
    reclamation_obligation _r
) D_NOEXCEPT
{
    switch (_r)
    {
        case reclamation_obligation::none:            { return "none";       }
        case reclamation_obligation::individual:      { return "individual"; }
        case reclamation_obligation::reference_count: { return "refcount";   }
        case reclamation_obligation::grace_period:    { return "grace";      }
        case reclamation_obligation::hazard:          { return "hazard";     }
        case reclamation_obligation::collector:       { return "collector";  }
        default:                                      { return nullptr;      }
    }
}

// mock_signature
//   struct: a LOCAL bundle of the four coordinates - the shape
// concurrency_strategy_traits.hpp's concurrency_signature has.  It is declared
// here, and not imported, precisely because the header under test carries only
// the VOCABULARY: that the four enums compose into a signature WITHOUT the
// trait header is the layering claim, and this aggregate is how the suite
// observes it.
struct mock_signature
{
    concurrency_progress   progress;
    concurrency_arity      arity;
    iteration_consistency  iteration;
    reclamation_obligation reclamation;
};

// mock_signature_of
//   trait: a LOCAL, ILLUSTRATIVE strategy -> signature mapping keyed on the
// tag (primary template: undefined, so an unknown tag is a compile error
// rather than a silent default).  The canonical mapping lives in
// concurrency_strategy_traits.hpp; this one exists only to show that the
// vocabulary can NAME every strategy's coordinates.
template<typename _Tag>
struct mock_signature_of;

// mock_signature_of<...>
//   trait: one specialization per tag.
template<>
struct mock_signature_of<none_strategy_tag>
{
    static D_CONSTEXPR mock_signature value()
    {
        return mock_signature{ concurrency_progress::sequential,
                               concurrency_arity::swmr,
                               iteration_consistency::inconsistent,
                               reclamation_obligation::none };
    }
};

template<>
struct mock_signature_of<locked_strategy_tag>
{
    static D_CONSTEXPR mock_signature value()
    {
        return mock_signature{ concurrency_progress::blocking,
                               concurrency_arity::mwmr,
                               iteration_consistency::snapshot,
                               reclamation_obligation::individual };
    }
};

template<>
struct mock_signature_of<cow_strategy_tag>
{
    static D_CONSTEXPR mock_signature value()
    {
        return mock_signature{ concurrency_progress::blocking,
                               concurrency_arity::mwmr,
                               iteration_consistency::snapshot,
                               reclamation_obligation::reference_count };
    }
};

template<>
struct mock_signature_of<rcu_strategy_tag>
{
    static D_CONSTEXPR mock_signature value()
    {
        return mock_signature{ concurrency_progress::lock_free,
                               concurrency_arity::swmr,
                               iteration_consistency::snapshot,
                               reclamation_obligation::grace_period };
    }
};

template<>
struct mock_signature_of<atomic_strategy_tag>
{
    static D_CONSTEXPR mock_signature value()
    {
        return mock_signature{ concurrency_progress::lock_free,
                               concurrency_arity::mwmr,
                               iteration_consistency::inconsistent,
                               reclamation_obligation::none };
    }
};

template<>
struct mock_signature_of<hazard_strategy_tag>
{
    static D_CONSTEXPR mock_signature value()
    {
        return mock_signature{ concurrency_progress::lock_free,
                               concurrency_arity::mwmr,
                               iteration_consistency::weakly_consistent,
                               reclamation_obligation::hazard };
    }
};

template<>
struct mock_signature_of<hybrid_strategy_tag>
{
    static D_CONSTEXPR mock_signature value()
    {
        return mock_signature{ concurrency_progress::blocking,
                               concurrency_arity::mwmr,
                               iteration_consistency::snapshot,
                               reclamation_obligation::reference_count };
    }
};


///////////////////////////////////////////////////////////////////////////////
///                II.  TEST DECLARATIONS                                    ///
///////////////////////////////////////////////////////////////////////////////

// a.   the header contract
//////////////////////////////////////////
bool tests_header_include_guard();
bool tests_header_is_idempotent();
bool tests_header_requires_cpp();
bool tests_header_pulls_only_the_core();
bool tests_header_has_no_trait_layer_cycle();
bool tests_header_defines_no_stray_macros();
bool tests_entities_live_in_the_djinterp_namespace();
bool tests_vocabulary_is_gated_to_cpp11();

// b.   the seven strategy tags
//////////////////////////////////////////
bool tests_tag_types_are_complete();
bool tests_tag_types_are_empty();
bool tests_tag_types_are_trivial();
bool tests_tag_types_are_standard_layout();
bool tests_tag_types_size_and_alignment();
bool tests_tag_types_are_default_constructible();
bool tests_tag_types_are_copyable_and_movable();
bool tests_tag_types_are_aggregates();
bool tests_tag_types_are_not_polymorphic();
bool tests_tag_types_are_constexpr_friendly();
bool tests_tag_types_pass_by_value();
bool tests_tag_types_are_cpp98_pods();

// c.   tag identity + composition
//////////////////////////////////////////
bool tests_tags_are_pairwise_distinct();
bool tests_tags_are_identical_to_themselves();
bool tests_tags_are_unrelated_by_inheritance();
bool tests_tags_do_not_interconvert();
bool tests_tag_dispatch_selects_the_matching_overload();
bool tests_tag_dispatch_accepts_a_derived_tag();
bool tests_tags_key_template_specializations();
bool tests_the_header_declares_exactly_seven_tags();
bool tests_tags_support_empty_base_optimization();
bool tests_tags_compose_by_multiple_inheritance();
bool tests_hybrid_is_a_peer_not_a_composite();
bool tests_the_tag_predicate_rejects_non_tags();

// d.   the self-tagging protocol
//////////////////////////////////////////
bool tests_self_tagging_is_detected();
bool tests_the_declared_tag_is_recovered();
bool tests_every_tag_is_declarable();
bool tests_untagged_types_are_not_detected();
bool tests_detection_is_sfinae_safe();
bool tests_a_private_alias_is_not_detected();
bool tests_the_typedef_form_is_equivalent();
bool tests_an_inherited_alias_is_detected();
bool tests_a_shadowing_alias_wins();
bool tests_a_template_can_self_tag();
bool tests_the_alias_need_not_name_a_known_tag();
bool tests_detection_after_decay();

// e.   concurrency_progress
//////////////////////////////////////////
bool tests_progress_enumerator_values();
bool tests_progress_is_an_enum_not_an_integer();
bool tests_progress_is_scoped();
bool tests_progress_underlying_type();
bool tests_progress_size_and_alignment();
bool tests_progress_is_a_total_order();
bool tests_progress_equality();
bool tests_progress_round_trips_through_the_underlying_type();
bool tests_progress_in_constant_expressions();
bool tests_progress_switch_covers_every_grade();
bool tests_progress_has_exactly_four_grades();
bool tests_progress_out_of_range_value_falls_through();

// f.   concurrency_arity
//////////////////////////////////////////
bool tests_arity_enumerator_values();
bool tests_arity_is_scoped();
bool tests_arity_underlying_type();
bool tests_arity_size_and_alignment();
bool tests_arity_is_a_total_order();
bool tests_arity_is_not_a_boolean();
bool tests_arity_round_trips_through_the_underlying_type();
bool tests_arity_in_constant_expressions();
bool tests_arity_switch_covers_both_arities();
bool tests_arity_has_exactly_two_arities();
bool tests_arity_out_of_range_value_falls_through();

// g.   iteration_consistency
//////////////////////////////////////////
bool tests_iteration_enumerator_values();
bool tests_iteration_is_scoped();
bool tests_iteration_underlying_type();
bool tests_iteration_orders_the_overlap_semantics();
bool tests_iteration_the_degenerate_is_the_floor();
bool tests_iteration_round_trips_through_the_underlying_type();
bool tests_iteration_in_constant_expressions();
bool tests_iteration_switch_covers_every_semantics();
bool tests_iteration_has_exactly_three_semantics();
bool tests_iteration_out_of_range_value_falls_through();

// h.   reclamation_obligation
//////////////////////////////////////////
bool tests_reclamation_enumerator_values();
bool tests_reclamation_is_scoped();
bool tests_reclamation_underlying_type();
bool tests_reclamation_orders_the_disciplines();
bool tests_reclamation_none_is_the_floor();
bool tests_reclamation_round_trips_through_the_underlying_type();
bool tests_reclamation_in_constant_expressions();
bool tests_reclamation_switch_covers_every_obligation();
bool tests_reclamation_has_exactly_six_obligations();
bool tests_reclamation_out_of_range_value_falls_through();
bool tests_reclamation_enumerators_do_not_collide_with_the_tags();

// i.   the signature vocabulary
//////////////////////////////////////////
bool tests_the_four_enums_are_distinct_types();
bool tests_the_enums_do_not_interconvert();
bool tests_coincident_values_are_not_coincident_types();
bool tests_the_enums_are_not_arithmetic();
bool tests_the_vocabulary_composes_into_a_signature();
bool tests_a_strategy_to_signature_mapping_is_expressible();
bool tests_the_signature_is_a_compile_time_value();
bool tests_every_enumerator_is_reachable();
bool tests_the_vocabulary_needs_no_trait_header();


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TESTS_CONCURRENCY_STRATEGY_TAGS_
