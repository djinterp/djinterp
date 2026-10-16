/*******************************************************************************
* djinterp [core]               concurrency_strategy_tags_tests_tag_protocol.cpp
*
*
* path:      /tests/djinterp/core/sync/concurrency_strategy_tags_tests_tag_protocol.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test]        concurrency_strategy_tags_tests_tag_protocol.cpp
//   Section I (c): the PROTOCOL - `using concurrency_strategy_tag = <tag>;`
//
//   This is the one sentence of the header's banner that is an interface
// rather than a definition: a type declares its strategy by naming the alias,
// and the trait system "reads it directly - no structural probing".  The suite
// re-implements that read (has_strategy_tag / strategy_tag_of, in the test
// header) and runs it against types shaped exactly like the production
// primitives that self-tag today.
//
//   The interesting tests here are the ones about what the fast path must NOT
// do: it must not see a PRIVATE alias, it must not hard-error on a type with
// no alias at all (or on a type that is not a class), and it must not pretend
// that an alias naming something other than a tag is a strategy.  Each of
// those is a way a trait built on this protocol could be made to misfire.

// djinterp
#include "concurrency_strategy_tags_tests.hpp"


NS_DJINTERP
NS_TESTING

namespace
{
    // private_tagged
    //   type: declares the alias, but PRIVATELY.  The fast path must not see
    // it - and, just as importantly, must not hard-error on the access check.
    // Access is part of substitution, so this is a clean SFINAE failure.
    class private_tagged
    {
    private:
        using concurrency_strategy_tag = locked_strategy_tag;
    };

    // typedef_tagged
    //   type: the same declaration in the C++98 spelling.  The tags are
    // C++98-shaped, so the protocol that reads them must be too.
    struct typedef_tagged
    {
        typedef rcu_strategy_tag concurrency_strategy_tag;
    };

    // cow_base
    //   type: a self-tagging base - cow_state, in miniature.
    struct cow_base
    {
        using concurrency_strategy_tag = cow_strategy_tag;
    };

    // inheriting_container
    //   type: a container built ON a cow payload, forwarding the base's tag by
    // saying nothing at all.  (cow.hpp: "containers built on top of cow_state
    // typically forward this alias".)
    struct inheriting_container : cow_base
    {};

    // shadowing_container
    //   type: locked metadata around that same cow payload - so it overrides
    // the inherited alias and calls itself hybrid.  Name hiding, used on
    // purpose.
    struct shadowing_container : cow_base
    {
        using concurrency_strategy_tag = hybrid_strategy_tag;
    };

    // template_tagged
    //   type: a TEMPLATE that self-tags, the way atomic_size / cow_state<T> /
    // rcu_protected<T> do.  The tag does not depend on the parameter.
    template<typename _Type>
    struct template_tagged
    {
        using value_type               = _Type;
        using concurrency_strategy_tag = atomic_strategy_tag;
    };

    // computed_tagged
    //   type: a template whose tag is COMPUTED from its parameter - lock-free
    // atomics for a type that fits in a word, a lock otherwise.  The fast path
    // reads whatever the alias resolves to, so a dependent alias is fine.
    template<typename _Type>
    struct computed_tagged
    {
        using concurrency_strategy_tag =
            typename std::conditional<(sizeof(_Type) <= sizeof(void*)),
                                      atomic_strategy_tag,
                                      locked_strategy_tag>::type;
    };

    // alien_tagged
    //   type: declares the alias, but names something that is NOT one of the
    // seven tags.  Structurally it self-tags; semantically it says nothing.
    struct alien_tagged
    {
        using concurrency_strategy_tag = int;
    };

    // wide_payload
    //   type: something too big to be atomic - the computed_tagged negative.
    struct wide_payload
    {
        std::size_t a;
        std::size_t b;
        std::size_t c;
    };
}

/*
tests_self_tagging_is_detected
  the fast path finds the alias on a type that declares it.
  Tests the following:
  - has_strategy_tag holds for a mock of every one of the seven strategies
  - detection is a compile-time answer (it is used in a constant expression)
*/
bool
tests_self_tagging_is_detected()
{
    D_CONSTEXPR bool detected =
        ( has_strategy_tag<mock_none>::value   &&
          has_strategy_tag<mock_locked>::value &&
          has_strategy_tag<mock_cow>::value    &&
          has_strategy_tag<mock_rcu>::value    &&
          has_strategy_tag<mock_atomic>::value &&
          has_strategy_tag<mock_hazard>::value &&
          has_strategy_tag<mock_hybrid>::value );

    static_assert(detected,
                  "a type naming the alias must be seen by the fast path");

    return detected;
}

/*
tests_the_declared_tag_is_recovered
  the fast path recovers the tag the type declared - not merely the fact that
  it declared one.
  Tests the following:
  - strategy_tag_of resolves to the exact tag for each of the seven mocks
  - a type declaring `cow` never reads back as `rcu`
*/
bool
tests_the_declared_tag_is_recovered()
{
    D_CONSTEXPR bool recovered =
        ( std::is_same<strategy_tag_of_t<mock_none>,
                       none_strategy_tag>::value   &&
          std::is_same<strategy_tag_of_t<mock_locked>,
                       locked_strategy_tag>::value &&
          std::is_same<strategy_tag_of_t<mock_cow>,
                       cow_strategy_tag>::value    &&
          std::is_same<strategy_tag_of_t<mock_rcu>,
                       rcu_strategy_tag>::value    &&
          std::is_same<strategy_tag_of_t<mock_atomic>,
                       atomic_strategy_tag>::value &&
          std::is_same<strategy_tag_of_t<mock_hazard>,
                       hazard_strategy_tag>::value &&
          std::is_same<strategy_tag_of_t<mock_hybrid>,
                       hybrid_strategy_tag>::value );

    static_assert(recovered, "the alias must read back as the tag declared");

    return recovered;
}

/*
tests_every_tag_is_declarable
  the whole path, end to end, for all seven tags: declare, read back, dispatch.
  Tests the following:
  - the tag read off each mock selects that tag's dispatch arm
  - the seven arms are the seven distinct arms, so every tag in section I is
    reachable THROUGH the protocol (this is the section's coverage sweep: no
    tag is declarable-in-principle only)
*/
bool
tests_every_tag_is_declarable()
{
    const bool end_to_end =
        ( (dispatch_on(strategy_tag_of_t<mock_none>())   == 0) &&
          (dispatch_on(strategy_tag_of_t<mock_locked>()) == 1) &&
          (dispatch_on(strategy_tag_of_t<mock_cow>())    == 2) &&
          (dispatch_on(strategy_tag_of_t<mock_rcu>())    == 3) &&
          (dispatch_on(strategy_tag_of_t<mock_atomic>()) == 4) &&
          (dispatch_on(strategy_tag_of_t<mock_hazard>()) == 5) &&
          (dispatch_on(strategy_tag_of_t<mock_hybrid>()) == 6) );

    // and every tag recovered through the protocol is one of the seven
    D_CONSTEXPR bool all_known =
        ( is_strategy_tag<strategy_tag_of_t<mock_none>>::value   &&
          is_strategy_tag<strategy_tag_of_t<mock_locked>>::value &&
          is_strategy_tag<strategy_tag_of_t<mock_cow>>::value    &&
          is_strategy_tag<strategy_tag_of_t<mock_rcu>>::value    &&
          is_strategy_tag<strategy_tag_of_t<mock_atomic>>::value &&
          is_strategy_tag<strategy_tag_of_t<mock_hazard>>::value &&
          is_strategy_tag<strategy_tag_of_t<mock_hybrid>>::value );

    return (end_to_end && all_known);
}

/*
tests_untagged_types_are_not_detected
  a type that declares nothing is seen to declare nothing.
  Tests the following:
  - has_strategy_tag is false for a class with members but no alias
  - it is false for one of the tags themselves (a tag does not tag itself)
  - the negative is quiet: no diagnostic, no hard error
*/
bool
tests_untagged_types_are_not_detected()
{
    D_CONSTEXPR bool not_detected =
        ( !has_strategy_tag<mock_untagged>::value       &&
          !has_strategy_tag<cow_strategy_tag>::value    &&
          !has_strategy_tag<hybrid_strategy_tag>::value );

    static_assert(not_detected,
                  "a type with no alias must not be seen to have one");

    return not_detected;
}

/*
tests_detection_is_sfinae_safe
  the fast path may be pointed at ANY type without breaking the build.
  Tests the following:
  - a fundamental type, void, a pointer, an array, a function type and an enum
    all answer false rather than failing to compile
  - which is what lets a trait apply the probe before it knows what it holds
*/
bool
tests_detection_is_sfinae_safe()
{
    D_CONSTEXPR bool safe =
        ( !has_strategy_tag<int>::value          &&
          !has_strategy_tag<void>::value         &&
          !has_strategy_tag<std::size_t>::value  &&
          !has_strategy_tag<int*>::value         &&
          !has_strategy_tag<int[3]>::value       &&
          !has_strategy_tag<int(int)>::value     &&
          !has_strategy_tag<double>::value );

    D_CONSTEXPR bool enums_safe =
        ( !has_strategy_tag<concurrency_progress>::value &&
          !has_strategy_tag<reclamation_obligation>::value );

    static_assert(safe, "the probe must be SFINAE-safe for any type");

    return (safe && enums_safe);
}

/*
tests_a_private_alias_is_not_detected
  an alias the world cannot name is an alias the fast path cannot read.
  Tests the following:
  - a class whose `concurrency_strategy_tag` is private answers FALSE
  - and the access check is a substitution failure, not a hard error - the TU
    compiles, which is half the assertion
  This is the sharp edge of "no structural probing": the trait system reads a
  PUBLIC alias, so a type that hides one has not declared a strategy.
*/
bool
tests_a_private_alias_is_not_detected()
{
    D_CONSTEXPR bool hidden = !has_strategy_tag<private_tagged>::value;

    static_assert(hidden, "a private alias must not be visible to the probe");

    return hidden;
}

/*
tests_the_typedef_form_is_equivalent
  the C++98 spelling of the declaration works identically.
  Tests the following:
  - `typedef rcu_strategy_tag concurrency_strategy_tag;` is detected
  - and resolves to the same tag the `using` form would
  - so a C++98 translation unit can still self-tag, as section I promises
*/
bool
tests_the_typedef_form_is_equivalent()
{
    D_CONSTEXPR bool detected = has_strategy_tag<typedef_tagged>::value;

    D_CONSTEXPR bool same_tag =
        std::is_same<strategy_tag_of_t<typedef_tagged>,
                     rcu_strategy_tag>::value;

    static_assert(detected && same_tag,
                  "the typedef spelling must be equivalent to the using one");

    return (detected && same_tag);
}

/*
tests_an_inherited_alias_is_detected
  a container that inherits its payload inherits its payload's strategy.
  Tests the following:
  - a type deriving from a self-tagging base, and saying nothing itself, is
    detected
  - the tag it reports is the base's
  - so forwarding a strategy costs a derivation and no declaration
*/
bool
tests_an_inherited_alias_is_detected()
{
    D_CONSTEXPR bool detected =
        has_strategy_tag<inheriting_container>::value;

    D_CONSTEXPR bool reports_the_base_tag =
        std::is_same<strategy_tag_of_t<inheriting_container>,
                     cow_strategy_tag>::value;

    static_assert(detected && reports_the_base_tag,
                  "an inherited alias must be found, and must be the base's");

    return (detected && reports_the_base_tag);
}

/*
tests_a_shadowing_alias_wins
  a type may override the strategy it inherits.
  Tests the following:
  - a derived type that redeclares the alias reports ITS tag, not the base's
  - the base still reports its own (the derivation did not disturb it)
  - which is exactly how "locked metadata around a cow payload" declares itself
    hybrid while its payload remains cow
*/
bool
tests_a_shadowing_alias_wins()
{
    D_CONSTEXPR bool derived_wins =
        std::is_same<strategy_tag_of_t<shadowing_container>,
                     hybrid_strategy_tag>::value;

    D_CONSTEXPR bool base_unchanged =
        std::is_same<strategy_tag_of_t<cow_base>,
                     cow_strategy_tag>::value;

    static_assert(derived_wins && base_unchanged,
                  "a redeclared alias must hide the inherited one");

    return (derived_wins && base_unchanged);
}

/*
tests_a_template_can_self_tag
  a class TEMPLATE can declare a strategy - which is what the production
  primitives actually are.
  Tests the following:
  - every instantiation of a self-tagging template is detected, and reports the
    same tag
  - a template whose alias is COMPUTED from its parameter is read as computed:
    a word-sized payload comes back atomic, a wide one comes back locked
  - so the fast path reads the alias, whatever it took to produce it
*/
bool
tests_a_template_can_self_tag()
{
    D_CONSTEXPR bool instantiations_detected =
        ( has_strategy_tag<template_tagged<int>>::value    &&
          has_strategy_tag<template_tagged<double>>::value &&
          std::is_same<strategy_tag_of_t<template_tagged<int>>,
                       atomic_strategy_tag>::value         &&
          std::is_same<strategy_tag_of_t<template_tagged<double>>,
                       atomic_strategy_tag>::value );

    D_CONSTEXPR bool computed_alias_is_read =
        ( std::is_same<strategy_tag_of_t<computed_tagged<char>>,
                       atomic_strategy_tag>::value         &&
          std::is_same<strategy_tag_of_t<computed_tagged<wide_payload>>,
                       locked_strategy_tag>::value );

    static_assert(instantiations_detected && computed_alias_is_read,
                  "a template must be able to declare (and compute) its tag");

    return (instantiations_detected && computed_alias_is_read);
}

/*
tests_the_alias_need_not_name_a_known_tag
  the fast path is STRUCTURAL: it finds an alias, it does not validate one.
  Tests the following:
  - a type aliasing `concurrency_strategy_tag` to `int` IS detected
  - but the thing it named is not one of the seven
  - so "declared a strategy" and "declared a strategy the system knows" are two
    questions, and a trait built on this protocol must ask both
*/
bool
tests_the_alias_need_not_name_a_known_tag()
{
    D_CONSTEXPR bool structurally_detected =
        has_strategy_tag<alien_tagged>::value;

    D_CONSTEXPR bool not_a_known_tag =
        !is_strategy_tag<strategy_tag_of_t<alien_tagged>>::value;

    static_assert(structurally_detected && not_a_known_tag,
                  "the probe must be structural, and no more than structural");

    return (structurally_detected && not_a_known_tag);
}

/*
tests_detection_after_decay
  the probe reads a TYPE, so a reference or a pointer to a tagged type is not
  a tagged type.
  Tests the following:
  - a reference to a self-tagging type is NOT detected, nor is a pointer to one
  - a cv-qualified one IS (the member is still there to be named)
  - std::decay recovers the class from either, and detection then succeeds and
    resolves to the same tag
  This is why the trait surface decays before it probes; the suite pins the
  behaviour rather than assuming it.
*/
bool
tests_detection_after_decay()
{
    D_CONSTEXPR bool references_and_pointers_are_not_classes =
        ( !has_strategy_tag<mock_cow&>::value        &&
          !has_strategy_tag<const mock_cow&>::value  &&
          !has_strategy_tag<mock_cow*>::value );

    D_CONSTEXPR bool cv_survives = has_strategy_tag<const mock_cow>::value;

    using decayed = typename std::decay<const mock_cow&>::type;

    D_CONSTEXPR bool decay_recovers =
        ( has_strategy_tag<decayed>::value &&
          std::is_same<strategy_tag_of_t<decayed>, cow_strategy_tag>::value );

    static_assert(decay_recovers,
                  "std::decay must recover the tagged class from a reference");

    return ( references_and_pointers_are_not_classes &&
             cv_survives                             &&
             decay_recovers );
}


NS_END  // testing
NS_END  // djinterp
