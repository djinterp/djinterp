/*******************************************************************************
* djinterp [core]                container_comparison_traits_tests_relations.cpp
*
*
* path:      /tests/djinterp/core/container/traits/container_comparison_traits_tests_relations.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// std
#include <cstring>
#include <type_traits>
// djinterp
#include "container_comparison_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_order_relation_enum
  The order_relation enum itself.
  Tests the following:
  - it is a scoped enum, so a caller cannot compare it against an int by accident
  - the four enumerators are distinct
  - their VALUES are 0, 1, 2, 3.  Pinned deliberately: the relation is the thing a
    diagnostic, a serialized profile or an agent-facing summary carries, and a
    reordering would silently change every recorded value.  `equivalent` being
    zero also matters - it is the unit of the meet, and a fold that started from a
    zero-initialized accumulator would depend on it
*/
bool
tests_order_relation_enum()
{
    bool ok = true;

    using underlying = std::underlying_type<order_relation>::type;

    D_CCM_CHECK(std::is_enum<order_relation>::value);
    D_CCM_CHECK(!std::is_convertible<order_relation, underlying>::value);

    D_CCM_CHECK(order_relation::equivalent != order_relation::less);
    D_CCM_CHECK(order_relation::less != order_relation::greater);
    D_CCM_CHECK(order_relation::greater != order_relation::incomparable);
    D_CCM_CHECK(order_relation::equivalent != order_relation::incomparable);

    D_CCM_CHECK(static_cast<int>(order_relation::equivalent) == 0);
    D_CCM_CHECK(static_cast<int>(order_relation::less) == 1);
    D_CCM_CHECK(static_cast<int>(order_relation::greater) == 2);
    D_CCM_CHECK(static_cast<int>(order_relation::incomparable) == 3);

    return ok;
}

/*
tests_order_relation_name
  order_relation_name - the stable spelling.
  Tests the following:
  - each enumerator maps to its own spelling, and the four are distinct
  - the function is constexpr and noexcept
  - an OUT-OF-RANGE value returns "incomparable"

  The last is a fall-through: the chain tests equivalent, less and greater and
  returns "incomparable" for anything else.  For a scoped enum that is only
  reachable by an explicit cast, and "incomparable" is at least the SAFE default -
  a value nobody can interpret should read as "I cannot relate these".  Contrast
  content_level_name, whose fall-through is the FINEST rung; see
  tests_content_level_name.
*/
bool
tests_order_relation_name()
{
    bool ok = true;

    D_CCM_CHECK(std::strcmp(order_relation_name(order_relation::equivalent),
                            "equivalent") == 0);
    D_CCM_CHECK(std::strcmp(order_relation_name(order_relation::less),
                            "less") == 0);
    D_CCM_CHECK(std::strcmp(order_relation_name(order_relation::greater),
                            "greater") == 0);
    D_CCM_CHECK(std::strcmp(order_relation_name(order_relation::incomparable),
                            "incomparable") == 0);

    // distinct, and never null
    D_CCM_CHECK(order_relation_name(order_relation::equivalent) != nullptr);
    D_CCM_CHECK(std::strcmp(order_relation_name(order_relation::less),
                            order_relation_name(order_relation::greater)) != 0);

    // constexpr: usable during constant evaluation
    D_CONSTEXPR const char* greater_name =
        order_relation_name(order_relation::greater);

    D_CCM_CHECK(std::strcmp(greater_name, "greater") == 0);

    // noexcept
    D_CCM_CHECK(noexcept(order_relation_name(order_relation::less)));

    // the fall-through, which happens to be the safe default
    const order_relation out_of_range = static_cast<order_relation>(42);

    D_CCM_CHECK(order_relation_name(out_of_range) != nullptr);
    D_CCM_CHECK(std::strcmp(order_relation_name(out_of_range),
                            "incomparable") == 0);

    return ok;
}

/*
tests_order_relation_meet_table
  order_relation_meet - ALL SIXTEEN CELLS.

  This is the function the whole module folds through: the refinement is three
  nested meets, so a single wrong cell corrupts every verdict that passes through
  it, and the wrong cells are exactly the ones a spot-check never reaches.  The
  table is written out in full rather than sampled.

                        equivalent    less          greater       incomparable
      equivalent        equivalent    less          greater       incomparable
      less              less          less          INCOMPARABLE  incomparable
      greater           greater       INCOMPARABLE  greater       incomparable
      incomparable      incomparable  incomparable  incomparable  incomparable

  Tests the following:
  - all sixteen cells
  - the two capitalised ones are the point of the whole function: OPPOSING
    directions collapse to incomparable.  A product order says "L refines R" only
    if L stands at least as high on EVERY axis, so an axis that says `less` and an
    axis that says `greater` do not average out - they mean neither refines the
    other, and a meet that took the max, or the first non-equivalent, or the more
    common, would be wrong here and only here
*/
bool
tests_order_relation_meet_table()
{
    bool ok = true;

    D_CONSTEXPR order_relation eq = order_relation::equivalent;
    D_CONSTEXPR order_relation ls = order_relation::less;
    D_CONSTEXPR order_relation gt = order_relation::greater;
    D_CONSTEXPR order_relation ic = order_relation::incomparable;

    // row: equivalent (the unit)
    D_CCM_REL(eq, order_relation_meet(eq, eq));
    D_CCM_REL(ls, order_relation_meet(eq, ls));
    D_CCM_REL(gt, order_relation_meet(eq, gt));
    D_CCM_REL(ic, order_relation_meet(eq, ic));

    // row: less
    D_CCM_REL(ls, order_relation_meet(ls, eq));
    D_CCM_REL(ls, order_relation_meet(ls, ls));
    D_CCM_REL(ic, order_relation_meet(ls, gt));     // *** opposing directions
    D_CCM_REL(ic, order_relation_meet(ls, ic));

    // row: greater
    D_CCM_REL(gt, order_relation_meet(gt, eq));
    D_CCM_REL(ic, order_relation_meet(gt, ls));     // *** opposing directions
    D_CCM_REL(gt, order_relation_meet(gt, gt));
    D_CCM_REL(ic, order_relation_meet(gt, ic));

    // row: incomparable (absorbing)
    D_CCM_REL(ic, order_relation_meet(ic, eq));
    D_CCM_REL(ic, order_relation_meet(ic, ls));
    D_CCM_REL(ic, order_relation_meet(ic, gt));
    D_CCM_REL(ic, order_relation_meet(ic, ic));

    // it is constexpr and noexcept
    D_CONSTEXPR order_relation folded =
        order_relation_meet(order_relation_meet(gt, eq), gt);

    D_CCM_REL(gt, folded);
    D_CCM_CHECK(noexcept(order_relation_meet(eq, eq)));

    return ok;
}

/*
tests_order_relation_meet_laws
  The meet is not merely a table - it is an algebraic operation, and the
  refinement folds four axes through it in a fixed left-nested order:

      meet( meet( meet(discipline, lifetime), iterability ), storage )

  If the meet were not ASSOCIATIVE and COMMUTATIVE, that order would matter, and
  the answer would depend on which axis the author happened to write first.  It
  does not - but only because the meet has the laws, and the laws are what this
  test pins.

  Tests the following:
  - COMMUTATIVE: meet(a, b) == meet(b, a), over all sixteen pairs
  - ASSOCIATIVE: meet(meet(a, b), c) == meet(a, meet(b, c)), over all 64 triples
  - IDENTITY: equivalent is the unit - meet(equivalent, a) == a
  - ABSORBING: incomparable swallows everything - meet(incomparable, a) ==
    incomparable
  - IDEMPOTENT: meet(a, a) == a

  Together these make (order_relation, meet) a commutative monoid with a zero,
  which is exactly what a product-order fold requires and exactly what nobody
  checks.
*/
bool
tests_order_relation_meet_laws()
{
    bool ok = true;

    D_CONSTEXPR order_relation all[4] = { order_relation::equivalent,
                                          order_relation::less,
                                          order_relation::greater,
                                          order_relation::incomparable };

    for (int i = 0; i < 4; ++i)
    {
        const order_relation a = all[i];

        // idempotent
        D_CCM_CHECK(order_relation_meet(a, a) == a);

        // identity
        D_CCM_CHECK(order_relation_meet(order_relation::equivalent, a) == a);
        D_CCM_CHECK(order_relation_meet(a, order_relation::equivalent) == a);

        // absorbing
        D_CCM_CHECK(order_relation_meet(order_relation::incomparable, a) ==
                    order_relation::incomparable);
        D_CCM_CHECK(order_relation_meet(a, order_relation::incomparable) ==
                    order_relation::incomparable);

        for (int j = 0; j < 4; ++j)
        {
            const order_relation b = all[j];

            // commutative
            D_CCM_CHECK(order_relation_meet(a, b) ==
                        order_relation_meet(b, a));

            for (int k = 0; k < 4; ++k)
            {
                const order_relation c = all[k];

                // associative - all 64 triples
                D_CCM_CHECK(order_relation_meet(order_relation_meet(a, b), c) ==
                            order_relation_meet(a, order_relation_meet(b, c)));
            }
        }
    }

    // ...so the fold order in comparison_profile::refinement is immaterial, which
    // is worth stating outright, because the header fixes one and a reader has no
    // way to know it did not have to
    D_CONSTEXPR order_relation as_written =
        order_relation_meet(
            order_relation_meet(
                order_relation_meet(order_relation::greater,
                                    order_relation::equivalent),
                order_relation::greater),
            order_relation::equivalent);

    D_CONSTEXPR order_relation reordered =
        order_relation_meet(
            order_relation::equivalent,
            order_relation_meet(
                order_relation::greater,
                order_relation_meet(order_relation::equivalent,
                                    order_relation::greater)));

    D_CCM_REL(order_relation::greater, as_written);
    D_CCM_CHECK(as_written == reordered);

    return ok;
}


NS_END  // testing
NS_END  // djinterp
