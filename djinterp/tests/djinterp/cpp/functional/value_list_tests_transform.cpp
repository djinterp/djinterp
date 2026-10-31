/*******************************************************************************
* djinterp [cpp]                                  value_list_tests_transform.cpp
*
*
* path:      /tests/djinterp/cpp/functional/value_list_tests_transform.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test]  value_list_tests_transform.cpp
//   Section V -- transform: a unary carrier-callable leaf mapped over the list.

// std
#include <type_traits>
// djinterp
#include "value_list_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_transform_maps_every_element
  transform applies the leaf to each element and returns a new list -- the
  value-domain analog of dtuple's tuple_apply_all.
  Tests the following:
  - every element is mapped, none skipped, order preserved
  - a singleton and a longer list
*/
bool tests_transform_maps_every_element()
{
    bool ok = true;

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    static_assert(std::is_same<decltype(transform(DValueList<1, 2, 3>{},
                                                  DDoubleOp{})),
                               DValueList<2, 4, 6> >::value, "doubled");

    static_assert(std::is_same<decltype(transform(DValueList<5>{}, DDoubleOp{})),
                               DValueList<10> >::value, "singleton");

    static_assert(std::is_same<decltype(transform(DValueList<0, 1, 2, 3>{},
                                                  DAdd3Op{})),
                               DValueList<3, 4, 5, 6> >::value, "order kept");

    ok = ok && (value_list_at_v<1, decltype(transform(DValueList<1, 2, 3>{},
                                                      DDoubleOp{}))> == 4);
#endif

    return ok;
}


/*
tests_transform_empty
  Mapping over the empty list yields the empty list -- the pack expansion has
  nothing to expand, so the leaf is never even probed.
  Tests the following:
  - the empty list maps to the empty list
  - it does so even with a leaf that could not be applied to anything
*/
bool tests_transform_empty()
{
    bool ok = true;

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    static_assert(std::is_same<decltype(transform(DValueList<>{}, DDoubleOp{})),
                               DValueList<> >::value, "empty -> empty");
    static_assert(decltype(transform(DValueList<>{}, DDoubleOp{}))::size() == 0u,
                  "still empty");

    // the leaf is never probed, so even the never-defined one is fine here.
    static_assert(std::is_same<decltype(transform(DValueList<>{},
                                                  DDeclOnlyOp{})),
                               DValueList<> >::value, "nothing to probe");

    ok = ok && (decltype(transform(DValueList<>{}, DDoubleOp{}))::size() == 0u);
#endif

    return ok;
}


/*
tests_transform_law_identity
  The functor identity law: mapping the identity leaf changes nothing.
  Tests the following:
  - transform(l, id) is l, for a populated, an empty, and a heterogeneous list
*/
bool tests_transform_law_identity()
{
    bool ok = true;

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    static_assert(std::is_same<decltype(transform(DValueList<1, 2, 3>{},
                                                  DIdentOp{})),
                               DValueList<1, 2, 3> >::value, "identity");
    static_assert(std::is_same<decltype(transform(DValueList<>{}, DIdentOp{})),
                               DValueList<> >::value, "identity, empty");
    static_assert(std::is_same<decltype(transform(DValueList<1, 'x', true>{},
                                                  DIdentOp{})),
                               DValueList<1, 'x', true> >::value,
                  "identity, heterogeneous -- types survive");

    ok = ok && (std::is_same<decltype(transform(DValueList<1, 2>{},
                                                DIdentOp{})),
                             DValueList<1, 2> >::value);
#endif

    return ok;
}


/*
tests_transform_law_composition
  The functor composition law: mapping f then g is mapping (g . f) once.
  Tests the following:
  - transform(transform(l, f), g) == transform(l, g . f)
  - the composite really is applied in that order (f first), which a
    non-commutative pair of leaves pins
*/
bool tests_transform_law_composition()
{
    bool ok = true;

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    using list = DValueList<1, 2>;

    // double, then add three: (1*2)+3 = 5, (2*2)+3 = 7.
    static_assert(std::is_same<decltype(transform(transform(list{}, DDoubleOp{}),
                                                  DAdd3Op{})),
                               DValueList<5, 7> >::value, "f then g");

    // the same thing in one pass.
    static_assert(std::is_same<decltype(transform(list{}, DDoubleThenAdd3Op{})),
                               DValueList<5, 7> >::value, "g . f in one pass");

    static_assert(std::is_same<decltype(transform(transform(list{}, DDoubleOp{}),
                                                  DAdd3Op{})),
                               decltype(transform(list{},
                                                  DDoubleThenAdd3Op{}))
                              >::value, "composition law");

    // order matters: add three, then double, is a different list.
    static_assert(std::is_same<decltype(transform(transform(list{}, DAdd3Op{}),
                                                  DDoubleOp{})),
                               DValueList<8, 10> >::value, "g then f differs");

    ok = ok && (std::is_same<decltype(transform(transform(list{}, DDoubleOp{}),
                                                DAdd3Op{})),
                             DValueList<5, 7> >::value);
#endif

    return ok;
}


/*
tests_transform_changes_value_type
  The leaf may change the value's TYPE, not merely its value -- an int list can
  come back a bool list.
  Tests the following:
  - int -> bool through a predicate leaf
  - the resulting elements really are bools
*/
bool tests_transform_changes_value_type()
{
    bool ok = true;

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    using mapped = decltype(transform(DValueList<0, 1, 2, 3>{}, DGt1Op{}));

    static_assert(std::is_same<mapped,
                               DValueList<false, false, true, true> >::value,
                  "int -> bool");
    static_assert(std::is_same<decltype(value_list_at_v<0, mapped>),
                               const bool>::value, "the element is a bool");
    static_assert(mapped::size() == 4u, "length unchanged");

    ok = ok && (value_list_at_v<3, mapped> == true);
#endif

    return ok;
}


/*
tests_transform_unevaluated_probe
  transform needs only the leaf's RESULT TYPE, so it probes it in an UNEVALUATED
  context: decltype(declval<_Op&>()(val<V>))::value. A leaf that is merely
  DECLARED -- never defined, not const, not constexpr -- therefore drives it
  perfectly well. (fold, which really invokes its op, could not.)
  Tests the following:
  - a declared-but-never-defined leaf maps the list correctly
  - it composes with an ordinary leaf
  - the probe takes the op as a non-const lvalue, so a non-const operator() is
    fine
*/
bool tests_transform_unevaluated_probe()
{
    bool ok = true;

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    // DDeclOnlyOp has no definition anywhere -- and this links.
    static_assert(std::is_same<decltype(transform(DValueList<5>{},
                                                  DDeclOnlyOp{})),
                               DValueList<15> >::value, "V -> V * 3");
    static_assert(std::is_same<decltype(transform(DValueList<1, 2, 3>{},
                                                  DDeclOnlyOp{})),
                               DValueList<3, 6, 9> >::value, "over a longer list");

    // and it composes with an ordinary, defined leaf.
    static_assert(std::is_same<decltype(transform(transform(DValueList<1, 2>{},
                                                            DDeclOnlyOp{}),
                                                  DAdd3Op{})),
                               DValueList<6, 9> >::value, "composed");

    ok = ok && (decltype(transform(DValueList<5>{}, DDeclOnlyOp{}))::size()
                == 1u);
#endif

    return ok;
}


/*
tests_transform_heterogeneous
  A generic leaf maps a mixed list, each element entering at its own type.
  Tests the following:
  - a truthiness leaf over an int / char / bool / enum list
*/
bool tests_transform_heterogeneous()
{
    bool ok = true;

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    // 0 -> false; 'x' -> true; false -> false; color::red (== 1) -> true.
    using mapped = decltype(transform(
        DValueList<0, 'x', false, color::red>{}, DTruthyOp{}));

    static_assert(std::is_same<mapped,
                               DValueList<false, true, false, true> >::value,
                  "truthiness across four kinds");
    static_assert(mapped::size() == 4u, "length preserved");

    ok = ok && (value_list_at_v<1, mapped> == true);
#endif

    return ok;
}


/*
tests_transform_preserves_length
  transform is a map, not a filter: the result has exactly as many elements as the
  source, always.
  Tests the following:
  - the length is invariant across several lists and leaves
*/
bool tests_transform_preserves_length()
{
    bool ok = true;

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    static_assert(decltype(transform(DValueList<>{}, DDoubleOp{}))::size()
                  == DValueList<>::size(), "0");
    static_assert(decltype(transform(DValueList<1>{}, DDoubleOp{}))::size()
                  == DValueList<1>::size(), "1");
    static_assert(decltype(transform(DValueList<1, 2, 3>{},
                                     DDoubleOp{}))::size()
                  == DValueList<1, 2, 3>::size(), "3");
    static_assert(decltype(transform(DValueList<0, 1, 2, 3, 4, 5, 6, 7>{},
                                     DGt1Op{}))::size() == 8u, "8");

    // even a leaf that collapses every value to the same one keeps the length.
    static_assert(decltype(transform(DValueList<7, 7, 7>{},
                                     DTruthyOp{}))::size() == 3u,
                  "duplicates are not merged");

    ok = ok && (decltype(transform(DValueList<1, 2, 3>{},
                                   DDoubleOp{}))::size() == 3u);
#endif

    return ok;
}


NS_END  // testing
NS_END  // djinterp
