/*******************************************************************************
* djinterp [core]               container_comparison_traits_tests_robustness.cpp
*
*
* path:      /tests/djinterp/core/container/traits/container_comparison_traits_tests_robustness.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// std
#include <type_traits>
#include <utility>
#include <vector>
// djinterp
#include "container_comparison_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


///////////////////////////////////////////////////////////////////////////////
///                PROBES + BINDERS                                          ///
///////////////////////////////////////////////////////////////////////////////
//   The traits in this header are BINARY, and the toolkit's quantifiers and
// cv-ref matrix take a ONE-parameter template-template argument.  So each battery
// binds one operand and varies the other - which is not a limitation but the
// right shape: cv-ref agnosticism is a property of each operand SEPARATELY, and a
// matrix that varied both at once would confound them.

// p_nested_type
//   probe: does the instantiated trait carry a nested `type`?  Probed rather than
// named, because naming a member that is not there is a hard error, not a false.
D_TEST_TYPE_PROBE(p_nested_type, typename _Type::type)

// p_bool_conv
//   probe: is the instantiated trait convertible to bool?  The value traits in
// this header must NOT be - see tests_trait_shape for why that is a feature.
D_TEST_TYPE_PROBE(p_bool_conv,
                  decltype(static_cast<bool>(std::declval<const _Type&>())))

// refines_runtime_seq / refined_by_runtime_seq
//   traits: refinement_of with one operand pinned, so the toolkit can run its
// cv-ref matrix over the other.  LEFT-varying and RIGHT-varying, because a trait
// that cleaned one operand and not the other would pass a one-sided check.
template<typename _Type>
struct refines_runtime_seq
    : ::djinterp::bool_constant<
          refinement_of<_Type, runtime_seq>::value == order_relation::greater>
{};

template<typename _Type>
struct refined_by_runtime_seq
    : ::djinterp::bool_constant<
          refinement_of<runtime_seq, _Type>::value == order_relation::greater>
{};

// self_realization_equal / self_realization_comparable
//   traits: the reflexive forms, as ONE-parameter traits the quantifiers can take.
template<typename _Type>
struct self_realization_equal : realization_equal<_Type, _Type>
{};

template<typename _Type>
struct self_realization_comparable : realization_comparable<_Type, _Type>
{};

// content_is_none
//   trait: true iff a type has no content rung.  Every hostile shape should.
template<typename _Type>
struct content_is_none
    : ::djinterp::bool_constant<
          native_content_level_of<_Type>::value == content_level::none>
{};


///////////////////////////////////////////////////////////////////////////////
///                BUILD-TIME PINS                                           ///
///////////////////////////////////////////////////////////////////////////////
//   Properties of the PROBES, not of any one answer.  If a trait stops cleaning
// its operands, or starts diagnosing where it should report, every other check in
// this suite becomes meaningless.
//
//   Note which list each uses.  native_content_level_of, discipline_relation and
// storage_relation run over the FULL zoo, incomplete fixture included, because
// they genuinely tolerate it.  Everything downstream of the lifetime axis runs
// over the complete-types-only list, because it genuinely does not - see
// tests_incomplete_type.

D_TEST_STATIC(::djinterp::test::holds_for_all<content_is_none,
                                              D_TEST_HOSTILE_TYPES>::value);

D_TEST_STATIC(
    ::djinterp::test::holds_for_all<self_realization_equal,
                                    D_TEST_HOSTILE_TYPES_COMPLETE>::value);

D_TEST_STATIC(
    ::djinterp::test::trait_ignores_cvref<refines_runtime_seq,
                                          capacity_seq>::value);


/*
tests_cvref_axes
  Every axis must answer identically for T, const T, volatile T, const volatile T,
  T&, const T&, T&& and const T&& - on BOTH operands.

  Each trait cleans its arguments with clean_t at the top of its private section.
  That is one clean_t per operand per trait, six traits, and a single omission is
  invisible to every test that only ever passes bare types.  This is that test.

  Tests the following:
  - the LEFT operand: the matrix over refines_runtime_seq, which varies the left
  - the RIGHT operand: the matrix over refined_by_runtime_seq, which varies the
    right
  - each axis separately, by hand, since the axes are binary and the matrix is not
  - the report names the first disagreeing cell, so a regression says
    "const _Type&" rather than merely "cv-ref"
*/
bool
tests_cvref_axes()
{
    bool ok = true;

    // the matrix, on each operand in turn
    D_CCM_CHECK((trait_ignores_cvref<refines_runtime_seq,
                                     capacity_seq>::value));
    D_CCM_CHECK((trait_ignores_cvref<refines_runtime_seq,
                                     runtime_seq>::value));
    D_CCM_CHECK((trait_ignores_cvref<refined_by_runtime_seq,
                                     const_seq>::value));
    D_CCM_CHECK((trait_ignores_cvref<refined_by_runtime_seq,
                                     runtime_seq>::value));

    D_CCM_CHECK((trait_across_cvref<refines_runtime_seq,
                                    capacity_seq>().all()));
    D_CCM_CHECK((trait_across_cvref<refines_runtime_seq,
                                    runtime_seq>().none()));

    // each axis, by hand, over all eight cv-ref forms of the LEFT operand
    #define D_CCM_CVREF_LEFT(TRAIT, EXPECTED, TYPE, OTHER)                    \
        D_CCM_REL((EXPECTED), (TRAIT<TYPE, OTHER>::value));                   \
        D_CCM_REL((EXPECTED), (TRAIT<const TYPE, OTHER>::value));             \
        D_CCM_REL((EXPECTED), (TRAIT<volatile TYPE, OTHER>::value));          \
        D_CCM_REL((EXPECTED), (TRAIT<const volatile TYPE, OTHER>::value));    \
        D_CCM_REL((EXPECTED), (TRAIT<TYPE&, OTHER>::value));                  \
        D_CCM_REL((EXPECTED), (TRAIT<const TYPE&, OTHER>::value));            \
        D_CCM_REL((EXPECTED), (TRAIT<TYPE&&, OTHER>::value));                 \
        D_CCM_REL((EXPECTED), (TRAIT<const TYPE&&, OTHER>::value))

    D_CCM_CVREF_LEFT(discipline_relation, order_relation::greater,
                     capacity_seq, runtime_seq);
    D_CCM_CVREF_LEFT(lifetime_relation, order_relation::greater,
                     compile_seq, runtime_seq);
    D_CCM_CVREF_LEFT(iterability_relation, order_relation::incomparable,
                     compile_const_seq, runtime_seq);
    D_CCM_CVREF_LEFT(storage_relation, order_relation::incomparable,
                     static_seq, dynamic_seq);
    D_CCM_CVREF_LEFT(refinement_of, order_relation::greater,
                     capacity_seq, runtime_seq);

    #undef D_CCM_CVREF_LEFT

    // and over the RIGHT operand
    #define D_CCM_CVREF_RIGHT(TRAIT, EXPECTED, OTHER, TYPE)                   \
        D_CCM_REL((EXPECTED), (TRAIT<OTHER, TYPE>::value));                   \
        D_CCM_REL((EXPECTED), (TRAIT<OTHER, const TYPE>::value));             \
        D_CCM_REL((EXPECTED), (TRAIT<OTHER, volatile TYPE>::value));          \
        D_CCM_REL((EXPECTED), (TRAIT<OTHER, const volatile TYPE>::value));    \
        D_CCM_REL((EXPECTED), (TRAIT<OTHER, TYPE&>::value));                  \
        D_CCM_REL((EXPECTED), (TRAIT<OTHER, const TYPE&>::value));            \
        D_CCM_REL((EXPECTED), (TRAIT<OTHER, TYPE&&>::value));                 \
        D_CCM_REL((EXPECTED), (TRAIT<OTHER, const TYPE&&>::value))

    D_CCM_CVREF_RIGHT(discipline_relation, order_relation::greater,
                      capacity_seq, runtime_seq);
    D_CCM_CVREF_RIGHT(refinement_of, order_relation::less,
                      runtime_seq, capacity_seq);

    #undef D_CCM_CVREF_RIGHT

    // native_content_level_of cleans too
    D_CCM_LEVEL(content_level::seq,
                native_content_level_of<const volatile runtime_seq&>::value);
    D_CCM_LEVEL(content_level::str,
                native_content_level_of<nested_seq&&>::value);

    return ok;
}

/*
tests_cvref_profile
  comparison_profile cleans BOTH operands - it declares `using L = clean_t<_Left>`
  and `using R = clean_t<_Right>` and passes those to every axis.  So the whole
  profile is cv-ref agnostic, and so are the two bool traits built on it.
  Tests the following:
  - every member of the profile agrees across the cv-ref forms of both operands
  - realization_equal and realization_comparable do too
*/
bool
tests_cvref_profile()
{
    bool ok = true;

    #define D_CCM_PROFILE_CVREF(LEFT, RIGHT)                                  \
        D_CCM_CHECK((comparison_profile<const LEFT&, RIGHT&&>::refinement) == \
                    (comparison_profile<LEFT, RIGHT>::refinement));           \
        D_CCM_CHECK(                                                          \
            (comparison_profile<const LEFT&, RIGHT&&>::content_shared) ==     \
            (comparison_profile<LEFT, RIGHT>::content_shared));               \
        D_CCM_CHECK(                                                          \
            (comparison_profile<volatile LEFT, const RIGHT>::discipline_order)\
            == (comparison_profile<LEFT, RIGHT>::discipline_order));          \
        D_CCM_CHECK((refinement_of<const LEFT&, const RIGHT&>::value) ==      \
                    (refinement_of<LEFT, RIGHT>::value));                     \
        D_CCM_CHECK((realization_equal<LEFT&&, const RIGHT&>::value) ==       \
                    (realization_equal<LEFT, RIGHT>::value));                 \
        D_CCM_CHECK((realization_comparable<const LEFT, volatile RIGHT>::value)\
                    == (realization_comparable<LEFT, RIGHT>::value))

    D_CCM_PROFILE_CVREF(compile_seq,       runtime_seq);
    D_CCM_PROFILE_CVREF(capacity_seq,      runtime_seq);
    D_CCM_PROFILE_CVREF(sorted_seq,        capacity_seq);
    D_CCM_PROFILE_CVREF(compile_const_seq, runtime_seq);
    D_CCM_PROFILE_CVREF(nested_seq,        runtime_seq);
    D_CCM_PROFILE_CVREF(static_seq,        dynamic_seq);
    D_CCM_PROFILE_CVREF(runtime_seq,       runtime_seq);

    #undef D_CCM_PROFILE_CVREF

    // and reflexivity survives the cv-ref forms, which is the check that would
    // catch a trait that cleaned one operand and not the other
    D_CCM_REL(order_relation::equivalent,
              (refinement_of<const runtime_seq&, runtime_seq&&>::value));
    D_CCM_REL(order_relation::equivalent,
              (refinement_of<volatile compile_seq, const compile_seq&>::value));
    D_CCM_CHECK((realization_equal<const runtime_seq&, runtime_seq>::value));

    return ok;
}

/*
tests_hostile_types
  Every trait must survive every adversarial shape - void, function types,
  arrays, references, member pointers, enums, and the class-type adversaries -
  and must REPORT rather than DIAGNOSE.
  Tests the following:
  - none of them has a content rung
  - none of them is a container on any axis, so every pair of them is
    realization-EQUAL (they agree by agreeing about nothing)
  - reflexivity holds over the whole zoo
  - the traits instantiate over every hostile shape without a hard error.
    count_holds does not short-circuit, so a trait that explodes on the twentieth
    of twenty-five types is still reached
*/
bool
tests_hostile_types()
{
    bool ok = true;

    // no hostile shape has a content rung
    D_CCM_CHECK((holds_for_all<content_is_none,
                               D_TEST_HOSTILE_TYPES>::value));
    D_CCM_CHECK((count_holds<content_is_none,
                             D_TEST_HOSTILE_TYPES>::value == 26));

    // reflexivity, over the whole zoo
    D_CCM_CHECK((holds_for_all<self_realization_equal,
                               D_TEST_HOSTILE_TYPES_COMPLETE>::value));
    D_CCM_CHECK((holds_for_all<self_realization_comparable,
                               D_TEST_HOSTILE_TYPES_COMPLETE>::value));

    // ...and none of them refines a real container, nor is refined by one - the
    // iterability axis vetoes both directions
    D_CCM_CHECK((holds_for_none<refines_runtime_seq,
                                D_TEST_HOSTILE_TYPES_COMPLETE>::value));
    D_CCM_CHECK((holds_for_none<refined_by_runtime_seq,
                                D_TEST_HOSTILE_TYPES_COMPLETE>::value));

    // the shapes most likely to break a probe, by hand
    D_CCM_LEVEL(content_level::none, native_content_level_of<void>::value);
    D_CCM_LEVEL(content_level::none, native_content_level_of<fn_type>::value);
    D_CCM_LEVEL(content_level::none, native_content_level_of<arr_type>::value);
    D_CCM_LEVEL(content_level::none, native_content_level_of<int&>::value);

    D_CCM_REL(order_relation::equivalent,
              (refinement_of<void, int>::value));
    D_CCM_REL(order_relation::equivalent,
              (refinement_of<fn_type, arr_type>::value));
    D_CCM_REL(order_relation::equivalent,
              (refinement_of<fixtures::evil, fixtures::greedy>::value));
    D_CCM_REL(order_relation::incomparable,
              (refinement_of<fixtures::abstract, runtime_seq>::value));

    // two non-containers "agree" on every axis, so they are realization-equal.
    // Correct, and worth pinning: the trait says nothing about whether either of
    // them is a container at all - that is what content_shared is for
    D_CCM_CHECK((realization_equal<void, int>::value));
    D_CCM_CHECK((comparison_profile<void, int>::content_shared ==
                 content_level::none));

    return ok;
}

/*
tests_incomplete_type
  THE COMPLETENESS BOUNDARY.  An incomplete type is tolerated by exactly half of
  this header, and the line falls in a place nobody chose.

  Tests the following:
  - native_content_level_of, discipline_relation and storage_relation all answer
    for an incomplete type, without diagnosing
  - lifetime_relation, iterability_relation, comparison_profile and refinement_of
    HARD-ERROR, so they cannot be checked here at all

  WHY.  The lifetime axis routes through container_lifetime (constexpr_container_
  traits.hpp) -> lifetime_of (meta/lifetime.hpp) -> is_literal_type, which on
  C++17+ is spelled

      std::is_scalar<T> ||
      ( std::is_trivially_destructible<T> &&
        std::is_trivially_default_constructible<T> )

  and the standard MANDATES a complete type for both of those.  Not "returns
  false" - mandates.  libstdc++ enforces it with a static_assert.

  It is the SAME defect constexpr_container_traits.hpp has, inherited by the same
  route, and it costs the same thing: a trait that cannot be asked about an
  incomplete type cannot be used in a forward-declared context, which is most of
  the places a container comparison is wanted.  Fixing lifetime.hpp fixes both.

  The checks below pin what CAN be pinned: the boundary itself.  Three of the six
  traits tolerate incompleteness, and that is a real property worth protecting -
  it is what would let the other three be fixed without touching them.
*/
bool
tests_incomplete_type()
{
    bool ok = true;

    using incomplete = fixtures::incomplete;

    // the three that tolerate it
    D_CCM_LEVEL(content_level::none,
                native_content_level_of<incomplete>::value);
    D_CCM_REL(order_relation::equivalent,
              (discipline_relation<incomplete, incomplete>::value));
    D_CCM_REL(order_relation::equivalent,
              (storage_relation<incomplete, incomplete>::value));

    // ...and they do so against a real container, too
    D_CCM_REL(order_relation::greater,
              (discipline_relation<incomplete, runtime_seq>::value));
    D_CCM_REL(order_relation::incomparable,
              (storage_relation<incomplete, static_seq>::value));

    // through the cv-ref forms as well
    D_CCM_LEVEL(content_level::none,
                native_content_level_of<const incomplete&>::value);
    D_CCM_REL(order_relation::equivalent,
              (discipline_relation<incomplete&&,
                                   const incomplete&>::value));

    // lifetime_relation<incomplete, ...>, iterability_relation<incomplete, ...>,
    // comparison_profile<incomplete, ...> and refinement_of<incomplete, ...> are
    // all HARD ERRORS and cannot be named here.  Uncomment any of them to
    // reproduce:
    //
    //   D_CCM_REL(order_relation::equivalent,
    //             (refinement_of<incomplete, incomplete>::value));
    //
    //   error: invalid use of incomplete type 'struct ...::incomplete'
    //   error: static assertion failed: template argument must be a complete
    //          class or an unbounded array
    //   note: in lifetime.hpp, is_trivially_destructible<clean_t<_Type>>

    return ok;
}

/*
tests_trait_shape
  Is each trait well-shaped?
  Tests the following:
  - realization_equal and realization_comparable ARE well-formed bool traits: they
    derive from std::integral_constant, so they carry value_type, ::type, a bool
    conversion and a call operator, and they compose with everything in the
    toolkit.  The header gets this right, where bounded_container_traits.hpp's two
    verdicts do not
  - the six VALUE traits - native_content_level_of, discipline_relation,
    lifetime_relation, iterability_relation, storage_relation, refinement_of - are
    NOT bool traits, and correctly so: their values are order_relations and
    content_levels, not bools.  A caller that folded one into a boolean would be
    reading `equivalent` as false, since it is enumerator zero
  - but note what they also lack: a `::type`.  None of the six supplies a carrier,
    where their neighbours do - multiplicity_kind_of has one, and so does
    capacity_bound_of in bounded_container_traits.hpp.  A caller who wants to
    tag-dispatch on a refinement has to build the integral_constant by hand for
    these six and not for the others
  - and the trap that makes it matter: order_relation::equivalent is ZERO, so
    `if (refinement_of<A, B>::value)` is false exactly when the two types AGREE.
    A bool trait would have made that impossible; a value trait with no conversion
    makes it a compile error, which is the next best thing - and it is worth
    checking that the conversion really is absent
*/
bool
tests_trait_shape()
{
    bool ok = true;

    // the two bool traits are bool traits
    D_CCM_CHECK(is_bool_trait<realization_equal<runtime_seq,
                                                runtime_seq>>::value);
    D_CCM_CHECK(is_bool_trait<realization_comparable<compile_seq,
                                                     runtime_seq>>::value);
    D_CCM_CHECK((trait_is_well_formed<self_realization_equal,
                                      D_TEST_HOSTILE_TYPES_COMPLETE>::value));
    D_CCM_CHECK((trait_is_well_formed<self_realization_comparable,
                                      D_TEST_HOSTILE_TYPES_COMPLETE>::value));

    // the value traits are NOT bool traits - and must not be
    D_CCM_CHECK(!is_bool_trait<refinement_of<compile_seq,
                                             runtime_seq>>::value);
    D_CCM_CHECK(!is_bool_trait<discipline_relation<compile_seq,
                                                   runtime_seq>>::value);
    D_CCM_CHECK(!is_bool_trait<lifetime_relation<compile_seq,
                                                 runtime_seq>>::value);
    D_CCM_CHECK(!is_bool_trait<iterability_relation<compile_seq,
                                                    runtime_seq>>::value);
    D_CCM_CHECK(!is_bool_trait<storage_relation<static_seq,
                                                dynamic_seq>>::value);
    D_CCM_CHECK(!is_bool_trait<native_content_level_of<runtime_seq>>::value);
    D_CCM_CHECK(!is_bool_trait<comparison_profile<compile_seq,
                                                  runtime_seq>>::value);

    // ...and none of the six supplies a ::type carrier, where its neighbours do
    D_CCM_CHECK((!::djinterp::is_detected<
                     p_nested_type,
                     refinement_of<compile_seq, runtime_seq>>::value));
    D_CCM_CHECK((!::djinterp::is_detected<
                     p_nested_type,
                     native_content_level_of<runtime_seq>>::value));
    D_CCM_CHECK((!::djinterp::is_detected<
                     p_nested_type,
                     discipline_relation<compile_seq, runtime_seq>>::value));

    // ...whereas multiplicity_kind_of, one dependency down, does
    D_CCM_CHECK((::djinterp::is_detected<
                     p_nested_type,
                     multiplicity_kind_of<runtime_seq>>::value));

    // THE TRAP.  `equivalent` is enumerator ZERO, so a boolean reading of a
    // refinement would be false exactly when the two types AGREE
    D_CCM_CHECK(static_cast<int>(order_relation::equivalent) == 0);
    D_CCM_REL(order_relation::equivalent,
              (refinement_of<runtime_seq, runtime_seq>::value));

    // the value traits have no conversion to bool, so that reading is a COMPILE
    // ERROR rather than a silent inversion.  Probed rather than named (see
    // p_bool_conv at the top of this file - the declarators emit alias templates,
    // which may not be declared inside a function)
    D_CCM_CHECK((!::djinterp::is_detected<
                     p_bool_conv,
                     refinement_of<runtime_seq, runtime_seq>>::value));

    // ...and the two BOOL traits do have it, which is what makes them safe to
    // branch on
    D_CCM_CHECK((::djinterp::is_detected<
                     p_bool_conv,
                     realization_equal<runtime_seq, runtime_seq>>::value));

    return ok;
}


NS_END  // testing
NS_END  // djinterp
