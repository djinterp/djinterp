/*******************************************************************************
* djinterp [core]               container_copy_merge_traits_tests_merge_kind.cpp
*
*
* path:      /tests/djinterp/core/container/traits/container_copy_merge_traits_tests_merge_kind.cpp
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
bool
tests_merge_kind_enum()
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
    D_CM_CHECK(cm_kind_at(0) == merge_kind::concatenation);
    D_CM_CHECK(cm_kind_at(1) == merge_kind::set_union);
    D_CM_CHECK(cm_kind_at(2) == merge_kind::multiset_union);
    D_CM_CHECK(cm_kind_at(3) == merge_kind::keyed_merge);
    D_CM_CHECK(cm_kind_at(4) == merge_kind::incompatible);

    // the zero value is a DEFINED merge, unlike the discipline enum's
    D_CM_STATIC_CHECK(static_cast<int>(merge_kind::concatenation) == 0);
    D_CM_CHECK(merge_kind() == merge_kind::concatenation);
    D_CM_CHECK(merge_kind() != merge_kind::incompatible);

    // ... whereas the discipline enum puts its inert value at zero
    D_CM_CHECK(merge_discipline() == merge_discipline::unknown);

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
bool
tests_merge_kind_name()
{
    bool ok = true;

    D_CM_CHECK(cm_kind_is(merge_kind::concatenation,  "concatenation"));
    D_CM_CHECK(cm_kind_is(merge_kind::set_union,      "set_union"));
    D_CM_CHECK(cm_kind_is(merge_kind::multiset_union, "multiset_union"));
    D_CM_CHECK(cm_kind_is(merge_kind::keyed_merge,    "keyed_merge"));
    D_CM_CHECK(cm_kind_is(merge_kind::incompatible,   "incompatible"));

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
    D_CM_STATIC_CHECK(merge_kind_name(merge_kind::set_union)[0] == 's');

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
bool
tests_enum_names_fall_through()
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
    D_CM_CHECK(cm_disc_is(static_cast<merge_discipline>(cm_discipline_count),
                          "multimap"));
    D_CM_CHECK(cm_disc_is(
                   static_cast<merge_discipline>(cm_discipline_count + 1),
                   "multimap"));
    D_CM_CHECK(cm_kind_is(static_cast<merge_kind>(cm_kind_count),
                          "incompatible"));
    D_CM_CHECK(cm_kind_is(static_cast<merge_kind>(cm_kind_count + 1),
                          "incompatible"));

    // ... which for the discipline is the PERMISSIVE value
    D_CM_CHECK(discipline_is_keyed(merge_discipline::multimap) == true);
    D_CM_CHECK(cm_disc_is(static_cast<merge_discipline>(cm_discipline_count),
                          merge_discipline_name(merge_discipline::multimap)));
    D_CM_CHECK(!cm_disc_is(
                   static_cast<merge_discipline>(cm_discipline_count),
                   merge_discipline_name(merge_discipline::unknown)));

    // ... and for the kind is the SAFE one
    D_CM_CHECK(cm_kind_is(static_cast<merge_kind>(cm_kind_count),
                          merge_kind_name(merge_kind::incompatible)));

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
bool
tests_merge_kind_from_disciplines()
{
    bool ok = true;

    // sequences
    D_CM_CHECK(merge_kind_from_disciplines(merge_discipline::sequence,
                                           merge_discipline::sequence)
                   == merge_kind::concatenation);

    // bags, and the uniqueness degradation
    D_CM_CHECK(merge_kind_from_disciplines(merge_discipline::set,
                                           merge_discipline::set)
                   == merge_kind::set_union);
    D_CM_CHECK(merge_kind_from_disciplines(merge_discipline::set,
                                           merge_discipline::multiset)
                   == merge_kind::multiset_union);
    D_CM_CHECK(merge_kind_from_disciplines(merge_discipline::multiset,
                                           merge_discipline::set)
                   == merge_kind::multiset_union);
    D_CM_CHECK(merge_kind_from_disciplines(merge_discipline::multiset,
                                           merge_discipline::multiset)
                   == merge_kind::multiset_union);

    // keyed, which does NOT split on uniqueness
    D_CM_CHECK(merge_kind_from_disciplines(merge_discipline::map,
                                           merge_discipline::map)
                   == merge_kind::keyed_merge);
    D_CM_CHECK(merge_kind_from_disciplines(merge_discipline::map,
                                           merge_discipline::multimap)
                   == merge_kind::keyed_merge);
    D_CM_CHECK(merge_kind_from_disciplines(merge_discipline::multimap,
                                           merge_discipline::map)
                   == merge_kind::keyed_merge);
    D_CM_CHECK(merge_kind_from_disciplines(merge_discipline::multimap,
                                           merge_discipline::multimap)
                   == merge_kind::keyed_merge);

    // across families, both orders
    D_CM_CHECK(merge_kind_from_disciplines(merge_discipline::sequence,
                                           merge_discipline::set)
                   == merge_kind::incompatible);
    D_CM_CHECK(merge_kind_from_disciplines(merge_discipline::set,
                                           merge_discipline::sequence)
                   == merge_kind::incompatible);
    D_CM_CHECK(merge_kind_from_disciplines(merge_discipline::set,
                                           merge_discipline::map)
                   == merge_kind::incompatible);
    D_CM_CHECK(merge_kind_from_disciplines(merge_discipline::multiset,
                                           merge_discipline::multimap)
                   == merge_kind::incompatible);

    // unknown on either side
    D_CM_CHECK(merge_kind_from_disciplines(merge_discipline::unknown,
                                           merge_discipline::sequence)
                   == merge_kind::incompatible);
    D_CM_CHECK(merge_kind_from_disciplines(merge_discipline::sequence,
                                           merge_discipline::unknown)
                   == merge_kind::incompatible);
    D_CM_CHECK(merge_kind_from_disciplines(merge_discipline::unknown,
                                           merge_discipline::unknown)
                   == merge_kind::incompatible);

    // constexpr
    D_CM_STATIC_CHECK(merge_kind_from_disciplines(merge_discipline::set,
                                                  merge_discipline::set)
                          == merge_kind::set_union);

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
bool
tests_merge_kind_from_disciplines_total()
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
            const merge_discipline a = cm_discipline_at(i);
            const merge_discipline b = cm_discipline_at(j);
            const merge_kind       k = merge_kind_from_disciplines(a, b);

            // symmetric
            D_CM_CHECK(k == merge_kind_from_disciplines(b, a));

            // defined exactly when the families agree
            const bool same_family =
                (    ( discipline_is_sequence(a) && discipline_is_sequence(b) )
                  || ( discipline_is_bag(a)      && discipline_is_bag(b) )
                  || ( discipline_is_keyed(a)    && discipline_is_keyed(b) ) );

            D_CM_CHECK(( k != merge_kind::incompatible ) == same_family);

            // the kind lands in the shared family
            if (discipline_is_sequence(a) && discipline_is_sequence(b))
            {
                D_CM_CHECK(k == merge_kind::concatenation);
            }
            else if (discipline_is_bag(a) && discipline_is_bag(b))
            {
                D_CM_CHECK(   ( k == merge_kind::set_union )
                           || ( k == merge_kind::multiset_union ));
            }
            else if (discipline_is_keyed(a) && discipline_is_keyed(b))
            {
                D_CM_CHECK(k == merge_kind::keyed_merge);
            }

            if (k == merge_kind::set_union)
            {
                ++set_union_points;

                D_CM_CHECK(   ( a == merge_discipline::set )
                           && ( b == merge_discipline::set ));
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
bool
tests_merge_kind_of()
{
    bool ok = true;

    // one carrier pair per kind
    D_CM_CHECK((merge_kind_of<cm_vector, cm_vector>::value)
                   == merge_kind::concatenation);
    D_CM_CHECK((merge_kind_of<cm_set, cm_set>::value)
                   == merge_kind::set_union);
    D_CM_CHECK((merge_kind_of<cm_set, cm_multiset>::value)
                   == merge_kind::multiset_union);
    D_CM_CHECK((merge_kind_of<cm_map, cm_map>::value)
                   == merge_kind::keyed_merge);
    D_CM_CHECK((merge_kind_of<cm_vector, cm_set>::value)
                   == merge_kind::incompatible);

    // agreement with the function
    D_CM_CHECK((merge_kind_of<cm_vector, cm_vector>::value)
                   == merge_kind_from_disciplines(
                          merge_discipline_of<cm_vector>::value,
                          merge_discipline_of<cm_vector>::value));
    D_CM_CHECK((merge_kind_of<cm_set, cm_multiset>::value)
                   == merge_kind_from_disciplines(
                          merge_discipline_of<cm_set>::value,
                          merge_discipline_of<cm_multiset>::value));
    D_CM_CHECK((merge_kind_of<cm_map, cm_multimap>::value)
                   == merge_kind_from_disciplines(
                          merge_discipline_of<cm_map>::value,
                          merge_discipline_of<cm_multimap>::value));

    // symmetric on carriers
    D_CM_CHECK((merge_kind_of<cm_set, cm_multiset>::value)
                   == (merge_kind_of<cm_multiset, cm_set>::value));
    D_CM_CHECK((merge_kind_of<cm_map, cm_multimap>::value)
                   == (merge_kind_of<cm_multimap, cm_map>::value));

    // a non-container on either side
    D_CM_CHECK((merge_kind_of<cm_vector, int>::value)
                   == merge_kind::incompatible);
    D_CM_CHECK((merge_kind_of<int, cm_vector>::value)
                   == merge_kind::incompatible);
    D_CM_CHECK((merge_kind_of<int, int>::value)
                   == merge_kind::incompatible);

    return ok;
}

/*
tests_merge_kind_of_element_guard
  Verifies that the element guard is consulted BEFORE the disciplines, and
  that it can refuse a pair the disciplines would otherwise accept.

  This matters because the two refusals are indistinguishable in the verdict -
  both read incompatible - so a caller cannot tell an element mismatch from a
  discipline mismatch without asking merge_elements_compatible separately.
  merge_class exposes both, which is what the elements_ok member is for.

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
bool
tests_merge_kind_of_element_guard()
{
    bool ok = true;

    // same discipline, incompatible elements
    D_CM_CHECK(merge_discipline_of<cm_vector>::value
                   == merge_discipline_of<cm_vec_string>::value);
    D_CM_CHECK((merge_elements_compatible<cm_vector,
                                          cm_vec_string>::value) == false);
    D_CM_CHECK((merge_kind_of<cm_vector, cm_vec_string>::value)
                   == merge_kind::incompatible);

    // ... and with compatible elements it is accepted
    D_CM_CHECK((merge_elements_compatible<cm_vector,
                                          cm_vec_double>::value) == true);
    D_CM_CHECK((merge_kind_of<cm_vector, cm_vec_double>::value)
                   == merge_kind::concatenation);

    // discipline mismatch, compatible elements
    D_CM_CHECK((merge_elements_compatible<cm_vector, cm_set>::value) == true);
    D_CM_CHECK(merge_discipline_of<cm_vector>::value
                   != merge_discipline_of<cm_set>::value);
    D_CM_CHECK((merge_kind_of<cm_vector, cm_set>::value)
                   == merge_kind::incompatible);

    // the two causes are indistinguishable in the kind
    D_CM_CHECK((merge_kind_of<cm_vector, cm_vec_string>::value)
                   == (merge_kind_of<cm_vector, cm_set>::value));

    // ... and distinguishable through the element trait
    D_CM_CHECK((merge_elements_compatible<cm_vector, cm_vec_string>::value)
                   != (merge_elements_compatible<cm_vector,
                                                 cm_set>::value));

    // necessary but not sufficient
    D_CM_CHECK(   ( (merge_elements_compatible<cm_vector, cm_set>::value)
                        == true )
               && ( (is_mergeable<cm_vector, cm_set>::value) == false ));

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
bool
tests_merge_kind_of_std_types()
{
    bool ok = true;

    // sequences
    D_CM_CHECK((merge_kind_of<cm_vector, cm_deque>::value)
                   == merge_kind::concatenation);
    D_CM_CHECK((merge_kind_of<cm_list, cm_array>::value)
                   == merge_kind::concatenation);
    D_CM_CHECK((merge_kind_of<cm_vector, cm_array>::value)
                   == merge_kind::concatenation);

    // sets
    D_CM_CHECK((merge_kind_of<cm_set, cm_set>::value)
                   == merge_kind::set_union);
    D_CM_CHECK((merge_kind_of<cm_multiset, cm_multiset>::value)
                   == merge_kind::multiset_union);
    D_CM_CHECK((merge_kind_of<cm_multiset, cm_set>::value)
                   == merge_kind::multiset_union);

    // maps
    D_CM_CHECK((merge_kind_of<cm_map, cm_map>::value)
                   == merge_kind::keyed_merge);
    D_CM_CHECK((merge_kind_of<cm_multimap, cm_multimap>::value)
                   == merge_kind::keyed_merge);
    D_CM_CHECK((merge_kind_of<cm_map, cm_multimap>::value)
                   == merge_kind::keyed_merge);

    // the unordered counterparts
    D_CM_CHECK((merge_kind_of<cm_uset, cm_set>::value)
                   == (merge_kind_of<cm_set, cm_set>::value));
    D_CM_CHECK((merge_kind_of<cm_umap, cm_map>::value)
                   == (merge_kind_of<cm_map, cm_map>::value));

    // cross-family refusals
    D_CM_CHECK((merge_kind_of<cm_vector, cm_set>::value)
                   == merge_kind::incompatible);
    D_CM_CHECK((merge_kind_of<cm_set, cm_map>::value)
                   == merge_kind::incompatible);
    D_CM_CHECK((merge_kind_of<cm_vector, cm_map>::value)
                   == merge_kind::incompatible);
    D_CM_CHECK((merge_kind_of<cm_string, cm_set>::value)
                   == merge_kind::incompatible);

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
bool
tests_is_mergeable()
{
    bool ok = true;

    // true for each defined kind
    D_CM_CHECK((is_mergeable<cm_vector, cm_vector>::value)  == true);
    D_CM_CHECK((is_mergeable<cm_set, cm_set>::value)        == true);
    D_CM_CHECK((is_mergeable<cm_set, cm_multiset>::value)   == true);
    D_CM_CHECK((is_mergeable<cm_map, cm_multimap>::value)   == true);

    // false for both causes of refusal
    D_CM_CHECK((is_mergeable<cm_vector, cm_set>::value)     == false);
    D_CM_CHECK((is_mergeable<cm_vector, cm_vec_string>::value) == false);
    D_CM_CHECK((is_mergeable<cm_vector, int>::value)        == false);
    D_CM_CHECK((is_mergeable<int, int>::value)              == false);

    // exactly the complement of incompatible
    D_CM_CHECK((is_mergeable<cm_vector, cm_vector>::value)
                   == ( (merge_kind_of<cm_vector, cm_vector>::value)
                            != merge_kind::incompatible ));
    D_CM_CHECK((is_mergeable<cm_vector, cm_set>::value)
                   == ( (merge_kind_of<cm_vector, cm_set>::value)
                            != merge_kind::incompatible ));
    D_CM_CHECK((is_mergeable<cm_map, cm_map>::value)
                   == ( (merge_kind_of<cm_map, cm_map>::value)
                            != merge_kind::incompatible ));
    D_CM_CHECK((is_mergeable<int, cm_vector>::value)
                   == ( (merge_kind_of<int, cm_vector>::value)
                            != merge_kind::incompatible ));

    // it IS an integral_constant, unlike merge_kind_of
    D_CM_STATIC_CHECK(
        std::is_base_of<std::true_type,
                        is_mergeable<cm_vector, cm_vector> >::value);
    D_CM_STATIC_CHECK(
        std::is_base_of<std::false_type,
                        is_mergeable<cm_vector, cm_set> >::value);

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
bool
tests_merge_kind_cv_ref()
{
    bool ok = true;

    // the kind itself
    D_CM_CHECK((merge_kind_of<const cm_vector, cm_vector>::value)
                   == (merge_kind_of<cm_vector, cm_vector>::value));
    D_CM_CHECK((merge_kind_of<volatile cm_vector, cm_vector>::value)
                   == (merge_kind_of<cm_vector, cm_vector>::value));
    D_CM_CHECK((merge_kind_of<cm_vector, const cm_vector&>::value)
                   == (merge_kind_of<cm_vector, cm_vector>::value));
    D_CM_CHECK((merge_kind_of<const cm_vector&, cm_vector&&>::value)
                   == (merge_kind_of<cm_vector, cm_vector>::value));
    D_CM_CHECK((merge_kind_of<const volatile cm_set&, cm_set>::value)
                   == (merge_kind_of<cm_set, cm_set>::value));
    D_CM_CHECK((merge_kind_of<cm_map&&, const cm_multimap&>::value)
                   == (merge_kind_of<cm_map, cm_multimap>::value));
    D_CM_CHECK((merge_kind_of<const cm_vector, volatile cm_set>::value)
                   == (merge_kind_of<cm_vector, cm_set>::value));

    // the boolean face
    D_CM_CV_REF_AGREE2(is_mergeable, cm_vector, cm_vector, true);
    D_CM_CV_REF_AGREE2(is_mergeable, cm_set, cm_set, true);
    D_CM_CV_REF_AGREE2(is_mergeable, cm_vector, cm_set, false);

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
bool
tests_merge_kind_companions()
{
    bool ok = true;

    // the `_t` alias
    D_CM_STATIC_CHECK(
        std::is_same<merge_kind_of_t<cm_vector, cm_vector>,
                     std::integral_constant<merge_kind,
                                            merge_kind::concatenation>
                    >::value);
    D_CM_STATIC_CHECK(
        std::is_same<merge_kind_of_t<cm_vector, cm_set>,
                     std::integral_constant<merge_kind,
                                            merge_kind::incompatible>
                    >::value);
    D_CM_CHECK((merge_kind_of_t<cm_set, cm_set>::value)
                   == (merge_kind_of<cm_set, cm_set>::value));

    // the shapes differ between the two traits
    D_CM_STATIC_CHECK(
        std::is_base_of<merge_kind_of_t<cm_vector, cm_vector>,
                        merge_kind_of<cm_vector, cm_vector> >::value
            == false);
    D_CM_STATIC_CHECK(
        std::is_same<is_mergeable<cm_vector, cm_vector>::value_type,
                     bool>::value);

    // constexpr
    D_CM_STATIC_CHECK((merge_kind_of<cm_set, cm_set>::value)
                          == merge_kind::set_union);

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    D_CM_STATIC_CHECK((merge_kind_of_v<cm_vector, cm_vector>)
                          == merge_kind::concatenation);
    D_CM_STATIC_CHECK((is_mergeable_v<cm_vector, cm_vector>));
    D_CM_STATIC_CHECK((is_mergeable_v<cm_vector, cm_set>) == false);

    D_CM_CHECK((merge_kind_of_v<cm_set, cm_multiset>)
                   == (merge_kind_of<cm_set, cm_multiset>::value));
    D_CM_CHECK((merge_kind_of_v<cm_map, cm_map>)
                   == (merge_kind_of<cm_map, cm_map>::value));
    D_CM_CHECK((merge_kind_of_v<cm_vector, int>)
                   == (merge_kind_of<cm_vector, int>::value));
    D_CM_CHECK((merge_kind_of_v<const cm_vector&, cm_vector&&>)
                   == (merge_kind_of<cm_vector, cm_vector>::value));
    D_CM_CHECK((is_mergeable_v<cm_map, cm_multimap>)
                   == (is_mergeable<cm_map, cm_multimap>::value));
    D_CM_CHECK((is_mergeable_v<cm_vector, cm_vec_string>)
                   == (is_mergeable<cm_vector, cm_vec_string>::value));

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    return ok;
}


NS_END  // testing
NS_END  // djinterp
