/*******************************************************************************
* djinterp [core]                  container_comparison_traits_tests_content.cpp
*
*
* path:      /tests/djinterp/core/container/traits/container_comparison_traits_tests_content.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// std
#include <array>
#include <cstring>
#include <map>
#include <set>
#include <type_traits>
#include <vector>
// djinterp
#include "container_comparison_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_content_level_enum
  The content_level enum.
  Tests the following:
  - scoped, four... five enumerators, all distinct
  - the VALUES are 0..4, IN COARSEST-TO-FINEST ORDER.  That is not cosmetic:
    content_level_rank is a bare cast, and content_level_coarser is a comparison
    of those casts, so the enum's DECLARATION ORDER *is* the hierarchy.  Reorder
    the enumerators and `coarser` silently starts returning the finer rung, with
    no other line in the header needing to change and nothing to warn you
*/
bool
tests_content_level_enum()
{
    bool ok = true;

    using underlying = std::underlying_type<content_level>::type;

    D_CCM_CHECK(std::is_enum<content_level>::value);
    D_CCM_CHECK(!std::is_convertible<content_level, underlying>::value);

    // coarsest to finest, and the order IS the semantics
    D_CCM_CHECK(static_cast<int>(content_level::none) == 0);
    D_CCM_CHECK(static_cast<int>(content_level::set) == 1);
    D_CCM_CHECK(static_cast<int>(content_level::bag) == 2);
    D_CCM_CHECK(static_cast<int>(content_level::seq) == 3);
    D_CCM_CHECK(static_cast<int>(content_level::str) == 4);

    // the nesting the formal frontier equalities describe: =str refines =seq
    // refines =bag refines =set
    D_CCM_CHECK(content_level_rank(content_level::none) <
                content_level_rank(content_level::set));
    D_CCM_CHECK(content_level_rank(content_level::set) <
                content_level_rank(content_level::bag));
    D_CCM_CHECK(content_level_rank(content_level::bag) <
                content_level_rank(content_level::seq));
    D_CCM_CHECK(content_level_rank(content_level::seq) <
                content_level_rank(content_level::str));

    return ok;
}

/*
tests_content_level_name
  content_level_name - the stable spelling.
  Tests the following:
  - each of the five rungs maps to its own spelling
  - the function is constexpr and noexcept
  - an OUT-OF-RANGE value returns "str"

  THE FINDING.  The chain tests none, set, bag and seq, and falls through to
  "str".  So a content_level nobody can interpret reads as the FINEST rung in the
  hierarchy - the one that asserts the most.

  That is the wrong default, and it is the wrong default in the dangerous
  direction.  order_relation_name has the same shape and falls through to
  "incomparable", which is the safe end of ITS scale: a value you cannot interpret
  should read as "I cannot relate these".  The analogous safe end here is `none` -
  "not a container / indeterminate" - and the chain returns the opposite.

  Reachable only by an explicit cast today, so not a live bug.  But the fix is to
  move one arm:

      : _l == content_level::str ? "str" : "none"

  and the checks below pin the current behaviour so that doing so is a visible
  change rather than a silent one.
*/
bool
tests_content_level_name()
{
    bool ok = true;

    D_CCM_CHECK(std::strcmp(content_level_name(content_level::none),
                            "none") == 0);
    D_CCM_CHECK(std::strcmp(content_level_name(content_level::set),
                            "set") == 0);
    D_CCM_CHECK(std::strcmp(content_level_name(content_level::bag),
                            "bag") == 0);
    D_CCM_CHECK(std::strcmp(content_level_name(content_level::seq),
                            "seq") == 0);
    D_CCM_CHECK(std::strcmp(content_level_name(content_level::str),
                            "str") == 0);

    // constexpr and noexcept
    D_CONSTEXPR const char* bag_name = content_level_name(content_level::bag);

    D_CCM_CHECK(std::strcmp(bag_name, "bag") == 0);
    D_CCM_CHECK(noexcept(content_level_name(content_level::seq)));

    // THE FALL-THROUGH: an uninterpretable rung reads as the FINEST one
    const content_level out_of_range = static_cast<content_level>(42);

    D_CCM_CHECK(content_level_name(out_of_range) != nullptr);
    D_CCM_CHECK(std::strcmp(content_level_name(out_of_range), "str") == 0);
    D_CCM_CHECK(std::strcmp(content_level_name(out_of_range), "none") != 0);

    // ...whereas the analogous chain on the other enum falls through to the SAFE
    // end of its scale.  The two are inconsistent, and only one of them can be
    // right
    D_CCM_CHECK(std::strcmp(
                    order_relation_name(static_cast<order_relation>(42)),
                    "incomparable") == 0);

    return ok;
}

/*
tests_content_level_rank
  content_level_rank - fineness as an integer.
  Tests the following:
  - it is exactly the enum's cast, for all five rungs
  - none is the coarsest (lowest) and str the finest (highest)
  - it is constexpr and noexcept
*/
bool
tests_content_level_rank()
{
    bool ok = true;

    D_CCM_CHECK(content_level_rank(content_level::none) == 0);
    D_CCM_CHECK(content_level_rank(content_level::set) == 1);
    D_CCM_CHECK(content_level_rank(content_level::bag) == 2);
    D_CCM_CHECK(content_level_rank(content_level::seq) == 3);
    D_CCM_CHECK(content_level_rank(content_level::str) == 4);

    // it IS the cast, restated - so a reordered enum is a reordered hierarchy
    D_CCM_CHECK(content_level_rank(content_level::bag) ==
                static_cast<int>(content_level::bag));

    D_CONSTEXPR int str_rank = content_level_rank(content_level::str);

    D_CCM_CHECK(str_rank == 4);
    D_CCM_CHECK(noexcept(content_level_rank(content_level::set)));

    return ok;
}

/*
tests_content_level_coarser
  content_level_coarser - the rung at which two types compare.

  The formal rule is "the coarser of the two native levels": a sequence and a set
  can only be compared as sets, because that is the finest equality both of them
  support.  So `coarser` is a MINIMUM over the rank, and a `max` here would be the
  single most damaging one-character bug in the module - it would claim two
  containers compare at a level neither of them supports.

  Tests the following:
  - all 25 ordered pairs of the five rungs
  - it is a genuine minimum: idempotent, commutative, associative
  - `none` is ABSORBING - anything compared with a non-container compares at none,
    which is to say not at all
  - `str` is the IDENTITY - the finest rung asserts nothing extra about the other
    operand
*/
bool
tests_content_level_coarser()
{
    bool ok = true;

    D_CONSTEXPR content_level all[5] = { content_level::none,
                                         content_level::set,
                                         content_level::bag,
                                         content_level::seq,
                                         content_level::str };

    // the coarser IS the minimum by rank - all 25 pairs
    for (int i = 0; i < 5; ++i)
    {
        for (int j = 0; j < 5; ++j)
        {
            const content_level a = all[i];
            const content_level b = all[j];
            const content_level expected =
                ( content_level_rank(a) <= content_level_rank(b) ) ? a : b;

            D_CCM_CHECK(content_level_coarser(a, b) == expected);

            // commutative
            D_CCM_CHECK(content_level_coarser(a, b) ==
                        content_level_coarser(b, a));
        }

        // idempotent
        D_CCM_CHECK(content_level_coarser(all[i], all[i]) == all[i]);

        // none is absorbing: nothing compares with a non-container
        D_CCM_CHECK(content_level_coarser(content_level::none, all[i]) ==
                    content_level::none);

        // str is the identity
        D_CCM_CHECK(content_level_coarser(content_level::str, all[i]) ==
                    all[i]);
    }

    // the cases a reader would want to see spelled out
    D_CCM_LEVEL(content_level::set,
                content_level_coarser(content_level::set, content_level::seq));
    D_CCM_LEVEL(content_level::seq,
                content_level_coarser(content_level::str, content_level::seq));
    D_CCM_LEVEL(content_level::bag,
                content_level_coarser(content_level::bag, content_level::str));
    D_CCM_LEVEL(content_level::none,
                content_level_coarser(content_level::none, content_level::str));

    D_CCM_CHECK(noexcept(content_level_coarser(content_level::set,
                                               content_level::seq)));

    return ok;
}

/*
tests_native_content_level_rungs
  native_content_level_of - the rung at which a type NATIVELY compares.  All five,
  one fixture each.
  Tests the following:
  - a non-container is `none`
  - a unique discipline (m = 1) is `set`
  - a multiset is `bag` - BOTH the bounded and the unbounded kind, which is the
    point of having two fixtures: their multiplicity RANKS differ (1 and 2) and
    their content rungs do not.  The discipline order and the content hierarchy
    are different orders over the same data, and a helper that reused one for the
    other would collapse them
  - a sequence is `seq`
  - a nested container is `str`
*/
bool
tests_native_content_level_rungs()
{
    bool ok = true;

    D_CCM_LEVEL(content_level::none,
                native_content_level_of<not_a_container>::value);
    D_CCM_LEVEL(content_level::set,
                native_content_level_of<unique_seq>::value);
    D_CCM_LEVEL(content_level::bag,
                native_content_level_of<bounded_bag>::value);
    D_CCM_LEVEL(content_level::bag,
                native_content_level_of<unbounded_bag>::value);
    D_CCM_LEVEL(content_level::seq,
                native_content_level_of<runtime_seq>::value);
    D_CCM_LEVEL(content_level::str,
                native_content_level_of<nested_seq>::value);

    // the two multiset kinds share a RUNG and not a RANK
    D_CCM_CHECK(native_content_level_of<bounded_bag>::value ==
                native_content_level_of<unbounded_bag>::value);
    D_CCM_CHECK(multiplicity_kind_rank(overlay_of<bounded_bag>::mult) !=
                multiplicity_kind_rank(overlay_of<unbounded_bag>::mult));

    // and a non-container really is one
    D_CCM_LEVEL(content_level::none, native_content_level_of<int>::value);
    D_CCM_LEVEL(content_level::none, native_content_level_of<void>::value);
    D_CCM_LEVEL(content_level::none,
                native_content_level_of<fixtures::empty>::value);

    // a countable but NON-traversable type is still a container on this axis -
    // the content rung is about the discipline, not about the interface
    D_CCM_LEVEL(content_level::seq, native_content_level_of<not_iterable>::value);

    return ok;
}

/*
tests_native_content_level_precedence
  THE PRECEDENCE RULE.  A nested container compares STRUCTURALLY, whatever its
  multiplicity.

  native_content_level_helper reads the structure verdict FIRST and only falls
  through to the multiplicity kind for a flat container.  So a nested container
  with a unique discipline is `str`, not `set` - and a helper that read mu first
  would return `set`, and would be wrong in precisely the case the structural rung
  exists for.

  Tests the following:
  - a nested container with m = 1 is `str`, not `set`
  - it really IS both nested and unique, so the check is about precedence and not
    about absence
  - a flat container with the same multiplicity is `set`, so the difference is the
    nesting and nothing else
*/
bool
tests_native_content_level_precedence()
{
    bool ok = true;

    // it is both
    D_CCM_CHECK(is_hierarchical_container<hierarchical_unique>::value);
    D_CCM_CHECK(overlay_of<hierarchical_unique>::mult ==
                multiplicity_kind::unique);

    // ...and the structure wins
    D_CCM_LEVEL(content_level::str,
                native_content_level_of<hierarchical_unique>::value);

    // the flat container with the SAME multiplicity is `set`, so the nesting is
    // the only difference
    D_CCM_CHECK(!is_hierarchical_container<unique_seq>::value);
    D_CCM_CHECK(overlay_of<unique_seq>::mult == multiplicity_kind::unique);
    D_CCM_LEVEL(content_level::set, native_content_level_of<unique_seq>::value);

    // ...and the nested SEQUENCE is str as well, so `str` is about the nesting
    // and not about the multiplicity at all
    D_CCM_CHECK(is_hierarchical_container<nested_seq>::value);
    D_CCM_CHECK(overlay_of<nested_seq>::mult == multiplicity_kind::sequence);
    D_CCM_LEVEL(content_level::str, native_content_level_of<nested_seq>::value);

    return ok;
}

/*
tests_native_content_level_value_companion
  native_content_level_of_v.
  Tests the following:
  - the shorthand carries the same value as the trait, on every rung
  - it is typed content_level, not int - so it cannot be folded into an arithmetic
    expression by accident, which is exactly what content_level_rank is for

  NOTE the shape: native_content_level_of exposes a `value` and a `_v` and NO
  `::type`.  Its neighbours do not agree with it - multiplicity_kind_of supplies a
  `::type` carrier, and so does capacity_bound_of in bounded_container_traits.hpp.
  A caller who wants to tag-dispatch on a content rung has to build the carrier by
  hand for this one trait and not for the others.
*/
bool
tests_native_content_level_value_companion()
{
    bool ok = true;

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    D_CCM_CHECK(native_content_level_of_v<runtime_seq> ==
                native_content_level_of<runtime_seq>::value);
    D_CCM_CHECK(native_content_level_of_v<unique_seq> ==
                native_content_level_of<unique_seq>::value);
    D_CCM_CHECK(native_content_level_of_v<nested_seq> ==
                native_content_level_of<nested_seq>::value);
    D_CCM_CHECK(native_content_level_of_v<not_a_container> ==
                native_content_level_of<not_a_container>::value);

    D_CCM_CHECK(native_content_level_of_v<runtime_seq> == content_level::seq);
    D_CCM_CHECK(native_content_level_of_v<unique_seq> == content_level::set);
    D_CCM_CHECK(native_content_level_of_v<bounded_bag> == content_level::bag);
    D_CCM_CHECK(native_content_level_of_v<nested_seq> == content_level::str);
    D_CCM_CHECK(native_content_level_of_v<not_a_container> ==
                content_level::none);

    D_CCM_CHECK((std::is_same<decltype(native_content_level_of_v<runtime_seq>),
                              const content_level>::value));

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    return ok;
}


NS_END  // testing
NS_END  // djinterp
