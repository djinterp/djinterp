/*******************************************************************************
* djinterp [core]        observational_comparison_traits_tests_level_algebra.cpp
*
*
* path:      /tests/djinterp/core/container/traits/observational_comparison_traits_tests_level_algebra.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "observational_comparison_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


// ===========================================================================
// II.  The observational level
// ===========================================================================
//   The enumeration and the four constexpr functions over it - a spelling, a
// rank, the meet, the join and the order.  The laws are checked EXHAUSTIVELY:
// six levels give thirty-six pairs and two hundred and sixteen triples, which
// is small enough to enumerate, so nothing here rests on chosen witnesses.


/*
tests_observational_level_enum
  Verifies the enumeration itself.
  Tests the following:
  - it is a SCOPED enumeration: it does not implicitly convert to int, so a
    level cannot be mistaken for the rank it maps to
  - the six enumerators carry the implicit ordinals 0..5 in refinement order,
    coarsest first, and are pairwise distinct
  - a value-initialised level is none, the coarsest - the safe default, unlike
    the fall-through of the two lookup functions, which is the finest
  - a level is usable as a non-type template argument, which is what lets the
    level traits derive from integral_constant
  - the underlying type is deliberately NOT asserted: this enumeration does not
    fix one, unlike its siblings in the trait family, so it is implementation-
    defined and no part of the contract.  The consequence that IS pinned, in
    tests_level_algebra_out_of_range, is that the well-defined value range is
    only 0..7
*/
bool
tests_observational_level_enum()
{
    bool ok = true;

    // scoped
    D_OC_STATIC_CHECK(std::is_enum<observational_level>::value == true);
    D_OC_STATIC_CHECK(
        std::is_convertible<observational_level, int>::value == false);

    // the implicit ordinals, in refinement order
    D_OC_CHECK(static_cast<int>(observational_level::none) == 0);
    D_OC_CHECK(static_cast<int>(observational_level::size) == 1);
    D_OC_CHECK(static_cast<int>(observational_level::set)  == 2);
    D_OC_CHECK(static_cast<int>(observational_level::bag)  == 3);
    D_OC_CHECK(static_cast<int>(observational_level::seq)  == 4);
    D_OC_CHECK(static_cast<int>(observational_level::str)  == 5);

    // a value-initialised level is the coarsest
    D_OC_CHECK(observational_level{} == observational_level::none);

    // pairwise distinct, checked over the whole enum rather than by hand
    for (int i = 0; i < oct_level_count; ++i)
    {
        for (int j = 0; j < oct_level_count; ++j)
        {
            const bool same = ( oct_level_at(i) == oct_level_at(j) );

            D_OC_CHECK(same == (i == j));
        }
    }

    // usable as a non-type template argument
    D_OC_STATIC_CHECK(
        std::integral_constant<observational_level,
                               observational_level::bag>::value
            == observational_level::bag);

    return ok;
}

/*
tests_observational_level_name
  Verifies the diagnostic spelling of each level.
  Tests the following:
  - each of the six levels spells its own name
  - the spellings are pairwise distinct, so a summary cannot conflate two
    levels
  - the function is constexpr and usable in a constant expression
*/
bool
tests_observational_level_name()
{
    bool ok = true;

    // each level's spelling
    D_OC_CHECK(oct_name_is(observational_level::none, "none"));
    D_OC_CHECK(oct_name_is(observational_level::size, "size"));
    D_OC_CHECK(oct_name_is(observational_level::set,  "set"));
    D_OC_CHECK(oct_name_is(observational_level::bag,  "bag"));
    D_OC_CHECK(oct_name_is(observational_level::seq,  "seq"));
    D_OC_CHECK(oct_name_is(observational_level::str,  "str"));

    // pairwise distinct spellings
    for (int i = 0; i < oct_level_count; ++i)
    {
        for (int j = 0; j < oct_level_count; ++j)
        {
            const bool same =
                ( std::strcmp(observational_level_name(oct_level_at(i)),
                              observational_level_name(oct_level_at(j)))
                      == 0 );

            D_OC_CHECK(same == (i == j));
        }
    }

    // constexpr
    D_OC_STATIC_CHECK(
        observational_level_name(observational_level::seq)[0] == 's');

    return ok;
}

/*
tests_observational_level_rank
  Verifies the refinement rank - the linear order the level preorder collapses
  to.
  Tests the following:
  - each level's rank, and that the ranks are strictly increasing in refinement
    order, so the order is total and has no ties
  - the rank COINCIDES with the enumerator's implicit ordinal.  That is not a
    guarantee the header states, it is a consequence of the enumerators being
    unnumbered; the test pins it because the two would silently disagree if an
    enumerator were ever inserted in the middle without the ternary chain being
    updated in lockstep
  - the function is constexpr
*/
bool
tests_observational_level_rank()
{
    bool ok = true;

    // each level's rank
    D_OC_CHECK(observational_level_rank(observational_level::none) == 0);
    D_OC_CHECK(observational_level_rank(observational_level::size) == 1);
    D_OC_CHECK(observational_level_rank(observational_level::set)  == 2);
    D_OC_CHECK(observational_level_rank(observational_level::bag)  == 3);
    D_OC_CHECK(observational_level_rank(observational_level::seq)  == 4);
    D_OC_CHECK(observational_level_rank(observational_level::str)  == 5);

    // strictly increasing, so no two levels tie
    for (int i = 1; i < oct_level_count; ++i)
    {
        D_OC_CHECK(observational_level_rank(oct_level_at(i))
                       > observational_level_rank(oct_level_at(i - 1)));
    }

    // the rank tracks the implicit ordinal - a lockstep the header does not
    // state but does depend on
    for (int i = 0; i < oct_level_count; ++i)
    {
        D_OC_CHECK(observational_level_rank(oct_level_at(i))
                       == static_cast<int>(oct_level_at(i)));
        D_OC_CHECK(observational_level_rank(oct_level_at(i)) == i);
    }

    // constexpr
    D_OC_STATIC_CHECK(
        observational_level_rank(observational_level::str) == 5);

    return ok;
}

/*
tests_observational_level_coarser
  Verifies the meet of the refinement order - the level a shared interface
  resolves.
  Tests the following:
  - it selects the lower-ranked of the two, in both argument orders
  - none annihilates: pairing anything with none gives none, because a
    counterpart that observes nothing bounds the comparison to identity
  - str is the identity: pairing anything with the finest level leaves it
    unchanged
  - the result is always one of the two arguments, never a third level
  - it is constexpr
*/
bool
tests_observational_level_coarser()
{
    bool ok = true;

    // the lower rank wins, in either order
    D_OC_CHECK(observational_level_coarser(observational_level::str,
                                           observational_level::seq)
                   == observational_level::seq);
    D_OC_CHECK(observational_level_coarser(observational_level::seq,
                                           observational_level::str)
                   == observational_level::seq);
    D_OC_CHECK(observational_level_coarser(observational_level::bag,
                                           observational_level::set)
                   == observational_level::set);
    D_OC_CHECK(observational_level_coarser(observational_level::size,
                                           observational_level::str)
                   == observational_level::size);

    // none annihilates, str is the identity, over the whole enum
    for (int i = 0; i < oct_level_count; ++i)
    {
        const observational_level l = oct_level_at(i);

        D_OC_CHECK(observational_level_coarser(l, observational_level::none)
                       == observational_level::none);
        D_OC_CHECK(observational_level_coarser(observational_level::none, l)
                       == observational_level::none);
        D_OC_CHECK(observational_level_coarser(l, observational_level::str)
                       == l);
        D_OC_CHECK(observational_level_coarser(observational_level::str, l)
                       == l);
    }

    // the result is one of the arguments, and is <= both by rank
    for (int i = 0; i < oct_level_count; ++i)
    {
        for (int j = 0; j < oct_level_count; ++j)
        {
            const observational_level a = oct_level_at(i);
            const observational_level b = oct_level_at(j);
            const observational_level m = observational_level_coarser(a, b);

            D_OC_CHECK((m == a) || (m == b));
            D_OC_CHECK(observational_level_rank(m)
                           <= observational_level_rank(a));
            D_OC_CHECK(observational_level_rank(m)
                           <= observational_level_rank(b));
        }
    }

    // constexpr
    D_OC_STATIC_CHECK(
        observational_level_coarser(observational_level::str,
                                    observational_level::set)
            == observational_level::set);

    return ok;
}

/*
tests_observational_level_finer
  Verifies the join of the refinement order.
  Tests the following:
  - it selects the higher-ranked of the two, in both argument orders
  - none is the identity and str annihilates - the duals of the meet's
  - the result is always one of the two arguments, and is >= both by rank
  - it is constexpr
*/
bool
tests_observational_level_finer()
{
    bool ok = true;

    // the higher rank wins, in either order
    D_OC_CHECK(observational_level_finer(observational_level::str,
                                         observational_level::seq)
                   == observational_level::str);
    D_OC_CHECK(observational_level_finer(observational_level::seq,
                                         observational_level::str)
                   == observational_level::str);
    D_OC_CHECK(observational_level_finer(observational_level::none,
                                         observational_level::size)
                   == observational_level::size);

    // none is the identity, str annihilates
    for (int i = 0; i < oct_level_count; ++i)
    {
        const observational_level l = oct_level_at(i);

        D_OC_CHECK(observational_level_finer(l, observational_level::none)
                       == l);
        D_OC_CHECK(observational_level_finer(observational_level::none, l)
                       == l);
        D_OC_CHECK(observational_level_finer(l, observational_level::str)
                       == observational_level::str);
        D_OC_CHECK(observational_level_finer(observational_level::str, l)
                       == observational_level::str);
    }

    // the result is one of the arguments, and is >= both by rank
    for (int i = 0; i < oct_level_count; ++i)
    {
        for (int j = 0; j < oct_level_count; ++j)
        {
            const observational_level a = oct_level_at(i);
            const observational_level b = oct_level_at(j);
            const observational_level j2 = observational_level_finer(a, b);

            D_OC_CHECK((j2 == a) || (j2 == b));
            D_OC_CHECK(observational_level_rank(j2)
                           >= observational_level_rank(a));
            D_OC_CHECK(observational_level_rank(j2)
                           >= observational_level_rank(b));
        }
    }

    // constexpr
    D_OC_STATIC_CHECK(
        observational_level_finer(observational_level::set,
                                  observational_level::bag)
            == observational_level::bag);

    return ok;
}

/*
tests_observational_level_subsumes
  Verifies the order predicate - "_a resolves at least as finely as _b".
  Tests the following:
  - the ordering on named pairs, in both directions
  - it is REFLEXIVE, so a level subsumes itself: subsumption is "at least as
    fine", not "strictly finer"
  - it is antisymmetric and transitive, checked over every pair and triple
  - it is TOTAL: for any two levels one subsumes the other, which is what makes
    a single coarser() well-defined
  - it agrees with the meet and the join: a subsumes b exactly when the coarser
    of the two is b and the finer is a
  - it is constexpr
*/
bool
tests_observational_level_subsumes()
{
    bool ok = true;

    // named pairs
    D_OC_CHECK(observational_level_subsumes(observational_level::str,
                                            observational_level::none)
                   == true);
    D_OC_CHECK(observational_level_subsumes(observational_level::none,
                                            observational_level::str)
                   == false);
    D_OC_CHECK(observational_level_subsumes(observational_level::seq,
                                            observational_level::bag)
                   == true);
    D_OC_CHECK(observational_level_subsumes(observational_level::set,
                                            observational_level::bag)
                   == false);

    // reflexive
    for (int i = 0; i < oct_level_count; ++i)
    {
        D_OC_CHECK(observational_level_subsumes(oct_level_at(i),
                                                oct_level_at(i)) == true);
    }

    // total, antisymmetric, and in agreement with the meet and the join
    for (int i = 0; i < oct_level_count; ++i)
    {
        for (int j = 0; j < oct_level_count; ++j)
        {
            const observational_level a = oct_level_at(i);
            const observational_level b = oct_level_at(j);
            const bool ab = observational_level_subsumes(a, b);
            const bool ba = observational_level_subsumes(b, a);

            D_OC_CHECK(ab || ba);
            D_OC_CHECK(!(ab && ba) || (a == b));
            D_OC_CHECK(ab == ( observational_level_coarser(a, b) == b ));
            D_OC_CHECK(ab == ( observational_level_finer(a, b)   == a ));
        }
    }

    // transitive, over every triple
    for (int i = 0; i < oct_level_count; ++i)
    {
        for (int j = 0; j < oct_level_count; ++j)
        {
            for (int k = 0; k < oct_level_count; ++k)
            {
                const observational_level a = oct_level_at(i);
                const observational_level b = oct_level_at(j);
                const observational_level c = oct_level_at(k);

                if (   observational_level_subsumes(a, b)
                    && observational_level_subsumes(b, c) )
                {
                    D_OC_CHECK(observational_level_subsumes(a, c));
                }
            }
        }
    }

    // constexpr
    D_OC_STATIC_CHECK(
        observational_level_subsumes(observational_level::seq,
                                     observational_level::set));

    return ok;
}

/*
tests_level_algebra_lattice_laws
  Verifies that the meet and the join really do form a lattice over the six
  levels, checked on every pair and every triple rather than on witnesses.
  Tests the following:
  - both operations are idempotent, commutative and associative
  - the two absorption laws hold, which is what makes the pair a lattice rather
    than merely two independent binary operations
  - the lattice is DISTRIBUTIVE, as a total order must be
  - meet and join are duals through the order: the meet is the greatest lower
    bound and the join the least upper bound
*/
bool
tests_level_algebra_lattice_laws()
{
    bool ok = true;

    // idempotent
    for (int i = 0; i < oct_level_count; ++i)
    {
        const observational_level a = oct_level_at(i);

        D_OC_CHECK(observational_level_coarser(a, a) == a);
        D_OC_CHECK(observational_level_finer(a, a)   == a);
    }

    // commutative, and absorbing
    for (int i = 0; i < oct_level_count; ++i)
    {
        for (int j = 0; j < oct_level_count; ++j)
        {
            const observational_level a = oct_level_at(i);
            const observational_level b = oct_level_at(j);

            D_OC_CHECK(observational_level_coarser(a, b)
                           == observational_level_coarser(b, a));
            D_OC_CHECK(observational_level_finer(a, b)
                           == observational_level_finer(b, a));

            D_OC_CHECK(observational_level_coarser(
                           a, observational_level_finer(a, b)) == a);
            D_OC_CHECK(observational_level_finer(
                           a, observational_level_coarser(a, b)) == a);
        }
    }

    // associative and distributive, over every triple
    for (int i = 0; i < oct_level_count; ++i)
    {
        for (int j = 0; j < oct_level_count; ++j)
        {
            for (int k = 0; k < oct_level_count; ++k)
            {
                const observational_level a = oct_level_at(i);
                const observational_level b = oct_level_at(j);
                const observational_level c = oct_level_at(k);

                D_OC_CHECK(
                    observational_level_coarser(
                        observational_level_coarser(a, b), c)
                        == observational_level_coarser(
                               a, observational_level_coarser(b, c)));
                D_OC_CHECK(
                    observational_level_finer(
                        observational_level_finer(a, b), c)
                        == observational_level_finer(
                               a, observational_level_finer(b, c)));

                D_OC_CHECK(
                    observational_level_coarser(
                        a, observational_level_finer(b, c))
                        == observational_level_finer(
                               observational_level_coarser(a, b),
                               observational_level_coarser(a, c)));
            }
        }
    }

    // the meet is the greatest lower bound, the join the least upper bound
    for (int i = 0; i < oct_level_count; ++i)
    {
        for (int j = 0; j < oct_level_count; ++j)
        {
            for (int k = 0; k < oct_level_count; ++k)
            {
                const observational_level a = oct_level_at(i);
                const observational_level b = oct_level_at(j);
                const observational_level c = oct_level_at(k);

                if (   observational_level_subsumes(a, c)
                    && observational_level_subsumes(b, c) )
                {
                    D_OC_CHECK(observational_level_subsumes(
                        observational_level_coarser(a, b), c));
                }

                if (   observational_level_subsumes(c, a)
                    && observational_level_subsumes(c, b) )
                {
                    D_OC_CHECK(observational_level_subsumes(
                        c, observational_level_finer(a, b)));
                }
            }
        }
    }

    return ok;
}

/*
tests_level_algebra_out_of_range
  Verifies what the algebra does with a value that names no enumerator, and
  records the two consequences that follow.

  Both observational_level_name and observational_level_rank are chained
  ternaries that test five of the six levels and FALL THROUGH to str / 5.  So
  an unnamed value does not read as none, the coarsest and safest answer - it
  reads as the FINEST, claiming an inspection surface it cannot have.

  The second consequence is subtler and is why this test exists.  Because rank
  collapses str and every unnamed value to 5, and because coarser and finer
  break a tie by returning their FIRST argument, the two operations stop being
  commutative on the full value range: coarser(unnamed, str) yields the
  unnamed value while coarser(str, unnamed) yields str.  An unnamed level
  therefore does not merely READ as str, it can PROPAGATE out of the algebra
  as itself.  For the same reason subsumes reports mutual subsumption between
  two distinct values, so antisymmetry fails there too.

  The lattice laws that tests_level_algebra_lattice_laws verifies exhaustively
  are thus properties of the six NAMED levels - where rank is injective, so
  ties occur only between equal values - and not of the type at large.  That
  is a perfectly reasonable contract, since nothing in the header manufactures
  an unnamed value; the point of pinning it is that the boundary stays a
  deliberate choice rather than an unexamined one.

  Note the range being probed.  This enumeration does not fix an underlying
  type, so its well-defined values are only those a three-bit field can hold -
  0 through 7 - because five is the largest enumerator.  6 and 7 are therefore
  valid values that name nothing, and are the correct witnesses; a larger cast
  would not be well-defined at all.

  Tests the following:
  - the two unnamed values in range both read as str and rank 5
  - they are rank-indistinguishable from str and from each other
  - against any level COARSER than str the meet still answers that level, so an
    unnamed value never bounds a comparison below the finest rung
  - at the tie with str the tie-break decides, so the meet is not commutative
    there and the unnamed value escapes as a value
  - subsumption is mutual between an unnamed value and str, though they differ,
    so antisymmetry fails outside the named set
  - by contrast a value-initialised level is none, so default construction is
    safe even though the fall-through is not
*/
bool
tests_level_algebra_out_of_range()
{
    bool ok = true;

    const observational_level six   = static_cast<observational_level>(6);
    const observational_level seven = static_cast<observational_level>(7);
    const observational_level str   = observational_level::str;

    // the fall-through answers the FINEST level, not the coarsest
    D_OC_CHECK(oct_name_is(six,   "str"));
    D_OC_CHECK(oct_name_is(seven, "str"));
    D_OC_CHECK(observational_level_rank(six)   == 5);
    D_OC_CHECK(observational_level_rank(seven) == 5);

    // ... so the three are indistinguishable by rank, though distinct as values
    D_OC_CHECK(observational_level_rank(six)
                   == observational_level_rank(str));
    D_OC_CHECK(observational_level_rank(six)
                   == observational_level_rank(seven));
    D_OC_CHECK(six != str);
    D_OC_CHECK(six != seven);

    // against every level coarser than str, the meet answers that level, so an
    // unnamed value never bounds a comparison below the finest rung
    for (int i = 0; i < oct_level_count - 1; ++i)
    {
        const observational_level l = oct_level_at(i);

        D_OC_CHECK(observational_level_subsumes(six, l) == true);
        D_OC_CHECK(observational_level_coarser(six, l)  == l);
        D_OC_CHECK(observational_level_coarser(l, six)  == l);
    }

    // at the tie with str the tie-break decides, and commutativity fails
    D_OC_CHECK(observational_level_coarser(six, str) == six);
    D_OC_CHECK(observational_level_coarser(str, six) == str);
    D_OC_CHECK(observational_level_coarser(six, str)
                   != observational_level_coarser(str, six));

    // the join ties the same way, in the other direction
    D_OC_CHECK(observational_level_finer(six, str) == six);
    D_OC_CHECK(observational_level_finer(str, six) == str);

    // and subsumption is mutual between two distinct values
    D_OC_CHECK(observational_level_subsumes(six, str) == true);
    D_OC_CHECK(observational_level_subsumes(str, six) == true);
    D_OC_CHECK(!( six == str ));

    // over the NAMED levels none of that can arise, rank being injective there
    for (int i = 0; i < oct_level_count; ++i)
    {
        for (int j = 0; j < oct_level_count; ++j)
        {
            const observational_level a = oct_level_at(i);
            const observational_level b = oct_level_at(j);

            D_OC_CHECK(   ( observational_level_rank(a)
                                == observational_level_rank(b) )
                       == ( a == b ));
        }
    }

    // the DEFAULT is the coarsest, so value initialisation is safe
    D_OC_CHECK(observational_level{} == observational_level::none);
    D_OC_CHECK(observational_level_rank(observational_level{}) == 0);
    D_OC_CHECK(oct_name_is(observational_level{}, "none"));

    return ok;
}

NS_END  // testing
NS_END  // djinterp
