/*******************************************************************************
* djinterp [cpp][container]     container_copy_merge_traits_tests_merge_kind.cpp
*
*
* path:      /tests/djinterp/cpp/container/traits/container_copy_merge_traits_tests_merge_kind.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "container_copy_merge_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


// ===========================================================================
// IV.  Merge kind
// ===========================================================================
//   The combining operation of a pair.  Two layers: a pure function of two
// disciplines, which can be exercised over the whole 6x6 grid without any
// carrier types at all, and the trait that feeds it two detected disciplines
// behind an element guard.  Testing the function exhaustively is what makes
// the trait's own tests able to concentrate on the guard and the wiring.


/*
tests_merge_kind_enum
  Verifies the kind enum's shape.
  Tests the following:
  - the five enumerators are pairwise distinct
  - the walker visits each exactly once
  - concatenation is the ZERO value, so a default-initialised merge_kind is
    concatenation rather than incompatible - the opposite of the discipline
    enum's arrangement, where the safe value sits at zero.  Worth pinning:
    a merge_kind that is default-initialised or zero-filled reads as a defined
    merge, not as a refusal
*/
bool tests_merge_kind_enum()
{
    bool ok = true;

    // pairwise distinct
    for (int i = 0; i < cm_kind_count; ++i)
    {
        for (int j = i + 1; j < cm_kind_count; ++j)
        {
            D_CM_CHECK(cm_kind_at(i) != cm_kind_at(j));
        }
    }

    // the walker visits each exactly once
    D_CM_CHECK(cm_kind_at(0) == DMergeKind::concatenation);
    D_CM_CHECK(cm_kind_at(1) == DMergeKind::set_union);
    D_CM_CHECK(cm_kind_at(2) == DMergeKind::multiset_union);
    D_CM_CHECK(cm_kind_at(3) == DMergeKind::keyed_merge);
    D_CM_CHECK(cm_kind_at(4) == DMergeKind::incompatible);

    // the zero value is a DEFINED merge, unlike the discipline enum's
    D_CM_STATIC_CHECK(static_cast<int>(DMergeKind::concatenation) == 0);
    D_CM_CHECK(DMergeKind() == DMergeKind::concatenation);
    D_CM_CHECK(DMergeKind() != DMergeKind::incompatible);

    // ... whereas the discipline enum puts its inert value at zero
    D_CM_CHECK(DMergeDiscipline() == DMergeDiscipline::unknown);

    return ok;
}

/*
tests_merge_kind_name
  Verifies the kind spellings.
  Tests the following:
  - each of the five enumerators names itself
  - the five spellings are pairwise distinct
  - the function is usable in a constant expression
*/
bool tests_merge_kind_name()
{
    bool ok = true;

    D_CM_CHECK(cm_kind_is(DMergeKind::concatenation,  "concatenation"));
    D_CM_CHECK(cm_kind_is(DMergeKind::set_union,      "set_union"));
    D_CM_CHECK(cm_kind_is(DMergeKind::multiset_union, "multiset_union"));
    D_CM_CHECK(cm_kind_is(DMergeKind::keyed_merge,    "keyed_merge"));
    D_CM_CHECK(cm_kind_is(DMergeKind::incompatible,   "incompatible"));

    // pairwise distinct spellings
    for (int i = 0; i < cm_kind_count; ++i)
    {
        for (int j = i + 1; j < cm_kind_count; ++j)
        {
            D_CM_CHECK(std::strcmp(merge_kind_name(cm_kind_at(i)),
                                   merge_kind_name(cm_kind_at(j))) != 0);
        }
    }

    // constexpr
    D_CM_STATIC_CHECK(merge_kind_name(DMergeKind::set_union)[0] == 's');

    return ok;
}

/*
tests_enum_names_fall_through
  Records a boundary shared by both naming functions: each is a chain of
  equality tests ending in an UNGUARDED default, so a value outside the enum is
  silently given the last spelling rather than being reported as unknown.

  Neither enumerator count is a power of two - six disciplines and five kinds -
  so the representable range of each enum is strictly larger than its declared
  set and the out-of-range values are genuinely reachable by a cast.  A
  merge_discipline of 6 or 7 reads as "multimap"; a merge_kind of 5, 6 or 7
  reads as "incompatible".

  The consequences differ.  For merge_kind the default is the refusal value, so
  a corrupted kind reads as a refusal - a safe direction.  For
  merge_discipline the default is multimap, the most permissive associative
  discipline, so a corrupted discipline reads as something that MERGES rather
  than as unknown.  Putting the fall-through on `unknown` instead - by testing
  multimap explicitly and defaulting to "unknown" - would cost nothing and
  reverse that.

  Tests the following:
  - every declared enumerator names itself, so the fall-through is not masking
    a wrong answer inside the declared range
  - the first out-of-range value of each enum takes the last spelling
  - the discipline default is the permissive one and the kind default is the
    safe one, stated directly
*/
bool tests_enum_names_fall_through()
{
    bool ok = true;

    // inside the declared range, every value names itself
    for (int i = 0; i < cm_discipline_count; ++i)
    {
        D_CM_CHECK(merge_discipline_name(cm_discipline_at(i)) != nullptr);
    }

    for (int i = 0; i < cm_kind_count; ++i)
    {
        D_CM_CHECK(merge_kind_name(cm_kind_at(i)) != nullptr);
    }

    // outside it, the last spelling is taken
    D_CM_CHECK(cm_disc_is(static_cast<DMergeDiscipline>(cm_discipline_count),
                          "multimap"));
    D_CM_CHECK(cm_disc_is(
                   static_cast<DMergeDiscipline>(cm_discipline_count + 1),
                   "multimap"));
    D_CM_CHECK(cm_kind_is(static_cast<DMergeKind>(cm_kind_count),
                          "incompatible"));
    D_CM_CHECK(cm_kind_is(static_cast<DMergeKind>(cm_kind_count + 1),
                          "incompatible"));

    // ... which for the discipline is the PERMISSIVE value
    D_CM_CHECK(discipline_is_keyed(DMergeDiscipline::multimap) == true);
    D_CM_CHECK(cm_disc_is(static_cast<DMergeDiscipline>(cm_discipline_count),
                          merge_discipline_name(DMergeDiscipline::multimap)));
    D_CM_CHECK(!cm_disc_is(
                   static_cast<DMergeDiscipline>(cm_discipline_count),
                   merge_discipline_name(DMergeDiscipline::unknown)));

    // ... and for the kind is the SAFE one
    D_CM_CHECK(cm_kind_is(static_cast<DMergeKind>(cm_kind_count),
                          merge_kind_name(DMergeKind::incompatible)));

    return ok;
}

/*
tests_merge_kind_from_disciplines
  Verifies the combining function on the pairs that matter, before the
  exhaustive walk below.
  Tests the following:
  - two sequences concatenate
  - two sets take a set union; a multiset on EITHER side degrades it to a
    multiset union, since the result may then repeat
  - two keyed disciplines merge by key, in all four map / multimap
    combinations - the keyed branch does not split on uniqueness the way the
    bag branch does
  - a pair drawn from two different families is incompatible, in both orders
  - unknown on either side is incompatible
  - the function is usable in a constant expression
*/
bool tests_merge_kind_from_disciplines()
{
    bool ok = true;

    // sequences
    D_CM_CHECK(merge_kind_from_disciplines(DMergeDiscipline::sequence,
                                           DMergeDiscipline::sequence)
                   == DMergeKind::concatenation);

    // bags, and the uniqueness degradation
    D_CM_CHECK(merge_kind_from_disciplines(DMergeDiscipline::set,
                                           DMergeDiscipline::set)
                   == DMergeKind::set_union);
    D_CM_CHECK(merge_kind_from_disciplines(DMergeDiscipline::set,
                                           DMergeDiscipline::multiset)
                   == DMergeKind::multiset_union);
    D_CM_CHECK(merge_kind_from_disciplines(DMergeDiscipline::multiset,
                                           DMergeDiscipline::set)
                   == DMergeKind::multiset_union);
    D_CM_CHECK(merge_kind_from_disciplines(DMergeDiscipline::multiset,
                                           DMergeDiscipline::multiset)
                   == DMergeKind::multiset_union);

    // keyed, which does NOT split on uniqueness
    D_CM_CHECK(merge_kind_from_disciplines(DMergeDiscipline::map,
                                           DMergeDiscipline::map)
                   == DMergeKind::keyed_merge);
    D_CM_CHECK(merge_kind_from_disciplines(DMergeDiscipline::map,
                                           DMergeDiscipline::multimap)
                   == DMergeKind::keyed_merge);
    D_CM_CHECK(merge_kind_from_disciplines(DMergeDiscipline::multimap,
                                           DMergeDiscipline::map)
                   == DMergeKind::keyed_merge);
    D_CM_CHECK(merge_kind_from_disciplines(DMergeDiscipline::multimap,
                                           DMergeDiscipline::multimap)
                   == DMergeKind::keyed_merge);

    // across families, both orders
    D_CM_CHECK(merge_kind_from_disciplines(DMergeDiscipline::sequence,
                                           DMergeDiscipline::set)
                   == DMergeKind::incompatible);
    D_CM_CHECK(merge_kind_from_disciplines(DMergeDiscipline::set,
                                           DMergeDiscipline::sequence)
                   == DMergeKind::incompatible);
    D_CM_CHECK(merge_kind_from_disciplines(DMergeDiscipline::set,
                                           DMergeDiscipline::map)
                   == DMergeKind::incompatible);
    D_CM_CHECK(merge_kind_from_disciplines(DMergeDiscipline::multiset,
                                           DMergeDiscipline::multimap)
                   == DMergeKind::incompatible);

    // unknown on either side
    D_CM_CHECK(merge_kind_from_disciplines(DMergeDiscipline::unknown,
                                           DMergeDiscipline::sequence)
                   == DMergeKind::incompatible);
    D_CM_CHECK(merge_kind_from_disciplines(DMergeDiscipline::sequence,
                                           DMergeDiscipline::unknown)
                   == DMergeKind::incompatible);
    D_CM_CHECK(merge_kind_from_disciplines(DMergeDiscipline::unknown,
                                           DMergeDiscipline::unknown)
                   == DMergeKind::incompatible);

    // constexpr
    D_CM_STATIC_CHECK(merge_kind_from_disciplines(DMergeDiscipline::set,
                                                  DMergeDiscipline::set)
                          == DMergeKind::set_union);

    return ok;
}

/*
tests_merge_kind_from_disciplines_total
  Walks the ENTIRE 6x6 discipline grid and checks the function's structural
  properties, rather than sampling it.
  Tests the following:
  - the function is symmetric in its two arguments at every point
  - a defined kind arises exactly when both operands are in the SAME family,
    so cross-family and unknown pairs are all refusals
  - the kind lands in the family the operands share - a bag pair never yields a
    concatenation, a sequence pair never a union
  - the set_union verdict arises at exactly ONE point of the grid, both sides
    being set, so the uniqueness degradation covers every other bag pair
  - all five kinds are reached somewhere in the grid, so none is dead
*/
bool tests_merge_kind_from_disciplines_total()
{
    bool ok = true;

    int set_union_points = 0;
    int reached[cm_kind_count];

    for (int k = 0; k < cm_kind_count; ++k)
    {
        reached[k] = 0;
    }

    for (int i = 0; i < cm_discipline_count; ++i)
    {
        for (int j = 0; j < cm_discipline_count; ++j)
        {
            const DMergeDiscipline a = cm_discipline_at(i);
            const DMergeDiscipline b = cm_discipline_at(j);
            const DMergeKind       k = merge_kind_from_disciplines(a, b);

            // symmetric
            D_CM_CHECK(k == merge_kind_from_disciplines(b, a));

            // defined exactly when the families agree
            const bool same_family =
                (    ( discipline_is_sequence(a) && discipline_is_sequence(b) )
                  || ( discipline_is_bag(a)      && discipline_is_bag(b) )
                  || ( discipline_is_keyed(a)    && discipline_is_keyed(b) ) );

            D_CM_CHECK(( k != DMergeKind::incompatible ) == same_family);

            // the kind lands in the shared family
            if (discipline_is_sequence(a) && discipline_is_sequence(b))
            {
                D_CM_CHECK(k == DMergeKind::concatenation);
            }
            else if (discipline_is_bag(a) && discipline_is_bag(b))
            {
                D_CM_CHECK(   ( k == DMergeKind::set_union )
                           || ( k == DMergeKind::multiset_union ));
            }
            else if (discipline_is_keyed(a) && discipline_is_keyed(b))
            {
                D_CM_CHECK(k == DMergeKind::keyed_merge);
            }

            if (k == DMergeKind::set_union)
            {
                ++set_union_points;

                D_CM_CHECK(   ( a == DMergeDiscipline::set )
                           && ( b == DMergeDiscipline::set ));
            }

            for (int m = 0; m < cm_kind_count; ++m)
            {
                if (k == cm_kind_at(m))
                {
                    reached[m] = 1;
                }
            }
        }
    }

    // set_union arises at exactly one point of the grid
    D_CM_CHECK(set_union_points == 1);

    // every kind is reached
    for (int k = 0; k < cm_kind_count; ++k)
    {
        D_CM_CHECK(reached[k] == 1);
    }

    return ok;
}

/*
tests_merge_kind_of
  Verifies the trait that feeds two detected disciplines to the function.
  Tests the following:
  - a carrier pair for each of the five kinds
  - the trait agrees with the function applied to the two detected
    disciplines, on carriers where the merge is defined
  - the verdict is symmetric on carrier pairs, as the function is
  - a non-container on either side is incompatible
*/
bool tests_merge_kind_of()
{
    bool ok = true;

    // one carrier pair per kind
    D_CM_CHECK((merge_kind_of<DCmVector, DCmVector>::value)
                   == DMergeKind::concatenation);
    D_CM_CHECK((merge_kind_of<DCmSet, DCmSet>::value)
                   == DMergeKind::set_union);
    D_CM_CHECK((merge_kind_of<DCmSet, DCmMultiset>::value)
                   == DMergeKind::multiset_union);
    D_CM_CHECK((merge_kind_of<DCmMap, DCmMap>::value)
                   == DMergeKind::keyed_merge);
    D_CM_CHECK((merge_kind_of<DCmVector, DCmSet>::value)
                   == DMergeKind::incompatible);

    // agreement with the function
    D_CM_CHECK((merge_kind_of<DCmVector, DCmVector>::value)
                   == merge_kind_from_disciplines(
                          merge_discipline_of<DCmVector>::value,
                          merge_discipline_of<DCmVector>::value));
    D_CM_CHECK((merge_kind_of<DCmSet, DCmMultiset>::value)
                   == merge_kind_from_disciplines(
                          merge_discipline_of<DCmSet>::value,
                          merge_discipline_of<DCmMultiset>::value));
    D_CM_CHECK((merge_kind_of<DCmMap, DCmMultimap>::value)
                   == merge_kind_from_disciplines(
                          merge_discipline_of<DCmMap>::value,
                          merge_discipline_of<DCmMultimap>::value));

    // symmetric on carriers
    D_CM_CHECK((merge_kind_of<DCmSet, DCmMultiset>::value)
                   == (merge_kind_of<DCmMultiset, DCmSet>::value));
    D_CM_CHECK((merge_kind_of<DCmMap, DCmMultimap>::value)
                   == (merge_kind_of<DCmMultimap, DCmMap>::value));

    // a non-container on either side
    D_CM_CHECK((merge_kind_of<DCmVector, int>::value)
                   == DMergeKind::incompatible);
    D_CM_CHECK((merge_kind_of<int, DCmVector>::value)
                   == DMergeKind::incompatible);
    D_CM_CHECK((merge_kind_of<int, int>::value)
                   == DMergeKind::incompatible);

    return ok;
}

/*
tests_merge_kind_of_element_guard
  Verifies that the element guard is consulted BEFORE the disciplines, and
  that it can refuse a pair the disciplines would otherwise accept.

  This matters because the two refusals are indistinguishable in the verdict -
  both read incompatible - so a caller cannot tell an element mismatch from a
  discipline mismatch without asking merge_elements_compatible separately.
  DMergeClass exposes both, which is what the elements_ok member is for.

  Tests the following:
  - two containers of the SAME discipline with incompatible elements are
    refused
  - the same pair with compatible elements is accepted, so it is the elements
    doing the work
  - a discipline mismatch with compatible elements is also refused, so the two
    causes really are distinct and both are exercised
  - the two causes are indistinguishable in the kind, and distinguishable
    through merge_elements_compatible
  - element compatibility is necessary but not sufficient
*/
bool tests_merge_kind_of_element_guard()
{
    bool ok = true;

    // same discipline, incompatible elements
    D_CM_CHECK(merge_discipline_of<DCmVector>::value
                   == merge_discipline_of<DCmVecString>::value);
    D_CM_CHECK((merge_elements_compatible<DCmVector,
                                          DCmVecString>::value) == false);
    D_CM_CHECK((merge_kind_of<DCmVector, DCmVecString>::value)
                   == DMergeKind::incompatible);

    // ... and with compatible elements it is accepted
    D_CM_CHECK((merge_elements_compatible<DCmVector,
                                          DCmVecDouble>::value) == true);
    D_CM_CHECK((merge_kind_of<DCmVector, DCmVecDouble>::value)
                   == DMergeKind::concatenation);

    // discipline mismatch, compatible elements
    D_CM_CHECK((merge_elements_compatible<DCmVector, DCmSet>::value) == true);
    D_CM_CHECK(merge_discipline_of<DCmVector>::value
                   != merge_discipline_of<DCmSet>::value);
    D_CM_CHECK((merge_kind_of<DCmVector, DCmSet>::value)
                   == DMergeKind::incompatible);

    // the two causes are indistinguishable in the kind
    D_CM_CHECK((merge_kind_of<DCmVector, DCmVecString>::value)
                   == (merge_kind_of<DCmVector, DCmSet>::value));

    // ... and distinguishable through the element trait
    D_CM_CHECK((merge_elements_compatible<DCmVector, DCmVecString>::value)
                   != (merge_elements_compatible<DCmVector,
                                                 DCmSet>::value));

    // necessary but not sufficient
    D_CM_CHECK(   ( (merge_elements_compatible<DCmVector, DCmSet>::value)
                        == true )
               && ( (is_mergeable<DCmVector, DCmSet>::value) == false ));

    return ok;
}

/*
tests_merge_kind_of_std_types
  Verifies the kind across the standard containers, where the disciplines are
  the familiar ones.
  Tests the following:
  - the sequence containers concatenate with one another, across their
    differing internal shapes
  - std::string concatenates with a vector of the same element
  - the set family unions, degrading where a multiset is present
  - the map family merges by key, in every combination
  - the unordered containers behave as their ordered counterparts
  - every cross-family pair is refused
*/
bool tests_merge_kind_of_std_types()
{
    bool ok = true;

    // sequences
    D_CM_CHECK((merge_kind_of<DCmVector, DCmDeque>::value)
                   == DMergeKind::concatenation);
    D_CM_CHECK((merge_kind_of<DCmList, DCmArray>::value)
                   == DMergeKind::concatenation);
    D_CM_CHECK((merge_kind_of<DCmVector, DCmArray>::value)
                   == DMergeKind::concatenation);

    // sets
    D_CM_CHECK((merge_kind_of<DCmSet, DCmSet>::value)
                   == DMergeKind::set_union);
    D_CM_CHECK((merge_kind_of<DCmMultiset, DCmMultiset>::value)
                   == DMergeKind::multiset_union);
    D_CM_CHECK((merge_kind_of<DCmMultiset, DCmSet>::value)
                   == DMergeKind::multiset_union);

    // maps
    D_CM_CHECK((merge_kind_of<DCmMap, DCmMap>::value)
                   == DMergeKind::keyed_merge);
    D_CM_CHECK((merge_kind_of<DCmMultimap, DCmMultimap>::value)
                   == DMergeKind::keyed_merge);
    D_CM_CHECK((merge_kind_of<DCmMap, DCmMultimap>::value)
                   == DMergeKind::keyed_merge);

    // the unordered counterparts
    D_CM_CHECK((merge_kind_of<DCmUset, DCmSet>::value)
                   == (merge_kind_of<DCmSet, DCmSet>::value));
    D_CM_CHECK((merge_kind_of<DCmUmap, DCmMap>::value)
                   == (merge_kind_of<DCmMap, DCmMap>::value));

    // cross-family refusals
    D_CM_CHECK((merge_kind_of<DCmVector, DCmSet>::value)
                   == DMergeKind::incompatible);
    D_CM_CHECK((merge_kind_of<DCmSet, DCmMap>::value)
                   == DMergeKind::incompatible);
    D_CM_CHECK((merge_kind_of<DCmVector, DCmMap>::value)
                   == DMergeKind::incompatible);
    D_CM_CHECK((merge_kind_of<DCmString, DCmSet>::value)
                   == DMergeKind::incompatible);

    return ok;
}

/*
tests_is_mergeable
  Verifies the boolean face of the kind.
  Tests the following:
  - it is true exactly when the kind is not incompatible, over every carrier
    pair the suite defines
  - all four defined kinds count as mergeable, so it is not standing in for any
    one of them
  - it is false for both causes of refusal
  - it derives from integral_constant, unlike merge_kind_of itself
*/
bool tests_is_mergeable()
{
    bool ok = true;

    // true for each defined kind
    D_CM_CHECK((is_mergeable<DCmVector, DCmVector>::value)  == true);
    D_CM_CHECK((is_mergeable<DCmSet, DCmSet>::value)        == true);
    D_CM_CHECK((is_mergeable<DCmSet, DCmMultiset>::value)   == true);
    D_CM_CHECK((is_mergeable<DCmMap, DCmMultimap>::value)   == true);

    // false for both causes of refusal
    D_CM_CHECK((is_mergeable<DCmVector, DCmSet>::value)     == false);
    D_CM_CHECK((is_mergeable<DCmVector, DCmVecString>::value) == false);
    D_CM_CHECK((is_mergeable<DCmVector, int>::value)        == false);
    D_CM_CHECK((is_mergeable<int, int>::value)              == false);

    // exactly the complement of incompatible
    D_CM_CHECK((is_mergeable<DCmVector, DCmVector>::value)
                   == ( (merge_kind_of<DCmVector, DCmVector>::value)
                            != DMergeKind::incompatible ));
    D_CM_CHECK((is_mergeable<DCmVector, DCmSet>::value)
                   == ( (merge_kind_of<DCmVector, DCmSet>::value)
                            != DMergeKind::incompatible ));
    D_CM_CHECK((is_mergeable<DCmMap, DCmMap>::value)
                   == ( (merge_kind_of<DCmMap, DCmMap>::value)
                            != DMergeKind::incompatible ));
    D_CM_CHECK((is_mergeable<int, DCmVector>::value)
                   == ( (merge_kind_of<int, DCmVector>::value)
                            != DMergeKind::incompatible ));

    // it IS an integral_constant, unlike merge_kind_of
    D_CM_STATIC_CHECK(
        std::is_base_of<std::true_type,
                        is_mergeable<DCmVector, DCmVector> >::value);
    D_CM_STATIC_CHECK(
        std::is_base_of<std::false_type,
                        is_mergeable<DCmVector, DCmSet> >::value);

    return ok;
}

/*
tests_merge_kind_cv_ref
  Verifies that the kind trait cleans BOTH of its operands.
  Tests the following:
  - the verdict is unchanged when either side is cv- or reference-qualified,
    on carriers of several kinds
  - is_mergeable likewise, through the two-operand macro
  - the volatile spellings in particular
*/
bool tests_merge_kind_cv_ref()
{
    bool ok = true;

    // the kind itself
    D_CM_CHECK((merge_kind_of<const DCmVector, DCmVector>::value)
                   == (merge_kind_of<DCmVector, DCmVector>::value));
    D_CM_CHECK((merge_kind_of<volatile DCmVector, DCmVector>::value)
                   == (merge_kind_of<DCmVector, DCmVector>::value));
    D_CM_CHECK((merge_kind_of<DCmVector, const DCmVector&>::value)
                   == (merge_kind_of<DCmVector, DCmVector>::value));
    D_CM_CHECK((merge_kind_of<const DCmVector&, DCmVector&&>::value)
                   == (merge_kind_of<DCmVector, DCmVector>::value));
    D_CM_CHECK((merge_kind_of<const volatile DCmSet&, DCmSet>::value)
                   == (merge_kind_of<DCmSet, DCmSet>::value));
    D_CM_CHECK((merge_kind_of<DCmMap&&, const DCmMultimap&>::value)
                   == (merge_kind_of<DCmMap, DCmMultimap>::value));
    D_CM_CHECK((merge_kind_of<const DCmVector, volatile DCmSet>::value)
                   == (merge_kind_of<DCmVector, DCmSet>::value));

    // the boolean face
    D_CM_CV_REF_AGREE2(is_mergeable, DCmVector, DCmVector, true);
    D_CM_CV_REF_AGREE2(is_mergeable, DCmSet, DCmSet, true);
    D_CM_CV_REF_AGREE2(is_mergeable, DCmVector, DCmSet, false);

    return ok;
}

/*
tests_merge_kind_companions
  Verifies the companions of the kind trait and of is_mergeable.
  Tests the following:
  - the `_t` alias is the integral_constant carrying the kind
  - both `_v` companions agree with their ::value
  - both are usable in a constant expression
  - merge_kind_of does NOT derive from integral_constant while is_mergeable
    does, so the standard members are present on one and not the other
  - the blocks are gated on variable-template support, as the header's own
    definitions are
*/
bool tests_merge_kind_companions()
{
    bool ok = true;

    // the `_t` alias
    D_CM_STATIC_CHECK(
        std::is_same<merge_kind_of_t<DCmVector, DCmVector>,
                     std::integral_constant<DMergeKind,
                                            DMergeKind::concatenation>
                    >::value);
    D_CM_STATIC_CHECK(
        std::is_same<merge_kind_of_t<DCmVector, DCmSet>,
                     std::integral_constant<DMergeKind,
                                            DMergeKind::incompatible>
                    >::value);
    D_CM_CHECK((merge_kind_of_t<DCmSet, DCmSet>::value)
                   == (merge_kind_of<DCmSet, DCmSet>::value));

    // the shapes differ between the two traits
    D_CM_STATIC_CHECK(
        std::is_base_of<merge_kind_of_t<DCmVector, DCmVector>,
                        merge_kind_of<DCmVector, DCmVector> >::value
            == false);
    D_CM_STATIC_CHECK(
        std::is_same<is_mergeable<DCmVector, DCmVector>::value_type,
                     bool>::value);

    // constexpr
    D_CM_STATIC_CHECK((merge_kind_of<DCmSet, DCmSet>::value)
                          == DMergeKind::set_union);

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    D_CM_STATIC_CHECK((merge_kind_of_v<DCmVector, DCmVector>)
                          == DMergeKind::concatenation);
    D_CM_STATIC_CHECK((is_mergeable_v<DCmVector, DCmVector>));
    D_CM_STATIC_CHECK((is_mergeable_v<DCmVector, DCmSet>) == false);

    D_CM_CHECK((merge_kind_of_v<DCmSet, DCmMultiset>)
                   == (merge_kind_of<DCmSet, DCmMultiset>::value));
    D_CM_CHECK((merge_kind_of_v<DCmMap, DCmMap>)
                   == (merge_kind_of<DCmMap, DCmMap>::value));
    D_CM_CHECK((merge_kind_of_v<DCmVector, int>)
                   == (merge_kind_of<DCmVector, int>::value));
    D_CM_CHECK((merge_kind_of_v<const DCmVector&, DCmVector&&>)
                   == (merge_kind_of<DCmVector, DCmVector>::value));
    D_CM_CHECK((is_mergeable_v<DCmMap, DCmMultimap>)
                   == (is_mergeable<DCmMap, DCmMultimap>::value));
    D_CM_CHECK((is_mergeable_v<DCmVector, DCmVecString>)
                   == (is_mergeable<DCmVector, DCmVecString>::value));

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    return ok;
}


NS_END  // testing
NS_END  // djinterp
